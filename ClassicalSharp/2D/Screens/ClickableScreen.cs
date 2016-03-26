// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public abstract class ClickableScreen : Screen {
		
		public ClickableScreen( Game game ) : base( game ) {
		}
		
		protected bool HandleMouseClick( Widget[] widgets, int mouseX, int mouseY, MouseButton button ) {
			// iterate backwards (because last elements rendered are shown over others)
			for( int i = widgets.Length - 1; i >= 0; i-- ) {
				Widget widget = widgets[i];
				if( widget != null && widget.Bounds.Contains( mouseX, mouseY ) ) {
					if( widget.OnClick != null && !widget.Disabled )
						widget.OnClick( game, widget, button );
					return true;
				}
			}
			return false;
		}
		
		int lastX = -1, lastY = -1;
		protected bool HandleMouseMove( Widget[] widgets, int mouseX, int mouseY ) {
			if( lastX == mouseX && lastY == mouseY )
				return true;
			for( int i = 0; i < widgets.Length; i++ ) {
				if( widgets[i] == null ) continue;
				widgets[i].Active = false;
			}
			
			for( int i = widgets.Length - 1; i >= 0; i-- ) {
				Widget widget = widgets[i];
				if( widget != null && widget.Bounds.Contains( mouseX, mouseY ) ) {
					widget.Active = true;
					lastX = mouseX; lastY = mouseY;
					WidgetSelected( widget );
					return true;
				}
			}
			lastX = mouseX; lastY = mouseY;
			WidgetSelected( null );
			return false;
		}
		
		protected virtual void WidgetSelected( Widget widget ) {
		}
		
		protected ButtonWidget MakeBack( bool toGame, Font font, Action<Game, Widget> onClick ) {
			string text = toGame ? "Back to game" : "Back to menu";
			return ButtonWidget.Create(
				game, 0, 5, 180, 35, text,
				Anchor.Centre, Anchor.BottomOrRight, font, LeftOnly( onClick ) );
		}
	}
}