using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;

namespace Launcher {
	
	public sealed partial class MainScreen : LauncherInputScreen {

		const int skipSSLIndex = 11;
		
		public override void Tick() {
			base.Tick();
			if( game.checkTask != null )
				 game.checkTask.TaskTick( SuccessfulUpdateCheck, this );
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

		void DrawClassicube() {
			MakeInput( Get(), 280, Anchor.Centre, false, 0, -120, 32, "&7Username.." );
			MakeInput( Get(), 280, Anchor.Centre, true, 0, -70, 64, "&7Password.." );
			
			MakeButtonAt( "Sign in", 100, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.Centre, -90, -20, LoginAsync );
			MakeLabelAt( Get(), inputFont, Anchor.Centre, Anchor.Centre, 0, 20 );
		}

		string lastStatus;
		void SetStatus( string text ) {
			lastStatus = text;
			LauncherLabelWidget widget = (LauncherLabelWidget)widgets[3];
			
			game.ClearArea( widget.X, widget.Y, widget.Width, widget.Height );
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				widget.SetDrawData( drawer, text, inputFont, Anchor.Centre, Anchor.Centre, 0, 20 );
				widget.Redraw( drawer );
			}
			Dirty = true;
		}
		
		bool HasServers {
			get { return game.Session.Servers != null && game.Session.Servers.Count != 0; }
		}
		
		bool signingIn;
		void LoginAsync( int mouseX, int mouseY ) {
			if( String.IsNullOrEmpty( Get( 0 ) ) ) {
				SetStatus( "&ePlease enter a username" ); return;
			}
			if( String.IsNullOrEmpty( Get( 1 ) ) ) {
				SetStatus( "&ePlease enter a password" ); return;
			}
			if( signingIn ) return;
			UpdateSignInInfo( Get( 0 ), Get( 1 ) );
			
			LauncherBooleanWidget booleanWidget = widgets[skipSSLIndex] as LauncherBooleanWidget;
			if( booleanWidget != null && booleanWidget.Value ) {
				ServicePointManager.ServerCertificateValidationCallback = delegate { return true; };
			} else {
				ServicePointManager.ServerCertificateValidationCallback = null;
			}
			
			game.Session.LoginAsync( Get( 0 ), Get( 1 ) );
			game.MakeBackground();
			Resize();
			SetStatus( "&eSigning in.." );
			signingIn = true;
		}

		void DisplayWebException( WebException ex, string action ) {
			ErrorHandler.LogError( action, ex );
			bool sslCertError = ex.Status == WebExceptionStatus.TrustFailure ||
				(ex.Status == WebExceptionStatus.SendFailure && OpenTK.Configuration.RunningOnMono);
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
				SetStatus( text );
			} else if( ex.Status == WebExceptionStatus.NameResolutionFailure ) {
				string text = "&eFailed to " + action + ":" +
					Environment.NewLine + "Unable to resolve classicube.net" +
					Environment.NewLine + "you may not be connected to the internet.";
				SetStatus( text );
			} else if( sslCertError ) {
				string text = "&eFailed to validate SSL certificate";
				SetStatus( text );
				using( drawer ) {
					drawer.SetBitmap( game.Framebuffer );
					widgetIndex = skipSSLIndex;
					MakeSSLSkipValidationBoolean();
					MakeSSLSkipValidationLabel();
					widgets[skipSSLIndex].Redraw( drawer );
					widgets[skipSSLIndex + 1].Redraw( drawer );
				}
			} else {
				string text = "&eFailed to " + action + ":" +
					Environment.NewLine + ex.Status;
				SetStatus( text );
			}
		}
		
		void MakeSSLSkipValidationBoolean() {
			MakeBooleanAt( Anchor.Centre, Anchor.Centre, inputFont, true, 30, 30, 160, -20, SSLSkipValidationClick );
		}
		
		void MakeSSLSkipValidationLabel() {
			MakeLabelAt( "Skip SSL check", inputFont, Anchor.Centre, Anchor.Centre, 250, -20 );
		}
		
		void SSLSkipValidationClick( int mouseX, int mouseY ) {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LauncherBooleanWidget widget = (LauncherBooleanWidget)widgets[skipSSLIndex];
				SetBool( !widget.Value );
			}
		}
		
		void SetBool( bool value ) {
			LauncherBooleanWidget widget = (LauncherBooleanWidget)widgets[skipSSLIndex];
			widget.Value = value;
			widget.Redraw( game.Drawer );
			Dirty = true;
		}
		
		void StoreFields() {
			Dictionary<string, object> metadata;
			if( !game.ScreenMetadata.TryGetValue( "screen-CC", out metadata ) ) {
				metadata = new Dictionary<string, object>();
				game.ScreenMetadata["screen-CC"] = metadata;
			}
			metadata["user"] = Get( 0 );
			metadata["pass"] = Get( 1 );
		}
		
		void LoadSavedInfo( IDrawer2D drawer ) {
			Dictionary<string, object> metadata;
			// restore what user last typed into the various fields
			if( game.ScreenMetadata.TryGetValue( "screen-CC", out metadata ) ) {
				Set( 0, (string)metadata["user"] );
				Set( 1, (string)metadata["pass"] );
			} else {
				LoadFromOptions();
			}
		}
		
		void LoadFromOptions() {
			if( !Options.Load() )
				return;
			
			string user = Options.Get( "launcher-cc-username" ) ?? "";
			string pass = Options.Get( "launcher-cc-password" ) ?? "";
			pass = Secure.Decode( pass, user );
			
			Set( 0, user );
			Set( 1, pass );
		}
		
		void UpdateSignInInfo( string user, string password ) {
			// If the client has changed some settings in the meantime, make sure we keep the changes
			if( !Options.Load() )
				return;
			
			Options.Set( "launcher-cc-username", user );
			Options.Set( "launcher-cc-password", Secure.Encode( password, user ) );
			Options.Save();
		}
	}
}
