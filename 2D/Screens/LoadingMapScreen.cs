using System;
using System.Drawing;

namespace ClassicalSharp {
	
	public class LoadingMapScreen : Screen {
		
		readonly Font font;
		public LoadingMapScreen( Game window, string name, string motd ) : base( window ) {
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
			GraphicsApi.ClearColour( FastColour.Black );
			titleWidget.Render( delta );
			messageWidget.Render( delta );
			progressBoxTexture.Render( GraphicsApi );
			GraphicsApi.Draw2DQuad( progX, progY, progWidth * progress / 100f, progHeight, FastColour.White );
		}
		
		public override void Init() {
			GraphicsApi.Fog = false;
			titleWidget = CreateTextWidget( 30, serverName );
			messageWidget = CreateTextWidget( 60, serverMotd );
			progX = Window.Width / 2f - progWidth / 2f;
			using( Bitmap bmp = new Bitmap( progWidth, progHeight ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					Utils2D.DrawRectBounds( g, Color.White, 5f, 0, 0, progWidth, progHeight );
				}
				progressBoxTexture = Utils2D.Make2DTexture( GraphicsApi, bmp, 0, 0 );
			}
			progressBoxTexture.X1 = (int)progX;
			progressBoxTexture.Y1 = (int)progY;
			Window.MapLoading += MapLoading;
		}

		void MapLoading( object sender, MapLoadingEventArgs e ) {
			progress = e.Progress;
		}
		
		public override void Dispose() {
			font.Dispose();
			messageWidget.Dispose();
			titleWidget.Dispose();
			GraphicsApi.DeleteTexture( ref progressBoxTexture );
			Window.MapLoading -= MapLoading;
		}
		
		TextWidget CreateTextWidget( int yOffset, string text ) {
			TextWidget widget = new TextWidget( Window, font );
			widget.Init();
			widget.HorizontalDocking = Docking.Centre;
			widget.YOffset = yOffset;
			widget.SetText( text );
			return widget;
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
