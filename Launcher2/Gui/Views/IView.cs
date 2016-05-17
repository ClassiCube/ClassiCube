// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Views {
	
	public abstract class IView {
		protected internal LauncherWindow game;
		protected internal IDrawer2D drawer;
		
		internal int widgetIndex;
		internal LauncherWidget[] widgets;
		protected Font titleFont, inputFont, inputHintFont;
		
		public IView( LauncherWindow game ) {
			this.game = game;
			drawer = game.Drawer;
		}
		
		/// <summary> Function called to setup the widgets and other data for this view. </summary>
		public abstract void Init();
		
		/// <summary> Function called to redraw all widgets in this view. </summary>
		public abstract void DrawAll();
		
		/// <summary> Cleans up all native resources held by this view. </summary>
		public virtual void Dispose() {
			if( titleFont != null ) titleFont.Dispose();
			if( inputFont != null ) inputFont.Dispose();
			if( inputHintFont != null ) inputHintFont.Dispose();
		}
		
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
		
		protected void MakeButtonAt( string text, int width, int height, Font font,
		                            Anchor verAnchor, int x, int y ) {
			MakeButtonAt( text, width, height, font, Anchor.Centre, verAnchor, x, y );
		}
		
		protected void MakeButtonAt( string text, int width, int height, Font font, Anchor horAnchor,
		                            Anchor verAnchor, int x, int y ) {
			WidgetConstructors.MakeButtonAt( game, widgets, ref widgetIndex,
			                                text, width, height, font, horAnchor,
			                                verAnchor, x, y, null );
		}
		
		protected void MakeLabelAt( string text, Font font, Anchor horAnchor, Anchor verAnchor, int x, int y ) {
			WidgetConstructors.MakeLabelAt( game, widgets, ref widgetIndex,
			                               text, font, horAnchor, verAnchor, x, y );
		}
		
		protected void MakeBooleanAt( Anchor horAnchor, Anchor verAnchor, Font font, bool initValue,
		                             int width, int height, int x, int y ) {
			WidgetConstructors.MakeBooleanAt( game, widgets, ref widgetIndex,
			                                 horAnchor, verAnchor, font, initValue,
			                                 width, height, x, y, null );
		}
		
		protected void MakeInput( string text, int width, Anchor horAnchor, Anchor verAnchor,
		                         bool password, int x, int y, int maxChars, string hint ) {
			WidgetConstructors.MakeInput( game, widgets, ref widgetIndex,
			                             text, width, horAnchor, verAnchor,
			                             inputFont, inputHintFont, null,
			                             password, x, y, maxChars, hint );
		}
	}
}
