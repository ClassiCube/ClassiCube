using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;

namespace Launcher2 {
	
	public sealed class DirectConnectScreen : LauncherInputScreen {
		
		Font booleanFont;
		public DirectConnectScreen( LauncherWindow game ) : base( game, true ) {
			booleanFont = new Font( "Arial", 22, FontStyle.Regular );
			enterIndex = 6;
			widgets = new LauncherWidget[11];
		}

		public override void Init() {
			base.Init();
			Resize();
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LoadSavedInfo();
			}
		}
		
		public override void Tick() { 
		}
		
		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Draw();
			}
			Dirty = true;
		}
		
		void SetBool( bool value ) {
			LauncherBooleanWidget widget = (LauncherBooleanWidget)widgets[7];
			widget.Value = value;
			widget.Redraw( game.Drawer );
			Dirty = true;
		}
		
		void Draw() {
			widgetIndex = 0;
			
			MakeInput( Get(), 300, Anchor.Centre, false, 30, -100, 32, "&7Username.." );
			MakeInput( Get(), 300, Anchor.Centre, false, 30, -50, 64, "&7IP address:Port number.." );
			MakeInput( Get(), 300, Anchor.Centre, false, 30, 0, 32, "&7Mppass.." );
			
			MakeButtonAt( "Connect", 110, 35, titleFont, Anchor.Centre, -65, 50, StartClient );
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre,
			             140, 50, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			MakeLabelAt( "", titleFont, Anchor.Centre, Anchor.Centre, 0, 100 );
			MakeLabelAt( "Use classicube.net for skins", inputFont, Anchor.Centre, Anchor.Centre, 30, 130 );
			MakeBooleanAt( Anchor.Centre, Anchor.Centre, booleanFont,
			              30, 30, -110, 130, UseClassicubeSkinsClick );
		}
		
		void SetStatus( string text ) {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LauncherLabelWidget widget = (LauncherLabelWidget)widgets[8];
				game.ClearArea( widget.X, widget.Y, widget.Width, widget.Height );
				widget.DrawAt( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 0, 100 );
				Dirty = true;
			}
		}
		
		void UseClassicubeSkinsClick( int mouseX, int mouseY ) {
			using( drawer ) {
				game.Drawer.SetBitmap( game.Framebuffer );
				LauncherBooleanWidget widget = (LauncherBooleanWidget)widgets[10];
				SetBool( !widget.Value );
			}
		}
		
		void StartClient( int mouseX, int mouseY ) {
			SetStatus( "" );
			
			if( String.IsNullOrEmpty( Get( 0 ) ) ) {
				SetStatus( "&ePlease enter a username" ); return;
			}
			
			string address = Get( 1 );
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
			
			string mppass = Get( 2 );
			if( String.IsNullOrEmpty( mppass ) ) mppass = "(none)";
			ClientStartData data = new ClientStartData( Get( 0 ), mppass, ipPart, portPart );
			bool ccSkins = ((LauncherBooleanWidget)widgets[7]).Value;
			Client.Start( data, ccSkins, ref game.ShouldExit );
		}
		
		public override void Dispose() {
			StoreFields();
			base.Dispose();
			booleanFont.Dispose();
		}
		
		void LoadSavedInfo() {
			Dictionary<string, object> metadata;
			// restore what user last typed into the various fields
			if( game.ScreenMetadata.TryGetValue( "screen-DC", out metadata ) ) {
				Set( 3, (string)metadata["user"] );
				Set( 4, (string)metadata["address"] );
				Set( 5, (string)metadata["mppass"] );
				SetBool( (bool)metadata["skins"] );
			} else {
				LoadFromOptions();
			}
		}
		
		void StoreFields() {
			Dictionary<string, object> metadata;
			if( !game.ScreenMetadata.TryGetValue( "screen-DC", out metadata ) ) {
				metadata = new Dictionary<string, object>();
				game.ScreenMetadata["screen-DC"] = metadata;
			}
			
			metadata["user"] = Get( 0 );
			metadata["address"] = Get( 1 );
			metadata["mppass"] = Get( 2 );
			metadata["skins"] = ((LauncherBooleanWidget)widgets[7]).Value;
		}
		
		void LoadFromOptions() {
			if( !Options.Load() )
				return;
			
			string user = Options.Get( "launcher-username" ) ?? "";
			string ip = Options.Get( "launcher-ip" ) ?? "127.0.0.1";
			string port = Options.Get( "launcher-port" ) ?? "25565";
			bool ccSkins = Options.GetBool( "launcher-ccskins", true );

			IPAddress address;
			if( !IPAddress.TryParse( ip, out address ) ) ip = "127.0.0.1";
			ushort portNum;
			if( !UInt16.TryParse( port, out portNum ) ) port = "25565";
			
			string mppass = Options.Get( "launcher-mppass" ) ?? null;
			mppass = Secure.Decode( mppass, user );
			
			Set( 0, user );
			Set( 1, ip + ":" + port );
			Set( 2, mppass );
			SetBool( ccSkins );
		}
	}
}
