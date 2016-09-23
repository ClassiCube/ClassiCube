using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Views;

namespace Launcher.Gui.Widgets {
	
	/// <summary> Helper methods to construct widgets. </summary>
	public static class Makers {
		
		public static LauncherWidget Button( IView view, string text, int width, int height, Font font ) {
			LauncherButtonWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (LauncherButtonWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new LauncherButtonWidget( view.game );
				widget.Text = text;
				view.widgets[view.widgetIndex] = widget;
			}
			
			widget.Active = false;
			widget.SetDrawData( view.game.Drawer, text, font, width, height );
			view.widgetIndex++;
			return widget;
		}
		
		public static LauncherWidget Label( IView view, string text, Font font ) {
			LauncherLabelWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (LauncherLabelWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new LauncherLabelWidget( view.game, font );
				view.widgets[view.widgetIndex] = widget;
			}
			
			widget.SetDrawData( view.game.Drawer, text );
			view.widgetIndex++;
			return widget;
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
		
		public static LauncherWidget Input( IView view, string text, int width, Font inputFont, 
		                                   Font inputHintFont, bool password, int maxChars, string hint ) {
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
			
			widget.SetDrawData( view.game.Drawer, text, 
			                   inputFont, inputHintFont, width, 30 );
			view.widgetIndex++;
			return widget;
		}
		
		public static LauncherWidget Slider( IView view, int width, int height,
		                                    int progress, FastColour progressCol ) {
			LauncherSliderWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (LauncherSliderWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new LauncherSliderWidget( view.game, width, height );
				widget.Progress = progress;
				widget.ProgressColour = progressCol;
				view.widgets[view.widgetIndex] = widget;
			}
			
			view.widgetIndex++;
			return widget;
		}
	}
}
