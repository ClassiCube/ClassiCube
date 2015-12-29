using System;
using System.Drawing;
using OpenTK.Input;

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
			SetTitle( serverName );
			SetMessage( serverMotd );
			progX = game.Width / 2f - progWidth / 2f;
			
			Size size = new Size( progWidth, progHeight );
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) ) {
				using( IDrawer2D drawer = game.Drawer2D ) {
					drawer.SetBitmap( bmp );
					drawer.DrawRectBounds( FastColour.White, 3f, 0, 0, progWidth, progHeight );
					progressBoxTexture = drawer.Make2DTexture( bmp, size, (int)progX, (int)progY );
				}
			}
			game.MapEvents.MapLoading += MapLoading;
		}
		
		public void SetTitle( string title ) {
			if( titleWidget != null )
				titleWidget.Dispose();
			titleWidget = TextWidget.Create( game, 0, 30, title, Anchor.Centre, Anchor.LeftOrTop, font );
		}
		
		public void SetMessage( string message ) {
			if( messageWidget != null )
				messageWidget.Dispose();
			messageWidget = TextWidget.Create( game, 0, 60, message, Anchor.Centre, Anchor.LeftOrTop, font );
		}
		
		public void SetProgress( float progress ) {
			this.progress = (int)(progress * 100);
		}

		void MapLoading( object sender, MapLoadingEventArgs e ) {
			progress = e.Progress;
		}
		
		public override void Dispose() {
			font.Dispose();
			messageWidget.Dispose();
			titleWidget.Dispose();
			graphicsApi.DeleteTexture( ref progressBoxTexture );
			game.MapEvents.MapLoading -= MapLoading;
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
		
		public override bool HidesHud {
			get { return false; }
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Tab ) return true;
			return game.hudScreen.HandlesKeyDown( key ); 
		}
		
		public override bool HandlesKeyPress( char key )  { 
			return game.hudScreen.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			if( key == Key.Tab ) return true;
			return game.hudScreen.HandlesKeyUp( key );
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) { return true; }
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) { return true; }
		
		public override bool HandlesMouseScroll( int delta )  { return true; }
		
		public override bool HandlesMouseUp( int mouseX, int mouseY, MouseButton button ) { return true; }
	}
}
