// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class HudScreen : Screen, IGameComponent {
		
		public HudScreen(Game game) : base(game) { }
		
		ChatScreen chat;
		internal Widget hotbar;
		PlayerListWidget playerList;
		Font playerFont;
		
		public void Init(Game game) { }
		public void Ready(Game game) { Init(); }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void OnNewMapLoaded(Game game) { }
		
		internal int BottomOffset { get { return hotbar.Height; } }
		
		public override void Render(double delta) {
			if (game.HideGui) return;
			
			bool showMinimal = game.Gui.ActiveScreen.BlocksWorld;
			if (chat.HandlesAllInput && !game.PureClassic)
				chat.RenderBackground();
			
			gfx.Texturing = true;
			if (!showMinimal) hotbar.Render(delta);
			chat.Render(delta);
			
			if (playerList != null && game.Gui.ActiveScreen == this) {
				playerList.Render(delta);
				// NOTE: Should usually be caught by KeyUp, but just in case.
				if (!game.IsKeyDown(KeyBind.PlayerList)) {
					playerList.Dispose();
					playerList = null;
				}
			}
			
			if (playerList == null && !showMinimal)
				DrawCrosshairs();
			gfx.Texturing = false;
		}
		
		const int chExtent = 16, chWeight = 2;
		static TextureRec chRec = new TextureRec(0, 0, 15/256f, 15/256f);
		void DrawCrosshairs() {			
			if (game.Gui.IconsTex <= 0) return;
			
			int cenX = game.Width / 2, cenY = game.Height / 2;
			int extent = (int)(chExtent * game.Scale(game.Height / 480f));
			Texture chTex = new Texture(game.Gui.IconsTex, cenX - extent,
			                            cenY - extent, extent * 2, extent * 2, chRec);
			chTex.Render(gfx);
		}
		
		bool hadPlayerList;
		protected override void ContextLost() {
			DisposePlayerList();
			hotbar.Dispose();
		}
		
		void DisposePlayerList() {
			hadPlayerList = playerList != null;
			if (playerList != null)
				playerList.Dispose();
			playerList = null;
		}
		
		protected override void ContextRecreated() {
			hotbar.Dispose();
			hotbar.Init();
			
			if (!hadPlayerList) return;
			
			if (game.UseClassicTabList) {
				playerList = new ClassicPlayerListWidget(game, playerFont);
			} else if (game.Server.UsingExtPlayerList) {
				playerList = new ExtPlayerListWidget(game, playerFont);
			} else {
				playerList = new NormalPlayerListWidget(game, playerFont);
			}
			
			playerList.Init();
			playerList.RecalcYOffset();
			playerList.Reposition();
		}
		
		public override void Dispose() {
			playerFont.Dispose();
			chat.Dispose();
			ContextLost();
			
			game.WorldEvents.OnNewMap -= OnNewMap;
			gfx.ContextLost -= ContextLost;
			gfx.ContextRecreated -= ContextRecreated;
		}
		
		public void GainFocus() {
			game.CursorVisible = false;
			if (game.Focused)
				game.Camera.RegrabMouse();
		}
		
		public void LoseFocus() {
			game.CursorVisible = true;
		}
		
		public override void OnResize(int width, int height) {
			chat.OnResize(width, height);
			hotbar.Reposition();
			
			if (playerList != null) {
				playerList.RecalcYOffset();
				playerList.Reposition();
			}
		}
		
		public override void Init() {
			int size = game.Drawer2D.UseBitmappedChat ? 16 : 11;
			playerFont = new Font(game.FontName, size);
			hotbar = game.Mode.MakeHotbar();
			hotbar.Init();
			chat = new ChatScreen(game, this);
			chat.Init();
			
			game.WorldEvents.OnNewMap += OnNewMap;
			gfx.ContextLost += ContextLost;
			gfx.ContextRecreated += ContextRecreated;
		}

		void OnNewMap(object sender, EventArgs e) { DisposePlayerList(); }

		public override bool HandlesKeyPress(char key) { return chat.HandlesKeyPress(key); }
		
		public override bool HandlesKeyDown(Key key) {
			Key playerListKey = game.Mapping(KeyBind.PlayerList);
			bool handles = playerListKey != Key.Tab || !game.TabAutocomplete || !chat.HandlesAllInput;
			if (key == playerListKey && handles) {
				if (playerList == null && !game.Server.IsSinglePlayer) {
					hadPlayerList = true;
					ContextRecreated();
				}
				return true;
			}
			
			if (chat.HandlesKeyDown(key))
				return true;
			return hotbar.HandlesKeyDown(key);
		}
		
		public override bool HandlesKeyUp(Key key) {
			if (key == game.Mapping(KeyBind.PlayerList)) {
				if (playerList != null) {
					hadPlayerList = false;
					playerList.Dispose();
					playerList = null;
					return true;
				}
			}
			
			if (chat.HandlesAllInput) return true;
			return hotbar.HandlesKeyUp(key);
		}
		
		public void OpenTextInputBar(string text) {
			chat.OpenTextInputBar(text);
		}
		
		public override bool HandlesMouseScroll(float delta) {
			return chat.HandlesMouseScroll(delta);
		}
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			if (button != MouseButton.Left || !HandlesAllInput) return false;
			
			string name;
			if (playerList == null || (name = playerList.GetNameUnder(mouseX, mouseY)) == null)
				return chat.HandlesMouseClick(mouseX, mouseY, button);
			chat.AppendTextToInput(name + " ");
			return true;
		}
	}
}
