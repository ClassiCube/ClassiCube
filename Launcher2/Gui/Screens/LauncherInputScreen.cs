using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;
using OpenTK;
using OpenTK.Input;

namespace Launcher2 {
	
	public abstract class LauncherInputScreen : LauncherScreen {
		
		protected Font titleFont, inputFont;
		protected int enterIndex = -1;
		public LauncherInputScreen( LauncherWindow game ) : base( game ) {
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			
			game.Window.KeyPress += KeyPress;
			game.Window.Keyboard.KeyDown += KeyDown;
			game.Window.Keyboard.KeyRepeat = true;
		}

		protected void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( lastInput != null && e.Key == Key.BackSpace ) {
				using( IDrawer2D drawer = game.Drawer ) {
					drawer.SetBitmap( game.Framebuffer );
					lastInput.RemoveChar( inputFont );
					Dirty = true;
				}
			} else if( e.Key == Key.Enter && enterIndex >= 0 ) {
				LauncherWidget widget = widgets[enterIndex];
				if( widget.OnClick != null )
					widget.OnClick();
			}
		}

		protected void KeyPress( object sender, KeyPressEventArgs e ) {
			if( lastInput != null ) {
				using( IDrawer2D drawer = game.Drawer ) {
					drawer.SetBitmap( game.Framebuffer );
					lastInput.AddChar( e.KeyChar, inputFont );
					Dirty = true;
				}
			}
		}
		
		protected string Get( int index ) {
			LauncherWidget widget = widgets[index];
			return widget == null ? "" : ((widget as LauncherTextInputWidget)).Text;
		}
		
		protected void Set( int index, string text ) {
			(widgets[index] as LauncherTextInputWidget)
				.Redraw( game.Drawer, text, inputFont );
		}
		
		protected override void UnselectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			if( button != null ) {
				button.Redraw( game.Drawer, button.Text, titleFont );
				FilterArea( widget.X, widget.Y, widget.Width, widget.Height, 180 );
				Dirty = true;
			}
		}
		
		protected override void SelectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			if( button != null ) {
				button.Redraw( game.Drawer, button.Text, titleFont );
				Dirty = true;
			}
		}
		
		protected LauncherTextInputWidget lastInput;
		protected void InputClick() {
			LauncherTextInputWidget input = selectedWidget as LauncherTextInputWidget;
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				if( lastInput != null ) {
					lastInput.Active = false;
					lastInput.Redraw( game.Drawer, lastInput.Text, inputFont );
				}
				
				input.Active = true;
				input.Redraw( game.Drawer, input.Text, inputFont );
			}
			lastInput = input;
			Dirty = true;
		}
		
		public override void Dispose() {
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			
			game.Window.KeyPress -= KeyPress;
			game.Window.Keyboard.KeyDown -= KeyDown;
			game.Window.Keyboard.KeyRepeat = false;
			
			titleFont.Dispose();
			inputFont.Dispose();
		}
	}
}
