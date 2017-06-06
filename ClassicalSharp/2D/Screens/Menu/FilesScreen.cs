// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class FilesScreen : ClickableScreen {
		
		public FilesScreen(Game game) : base(game) {
			HandlesAllInput = true;
		}
		
		protected Font textFont, arrowFont, titleFont;
		protected string[] entries;
		protected int currentIndex;
		protected ButtonWidget[] buttons;
		protected const int items = 5;
		
		TextWidget title;
		protected string titleText;
		
		public override void Init() {
			textFont = new Font(game.FontName, 16, FontStyle.Bold);
			arrowFont = new Font(game.FontName, 16, FontStyle.Bold);
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			
			ContextRecreated();
			gfx.ContextLost += ContextLost;
			gfx.ContextRecreated += ContextRecreated;
		}
		
		protected override void ContextLost() {
			for (int i = 0; i < buttons.Length; i++)
				buttons[i].Dispose();
			title.Dispose();
		}
		
		protected override void ContextRecreated() {
			title = TextWidget.Create(game, titleText, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -155);
			
			buttons = new ButtonWidget[] {
				MakeText(0, -100, Get(0)),
				MakeText(0, -50, Get(1)),
				MakeText(0, 0, Get(2)),
				MakeText(0, 50, Get(3)),
				MakeText(0, 100, Get(4)),
				
				Make(-220, 0, "<", (g, w) => PageClick(false)),
				Make(220, 0, ">", (g, w) => PageClick(true)),
				MakeBack(false, titleFont, 
				         (g, w) => g.Gui.SetNewScreen(new PauseScreen(g))),
			};
			UpdateArrows();
		}
		
		string Get(int index) {
			return index < entries.Length ? entries[index] : "-----";
		}
		
		public override void Dispose() {
			ContextLost();
			gfx.ContextLost -= ContextLost;
			gfx.ContextRecreated -= ContextRecreated;			
			
			textFont.Dispose();
			arrowFont.Dispose();
			titleFont.Dispose();
		}
		
		ButtonWidget MakeText(int x, int y, string text) {
			return ButtonWidget.Create(game, 300, text, textFont, TextButtonClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, x, y);
		}
		
		ButtonWidget Make(int x, int y, string text, SimpleClickHandler onClick) {
			return ButtonWidget.Create(game, 40, text, arrowFont, LeftOnly(onClick))
				.SetLocation(Anchor.Centre, Anchor.Centre, x, y);
		}
		
		protected abstract void TextButtonClick(Game game, Widget widget, MouseButton btn, int x, int y);
		
		protected void PageClick(bool forward) {
			SetCurrentIndex(currentIndex + (forward ? items : -items));
		}
		
		protected void SetCurrentIndex(int index) {			
			if (index >= entries.Length) index -= items;
			if (index < 0) index = 0;
			currentIndex = index;
			
			for (int i = 0; i < items; i++)
				buttons[i].SetText(Get(currentIndex + i));
			UpdateArrows();
		}
		
		protected void UpdateArrows() {
			buttons[5].Disabled = false;
			buttons[6].Disabled = false;
			if (currentIndex < items)
				buttons[5].Disabled = true;
			if (currentIndex >= entries.Length - items)
				buttons[6].Disabled = true;
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
			return HandleMouseMove(buttons, mouseX, mouseY);
		}
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			return HandleMouseClick(buttons, mouseX, mouseY, button);
		}
		
		public override void OnResize(int width, int height) {
			for (int i = 0; i < buttons.Length; i++)
				buttons[i].CalculatePosition();
			title.CalculatePosition();
		}
		
		public override void Render(double delta) {
			gfx.Draw2DQuad(0, 0, game.Width, game.Height, new FastColour(60, 60, 60, 160));
			gfx.Texturing = true;
			title.Render(delta);
			for (int i = 0; i < buttons.Length; i++)
				buttons[i].Render(delta);
			gfx.Texturing = false;
		}
	}
}