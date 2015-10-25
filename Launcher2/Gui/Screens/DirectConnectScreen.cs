using System;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;

namespace Launcher2 {
	
	public sealed class DirectConnectScreen : LauncherInputScreen {
		
		public DirectConnectScreen( LauncherWindow game ) : base( game ) {
			titleFont = new Font( "Arial", 15, FontStyle.Bold );
			inputFont = new Font( "Arial", 14, FontStyle.Regular );
			enterIndex = 6;		
			widgets = new LauncherWidget[9];
		}

		public override void Init() {
			base.Init();
			Resize();
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LoadSavedInfo( drawer );
			}
		}
		
		public override void Tick() {
		}
		
		bool ccSkins;
		void LoadSavedInfo( IDrawer2D drawer ) {
			try {
				Options.Load();
			} catch( IOException ) {
				return;
			}
			
			string user = Options.Get( "launcher-username" ) ?? "";
			string ip = Options.Get( "launcher-ip" ) ?? "127.0.0.1";
			string port = Options.Get( "launcher-port" ) ?? "25565";
			ccSkins = Options.GetBool( "launcher-ccskins", false );

			IPAddress address;
			if( !IPAddress.TryParse( ip, out address ) ) ip = "127.0.0.1";
			ushort portNum;
			if( !UInt16.TryParse( port, out portNum ) ) port = "25565";
			
			string mppass = Options.Get( "launcher-mppass" ) ?? null;
			mppass = Secure.Decode( mppass, user );
			
			Set( 3, user );
			Set( 4, ip + ":" + port );
			Set( 5, mppass );
		}
		
		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Draw();
			}
			Dirty = true;
		}
		
		void Draw() {
			widgetIndex = 0;
			MakeTextAt( "Username", -180, -100 );
			MakeTextAt( "Address", -180, -50 );
			MakeTextAt( "Mppass", -180, 0 );
			
			MakeInput( Get(), 300, Anchor.Centre, false, 30, -100, 32 );
			MakeInput( Get(), 300, Anchor.Centre, false, 30, -50, 64 );
			MakeInput( Get(), 300, Anchor.Centre, false, 30, 0, 32 );
			
			MakeButtonAt( "Connect", 110, 35, -65, 50, StartClient );			
			MakeButtonAt( "Back", 80, 35, 140, 50, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			MakeTextAt( "", 0, 100 );
		}
		
		void SetStatus( string text ) {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LauncherLabelWidget widget = (LauncherLabelWidget)widgets[8];
				drawer.Clear( game.clearColour, widget.X, widget.Y,
				             widget.Width, widget.Height );
				widget.DrawAt( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 0, 100 );
				Dirty = true;
			}
		}

		void MakeTextAt( string text, int x, int y ) {
			LauncherLabelWidget widget = new LauncherLabelWidget( game, text );
			widget.DrawAt( drawer, text, titleFont, Anchor.Centre, Anchor.Centre, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeButtonAt( string text, int width, int height,
		                  int x, int y, Action<int, int> onClick ) {
			LauncherButtonWidget widget = new LauncherButtonWidget( game );
			widget.Text = text;
			widget.OnClick = onClick;
			
			widget.Active = false;
			widget.DrawAt( drawer, text, titleFont, Anchor.Centre, Anchor.Centre, width, height, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void StartClient( int mouseX, int mouseY ) {
			SetStatus( "" );
			
			if( String.IsNullOrEmpty( Get( 3 ) ) ) {
				SetStatus( "&ePlease enter a username" ); return;
			}
			
			string address = Get( 4 );
			int index = address.LastIndexOf( ':' );
			if( index <= 0 || index == address.Length - 1 ) {
				SetStatus( "&eInvalid address" ); return;
			}
			
			string ipPart = address.Substring( 0, index ); IPAddress ip;
			if( !IPAddress.TryParse( ipPart, out ip ) && ipPart != "localhost" ) {
				SetStatus( "&eInvalid ip" ); return;
			}
			if( ipPart == "localhost" ) ipPart = "127.0.0.1";
			
			string portPart = address.Substring( index + 1, address.Length - index - 1 ); ushort port;
			if( !UInt16.TryParse( portPart, out port ) ) {
				SetStatus( "&eInvalid port" ); return;
			}
			
			string mppass = Get( 5 );
			if( String.IsNullOrEmpty( mppass ) ) mppass = "(none)";
			ClientStartData data = new ClientStartData( Get( 3 ), mppass, ipPart, portPart );
			Client.Start( data, ccSkins );
		}
	}
}
