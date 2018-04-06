// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class KeyBindingsScreen : MenuScreen {
		
		public KeyBindingsScreen(Game game) : base(game) { }
		
		static string[] keyNames;
		protected string[] desc;
		protected KeyBind[] binds;
		protected int leftLength = -1;
		
		protected int btnDistance = 50, btnWidth = 260;
		protected string title = "Controls";
		protected int index;
		protected ClickHandler leftPage, rightPage;
		
		public override void Init() {
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16);
			base.Init();
			
			if (keyNames == null)
				keyNames = Enum.GetNames(typeof(Key));
		}

		protected void MakeWidgets(int y, int arrowsY) {
			index = 0;
			int origin = y;
			MakeOthers();
			
			int i = 0;
			if (leftLength == -1) {
				for (i = 0; i < binds.Length; i++)
					Make(i, 0, ref y);
			} else {
				for (i = 0; i < leftLength; i++)
					Make(i, -btnWidth / 2 - 5, ref y);
				
				y = origin;
				for (; i < binds.Length; i++)
					Make(i, btnWidth / 2 + 5, ref y);
			}
			MakePages(arrowsY);
		}
		
		void MakeOthers() {
			widgets[index++] = TextWidget.Create(game, title, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -180);
			
			if (game.UseClassicOptions) {
				widgets[index++] = MakeBack(false, titleFont, SwitchClassicOptions);
			} else {
				widgets[index++] = MakeBack(false, titleFont, SwitchOptions);
			}
		}
		
		static void SwitchClassicOptions(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicOptionsScreen(g)); }
		protected static void SwitchClassic(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicKeyBindingsScreen(g)); }
		protected static void SwitchClassicHacks(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicHacksKeyBindingsScreen(g)); }
		protected static void SwitchNormal(Game g, Widget w) { g.Gui.SetNewScreen(new NormalKeyBindingsScreen(g)); }
		protected static void SwitchHacks(Game g, Widget w) { g.Gui.SetNewScreen(new HacksKeyBindingsScreen(g)); }
		protected static void SwitchOther(Game g, Widget w) { g.Gui.SetNewScreen(new OtherKeyBindingsScreen(g)); }
		protected static void SwitchMouse(Game g, Widget w) { g.Gui.SetNewScreen(new MouseKeyBindingsScreen(g)); }
		
		void MakePages(int btnY) {
			if (leftPage == null && rightPage == null) return;
			
			ButtonWidget left = ButtonWidget.Create(game, 40, "<", titleFont, leftPage)
				.SetLocation(Anchor.Centre, Anchor.Centre, -btnWidth - 35, btnY);
			left.Disabled = leftPage == null;
			widgets[index++] = left;
			
			ButtonWidget right = ButtonWidget.Create(game, 40, ">", titleFont, rightPage)
				.SetLocation(Anchor.Centre, Anchor.Centre, btnWidth + 35, btnY);
			right.Disabled = rightPage == null;
			widgets[index++] = right;
		}
		
		void Make(int i, int x, ref int y) {
			string text = ButtonText(i);
			widgets[index++] = ButtonWidget.Create(game, btnWidth, text, titleFont, OnBindingClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, x, y);
			y += btnDistance;
		}
		
		
		ButtonWidget curWidget;
		void OnBindingClick(Game game, Widget widget) {
			int index = 0;
			if (curWidget != null) {
				index = IndexOfWidget(curWidget) - 2;
				curWidget.SetText(ButtonText(index));
				curWidget = null;
			}
			
			index = IndexOfWidget(widget) - 2;
			string text = ButtonText(index);
			curWidget = (ButtonWidget)widget;
			curWidget.SetText("> " + text + " <");
		}
		
		string ButtonText(int i) {
			Key key = game.Input.Keys[binds[i]];
			return desc[i] + ": " + keyNames[(int)key];
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (curWidget != null) {
				int index = IndexOfWidget(curWidget) - 2;
				game.Input.Keys[binds[index]] = key;
				curWidget.SetText(ButtonText(index));
				curWidget = null;
				return true;
			}
			return base.HandlesKeyDown(key);
		}
		
		public override bool HandlesMouseDown(int mouseX, int mouseY, MouseButton button) {
			if (button == MouseButton.Right) {
				int i = HandleMouseDown(widgets, mouseX, mouseY, button);
				if (i == -1) return false;
				
				if (curWidget == null || curWidget == widgets[i]) {
					// Reset a key binding
					if (i >= 2 && (i - 2) < binds.Length) {
						curWidget = (ButtonWidget)widgets[i];
						KeyBind mapping = binds[i - 2];
						HandlesKeyDown(game.Input.Keys.GetDefault(mapping));
					}
				}
				return true;
			} else {
				return base.HandlesMouseDown(mouseX, mouseY, button);
			}
		}
	}
}