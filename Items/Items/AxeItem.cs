using System;

namespace ClassicalSharp.Items {
	
	public class AxeItem : ToolItem {
		
		public ToolGroup Group;
		public AxeItem( short id, ToolGroup group ) : base( id ) {
			Group = group;
		}
		
		public override bool CanHarvest( BlockId block ) {
			return block == BlockId.WoodenDoor || block == BlockId.Trapdoor ||
				block == BlockId.Chest || block == BlockId.CraftingTable ||
				block == BlockId.Fence || block == BlockId.Jukebox ||
				block == BlockId.Wood || block == BlockId.WoodenPlanks ||
				block == BlockId.WoodenStairs || block == BlockId.Bookshelf ||
				block == BlockId.JackOLantern || block == BlockId.Pumpkin ||
				block == BlockId.SignPost || block == BlockId.WallSign ||
				block == BlockId.NoteBlock || block == BlockId.WoodenPressurePlate;
		}
		
		public override float GetEffectiveness( BlockId block ) {
			if( CanHarvest( block ) ) return GetMultiplier( Group );
			return 1f;
		}
	}
}
