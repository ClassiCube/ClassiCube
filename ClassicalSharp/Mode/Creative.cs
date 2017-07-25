// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Screens;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Mode {
	
	public sealed class CreativeGameMode : IGameMode {
		
		Game game;
		
		public bool HandlesKeyDown(Key key) {
			if (key == game.Input.Keys[KeyBind.Inventory]) {
				game.Gui.SetNewScreen(new InventoryScreen(game));
				return true;
			}
			return false;
		}
		
		public void PickLeft(BlockID old) {
			Vector3I pos = game.SelectedPos.BlockPos;
			game.UpdateBlock(pos.X, pos.Y, pos.Z, Block.Air);
			game.UserEvents.RaiseBlockChanged(pos, old, Block.Air);
		}
		
		public void PickMiddle(BlockID old) {
			Inventory inv = game.Inventory;
			if (game.BlockInfo.Draw[old] == DrawType.Gas) return;
			if (!(inv.CanPlace[old] || inv.CanDelete[old])) return;
			if (!inv.CanChangeSelected()) return;
			
			// Is the currently selected block an empty slot
			if (inv.Hotbar[inv.SelectedIndex] == Block.Air) { 
				inv.Selected = old; return; 
			}
			
			// Try to replace same block or empty slots first.
			for (int i = 0; i < Inventory.BlocksPerRow; i++) {
				if (inv[i] != old && inv[i] != Block.Air) continue;
				
				inv[i] = old;
				inv.SelectedIndex = i; return;
			}
			
			// Finally, replace the currently selected block.
			inv.Selected = old;
		}
		
		public void PickRight(BlockID old, BlockID block) {
			Vector3I pos = game.SelectedPos.TranslatedPos;
			game.UpdateBlock(pos.X, pos.Y, pos.Z, block);
			game.UserEvents.RaiseBlockChanged(pos, old, block);
		}
		
		public bool PickEntity(byte id) { return false; }
		public Widget MakeHotbar() { return new HotbarWidget(game); }
		
		
		public void OnNewMapLoaded(Game game) {
		}

		public void Init(Game game) {
			this.game = game;
			Inventory inv = game.Inventory;
			inv[0] = Block.Stone;  inv[1] = Block.Cobblestone; inv[2] = Block.Brick;
			inv[3] = Block.Dirt;   inv[4] = Block.Wood;        inv[5] = Block.Log;
			inv[6] = Block.Leaves; inv[7] = Block.Grass;       inv[8] = Block.Slab;
		}
		
		
		public void Ready(Game game) { }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void Dispose() { }
		
		public void BeginFrame(double delta) { }
		public void EndFrame(double delta) { }
	}
}
