using System;

namespace ClassicalSharp.Items {
	
	public class ShovelItem : ToolItem {
		
		public ToolGroup Group;
		public ShovelItem( short id, ToolGroup group ) : base( id ) {
			Group = group;
		}
		
		public override bool CanHarvest( BlockId block ) {
			return block == BlockId.ClayBlock || block == BlockId.Farmland ||
				block == BlockId.Grass || block == BlockId.Gravel ||
				block == BlockId.Dirt || block == BlockId.Sand ||
				block == BlockId.SoulSand || block == BlockId.Snow ||
				block == BlockId.SnowBlock;
		}
		
		public override float GetEffectiveness( BlockId block ) {
			if( CanHarvest( block ) ) return GetMultiplier( Group );
			return 1f;
		}
	}
}
