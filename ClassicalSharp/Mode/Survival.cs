// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Entities.Mobs;
using ClassicalSharp.Gui.Widgets;
using OpenTK;
using OpenTK.Input;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Mode {
	
	public sealed class SurvivalGameMode : IGameMode {
		
		Game game;
		int score = 0;
		internal byte[] invCount = new byte[Inventory.BlocksPerRow * Inventory.Rows];
		Random rnd = new Random();
		
		public SurvivalGameMode() { invCount[8] = 10; } // tnt
		
		public bool HandlesKeyDown(Key key) { return false; }

		public void PickLeft(BlockID old) {
			Vector3I pos = game.SelectedPos.BlockPos;
			game.UpdateBlock(pos.X, pos.Y, pos.Z, Block.Air);
			game.UserEvents.RaiseBlockChanged(pos, old, Block.Air);
			HandleDelete(old);
		}
		
		public void PickMiddle(BlockID old) {
		}
		
		public void PickRight(BlockID old, BlockID block) {
			int index = game.Inventory.SelectedIndex, offset = game.Inventory.Offset;
			if (invCount[offset + index] == 0) return;
			
			Vector3I pos = game.SelectedPos.TranslatedPos;
			game.UpdateBlock(pos.X, pos.Y, pos.Z, block);
			game.UserEvents.RaiseBlockChanged(pos, old, block);
			
			invCount[offset + index]--;
			if (invCount[offset + index] != 0) return;
			
			// bypass HeldBlock's normal behaviour
			game.Inventory[index] = Block.Air;
			game.Events.RaiseHeldBlockChanged();
		}
		
		public bool PickEntity(byte id) {
			Entity entity = game.Entities[id];
			Entity player = game.Entities[EntityList.SelfID];
			
			Vector3 delta = player.Position - entity.Position;
			delta.Y = 0.0f;
			delta = Vector3.Normalize(delta);
			delta.Y = -0.5f;
			
			entity.Velocity -= delta;
			game.Chat.Add("PICKED ON: " + id + "," + entity.ModelName);
			
			entity.Health -= 2;
			if (entity.Health < 0) {
				game.Entities.RemoveEntity(id);
				score += GetScore(entity.ModelName);
				UpdateScore();
			}
			return true;
		}
		
		public Widget MakeHotbar() { return new SurvivalHotbarWidget(game); }
		
		
		void HandleDelete(BlockID old) {
			if (old == Block.Log) {
				AddToHotbar(Block.Wood, rnd.Next(3, 6));
			} else if (old == Block.CoalOre) {
				AddToHotbar(Block.Slab, rnd.Next(1, 4));
			} else if (old == Block.IronOre) {
				AddToHotbar(Block.Iron, 1);
			} else if (old == Block.GoldOre) {
				AddToHotbar(Block.Gold, 1);
			} else if (old == Block.Grass) {
				AddToHotbar(Block.Dirt, 1);
			} else if (old == Block.Stone) {
				AddToHotbar(Block.Cobblestone, 1);
			} else if (old == Block.Leaves) {
				if (rnd.Next(1, 16) == 1) { // TODO: is this chance accurate?
					AddToHotbar(Block.Sapling, 1);
				}
			} else {
				AddToHotbar(old, 1);
			}
		}
		
		void AddToHotbar(BlockID block, int count) {
			int index = -1, offset = game.Inventory.Offset;
			
			// Try searching for same block, then try invalid block
			for (int i = 0; i < Inventory.BlocksPerRow; i++) {
				if (game.Inventory[i] == block) index = i;
			}
			if (index == -1) {
				for (int i = Inventory.BlocksPerRow - 1; i >= 0; i--) {
					if (game.Inventory[i] == Block.Air) index = i;
				}
			}
			if (index == -1) return; // no free slots
			
			for (int j = 0; j < count; j++) {
				if (invCount[offset + index] >= 99) return; // no more count
				game.Inventory[index] = block;
				invCount[offset + index]++; // TODO: do we need to raise an event if changing held block still?
				// TODO: we need to spawn block models instead
			}
		}

		
		public void OnNewMapLoaded(Game game) {
			UpdateScore();
			string[] models = { "sheep", "pig", "skeleton", "zombie", "creeper", "spider" };
			for (int i = 0; i < 254; i++) {
				MobEntity fail = new MobEntity(game, models[rnd.Next(models.Length)]);
				float x = rnd.Next(0, game.World.Width) + 0.5f;
				float z = rnd.Next(0, game.World.Length) + 0.5f;
				
				Vector3 pos = Respawn.FindSpawnPosition(game, x, z, fail.Size);
				fail.SetLocation(LocationUpdate.MakePos(pos, false), false);
				game.Entities[i] = fail;
			}
		}
		
		public void Init(Game game) {
			this.game = game;
			BlockID[] hotbar = game.Inventory.Hotbar;
			for (int i = 0; i < hotbar.Length; i++)
				hotbar[i] = Block.Air;
			hotbar[Inventory.BlocksPerRow - 1] = Block.TNT;
			game.Server.AppName += " (survival)";
		}
		
		
		int GetScore(string model) {
			if (model == "sheep" || model == "pig") return 10;
			if (model == "zombie") return 80;
			if (model == "spider") return 105;
			if (model == "skeleton") return 120;
			if (model == "creeper") return 200;
			return 5;
		}
		
		void UpdateScore() {
			game.Chat.Add("&fScore: &e" + score, MessageType.Status1);
		}
		
		
		public void Ready(Game game) { }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void Dispose() { }
	}
}
