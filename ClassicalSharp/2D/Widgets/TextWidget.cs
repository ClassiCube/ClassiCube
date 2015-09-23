using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed class TextWidget : Widget {
		
		public TextWidget( Game game, Font font ) : base( game ) {
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
		public int XOffset = 0, YOffset = 0;
		int defaultHeight;
		readonly Font font;
		
		public override void Init() {
			defaultHeight = Utils2D.MeasureSize( "I", font, true ).Height;
			Height = defaultHeight;
		}
		
		public void SetText( string text ) {
			graphicsApi.DeleteTexture( ref texture );
			if( String.IsNullOrEmpty( text ) ) {
				texture = new Texture();
				Height = defaultHeight;
			} else {
				DrawTextArgs args = new DrawTextArgs( graphicsApi, text, true );
				texture = Utils2D.MakeTextTexture( font, 0, 0, ref args );
				X = texture.X1 = CalcOffset( game.Width, texture.Width, XOffset, HorizontalDocking );
				Y = texture.Y1 = CalcOffset( game.Height, texture.Height, YOffset, VerticalDocking );
				Height = texture.Height;
			}		
			Width = texture.Width;
		}
		
		public override void Render( double delta ) {
			if( texture.IsValid ) {
				texture.Render( graphicsApi );
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
	}
}