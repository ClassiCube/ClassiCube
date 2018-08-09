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
				game.LoadTexture(ref GuiTex, e.Name, e.Data);
			} else if (e.Name == "gui_classic.png") {
				game.LoadTexture(ref GuiClassicTex, e.Name, e.Data);
			} else if (e.Name == "icons.png") {
				game.LoadTexture(ref IconsTex, e.Name, e.Data);
			}
		}
		
		
		public void SetNewScreen(Screen screen) { SetNewScreen(screen, true); }
		
		public void SetNewScreen(Screen screen, bool disposeOld) {
			game.Input.ScreenChanged(activeScreen, screen);
			if (activeScreen != null && disposeOld)
				activeScreen.Dispose();
			
			if (screen != null) screen.Init();
			activeScreen = screen;
			CalcCursorVisible();
		}
		
		public void RefreshHud() { hudScreen.Recreate(); }
		
		public void ShowOverlay(Overlay overlay, bool inFront) {
			if (inFront) {
				overlays.Insert(0, overlay);
			} else {
				overlays.Add(overlay);
			}
			
			overlay.Init();
			CalcCursorVisible();
		}
		
		public void DisposeOverlay(Overlay overlay) {
			overlay.Dispose();		
			overlays.Remove(overlay);
			CalcCursorVisible();
		}
		
		
		public void Render(double delta) {
			game.Graphics.Mode2D(game.Width, game.Height);
			bool showHUD   = activeScreen == null || !activeScreen.HidesHud;
			bool hudBefore = activeScreen == null || !activeScreen.RenderHudOver;
			if (showHUD) statusScreen.Render(delta);
			
			if (showHUD && hudBefore) hudScreen.Render(delta);
			if (activeScreen != null) activeScreen.Render(delta);
			if (showHUD && !hudBefore) hudScreen.Render(delta);
			
			if (overlays.Count > 0) { overlays[0].Render(delta); }
			game.Graphics.Mode3D();
		}
		
		internal void OnResize() {
			if (activeScreen != null) activeScreen.OnResize();
			hudScreen.OnResize();
			
			for (int i = 0; i < overlays.Count; i++) {
				overlays[i].OnResize();
			}
		}
		
		bool cursorVisible = true;
		public void CalcCursorVisible() {
			bool vis = ActiveScreen.HandlesAllInput;
			if (vis == cursorVisible) return;
			cursorVisible = vis;
			
			game.window.CursorVisible = vis;
			if (game.window.Focused)
				game.Camera.RegrabMouse();
		}
	}
}