// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Gui.Widgets {
	public sealed class SurvivalHudWidget : Widget {
		
		public SurvivalHudWidget( Game game ) : base( game ) {
			HorizontalAnchor = Anchor.Centre;
			VerticalAnchor = Anchor.BottomOrRight;
		}
		
		// TODO: scaling
		public override void Init() {
			//float scale = 2 * game.GuiHotbarScale;
			Width = (int)(9 * 10);// * scale);
			Height = (int)9;// * scale);
			
			X = game.Width / 2 - Width / 2;
			Y = game.Height - Height - 100;
		}
		
		public override void Render( double delta ) {
			Model.ModelCache cache = game.ModelCache;
			int index = 0, health = game.LocalPlayer.Health;			
			for( int heart = 0; heart < 10; heart++ ) {
				Texture tex = new Texture( 0, X + 16 * heart, Y - 18, 18, 18, backRec );
				IGraphicsApi.Make2DQuad( ref tex, FastColour.WhitePacked,
				                        cache.vertices, ref index );
				
				if( health >= 2 ) {
					tex = new Texture( 0, X + 16 * heart + 2, Y - 18 + 2, 14, 14, fullRec );
				} else if( health == 1 ) {
					tex = new Texture( 0, X + 16 * heart + 2, Y - 18 + 2, 14, 14, halfRec );
				} else {
					continue;
				}
				
				IGraphicsApi.Make2DQuad( ref tex, FastColour.WhitePacked,
				                        cache.vertices, ref index );
				health -= 2;
			}
			
			game.Graphics.Texturing = true;
			game.Graphics.BindTexture( game.Gui.IconsTex );
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index );
			game.Graphics.Texturing = false;
		}
		
		static TextureRec backRec = new TextureRec( 16 / 256f, 0 / 256f, 9 / 256f, 9 / 256f );
		static TextureRec fullRec = new TextureRec( 53 / 256f, 1 / 256f, 7 / 256f, 7 / 256f );
		static TextureRec halfRec = new TextureRec( 62 / 256f, 1 / 256f, 7 / 256f, 7 / 256f );
		
		public override void Dispose() { }
	}
}