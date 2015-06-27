using System;

namespace ClassicalSharp {
	
	public class Inventory {
		
		public Game Window;
		
		public Inventory( Game window ) {
			Window = window;
		}
		
		int hotbarIndex = 0;
		public bool CanChangeHeldBlock = true;
		public Block[] BlocksHotbar = new Block[] { Block.Stone, Block.Cobblestone,
			Block.Brick, Block.Dirt, Block.WoodenPlanks, Block.Wood, Block.Leaves, Block.Glass, Block.Slab };
		
		public int HeldBlockIndex {
			get { return hotbarIndex; }
			set {
				if( !CanChangeHeldBlock ) {
					Window.AddChat( "&e/client: &cThe server has forbidden you from changing your held block." );
					return;
				}
				hotbarIndex = value;
				Window.RaiseHeldBlockChanged();
			}
		}
		
		public Block HeldBlock {
			get { return BlocksHotbar[hotbarIndex]; }
			set {
				if( !CanChangeHeldBlock ) {
					Window.AddChat( "&e/client: &cThe server has forbidden you from changing your held block." );
					return;
				}
				BlocksHotbar[hotbarIndex] = value;
				Window.RaiseHeldBlockChanged();
			}
		}
		
		public bool CanReplace( byte block ) {
			return block == 0 || ( Window.BlockInfo.IsLiquid( block ) && !CanPlace[block] && !CanDelete[block] );
		}
		
		public bool[] CanPlace = new bool[256];
		public bool[] CanDelete = new bool[256];
		
		internal void ScrollHeldIndex( int diff ) {
			int diffIndex = diff % BlocksHotbar.Length;
			int newIndex = HeldBlockIndex + diffIndex;
			if( newIndex < 0 ) newIndex += BlocksHotbar.Length;
			if( newIndex >= BlocksHotbar.Length ) newIndex -= BlocksHotbar.Length;
			HeldBlockIndex = newIndex;
		}
	}
}