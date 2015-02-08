using System;

namespace ClassicalSharp.Items {
	
	public class ShearsItem : ToolItem {
		
		public ShearsItem() : base( ItemId.Shears ) {
		}
		
		public override bool CanHarvest( BlockId block ) {
			return block == BlockId.Cobweb || block == BlockId.Leaves || block == BlockId.Wool;
		}
		
		public override float GetEffectiveness( BlockId block ) {
			if( block == BlockId.Cobweb || block == BlockId.Leaves ) return 15f;
			if( block == BlockId.Wool ) return 5f;
			return 1f;
		}
	}
}
