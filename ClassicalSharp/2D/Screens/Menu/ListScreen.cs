// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class ListScreen : ClickableScreen {
		
		public ListScreen(Game game) : base(game) { }
		
		protected Font font;
		protected string[] entries;
		protected int currentIndex;
		protected Widget[] widgets;
		protected const int items = 5;
		protected const string empty = "-----";		
		protected string titleText;
		
		public override void Init() {
			font = new Font(game.FontName, 16, FontStyle.Bold);
			Keyboard.KeyRepeat = true;
			ContextRecreated();
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
		}
		
		public override void Render(double delta) {
			RenderMenuBounds();
			game.Graphics.Texturing = true;
			RenderWidgets(widgets, delta);
			game.Graphics.Texturing = false;
		}
		
		public override void Dispose() {
			font.Dispose();
			Keyboard.KeyRepeat = false;
			ContextLost();
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}
		
		protected override void ContextLost() {
			DisposeWidgets(widgets);
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[] {
				MakeText(0), MakeText(1), MakeText(2), MakeText(3), MakeText(4),
				
				Make(-220, "<", MoveBackwards),
				Make(220,  ">", MoveForwards),
				MakeBack(false, font, SwitchPause),
				TextWidget.Create(game, titleText, font)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -155),
				TextWidget.Create(game, "", font)
					.SetLocation(Anchor.Centre, Anchor.Max, 0, 75),
			};
			UpdatePage();
		}
		
		void MoveBackwards(Game g, Widget w) { PageClick(false); }
		void MoveForwards(Game g, Widget w) { PageClick(true); }
		string Get(int i) { return i < entries.Length ? entries[i] : empty; }
		
		protected string GetCur(Widget w) {
			int idx = IndexWidget(widgets, w);
			return Get(currentIndex + idx);
		}
		
		ButtonWidget MakeText(int i) {
			string text = Get(currentIndex + i);
			ButtonWidget btn = ButtonWidget.Create(game, 300, "", font, TextButtonClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, (i - 2) * 50);
			UpdateText(btn, text);
			return btn;
		}
		
		ButtonWidget Make(int x, string text, ClickHandler onClick) {
			return ButtonWidget.Create(game, 40, text, font, onClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, x, 0);
		}
		
		protected abstract void TextButtonClick(Game game, Widget widget);
		
		void PageClick(bool forward) {
			SetCurrentIndex(currentIndex + (forward ? items : -items));
		}
		
		protected void SetCurrentIndex(int index) {
			if (index >= entries.Length) { index = entries.Length - 1; }
			if (index < 0) index = 0;
			currentIndex = index;
			
			for (int i = 0; i < items; i++) {
				UpdateText((ButtonWidget)widgets[i], Get(currentIndex + i));
			}
			UpdatePage();
		}
		
		protected virtual void UpdateText(ButtonWidget widget, string text) {
			widget.SetText(text);
		}
		
		void UpdatePage() {
			int start = items, end = entries.Length - items;
			widgets[5].Disabled = currentIndex < start;
			widgets[6].Disabled = currentIndex >= end;		
			if (game.ClassicMode) return;
			
			TextWidget page = (TextWidget)widgets[9];
			int num   = (currentIndex / items) + 1;
			int pages = Utils.CeilDiv(entries.Length, items);
			if (pages == 0) pages = 1;
			page.SetText("&7Page " + num + " of " + pages);
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (key == Key.Escape) {
				game.Gui.SetNewScreen(null);
			} else if (key == Key.Left) {
				PageClick(false);
			} else if (key == Key.Right) {
				PageClick(true);
			} else {
				return false;
			}
			return true;
		}
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			return HandleMouseMove(widgets, mouseX, mouseY) >= 0;
		}
		
		public override bool HandlesMouseDown(int mouseX, int mouseY, MouseButton button) {
			return HandleMouseDown(widgets, mouseX, mouseY, button) >= 0;
		}
		
		float wheelAcc;
		public override bool HandlesMouseScroll(float delta) {
			int steps = Utils.AccumulateWheelDelta(ref wheelAcc, delta);
			if (steps != 0) SetCurrentIndex(currentIndex - steps);
			return true;
		}
		
		public override void OnResize() {
			RepositionWidgets(widgets);
		}
	}
}