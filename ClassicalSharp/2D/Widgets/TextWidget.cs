// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;

namespace ClassicalSharp.Gui {
	
	public class TextWidget : Widget {
		
		public TextWidget( Game game, Font font ) : base( game ) {
			this.font = font;
		}
		
		public static TextWidget Create( Game game, int x, int y, string text, Anchor horizontal, Anchor vertical, Font font ) {
			TextWidget widget = new TextWidget( game, font );
			widget.Init();
			widget.HorizontalAnchor = horizontal;
			widget.VerticalAnchor = vertical;
			widget.XOffset = x;
			widget.YOffset = y;
			widget.SetText( text );
			return widget;
		}
		
		protected Texture texture;
		public int XOffset = 0, YOffset = 0;
		protected int defaultHeight;
		protected internal Font font;
		
		public bool IsValid { get { return texture.IsValid; } }
		
		public override void Init() {
			DrawTextArgs args = new DrawTextArgs( "I", font, true );
			defaultHeight = game.Drawer2D.MeasureSize( ref args ).Height;
			Height = defaultHeight;
		}
		
		public virtual void SetText( string text ) {
			api.DeleteTexture( ref texture );
			if( String.IsNullOrEmpty( text ) ) {
				texture = new Texture();
				Height = defaultHeight;
			} else {
				DrawTextArgs args = new DrawTextArgs( text, font, true );
				texture = game.Drawer2D.MakeTextTexture( ref args, 0, 0 );
				X = texture.X1 = CalcOffset( game.Width, texture.Width, XOffset, HorizontalAnchor );
				Y = texture.Y1 = CalcOffset( game.Height, texture.Height, YOffset, VerticalAnchor );
				Height = texture.Height;
			}		
			Width = texture.Width;
		}
		
		public override void Render( double delta ) {
			if( texture.IsValid )
				texture.Render( api );
		}
		
		public override void Dispose() {
			api.DeleteTexture( ref texture );
		}
		
		public override void MoveTo( int newX, int newY ) {
			int diffX = newX - X, diffY = newY - Y;
			texture.X1 += diffX; texture.Y1 += diffY;
			X = newX; Y = newY;
		}
	}
}