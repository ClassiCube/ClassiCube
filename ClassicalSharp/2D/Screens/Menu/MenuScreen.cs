// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class MenuScreen : ClickableScreen {
		
		public MenuScreen(Game game) : base(game) {
			HandlesAllInput = true;
		}
		
		protected Widget[] widgets;
		protected Font titleFont, textFont;
		protected int IndexWidget(Widget w) { return IndexWidget(widgets, w); }
		
		public override void Render(double delta) {
			RenderMenuBounds();
			game.Graphics.Texturing = true;
			RenderWidgets(widgets, delta);
			game.Graphics.Texturing = false;
		}
		
		public override void Init() {
			if (titleFont == null) titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			if (textFont == null) textFont = new Font(game.FontName, 16);
			
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
		}
		
		public override void Dispose() {
			ContextLost();
			if (titleFont != null) { titleFont.Dispose(); titleFont = null; }
			if (textFont != null)  { textFont.Dispose(); textFont = null; }
			
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}
		
		protected override void ContextLost() { DisposeWidgets(widgets); }

		public override void OnResize(int width, int height) {
			RepositionWidgets(widgets);
		}
		
		
		public override bool HandlesMouseDown(int mouseX, int mouseY, MouseButton button) {
			return HandleMouseDown(widgets, mouseX, mouseY, button) >= 0;
		}
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			return HandleMouseMove(widgets, mouseX, mouseY) >= 0;
		}		
		public override bool HandlesMouseScroll(float delta) { return true; }	
		
		public override bool HandlesKeyDown(Key key) { 
			if (key == Key.Escape) {
				game.Gui.SetNewScreen(null);
				return true;
			}
			return key < Key.F1 || key > Key.F35;
		}
		public override bool HandlesKeyPress(char key) { return true; }
		public override bool HandlesKeyUp(Key key) { return true; }
	}
}