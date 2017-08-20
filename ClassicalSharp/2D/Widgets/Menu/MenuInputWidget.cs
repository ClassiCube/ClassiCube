// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Widgets {
	public sealed class MenuInputWidget : InputWidget {
		
		public MenuInputWidget(Game game, Font font) : base(game, font) { }
		
		public static MenuInputWidget Create(Game game, int width, int height, string text,
		                                     Font font, MenuInputValidator validator) {
			MenuInputWidget input = new MenuInputWidget(game, font);
			input.MinWidth = width;
			input.MinHeight = height;
			input.Validator = validator;
			
			input.Init();
			input.Append(text);
			return input;
		}
		
		static FastColour backCol = new FastColour(30, 30, 30, 200);
		public int MinWidth, MinHeight;
		public MenuInputValidator Validator;
		
		public override int MaxLines { get { return 1; } }
		public override string Prefix { get { return null; } }
		public override int Padding { get { return 3; } }
		public override int MaxCharsPerLine { get { return Utils.StringLength; } }
		
		public override void Render(double delta) {
			gfx.Texturing = false;
			gfx.Draw2DQuad(X, Y, Width, Height, backCol);
			gfx.Texturing = true;
			
			inputTex.Render(gfx);
			RenderCaret(delta);
		}
		
		public override void RemakeTexture() {
			DrawTextArgs args = new DrawTextArgs(lines[0], font, false);
			Size size = game.Drawer2D.MeasureSize(ref args);
			caretAccumulator = 0;
			
			// Ensure we don't have 0 text height
			if (size.Height == 0) {
				args.Text = Validator.Range;
				size.Height = game.Drawer2D.MeasureSize(ref args).Height;
				args.Text = lines[0];
			} else {
				args.SkipPartsCheck = true;
			}
			
			Width = Math.Max(size.Width, MinWidth);
			Height = Math.Max(size.Height, MinHeight);
			Size adjSize = size; adjSize.Width = Width;
			
			using (Bitmap bmp = IDrawer2D.CreatePow2Bitmap(adjSize))
				using (IDrawer2D drawer = game.Drawer2D)
			{
				drawer.SetBitmap(bmp);
				drawer.DrawText(ref args, Padding, 0);
				
				args.Text = Validator.Range;
				args.SkipPartsCheck = false;
				Size hintSize = drawer.MeasureSize(ref args);
				
				args.SkipPartsCheck = true;
				int hintX = adjSize.Width - hintSize.Width;
				if (size.Width + 3 < hintX)
					drawer.DrawText(ref args, hintX, 0);
				inputTex = drawer.Make2DTexture(bmp, adjSize, 0, 0);
			}

			Reposition();			
			inputTex.X1 = X; inputTex.Y1 = Y;
			if (size.Height < MinHeight)
				inputTex.Y1 += MinHeight / 2 - size.Height / 2;
		}
		
		protected override bool AllowedChar(char c) {
			if (c == '&' || !Utils.IsValidInputChar(c, true)) return false;
			if (!Validator.IsValidChar(c)) return false;
			if (Text.Length == MaxCharsPerLine) return false;
			
			// See if the new string is in valid format
			AppendChar(c);
			bool valid = Validator.IsValidString(Text.ToString());
			DeleteChar();
			return valid;
		}
	}
}