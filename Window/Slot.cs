using System;
using ClassicalSharp.Items;

namespace ClassicalSharp.Window {
	
	public class Slot {
		
		public short Id;
		
		public byte Count;
		
		public short Damage;
		
		public bool IsBlock {
			get { return Id >= 0 && Id <= 255; }
		}
		
		public bool IsItem {
			get { return Id > 255; }
		}
		
		public bool IsEmpty {
			get { return Id < 0; }
		}
		
		public static Slot CreateEmpty() {
			return new Slot() { Id = -1 };
		}
		
		public override string ToString() {
			if( IsEmpty ) return "(empty slot)";
			return "(" + Id + "," + Damage + ":" + Count + ")";
		}
		
		public bool CanHarvest( BlockId block, ItemInfo info ) {
			if( block == BlockId.Bed || block == BlockId.Bookshelf ||
			   block == BlockId.StoneButton || block == BlockId.Cactus ||
			   block == BlockId.CakeBlock || block == BlockId.Chest ||
			   block == BlockId.ClayBlock || block == BlockId.CraftingTable ||
			   block == BlockId.DeadShrubs || block == BlockId.Dirt ||
			   block == BlockId.Farmland || block == BlockId.Fence ||
			   block == BlockId.Dandelion || block == BlockId.Rose ||
			   block == BlockId.Glass || block == BlockId.GlowstoneBlock ||
			   block == BlockId.TallGrass || block == BlockId.Grass ||
			   block == BlockId.Gravel || block == BlockId.JackOLantern ||
			   block == BlockId.Jukebox || block == BlockId.Ladders ||
			   block == BlockId.Leaves || block == BlockId.Lever ||
			   block == BlockId.RedMushroom || block == BlockId.BrownMushroom ||
			   block == BlockId.NoteBlock || block == BlockId.Piston ||
			   block == BlockId.Pumpkin || block == BlockId.Rails || 
			   block == BlockId.PoweredRail || block == BlockId.DetectorRail ||
			   block == BlockId.Sand || block == BlockId.Sapling ||
			   block == BlockId.SignPost || block == BlockId.WallSign ||
			   block == BlockId.SoulSand || block == BlockId.Sponge ||
			   block == BlockId.SugarCane || block == BlockId.TNT ||
			   block == BlockId.Torch || block == BlockId.RedstoneRepeaterOff ||
			   block == BlockId.RedstoneRepeaterOn || block == BlockId.RedstoneWire ||
			   block == BlockId.RedstoneTorchOff || block == BlockId.RedstoneTorchOn ||
			   block == BlockId.Seeds || block == BlockId.Wood || 
			   block == BlockId.WoodenPlanks || block == BlockId.WoodenDoor || 
			   block == BlockId.WoodenPressurePlate || block == BlockId.Trapdoor || 
			   block == BlockId.WoodenStairs ) 
				return true;
			
			if( IsEmpty ) return false;
			Item item = info.GetItem( Id );
			return item == null ? false : item.CanHarvest( block );
		}
		
		public float GetEffectiveness( BlockId block, ItemInfo info ) {
			if( IsEmpty ) return 1f;
			Item item = info.GetItem( Id );
			return item == null ? 1f : item.GetEffectiveness( block );
		}
	}
}
