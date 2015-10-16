using System;
using System.Drawing;
using System.Drawing.Drawing2D;

namespace ClassicalSharp {
	
	public sealed class ButtonWidget : Widget {
		
		public ButtonWidget( Game game, Font font ) : base( game ) {
			this.font = font;
		}
		
		public static ButtonWidget Create( Game game, int x, int y, int width, int height, string text, Anchor horizontal,
		                                  Anchor vertical, Font font, Action<Game, ButtonWidget> onClick ) {
			ButtonWidget widget = new ButtonWidget( game, font );
			widget.Init();
			widget.HorizontalAnchor = horizontal;
			widget.VerticalAnchor = vertical;
			widget.XOffset = x;
			widget.YOffset = y;
			widget.DesiredMaxWidth = width;
			widget.DesiredMaxHeight = height;
			widget.SetText( text );
			widget.OnClick = onClick;
			return widget;
		}
		
		Texture texture;
		public int XOffset = 0, YOffset = 0;
		public int DesiredMaxWidth, DesiredMaxHeight;
		int defaultHeight;
		readonly Font font;
		
		public override void Init() {
			defaultHeight = game.Drawer2D.MeasureSize( "I", font, true ).Height;
			Height = defaultHeight;
		}
		
		static FastColour boxCol = new FastColour( 169, 143, 192 ), shadowCol = new FastColour( 97, 81, 110 );
		//static FastColour boxCol = new FastColour( 29, 126, 192 ), shadowCol = new FastColour( 16, 72, 109 );
		public string Text;
		public void SetText( string text ) {
			graphicsApi.DeleteTexture( ref texture );
			Text = text;
			if( String.IsNullOrEmpty( text ) ) {
				texture = new Texture();
				Height = defaultHeight;
			} else {
				MakeTexture( text );
				X = texture.X1 = CalcOffset( game.Width, texture.Width, XOffset, HorizontalAnchor );
				Y = texture.Y1 = CalcOffset( game.Height, texture.Height, YOffset, VerticalAnchor );
				Height = texture.Height;
			}
			Width = texture.Width;
		}
		
		public override void Render( double delta ) {
			if( texture.IsValid ) {
				FastColour col = Active ? FastColour.White : new FastColour( 200, 200, 200 );
				graphicsApi.BindTexture( texture.ID );
				graphicsApi.Draw2DTexture( ref texture, col );
			}
		}
		
		public override void Dispose() {
			graphicsApi.DeleteTexture( ref texture );
		}
		
		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X;
			int deltaY = newY - Y;
			texture.X1 += deltaX;
			texture.Y1 += deltaY;
			X = newX;
			Y = newY;
		}
		
		public Action<Game, ButtonWidget> OnClick;
		public Func<Game, string> GetValue;
		public Action<Game, string> SetValue;
		public bool Active;
		
		void MakeTexture( string text ) {			
			Size size = game.Drawer2D.MeasureSize( text, font, true );
			int xOffset = Math.Max( size.Width, DesiredMaxWidth ) - size.Width;
			size.Width = Math.Max( size.Width, DesiredMaxWidth );
			int yOffset = Math.Max( size.Height, DesiredMaxHeight ) - size.Height;
			size.Height = Math.Max( size.Height, DesiredMaxHeight );
			Size baseSize = size;
			const int borderSize = 3; // 1 px for base border, 2 px for shadow, 1 px for offset text
			size.Width += borderSize; size.Height += borderSize;
			
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) ) {
				using( IDrawer2D drawer = game.Drawer2D ) {
					drawer.SetBitmap( bmp );					
					drawer.DrawRoundedRect( shadowCol, 3, IDrawer2D.Offset, IDrawer2D.Offset, 
					                       baseSize.Width, baseSize.Height );
					drawer.DrawRoundedRect( boxCol, 3, 0, 0, baseSize.Width, baseSize.Height );
					
					DrawTextArgs args = new DrawTextArgs( text, true );
					args.SkipPartsCheck = true;
					drawer.DrawText( font, ref args, 1 + xOffset / 2, 1 + yOffset / 2 );
					texture = drawer.Make2DTexture( bmp, size, 0, 0 );
				}
			}
		}
	}
}