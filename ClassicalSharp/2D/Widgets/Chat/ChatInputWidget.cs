// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Entities;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Widgets {
	public sealed class ChatInputWidget : InputWidget {

		public ChatInputWidget(Game game, Font font) : base(game, font, "> ", 3) {
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
			ShowCaret = true;
			Padding = 5;
		}

		static FastColour backColour = new FastColour(0, 0, 0, 127);
		int typingLogPos;
		string originalText;
		
		public override int UsedLines {
			get { return !game.ClassicMode && game.Server.SupportsPartialMessages ? 3 : 1; }
		}
		
		public override void Render(double delta) {
			game.Graphics.Texturing = false;
			int y = Y, x = X;
			
			for (int i = 0; i < lineSizes.Length; i++) {
				if (i > 0 && lineSizes[i].Height == 0) break;
				bool caretAtEnd = (caretY == i) && (caretX == MaxCharsPerLine || caret == -1);
				int drawWidth = lineSizes[i].Width + (caretAtEnd ? (int)caretTex.Width : 0);
				// Cover whole window width to match original classic behaviour
				if (game.PureClassic)
					drawWidth = Math.Max(drawWidth, game.Width - X * 4);
				
				game.Graphics.Draw2DQuad(x, y, drawWidth + Padding * 2, prefixHeight, backColour);
				y += lineSizes[i].Height;
			}
			
			game.Graphics.Texturing = true;
			inputTex.Render(game.Graphics);
			RenderCaret(delta);
		}

		
		public override void EnterInput() {
			// Don't want trailing spaces in output message
			int length = Text.Length;
			while (length > 0 && Text.value[length - 1] == ' ') { length--; }			
			if (length > 0) {
				string text = new String(Text.value, 0, length);
				game.Chat.Send(text);
			}
			
			originalText = null;
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
			
			game.Chat.Add(null, MessageType.ClientStatus2);
			game.Chat.Add(null, MessageType.ClientStatus3);
			base.EnterInput();
		}
		
		#region Input handling
		
		public override bool HandlesKeyDown(Key key) {
			if (key == Key.Tab) { TabKey(); return true; }
			if (key == Key.Up) { UpKey(); return true; }
			if (key == Key.Down) { DownKey(); return true; }			
			return base.HandlesKeyDown(key);
		}
		
		void UpKey() {
			if (ControlDown()) {
				int pos = caret == -1 ? Text.Length : caret;
				if (pos < MaxCharsPerLine) return;
				
				caret = pos - MaxCharsPerLine;
				UpdateCaret();
				return;
			}
			
			if (typingLogPos == game.Chat.InputLog.Count) {
				originalText = Text.ToString();
			}
			
			if (game.Chat.InputLog.Count == 0) return;
			typingLogPos--;					
			Text.Clear();
			
			if (typingLogPos < 0) typingLogPos = 0;	
			Text.Set(game.Chat.InputLog[typingLogPos]);
			
			caret = -1;
			Recreate();
		}
		
		void DownKey() {
			if (ControlDown()) {
				if (caret == -1 || caret >= (UsedLines - 1) * MaxCharsPerLine) return;
				caret += MaxCharsPerLine;
				UpdateCaret();
				return;
			}
			
			if (game.Chat.InputLog.Count == 0) return;
			typingLogPos++;
			Text.Clear();
				
			if (typingLogPos >= game.Chat.InputLog.Count) {
				typingLogPos = game.Chat.InputLog.Count;
				if (originalText != null) Text.Set(originalText);
			} else {
				Text.Set(game.Chat.InputLog[typingLogPos]);
			}
			
			caret = -1;
			Recreate();
		}
		
		void TabKey() {
			int pos = caret == -1 ? Text.Length - 1 : caret;
			int start = pos;
			char[] value = Text.value;
			
			while (start >= 0 && IsNameChar(value[start]))
				start--;
			start++;
			if (pos < 0 || start > pos) return;
			
			string part = new String(value, start, pos + 1 - start);
			List<string> matches = new List<string>();
			game.Chat.Add(null, MessageType.ClientStatus3);
			
			TabListEntry[] entries = TabList.Entries;
			for (int i = 0; i < EntityList.MaxCount; i++) {
				if (entries[i] == null) continue;
				string name = entries[i].PlayerName;
				if (Utils.CaselessStarts(name, part)) matches.Add(name);
			}
			
			if (matches.Count == 1) {
				if (caret == -1) pos++;
				int len = pos - start;
				for (int i = 0; i < len; i++)
					Text.DeleteAt(start);
				if (caret != -1) caret -= len;
				Append(matches[0]);
			} else if (matches.Count > 1) {
				StringBuffer sb = new StringBuffer(Utils.StringLength);
				sb.Append("&e");
				sb.AppendNum(matches.Count);
				sb.Append(" matching names: ");
				
				for (int i = 0; i < matches.Count; i++) {
					string match = matches[i];
					if ((sb.Length + match.Length + 1) > sb.Capacity) break;
					sb.Append(match);
					sb.Append(' ');
				}
				game.Chat.Add(sb.ToString(), MessageType.ClientStatus3);
			}
		}
		
		bool IsNameChar(char c) {
			return c == '_' || c == '.' || (c >= '0' && c <= '9')
				|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
		}
		
		#endregion
	}
}