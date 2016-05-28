// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp {
	
	/// <summary> Contains the hotbar of blocks, as well as the permissions for placing and deleting all blocks. </summary>
	public sealed class Inventory : IGameComponent {
		
		public void Init( Game game ) {
			this.game = game;
			// We can't use enum array initaliser because this causes problems when building with mono
			// and running on default .NET (https://bugzilla.xamarin.com/show_bug.cgi?id=572)
			Hotbar = new Block[9];
			SetDefaultHotbar();
			MakeMap();
		}
		
		void SetDefaultHotbar() {
			Hotbar[0] = Block.Stone; Hotbar[1] = Block.Cobblestone;
			Hotbar[2] = Block.Brick; Hotbar[3] = Block.Dirt;
			Hotbar[4] = Block.Wood; Hotbar[5] = Block.Log;
			Hotbar[6] = Block.Leaves; Hotbar[7] = Block.Grass;
			Hotbar[8] = Block.Slab;
		}

		public void Ready( Game game ) { }
		public void Reset( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		public void Dispose() { }
		
		int hotbarIndex = 0;
		public bool CanChangeHeldBlock = true;
		public Block[] Hotbar;
		Game game;
		
		public InventoryPermissions CanPlace = new InventoryPermissions();
		public InventoryPermissions CanDelete = new InventoryPermissions();
		
		/// <summary> Gets or sets the index of the held block.
		/// Fails if the server has forbidden up from changing the held block. </summary>
		public int HeldBlockIndex {
			get { return hotbarIndex; }
			set {
				if( !CanChangeHeldBlock ) {
					game.Chat.Add( "&e/client: &cThe server has forbidden you from changing your held block." );
					return;
				}
				hotbarIndex = value;
				game.Events.RaiseHeldBlockChanged();
			}
		}
		
		/// <summary> Gets or sets the block currently held by the player.
		/// Fails if the server has forbidden up from changing the held block. </summary>
		public Block HeldBlock {
			get { return Hotbar[hotbarIndex]; }
			set {
				if( !CanChangeHeldBlock ) {
					game.Chat.Add( "&e/client: &cThe server has forbidden you from changing your held block." );
					return;
				}
				for( int i = 0; i < Hotbar.Length; i++ ) {
					if( Hotbar[i] == value ) {
						Block held = Hotbar[hotbarIndex];
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
		
		Block[] map = new Block[256];
		public Block MapBlock( int i ) { return map[i]; }
		
		void MakeMap() {
			for( int i = 0; i < map.Length; i++ )
				map[i] = (Block)i;
			if( !game.ClassicMode ) return;
			
			// First row
			map[(byte)Block.Dirt] = Block.Cobblestone;
			map[(byte)Block.Cobblestone] = Block.Brick;
			map[(byte)Block.Wood] = Block.Dirt;
			map[(byte)Block.Sapling] = Block.Wood;
			map[(byte)Block.Sand] = Block.Log;
			map[(byte)Block.Gravel] = Block.Leaves;
			map[(byte)Block.GoldOre] = Block.Glass;
			map[(byte)Block.IronOre] = Block.Slab;
			map[(byte)Block.CoalOre] = Block.MossyRocks;
			// Second row
			map[(byte)Block.Log] = Block.Sapling;
			for( int i = 0; i < 4; i++ )
				map[(byte)Block.Leaves + i] = (Block)((byte)Block.Dandelion + i);
			map[(byte)Block.Orange] = Block.Sand;
			map[(byte)Block.Yellow] = Block.Gravel;
			map[(byte)Block.Lime] = Block.Sponge;
			// Third and fourth row
			for( int i = 0; i < 16; i++ )
				map[(byte)Block.Green + i] = (Block)((byte)Block.Red + i);
			map[(byte)Block.Gold] = Block.CoalOre;
			map[(byte)Block.Iron] = Block.IronOre;
			// Fifth row
			if( !game.PureClassic ) map[(byte)Block.DoubleSlab] = Block.GoldOre;
			map[(byte)Block.Slab] = game.PureClassic ? Block.GoldOre : Block.DoubleSlab;
			map[(byte)Block.Brick] = Block.Iron;
			map[(byte)Block.TNT] = Block.Gold;
			map[(byte)Block.MossyRocks] = Block.TNT;
		}
	}
	
	public class InventoryPermissions {
		
		byte[] values = new byte[256];
		public bool this[int index] {
			get { return (values[index] & 1) != 0; }
			set {
				if( values[index] >= 0x80 ) return;
				values[index] &= 0xFE; // reset perm bit
				values[index] |= (byte)(value ? 1 : 0);
			}
		}
		
		public void SetNotOverridable( bool value, int index ) {
			values[index] &= 0xFE; // reset perm bit
			values[index] |= (byte)(value ? 0x81 : 0x80); // set 'don't override' bit
		}
	}
}
