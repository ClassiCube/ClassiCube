// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {

	/// <summary> Widget that represents text can have modified by the user. </summary>
	public sealed class LauncherInputWidget : LauncherWidget {
		
		public int ButtonWidth, ButtonHeight;
		
		/// <summary> Text displayed when the user has not entered anything in the text field. </summary>
		public string HintText;
		
		/// <summary> Whether the input widget currently is focused through a mouse click or tab. </summary>
		public bool Active;
		
		/// <summary> Whether all characters should be rendered as *. </summary>
		public bool Password;
		
		public LauncherInputText Chars;
		
		Font font, hintFont;
		int textHeight;
		public LauncherInputWidget( LauncherWindow window ) : base( window ) {
			Chars = new LauncherInputText( this );
		}

		public void SetDrawData( IDrawer2D drawer, string text, Font font, Font hintFont,
		                        Anchor horAnchor, Anchor verAnchor, int width, int height, int x, int y ) {
			ButtonWidth = width; ButtonHeight = height;
			Width = width; Height = height;
			SetAnchors( horAnchor, verAnchor ).SetOffsets( x, y )
				.CalculatePosition();
			
			Text = text;
			if( Password ) text = new String( '*', text.Length );
			this.font = font;
			this.hintFont = hintFont;
			
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			Width = Math.Max( ButtonWidth, size.Width + 15 );
			textHeight = size.Height;
		}
		
		public void SetDrawData( IDrawer2D drawer, string text ) {
			Text = text;
			if( Password ) text = new String( '*', text.Length );
			
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			Width = Math.Max( ButtonWidth, size.Width + 15 );
			textHeight = size.Height;
		}
		
		public override void Redraw( IDrawer2D drawer ) {
			string text = Text;
			if( Password ) text = new String( '*', text.Length );
			DrawTextArgs args = new DrawTextArgs( "&0" + text, font, false );
			
			Size size = drawer.MeasureSize( ref args );
			Width = Math.Max( ButtonWidth, size.Width + 15 );
			textHeight = size.Height;
			args.SkipPartsCheck = true;
			if( Window.Minimised ) return;
			
			using( FastBitmap bmp = Window.LockBits() ) {
				DrawOuterBorder( bmp );
				DrawInnerBorder( bmp );
				Clear( bmp, FastColour.White, X + 2, Y + 2, Width - 4, Height - 4 );
				BlendBoxTop( bmp );
			}
			DrawText( drawer, args );
		}
		
		static FastColour borderIn = new FastColour( 165, 142, 168 );
		static FastColour borderOut = new FastColour( 97, 81, 110 );
		const int border = 1;
		
		void DrawOuterBorder( FastBitmap bmp ) {
			FastColour col = borderOut;
			if( Active ) {
				Clear( bmp, col, X, Y, Width, border );
				Clear( bmp, col, X, Y + Height - border, Width, border );
				Clear( bmp, col, X, Y, border, Height );
				Clear( bmp, col, X + Width - border, Y, border, Height );
			} else {
				Window.ResetArea( X, Y, Width, border, bmp );
				Window.ResetArea( X, Y + Height - border, Width, border, bmp );
				Window.ResetArea( X, Y, border, Height, bmp );
				Window.ResetArea( X + Width - border, Y, border, Height, bmp );
			}
		}
		
		void DrawInnerBorder( FastBitmap bmp ) {
			FastColour col = borderIn;
			Clear( bmp, col, X + border, Y + border, Width - border * 2, border );
			Clear( bmp, col, X + border, Y + Height - border * 2, Width - border * 2, border );
			Clear( bmp, col, X + border, Y + border, border, Height - border * 2 );
			Clear( bmp, col, X + Width - border * 2, Y + border, border, Height - border * 2 );
		}
		
		void BlendBoxTop( FastBitmap bmp ) {
			Rectangle r = new Rectangle( X + border, Y, Width - border * 2, border );
			r.Y += border; Gradient.Blend( bmp, r, FastColour.Black, 75 );
			r.Y += border; Gradient.Blend( bmp, r, FastColour.Black, 50 );
			r.Y += border; Gradient.Blend( bmp, r, FastColour.Black, 25 );
		}
		
		void Clear( FastBitmap bmp, FastColour col, 
		           int x, int y, int width, int height ) {
			Drawer2DExt.Clear( bmp, new Rectangle( x, y, width, height ), col );
		}
		
		void DrawText( IDrawer2D drawer, DrawTextArgs args ) {
			if( Text.Length != 0 || HintText == null ) {
				int y = Y + 2 + (Height - textHeight) / 2;
				drawer.DrawText( ref args, X + 5, y );
			} else {
				args.SkipPartsCheck = false;
				args.Text = HintText;
				args.Font = hintFont;
				
				Size hintSize = drawer.MeasureSize( ref args );
				int y = Y + (Height - hintSize.Height) / 2;
				args.SkipPartsCheck = true;
				drawer.DrawText( ref args, X + 5, y );
			}
		}
		
		public Rectangle MeasureCaret( IDrawer2D drawer, Font font ) {
			string text = Text;
			if( Password )
				text = new String( '*', text.Length );
			Rectangle r = new Rectangle( X + 5, Y + Height - 5, 0, 2 );
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			
			if( Chars.CaretPos == -1 ) {
				Size size = drawer.MeasureSize( ref args );
				r.X += size.Width; r.Width = 10;
			} else {
				args.Text = text.Substring( 0, Chars.CaretPos );
				int trimmedWidth = drawer.MeasureChatSize( ref args ).Width;
				args.Text = new String( text[Chars.CaretPos], 1 );
				int charWidth = drawer.MeasureChatSize( ref args ).Width;
				r.X += trimmedWidth; r.Width = charWidth;
			}
			return r;
		}
		
		public void AdvanceCursorPos( bool forwards ) {
			if( forwards && Chars.CaretPos == -1 ) return;
			if( !forwards && Chars.CaretPos == 0 ) return;
			if( Chars.CaretPos == -1 && !forwards ) // caret after text
				Chars.CaretPos = Text.Length;
			
			Chars.CaretPos += (forwards ? 1 : -1);
			if( Chars.CaretPos < 0 || Chars.CaretPos >= Text.Length )
				Chars.CaretPos = -1;
		}
		
		public void SetCaretToCursor( int mouseX, int mouseY, IDrawer2D drawer, Font font ) {
			string text = Text;
			if( Password )
				text = new String( '*', text.Length );
			mouseX -= X; mouseY -= Y;
			
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			if( mouseX >= size.Width ) {
				Chars.CaretPos = -1; return;
			}
			
			for( int i = 0; i < Text.Length; i++ ) {
				args.Text = text.Substring( 0, i );
				int trimmedWidth = drawer.MeasureChatSize( ref args ).Width;
				args.Text = new String( text[i], 1 );
				int charWidth = drawer.MeasureChatSize( ref args ).Width;
				if( mouseX >= trimmedWidth && mouseX < trimmedWidth + charWidth ) {
					Chars.CaretPos = i; return;
				}
			}
		}
	}
}
