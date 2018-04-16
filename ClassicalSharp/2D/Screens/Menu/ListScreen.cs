// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class ListScreen : ClickableScreen {
		
		public ListScreen(Game game) : base(game) {
			HandlesAllInput = true;
		}
		
		protected Font font;
		protected string[] entries;
		protected int currentIndex;
		protected Widget[] widgets;
		protected const int items = 5;
		protected const string empty = "-----";		
		protected string titleText;
		
		public override void Init() {
			font = new Font(game.FontName, 16, FontStyle.Bold);
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
			};
			UpdateArrows();
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
			return ButtonWidget.Create(game, 300, text, font, TextButtonClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, (i - 2) * 50);
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
			if (index >= entries.Length) index -= items;
			if (index < 0) index = 0;
			currentIndex = index;
			
			for (int i = 0; i < items; i++) {
				((ButtonWidget)widgets[i]).SetText(Get(currentIndex + i));
			}
			UpdateArrows();
		}
		
		void UpdateArrows() {
			widgets[5].Disabled = false;
			widgets[6].Disabled = false;
			if (currentIndex < items)
				widgets[5].Disabled = true;
			if (currentIndex >= entries.Length - items)
				widgets[6].Disabled = true;
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
		
		public override void OnResize(int width, int height) {
			RepositionWidgets(widgets);
		}
	}
}