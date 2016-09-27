// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Screens {
	public abstract class Screen {
		protected internal LauncherWindow game;
		protected internal IDrawer2D drawer;

		protected bool mouseMoved = false;
		static bool supressMove = true;
		
		public Screen( LauncherWindow game ) {
			this.game = game;
			drawer = game.Drawer;
		}
		
		/// <summary> Function called to setup the widgets and other data for this screen. </summary>
		public abstract void Init();
		
		/// <summary> Function that is repeatedly called multiple times every second. </summary>
		public abstract void Tick();
		
		/// <summary> Function called to redraw all widgets in this screen. </summary>
		public abstract void Resize();
		
		/// <summary> Cleans up all native resources held by this screen. </summary>
		public abstract void Dispose();
		
		/// <summary> Function called when the pixels from the framebuffer
		/// are about to be transferred to the window. </summary>
		public virtual void OnDisplay() { }
		
		protected Widget selectedWidget;
		protected Widget[] widgets;
		
		/// <summary> Called when the user has moved their mouse away from a previously selected widget. </summary>
		protected virtual void UnselectWidget( Widget widget ) {
			if( widget != null ) widget.Active = false;
			ChangedActiveState( widget );
		}
		
		/// <summary> Called when user has moved their mouse over a given widget. </summary>
		protected virtual void SelectWidget( Widget widget ) {
			if( widget != null ) widget.Active = true;
			ChangedActiveState( widget );
		}
		
		void ChangedActiveState( Widget widget ) {
			ButtonWidget button = widget as ButtonWidget;
			if( button != null )  {
				button.RedrawBackground();
				RedrawWidget( button );
			}
			
			LabelWidget label = widget as LabelWidget;
			if( label != null && label.DarkenWhenInactive ) {
				game.ResetArea( label.X, label.Y, label.Width, label.Height );
				RedrawWidget( label );
			}
		}
		
		/// <summary>Redraws the given widget and marks the window as needing to be redrawn. </summary>
		protected void RedrawWidget( Widget widget ) {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				widget.Redraw( drawer );
			}
			game.Dirty = true;
		}
		
		protected Widget lastClicked;
		protected void MouseButtonDown( object sender, MouseButtonEventArgs e ) {
			if( e.Button != MouseButton.Left ) return;
			
			if( lastClicked != null && lastClicked != selectedWidget )
				WidgetUnclicked( lastClicked );
			if( selectedWidget != null && selectedWidget.OnClick != null )
				selectedWidget.OnClick( e.X, e.Y );
			lastClicked = selectedWidget;
		}
		
		protected virtual void WidgetUnclicked( Widget widget ) {
		}
		
		protected virtual void MouseMove( object sender, MouseMoveEventArgs e ) {
			if( supressMove ) { supressMove = false; return; }
			mouseMoved = true;
			
			for( int i = 0; i < widgets.Length; i++ ) {
				Widget widget = widgets[i];
				if( widget == null || !widget.Visible ) continue;
				int width = widget.Width, height = widget.Height;
				if( widgets[i] is InputWidget )
					width = ((InputWidget)widgets[i]).RealWidth;
				
				if( e.X >= widget.X && e.Y >= widget.Y &&
				   e.X < widget.X + width && e.Y < widget.Y + height ) {
					if( selectedWidget == widget ) return;
					
					if( selectedWidget != null )
						UnselectWidget( selectedWidget );
					SelectWidget( widget );
					selectedWidget = widget;
					return;
				}
			}
			
			if( selectedWidget == null ) return;
			UnselectWidget( selectedWidget );
			selectedWidget = null;
		}
		
		protected bool tabDown = false;
		MouseMoveEventArgs moveArgs = new MouseMoveEventArgs();
		MouseButtonEventArgs pressArgs = new MouseButtonEventArgs();
		protected void HandleTab() {
			if( tabDown ) return;
			tabDown = true;
			int index = lastClicked == null ? -1 :
				Array.IndexOf<Widget>( widgets, lastClicked );
			int dir = (game.Window.Keyboard[Key.ShiftLeft]
			           || game.Window.Keyboard[Key.ShiftRight]) ? -1 : 1;
			index += dir;
			Utils.Clamp( ref index, 0, widgets.Length - 1 );
			
			for( int j = 0; j < widgets.Length * 2; j++ ) {
				int i = (j * dir + index) % widgets.Length;
				if( i < 0 ) i += widgets.Length;
				if( !widgets[i].Visible || !widgets[i].TabSelectable ) continue;
				
				Widget widget = widgets[i];
				int width = widget.Width;
				if( widgets[i] is InputWidget )
					width = ((InputWidget)widgets[i]).RealWidth;
				moveArgs.X = widget.X + width / 2;
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
				
				if( widgets[i] is InputWidget ) {
					MouseButtonDown( null, pressArgs );
					((InputWidget)widgets[i]).Chars.CaretPos = -1;
				}
				break;
			}
		}
	}
}
