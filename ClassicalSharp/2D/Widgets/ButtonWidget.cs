using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;

namespace ClassicalSharp {
	
	public sealed class ButtonWidget : Widget {
		
		public ButtonWidget( Game game, Font font ) : base( game ) {
			this.font = font;
		}
		
		public static ButtonWidget Create( Game game, int x, int y, int width, int height, string text, Docking horizontal,
		                                  Docking vertical, Font font, Action<Game> onClick ) {
			ButtonWidget widget = new ButtonWidget( game, font );
			widget.Init();
			widget.HorizontalDocking = horizontal;
			widget.VerticalDocking = vertical;
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
			defaultHeight = Utils2D.MeasureSize( "I", font, true ).Height;
			Height = defaultHeight;
		}
		
		static FastColour boxCol = new FastColour( 169, 143, 192 ), shadowCol = new FastColour( 97, 81, 110 );
		//static FastColour boxCol = new FastColour( 29, 126, 192 ), shadowCol = new FastColour( 16, 72, 109 );
		public void SetText( string text ) {
			graphicsApi.DeleteTexture( ref texture );
			if( String.IsNullOrEmpty( text ) ) {
				texture = new Texture();
				Height = defaultHeight;
			} else {
				MakeTexture( text );
				X = texture.X1 = CalcOffset( game.Width, texture.Width, XOffset, HorizontalDocking );
				Y = texture.Y1 = CalcOffset( game.Height, texture.Height, YOffset, VerticalDocking );
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
		
		public Action<Game> OnClick;
		public bool Active;
		
		void MakeTexture( string text ) {
			DrawTextArgs args = new DrawTextArgs( graphicsApi, text, true );
			Size size = Utils2D.MeasureSize( text, font, true );
			int xOffset = Math.Max( size.Width, DesiredMaxWidth ) - size.Width;
			size.Width = Math.Max( size.Width, DesiredMaxWidth );
			int yOffset = Math.Max( size.Height, DesiredMaxHeight ) - size.Height;
			size.Height = Math.Max( size.Height, DesiredMaxHeight );
			Size baseSize = size;
			const int borderSize = 3; // 1 px for base border, 2 px for shadow, 1 px for offset text
			size.Width += borderSize; size.Height += borderSize;
			
			using( Bitmap bmp = Utils2D.CreatePow2Bitmap( size ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					args.SkipPartsCheck = true;
					g.SmoothingMode = SmoothingMode.HighQuality;
					// Draw shadow rectangle
					GraphicsPath path = MakePath( 1.3f, baseSize.Width, baseSize.Height );
					using( Brush brush = new SolidBrush( shadowCol ) )
						g.FillPath( brush, path );
					path.Dispose();
					// Draw base rectangle
					path = MakePath( 0, baseSize.Width, baseSize.Height );
					using( Brush brush = new SolidBrush( boxCol ) )
						g.FillPath( brush, path );
					path.Dispose();
					
					Utils2D.DrawText( g, font, ref args, 1 + xOffset / 2, 1 + yOffset / 2 );
				}
				texture = Utils2D.Make2DTexture( graphicsApi, bmp, size, 0, 0 );
			}
		}
		GraphicsPath MakePath( float offset, float width, float height ) {
			GraphicsPath path = new GraphicsPath();
			float x1 = offset, y1 = offset;
			float x2 = offset + width, y2 = offset + height;
			const float r = 3, dia = r * 2;
			path.AddArc( x1, y1, dia, dia, 180, 90 );
			path.AddLine( x1 + r, y1, x2 - r, y1 );
			path.AddArc( x2 - dia, y1, dia, dia, 270, 90 );
			path.AddLine( x2, y1 + r, x2, y2 - r );
			path.AddArc( x2 - dia, y2 - dia, dia, dia, 0, 90 );
			path.AddLine( x1 + r, y2, x2 - r, y2 );
			path.AddArc( x1, y2 - dia, dia, dia, 90, 90 );
			path.AddLine( x1, y1 + r, x1, y2 - r );
			path.CloseAllFigures();
			return path;
		}
	}
}