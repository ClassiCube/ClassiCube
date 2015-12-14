using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class ClickableScreen : Screen {
		
		public ClickableScreen( Game game ) : base( game ) {
		}
		
		protected bool HandleMouseClick( Widget[] widgets, int mouseX, int mouseY, MouseButton button ) {
			if( button != MouseButton.Left ) return false;
			
			// iterate backwards (because last elements rendered are shown over others)
			for( int i = widgets.Length - 1; i >= 0; i-- ) {
				Widget widget = widgets[i];
				if( widget != null && widget.Bounds.Contains( mouseX, mouseY ) ) {
					if( widget.OnClick != null && !widget.Disabled )
						widget.OnClick( game, widget );
					return true;
				}
			}
			return false;
		}
		
		protected bool HandleMouseMove( Widget[] widgets, int mouseX, int mouseY ) {
			for( int i = 0; i < widgets.Length; i++ ) {
				if( widgets[i] == null ) continue;
				widgets[i].Active = false;
			}
			
			for( int i = widgets.Length - 1; i >= 0; i-- ) {
				Widget widget = widgets[i];
				if( widget != null && widget.Bounds.Contains( mouseX, mouseY ) ) {
					widget.Active = true;
					WidgetSelected( widget );
					return true;
				}
			}
			WidgetSelected( null );
			return false;
		}
		
		protected virtual void WidgetSelected( Widget widget ) {
		}
		
		protected ButtonWidget MakeBack( bool toGame, Font font, Action<Game, Widget> onClick ) {
			string text = toGame ? "Back to game" : "Back to menu";
			return ButtonWidget.Create(
				game, 0, 5, 180, 35, text,
				Anchor.Centre, Anchor.BottomOrRight, font, onClick );
		}
	}
}