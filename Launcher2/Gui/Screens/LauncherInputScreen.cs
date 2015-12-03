using System;
using System.Drawing;
using OpenTK;
using OpenTK.Input;
using ClassicalSharp;

namespace Launcher2 {
	
	public abstract class LauncherInputScreen : LauncherScreen {
		
		protected Font titleFont, inputFont;
		protected int enterIndex = -1;
		public LauncherInputScreen( LauncherWindow game ) : base( game ) {
		}
		
		public override void Init() {
			buttonFont = titleFont;
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			
			game.Window.KeyPress += KeyPress;
			game.Window.Keyboard.KeyDown += KeyDown;
			game.Window.Keyboard.KeyUp += KeyUp;
			game.Window.Keyboard.KeyRepeat = true;
		}
		
		protected virtual void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( lastInput != null && e.Key == Key.BackSpace ) {
				if( lastInput.RemoveChar() ) {
					RedrawLastInput();
					OnRemovedChar();
				}
			} else if( e.Key == Key.Enter && enterIndex >= 0 ) {
				LauncherWidget widget = widgets[enterIndex];
				if( widget.OnClick != null )
					widget.OnClick( 0, 0 );
			} else if( e.Key == Key.Tab ) {
				HandleTab();
			} else if( lastInput != null && e.Key == Key.C && ControlDown ) {
				lastInput.CopyToClipboard();
			} else if( lastInput != null && e.Key == Key.V && ControlDown ) {
				if( lastInput.CopyFromClipboard() )
					RedrawLastInput();
			} else if( lastInput != null && e.Key == Key.Escape ) {
				if( lastInput.ClearText() )
					RedrawLastInput();
			}
		}
		
		bool ControlDown {
			get {
				KeyboardDevice keyboard = game.Window.Keyboard;
				return keyboard[Key.ControlLeft] || keyboard[Key.ControlRight];
			}
		}
		
		protected void KeyUp( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Tab )
				tabDown = false;
		}

		protected void KeyPress( object sender, KeyPressEventArgs e ) {
			if( lastInput != null && lastInput.AppendChar( e.KeyChar ) ) {
				RedrawLastInput();
				OnAddedChar();
			}
		}
		
		protected virtual void RedrawLastInput() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				if( lastInput.Width > lastInput.ButtonWidth )
					game.ClearArea( lastInput.X, lastInput.Y,
					               lastInput.Width + 1, lastInput.Height + 1 );
				lastInput.Redraw( drawer, lastInput.Text, inputFont );
				Dirty = true;
			}
		}
		
		protected virtual void OnAddedChar() { }
		
		protected virtual void OnRemovedChar() { }
		
		protected string Get() { return Get( widgetIndex ); }
		
		protected string Get( int index ) { 
			LauncherWidget widget = widgets[index];
			return widget == null ? "" : widget.Text;
		}
		
		protected void Set( int index, string text ) {
			((LauncherInputWidget)widgets[index])
				.Redraw( drawer, text, inputFont );
		}
		
		protected LauncherInputWidget lastInput;
		protected virtual void InputClick( int mouseX, int mouseY ) {
			LauncherInputWidget input = (LauncherInputWidget)selectedWidget;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				if( lastInput != null ) {
					lastInput.Active = false;
					lastInput.Redraw( drawer, lastInput.Text, inputFont );
				}
				
				input.Active = true;
				input.Redraw( drawer, input.Text, inputFont );
			}
			lastInput = input;
			Dirty = true;
		}
		
		protected void MakeInput( string text, int width, Anchor verAnchor, bool password,
		                         int x, int y, int maxChars ) {
			if( widgets[widgetIndex] != null ) {
				LauncherInputWidget input = (LauncherInputWidget)widgets[widgetIndex];
				input.DrawAt( drawer, text, inputFont, Anchor.Centre, verAnchor, width, 30, x, y );
				widgetIndex++;
				return;
			}
			
			LauncherInputWidget widget = new LauncherInputWidget( game );
			widget.OnClick = InputClick;
			widget.Password = password;
			widget.MaxTextLength = maxChars;
			
			widget.DrawAt( drawer, text, inputFont, Anchor.Centre, verAnchor, width, 30, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		public override void Dispose() {
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			
			game.Window.KeyPress -= KeyPress;
			game.Window.Keyboard.KeyDown -= KeyDown;
			game.Window.Keyboard.KeyUp -= KeyUp;
			game.Window.Keyboard.KeyRepeat = false;
			
			titleFont.Dispose();
			inputFont.Dispose();
		}
	}
}
