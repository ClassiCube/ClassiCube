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
		
		/// <summary> Called when the user has moved their mouse away from a previously selected widget. </summary>
		protected virtual void UnselectWidget( LauncherWidget widget ) {
		}
		
		/// <summary> Called when user has moved their mouse over a given widget. </summary>
		protected virtual void SelectWidget( LauncherWidget widget ) {			
		}
		
		protected void MouseButtonDown( object sender, MouseButtonEventArgs e ) {
			if( e.Button != MouseButton.Left || selectedWidget == null ) return;
			
			if( selectedWidget.OnClick != null )
				selectedWidget.OnClick( e.X, e.Y );
		}
	}
}
