// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class MenuOptionsScreen : MenuScreen {
		
		public MenuOptionsScreen(Game game) : base(game) {
		}
		
		protected MenuInputValidator[] validators;
		protected string[][] descriptions;
		protected string[] defaultValues;
		protected int activeI = -1, selectedI = -1;
		TextGroupWidget extHelp;
		InputWidget input;				
		
		public override void Init() {
			base.Init();
			game.Keyboard.KeyRepeat = true;
			ContextRecreated();
		}
		
		static FastColour tableCol = new FastColour(20, 20, 20, 200);
		public override void Render(double delta) {
			base.Render(delta);
			if (extHelp == null) return;
					
			int x = extHelp.X - 5, y = extHelp.Y - 5;
			int width = extHelp.Width, height = extHelp.Height;
			game.Graphics.Draw2DQuad(x, y, width + 10, height + 10, tableCol);
			
			game.Graphics.Texturing = true;
			extHelp.Render(delta);
			game.Graphics.Texturing = false;
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			base.Dispose();
		}
		
		public override void OnResize(int width, int height) {
			base.OnResize(width, height);
			if (extHelp == null) return;
			RepositionExtendedHelp();
		}
		
		protected override void ContextLost() {
			base.ContextLost();
			input = null;
			DisposeExtendedHelp();
		}

		public override bool HandlesKeyPress(char key) {
			if (input == null) return true;
			return input.HandlesKeyPress(key);
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (input != null) {
				if (input.HandlesKeyDown(key)) return true;
				if (key == Key.Enter || key == Key.KeypadEnter) {
					EnterInput(); return true;
				}
			}
			return base.HandlesKeyDown(key);
		}
		
		public override bool HandlesKeyUp(Key key) {
			if (input == null) return true;
			return input.HandlesKeyUp(key);
		}
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			int i = HandleMouseMove(widgets, mouseX, mouseY);
			if (i == -1 || i == selectedI) return true;
			if (descriptions == null || i >= descriptions.Length) return true;
			
			selectedI = i;
			if (activeI == -1) SelectExtendedHelp(i);
			return true;
		}
		
		protected ButtonWidget MakeOpt(int dir, int y, string optName, ClickHandler onClick,
		                               ButtonValueGetter getter, ButtonValueSetter setter) {
			ButtonWidget btn = ButtonWidget.Create(game, 300, optName + ": " + getter(game), titleFont, onClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 160 * dir, y);
			btn.OptName  = optName;
			btn.GetValue = getter;
			btn.SetValue = setter;
			return btn;
		}
		
		protected static string GetBool(bool v) { return v ? "ON" : "OFF"; }
		protected static bool SetBool(string v, string key) {
			Options.Set(key, v == "ON");
			return v == "ON";
		}
		
		protected static string GetFPS(Game g) { return g.FpsLimit.ToString(); }
		protected void SetFPS(Game g, string v) {
			object raw = Enum.Parse(typeof(FpsLimitMethod), v);
			g.SetFpsLimitMethod((FpsLimitMethod)raw);
			Options.Set(OptionsKey.FpsLimit, v);
		}
		
		void SelectExtendedHelp(int idx) {
			DisposeExtendedHelp();
			if (descriptions == null || input != null) return;		
			string[] desc = descriptions[idx];
			if (desc == null) return;
			
			extHelp = new TextGroupWidget(game, desc.Length, textFont, null)
				.SetLocation(Anchor.Min, Anchor.Min, 0, 0);
			extHelp.Init();
			
			for (int i = 0; i < desc.Length; i++) {
				extHelp.SetText(i, desc[i]);
			}
			RepositionExtendedHelp();
		}
		
		void RepositionExtendedHelp() {
			extHelp.XOffset = game.Width / 2 - extHelp.Width / 2;
			extHelp.YOffset = game.Height / 2 + 100;
			extHelp.Reposition();
		}
		
		void DisposeExtendedHelp() {
			if (extHelp == null) return;
			extHelp.Dispose();
			extHelp = null;
		}
		
		void SetButtonValue(int index, string text) {
			ButtonWidget btn = (ButtonWidget)widgets[index];
			btn.SetValue(game, text);
			
			// need to get btn again here (e.g. changing FPS invalidates all widgets)
			btn = (ButtonWidget)widgets[index];
			btn.SetText(btn.OptName + ": " + btn.GetValue(game));
		}
		
		protected void OnBoolClick(Game game, Widget widget) {
			ButtonWidget button = (ButtonWidget)widget;
			int index = IndexWidget(widget);
			SelectExtendedHelp(index);

			string value = button.GetValue(game);
			SetButtonValue(index, value == "ON" ? "OFF" : "ON");			
		}
		
		protected void OnEnumClick(Game game, Widget widget) {
			ButtonWidget button = (ButtonWidget)widget;	
			int index = IndexWidget(widget);
			SelectExtendedHelp(index);
			
			MenuInputValidator validator = validators[index];
			Type type = ((EnumValidator)validator).EnumType;
			
			string value = button.GetValue(game);
			int raw = (int)Enum.Parse(type, value, true) + 1;		
			if (!Enum.IsDefined(type, raw)) raw = 0; // go back to first value
			
			SetButtonValue(index, Enum.GetName(type, raw));
		}
		
		protected void OnInputClick(Game game, Widget widget) {
			ButtonWidget button = (ButtonWidget)widget;	
			activeI = IndexWidget(button);
			DisposeExtendedHelp();
			
			DisposeInput();
			MenuInputValidator validator = validators[activeI];
			input = MenuInputWidget.Create(game, 400, 30, button.GetValue(game), textFont, validator)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 110);
			input.ShowCaret = true;
			
			widgets[widgets.Length - 1] = input;
			widgets[widgets.Length - 2] = ButtonWidget.Create(game, 40, "OK", titleFont, OKButtonClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 240, 110);
			widgets[widgets.Length - 3] = ButtonWidget.Create(game, 200, "Default value", titleFont, DefaultButtonClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 150);
		}
		
		void OKButtonClick(Game game, Widget widget) { EnterInput(); }
		
		void DefaultButtonClick(Game game, Widget widget) {
			string defValue = defaultValues[activeI];
			input.Clear();
			input.Append(defValue);
		}
		
		void EnterInput() {
			string text = input.Text.ToString();
			if (((MenuInputWidget)input).Validator.IsValidValue(text)) {
				SetButtonValue(activeI, text);
			}
			
			SelectExtendedHelp(activeI);
			activeI = -1;
			DisposeInput();
		}
		
		void DisposeInput() {
			if (input == null) return;
			input = null;
			
			for (int i = widgets.Length - 3; i < widgets.Length; i++) {
				widgets[i].Dispose();
				widgets[i] = null;
			}
		}
	}
}