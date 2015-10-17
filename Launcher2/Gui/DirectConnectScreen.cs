using System;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;
using OpenTK;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed class DirectConnectScreen : LauncherScreen {
		
		public DirectConnectScreen( LauncherWindow game ) : base( game ) {
			textFont = new Font( "Arial", 16, FontStyle.Bold );
			widgets = new LauncherWidget[9];
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			
			game.Window.KeyPress += KeyPress;
			game.Window.Keyboard.KeyDown += KeyDown;
			game.Window.Keyboard.KeyRepeat = true;
		}

		void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( lastInput != null && e.Key == Key.BackSpace ) {
				using( IDrawer2D drawer = game.Drawer ) {
					drawer.SetBitmap( game.Framebuffer );
					lastInput.RemoveChar( textFont );
					Dirty = true;
				}
			} else if( e.Key == Key.Enter ) { // Click connect button			
				LauncherWidget widget = widgets[6];
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

		public override void Init() {
			Resize();
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LoadSavedInfo( drawer );
				Dirty = true;
			}
		}
		
		void LoadSavedInfo( IDrawer2D drawer ) {
			try {
				Options.Load();
			} catch( IOException ) {
				return;
			}
			
			string user = Options.Get( "launcher-username" ) ?? "";
			string ip = Options.Get( "launcher-ip" ) ?? "127.0.0.1";
			string port = Options.Get( "launcher-port" ) ?? "25565";

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
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				DrawButtons( drawer );
			}
			Dirty = true;
		}
		
		Font textFont;
		static FastColour boxCol = new FastColour( 169, 143, 192 ), shadowCol = new FastColour( 97, 81, 110 );
		void DrawButtons( IDrawer2D drawer ) {
			widgetIndex = 0;
			MakeTextAt( drawer, "Username", -180, -100 );
			MakeTextAt( drawer, "Address", -180, -50 );
			MakeTextAt( drawer, "Mppass", -180, 0 );
			
			MakeTextInputAt( drawer, Get( widgetIndex ), 30, -100 );
			MakeTextInputAt( drawer, Get( widgetIndex ), 30, -50 );
			MakeTextInputAt( drawer, Get( widgetIndex ), 30, 0 );
			
			MakeButtonAt( drawer, "Connect", 110, 35, -65, 50, StartClient );			
			MakeButtonAt( drawer, "Back", 80, 35, 140, 50, () => game.SetScreen( new MainScreen( game ) ) );
			MakeTextAt( drawer, "", 0, 100 );
		}
		
		void StartClient() {
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
			if( !IPAddress.TryParse( ipPart, out ip ) ) {
				SetStatus( "&eInvalid ip" ); return;
			}
			string portPart = address.Substring( index + 1, address.Length - index - 1 ); ushort port;
			if( !UInt16.TryParse( portPart, out port ) ) {
				SetStatus( "&eInvalid port" ); return;
			}
			
			string mppass = Get( 5 );
			if( String.IsNullOrEmpty( mppass ) ) mppass = "(none)";
			GameStartData data = new GameStartData( Get( 3 ), mppass, ipPart, portPart );
			Client.Start( data, false );
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
				LauncherTextWidget widget = (LauncherTextWidget)widgets[8];
				drawer.Clear( LauncherWindow.clearColour, widget.X, widget.Y,
				             widget.Width, widget.Height );
				widget.DrawAt( drawer, text, textFont, Anchor.Centre, Anchor.Centre, 0, 100 );
				Dirty = true;
			}
		}

		void MakeTextAt( IDrawer2D drawer, string text, int x, int y ) {
			LauncherTextWidget widget = new LauncherTextWidget( game );
			widget.Text = text;
			
			widget.DrawAt( drawer, text, textFont, Anchor.Centre, Anchor.Centre, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeTextInputAt( IDrawer2D drawer, string text, int x, int y ) {
			LauncherTextInputWidget widget = new LauncherTextInputWidget( game );
			widget.OnClick = InputClick;
			
			widget.DrawAt( drawer, text, textFont, Anchor.Centre, Anchor.Centre, 300, 30, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeButtonAt( IDrawer2D drawer, string text, int width, int height,
		                  int x, int y, Action onClick ) {
			LauncherButtonWidget widget = new LauncherButtonWidget( game );
			widget.Text = text;
			widget.OnClick = onClick;
			
			widget.DrawAt( drawer, text, textFont, Anchor.Centre, Anchor.Centre, width, height, x, y );
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
	}
}
