using System;
using System.Drawing;
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
		
		/// <summary> Function that is repeatedly called multiple times every second. </summary>
		public abstract void Tick();
		
		public abstract void Resize();
		
		/// <summary> Cleans up all native resources held by this screen. </summary>
		public abstract void Dispose();
		
		protected static uint clearColourBGRA = (uint)LauncherWindow.clearColour.ToArgb();
		protected unsafe void FilterArea( int x, int y, int width, int height, byte scale ) {
			FilterArea( x, y, width, height, scale, clearColourBGRA );
		}
		
		/// <summary> Scales the RGB components of the bitmap in the specified region by the given amount. </summary>
		/// <remarks> Pixels with same value as clearColour are left untouched. </remarks>
		protected unsafe void FilterArea( int x, int y, int width, int height, 
		                                 byte scale, uint clearColour ) {
			Bitmap buffer = game.Framebuffer;
			if( x >= buffer.Width || y >= buffer.Height ) return;
			width = Math.Min( x + width, buffer.Width ) - x;
			height = Math.Min( y + height, buffer.Height ) - y;
			
			using( FastBitmap bmp = new FastBitmap( buffer, true ) ) {
				for( int yy = y; yy < y + height; yy++ ) {
					int* row = bmp.GetRowPtr( yy ) + x;
					for( int xx = 0; xx < width; xx++ ) {
						uint pixel = (uint)row[xx];
						if( pixel == clearColour ) continue;
						
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
				if( widget == null ) continue;
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
		
		/// <summary> Called when the user has moved their mouse away from a previously selected widget. </summary>
		protected virtual void UnselectWidget( LauncherWidget widget ) {
		}
		
		/// <summary> Called when user has moved their mouse over a given widget. </summary>
		protected virtual void SelectWidget( LauncherWidget widget ) {			
		}
		
		protected void MouseButtonDown( object sender, MouseButtonEventArgs e ) {
			if( e.Button != MouseButton.Left || selectedWidget == null ) return;
			
			if( selectedWidget.OnClick != null )
				selectedWidget.OnClick();
		}
	}
}
