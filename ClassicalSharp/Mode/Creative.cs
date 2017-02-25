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
			game.UpdateBlock(pos.X, pos.Y, pos.Z, 0);
			game.UserEvents.RaiseBlockChanged(pos, old, 0);
		}
		
		public void PickMiddle(BlockID old) {
			Inventory inv = game.Inventory;			
			if (game.BlockInfo.Draw[old] != DrawType.Gas && (inv.CanPlace[old] || inv.CanDelete[old])) {
				for (int i = 0; i < inv.Hotbar.Length; i++) {
					if (inv.Hotbar[i] == old) {
						inv.HeldBlockIndex = i; return;
					}
				}
				inv.HeldBlock = old;
			}
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
			game.Inventory.Hotbar = new BlockID[] { Block.Stone,
				Block.Cobblestone, Block.Brick, Block.Dirt, Block.Wood,
				Block.Log, Block.Leaves, Block.Grass, Block.Slab };
		}
		
		
		public void Ready(Game game) { }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void Dispose() { }
	}
}
