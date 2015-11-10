using System;
using System.Drawing;
using OpenTK.Input;
using System.Windows.Forms;

namespace ClassicalSharp {
	
	public sealed partial class AltTextInputWidget : Widget {

		public AltTextInputWidget( Game game, Font font, Font boldFont, TextInputWidget parent ) : base( game ) {
			HorizontalAnchor = Anchor.LeftOrTop;
			VerticalAnchor = Anchor.LeftOrTop;
			this.font = font;
			this.boldFont = boldFont;
			this.parent = parent;
		}
		
		Texture chatInputTexture;
		readonly Font font, boldFont;
		TextInputWidget parent;
		Size partSize;
		
		public override void Render( double delta ) {
			chatInputTexture.Render( graphicsApi );
		}
		
		public override void Init() {
			X = 5; Y = 45;
			DrawString();
		}
		
		static FastColour backColour = new FastColour( 60, 60, 60, 200 );
		void DrawString() {
			DrawTextArgs args = new DrawTextArgs( "Text ", font, false );
			partSize = game.Drawer2D.MeasureChatSize( ref args );
			Size size = new Size( partSize.Width * 6, partSize.Height * 3 );
			
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) ) {
				using( IDrawer2D drawer = game.Drawer2D ) {
					drawer.SetBitmap( bmp );
					drawer.Clear( backColour, 0, 0, size.Width, size.Height );
					for( int code = 0; code <= 15; code++ ) {
						int c = code < 10 ? '0' + code : 'a' + (code - 10);
						args.Text = "&" + (char)c + "Text";
						
						int x = (code % 6);
						int y = (code / 6);
						drawer.DrawChatText( ref args, x * partSize.Width, y * partSize.Height );
					}
					chatInputTexture = drawer.Make2DTexture( bmp, size, X, Y );
				}
			}		
			Height = size.Height;
			Width = size.Width;		
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {	
			mouseX -= X; mouseY -= Y;
			mouseX /= partSize.Width; mouseY /= partSize.Height;
			game.Chat.Add( "CLICKY CLICK" + mouseX + "," + mouseY );
			
			int code = mouseY * 6 + mouseX;
			if( code <= 15 ) {
				int c = code < 10 ? '0' + code : 'a' + (code - 10);
				string text = "&" + (char)c;
				parent.AppendText( text );
			}
			return true;
		}

		public override void Dispose() {
			graphicsApi.DeleteTexture( ref chatInputTexture );
		}
	}
}