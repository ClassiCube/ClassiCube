using System;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher2 {
	
	public abstract class LauncherScreen {
		protected LauncherWindow game;
		
		public bool Dirty;
		protected int widgetIndex;
		
		public LauncherScreen( LauncherWindow game ) {
			this.game = game;
		}
		
		public abstract void Init();
		
		public abstract void Resize();
		
		public abstract void Dispose();
		
		static uint clearColourBGRA = (uint)LauncherWindow.clearColour.ToArgb();
		protected unsafe void FilterArea( int x, int y, int width, int height, byte scale ) {
			using( FastBitmap bmp = new FastBitmap( game.Framebuffer, true ) ) {
				for( int yy = y; yy < y + height; yy++ ) {
					int* row = bmp.GetRowPtr( yy ) + x;
					for( int xx = 0; xx < width; xx++ ) {
						uint pixel = (uint)row[xx];
						if( pixel == clearColourBGRA ) continue;
						
						uint a = pixel & 0xFF000000;
						uint r = (pixel >> 16) & 0xFF;
						uint g = (pixel >> 8) & 0xFF;
						uint b = pixel & 0xFF;
						
						r = (r * scale) / 255;
						g = (g * scale) / 255;
						b = (b * scale) / 255;
						row[xx] = (int)(a | (r << 16) | (g << 8) | b);
					}
				}
			}
		}
		
		protected LauncherWidget selectedWidget;
		protected LauncherWidget[] widgets;
		protected void MouseMove( object sender, MouseMoveEventArgs e ) {		
			for( int i = 0; i < widgets.Length; i++ ) {
				LauncherWidget widget = widgets[i];
				if( e.X >= widget.X && e.Y >= widget.Y &&
				   e.X < widget.X + widget.Width && e.Y < widget.Y + widget.Height ) {
					if( selectedWidget == widget ) return;
					
					using( IDrawer2D drawer = game.Drawer ) {
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
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				UnselectWidget( selectedWidget );
			}
			selectedWidget = null;
		}
		
		protected virtual void UnselectWidget( LauncherWidget widget ) {
		}
		
		protected virtual void SelectWidget( LauncherWidget widget ) {			
		}
		
		protected void MouseButtonDown( object sender, MouseButtonEventArgs e ) {
			if( e.Button != MouseButton.Left ) return;
			
			if( selectedWidget != null && selectedWidget.OnClick != null )
				selectedWidget.OnClick();
		}
	}
}
