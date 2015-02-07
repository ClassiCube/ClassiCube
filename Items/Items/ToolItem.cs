using System;

namespace ClassicalSharp.Items {
	
	public abstract class ToolItem : Item {
		
		public ToolItem( short id ) : base( id ) {
		}
		
		public static float GetMultiplier( ToolGroup group ) {
			if( group == ToolGroup.Wood ) return 2;
			if( group == ToolGroup.Stone ) return 4;
			if( group == ToolGroup.Iron ) return 6;
			if( group == ToolGroup.Diamond ) return 8;
			if( group == ToolGroup.Gold ) return 12;
			return 1;
		}
	}
	
	public enum ToolGroup {
		Wood,
		Stone,
		Iron,
		Gold,
		Diamond,
	}
}
