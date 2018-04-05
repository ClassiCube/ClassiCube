// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class ClickableScreen : Screen {
		
		public ClickableScreen(Game game) : base(game) {
		}		
		
		// These were sourced by taking a screenshot of vanilla
		// Then using paint to extract the colour components
		// Then using wolfram alpha to solve the glblendfunc equation
		static FastColour topBackCol = new FastColour(24, 24, 24, 105);
		static FastColour bottomBackCol = new FastColour(51, 51, 98, 162);

		protected void RenderMenuBounds() {
			game.Graphics.Draw2DQuad(0, 0, game.Width, game.Height, topBackCol, bottomBackCol);
		}
		
		protected int HandleMouseDown(Widget[] widgets, int mouseX, int mouseY, MouseButton button) {
			// iterate backwards (because last elements rendered are shown over others)
			for (int i = widgets.Length - 1; i >= 0; i--) {
				Widget widget = widgets[i];
				if (widget == null || !widget.Bounds.Contains(mouseX, mouseY)) continue;
				if (widget.Disabled) return i;
				
				if (widget.MenuClick != null && button == MouseButton.Left) {
					widget.MenuClick(game, widget);
				} else {
					widget.HandlesMouseDown(mouseX, mouseY, button);
				}
				return i;
			}
			return -1;
		}
		
		protected int HandleMouseMove(Widget[] widgets, int mouseX, int mouseY) {
			for (int i = 0; i < widgets.Length; i++) {
				if (widgets[i] == null || !widgets[i].Active) continue;
				widgets[i].Active = false;
			}
			
			for (int i = widgets.Length - 1; i >= 0; i--) {
				Widget widget = widgets[i];
				if (widget == null || !widget.Bounds.Contains(mouseX, mouseY)) continue;
				
				widget.Active = true;
				return i;
			}
			return -1;
		}

		protected ButtonWidget MakeBack(bool toGame, Font font, ClickHandler onClick) {
			int width = game.UseClassicOptions ? 400 : 200;
			return MakeBack(width, toGame ? "Back to game" : "Cancel", 25, font, onClick);
		}
		
		protected ButtonWidget MakeBack(int width, string text, int y, Font font, ClickHandler onClick) {
			return ButtonWidget.Create(game, width, text, font, onClick)
				.SetLocation(Anchor.Centre, Anchor.Max, 0, y);
		}
		
		protected static void SwitchOptions(Game g, Widget w) { g.Gui.SetNewScreen(new OptionsGroupScreen(g)); }
		protected static void SwitchPause(Game g, Widget w) { g.Gui.SetNewScreen(new PauseScreen(g)); }
		
				
		protected static void DisposeWidgets<T>(T[] widgets) where T : Widget {
			if (widgets == null) return;
			
			for (int i = 0; i < widgets.Length; i++) {
				if (widgets[i] != null) widgets[i].Dispose();
			}
		}
		
		protected static void RepositionWidgets<T>(T[] widgets) where T : Widget {
			if (widgets == null) return;
			
			for (int i = 0; i < widgets.Length; i++) {
				if (widgets[i] != null) widgets[i].Reposition();
			}
		}
		
		protected static void RenderWidgets<T>(T[] widgets, double delta) where T : Widget {
			if (widgets == null) return;
			
			for (int i = 0; i < widgets.Length; i++) {
				if (widgets[i] != null) widgets[i].Render(delta);
			}
		}
	}
}