// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Entities.Mobs;
using ClassicalSharp.Gui;
using ClassicalSharp.Gui.Screens;
using ClassicalSharp.Gui.Widgets;
using OpenTK;
using OpenTK.Input;
using BlockID = System.UInt16;

namespace ClassicalSharp.Mode {
	
	public sealed class SurvivalGameMode : IGameMode {
		
		Game game;
		int score = 0;
		internal byte[] invCount = new byte[Inventory.BlocksPerRow * Inventory.Rows];
		Random rnd = new Random();
		
		public SurvivalGameMode() { invCount[8] = 10; } // tnt
		
		public bool HandlesKeyDown(Key key) { return false; }		
		
		public bool PickingLeft() {
			// always play delete animations, even if we aren't picking a block.
			game.HeldBlockRenderer.ClickAnim(true);
			byte id = game.Entities.GetClosetPlayer(game.LocalPlayer);
			return id != EntityList.SelfID && PickEntity(id);
		}
		
		public bool PickingRight() {
			if (game.Inventory.Selected == Block.RedMushroom) {
				DepleteInventoryHeld();
				game.LocalPlayer.Health -= 5;
				CheckPlayerDied();
				return true;
			} else if (game.Inventory.Selected == Block.BrownMushroom) {
				DepleteInventoryHeld();
				game.LocalPlayer.Health += 5;
				if (game.LocalPlayer.Health > 20) game.LocalPlayer.Health = 20;
				return true;
			}
			return false; 
		}

		public void PickLeft(BlockID old) {
			Vector3I pos = game.SelectedPos.BlockPos;
			game.UpdateBlock(pos.X, pos.Y, pos.Z, Block.Air);
			game.UserEvents.RaiseBlockChanged(pos, old, Block.Air);
			HandleDelete(old);
		}
		
		public void PickMiddle(BlockID old) { }
		
		public void PickRight(BlockID old, BlockID block) {
			int index = game.Inventory.SelectedIndex, offset = game.Inventory.Offset;
			if (invCount[offset + index] == 0) return;
			
			Vector3I pos = game.SelectedPos.TranslatedPos;
			game.UpdateBlock(pos.X, pos.Y, pos.Z, block);
			game.UserEvents.RaiseBlockChanged(pos, old, block);
			DepleteInventoryHeld();
		}
		
		void DepleteInventoryHeld() {
			int index = game.Inventory.SelectedIndex, offset = game.Inventory.Offset;
			invCount[offset + index]--;
			if (invCount[offset + index] != 0) return;
			
			// bypass HeldBlock's normal behaviour
			game.Inventory[index] = Block.Air;
			game.Events.RaiseHeldBlockChanged();
		}
		
		bool PickEntity(byte id) {
			Entity entity = game.Entities.List[id];
			LocalPlayer p = game.LocalPlayer;
			
			Vector3 delta = p.Position - entity.Position;
			if (delta.LengthSquared > p.ReachDistance * p.ReachDistance) return true;
			
			delta.Y = 0.0f;
			delta = Vector3.Normalize(delta) * 0.5f;
			delta.Y = -0.5f;
			
			entity.Velocity -= delta;
			game.Chat.Add("PICKED ON: " + id + "," + entity.ModelName);
			
			entity.Health -= 2;
			if (entity.Health < 0) {
				game.Entities.RemoveEntity(id);
				score += entity.Model.SurivalScore;
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

		
		void IGameComponent.OnNewMapLoaded(Game game) {
			UpdateScore();
			wasOnGround = true;
			showedDeathScreen = false;
			game.LocalPlayer.Health = 20;
			string[] models = { "sheep", "pig", "skeleton", "zombie", "creeper", "spider" };
			
			for (int i = 0; i < 254; i++) {
				MobEntity fail = new MobEntity(game, models[rnd.Next(models.Length)]);
				float x = rnd.Next(0, game.World.Width) + 0.5f;
				float z = rnd.Next(0, game.World.Length) + 0.5f;
				
				Vector3 pos = Respawn.FindSpawnPosition(game, x, z, fail.Size);
				fail.SetLocation(LocationUpdate.MakePos(pos, false), false);
				game.Entities.List[i] = fail;
			}
		}
		
		void IGameComponent.Init(Game game) {
			this.game = game;
			ResetInventory();
			game.Server.AppName += " (survival)";
		}
		
		void ResetInventory() {
			BlockID[] hotbar = game.Inventory.Hotbar;
			for (int i = 0; i < hotbar.Length; i++)
				hotbar[i] = Block.Air;
			hotbar[Inventory.BlocksPerRow - 1] = Block.TNT;
		}
		
		void UpdateScore() {
			game.Chat.Add("&fScore: &e" + score, MessageType.Status1);
		}
		
		
		void IGameComponent.Ready(Game game) { }
		void IGameComponent.Reset(Game game) { }
		void IGameComponent.OnNewMap(Game game) { }
		void IDisposable.Dispose() { }
		
		public void BeginFrame(double delta) { }
		
		bool wasOnGround = true;
		float fallY = -1000;
		bool showedDeathScreen = false;
		
		public void EndFrame(double delta) {
			LocalPlayer p = game.LocalPlayer;
			if (p.onGround) {
				if (wasOnGround) return;
				short damage = (short)((fallY - p.interp.next.Pos.Y) - 3);
				// TODO: shouldn't take damage when land in water or lava
				// TODO: is the damage formula correct
				if (damage > 0) p.Health -= damage;
				fallY = -1000;
			} else {
				fallY = Math.Max(fallY, p.interp.prev.Pos.Y);
			}
			
			wasOnGround = p.onGround;
			CheckPlayerDied();
		}
		
		void CheckPlayerDied() {
			LocalPlayer p = game.LocalPlayer;
			if (p.Health <= 0 && !showedDeathScreen) {
				showedDeathScreen = true;
				game.Gui.SetNewScreen(new DeathScreen(game));
				// TODO: Should only reset inventory when actually click on 'load' or 'generate'
				ResetInventory();
			}
		}
	}
}
