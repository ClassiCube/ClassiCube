// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

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
		
		int hotbarIndex = 0;
		Game game;
		public bool CanChangeHeldBlock = true;
		
		public byte[] Hotbar = new byte[9];
		public InventoryPermissions CanPlace = new InventoryPermissions();
		public InventoryPermissions CanDelete = new InventoryPermissions();
		
		/// <summary> Gets or sets the index of the held block.
		/// Fails if the server has forbidden user from changing the held block. </summary>
		public int HeldBlockIndex {
			get { return hotbarIndex; }
			set {
				if (!CanChangeHeldBlock) {
					game.Chat.Add("&e/client: &cThe server has forbidden you from changing your held block.");
					return;
				}
				hotbarIndex = value;
				game.Events.RaiseHeldBlockChanged();
			}
		}
		
		/// <summary> Gets or sets the block currently held by the player.
		/// Fails if the server has forbidden user from changing the held block. </summary>
		public byte HeldBlock {
			get { return Hotbar[hotbarIndex]; }
			set {
				if (!CanChangeHeldBlock) {
					game.Chat.Add("&e/client: &cThe server has forbidden you from changing your held block.");
					return;
				}
				for (int i = 0; i < Hotbar.Length; i++) {
					if (Hotbar[i] == value) {
						byte held = Hotbar[hotbarIndex];
						Hotbar[hotbarIndex] = Hotbar[i];
						Hotbar[i] = held;
						
						game.Events.RaiseHeldBlockChanged();
						return;
					}
				}
				Hotbar[hotbarIndex] = value;
				game.Events.RaiseHeldBlockChanged();
			}
		}
		
		byte[] map = new byte[256];
		public byte MapBlock(int i) { return map[i]; }
		
		void MakeMap() {
			for (int i = 0; i < map.Length; i++)
				map[i] = (byte)i;
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
				map[Block.Leaves + i] = (byte)(Block.Dandelion + i);
			map[Block.Orange] = Block.Sand;
			map[Block.Yellow] = Block.Gravel;
			map[Block.Lime] = Block.Sponge;
			// Third and fourth row
			for (int i = 0; i < 16; i++)
				map[Block.Green + i] = (byte)(Block.Red + i);
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
		
		byte[] values = new byte[256];
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
