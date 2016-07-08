// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;

namespace ClassicalSharp.Gui {
	
	public sealed class ClassicPlayerListWidget : NormalPlayerListWidget {
		
		ChatTextWidget overview;
		static FastColour topCol = new FastColour( 0, 0, 0, 180 );
		static FastColour bottomCol = new FastColour( 50, 50, 50, 205 );
		public ClassicPlayerListWidget( Game game, Font font ) : base( game, font ) {
		}
		
		protected override void OnSort() {
			int width = 0, centreX = game.Width / 2;
			for( int col = 0; col < columns; col++)
				width += GetColumnWidth( col );
			if( width < 480 ) width = 480;
			
			xMin = centreX - width / 2;
			xMax = centreX + width / 2;
			
			int x = xMin, y = game.Height / 2 - yHeight / 2;
			for( int col = 0; col < columns; col++ ) {
				SetColumnPos( col, x, y );
				x += GetColumnWidth( col );
			}
		}
		
		public override void Render( double delta ) {
			api.Texturing = false;
			int offset = overview.Height + 10;
			int height = Math.Max( 300, Height + overview.Height );
			api.Draw2DQuad( X, Y - offset, Width, height, topCol, bottomCol );
			
			api.Texturing = true;
			overview.MoveTo( game.Width / 2 - overview.Width / 2,
			                Y - offset + boundsSize / 2 );
			overview.Render( delta );
			
			for( int i = 0; i < namesCount; i++ ) {
				Texture tex = textures[i];
				int texY = tex.Y;
				tex.Y1 -= 10;
				if( tex.IsValid ) tex.Render( api );
				tex.Y1 = texY;
			}
		}
		
		public override void Init() {
			overview = ChatTextWidget.Create( game, 0, 0, "Connected players:",
			                                 Anchor.Centre, Anchor.Centre, font );			
			base.Init();
		}
		
		protected override Texture DrawName( PlayerInfo pInfo ) {
			string name = pInfo.ColouredName;
			if( game.PureClassic ) name = Utils.StripColours( name );
			
			DrawTextArgs args = new DrawTextArgs( name, font, false );
			Texture tex = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
			game.Drawer2D.ReducePadding( ref tex, Utils.Floor( font.Size ), 3 );
			return tex;
		}
		
		public override void Dispose() {
			base.Dispose();
			overview.Dispose();
		}
	}
}