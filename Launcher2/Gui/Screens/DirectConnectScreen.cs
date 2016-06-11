// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Net;
using ClassicalSharp;
using Launcher.Gui.Views;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Screens {
	
	public sealed class DirectConnectScreen : LauncherInputScreen {
		
		Font booleanFont;
		DirectConnectView view;
		public DirectConnectScreen( LauncherWindow game ) : base( game, true ) {
			booleanFont = new Font( game.FontName, 22, FontStyle.Regular );
			enterIndex = 3;
			view = new DirectConnectView( game );
			widgets = view.widgets;
		}

		public override void Init() {
			base.Init();
			view.Init();
			SetWidgetHandlers();
			Resize();
		}
		
		public override void Resize() {
			view.DrawAll();
			Dirty = true;
		}
		
		void SetWidgetHandlers() {
			widgets[view.backIndex].OnClick =
				(x, y) => game.SetScreen( new MainScreen( game ) );
			widgets[view.connectIndex].OnClick = StartClient;
			widgets[view.ccSkinsIndex].OnClick = UseClassicubeSkinsClick;			
			SetupInputHandlers();
			LoadSavedInfo();
		}
		
		void SetStatus( string text ) {
			LauncherLabelWidget widget = (LauncherLabelWidget)widgets[5];
			game.ClearArea( widget.X, widget.Y, widget.Width, widget.Height );
			widget.SetDrawData( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 0, 100 );
			RedrawWidget( widget );
		}
		
		void UseClassicubeSkinsClick( int mouseX, int mouseY ) {
			LauncherBoolWidget widget = (LauncherBoolWidget)widgets[view.ccSkinsIndex];
			widget.Value = !widget.Value;
			RedrawWidget( widget );
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
				SetText( 0, (string)metadata["user"] );
				SetText( 1, (string)metadata["address"] );
				SetText( 2, (string)metadata["mppass"] );
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
			metadata["skins"] = ((LauncherBoolWidget)widgets[view.ccSkinsIndex]).Value;
		}
		
		void LoadFromOptions() {
			if( !Options.Load() ) return;
			
			string user = Options.Get( "launcher-dc-username" ) ?? "";
			string ip = Options.Get( "launcher-dc-ip" ) ?? "127.0.0.1";
			string port = Options.Get( "launcher-dc-port" ) ?? "25565";
			bool ccSkins = Options.GetBool( "launcher-dc-ccskins", true );

			IPAddress address;
			if( !IPAddress.TryParse( ip, out address ) ) ip = "127.0.0.1";
			ushort portNum;
			if( !UInt16.TryParse( port, out portNum ) ) port = "25565";
			
			string mppass = Options.Get( "launcher-dc-mppass" );
			mppass = Secure.Decode( mppass, user );
			
			SetText( 0, user );
			SetText( 1, ip + ":" + port );
			SetText( 2, mppass );
			SetBool( ccSkins );
		}
		
		void SaveToOptions( ClientStartData data, bool ccSkins ) {
			if( !Options.Load() )
				return;
			
			Options.Set( "launcher-dc-username", data.Username );
			Options.Set( "launcher-dc-ip", data.Ip );
			Options.Set( "launcher-dc-port", data.Port );
			Options.Set( "launcher-dc-mppass", Secure.Encode( data.Mppass, data.Username ) );
			Options.Set( "launcher-dc-ccskins", ccSkins );
			Options.Save();
		}
		
		void SetText( int index, string text ) {
			((LauncherInputWidget)widgets[index]).SetDrawData( drawer, text );
		}
		
		void SetBool( bool value ) {
			((LauncherBoolWidget)widgets[view.ccSkinsIndex]).Value = value;
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
			
			bool ccSkins = ((LauncherBoolWidget)widgets[view.ccSkinsIndex]).Value;
			SaveToOptions( data, ccSkins );
			Client.Start( data, ccSkins, ref game.ShouldExit );
		}
		
		static Random rnd = new Random();
		static byte[] rndBytes = new byte[8];
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
			
			ClientStartData data = new ClientStartData( user, mppass, ip, port );
			if( user.ToLowerInvariant() == "rand()" || user.ToLowerInvariant() == "random()") {
				rnd.NextBytes( rndBytes );
				data.Username = Convert.ToBase64String( rndBytes ).TrimEnd( '=' );
			}
			return data;
		}
	}
}
