// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class MenuScreen : ClickableScreen {
		
		public MenuScreen(Game game) : base(game) {
			HandlesAllInput = true;
		}
		protected Widget[] widgets;
		protected Font titleFont, regularFont;

		protected int IndexOfWidget(Widget w) {
			for (int i = 0; i < widgets.Length; i++) {
				if (widgets[i] == w) return i;
			}
			return -1;
		}
		
		public override void Render(double delta) {
			RenderMenuBounds();
			gfx.Texturing = true;
			RenderWidgets(widgets, delta);
			gfx.Texturing = false;
		}
		
		public override void Init() {
			gfx.ContextLost += ContextLost;
			gfx.ContextRecreated += ContextRecreated;
		}
		
		public override void Dispose() {
			ContextLost();
			if (titleFont != null) titleFont.Dispose();
			if (regularFont != null) regularFont.Dispose();
			
			gfx.ContextLost -= ContextLost;
			gfx.ContextRecreated -= ContextRecreated;
		}
		
		protected override void ContextLost() { DisposeWidgets(widgets); }

		public override void OnResize(int width, int height) {
			RepositionWidgets(widgets);
		}
		
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			return HandleMouseClick(widgets, mouseX, mouseY, button);
		}
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			return HandleMouseMove(widgets, mouseX, mouseY);
		}
		
		public override bool HandlesMouseScroll(float delta) { return true; }
		
		public override bool HandlesKeyPress(char key) { return true; }
		
		public override bool HandlesKeyDown(Key key) { return true; }
		
		public override bool HandlesKeyUp(Key key) { return true; }
	}
}