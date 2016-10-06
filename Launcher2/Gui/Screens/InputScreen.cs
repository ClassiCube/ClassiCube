// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;
using OpenTK;
using OpenTK.Input;

namespace Launcher.Gui.Screens {	
	public abstract class InputScreen : Screen {
		
		protected int enterIndex = -1;
		public InputScreen( LauncherWindow game ) : base( game ) { }
		
		public override void Init() {
			base.Init();
			game.Window.Mouse.WheelChanged += MouseWheelChanged;
			game.Window.KeyPress += KeyPress;
			game.Window.Keyboard.KeyRepeat = true;
		}
		
		DateTime widgetOpenTime;
		bool lastCaretFlash = false;
		Rectangle lastRec;
		public override void Tick() {
			double elapsed = (DateTime.UtcNow - widgetOpenTime).TotalSeconds;
			bool caretShow = (elapsed % 1) < 0.5;
			if( caretShow == lastCaretFlash || curInput == null )
				return;
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				curInput.SetDrawData( drawer, curInput.Text );
				curInput.Redraw( drawer );
				
				Rectangle r = curInput.MeasureCaret( drawer );
				if( caretShow ) {
					drawer.Clear( FastColour.Black, r.X, r.Y, r.Width, r.Height );
				}
				
				if( lastRec == r ) game.DirtyArea = r;
				lastRec = r;
				game.Dirty = true;
			}
			lastCaretFlash = caretShow;
		}
		
		protected override void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Enter && enterIndex >= 0 ) {
				Widget widget = (selectedWidget != null && mouseMoved) ?
					selectedWidget : widgets[enterIndex];
				if( widget.OnClick != null )
					widget.OnClick( 0, 0 );
			} else if( e.Key == Key.Tab ) {
				HandleTab();
			}
			if( curInput == null ) {
				if( e.Key == Key.Escape )
					game.SetScreen( new MainScreen( game ) );
				return;
			}
			
			if( e.Key == Key.BackSpace && curInput.Chars.Backspace() ) {
				RedrawLastInput();
				OnRemovedChar();
			} else if( e.Key == Key.Delete && curInput.Chars.Delete() ) {
				RedrawLastInput();
				OnRemovedChar();
			} else if( e.Key == Key.C && ControlDown ) {
				curInput.Chars.CopyToClipboard();
			} else if( e.Key == Key.V && ControlDown ) {
				if( curInput.Chars.CopyFromClipboard() )
					RedrawLastInput();
			} else if( e.Key == Key.Escape ) {
				if( curInput.Chars.Clear() )
					RedrawLastInput();
			} else if( e.Key == Key.Left ) {
				curInput.AdvanceCursorPos( false );
				RedrawLastInput();
			} else if( e.Key == Key.Right ) {
				curInput.AdvanceCursorPos( true );
				RedrawLastInput();
			}
		}
		
		bool ControlDown {
			get {
				KeyboardDevice keyboard = game.Window.Keyboard;
				return keyboard[Key.ControlLeft] || keyboard[Key.ControlRight];
			}
		}

		protected void KeyPress( object sender, KeyPressEventArgs e ) {
			if( curInput != null && curInput.Chars.Append( e.KeyChar ) ) {
				RedrawLastInput();
				OnAddedChar();
			}
		}
		
		protected virtual void RedrawLastInput() {
			if( curInput.RealWidth > curInput.ButtonWidth )
				game.ResetArea( curInput.X, curInput.Y, curInput.RealWidth, curInput.Height );
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				curInput.Redraw( drawer );
				game.Dirty = true;
			}
		}
		
		protected virtual void OnAddedChar() { }
		
		protected virtual void OnRemovedChar() { }
		
		protected string Get( int index ) {
			Widget widget = widgets[index];
			return widget == null ? "" : widget.Text;
		}
		
		protected void Set( int index, string text ) {
			((InputWidget)widgets[index]).SetDrawData( drawer, text );
			((InputWidget)widgets[index]).Redraw( drawer );
		}
		
		protected virtual void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
		}
		
		protected InputWidget curInput;
		protected virtual void InputClick( int mouseX, int mouseY ) {
			InputWidget input = (InputWidget)selectedWidget;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				if( curInput != null ) {
					curInput.Active = false;
					curInput.Redraw( drawer );
				}
				
				input.Active = true;
				widgetOpenTime = DateTime.UtcNow;
				lastCaretFlash = false;
				input.SetCaretToCursor( mouseX, mouseY, drawer );
				input.Redraw( drawer );
			}
			curInput = input;
			game.Dirty = true;
		}
		
		protected override void WidgetUnclicked( Widget widget ) {
			InputWidget input = widget as InputWidget;
			if( input == null ) return;
			input.Active = false;
			RedrawWidget( input );
			curInput = null;
			game.Dirty = true;
		}
		
		protected void SetupInputHandlers() {
			for( int i = 0; i < widgets.Length; i++ ) {
				if( !(widgets[i] is InputWidget) ) continue;
				widgets[i].OnClick = InputClick;
			}
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Window.Mouse.WheelChanged -= MouseWheelChanged;			
			game.Window.KeyPress -= KeyPress;
			game.Window.Keyboard.KeyRepeat = false;
		}
	}
}
