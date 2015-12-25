using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher2 {
	
	public abstract class LauncherScreen {
		protected LauncherWindow game;
		protected IDrawer2D drawer;
		
		public bool Dirty;
		protected int widgetIndex;
		protected bool mouseMoved = false;
		static bool supressMove = true;
		
		public LauncherScreen( LauncherWindow game ) {
			this.game = game;
			drawer = game.Drawer;
		}
		
		public abstract void Init();
		
		/// <summary> Function that is repeatedly called multiple times every second. </summary>
		public abstract void Tick();
		
		public abstract void Resize();
		
		/// <summary> Cleans up all native resources held by this screen. </summary>
		public abstract void Dispose();
		
		protected LauncherWidget selectedWidget;
		protected LauncherWidget[] widgets;
		protected virtual void MouseMove( object sender, MouseMoveEventArgs e ) {
			if( supressMove ) {
				supressMove = false;
				return;
			}
			mouseMoved = true;
			for( int i = 0; i < widgets.Length; i++ ) {
				LauncherWidget widget = widgets[i];
				if( widget == null ) continue;
				if( e.X >= widget.X && e.Y >= widget.Y &&
				   e.X < widget.X + widget.Width && e.Y < widget.Y + widget.Height ) {
					if( selectedWidget == widget ) return;
					
					using( drawer ) {
						drawer.SetBitmap( game.Framebuffer );
						if( selectedWidget != null )
							UnselectWidget( selectedWidget );
						SelectWidget( widget );
					}
					selectedWidget = widget;
					return;
				}
			}
			
			if( selectedWidget == null ) return;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				UnselectWidget( selectedWidget );
			}
			selectedWidget = null;
		}
		
		protected Font buttonFont;
		
		/// <summary> Called when the user has moved their mouse away from a previously selected widget. </summary>
		protected virtual void UnselectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			if( button != null ) {
				button.Active = false;
				button.Redraw( drawer, button.Text, buttonFont );
				Dirty = true;
			}
		}
		
		/// <summary> Called when user has moved their mouse over a given widget. </summary>
		protected virtual void SelectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			if( button != null ) {
				button.Active = true;
				button.Redraw( drawer, button.Text, buttonFont );
				Dirty = true;
			}
		}
		
		protected LauncherWidget lastClicked;
		protected void MouseButtonDown( object sender, MouseButtonEventArgs e ) {
			if( e.Button != MouseButton.Left || selectedWidget == null ) return;
			
			if( selectedWidget.OnClick != null )
				selectedWidget.OnClick( e.X, e.Y );
			lastClicked = selectedWidget;
		}
		
		protected bool tabDown = false;
		MouseMoveEventArgs moveArgs = new MouseMoveEventArgs();
		MouseButtonEventArgs pressArgs = new MouseButtonEventArgs();
		protected void HandleTab() {
			if( tabDown ) return;
			tabDown = true;
			int index = lastClicked == null ? -1 :
				Array.IndexOf<LauncherWidget>( widgets, lastClicked );
			int dir = (game.Window.Keyboard[Key.ShiftLeft] 
			           || game.Window.Keyboard[Key.ShiftRight]) ? -1 : 1;
			index += dir;
			Utils.Clamp( ref index, 0, widgets.Length - 1);
			
			for( int j = 0; j < widgets.Length * 2; j++ ) {
				int i = (j * dir + index) % widgets.Length;
				if( i < 0 ) i += widgets.Length;
				
				if( widgets[i] is LauncherInputWidget || widgets[i] is LauncherButtonWidget ) {
					LauncherWidget widget = widgets[i];
					moveArgs.X = widget.X + widget.Width / 2;
					moveArgs.Y = widget.Y + widget.Height / 2;
					
					pressArgs.Button = MouseButton.Left;
					pressArgs.IsPressed = true;
					pressArgs.X = moveArgs.X;
					pressArgs.Y = moveArgs.Y;
					
					MouseMove( null, moveArgs );
					Point p = game.Window.PointToScreen( Point.Empty );
					p.Offset( moveArgs.X, moveArgs.Y );
					game.Window.DesktopCursorPos = p;
					lastClicked = widget;
					
					if( widgets[i] is LauncherInputWidget )
						MouseButtonDown( null, pressArgs );
					break;
				}
			}
		}
		
		protected void MakeButtonAt( string text, int width, Font font,
		                            Anchor verAnchor, int x, int y, Action<int, int> onClick ) {
			MakeButtonAt( text, width, font, Anchor.Centre, verAnchor, x, y, onClick );
		}
		
		protected void MakeButtonAt( string text, int width, Font font, Anchor horAnchor,
		                            Anchor verAnchor, int x, int y, Action<int, int> onClick ) {
			if( widgets[widgetIndex] != null ) {
				LauncherButtonWidget button = (LauncherButtonWidget)widgets[widgetIndex];
				button.Active = false;
				button.DrawAt( drawer, text, font, horAnchor, verAnchor, width, x, y );
				widgetIndex++;
				return;
			}
			
			LauncherButtonWidget widget = new LauncherButtonWidget( game );
			widget.Text = text;
			widget.OnClick = onClick;
			
			widget.Active = false;
			widget.DrawAt( drawer, text, font, horAnchor, verAnchor, width, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		protected void MakeLabelAt( string text, Font font, Anchor horAnchor, Anchor verAnchor, int x, int y ) {
			if( widgets[widgetIndex] != null ) {
				LauncherLabelWidget input = (LauncherLabelWidget)widgets[widgetIndex];
				input.DrawAt( drawer, text, font, horAnchor, verAnchor, x, y );
			} else {
				LauncherLabelWidget widget = new LauncherLabelWidget( game, text );
				widget.DrawAt( drawer, text, font, horAnchor, verAnchor, x, y );
				widgets[widgetIndex] = widget;
			}
			widgetIndex++;
		}
		
		protected void MakeBooleanAt( Anchor horAnchor, Anchor verAnchor, Font font,
		                             int width, int height, int x, int y, Action<int, int> onClick ) {
			if( widgets[widgetIndex] != null ) {
				LauncherBooleanWidget input = (LauncherBooleanWidget)widgets[widgetIndex];
				input.DrawAt( drawer, horAnchor, verAnchor, x, y );
			} else {
				LauncherBooleanWidget widget = new LauncherBooleanWidget( game, font, width, height );
				widget.DrawAt( drawer, horAnchor, verAnchor, x, y );
				widget.OnClick = onClick;
				widgets[widgetIndex] = widget;
			}
			widgetIndex++;
		}
	}
}
