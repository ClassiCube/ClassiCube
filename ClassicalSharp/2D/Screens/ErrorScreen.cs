// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public class ErrorScreen : ClickableScreen {
		
		string title, message;
		readonly Font titleFont, messageFont;
		Widget[] widgets;
		DateTime initTime;
		
		public ErrorScreen( Game game, string title, string message ) : base( game ) {
			this.title = title;
			this.message = message;
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			messageFont = new Font( game.FontName, 16, FontStyle.Regular );
		}
		
		public override void Render( double delta ) {
			// NOTE: We need to make sure that both the front and back buffers have
			// definitely been drawn over, so we redraw the background multiple times.
			if( drawnBackground < 5 ) 
				DrawBackground();
			
			UpdateReconnectState();
			api.Texturing = true;
			for( int i = 0; i < widgets.Length; i++ )
				widgets[i].Render( delta );
			api.Texturing = false;			
		}
		
		int lastSecsLeft;
		const int delay = 5;
		void UpdateReconnectState() {
			ButtonWidget btn = (ButtonWidget)widgets[2];
			double elapsed = (DateTime.UtcNow - initTime).TotalSeconds;
			int scsLeft = Math.Max( 0, (int)(delay - elapsed) );
			
			if( lastSecsLeft != scsLeft ) {
				string suffix = scsLeft == 0 ? "" : ".. " + scsLeft;
				btn.SetText( "Try to reconnect" + suffix );
			}
			btn.Disabled = scsLeft != 0;
		}
		
		public override void Init() {
			game.SkipClear = true;
			widgets = new Widget[] {
				ChatTextWidget.Create( game, 0, -30, title, Anchor.Centre, Anchor.Centre, titleFont ),
				ChatTextWidget.Create( game, 0, 10, message, Anchor.Centre, Anchor.Centre, messageFont ),
				ButtonWidget.Create( game, 0, 80, 301, 40, "Try to reconnect.. " + delay, 
				                    Anchor.Centre, Anchor.Centre, titleFont, ReconnectClick ),
			};
			initTime = DateTime.UtcNow;
			lastSecsLeft = delay;
		}
		
		void ReconnectClick( Game g, Widget w, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			string connectString = "Connecting to " + game.IPAddress + ":" + game.Port +  "..";
			foreach( IGameComponent comp in game.Components )
				comp.Reset( game );
			game.BlockInfo.Reset( game );
			
			game.SetNewScreen( new LoadingMapScreen( game, connectString, "Waiting for handshake" ) );
			game.Network.Connect( game.IPAddress, game.Port );
		}
		
		public override void Dispose() {
			game.SkipClear = false;
			titleFont.Dispose();
			messageFont.Dispose();
			for( int i = 0; i < widgets.Length; i++ )
				widgets[i].Dispose();
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			for( int i = 0; i < widgets.Length; i++ )
				widgets[i].OnResize( oldWidth, oldHeight, width, height );
			drawnBackground = 0;
		}
		
		public override bool BlocksWorld { get { return true; } }
		
		public override bool HandlesAllInput { get { return true; } }
		
		public override bool HidesHud { get { return true; } }
		
		public override bool HandlesKeyDown( Key key ) { return true; }
		
		public override bool HandlesKeyPress( char key ) { return true; }
		
		public override bool HandlesKeyUp( Key key ) { return true; }
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			return HandleMouseClick( widgets, mouseX, mouseY, button );
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			return HandleMouseMove( widgets, mouseX, mouseY );
		}
		
		public override bool HandlesMouseScroll( int delta )  { return true; }
		
		public override bool HandlesMouseUp( int mouseX, int mouseY, MouseButton button ) { return true; }
		
		int drawnBackground;
		readonly FastColour top = new FastColour( 64, 32, 32 ), bottom = new FastColour( 80, 16, 16 );
		void DrawBackground() {
			drawnBackground++;
			api.Draw2DQuad( 0, 0, game.Width, game.Height, top, bottom );
		}
	}
}
