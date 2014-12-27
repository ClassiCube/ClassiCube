using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed class TextWidget : Widget {	
		
		public TextWidget( Game window, int fontSize ) : base( window ) {
			this.fontSize = fontSize;
		}
		
		Texture texture;
		string textCache = null;
		public int XOffset = 0, YOffset = 0;
		public FontStyle Style = FontStyle.Regular;
		int defaultHeight;
		readonly int fontSize;
		
		public override void Init() {
			defaultHeight = Utils2D.MeasureSize( "I", "Arial", fontSize, Style, true ).Height;
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
			parts = Utils.SplitText( GraphicsApi, text, true );
			size = Utils2D.MeasureSize( Utils.StripColours( text ), "Arial", fontSize, Style, true );
			
			X = CalcAdjOffset( XOffset, Window.Width, size.Width, HorizontalDocking );
			Y = CalcAdjOffset( YOffset, Window.Height, size.Height, VerticalDocking );
			texture = Utils2D.MakeTextTexture( parts, "Arial", fontSize, Style, size, X, Y );
			UpdateDimensions();
		}
		
		int CalcAdjOffset( int offset, int totalSize, int axisSize, Docking mode ) {
			if( mode == Docking.LeftOrTop ) return offset;
			if( mode == Docking.BottomOrRight) return totalSize - axisSize + offset;
			return ( totalSize - axisSize ) / 2 + offset;
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