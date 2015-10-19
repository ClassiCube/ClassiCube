using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;
using OpenTK;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed class ClassiCubeServersScreen : LauncherScreen {
		
		public ClassiCubeServersScreen( LauncherWindow game ) : base( game ) {
			textFont = new Font( "Arial", 16, FontStyle.Bold );
			widgets = new LauncherWidget[5];
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			
			game.Window.KeyPress += KeyPress;
			game.Window.Keyboard.KeyDown += KeyDown;
			game.Window.Keyboard.KeyRepeat = true;
		}
		
		public override void Tick() {
		}

		void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( lastInput != null && e.Key == Key.BackSpace ) {
				using( IDrawer2D drawer = game.Drawer ) {
					drawer.SetBitmap( game.Framebuffer );
					lastInput.RemoveChar( textFont );
					Dirty = true;
				}
			} else if( e.Key == Key.Enter ) { // Click sign in button
				LauncherWidget widget = widgets[4];
				if( widget.OnClick != null )
					widget.OnClick();
			}
		}

		void KeyPress( object sender, KeyPressEventArgs e ) {
			if( lastInput != null ) {
				using( IDrawer2D drawer = game.Drawer ) {
					drawer.SetBitmap( game.Framebuffer );
					lastInput.AddChar( e.KeyChar, textFont );
					Dirty = true;
				}
			}
		}

		public override void Init() { Resize(); }
		
		public override void Resize() {
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.Clear( LauncherWindow.clearColour );
				DrawButtons( drawer );
			}
			Dirty = true;
		}
		
		Font textFont;
		static FastColour boxCol = new FastColour( 169, 143, 192 ), shadowCol = new FastColour( 97, 81, 110 );
		void DrawButtons( IDrawer2D drawer ) {
			widgetIndex = 0;
			MakeTextAt( drawer, "Search", -180, 0 );
			MakeTextInputAt( drawer, false, Get( widgetIndex ), 30, 0 );
			
			MakeTextAt( drawer, "classicube.net/server/play/", -320, 50 );
			MakeTextInputAt( drawer, false, Get( widgetIndex ), 30, 50 );
			
			MakeButtonAt( drawer, "Back", 80, 35, 180, 0, () => game.SetScreen( new MainScreen( game ) ) );
		}
		
		ClassicubeSession session = new ClassicubeSession();
		List<ServerListEntry> servers = new List<ServerListEntry>();
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
				servers = session.GetPublicServers();
			} catch( WebException ex ) {
				servers = new List<ServerListEntry>();
				DisplayWebException( ex, "retrieve servers list" );
				return;
			}
			SetStatus( "&eSigned in" );
		}
		
		string Get( int index ) {
			LauncherWidget widget = widgets[index];
			return widget == null ? "" : ((widget as LauncherTextInputWidget)).Text;
		}
		
		void Set( int index, string text ) {
			(widgets[index] as LauncherTextInputWidget)
				.Redraw( game.Drawer, text, textFont );
		}
		
		void SetStatus( string text ) {
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LauncherTextWidget widget = (LauncherTextWidget)widgets[6];
				drawer.Clear( LauncherWindow.clearColour, widget.X, widget.Y,
				             widget.Width, widget.Height );
				widget.DrawAt( drawer, text, textFont, Anchor.Centre, Anchor.Centre, 0, 50 );
				Dirty = true;
			}
		}

		void MakeTextAt( IDrawer2D drawer, string text, int x, int y ) {
			LauncherTextWidget widget = new LauncherTextWidget( game, text );			
			widget.DrawAt( drawer, text, textFont, Anchor.Centre, Anchor.LeftOrTop, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeTextInputAt( IDrawer2D drawer, bool password, string text, int x, int y ) {
			LauncherTextInputWidget widget = new LauncherTextInputWidget( game );
			widget.OnClick = InputClick;
			widget.Password = password;
			
			widget.DrawAt( drawer, text, textFont, Anchor.Centre, Anchor.LeftOrTop, 300, 30, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeButtonAt( IDrawer2D drawer, string text, int width, int height,
		                  int x, int y, Action onClick ) {
			LauncherButtonWidget widget = new LauncherButtonWidget( game );
			widget.Text = text;
			widget.OnClick = onClick;
			
			widget.DrawAt( drawer, text, textFont, Anchor.Centre, Anchor.LeftOrTop, width, height, x, y );
			FilterArea( widget.X, widget.Y, widget.Width, widget.Height, 180 );
			widgets[widgetIndex++] = widget;
		}
		
		protected override void UnselectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			if( button != null ) {
				button.Redraw( game.Drawer, button.Text, textFont );
				FilterArea( widget.X, widget.Y, widget.Width, widget.Height, 180 );
				Dirty = true;
			}
		}
		
		protected override void SelectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			if( button != null ) {
				button.Redraw( game.Drawer, button.Text, textFont );
				Dirty = true;
			}
		}
		
		LauncherTextInputWidget lastInput;
		void InputClick() {
			LauncherTextInputWidget input = selectedWidget as LauncherTextInputWidget;
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				if( lastInput != null ) {
					lastInput.Active = false;
					lastInput.Redraw( game.Drawer, lastInput.Text, textFont );
				}
				
				input.Active = true;
				input.Redraw( game.Drawer, input.Text, textFont );
			}
			lastInput = input;
			Dirty = true;
		}
		
		public override void Dispose() {
			textFont.Dispose();
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			
			game.Window.KeyPress -= KeyPress;
			game.Window.Keyboard.KeyDown -= KeyDown;
			game.Window.Keyboard.KeyRepeat = false;
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
