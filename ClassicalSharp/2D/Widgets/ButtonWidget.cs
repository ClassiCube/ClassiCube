// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui {
	
	public sealed class ButtonWidget : Widget {
		
		public ButtonWidget( Game game, Font font ) : base( game ) {
			this.font = font;
		}
		
		public static ButtonWidget Create( Game game, int x, int y, int width, int height, string text, Anchor horizontal,
		                                  Anchor vertical, Font font, ClickHandler onClick ) {
			ButtonWidget widget = new ButtonWidget( game, font );
			widget.Init();
			widget.HorizontalAnchor = horizontal;
			widget.VerticalAnchor = vertical;
			widget.XOffset = x; widget.YOffset = y;
			widget.DesiredMaxWidth = width; widget.DesiredMaxHeight = height;
			widget.SetText( text );
			widget.OnClick = onClick;
			return widget;
		}
		
		Texture texture;
		public int DesiredMaxWidth, DesiredMaxHeight;
		int defaultHeight;
		internal Font font;
		
		public override void Init() {
			DrawTextArgs args = new DrawTextArgs( "I", font, true );
			defaultHeight = game.Drawer2D.MeasureChatSize( ref args ).Height;
			Height = defaultHeight;
		}
		
		static Texture shadowTex = new Texture( 0, 0, 0, 0, 0,
		                                       new TextureRec( 0, 66/256f, 200/256f, 20/256f ) );
		static Texture selectedTex = new Texture( 0, 0, 0, 0, 0,
		                                         new TextureRec( 0, 86/256f, 200/256f, 20/256f ) );
		static Texture disabledTex = new Texture( 0, 0, 0, 0, 0,
		                                         new TextureRec( 0, 46/256f, 200/256f, 20/256f ) );
		public string Text;
		public void SetText( string text ) {
			api.DeleteTexture( ref texture );
			Text = text;
			if( String.IsNullOrEmpty( text ) ) {
				texture = default(Texture);
				Width = 0; Height = defaultHeight;
			} else {
				DrawTextArgs args = new DrawTextArgs( text, font, true );
				texture = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
				Width = Math.Max( texture.Width, DesiredMaxWidth );
				Height = Math.Max( texture.Height, DesiredMaxHeight );
				
				X = CalcOffset( game.Width, Width, XOffset, HorizontalAnchor );
				Y = CalcOffset( game.Height, Height, YOffset, VerticalAnchor );				
				texture.X1 = X + (Width / 2 - texture.Width / 2);
				texture.Y1 = Y + (Height / 2 - texture.Height / 2);
			}
		}
		
		static FastColour normCol = new FastColour( 224, 224, 224 ),
		activeCol = new FastColour( 255, 255, 160 ),
		disabledCol = new FastColour( 160, 160, 160 );
		public override void Render( double delta ) {
			if( !texture.IsValid ) return;
			Texture backTex = Active ? selectedTex : shadowTex;
			if( Disabled ) backTex = disabledTex;
			
			backTex.ID = game.UseClassicGui ? game.GuiClassicTex : game.GuiTex;
			backTex.X1 = X; backTex.Y1 = Y;
			backTex.Width = (short)Width; backTex.Height = (short)Height;
			
			backTex.Render( api );
			FastColour col = Disabled ? disabledCol : (Active ? activeCol : normCol);
			texture.Render( api, col );
		}
		
		public override void Dispose() {
			api.DeleteTexture( ref texture );
		}
		
		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X, deltaY = newY - Y;
			texture.X1 += deltaX; texture.Y1 += deltaY;
			X = newX; Y = newY;
		}
		
		public Func<Game, string> GetValue;
		public Action<Game, string> SetValue;
	}
}