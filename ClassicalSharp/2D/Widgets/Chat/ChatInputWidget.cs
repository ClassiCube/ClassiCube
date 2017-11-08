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
			gfx.Texturing = false;
			int y = Y, x = X;
			
			for (int i = 0; i < lineSizes.Length; i++) {
				if (i > 0 && lineSizes[i].Height == 0) break;
				bool caretAtEnd = (caretY == i) && (caretX == MaxCharsPerLine || caret == -1);
				int drawWidth = lineSizes[i].Width + (caretAtEnd ? (int)caretTex.Width : 0);
				// Cover whole window width to match original classic behaviour
				if (game.PureClassic)
					drawWidth = Math.Max(drawWidth, game.Width - X * 4);
				
				gfx.Draw2DQuad(x, y, drawWidth + Padding * 2, prefixHeight, backColour);
				y += lineSizes[i].Height;
			}
			
			gfx.Texturing = true;
			inputTex.Render(gfx);
			RenderCaret(delta);
		}

		
		public override void EnterInput() {
			if (!Text.Empty) {
				// Don't want trailing spaces in output message
				string text = new String(Text.value, 0, Text.TextLength);
				game.Chat.Send(text);
			}
			
			originalText = null;
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
			
			game.Chat.Add(null, MessageType.ClientStatus2);
			game.Chat.Add(null, MessageType.ClientStatus3);
			base.EnterInput();
		}
		
		#region Input handling
		
		public override bool HandlesKeyPress(char key) {
			Append(key); return true;
		}
		
		public override bool HandlesKeyDown(Key key) {
			bool controlDown = ControlDown();
			
			if (key == Key.Tab) { TabKey(); return true; }
			if (key == Key.Up) { UpKey(controlDown); return true; }
			if (key == Key.Down) { DownKey(controlDown); return true; }
			
			return base.HandlesKeyDown(key);
		}
		
		void UpKey(bool controlDown) {
			if (controlDown) {
				int pos = caret == -1 ? Text.Length : caret;
				if (pos < MaxCharsPerLine) return;
				
				caret = pos - MaxCharsPerLine;
				UpdateCaret();
				return;
			}
			
			if (typingLogPos == game.Chat.InputLog.Count)
				originalText = Text.ToString();
			if (game.Chat.InputLog.Count > 0) {
				typingLogPos--;
				if (typingLogPos < 0) typingLogPos = 0;
				
				Text.Clear();
				Text.Append(0, game.Chat.InputLog[typingLogPos]);
				caret = -1;
				Recreate();
			}
		}
		
		void DownKey(bool controlDown) {
			if (controlDown) {
				if (caret == -1 || caret >= (UsedLines - 1) * MaxCharsPerLine) return;
				caret += MaxCharsPerLine;
				UpdateCaret();
				return;
			}
			
			if (game.Chat.InputLog.Count > 0) {
				typingLogPos++;
				Text.Clear();
				if (typingLogPos >= game.Chat.InputLog.Count) {
					typingLogPos = game.Chat.InputLog.Count;
					if (originalText != null)
						Text.Append(0, originalText);
				} else {
					Text.Append(0, game.Chat.InputLog[typingLogPos]);
				}
				caret = -1;
				Recreate();
			}
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
			
			TabListEntry[] entries = game.TabList.Entries;
			for (int i = 0; i < EntityList.MaxCount; i++) {
				if (entries[i] == null) continue;
				
				string rawName = entries[i].PlayerName;
				string name = Utils.StripColours(rawName);
				if (name.StartsWith(part, StringComparison.OrdinalIgnoreCase))
					matches.Add(name);
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
				int index = 0;
				sb.Append(ref index, "&e");
				sb.AppendNum(ref index, matches.Count);
				sb.Append(ref index, " matching names: ");
				
				for (int i = 0; i < matches.Count; i++) {
					string match = matches[i];
					if ((sb.Length + match.Length + 1) > sb.Capacity) break;
					sb.Append(ref index, match);
					sb.Append(ref index, ' ');
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