using System;
using System.Drawing;

namespace ClassicalSharp {
	
	public class ErrorScreen : Screen {
		
		string title, message;
		readonly Font titleFont, messageFont;
		TextWidget titleWidget, messageWidget;
		
		public ErrorScreen( Game game, string title, string message ) : base( game ) {
			this.title = title;
			this.message = message;
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			messageFont = new Font( "Arial", 14, FontStyle.Regular );
		}	
		
		public override void Render( double delta ) {
			graphicsApi.Texturing = true;
			titleWidget.Render( delta );
			messageWidget.Render( delta );
			graphicsApi.Texturing = false;
		}
		
		public override void Init() {
			graphicsApi.ClearColour( new FastColour( 65, 31, 31 ) );
			titleWidget = TextWidget.Create( game, 0, -30, title, Anchor.Centre, Anchor.Centre, titleFont );
			messageWidget = TextWidget.Create( game, 0, 10, message, Anchor.Centre, Anchor.Centre, messageFont );
		}
		
		public override void Dispose() {
			titleFont.Dispose();
			messageFont.Dispose();
			titleWidget.Dispose();
			messageWidget.Dispose();
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			titleWidget.OnResize( oldWidth, oldHeight, width, height );
			messageWidget.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override bool BlocksWorld {
			get { return true; }
		}
		
		public override bool HandlesAllInput {
			get { return true; }
		}
	}
}
