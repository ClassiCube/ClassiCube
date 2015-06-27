using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed class TextWidget : Widget {	
		
		public TextWidget( Game window, Font font ) : base( window ) {
			this.font = font;
		}
		
		public static TextWidget Create( Game game, int x, int y, string text, Docking horizontal, Docking vertical, Font font ) {
			TextWidget widget = new TextWidget( game, font );
			widget.Init();
			widget.HorizontalDocking = horizontal;
			widget.VerticalDocking = vertical;
			widget.XOffset = x;
			widget.YOffset = y;
			widget.SetText( text );
			return widget;
		}
		
		Texture texture;
		string textCache = null;
		public int XOffset = 0, YOffset = 0;
		int defaultHeight;
		readonly Font font;
		
		public override void Init() {
			defaultHeight = Utils2D.MeasureSize( "I", font, true ).Height;
			texture.Height = defaultHeight;
			UpdateDimensions();
		}
		
		public void SetText( string text ) {
			GraphicsApi.DeleteTexture( ref texture );
			textCache = text;
			if( String.IsNullOrEmpty( text ) ) {
				texture = new Texture( -1, 0, 0, 0, defaultHeight, 0, 0 );
				UpdateDimensions();
				return;
			}
			
			List<DrawTextArgs> parts = null;
			Size size = new Size( 0, defaultHeight );
			parts = Utils2D.SplitText( GraphicsApi, text, true );
			size = Utils2D.MeasureSize( parts, font, true );
			
			X = CalcOffset( Window.Width, size.Width, XOffset, HorizontalDocking );
			Y = CalcOffset( Window.Height, size.Height, YOffset, VerticalDocking );
			texture = Utils2D.MakeTextTexture( parts, font, size, X, Y );
			UpdateDimensions();
		}
		
		public string GetText() {
			return textCache;
		}
		
		void UpdateDimensions() {
			Height = texture.Height;
			Width = texture.Width;
		}
		
		public override void Render( double delta ) {
			if( texture.IsValid ) {
				texture.Render( GraphicsApi );
			}
		}
		
		public override void Dispose() {
			GraphicsApi.DeleteTexture( ref texture );
		}
		
		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X;
			int deltaY = newY - Y;
			texture.X1 += deltaX;
			texture.Y1 += deltaY;
			X = newX;
			Y = newY;
		}
	}
}