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
			messageFont = new Font( game.FontName, 14, FontStyle.Regular );
		}
		
		public override void Render( double delta ) {
			UpdateReconnectState();
			graphicsApi.Texturing = true;
			for( int i = 0; i < widgets.Length; i++ )
				widgets[i].Render( delta );
			graphicsApi.Texturing = false;			
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
			graphicsApi.ClearColour( new FastColour( 65, 31, 31 ) );
			widgets = new Widget[] {
				ChatTextWidget.Create( game, 0, -30, title, Anchor.Centre, Anchor.Centre, titleFont ),
				ChatTextWidget.Create( game, 0, 10, message, Anchor.Centre, Anchor.Centre, messageFont ),
				ButtonWidget.Create( game, 0, 80, 280, 35, "Try to reconnect.. " + delay, 
				                    Anchor.Centre, Anchor.Centre, titleFont, ReconnectClick ),
			};
			initTime = DateTime.UtcNow;
			lastSecsLeft = delay;
		}
		
		void ReconnectClick( Game g, Widget w, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			string connectString = "Connecting to " + game.IPAddress + ":" + game.Port +  "..";
			game.SetNewScreen( new LoadingMapScreen( game, connectString, "Waiting for handshake" ) );
			game.Network.Connect( game.IPAddress, game.Port );
		}
		
		public override void Dispose() {
			titleFont.Dispose();
			messageFont.Dispose();
			for( int i = 0; i < widgets.Length; i++ )
				widgets[i].Dispose();
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			for( int i = 0; i < widgets.Length; i++ )
				widgets[i].OnResize( oldWidth, oldHeight, width, height );
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
	}
}
