// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK;
using OpenTK.Input;

namespace Launcher {
	
	public abstract class LauncherInputScreen : LauncherScreen {
		
		protected Font titleFont, inputFont, inputHintFont;
		protected int enterIndex = -1;
		public LauncherInputScreen( LauncherWindow game, bool makeFonts ) : base( game ) {
			if( !makeFonts ) return;
			titleFont = new Font( game.FontName, 15, FontStyle.Bold );
			inputFont = new Font( game.FontName, 14, FontStyle.Regular );
			inputHintFont = new Font( game.FontName, 12, FontStyle.Italic );
		}
		
		public override void Init() {
			buttonFont = titleFont;
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			game.Window.Mouse.WheelChanged += MouseWheelChanged;
			
			game.Window.KeyPress += KeyPress;
			game.Window.Keyboard.KeyDown += KeyDown;
			game.Window.Keyboard.KeyUp += KeyUp;
			game.Window.Keyboard.KeyRepeat = true;
		}
		
		DateTime widgetOpenTime;
		bool lastCaretFlash = false;
		public override void Tick() {
			double elapsed = (DateTime.UtcNow - widgetOpenTime).TotalSeconds;
			bool caretShow = (elapsed % 1) < 0.5;
			if( caretShow == lastCaretFlash || lastInput == null )
				return;
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				lastInput.SetDrawData( drawer, lastInput.Text );
				lastInput.Redraw( drawer );
				if( caretShow )
					lastInput.DrawCaret( drawer, inputFont );
				Dirty = true;
			}
			lastCaretFlash = caretShow;
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
			if( lastInput == null ) {
				if( e.Key == Key.Escape )
					game.SetScreen( new MainScreen( game ) );
				return;
			}
			
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
				lastInput.AdvanceCursorPos( -1 );
				RedrawLastInput();
			} else if( e.Key == Key.Right ) {
				lastInput.AdvanceCursorPos( +1 );
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
		
		protected override void SelectWidget( LauncherWidget widget ) {
			base.SelectWidget( widget );
			if( widget is LauncherInputWidget ) {
				widgetOpenTime = DateTime.UtcNow;
				lastCaretFlash = false;
			}
		}
		
		protected virtual void RedrawLastInput() {
			if( lastInput.Width > lastInput.ButtonWidth )
				game.ClearArea( lastInput.X, lastInput.Y, lastInput.Width, lastInput.Height );
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				lastInput.Redraw( drawer );
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
			((LauncherInputWidget)widgets[index]).SetDrawData( drawer, text );
			((LauncherInputWidget)widgets[index]).Redraw( drawer );
		}
		
		protected virtual void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
		}
		
		protected LauncherInputWidget lastInput;
		protected virtual void InputClick( int mouseX, int mouseY ) {
			LauncherInputWidget input = (LauncherInputWidget)selectedWidget;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				if( lastInput != null ) {
					lastInput.Active = false;
					lastInput.Redraw( drawer );
				}
				
				input.Active = true;
				widgetOpenTime = DateTime.UtcNow;
				lastCaretFlash = false;
				input.SetCaretToCursor( mouseX, mouseY, drawer, inputFont );
				input.Redraw( drawer );
			}
			lastInput = input;
			Dirty = true;
		}
		
		protected override void WidgetUnclicked( LauncherWidget widget ) {
			LauncherInputWidget input = widget as LauncherInputWidget;
			if( input == null ) return;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				input.Active = false;
				input.Redraw( drawer );
			}
			lastInput = null;
			Dirty = true;
		}
		
		protected void MakeInput( string text, int width, Anchor verAnchor, bool password,
		                         int x, int y, int maxChars, string hint ) {
			MakeInput( text, width, Anchor.Centre, verAnchor, password, x, y, maxChars, hint );
		}
		
		protected void MakeInput( string text, int width, Anchor horAnchor, Anchor verAnchor,
		                         bool password, int x, int y, int maxChars, string hint ) {
			LauncherInputWidget widget;
			if( widgets[widgetIndex] != null ) {
				widget = (LauncherInputWidget)widgets[widgetIndex];
			} else {
				widget = new LauncherInputWidget( game );
				widget.OnClick = InputClick;
				widget.Password = password;
				widget.MaxTextLength = maxChars;
				widget.HintText = hint;
				widgets[widgetIndex] = widget;
			}
			
			widget.SetDrawData( drawer, text, inputFont, inputHintFont, horAnchor, verAnchor, width, 30, x, y );
			widgetIndex++;
		}
		
		public override void Dispose() {
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			game.Window.Mouse.WheelChanged -= MouseWheelChanged;
			
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
