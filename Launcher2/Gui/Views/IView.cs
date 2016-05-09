// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher {
	
	public abstract class IView {
		protected internal LauncherWindow game;
		protected internal IDrawer2D drawer;
		
		public bool Dirty;
		protected int widgetIndex;
		protected LauncherWidget[] widgets;
		
		public IView( LauncherWindow game ) {
			this.game = game;
			drawer = game.Drawer;
		}
		
		/// <summary> Function called to setup the widgets and other data for this view. </summary>
		public abstract void Init();
		
		/// <summary> Function called to redraw all widgets in this view. </summary>
		public abstract void Resize();
		
		/// <summary> Cleans up all native resources held by this view. </summary>
		public abstract void Dispose();
		
		protected void RedrawAllButtonBackgrounds() {
			int buttons = 0;
			for( int i = 0; i < widgets.Length; i++ ) {
				if( widgets[i] == null || !(widgets[i] is LauncherButtonWidget) ) continue;
				buttons++;
			}
			if( buttons == 0 ) return;
			
			using( FastBitmap dst = new FastBitmap( game.Framebuffer, true, false ) ) {
				for( int i = 0; i < widgets.Length; i++ ) {
					if( widgets[i] == null ) continue;
					LauncherButtonWidget button = widgets[i] as LauncherButtonWidget;
					if( button != null )
						button.RedrawBackground( dst );
				}
			}
		}
		
		protected void RedrawAll() {
			for( int i = 0; i < widgets.Length; i++ ) {
				if( widgets[i] == null ) continue;
				widgets[i].Redraw( drawer );
			}
		}
	}
}
