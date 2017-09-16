// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
//#define TEST_VANILLA
using System;
using System.Net;
using ClassicalSharp.Entities;
using ClassicalSharp.Generator;
using ClassicalSharp.Gui.Screens;
using ClassicalSharp.Physics;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Singleplayer {

	public sealed class SinglePlayerServer : IServerConnection {
		
		internal PhysicsBase physics;
		IMapGenerator gen;
		string lastState;
		
		public SinglePlayerServer(Game window) {
			game = window;
			physics = new PhysicsBase(game);
			SupportsFullCP437 = !game.ClassicMode;
			SupportsPartialMessages = true;
		}
		
		public override bool IsSinglePlayer { get { return true; } }
		
		public override void Connect(IPAddress address, int port) {
			game.Chat.SetLogName("Singleplayer");
			game.UseCPEBlocks = game.UseCPE;
			int max = game.UseCPEBlocks ? Block.MaxCpeBlock : Block.MaxOriginalBlock;
			for (int i = 1; i <= max; i++) {
				BlockInfo.CanPlace[i] = true;
				BlockInfo.CanDelete[i] = true;
			}
			game.AsyncDownloader.DownloadSkin(game.LocalPlayer.SkinName,
			                                  game.LocalPlayer.SkinName);
			
			game.Events.RaiseBlockPermissionsChanged();
			int seed = new Random().Next();
			GenMap(128, 64, 128, seed, new NotchyGenerator());
		}
		
		char lastCol = '\0';
		public override void SendChat(string text, bool partial) {
			if (!String.IsNullOrEmpty(text))
				AddChat(text);
			if (!partial) lastCol = '\0';
		}
		
		void AddChat(string text) {
			text = text.TrimEnd().Replace('%', '&');
			if (!IDrawer2D.IsWhiteColour(lastCol))
				text = "&" + lastCol + text;
			
			char col = game.Drawer2D.LastColour(text, text.Length);
			if (col != '\0') lastCol = col;
			game.Chat.Add(text, MessageType.Normal);
		}
		
		public override void SendPosition(Vector3 pos, float rotY, float headX) {
		}
		
		public override void SendPlayerClick(MouseButton button, bool buttonDown, byte targetId, PickedPos pos) {
		}
		
		public override void Dispose() {
			physics.Dispose();
		}
		
		public override void Tick(ScheduledTask task) {
			if (Disconnected) return;
			physics.Tick();
			CheckAsyncResources();
			
			if (gen == null) return;
			if (gen.Done) { EndGeneration(); return; }
			
			string state = gen.CurrentState;
			float progress = gen.CurrentProgress;
			LoadingMapScreen screen = ((LoadingMapScreen)game.Gui.UnderlyingScreen);
			
			screen.SetProgress(progress);
			if (state == lastState) return;
			lastState = state;
			screen.SetMessage(state);
		}
		
		void EndGeneration() {
			game.Gui.SetNewScreen(null);
			if (gen.Blocks == null) {
				game.Chat.Add("&cFailed to generate the map.");
			} else {
				game.World.SetNewMap(gen.Blocks, gen.Width, gen.Height, gen.Length);
				gen.Blocks = null;
				ResetPlayerPosition();
				
				game.WorldEvents.RaiseOnNewMapLoaded();
				gen.ApplyEnv(game.World);
			}
			
			gen = null;
			GC.Collect();
		}
		
		internal void GenMap(int width, int height, int length, int seed, IMapGenerator gen) {
			game.World.Reset();
			game.WorldEvents.RaiseOnNewMap();
			
			GC.Collect();
			this.gen = gen;
			game.Gui.SetNewScreen(new LoadingMapScreen(game, "Generating level", "Generating.."));
			gen.Width = width; gen.Height = height; gen.Length = length; gen.Seed = seed;
			gen.GenerateAsync(game);
		}		

		void ResetPlayerPosition() {
			float x = (game.World.Width / 2) + 0.5f;
			float z = (game.World.Length / 2) + 0.5f;			
			Vector3 spawn = Respawn.FindSpawnPosition(game, x, z, game.LocalPlayer.Size);
			
			LocationUpdate update = LocationUpdate.MakePosAndOri(spawn, 0, 0, false);
			game.LocalPlayer.SetLocation(update, false);
			game.LocalPlayer.Spawn = spawn;
			game.CurrentCameraPos = game.Camera.GetCameraPos(0);
		}
	}
}