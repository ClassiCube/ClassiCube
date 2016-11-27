// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class MenuOptionsScreen : MenuScreen {
		
		public MenuOptionsScreen(Game game) : base(game) {
		}
		
		protected InputWidget input;
		protected MenuInputValidator[] validators;
		protected string[][] descriptions;
		protected TextGroupWidget extendedHelp;
		
		public override void Render(double delta) {
			RenderMenuBounds();
			int extClipY = extendedHelp == null ? 0 : widgets[widgets.Length - 3].Y;
			int extEndY = extendedHelp == null ? 0 : extendedHelp.Y + extendedHelp.Height;
			
			if (extendedHelp != null && extEndY <= extClipY) {
				int tableWidth = extendedHelp.Width, tableHeight = extendedHelp.Height;
				int x = game.Width / 2 - tableWidth / 2 - 5;
				int y = game.Height / 2 + extHelpY - 5;
				gfx.Draw2DQuad(x, y, tableWidth + 10, tableHeight + 10, tableCol);
			}
			
			gfx.Texturing = true;
			RenderMenuWidgets(delta);
			
			if (extendedHelp != null && extEndY <= extClipY)
				extendedHelp.Render(delta);
			gfx.Texturing = false;
		}
		
		public override void Init() {
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16, FontStyle.Regular);
			game.Keyboard.KeyRepeat = true;
		}
		
		public override bool HandlesKeyPress(char key) {
			if (input == null) return true;
			return input.HandlesKeyPress(key);
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (key == Key.Escape) {
				game.Gui.SetNewScreen(null);
				return true;
			} else if ((key == Key.Enter || key == Key.KeypadEnter)
			          && input != null) {
				ChangeSetting();
				return true;
			}
			if (input == null)
				return key < Key.F1 || key > Key.F35;
			return input.HandlesKeyDown(key);
		}
		
		public override bool HandlesKeyUp(Key key) {
			if (input == null) return true;
			return input.HandlesKeyUp(key);
		}
		
		public override void OnResize(int width, int height) {
			base.OnResize(width, height);
			if (extendedHelp == null) return;
			
			extendedHelp.XOffset = game.Width / 2 - extendedHelp.Width / 2;
			extendedHelp.YOffset = game.Height / 2 + extHelpY;
			extendedHelp.CalculatePosition();
		}
		
		public override void Dispose() {
			DisposeWidgets();
			game.Keyboard.KeyRepeat = false;
			DisposeExtendedHelp();
			base.Dispose();
		}
		
		protected ButtonWidget selectedWidget, targetWidget;
		protected override void WidgetSelected(Widget widget) {
			ButtonWidget button = widget as ButtonWidget;
			if (selectedWidget == button || button == null ||
			   button == widgets[widgets.Length - 2]) return;
			
			selectedWidget = button;
			if (targetWidget != null) return;
			UpdateDescription(selectedWidget);
		}
		
		protected void UpdateDescription(ButtonWidget widget) {
			DisposeExtendedHelp();
			if (widget == null || widget.GetValue == null) return;
			
			ShowExtendedHelp();
		}
		
		protected virtual void InputOpened() { }
		
		protected virtual void InputClosed() { }
		
		protected ButtonWidget MakeOpt(int dir, int y, string text, ClickHandler onClick,
		                               Func<Game, string> getter, Action<Game, string> setter) {
			ButtonWidget widget = ButtonWidget.Create(game, 301, 41, text + ": " + getter(game), titleFont, onClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 160 * dir, y);
			widget.Metadata = text;
			widget.GetValue = getter;
			widget.SetValue = (g, v) => {
				setter(g, v);
				widget.SetText((string)widget.Metadata + ": " + getter(g));
			};
			return widget;
		}
		
		protected ButtonWidget MakeBool(int dir, int y, string text, string optKey,
		                                ClickHandler onClick, Func<Game, bool> getter, Action<Game, bool> setter) {
			return MakeBool(dir, y, text, optKey, false, onClick, getter, setter);
		}

		protected ButtonWidget MakeBool(int dir, int y, string text, string optKey, bool invert,
		                                ClickHandler onClick, Func<Game, bool> getter, Action<Game, bool> setter) {
			string optName = text;
			text = text + ": " + (getter(game) ? "ON" : "OFF");
			ButtonWidget widget = ButtonWidget.Create(game, 301, 41, text, titleFont, onClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 160 * dir, y);
			widget.Metadata = optName;
			widget.GetValue = g => getter(g) ? "yes" : "no";
			string target = invert ? "no" : "yes";
			
			widget.SetValue = (g, v) => {
				setter(g, v == "yes");
				Options.Set(optKey, v == target);
				widget.SetText((string)widget.Metadata + ": " + (v == "yes" ? "ON" : "OFF"));
			};
			return widget;
		}
		
		void ShowExtendedHelp() {
			bool canShow = input == null && selectedWidget != null && descriptions != null;
			if (!canShow) return;
			
			int index = Array.IndexOf<Widget>(widgets, selectedWidget);
			if (index < 0 || index >= descriptions.Length) return;
			string[] desc = descriptions[index];
			if (desc == null) return;
			MakeExtendedHelp(desc);
		}
		
		static FastColour tableCol = new FastColour(20, 20, 20, 200);
		const int extHelpY = 100;
		
		void MakeExtendedHelp(string[] desc) {
			extendedHelp = new TextGroupWidget(game, desc.Length, regularFont, null)
				.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 0, 0);
			extendedHelp.Init();
			for (int i = 0; i < desc.Length; i++)
				extendedHelp.SetText(i, desc[i]);
			
			extendedHelp.XOffset = game.Width / 2 - extendedHelp.Width / 2;
			extendedHelp.YOffset = game.Height / 2 + extHelpY;
			extendedHelp.CalculatePosition();
		}
		
		void DisposeExtendedHelp() {
			if (extendedHelp == null) return;
			extendedHelp.Dispose();
			extendedHelp = null;
		}
		
		protected void OnWidgetClick(Game game, Widget widget, MouseButton btn, int x, int y) {
			ButtonWidget button = widget as ButtonWidget;
			if (btn != MouseButton.Left) return;
			if (widget == widgets[widgets.Length - 1]) {
				ChangeSetting(); return;
			}
			if (button == null) return;
			DisposeExtendedHelp();
			
			int index = Array.IndexOf<Widget>(widgets, button);
			MenuInputValidator validator = validators[index];
			if (validator is BooleanValidator) {
				string value = button.GetValue(game);
				button.SetValue(game, value == "yes" ? "no" : "yes");
				UpdateDescription(button);
				return;
			} else if (validator is EnumValidator) {
				Type type = ((EnumValidator)validator).EnumType;
				HandleEnumOption(button, type);
				return;
			}
			
			targetWidget = selectedWidget;
			if (input != null) input.Dispose();
			
			input = MenuInputWidget.Create(game, 400, 30,
			                               button.GetValue(game), regularFont, validator)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 110);
			input.ShowCaret = true;
			input.OnClick = InputClick;
			
			widgets[widgets.Length - 2] = input;
			widgets[widgets.Length - 1] = ButtonWidget.Create(game, 40, 30, "OK", titleFont, OnWidgetClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 240, 110);

			InputOpened();
			UpdateDescription(targetWidget);
		}
		
		void InputClick(Game game, Widget widget, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			widget.HandlesMouseClick(x, y, btn);
		}		
		
		void HandleEnumOption(ButtonWidget button, Type type) {
			string value = button.GetValue(game);
			int enumValue = (int)Enum.Parse(type, value, true);
			enumValue++;
			// go back to first value
			if (!Enum.IsDefined(type, enumValue))
				enumValue = 0;
			button.SetValue(game, Enum.GetName(type, enumValue));
			UpdateDescription(button);
		}
		
		void ChangeSetting() {
			string text = input.Text.ToString();
			if (((MenuInputWidget)input).Validator.IsValidValue(text))
				targetWidget.SetValue(game, text);
			
			DisposeWidgets();
			UpdateDescription(targetWidget);
			targetWidget = null;
			InputClosed();
		}
		
		void DisposeWidgets() {
			if (input != null)
				input.Dispose();
			widgets[widgets.Length - 2] = null;
			input = null;
			
			int okayIndex = widgets.Length - 1;
			if (widgets[okayIndex] != null)
				widgets[okayIndex].Dispose();
			widgets[okayIndex] = null;
		}
	}
}