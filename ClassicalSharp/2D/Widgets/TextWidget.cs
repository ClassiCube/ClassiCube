// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;

namespace ClassicalSharp.Gui.Widgets {	
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
		protected int defaultHeight;
		protected internal Font font;
		
		public bool ReducePadding;
		public FastColour Colour = FastColour.White;
		public bool IsValid { get { return texture.IsValid; } }
		
		public override void Init() {
			DrawTextArgs args = new DrawTextArgs( "I", font, true );
			int height = game.Drawer2D.MeasureSize( ref args ).Height;
			SetHeight( height );
		}
		
		protected void SetHeight( int height ) {
			if( ReducePadding )
				game.Drawer2D.ReducePadding( ref height, Utils.Floor( font.Size ) );
			defaultHeight = height;
			Height = height;
		}
		
		public virtual void SetText( string text ) {
			api.DeleteTexture( ref texture );
			if( String.IsNullOrEmpty( text ) ) {
				texture = new Texture();
				Width = 0; Height = defaultHeight;
			} else {
				DrawTextArgs args = new DrawTextArgs( text, font, true );
				texture = game.Drawer2D.MakeTextTexture( ref args, 0, 0 );
				if( ReducePadding )
					game.Drawer2D.ReducePadding( ref texture, Utils.Floor( font.Size ) );
				Width = texture.Width; Height = texture.Height;
				
				X = texture.X1 = CalcOffset( game.Width, Width, XOffset, HorizontalAnchor );
				Y = texture.Y1 = CalcOffset( game.Height, Height, YOffset, VerticalAnchor );
			}
		}
		
		public override void Render( double delta ) {
			if( texture.IsValid )
				texture.Render( api, Colour );
		}
		
		public override void Dispose() {
			api.DeleteTexture( ref texture );
		}
		
		public override void MoveTo( int newX, int newY ) {
			int dx = newX - X, dy = newY - Y;
			texture.X1 += dx; texture.Y1 += dy;
			X = newX; Y = newY;
		}
	}
}