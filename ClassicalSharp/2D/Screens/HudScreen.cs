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
			IGraphicsApi gfx = game.Graphics;
			if (game.HideGui && chat.HandlesAllInput) {
				gfx.Texturing = true;
				chat.input.Render(delta);
				gfx.Texturing = false;
			}
			if (game.HideGui) return;
			bool showMinimal = game.Gui.ActiveScreen.BlocksWorld;
			
			if (playerList == null && !showMinimal) {
				gfx.Texturing = true;
				DrawCrosshairs();
				gfx.Texturing = false;
			}			
			if (chat.HandlesAllInput && !game.PureClassic) {
				chat.RenderBackground();
			}
			
			gfx.Texturing = true;
			if (!showMinimal) hotbar.Render(delta);
			chat.Render(delta);
			
			if (playerList != null && game.Gui.ActiveScreen == this) {
				playerList.Active = chat.HandlesAllInput;
				playerList.Render(delta);
				// NOTE: Should usually be caught by KeyUp, but just in case.
				if (!game.IsKeyDown(KeyBind.PlayerList)) {
					playerList.Dispose();
					playerList = null;
				}
			}
			
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
			chTex.Render(game.Graphics);
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
			bool extended = game.Server.UsingExtPlayerList && !game.UseClassicTabList;
			playerList = new PlayerListWidget(game, playerFont, !extended);

			playerList.Init();
			playerList.Reposition();
		}
		
		public override void Dispose() {
			playerFont.Dispose();
			chat.Dispose();
			ContextLost();
			
			game.WorldEvents.OnNewMap -= OnNewMap;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}
		
		public override void OnResize(int width, int height) {
			chat.OnResize(width, height);
			hotbar.Reposition();
			
			if (playerList != null) {
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
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
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
			
			return chat.HandlesKeyDown(key) || hotbar.HandlesKeyDown(key);
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
			
			return chat.HandlesKeyUp(key) || hotbar.HandlesKeyUp(key);
		}
		
		public void OpenInput(string text) { chat.OpenInput(text); }
		
		public void AppendInput(string text) { chat.input.Append(text); }
		
		public override bool HandlesMouseScroll(float delta) {
			return chat.HandlesMouseScroll(delta);
		}
		
		public override bool HandlesMouseDown(int mouseX, int mouseY, MouseButton button) {
			if (button != MouseButton.Left || !HandlesAllInput) return false;
			
			string name;
			if (playerList == null || (name = playerList.GetNameUnder(mouseX, mouseY)) == null)
				return chat.HandlesMouseDown(mouseX, mouseY, button);
			chat.AppendTextToInput(name + " ");
			return true;
		}
	}
}
