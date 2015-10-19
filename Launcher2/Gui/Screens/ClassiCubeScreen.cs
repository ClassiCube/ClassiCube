using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;
using OpenTK;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed class ClassiCubeScreen : LauncherInputScreen {
		
		public ClassiCubeScreen( LauncherWindow game ) : base( game ) {
			titleFont = new Font( "Arial", 15, FontStyle.Bold );
			inputFont = new Font( "Arial", 15, FontStyle.Regular );
			enterIndex = 4;
			widgets = new LauncherWidget[7];
		}

		public override void Init() {
			Resize();
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LoadSavedInfo( drawer );
			}
		}
		
		public override void Tick() {
		}
		
		void LoadSavedInfo( IDrawer2D drawer ) {
			try {
				Options.Load();
			} catch( IOException ) {
				return;
			}
			
			string user = Options.Get( "launcher-cc-username" ) ?? "";
			string pass = Options.Get( "launcher-cc-password" ) ?? "";
			pass = Secure.Decode( pass, user );
			
			Set( 2, user );
			Set( 3, pass );
		}
		
		public override void Resize() {
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Draw( drawer );
			}
			Dirty = true;
		}
		
		static FastColour boxCol = new FastColour( 169, 143, 192 ), shadowCol = new FastColour( 97, 81, 110 );
		void Draw( IDrawer2D drawer ) {
			widgetIndex = 0;
			MakeTextAt( drawer, "Username", -180, -100 );
			MakeTextAt( drawer, "Password", -180, -50 );
			
			MakeTextInputAt( drawer, false, Get( widgetIndex ), 30, -100 );
			MakeTextInputAt( drawer, true, Get( widgetIndex ), 30, -50 );
			
			MakeButtonAt( drawer, "Sign in", 90, 35, -75, 0, StartClient );
			MakeButtonAt( drawer, "Back", 80, 35, 140, 0, () => game.SetScreen( new MainScreen( game ) ) );
			MakeTextAt( drawer, "", 0, 50 );
		}

		void SetStatus( string text ) {
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LauncherTextWidget widget = (LauncherTextWidget)widgets[6];
				drawer.Clear( LauncherWindow.clearColour, widget.X, widget.Y,
				             widget.Width, widget.Height );
				widget.DrawAt( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 0, 50 );
				Dirty = true;
			}
		}

		void MakeTextAt( IDrawer2D drawer, string text, int x, int y ) {
			LauncherTextWidget widget = new LauncherTextWidget( game, text );
			widget.DrawAt( drawer, text, titleFont, Anchor.Centre, Anchor.Centre, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeTextInputAt( IDrawer2D drawer, bool password, string text, int x, int y ) {
			LauncherTextInputWidget widget = new LauncherTextInputWidget( game );
			widget.OnClick = InputClick;
			widget.Password = password;
			
			widget.DrawAt( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 300, 30, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeButtonAt( IDrawer2D drawer, string text, int width, int height,
		                  int x, int y, Action onClick ) {
			LauncherButtonWidget widget = new LauncherButtonWidget( game );
			widget.Text = text;
			widget.OnClick = onClick;
			
			widget.DrawAt( drawer, text, titleFont, Anchor.Centre, Anchor.Centre, width, height, x, y );
			FilterArea( widget.X, widget.Y, widget.Width, widget.Height, 180 );
			widgets[widgetIndex++] = widget;
		}
		
		void StartClient() {
			if( String.IsNullOrEmpty( Get( 2 ) ) ) {
				SetStatus( "&ePlease enter a username" ); return;
			}
			if( String.IsNullOrEmpty( Get( 3 ) ) ) {
				SetStatus( "&ePlease enter a username" ); return;
			}
			System.Diagnostics.Debug.WriteLine( Get( 2 ) );
			System.Diagnostics.Debug.WriteLine( Get( 3 ) );
			
			SetStatus( "&eSigning in..." );
			ClassicubeSession session = game.Session;
			try {
				session.Login( Get( 2 ), Get( 3 ) );
			} catch( WebException ex ) {
				session.Username = null;
				DisplayWebException( ex, "sign in" );
				return;
			} catch( InvalidOperationException ex ) {
				session.Username = null;
				string text = "&eFailed to sign in: " +
					Environment.NewLine + ex.Message;
				SetStatus( text );
				return;
			}
			
			SetStatus( "&eRetrieving public servers list.." );
			try {
				game.Servers = session.GetPublicServers();
			} catch( WebException ex ) {
				game.Servers = new List<ServerListEntry>();
				DisplayWebException( ex, "retrieve servers list" );
				return;
			}
			SetStatus( "&eSigned in" );
		}

		void DisplayWebException( WebException ex, string action ) {
			Program.LogException( ex );
			if( ex.Status == WebExceptionStatus.Timeout ) {
				string text = "&eFailed to " + action + ":" +
					Environment.NewLine + "Timed out while connecting to classicube.net.";
				SetStatus( text );
			} else if( ex.Status == WebExceptionStatus.ProtocolError ) {
				HttpWebResponse response = (HttpWebResponse)ex.Response;
				int errorCode = (int)response.StatusCode;
				string description = response.StatusDescription;
				string text = "&eFailed to " + action + ":" +
					Environment.NewLine + " classicube.net returned: (" + errorCode + ") " + description;
				SetStatus(text );
			} else if( ex.Status == WebExceptionStatus.NameResolutionFailure ) {
				string text = "&eFailed to " + action + ":" +
					Environment.NewLine + "Unable to resolve classicube.net" +
					Environment.NewLine + "you may not be connected to the internet.";
				SetStatus( text );
			} else {
				string text = "&eFailed to " + action + ":" +
					Environment.NewLine + ex.Status;
				SetStatus( text );
			}
		}
	}
}
