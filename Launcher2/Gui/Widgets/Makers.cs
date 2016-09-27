using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Views;

namespace Launcher.Gui.Widgets {	
	/// <summary> Helper methods to construct widgets. </summary>
	public static class Makers {
		
		public static Widget Button( IView view, string text, int width, int height, Font font ) {
			ButtonWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (ButtonWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new ButtonWidget( view.game );
				widget.Text = text;
				view.widgets[view.widgetIndex] = widget;
			}
			
			widget.Active = false;
			widget.SetDrawData( view.game.Drawer, text, font, width, height );
			view.widgetIndex++;
			return widget;
		}
		
		public static Widget Label( IView view, string text, Font font ) {
			LabelWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (LabelWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new LabelWidget( view.game, font );
				view.widgets[view.widgetIndex] = widget;
			}
			
			widget.SetDrawData( view.game.Drawer, text );
			view.widgetIndex++;
			return widget;
		}
		
		public static Widget Checkbox( IView view, Font font, bool initValue, int size ) {
			CheckboxWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (CheckboxWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new CheckboxWidget( view.game, font, size, size );
				widget.Value = initValue;
				view.widgets[view.widgetIndex] = widget;
			}
			
			view.widgetIndex++;
			return widget;
		}
		
		public static Widget Input( IView view, string text, int width, Font inputFont,
		                           Font inputHintFont, bool password, int maxChars, string hint ) {
			InputWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (InputWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new InputWidget( view.game );
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
		
		public static Widget Slider( IView view, int width, int height,
		                            int progress, FastColour progressCol ) {
			SliderWidget widget;
			if( view.widgets[view.widgetIndex] != null ) {
				widget = (SliderWidget)view.widgets[view.widgetIndex];
			} else {
				widget = new SliderWidget( view.game, width, height );
				view.widgets[view.widgetIndex] = widget;
			}

			widget.Progress = progress;
			widget.ProgressColour = progressCol;
			view.widgetIndex++;
			return widget;
		}
	}
}
