// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed class MenuInputWidget : Widget {
		
		public MenuInputWidget( Game game, Font font, Font boldFont ) : base( game ) {
			HorizontalAnchor = Anchor.LeftOrTop;
			VerticalAnchor = Anchor.BottomOrRight;
			this.font = font;
			this.boldFont = boldFont;
			chatInputText = new StringBuffer( 64 );
		}
		
		public static MenuInputWidget Create( Game game, int x, int y, int width, int height, string text, Anchor horizontal,
		                                     Anchor vertical, Font font, Font tildeFont, MenuInputValidator validator ) {
			MenuInputWidget widget = new MenuInputWidget( game, font, tildeFont );
			
			widget.HorizontalAnchor = horizontal;
			widget.VerticalAnchor = vertical;
			widget.XOffset = x;
			widget.YOffset = y;
			widget.DesiredMaxWidth = width;
			widget.DesiredMaxHeight = height;
			widget.chatInputText.Append( 0, text );
			widget.Validator = validator;
			widget.Init();
			return widget;
		}
		
		Texture chatInputTexture, chatCaretTexture;
		Color backColour = Color.FromArgb( 200, 30, 30, 30 );
		readonly Font font, boldFont;
		StringBuffer chatInputText;
		public int XOffset = 0, YOffset = 0;
		public int DesiredMaxWidth, DesiredMaxHeight;
		public MenuInputValidator Validator;
		
		double accumulator;
		public override void Render( double delta ) {
			chatInputTexture.Render( api );
			//if( (accumulator % 1) < 0.5 && Active ) {
			//	chatCaretTexture.Y1 = chatInputTexture.Y1 + yOffset;
			//	chatCaretTexture.Render( graphicsApi );
			//}
			accumulator += delta;
		}

		public override void Init() {
			DrawTextArgs caretArgs = new DrawTextArgs( "_", boldFont, false );
			chatCaretTexture = game.Drawer2D.MakeChatTextTexture( ref caretArgs, 0, 0 );
			SetText( chatInputText.GetString() );
		}
		
		int yOffset;
		public void SetText( string value ) {
			chatInputText.Clear();
			chatInputText.Append( 0, value );
			DrawTextArgs args = new DrawTextArgs( value, font, false );
			Size textSize = game.Drawer2D.MeasureChatSize( ref args );
			// Ensure we don't have 0 text height
			if( textSize.Height == 0 ) {
				args.Text = Validator.Range;
				textSize.Height = game.Drawer2D.MeasureChatSize( ref args ).Height;
				args.Text = value;
			} else {
				args.SkipPartsCheck = true;
			}
			
			Size size = new Size( Math.Max( textSize.Width, DesiredMaxWidth ),
			                     Math.Max( textSize.Height, DesiredMaxHeight ) );
			yOffset = 0;
			if( textSize.Height < DesiredMaxHeight )
				yOffset = DesiredMaxHeight / 2 - textSize.Height / 2;
			
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) )
				using( IDrawer2D drawer = game.Drawer2D )
			{
				drawer.SetBitmap( bmp );
				drawer.DrawRect( backColour, 0, 0, size.Width, size.Height );
				drawer.DrawChatText( ref args, 3, yOffset );
				
				args.Text = Validator.Range;
				args.SkipPartsCheck = false;
				Size hintSize = drawer.MeasureChatSize( ref args );
				
				args.SkipPartsCheck = true;
				int hintX = size.Width - hintSize.Width;
				if( textSize.Width + 3 < hintX )
					drawer.DrawChatText( ref args, hintX, yOffset );
				chatInputTexture = drawer.Make2DTexture( bmp, size, 0, 0 );
			}
			
			X = CalcOffset( game.Width, size.Width, XOffset, HorizontalAnchor );
			Y = CalcOffset( game.Height, size.Height, YOffset, VerticalAnchor );
			chatCaretTexture.X1 = chatInputTexture.X1 = X;
			chatCaretTexture.X1 += textSize.Width;
			chatCaretTexture.Y1 = chatInputTexture.Y1 = Y;
			chatCaretTexture.Y1 = (Y + size.Height) - chatCaretTexture.Height;
			Width = size.Width;
			Height = size.Height;
		}
		
		public string GetText() {
			return chatInputText.GetString();
		}

		public override void Dispose() {
			api.DeleteTexture( ref chatCaretTexture );
			api.DeleteTexture( ref chatInputTexture );
		}

		public override void MoveTo( int newX, int newY ) {
			int dx = newX - X, dy = newY - Y;
			X = newX; Y = newY;
			chatCaretTexture.X1 += dx;
			chatCaretTexture.Y1 += dy;
			chatInputTexture.X1 += dx;
			chatInputTexture.Y1 += dy;
		}
		
		static bool IsInvalidChar( char c ) {
			// Make sure we're in the printable text range from 0x20 to 0x7E
			return c < ' ' || c == '&' || c > '~';
			// TODO: Uncomment this for full unicode support for save level screen?
		}
		
		public override bool HandlesKeyPress( char key ) {
			if( chatInputText.Length < 64 && !IsInvalidChar( key ) ) {
				if( !Validator.IsValidChar( key ) ) return true;
				chatInputText.Append( chatInputText.Length, key );
				
				if( !Validator.IsValidString( chatInputText.GetString() ) ) {
					chatInputText.DeleteAt( chatInputText.Length - 1 );
					return true;
				}
				api.DeleteTexture( ref chatInputTexture );
				SetText( chatInputText.ToString() );
			}
			return true;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.BackSpace && !chatInputText.Empty ) {
				chatInputText.DeleteAt( chatInputText.Length - 1 );
				api.DeleteTexture( ref chatInputTexture );
				SetText( chatInputText.ToString() );
			}
			return key < Key.F1 || key > Key.F35;
		}
		
		public override bool HandlesKeyUp( Key key ) {
			return true;
		}
	}
}