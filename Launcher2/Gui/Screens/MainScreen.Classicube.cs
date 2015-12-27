using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;

namespace Launcher2 {
	
	public sealed partial class MainScreen : LauncherInputScreen {

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
				} else if( HasServers ) {
					game.SetScreen( new ClassiCubeServersScreen( game ) );
					return;
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

		void DrawClassicube() {
			MakeLabelAt( "Username", titleFont, Anchor.Centre, Anchor.Centre, -170, -150 );
			MakeLabelAt( "Password", titleFont, Anchor.Centre, Anchor.Centre, -170, -100 );
			
			MakeInput( Get(), 280, Anchor.Centre, false, 30, -150, 32 );
			MakeInput( Get(), 280, Anchor.Centre, true, 30, -100, 32 );
			
			MakeButtonAt( "Sign in", buttonWidth / 2, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.Centre, 0, -50, LoginAsync );
			string text = widgets[5] == null ? "" : widgets[5].Text;
			MakeLabelAt( text, inputFont, Anchor.Centre, Anchor.Centre, 0, 0 );
		}

		string lastStatus;
		void SetStatus( string text ) {
			lastStatus = text;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LauncherLabelWidget widget = (LauncherLabelWidget)widgets[5];
				
				game.ClearArea( widget.X, widget.Y, widget.Width, widget.Height );
				widget.DrawAt( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 0, 0 );
				Dirty = true;
			}
		}
		
		bool HasServers {
			get {
				return !(game.Session.Servers == null || game.Session.Servers.Count == 0 );
			}
		}
		
		bool signingIn;
		void LoginAsync( int mouseX, int mouseY ) {
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
