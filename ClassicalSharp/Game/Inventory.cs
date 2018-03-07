// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp {
	
	/// <summary> Manages the hotbar and inventory of blocks. </summary>
	public sealed class Inventory : IGameComponent {
		
		public void Init(Game game) {
			this.game = game;
			Reset(game);
		}
		
		public void Reset(Game game) { 
			SetDefaultMapping(); 
			CanChangeHeldBlock = true; 
			CanPick = true; 
		}

		public void Ready(Game game) { }
		public void OnNewMap(Game game) { }
		public void OnNewMapLoaded(Game game) { }
		public void Dispose() { }
		
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
		
		public BlockID[] Map = new BlockID[Block.Count];
		public void SetDefaultMapping() {
			for (int i = 0; i < Map.Length; i++) Map[i] = (BlockID)i;
			for (int i = 0; i < Map.Length; i++) {
				BlockID mapping = DefaultMapping(i);
				if (game.PureClassic && IsHackBlock(mapping)) mapping = Block.Air;
				if (mapping != i) Map[i] = mapping;
			}
		}
		
		BlockID DefaultMapping(int i) {
#if USE16_BIT
			if ((i >= Block.CpeCount) || i == Block.Air) return Block.Air;
#else
			if (i >= Block.CpeCount || i == Block.Air) return Block.Air;
#endif
			if (!game.ClassicMode) return (BlockID)i;
			
			if (i >= 25 && i <= 40) {
				return (BlockID)(Block.Red + (i - 25));
			}
			if (i >= 18 && i <= 21) {
				return (BlockID)(Block.Dandelion + (i - 18));
			}
			
			switch (i) {
				// First row
				case 3: return Block.Cobblestone;
				case 4: return Block.Brick;
				case 5: return Block.Dirt;
				case 6: return Block.Wood;
				
				// Second row
				case 12: return Block.Log;
				case 13: return Block.Leaves;
				case 14: return Block.Glass;
				case 15: return Block.Slab;
				case 16: return Block.MossyRocks;
				case 17: return Block.Sapling;
					
				// Third row
				case 22: return Block.Sand;
				case 23: return Block.Gravel;
				case 24: return Block.Sponge;
					
				// Fifth row
				case 41: return Block.CoalOre;
				case 42: return Block.IronOre;
				case 43: return Block.GoldOre;
				case 44: return Block.DoubleSlab;
				case 45: return Block.Iron;
				case 46: return Block.Gold;
				case 47: return Block.Bookshelf;
				case 48: return Block.TNT;
			}
			return (BlockID)i;
		}
		
		static bool IsHackBlock(BlockID b) {
			return b == Block.DoubleSlab || b == Block.Bedrock ||
				b == Block.Grass || BlockInfo.IsLiquid[b];
		}
		
		public void AddDefault(BlockID block) {
			if (block >= Block.CpeCount || DefaultMapping(block) == block) {
				Map[block] = block;
				return;
			}
			
			for (int i = 0; i < Block.CpeCount; i++) {
				if (DefaultMapping(i) != block) continue;
				Map[i] = block; return;
			}
		}
		
		public void Reset(BlockID block) {
			for (int i = 0; i < Map.Length; i++) {
				if (Map[i] == block) Map[i] = DefaultMapping(i);
			}
		}
		
		public void Remove(BlockID block) {
			for (int i = 0; i < Map.Length; i++) {
				if (Map[i] == block) Map[i] = Block.Air;
			}
		}
		
		public void Insert(int i, BlockID block) {
			if (Map[i] == block) return;		
			// Need to push the old block to a different slot if different block			
			if (Map[i] != Block.Air) PushToFreeSlots(i);
			
			Map[i] = block;
		}
		
		void PushToFreeSlots(int i) {
			BlockID block = Map[i];
			// The slot was already pushed out in the past
			// TODO: find a better way of fixing this
			for (int j = 1; j < Map.Length; j++) {
				if (j != i && Map[j] == block) return;
			}
			
			for (int j = block; j < Map.Length; j++) {
				if (Map[j] == Block.Air) { Map[j] = block; return; }
			}
			for (int j = 1; j < block; j++) {
				if (Map[j] == Block.Air) { Map[j] = block; return; }
			}
		}
	}
}
