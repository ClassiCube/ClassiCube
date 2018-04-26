// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using BlockID = System.UInt16;

namespace ClassicalSharp {
	
	/// <summary> Manages the hotbar and inventory of blocks. </summary>
	public sealed class Inventory : IGameComponent {
		
		void IGameComponent.Init(Game game) {
			this.game = game;
			Reset(game);
		}
		
		public void Reset(Game game) { 
			SetDefaultMapping();
			CanChangeHeldBlock = true; 
			CanPick = true; 
		}

		void IGameComponent.Ready(Game game) { }
		void IGameComponent.OnNewMap(Game game) { }
		void IGameComponent.OnNewMapLoaded(Game game) { }
		void IDisposable.Dispose() { }
		
		int selectedI, offset;
		Game game;
		public bool CanChangeHeldBlock, CanPick;
		
		public const int BlocksPerRow = 9, Rows = 9;
		public BlockID[] Hotbar = new BlockID[BlocksPerRow * Rows];
		
		/// <summary> Gets or sets the block at the given index within the current row. </summary>
		public BlockID this[int index] {
			get { return Hotbar[offset + index]; }
			set { Hotbar[offset + index] = value; }
		}
		
		public bool CanChangeSelected() {
			if (!CanChangeHeldBlock) {
				game.Chat.Add("&e/client: &cThe server has forbidden you from changing your held block.");
				return false;
			}
			return true;
		}
		
		/// <summary> Gets or sets the index of the selected block within the current row. </summary>
		/// <remarks> Fails if the server has forbidden user from changing the held block. </remarks>
		public int SelectedIndex {
			get { return selectedI; }
			set {
				if (!CanChangeSelected()) return;
				CanPick = true;
				selectedI = value; game.Events.RaiseHeldBlockChanged();
			}
		}
		
		/// <summary> Gets or sets the currently selected row. </summary>
		/// <remarks> Fails if the server has forbidden user from changing the held block. </remarks>
		public int Offset {
			get { return offset; }
			set {
				if (!CanChangeSelected() || game.ClassicMode) return;
				offset = value; game.Events.RaiseHeldBlockChanged();
			}
		}
		
		/// <summary> Gets or sets the block currently selected by the player. </summary>
		/// <remarks> Fails if the server has forbidden user from changing the held block. </remarks>
		public BlockID Selected {
			get { return Hotbar[Offset + selectedI]; }
			set {
				if (!CanChangeSelected()) return;
				CanPick = true;
				
				// Change the selected index if this block already in hotbar
				for (int i = 0; i < BlocksPerRow; i++) {
					if (this[i] != value) continue;
					
					BlockID prevSelected = this[selectedI];
					this[selectedI] = this[i];
					this[i] = prevSelected;
					
					game.Events.RaiseHeldBlockChanged();
					return;
				}
				
				this[selectedI] = value;
				game.Events.RaiseHeldBlockChanged();
			}
		}
		
		public BlockID[] Map;
		public void SetDefaultMapping() {
			for (int i = 0; i < Map.Length; i++) {
				Map[i] = DefaultMapping(i);
			}
		}
		
		static BlockID[] classicTable = new BlockID[] {
			Block.Stone, Block.Cobblestone, Block.Brick, Block.Dirt, Block.Wood, Block.Log, Block.Leaves, Block.Glass, Block.Slab,
			Block.MossyRocks, Block.Sapling, Block.Dandelion, Block.Rose, Block.BrownMushroom, Block.RedMushroom, Block.Sand, Block.Gravel, Block.Sponge,
			Block.Red, Block.Orange, Block.Yellow, Block.Lime, Block.Green, Block.Teal, Block.Aqua, Block.Cyan, Block.Blue, 
			Block.Indigo, Block.Violet, Block.Magenta, Block.Pink, Block.Black, Block.Gray, Block.White, Block.CoalOre, Block.IronOre, 
			Block.GoldOre, Block.Iron, Block.Gold, Block.Bookshelf, Block.TNT, Block.Obsidian,
		};		
		static BlockID[] classicHacksTable = new BlockID[] {
			Block.Stone, Block.Grass, Block.Cobblestone, Block.Brick, Block.Dirt, Block.Wood, Block.Bedrock, Block.Water, Block.StillWater, Block.Lava,
			Block.StillLava, Block.Log, Block.Leaves, Block.Glass, Block.Slab, Block.MossyRocks, Block.Sapling, Block.Dandelion, Block.Rose, Block.BrownMushroom, 
			Block.RedMushroom, Block.Sand, Block.Gravel, Block.Sponge, Block.Red, Block.Orange, Block.Yellow, Block.Lime, Block.Green, Block.Teal, 
			Block.Aqua, Block.Cyan, Block.Blue, Block.Indigo, Block.Violet, Block.Magenta, Block.Pink, Block.Black, Block.Gray, Block.White,
			Block.CoalOre, Block.IronOre, Block.GoldOre, Block.DoubleSlab, Block.Iron, Block.Gold, Block.Bookshelf, Block.TNT, Block.Obsidian,			
		};
		
		BlockID DefaultMapping(int slot) {
			if (game.PureClassic) {
				if (slot < 9 * 4 + 6) return classicTable[slot];
			} else if (game.ClassicMode) {
				if (slot < 10 * 4 + 9) return classicHacksTable[slot];
			} else if (slot < Block.MaxCpeBlock) {
				return (BlockID)(slot + 1);
			}
			return Block.Air;
		}
		
		public void AddDefault(BlockID block) {
			if (block >= Block.CpeCount) {
				Map[block - 1] = block; return;
			}
			
			for (int slot = 0; slot < Block.MaxCpeBlock; slot++) {
				if (DefaultMapping(slot) != block) continue;
				Map[slot] = block;  
				return;
			}
		}
		
		public void Remove(BlockID block) {
			for (int i = 0; i < Map.Length; i++) {
				if (Map[i] == block) Map[i] = Block.Air;
			}
		}
	}
}
