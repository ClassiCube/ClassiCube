// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp {
	
	/// <summary> Contains the hotbar of blocks, as well as the permissions for placing and deleting all blocks. </summary>
	public sealed class Inventory : IGameComponent {
		
		public void Init(Game game) {
			this.game = game;
			MakeMap();
		}

		public void Ready(Game game) { }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void OnNewMapLoaded(Game game) { }
		public void Dispose() { }
		
		int selectedI, offset;
		Game game;
		public bool CanChangeHeldBlock = true;
		
		public const int BlocksPerRow = 9, Rows = 9;
		public BlockID[] Hotbar = new BlockID[BlocksPerRow * Rows];
		public InventoryPermissions CanPlace = new InventoryPermissions();
		public InventoryPermissions CanDelete = new InventoryPermissions();
		
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
		
		BlockID[] map = new BlockID[Block.Count];
		public BlockID MapBlock(int i) { return map[i]; }
		
		void MakeMap() {
			for (int i = 0; i < map.Length; i++)
				map[i] = (BlockID)i;
			if (!game.ClassicMode) return;
			
			// First row
			map[Block.Dirt] = Block.Cobblestone;
			map[Block.Cobblestone] = Block.Brick;
			map[Block.Wood] = Block.Dirt;
			map[Block.Sapling] = Block.Wood;
			map[Block.Sand] = Block.Log;
			map[Block.Gravel] = Block.Leaves;
			map[Block.GoldOre] = Block.Glass;
			map[Block.IronOre] = Block.Slab;
			map[Block.CoalOre] = Block.MossyRocks;
			// Second row
			map[Block.Log] = Block.Sapling;
			for (int i = 0; i < 4; i++)
				map[Block.Leaves + i] = (BlockID)(Block.Dandelion + i);
			map[Block.Orange] = Block.Sand;
			map[Block.Yellow] = Block.Gravel;
			map[Block.Lime] = Block.Sponge;
			// Third and fourth row
			for (int i = 0; i < 16; i++)
				map[Block.Green + i] = (BlockID)(Block.Red + i);
			map[Block.Gold] = Block.CoalOre;
			map[Block.Iron] = Block.IronOre;
			// Fifth row
			if (!game.PureClassic) map[Block.DoubleSlab] = Block.GoldOre;
			map[Block.Slab] = game.PureClassic ? Block.GoldOre : Block.DoubleSlab;
			map[Block.Brick] = Block.Iron;
			map[Block.TNT] = Block.Gold;
			map[Block.MossyRocks] = Block.TNT;
		}
	}
	
	public class InventoryPermissions {
		
		byte[] values = new byte[Block.Count];
		public bool this[int index] {
			get { return (values[index] & 1) != 0; }
			set {
				if (values[index] >= 0x80) return;
				values[index] &= 0xFE; // reset perm bit
				values[index] |= (byte)(value ? 1 : 0);
			}
		}
		
		public void SetNotOverridable(bool value, int index) {
			values[index] &= 0xFE; // reset perm bit
			values[index] |= (byte)(value ? 0x81 : 0x80); // set 'don't override' bit
		}
	}
}
