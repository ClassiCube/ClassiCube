using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK;
using OpenTK.Input;

namespace Launcher2 {
	
	public abstract class LauncherInputScreen : LauncherScreen {
		
		protected Font titleFont, inputFont, inputHintFont;
		protected int enterIndex = -1;
		public LauncherInputScreen( LauncherWindow game, bool makeFonts ) : base( game ) {
			if( !makeFonts ) return;
			titleFont = new Font( "Arial", 15, FontStyle.Bold );
			inputFont = new Font( "Arial", 14, FontStyle.Regular );
			inputHintFont = new Font( "Arial", 12, FontStyle.Italic );
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
			if( e.Key == Key.Enter && enterIndex >= 0 ) {
				LauncherWidget widget = (selectedWidget != null && mouseMoved) ?
					selectedWidget : widgets[enterIndex];
				if( widget.OnClick != null )
					widget.OnClick( 0, 0 );
			} else if( e.Key == Key.Tab ) {
				HandleTab();
			}
			if( lastInput == null ) return;
			
			
			if( e.Key == Key.BackSpace && lastInput.BackspaceChar() ) {
				RedrawLastInput();
				OnRemovedChar();
			} else if( e.Key == Key.Delete && lastInput.DeleteChar() ) {
				RedrawLastInput();
				OnRemovedChar();
			} else if( e.Key == Key.C && ControlDown ) {
				lastInput.CopyToClipboard();
			} else if( e.Key == Key.V && ControlDown ) {
				if( lastInput.CopyFromClipboard() )
					RedrawLastInput();
			} else if( e.Key == Key.Escape ) {
				if( lastInput.ClearText() )
					RedrawLastInput();
			} else if( e.Key == Key.Left ) {
				lastInput.ChangeCursorPos( -1 );
				RedrawLastInput();
			} else if( e.Key == Key.Right ) {
				lastInput.ChangeCursorPos( +1 );
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
				lastInput.Redraw( drawer, lastInput.Text, inputFont, inputHintFont );
				lastInput.DrawCursor( inputFont, drawer );
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
				.Redraw( drawer, text, inputFont, inputHintFont );
		}
		
		protected LauncherInputWidget lastInput;
		protected virtual void InputClick( int mouseX, int mouseY ) {
			LauncherInputWidget input = (LauncherInputWidget)selectedWidget;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				if( lastInput != null ) {
					lastInput.Active = false;
					lastInput.Redraw( drawer, lastInput.Text, inputFont, inputHintFont );
				}
				
				input.Active = true;
				input.Redraw( drawer, input.Text, inputFont, inputHintFont );
			}
			lastInput = input;
			Dirty = true;
		}
		
		protected void MakeInput( string text, int width, Anchor verAnchor, bool password,
		                         int x, int y, int maxChars, string hint ) {
			MakeInput( text, width, Anchor.Centre, verAnchor, password, x, y, maxChars, hint );
		}
		
		protected void MakeInput( string text, int width, Anchor horAnchor, Anchor verAnchor,
		                         bool password, int x, int y, int maxChars, string hint ) {
			if( widgets[widgetIndex] != null ) {
				LauncherInputWidget input = (LauncherInputWidget)widgets[widgetIndex];
				input.DrawAt( drawer, text, inputFont, inputHintFont, horAnchor, verAnchor, width, 30, x, y );
				widgetIndex++;
				return;
			}
			
			LauncherInputWidget widget = new LauncherInputWidget( game );
			widget.OnClick = InputClick;
			widget.Password = password;
			widget.MaxTextLength = maxChars;
			widget.HintText = hint;
			
			widget.DrawAt( drawer, text, inputFont, inputHintFont, horAnchor, verAnchor, width, 30, x, y );
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
			inputHintFont.Dispose();
		}
	}
}
