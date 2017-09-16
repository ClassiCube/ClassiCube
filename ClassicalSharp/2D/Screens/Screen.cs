// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	/// <summary> Represents a container of widgets and other 2D elements. </summary>
	/// <remarks> May cover the entire game window. </remarks>
	public abstract class Screen : GuiElement {
		
		public Screen(Game game) : base(game) {
		}
		
		/// <summary> Whether this screen handles all mouse and keyboard input. </summary>
		/// <remarks> This prevents the client from interacting with the world. </remarks>
		public bool HandlesAllInput;
		
		/// <summary> Whether this screen completely and opaquely covers the game world behind it. </summary>
		public bool BlocksWorld;
		
		/// <summary> Whether this screen hides the normal in-game hud. </summary>
		public bool HidesHud;
		
		/// <summary> Whether the normal in-game hud should be drawn over the top of this screen. </summary>
		public bool RenderHudOver;

		/// <summary> Called when the game window is resized. </summary>
		public abstract void OnResize(int width, int height);
		
		protected ClickHandler LeftOnly(SimpleClickHandler action) {
			return delegate(Game g, Widget w, MouseButton btn, int x, int y) {
				if (btn != MouseButton.Left) return;
				if (action != null) action(g, w);
			};
		}
		
		protected abstract void ContextLost();
		
		protected abstract void ContextRecreated();
		
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