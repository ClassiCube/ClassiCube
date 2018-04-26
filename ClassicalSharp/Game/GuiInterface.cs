// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui;
using ClassicalSharp.Gui.Screens;
using ClassicalSharp.Renderers;

namespace ClassicalSharp {

	public sealed class GuiInterface : IGameComponent {
		
		public int GuiTex, GuiClassicTex, IconsTex;
		Game game;
		StatusScreen statusScreen;
		internal HudScreen hudScreen;
		internal Screen activeScreen;
		internal List<Overlay> overlays = new List<Overlay>();
		
		public GuiInterface(Game game) {
			statusScreen = new StatusScreen(game); game.Components.Add(statusScreen);
			hudScreen = new HudScreen(game); game.Components.Add(hudScreen);
		}
		
		/// <summary> Gets the screen that the user is currently interacting with. </summary>
		/// <remarks> This means if an overlay is active, it will be over the top of other screens. </remarks>
		public Screen ActiveScreen {
			get { return overlays.Count > 0 ? overlays[0]
					: activeScreen == null ? hudScreen : activeScreen; }
		}

		/// <summary> Gets the non-overlay screen that the user is currently interacting with. </summary>
		/// <remarks> This means if an overlay is active, it will return the screen under it. </remarks>
		public Screen UnderlyingScreen {
			get { return activeScreen == null ? hudScreen : activeScreen; }
		}
		
		void IGameComponent.OnNewMap(Game game) { }
		void IGameComponent.OnNewMapLoaded(Game game) { }
		void IGameComponent.Ready(Game game) { }
		
		void IGameComponent.Init(Game game) {
			this.game = game;
			game.Events.TextureChanged += TextureChanged;
		}
		
		public void Reset(Game game) {
			for (int i = 0; i < overlays.Count; i++) {
				overlays[i].Dispose();
			}
			overlays.Clear();
		}
		
		void IDisposable.Dispose() {
			game.Events.TextureChanged -= TextureChanged;
			SetNewScreen(null);
			statusScreen.Dispose();
			
			if (activeScreen != null)
				activeScreen.Dispose();
			game.Graphics.DeleteTexture(ref GuiTex);
			game.Graphics.DeleteTexture(ref GuiClassicTex);
			game.Graphics.DeleteTexture(ref IconsTex);
			
			Reset(game);
		}
		
		void TextureChanged(object sender, TextureEventArgs e) {
			if (e.Name == "gui.png") {
				game.UpdateTexture(ref GuiTex, e.Name, e.Data, false);
			} else if (e.Name == "gui_classic.png") {
				game.UpdateTexture(ref GuiClassicTex, e.Name, e.Data, false);
			} else if (e.Name == "icons.png") {
				game.UpdateTexture(ref IconsTex, e.Name, e.Data, false);
			}
		}
		
		
		public void SetNewScreen(Screen screen) { SetNewScreen(screen, true); }
		
		public void SetNewScreen(Screen screen, bool disposeOld) {
			game.Input.ScreenChanged(activeScreen, screen);
			if (activeScreen != null && disposeOld)
				activeScreen.Dispose();
			
			if (screen == null) {
				game.CursorVisible = false;
				if (game.Focused) game.Camera.RegrabMouse();
			} else if (activeScreen == null) {
				game.CursorVisible = true;
			}
			
			if (screen != null)
				screen.Init();
			activeScreen = screen;
		}
		
		public void RefreshHud() { hudScreen.Recreate(); }
		
		public void ShowOverlay(Overlay overlay, bool inFront) {
			bool cursorVis = game.CursorVisible;
			if (overlays.Count == 0) game.CursorVisible = true;
			
			if (inFront) {
				overlays.Insert(0, overlay);
			} else {
				overlays.Add(overlay);
			}
			if (overlays.Count == 1) game.CursorVisible = cursorVis;
			// Save cursor visibility state
			overlay.Init();
		}
		
		
		public void Render(double delta) {
			game.Graphics.Mode2D(game.Width, game.Height);
			if (activeScreen == null || !activeScreen.HidesHud)
				statusScreen.Render(delta);
			
			if (activeScreen == null || !activeScreen.HidesHud && !activeScreen.RenderHudOver)
				hudScreen.Render(delta);
			if (activeScreen != null)
				activeScreen.Render(delta);
			if (activeScreen != null && !activeScreen.HidesHud && activeScreen.RenderHudOver)
				hudScreen.Render(delta);
			
			if (overlays.Count > 0)
				overlays[0].Render(delta);
			game.Graphics.Mode3D();
		}
		
		internal void OnResize() {
			if (activeScreen != null)
				activeScreen.OnResize(game.Width, game.Height);
			hudScreen.OnResize(game.Width, game.Height);
			
			for (int i = 0; i < overlays.Count; i++) {
				overlays[i].OnResize(game.Width, game.Height);
			}
		}
	}
}