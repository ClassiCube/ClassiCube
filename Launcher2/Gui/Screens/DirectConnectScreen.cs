using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;

namespace Launcher2 {
	
	public sealed class DirectConnectScreen : LauncherInputScreen {
		
		Font booleanFont;
		const int skinsIndex = 7;
		public DirectConnectScreen( LauncherWindow game ) : base( game, true ) {
			booleanFont = new Font( "Arial", 22, FontStyle.Regular );
			enterIndex = 3;
			widgets = new LauncherWidget[8];
		}

		public override void Init() {
			base.Init();
			Resize();
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LoadSavedInfo();
			}
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
			
			MakeInput( Get(), 330, Anchor.Centre, false, 0, -100, 32, "&7Username.." );
			MakeInput( Get(), 330, Anchor.Centre, false, 0, -50, 64, "&7IP address:Port number.." );
			MakeInput( Get(), 330, Anchor.Centre, false, 0, 0, 32, "&7Mppass.." );
			
			MakeButtonAt( "Connect", 110, 35, titleFont, Anchor.Centre, -110, 50, StartClient );
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre,
			             125, 50, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			MakeLabelAt( "", titleFont, Anchor.Centre, Anchor.Centre, 0, 100 );
			MakeLabelAt( "Use classicube.net for skins", inputFont, Anchor.Centre, Anchor.Centre, 30, 130 );
			MakeBooleanAt( Anchor.Centre, Anchor.Centre, booleanFont,
			              30, 30, -110, 130, UseClassicubeSkinsClick );
		}
		
		void SetStatus( string text ) {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LauncherLabelWidget widget = (LauncherLabelWidget)widgets[5];
				game.ClearArea( widget.X, widget.Y, widget.Width, widget.Height );
				widget.DrawAt( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 0, 100 );
				Dirty = true;
			}
		}
		
		void UseClassicubeSkinsClick( int mouseX, int mouseY ) {
			using( drawer ) {
				game.Drawer.SetBitmap( game.Framebuffer );
				LauncherBooleanWidget widget = (LauncherBooleanWidget)widgets[skinsIndex];
				SetBool( !widget.Value );
			}
		}
		
		void SetBool( bool value ) {
			LauncherBooleanWidget widget = (LauncherBooleanWidget)widgets[skinsIndex];
			widget.Value = value;
			widget.Redraw( game.Drawer );
			Dirty = true;
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
				Set( 0, (string)metadata["user"] );
				Set( 1, (string)metadata["address"] );
				Set( 2, (string)metadata["mppass"] );
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
			metadata["skins"] = ((LauncherBooleanWidget)widgets[skinsIndex]).Value;
		}
		
		void LoadFromOptions() {
			if( !Options.Load() )
				return;
			
			string user = Options.Get( "launcher-DC-username" ) ?? "";
			string ip = Options.Get( "launcher-DC-ip" ) ?? "127.0.0.1";
			string port = Options.Get( "launcher-DC-port" ) ?? "25565";
			bool ccSkins = Options.GetBool( "launcher-DC-ccskins", true );

			IPAddress address;
			if( !IPAddress.TryParse( ip, out address ) ) ip = "127.0.0.1";
			ushort portNum;
			if( !UInt16.TryParse( port, out portNum ) ) port = "25565";
			
			string mppass = Options.Get( "launcher-DC-mppass" ) ?? null;
			mppass = Secure.Decode( mppass, user );
			
			Set( 0, user );
			Set( 1, ip + ":" + port );
			Set( 2, mppass );
			SetBool( ccSkins );
		}
		
		void SaveToOptions( ClientStartData data, bool ccSkins ) {
			if( !Options.Load() )
				return;
			
			Options.Set( "launcher-DC-username", data.Username );
			Options.Set( "launcher-DC-ip", data.Ip );
			Options.Set( "launcher-DC-port", data.Port );
			Options.Set( "launcher-DC-mppass", Secure.Encode( data.Mppass, data.Username ) );
			Options.Set( "launcher-DC-ccskins", ccSkins );
			Options.Save();
		}
		
		void StartClient( int mouseX, int mouseY ) {
			string address = Get( 1 );
			int index = address.LastIndexOf( ':' );
			if( index <= 0 || index == address.Length - 1 ) {
				SetStatus( "&eInvalid address" ); return;
			}
			
			string ipPart = address.Substring( 0, index );
			string portPart = address.Substring( index + 1, address.Length - index - 1 );			
			ClientStartData data = GetStartData( Get( 0 ), Get( 2 ), ipPart, portPart );
			if( data == null ) return;
			
			bool ccSkins = ((LauncherBooleanWidget)widgets[skinsIndex]).Value;
			SaveToOptions( data, ccSkins );
			Client.Start( data, ccSkins, ref game.ShouldExit );
		}
		
		ClientStartData GetStartData( string user, string mppass, string ip, string port ) {
			SetStatus( "" );
			
			if( String.IsNullOrEmpty( user ) ) {
				SetStatus( "&ePlease enter a username" ); return null;
			}
			
			IPAddress realIp;
			if( !IPAddress.TryParse( ip, out realIp ) && ip != "localhost" ) {
				SetStatus( "&eInvalid ip" ); return null;
			}
			if( ip == "localhost" ) ip = "127.0.0.1";
			
			ushort realPort;
			if( !UInt16.TryParse( port, out realPort ) ) {
				SetStatus( "&eInvalid port" ); return null;
			}
			
			if( String.IsNullOrEmpty( mppass ) )
				mppass = "(none)";
			return new ClientStartData( user, mppass, ip, port );
		}
	}
}
