// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp {
	
	/// <summary> Contains the hotbar of blocks, as well as the permissions for placing and deleting all blocks. </summary>
	public sealed class Inventory {
		
		public Inventory( Game game ) {
			this.game = game;
			// We can't use enum array initaliser because this causes problems when building with mono
			// and running on default .NET (https://bugzilla.xamarin.com/show_bug.cgi?id=572)
			#if !__MonoCS__
			Hotbar = new Block[] { Block.Stone, Block.Cobblestone, Block.Brick, Block.Dirt,
				Block.WoodenPlanks, Block.Wood, Block.Leaves, Block.Grass, Block.Slab };
			#else
			Hotbar = new Block[9];
			Hotbar[0] = Block.Stone; Hotbar[1] = Block.Cobblestone;
			Hotbar[2] = Block.Brick; Hotbar[3] = Block.Dirt;
			Hotbar[4] = Block.WoodenPlanks; Hotbar[5] = Block.Wood;
			Hotbar[6] = Block.Leaves; Hotbar[7] = Block.Grass;
			Hotbar[8] = Block.Slab;
			#endif
		}
		
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
