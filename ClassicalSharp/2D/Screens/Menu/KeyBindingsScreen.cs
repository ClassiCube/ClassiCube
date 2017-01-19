// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class KeyBindingsScreen : MenuScreen {
		
		public KeyBindingsScreen(Game game) : base(game) { }
		
		Font keyFont;
		static string[] keyNames;
		protected string[] leftDesc, rightDesc;
		protected KeyBind[] left, right;
		
		protected int btnDistance = 45, btnWidth = 260, btnHeight = 35;
		protected string title = "Controls";
		protected int index;
		protected Action<Game, Widget> leftPage, rightPage;
		
		public override void Init() {
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			keyFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16, FontStyle.Italic);
			
			if (keyNames == null)
				keyNames = Enum.GetNames(typeof(Key));
		}

		protected void MakeWidgets(int y) {
			int origin = y;
			MakeOthers();
			
			if (right == null) {
				for (int i = 0; i < left.Length; i++)
					Make(i, 0, ref y);
			} else {
				for (int i = 0; i < left.Length; i++)
					Make(i, -btnWidth / 2 - 5, ref y);
				
				y = origin;
				for (int i = 0; i < right.Length; i++)
					Make(i + left.Length, btnWidth / 2 + 5, ref y);
			}
			MakePages(origin);
		}
		
		void MakeOthers() {
			widgets[index++] = TextWidget.Create(game, title, keyFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -180);
			if (game.ClassicMode) {
				widgets[index++] = MakeBack(false, titleFont,
				                            (g, w) => g.Gui.SetNewScreen(new ClassicOptionsScreen(g)));
			} else {
				widgets[index++] = MakeBack("Back to menu", 5, titleFont,
				                            (g, w) => g.Gui.SetNewScreen(new OptionsGroupScreen(g)));
			}
		}
		
		void MakePages(int origin) {
			if (leftPage == null && rightPage == null) return;
			int btnY = origin + btnDistance * (left.Length / 2);
			widgets[index++] = ButtonWidget.Create(game, btnHeight, btnHeight, "<", keyFont, LeftOnly(leftPage))
				.SetLocation(Anchor.Centre, Anchor.Centre, -btnWidth - 35, btnY);
			widgets[index++] = ButtonWidget.Create(game, btnHeight, btnHeight, ">", keyFont, LeftOnly(rightPage))
				.SetLocation(Anchor.Centre, Anchor.Centre, btnWidth + 35, btnY);
			if (leftPage == null) widgets[index - 2].Disabled = true;
			if (rightPage == null) widgets[index - 1].Disabled = true;
		}
		
		void Make(int i, int x, ref int y) {
			string text = ButtonText(i);
			widgets[index++] = ButtonWidget.Create(game, btnWidth, btnHeight, text, keyFont, OnBindingClick)				
				.SetLocation(Anchor.Centre, Anchor.Centre, x, y);
			y += btnDistance;
		}
		
		
		ButtonWidget curWidget;
		void OnBindingClick(Game game, Widget widget, MouseButton btn, int x, int y) {
			int index = 0;
			if (btn == MouseButton.Right && (curWidget == null || curWidget == widget)) {
				curWidget = (ButtonWidget)widget;
				index = Array.IndexOf<Widget>(widgets, curWidget) - 2;
				KeyBind mapping = Get(index, left, right);
				HandlesKeyDown(game.Input.Keys.GetDefault(mapping));
			}
			if (btn != MouseButton.Left) return;
			
			if (curWidget != null) {
				index = Array.IndexOf<Widget>(widgets, curWidget) - 2;
				curWidget.SetText(ButtonText(index));
				curWidget = null;
			}
			
			index = Array.IndexOf<Widget>(widgets, widget) - 2;
			string text = ButtonText(index);
			curWidget = (ButtonWidget)widget;
			curWidget.SetText("> " + text + " <");
		}
		
		string ButtonText(int i) {
			KeyBind mapping = Get(i, left, right);
			Key key = game.Input.Keys[mapping];
			string desc = Get(i, leftDesc, rightDesc);
			return desc + ": " + keyNames[(int)key];
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (key == Key.Escape) {
				game.Gui.SetNewScreen(null);
			} else if (curWidget != null) {
				int index = Array.IndexOf<Widget>(widgets, curWidget) - 2;
				KeyBind mapping = Get(index, left, right);
				game.Input.Keys[mapping] = key;			
				curWidget.SetText(ButtonText(index));
				curWidget = null;
			}
			return true;
		}
		
		T Get<T>(int index, T[] a, T[] b) {
			return index < a.Length ? a[index] : b[index - a.Length];
		}
		
		public override void Dispose() {
			keyFont.Dispose();
			base.Dispose();
		}
	}
}