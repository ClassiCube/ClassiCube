using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;

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
			Dictionary<string, object> metadata;
			// restore what user last typed into the various fields
			if( game.ScreenMetadata.TryGetValue( "screen-CC", out metadata ) ) {
				Set( 2, (string)metadata["user"] );
				Set( 3, (string)metadata["pass"] );
			} else {
				LoadFromOptions();
			}
		}
		
		void LoadFromOptions() {
			if( !Options.Load() )
				return;
			
			string user = Options.Get( "launcher-cc-username" ) ?? "UserXYZ";
			string pass = Options.Get( "launcher-cc-password" ) ?? "PassXYZ";
			pass = Secure.Decode( pass, user );
			
			Set( 2, user );
			Set( 3, pass );
		}
		
		void UpdateSignInInfo( string user, string password ) {
			// If the client has changed some settings in the meantime, make sure we keep the changes
			if( !Options.Load() )
				return;
			
			Options.Set( "launcher-cc-username", user );
			Options.Set( "launcher-cc-password", Secure.Encode( password, user ) );
			Options.Save();
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
			MakeTextAt( "Username", titleFont, -180, -100 );
			MakeTextAt( "Password", titleFont, -180, -50 );
			
			MakeInput( Get(), 300, Anchor.Centre, false, 30, -100, 32 );
			MakeInput( Get(), 300, Anchor.Centre, true, 30, -50, 32 );
			
			MakeButtonAt( "Sign in", 90, 35, titleFont, Anchor.Centre,
			             -75, 0, StartClient );
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre,
			             140, 0, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			string text = widgets[6] == null ? "" : ((LauncherLabelWidget)widgets[6]).Text;
			MakeTextAt( text, inputFont, 0, 50 );
			
			if( HasServers && !signingIn )
				MakeButtonAt( "Servers", 90, 35, titleFont, Anchor.Centre,
				             35, 0, ShowServers );
		}

		string lastStatus;
		void SetStatus( string text ) {
			lastStatus = text;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LauncherLabelWidget widget = (LauncherLabelWidget)widgets[6];
				
				drawer.Clear( game.clearColour, widget.X, widget.Y,
				             widget.Width, widget.Height );
				widget.DrawAt( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 0, 50 );
				Dirty = true;
			}
		}

		void MakeTextAt( string text, Font font, int x, int y ) {
			LauncherLabelWidget widget = new LauncherLabelWidget( game, text );
			widget.DrawAt( drawer, text, font, Anchor.Centre, Anchor.Centre, x, y );
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
				SetStatus( "&ePlease enter a password" ); return;
			}
			if( signingIn ) return;
			UpdateSignInInfo( Get( 2 ), Get( 3 ) );
			
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
			ErrorHandler.LogError( action, ex );
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
		
		public override void Dispose() {
			StoreFields();
			base.Dispose();
		}
		
		void StoreFields() {
			Dictionary<string, object> metadata;
			if( !game.ScreenMetadata.TryGetValue( "screen-CC", out metadata ) ) {
				metadata = new Dictionary<string, object>();
				game.ScreenMetadata["screen-CC"] = metadata;
			}
			
			metadata["user"] = Get( 2 );
			metadata["pass"] = Get( 3 );
		}
	}
}
