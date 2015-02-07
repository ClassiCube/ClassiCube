using System;

namespace ClassicalSharp.Items {
	
	public class SwordItem : ToolItem {
		
		public ToolGroup Group;
		public SwordItem( short id, ToolGroup group ) : base( id ) {
			Group = group;
		}
		
		public override bool CanHarvest( BlockId block ) {
			return block == BlockId.Cobweb || block == BlockId.JackOLantern || block == BlockId.Pumpkin ||
				block == BlockId.Leaves;
		}
		
		public override float GetEffectiveness( BlockId block ) {
			if( block == BlockId.Cobweb ) return 15f;
			if( block == BlockId.JackOLantern || block == BlockId.Pumpkin ||
			   block == BlockId.Leaves ) return 1.5f;
			return 1f;
		}
	}
}
