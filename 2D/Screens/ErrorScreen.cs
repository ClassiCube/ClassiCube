using System;
using System.Drawing;
using System.Collections.Generic;

namespace ClassicalSharp {
	
	public class ErrorScreen : Screen {
		
		string title, message;
		public ErrorScreen( Game window, string title, string message ) : base( window ) {
			this.title = title;
			this.message = message;
		}
		
		Texture titleTexture, messageTexture;
		
		public override void Render( double delta ) {
			titleTexture.Render( GraphicsApi );
			messageTexture.Render( GraphicsApi );
		}
		
		public override void Init() {
			GraphicsApi.ClearColour( new FastColour( 65, 31, 31 ) );
			titleTexture = ModifyText( 30, title, 16, FontStyle.Bold );
			messageTexture = ModifyText( -10, message, 14, FontStyle.Regular );
		}
		
		public override void Dispose() {
			GraphicsApi.DeleteTexture( ref titleTexture );
			GraphicsApi.DeleteTexture( ref messageTexture );
		}
		
		Texture ModifyText( int yOffset, string text, int fontSize, FontStyle style ) {			
			List<DrawTextArgs> parts = Utils.SplitText( GraphicsApi, text, true );
			Size size = Utils2D.MeasureSize( Utils.StripColours( text ), "Arial", fontSize, style, true );
			int x = Window.Width / 2 - size.Width / 2;
			int y = Window.Height / 2 - yOffset;
			return Utils2D.MakeTextTexture( parts, "Arial", fontSize, style, size, x, y );
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			int xDiff = ( width - oldWidth ) / 2;
			int yDiff = ( height - oldHeight ) / 2;
			messageTexture.X1 += xDiff;
			titleTexture.X1 += xDiff;
			messageTexture.Y1 += yDiff;
			titleTexture.Y1 += yDiff;
		}
		
		public override bool BlocksWorld {
			get { return true; }
		}
		
		public override bool HandlesAllInput {
			get { return true; }
		}
	}
}
