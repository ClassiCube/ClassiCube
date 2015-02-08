using System;

namespace ClassicalSharp.Items {
	
	public class PickaxeItem : ToolItem {
		
		public ToolGroup Group;
		public PickaxeItem( ItemId id, ToolGroup group ) : base( id ) {
			Group = group;
		}
		
		public override bool CanHarvest( BlockId block ) {
			return CanHarvest( block, Group );
		}
		
		bool CanHarvest( BlockId block, ToolGroup group ) {
			if( group == ToolGroup.Diamond && block == BlockId.Obsidian ) return true;
			if( ( group == ToolGroup.Diamond || group == ToolGroup.Iron ) &&
			   block == BlockId.BlockOfDiamond || block == BlockId.BlockOfGold ||
			   block == BlockId.DiamondOre || block == BlockId.RedstoneOre ||
			   block == BlockId.GoldOre ) return true;
			if( ( group == ToolGroup.Diamond || group == ToolGroup.Iron ||
			     group == ToolGroup.Stone ) &&
			   block == BlockId.BlockOfIron || block == BlockId.IronOre ||
			   block == BlockId.LapisLazuliBlock || block == BlockId.LapisLazuliOre ) return true;
			
			return block == BlockId.IronDoor || block == BlockId.MonsterSpawner ||
				block == BlockId.Dispenser || block == BlockId.Furnace || 
				block == BlockId.BurningFurnace || block == BlockId.CoalOre ||
				block == BlockId.BrickBlock || block == BlockId.Cobblestone ||
				block == BlockId.CobblestoneStairs || block == BlockId.MossStone ||
				block == BlockId.Slabs || block == BlockId.Stone ||
				block == BlockId.Sandstone || block == BlockId.Ice ||
				block == BlockId.StonePressurePlate || block == BlockId.Netherrack;
		}
		
		public override float GetEffectiveness( BlockId block ) {
			if( CanHarvest( block, ToolGroup.Diamond ) ) return GetMultiplier( Group );
			return 1f;
		}
	}
}
