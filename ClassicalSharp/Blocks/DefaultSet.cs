// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp.Blocks {
	
	/// <summary> Stores default properties for blocks in Minecraft Classic. </summary>
	public static class DefaultSet {
		
		public static float Height( byte block ) {
			if( block == Block.Slab ) return 8/16f;
			if( block == Block.CobblestoneSlab ) return 8/16f;
			if( block == Block.Snow ) return 2/16f;
			return 1;
		}
		
		public static bool FullBright( byte block ) {
			return block == Block.Lava || block == Block.StillLava
				|| block == Block.Magma || block == Block.Fire;
		}
		
		public static float FogDensity( byte block ) {
			if( block == Block.Water || block == Block.StillWater )
				return 0.1f;
			if( block == Block.Lava || block == Block.StillLava )
				return 2f;
			return 0;
		}
		
		public static FastColour FogColour( byte block ) {
			if( block == Block.Water || block == Block.StillWater )
				return new FastColour( 5, 5, 51 );
			if( block == Block.Lava || block == Block.StillLava )
				return new FastColour( 153, 25, 0 );
			return default(FastColour);
		}
		
		public static CollideType Collide( byte block ) {
			if( block >= Block.Water && block <= Block.StillLava )
				return CollideType.SwimThrough;
			if( block >= Block.Dandelion && block <= Block.RedMushroom )
				return CollideType.WalkThrough;
			
			if( block == Block.Sapling || block == Block.Rope )
				return CollideType.WalkThrough;
			if( block == Block.Fire || block == Block.Snow )
				return CollideType.WalkThrough;
			return CollideType.Solid;
		}
	}
}