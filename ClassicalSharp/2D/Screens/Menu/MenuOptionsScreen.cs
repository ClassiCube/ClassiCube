// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
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
			RenderWidgets(widgets, delta);
			
			if (extendedHelp != null && extEndY <= extClipY)
				extendedHelp.Render(delta);
			gfx.Texturing = false;
		}
		
		public override void Init() {
			base.Init();
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16);
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
			extendedHelp.Reposition();
		}
		
		protected override void ContextLost() {
			base.ContextLost();
			InputClosed();
			DisposeExtendedHelp();
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
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
		
		protected virtual void InputClosed() {
			if (input != null) input.Dispose();
			widgets[widgets.Length - 2] = null;
			input = null;
			
			int okIndex = widgets.Length - 1;
			if (widgets[okIndex] != null) widgets[okIndex].Dispose();
			widgets[okIndex] = null;
		}
		
		protected ButtonWidget MakeOpt(int dir, int y, string optName, ClickHandler onClick,
		                               ButtonValueGetter getter, ButtonValueSetter setter) {
			ButtonWidget btn = ButtonWidget.Create(game, 300, optName + ": " + getter(game), titleFont, onClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 160 * dir, y);
			btn.OptName = optName;
			btn.GetValue = getter;
			btn.SetValue = setter;
			return btn;
		}
		
		protected static string GetBool(bool v) { return v ? "ON" : "OFF"; }
		protected static bool SetBool(string v, string key) {
			Options.Set(key, v == "ON");
			return v == "ON";
		}
		
		void ShowExtendedHelp() {
			bool canShow = input == null && selectedWidget != null && descriptions != null;
			if (!canShow) return;
			
			int index = IndexOfWidget(selectedWidget);
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
			extendedHelp.Reposition();
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
			
			int index = IndexOfWidget(button);
			MenuInputValidator validator = validators[index];
			if (validator is BooleanValidator) {
				string value = button.GetValue(game);
				SetButtonValue(button, value == "ON" ? "OFF" : "ON");
				UpdateDescription(button);
				return;
			} else if (validator is EnumValidator) {
				Type type = ((EnumValidator)validator).EnumType;
				HandleEnumOption(button, type);
				return;
			}
			
			targetWidget = selectedWidget;
			InputClosed();
			
			input = MenuInputWidget.Create(game, 400, 30,
			                               button.GetValue(game), regularFont, validator)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 110);
			input.ShowCaret = true;
			input.OnClick = InputClick;
			
			widgets[widgets.Length - 2] = input;
			widgets[widgets.Length - 1] = ButtonWidget.Create(game, 40, "OK", titleFont, OnWidgetClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 240, 110);

			InputOpened();
			UpdateDescription(targetWidget);
		}
		
		void InputClick(Game game, Widget widget, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			widget.HandlesMouseClick(x, y, btn);
		}
		
		void HandleEnumOption(ButtonWidget button, Type type) {
			string rawName = button.GetValue(game);
			int value = (int)Enum.Parse(type, rawName, true);
			value++;
			// go back to first value
			if (!Enum.IsDefined(type, value)) value = 0;
			
			SetButtonValue(button, Enum.GetName(type, value));
			UpdateDescription(button);
		}
		
		void ChangeSetting() {
			string text = input.Text.ToString();
			if (((MenuInputWidget)input).Validator.IsValidValue(text)) {
				SetButtonValue(targetWidget, text);
			}
			
			UpdateDescription(targetWidget);
			targetWidget = null;
			InputClosed();
		}
		
		void SetButtonValue(ButtonWidget btn, string text) {
			btn.SetValue(game, text);
			int index = IndexOfWidget(btn);
			// e.g. changing FPS invalidates all widgets
			if (index >= 0) btn.SetText(btn.OptName + ": " + btn.GetValue(game));
		}
		
		protected static string GetFPS(Game g) { return g.FpsLimit.ToString(); }
		protected void SetFPS(Game g, string v) {
			object raw = Enum.Parse(typeof(FpsLimitMethod), v);
			g.SetFpsLimitMethod((FpsLimitMethod)raw);
			Options.Set(OptionsKey.FpsLimit, v);
		}
	}
}