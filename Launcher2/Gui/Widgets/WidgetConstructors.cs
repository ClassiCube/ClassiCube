using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {
	
	/// <summary> Helper methods to construct widgets. </summary>
	public static class WidgetConstructors {
		
		public static void MakeButtonAt( LauncherWindow game, LauncherWidget[] widgets, ref int widgetIndex,
		                                string text, int width, int height, Font font, Anchor horAnchor,
		                                Anchor verAnchor, int x, int y, Action<int, int> onClick ) {
			LauncherButtonWidget widget;
			if( widgets[widgetIndex] != null ) {
				widget = (LauncherButtonWidget)widgets[widgetIndex];
			} else {
				widget = new LauncherButtonWidget( game );
				widget.Text = text;
				widget.OnClick = onClick;
				widgets[widgetIndex] = widget;
			}
			
			widget.Active = false;
			widget.SetDrawData( game.Drawer, text, font, horAnchor, verAnchor, width, height, x, y );
			widgetIndex++;
		}
		
		public static void MakeLabelAt( LauncherWindow game, LauncherWidget[] widgets, ref int widgetIndex,
		                               string text, Font font, Anchor horAnchor,
		                               Anchor verAnchor, int x, int y ) {
			LauncherLabelWidget widget;
			if( widgets[widgetIndex] != null ) {
				widget = (LauncherLabelWidget)widgets[widgetIndex];
			} else {
				widget = new LauncherLabelWidget( game, text );
				widgets[widgetIndex] = widget;
			}
			
			widget.SetDrawData( game.Drawer, text, font, horAnchor, verAnchor, x, y );
			widgetIndex++;
		}
		
		public static void MakeBooleanAt( LauncherWindow game, LauncherWidget[] widgets, ref int widgetIndex,
		                                 Anchor horAnchor, Anchor verAnchor, Font font, bool initValue,
		                                 int width, int height, int x, int y, Action<int, int> onClick ) {
			LauncherBoolWidget widget;
			if( widgets[widgetIndex] != null ) {
				widget = (LauncherBoolWidget)widgets[widgetIndex];
			} else {
				widget = new LauncherBoolWidget( game, font, width, height );
				widget.Value = initValue;
				widget.OnClick = onClick;
				widgets[widgetIndex] = widget;
			}
			
			widget.SetDrawData( game.Drawer, horAnchor, verAnchor, x, y );
			widgetIndex++;
		}
		
		public static void MakeInput( LauncherWindow game, LauncherWidget[] widgets, ref int widgetIndex,
		                             string text, int width, Anchor horAnchor, Anchor verAnchor,
		                             Font inputFont, Font inputHintFont, Action<int, int> onClick,
		                             bool password, int x, int y, int maxChars, string hint ) {
			LauncherInputWidget widget;
			if( widgets[widgetIndex] != null ) {
				widget = (LauncherInputWidget)widgets[widgetIndex];
			} else {
				widget = new LauncherInputWidget( game );
				widget.OnClick = onClick;
				widget.Password = password;
				widget.Chars.MaxChars = maxChars;
				widget.HintText = hint;
				widgets[widgetIndex] = widget;
			}
			
			widget.SetDrawData( game.Drawer, text, inputFont, inputHintFont, 
			                   horAnchor, verAnchor, width, 30, x, y );
			widgetIndex++;
		}
	}
}
