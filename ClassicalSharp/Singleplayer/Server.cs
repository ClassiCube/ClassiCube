// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
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
		internal byte[] generatedMap;
		IMapGenerator generator;
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
				game.Inventory.CanPlace[i] = true;
				game.Inventory.CanDelete[i] = true;
			}
			game.AsyncDownloader.DownloadSkin(game.LocalPlayer.SkinIdentifier,
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
		
		public override void SendPosition(Vector3 pos, float yaw, float pitch) {
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
			
			if (generator == null) return;
			if (generator.Done) { EndGeneration(); return; }
			
			string state = generator.CurrentState;
			float progress = generator.CurrentProgress;
			LoadingMapScreen screen = ((LoadingMapScreen)game.Gui.ActiveScreen);
			
			screen.SetProgress(progress);
			if (state == lastState) return;
			lastState = state;
			screen.SetMessage(state);
		}
		
		void EndGeneration() {
			game.Gui.SetNewScreen(null);
			if (generatedMap == null) {
				game.Chat.Add("&cFailed to generate the map.");
			} else {
				IMapGenerator gen = generator;
				game.World.SetNewMap(generatedMap, gen.Width, gen.Height, gen.Length);
				generatedMap = null;
				ResetPlayerPosition();
				game.WorldEvents.RaiseOnNewMapLoaded();
			}
			
			generator = null;
			game.Chat.Add("&ePlaying single player", MessageType.Status1);
			GC.Collect();
		}
		
		internal void GenMap(int width, int height, int length, int seed, IMapGenerator generator) {
			game.World.Reset();
			GC.Collect();
			this.generator = generator;
			game.Gui.SetNewScreen(new LoadingMapScreen(game, "Generating level", "Generating.."));
			generator.GenerateAsync(game, width, height, length, seed);
		}
		

		void ResetPlayerPosition() {
			Vector3 spawn = default(Vector3);
			spawn.X = (game.World.Width / 2) + 0.5f;
			spawn.Y = game.World.Height + Entity.Adjustment;
			spawn.Z = (game.World.Length / 2) + 0.5f;
			
			AABB bb = AABB.Make(spawn, game.LocalPlayer.Size);
			spawn.Y = 0;		
			for (int y = game.World.Height; y >= 0; y--) {
				float highestY = Respawn.HighestFreeY(game, ref bb);
				if (highestY != float.NegativeInfinity) {
					spawn.Y = highestY; break;
				}
				bb.Min.Y -= 1; bb.Max.Y -= 1;
			}
			
			LocationUpdate update = LocationUpdate.MakePosAndOri(spawn, 0, 0, false);
			game.LocalPlayer.SetLocation(update, false);
			game.LocalPlayer.Spawn = spawn;
			game.CurrentCameraPos = game.Camera.GetCameraPos(0);
		}
	}
}