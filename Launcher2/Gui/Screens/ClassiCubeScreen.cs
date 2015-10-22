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
			widgets = new LauncherWidget[8];
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
			if( !signingIn ) return;
			
			ClassicubeSession session = game.Session;
			string status = session.Status;
			if( status != lastStatus )
				SetStatus( status );
			
			if( !session.Working ) {
				if( session.Exception != null ) {
					DisplayWebException( session.Exception, session.Status );
				}
				signingIn = false;
				game.MakeBackground();
				Resize();
			}
			
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
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Draw();
			}
			Dirty = true;
		}

		void Draw() {
			widgetIndex = 0;
			MakeTextAt( "Username", -180, -100 );
			MakeTextAt( "Password", -180, -50 );
			
			MakeTextInputAt( false, Get( widgetIndex ), 30, -100 );
			MakeTextInputAt( true, Get( widgetIndex ), 30, -50 );
			
			MakeButtonAt( "Sign in", 90, 35, -75, 0, StartClient );
			MakeButtonAt( "Back", 80, 35, 140, 0, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			MakeTextAt( "", 0, 50 );
			
			if( HasServers && !signingIn )
				MakeButtonAt( "Servers", 90, 35, 35, 0, ShowServers );
		}

		string lastStatus;
		void SetStatus( string text ) {
			lastStatus = text;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LauncherTextWidget widget = (LauncherTextWidget)widgets[6];
				
				drawer.Clear( LauncherWindow.clearColour, widget.X, widget.Y,
				             widget.Width, widget.Height );
				widget.DrawAt( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 0, 50 );
				Dirty = true;
			}
		}

		void MakeTextAt( string text, int x, int y ) {
			LauncherTextWidget widget = new LauncherTextWidget( game, text );
			widget.DrawAt( drawer, text, titleFont, Anchor.Centre, Anchor.Centre, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeTextInputAt( bool password, string text, int x, int y ) {
			LauncherTextInputWidget widget = new LauncherTextInputWidget( game );
			widget.OnClick = InputClick;
			widget.Password = password;
			
			widget.DrawAt( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 300, 30, x, y );
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
		
		bool HasServers {
			get {
				return !(game.Session.Servers == null || game.Session.Servers.Count == 0 );
			}
		}
		
		bool signingIn;
		void StartClient( int mouseX, int mouseY ) {
			if( String.IsNullOrEmpty( Get( 2 ) ) ) {
				SetStatus( "&ePlease enter a username" ); return;
			}
			if( String.IsNullOrEmpty( Get( 3 ) ) ) {
				SetStatus( "&ePlease enter a username" ); return;
			}
			if( signingIn ) return;
			
			game.Session.LoginAsync( Get( 2 ), Get( 3 ) );
			game.MakeBackground();
			Resize();
			SetStatus( "&eSigning in.." );
			signingIn = true;
		}
		
		void ShowServers( int mouseX, int mouseY ) {
			if( signingIn || !HasServers ) return;	
			game.SetScreen( new ClassiCubeServersScreen( game ) );
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
