using System;

namespace ClassicalSharp.Items {
	
	public class SwordItem : Item {
		
		public SwordItem( short id ) : base( id ) {
		}
		
		public override bool CanHarvest( BlockId block ) {
			return block == BlockId.Cobweb || block == BlockId.JackOLantern || block == BlockId.Pumpkin ||
				block == BlockId.Leaves || block == BlockId.Leaves;
		}
		
		public override float GetEffectiveness( BlockId block ) {
			if( block == BlockId.Cobweb ) return 15f;
			if( block == BlockId.JackOLantern || block == BlockId.Pumpkin ||
			   block == BlockId.Leaves || block == BlockId.Leaves ) return 1.5f;
			return 1f;
		}
	}
}
