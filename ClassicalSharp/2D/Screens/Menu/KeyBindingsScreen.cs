// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class KeyBindingsScreen : MenuScreen {
		
		public KeyBindingsScreen(Game game) : base(game) { }
		
		static string[] keyNames;
		protected string[] desc;
		protected KeyBind[] binds;
		protected ClickHandler leftPage, rightPage;
		int curI = -1;

		protected int MakeWidgets(int y, int arrowsY, int leftLength, string title, int btnWidth) {
			int i, origin = y, xOffset = btnWidth / 2 + 5;
			for (i = 0; i < binds.Length; i++) {
				if (i == leftLength) y = origin; // reset y for next column
				int xDir = leftLength == -1 ? 0 : (i < leftLength ? -1 : 1);
				
				string text = ButtonText(i);
				widgets[i] = ButtonWidget.Create(game, btnWidth, text, titleFont, OnBindingClick)
					.SetLocation(Anchor.Centre, Anchor.Centre, xDir * xOffset, y);
				y += 50; // distance between buttons
			}
			
			widgets[i++] = TextWidget.Create(game, title, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -180);
			if (game.UseClassicOptions) {
				widgets[i++] = MakeBack(false, titleFont, SwitchClassicOptions);
			} else {
				widgets[i++] = MakeBack(false, titleFont, SwitchOptions);
			}
			if (leftPage == null && rightPage == null) return i;
			
			ButtonWidget left = ButtonWidget.Create(game, 40, "<", titleFont, leftPage)
				.SetLocation(Anchor.Centre, Anchor.Centre, -btnWidth - 35, arrowsY);
			left.Disabled = leftPage == null;
			widgets[i++] = left;
			
			ButtonWidget right = ButtonWidget.Create(game, 40, ">", titleFont, rightPage)
				.SetLocation(Anchor.Centre, Anchor.Centre, btnWidth + 35, arrowsY);
			right.Disabled = rightPage == null;
			widgets[i++] = right;
			return i;
		}
		
		static void SwitchClassicOptions(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicOptionsScreen(g)); }
		protected static void SwitchClassic(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicKeyBindingsScreen(g)); }
		protected static void SwitchClassicHacks(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicHacksKeyBindingsScreen(g)); }
		protected static void SwitchNormal(Game g, Widget w) { g.Gui.SetNewScreen(new NormalKeyBindingsScreen(g)); }
		protected static void SwitchHacks(Game g, Widget w) { g.Gui.SetNewScreen(new HacksKeyBindingsScreen(g)); }
		protected static void SwitchOther(Game g, Widget w) { g.Gui.SetNewScreen(new OtherKeyBindingsScreen(g)); }
		protected static void SwitchMouse(Game g, Widget w) { g.Gui.SetNewScreen(new MouseKeyBindingsScreen(g)); }
		
		void OnBindingClick(Game game, Widget widget) {
			if (curI >= 0) {
				ButtonWidget curButton = (ButtonWidget)widgets[curI];
				curButton.SetText(ButtonText(curI));
			}
			
			curI = IndexWidget(widget);
			string text = ButtonText(curI);
			((ButtonWidget)widget).SetText("> " + text + " <");
		}
		
		string ButtonText(int i) {
			if (keyNames == null) keyNames = Enum.GetNames(typeof(Key));
			Key key = game.Input.Keys[binds[i]];
			return desc[i] + ": " + keyNames[(int)key];
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (curI == -1) return base.HandlesKeyDown(key);
			KeyBind bind = binds[curI];
			
			if (key == Key.Escape) key = game.Input.Keys.GetDefault(bind);
			game.Input.Keys[bind] = key;
			
			ButtonWidget curButton = (ButtonWidget)widgets[curI];
			curButton.SetText(ButtonText(curI));
			curI = -1;
			return true;
		}
		
		public override bool HandlesMouseDown(int mouseX, int mouseY, MouseButton button) {
			if (button != MouseButton.Right) {
				return base.HandlesMouseDown(mouseX, mouseY, button);
			}
			int i = HandleMouseDown(widgets, mouseX, mouseY, button);
			if (i == -1) return false;
			
			// Reset a key binding
			if ((curI == -1 || curI == i) && i < binds.Length) {
				curI = i;
				HandlesKeyDown(game.Input.Keys.GetDefault(binds[i]));
			}
			return true;
		}
	}
}