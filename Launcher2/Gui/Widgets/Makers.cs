using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Views;

namespace Launcher.Gui.Widgets {
	
	/// <summary> Helper methods to construct widgets. </summary>
	public static class Makers {
		
		public static void Button( IView view, string text, int width, int height, Font font, 
		                          Anchor horAnchor, Anchor verAnchor, int x, int y ) {
			LauncherButtonWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (LauncherButtonWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new LauncherButtonWidget( view.game );
				widget.Text = text;
				view.widgets[view.widgetIndex] = widget;
			}
			
			widget.Active = false;
			widget.SetDrawData( view.game.Drawer, text, font, horAnchor, verAnchor, width, height, x, y );
			view.widgetIndex++;
		}
		
		public static void Label( IView view, string text, Font font, 
		                         Anchor horAnchor, Anchor verAnchor, int x, int y ) {
			LauncherLabelWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (LauncherLabelWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new LauncherLabelWidget( view.game, text );
				view.widgets[view.widgetIndex] = widget;
			}
			
			widget.SetDrawData( view.game.Drawer, text, font, horAnchor, verAnchor, x, y );
			view.widgetIndex++;
		}
		
		public static LauncherWidget Boolean( IView view, Font font, bool initValue, int size ) {
			LauncherBoolWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (LauncherBoolWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new LauncherBoolWidget( view.game, font, size, size );
				widget.Value = initValue;
				view.widgets[view.widgetIndex] = widget;
			}
			
			view.widgetIndex++;
			return widget;
		}
		
		public static void Input( IView view, string text, int width, Anchor horAnchor, 
		                         Anchor verAnchor, Font inputFont, Font inputHintFont,
		                         bool password, int x, int y, int maxChars, string hint ) {
			LauncherInputWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (LauncherInputWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new LauncherInputWidget( view.game );
				widget.Password = password;
				widget.Chars.MaxChars = maxChars;
				widget.HintText = hint;
				view.widgets[view.widgetIndex] = widget;
			}
			
			widget.SetDrawData( view.game.Drawer, text, inputFont, inputHintFont,
			                   horAnchor, verAnchor, width, 30, x, y );
			view.widgetIndex++;
		}
	}
}
