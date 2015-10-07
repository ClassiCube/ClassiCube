using System;
using System.Drawing;

namespace ClassicalSharp {
	
	public class LoadingMapScreen : Screen {
		
		readonly Font font;
		public LoadingMapScreen( Game game, string name, string motd ) : base( game ) {
			serverName = name;
			serverMotd = motd;
			font = new Font( "Arial", 14 );
		}
		
		string serverName, serverMotd;
		int progress;
		Texture progressBoxTexture;
		TextWidget titleWidget, messageWidget;
		float progX, progY = 100f;
		int	progWidth = 200, progHeight = 40;
		
		public override void Render( double delta ) {		
			graphicsApi.ClearColour( FastColour.Black );
			graphicsApi.Texturing = true;
			titleWidget.Render( delta );
			messageWidget.Render( delta );
			progressBoxTexture.Render( graphicsApi );
			graphicsApi.Texturing = false;
			graphicsApi.Draw2DQuad( progX, progY, progWidth * progress / 100f, progHeight, FastColour.White );
		}
		
		public override void Init() {
			graphicsApi.Fog = false;
			titleWidget = TextWidget.Create( game, 0, 30, serverName, Docking.Centre, Docking.LeftOrTop, font );
			messageWidget = TextWidget.Create( game, 0, 60, serverMotd, Docking.Centre, Docking.LeftOrTop, font );
			progX = game.Width / 2f - progWidth / 2f;
			
			Size size = new Size( progWidth, progHeight );
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) ) {
				using( IDrawer2D drawer = game.Drawer2D ) {
					drawer.SetBitmap( bmp );
					drawer.DrawRectBounds( Color.White, 3f, 0, 0, progWidth, progHeight );
					progressBoxTexture = drawer.Make2DTexture( bmp, size, (int)progX, (int)progY );
				}			
			}
			game.Events.MapLoading += MapLoading;
		}

		void MapLoading( object sender, MapLoadingEventArgs e ) {
			progress = e.Progress;
		}
		
		public override void Dispose() {
			font.Dispose();
			messageWidget.Dispose();
			titleWidget.Dispose();
			graphicsApi.DeleteTexture( ref progressBoxTexture );
			game.Events.MapLoading -= MapLoading;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			int deltaX = ( width - oldWidth ) / 2;
			messageWidget.OnResize( oldWidth, oldHeight, width, height );
			titleWidget.OnResize( oldWidth, oldHeight, width, height );
			progressBoxTexture.X1 += deltaX;
			progX += deltaX;
		}
		
		public override bool BlocksWorld {
			get { return true; }
		}
		
		public override bool HandlesAllInput {
			get { return true; }
		}
	}
}
