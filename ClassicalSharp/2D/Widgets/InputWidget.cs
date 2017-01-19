// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Widgets {
	public abstract class InputWidget : Widget {
		
		public InputWidget(Game game, Font font) : base(game) {
			Text = new WrappableStringBuffer(Utils.StringLength * MaxLines);
			lines = new string[MaxLines];
			lineSizes = new Size[MaxLines];
			
			DrawTextArgs args = new DrawTextArgs("_", font, true);
			caretTex = game.Drawer2D.MakeChatTextTexture(ref args, 0, 0);
			caretTex.Width = (short)((caretTex.Width * 3) / 4);
			caretWidth = caretTex.Width; caretHeight = caretTex.Height;
			this.font = font;
			
			if (Prefix == null) return;
			args = new DrawTextArgs(Prefix, font, true);
			Size size = game.Drawer2D.MeasureChatSize(ref args);
			prefixWidth = Width = size.Width;
			prefixHeight = Height = size.Height;
		}
		
		public InputWidget SetLocation(Anchor horAnchor, Anchor verAnchor, int xOffset, int yOffset) {
			HorizontalAnchor = horAnchor; VerticalAnchor = verAnchor;
			XOffset = xOffset; YOffset = yOffset;
			CalculatePosition();
			return this;
		}
		
		protected int caret = -1;
		protected Texture inputTex, caretTex, prefixTex;
		protected readonly Font font;
		protected int caretWidth, caretHeight, prefixWidth, prefixHeight;
		protected FastColour caretColour;
		
		/// <summary> The raw text entered. </summary>
		/// <remarks> You should Append() to add more text, as that also updates the caret position and texture. </remarks>
		public WrappableStringBuffer Text;
		
		/// <summary> The maximum number of lines that may be entered. </summary>
		public abstract int MaxLines { get; }
		
		/// <summary> The maximum number of characters that can fit on one line. </summary>
		public abstract int MaxCharsPerLine { get; }
		
		/// <summary> The prefix string that is always shown before the input text. Can be null. </summary>
		public abstract string Prefix { get; }
		
		/// <summary> The horizontal offset (in pixels) from the start of the box background
		/// to the beginning of the input texture. </summary>
		public abstract int Padding { get; }
		
		/// <summary> Whether a caret should be drawn at the position characters 
		/// are inserted/deleted from the input text. </summary>
		public bool ShowCaret;
		
		protected string[] lines; // raw text of each line
		protected Size[] lineSizes; // size of each line in pixels
		protected int caretCol, caretRow; // coordinates of caret
		protected double caretAccumulator;
		
		public override void Init() {
			if (lines.Length > 1) {
				Text.WordWrap(game.Drawer2D, lines, MaxCharsPerLine);
			} else {
				lines[0] = Text.ToString();
			}
			
			CalculateLineSizes();
			RemakeTexture();
			UpdateCaret();
		}
		
		public override void Dispose() {
			gfx.DeleteTexture(ref inputTex);
		}
		
		public void DisposeFully() {
			Dispose();
			gfx.DeleteTexture(ref caretTex);
			gfx.DeleteTexture(ref prefixTex);
		}

		public override void CalculatePosition() {
			int oldX = X, oldY = Y;
			base.CalculatePosition();
			
			caretTex.X1 += X - oldX; caretTex.Y1 += Y - oldY;
			inputTex.X1 += X - oldX; inputTex.Y1 += Y - oldY;
		}
		
		
		/// <summary> Calculates the sizes of each line in the text buffer. </summary>
		public void CalculateLineSizes() {
			for (int y = 0; y < lineSizes.Length; y++)
				lineSizes[y] = Size.Empty;
			lineSizes[0].Width = prefixWidth;
			
			DrawTextArgs args = new DrawTextArgs(null, font, true);
			for (int y = 0; y < MaxLines; y++) {
				args.Text = lines[y];
				lineSizes[y] += game.Drawer2D.MeasureChatSize(ref args);
			}
			if (lineSizes[0].Height == 0) lineSizes[0].Height = prefixHeight;
		}
		
		/// <summary> Calculates the location and size of the caret character </summary>
		public void UpdateCaret() {
			if (caret >= Text.Length) caret = -1;
			Text.GetCoords(caret, lines, out caretCol, out caretRow);
			DrawTextArgs args = new DrawTextArgs(null, font, true);
			IDrawer2D drawer = game.Drawer2D;
			caretAccumulator = 0;

			if (caretCol == MaxCharsPerLine) {
				caretTex.X1 = X + Padding + lineSizes[caretRow].Width;
				caretColour = FastColour.Yellow;
				caretTex.Width = (short)caretWidth;
			} else {
				args.Text = lines[caretRow].Substring(0, caretCol);
				Size trimmedSize = drawer.MeasureChatSize(ref args);
				if (caretRow == 0) trimmedSize.Width += prefixWidth;

				caretTex.X1 = X + Padding + trimmedSize.Width;
				caretColour = FastColour.Scale(FastColour.White, 0.8f);
				
				string line = lines[caretRow];
				if (caretCol < line.Length) {
					args.Text = new String(line[caretCol], 1);
					caretTex.Width = (short)drawer.MeasureChatSize(ref args).Width;
				} else {
					caretTex.Width = (short)caretWidth;
				}
			}
			caretTex.Y1 = lineSizes[0].Height * caretRow + inputTex.Y1 + 2;
			
			// Update the colour of the caret
			char code = GetLastColour(caretCol, caretRow);
			if (code != '\0') caretColour = drawer.Colours[code];
		}
		
		protected void RenderCaret(double delta) {
			if (!ShowCaret) return;
			
			caretAccumulator += delta;
			if ((caretAccumulator % 1) < 0.5)
				caretTex.Render(gfx, caretColour);
		}
		
		/// <summary> Remakes the raw texture containg all the chat lines. </summary>
		/// <remarks> Also updates the dimensions of the widget. </remarks>
		public virtual void RemakeTexture() {
			int totalHeight = 0, maxWidth = 0;
			for (int i = 0; i < MaxLines; i++) {
				totalHeight += lineSizes[i].Height;
				maxWidth = Math.Max(maxWidth, lineSizes[i].Width);
			}
			Size size = new Size(maxWidth, totalHeight);
			caretAccumulator = 0;
			
			int realHeight = 0;
			using (Bitmap bmp = IDrawer2D.CreatePow2Bitmap(size))
				using (IDrawer2D drawer = game.Drawer2D)
			{
				drawer.SetBitmap(bmp);
				DrawTextArgs args = new DrawTextArgs(null, font, true);
				if (Prefix != null) {
					args.Text = Prefix;
					drawer.DrawChatText(ref args, 0, 0);
				}
				
				for (int i = 0; i < lines.Length; i++) {
					if (lines[i] == null) break;
					args.Text = lines[i];
					char lastCol = GetLastColour(0, i);
					if (!IDrawer2D.IsWhiteColour(lastCol))
						args.Text = "&" + lastCol + args.Text;
					
					int offset = i == 0 ? prefixWidth : 0;
					drawer.DrawChatText(ref args, offset, realHeight);
					realHeight += lineSizes[i].Height;
				}
				inputTex = drawer.Make2DTexture(bmp, size, 0, 0);
			}
			
			Width = size.Width;
			Height = realHeight == 0 ? prefixHeight : realHeight;
			CalculatePosition();
			inputTex.X1 = X + Padding; inputTex.Y1 = Y;
		}
		
		protected char GetLastColour(int indexX, int indexY) {
			int x = indexX;
			IDrawer2D drawer = game.Drawer2D;
			for (int y = indexY; y >= 0; y--) {
				string part = lines[y];
				char code = drawer.LastColour(part, x);
				if (code != '\0') return code;
				if (y > 0) x = lines[y - 1].Length;
			}
			return '\0';
		}

		
		/// <summary> Invoked when the user presses enter. </summary>
		public virtual void EnterInput() {
			Clear();
			Height = prefixHeight;
		}
		
		
		/// <summary> Clears all the characters from the text buffer </summary>
		/// <remarks> Deletes the native texture. </remarks>
		public void Clear() {
			Text.Clear();
			for (int i = 0; i < lines.Length; i++)
				lines[i] = null;
			
			caret = -1;
			Dispose();
		}

		/// <summary> Appends a sequence of characters to current text buffer. </summary>
		/// <remarks> Potentially recreates the native texture. </remarks>
		public void Append(string text) {
			int appended = 0;
			foreach (char c in text) {
				if (TryAppendChar(c)) appended++;
			}
			
			if (appended == 0) return;
			Recreate();
		}
		
		/// <summary> Appends a single character to current text buffer. </summary>
		/// <remarks> Potentially recreates the native texture. </remarks>
		public void Append(char c) {
			if (!TryAppendChar(c)) return;
			Recreate();
		}
		
		
		bool TryAppendChar(char c) {
			int totalChars = MaxCharsPerLine * lines.Length;
			if (Text.Length == totalChars) return false;
			if (!AllowedChar(c)) return false;
			
			AppendChar(c);
			return true;
		}
		
		protected virtual bool AllowedChar(char c) {
			return Utils.IsValidInputChar(c, game);
		}
		
		protected void AppendChar(char c) {
			if (caret == -1) {
				Text.InsertAt(Text.Length, c);
			} else {
				Text.InsertAt(caret, c);
				caret++;
				if (caret >= Text.Length) caret = -1;
			}
		}
		
		protected void DeleteChar() {
			if (Text.Length == 0) return;
			
			if (caret == -1) {
				Text.DeleteAt(Text.Length - 1);
			} else if (caret > 0) {
				caret--;
				Text.DeleteAt(caret);
			}
		}
		
		
		#region Input handling
		
		protected bool ControlDown() {
			return OpenTK.Configuration.RunningOnMacOS ?
				(game.IsKeyDown(Key.WinLeft) || game.IsKeyDown(Key.WinRight))
				: (game.IsKeyDown(Key.ControlLeft) || game.IsKeyDown(Key.ControlRight));
		}
		
		public override bool HandlesKeyPress(char key) {
			if (!game.HideGui) Append(key);
			return true;
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (game.HideGui) return key < Key.F1 || key > Key.F35;
			bool clipboardDown = ControlDown();
			
			if (key == Key.Left) LeftKey(clipboardDown);
			else if (key == Key.Right) RightKey(clipboardDown);
			else if (key == Key.BackSpace) BackspaceKey(clipboardDown);
			else if (key == Key.Delete) DeleteKey();
			else if (key == Key.Home) HomeKey();
			else if (key == Key.End) EndKey();
			else if (clipboardDown && !OtherKey(key)) return false;
			
			return true;
		}
		
		public override bool HandlesKeyUp(Key key) { return true; }
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			if (button == MouseButton.Left)
				SetCaretToCursor(mouseX, mouseY);
			return true;
		}
		
		
		void BackspaceKey(bool controlDown) {
			if (controlDown) {
				if (caret == -1) caret = Text.Length - 1;
				int len = Text.GetBackLength(caret);
				if (len == 0) return;
				
				caret -= len;
				if (caret < 0) caret = 0;
				for (int i = 0; i <= len; i++)
					Text.DeleteAt(caret);
				
				if (caret >= Text.Length) caret = -1;
				if (caret == -1 &&  Text.Length > 0) {
					Text.value[Text.Length] = ' ';
				} else if (caret >= 0 && Text.value[caret] != ' ') {
					Text.InsertAt(caret, ' ');
				}
				Recreate();
			} else if (!Text.Empty && caret != 0) {
				int index = caret == -1 ? Text.Length - 1 : caret;
				if (CheckColour(index - 1)) {
					DeleteChar(); // backspace XYZ%e to XYZ
				} else if (CheckColour(index - 2)) {
					DeleteChar(); DeleteChar(); // backspace XYZ%eH to XYZ
				}
				
				DeleteChar();
				Recreate();
			}
		}

		bool CheckColour(int index) {
			if (index < 0) return false;
			char code = Text.value[index], col = Text.value[index + 1];
			return (code == '%' || code == '&') && game.Drawer2D.ValidColour(col);
		}
		
		void DeleteKey() {
			if (!Text.Empty && caret != -1) {
				Text.DeleteAt(caret);
				if (caret >= Text.Length) caret = -1;
				Recreate();
			}
		}
		
		void LeftKey(bool controlDown) {
			if (controlDown) {
				if (caret == -1)
					caret = Text.Length - 1;
				caret -= Text.GetBackLength(caret);
				UpdateCaret();
				return;
			}
			
			if (!Text.Empty) {
				if (caret == -1) caret = Text.Length;
				caret--;
				if (caret < 0) caret = 0;
				UpdateCaret();
			}
		}
		
		void RightKey(bool controlDown) {
			if (controlDown) {
				caret += Text.GetForwardLength(caret);
				if (caret >= Text.Length) caret = -1;
				UpdateCaret();
				return;
			}
			
			if (!Text.Empty && caret != -1) {
				caret++;
				if (caret >= Text.Length) caret = -1;
				UpdateCaret();
			}
		}
		
		void HomeKey() {
			if (Text.Empty) return;
			caret = 0;
			UpdateCaret();
		}
		
		void EndKey() {
			caret = -1;
			UpdateCaret();
		}
		
		static char[] trimChars = {'\r', '\n', '\v', '\f', ' ', '\t', '\0'};
		bool OtherKey(Key key) {
			int totalChars = MaxCharsPerLine * lines.Length;
			if (key == Key.V && Text.Length < totalChars) {
				string text = null;
				try {
					text = game.window.ClipboardText.Trim(trimChars);
				} catch (Exception ex) {
					ErrorHandler.LogError("Paste from clipboard", ex);
					const string warning = "&cError while trying to paste from clipboard.";
					game.Chat.Add(warning, MessageType.ClientStatus4);
					return true;
				}

				if (String.IsNullOrEmpty(text)) return true;
				Append(text);
				return true;
			} else if (key == Key.C) {
				if (Text.Empty) return true;
				try {
					game.window.ClipboardText = Text.ToString();
				} catch (Exception ex) {
					ErrorHandler.LogError("Copy to clipboard", ex);
					const string warning = "&cError while trying to copy to clipboard.";
					game.Chat.Add(warning, MessageType.ClientStatus4);
				}
				return true;
			}
			return false;
		}
		
		
		protected unsafe void SetCaretToCursor(int mouseX, int mouseY) {
			mouseX -= inputTex.X1; mouseY -= inputTex.Y1;
			DrawTextArgs args = new DrawTextArgs(null, font, true);
			IDrawer2D drawer = game.Drawer2D;
			int offset = 0, elemHeight = caretHeight;
			string oneChar = new String('A', 1);
			
			for (int y = 0; y < lines.Length; y++) {
				string line = lines[y];
				int xOffset = y == 0 ? prefixWidth : 0;
				if (line == null) continue;
				
				for (int x = 0; x < line.Length; x++) {
					args.Text = line.Substring(0, x);
					int trimmedWidth = drawer.MeasureChatSize(ref args).Width + xOffset;
					// avoid allocating an unnecessary string
					fixed(char* ptr = oneChar)
						ptr[0] = line[x];
					
					args.Text = oneChar;
					int elemWidth = drawer.MeasureChatSize(ref args).Width;
					
					if (GuiElement.Contains(trimmedWidth, y * elemHeight, elemWidth, elemHeight, mouseX, mouseY)) {
						caret = offset + x;
						UpdateCaret(); return;
					}
				}
				offset += line.Length;
			}
			caret = -1;
			UpdateCaret();
		}
		
		#endregion
	}
}