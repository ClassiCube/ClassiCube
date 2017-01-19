// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Blocks {
	
	/// <summary> Stores default properties for blocks in Minecraft Classic. </summary>
	public static class DefaultSet {
		
		public static float Height(byte b) {
			if (b == Block.Slab) return 8/16f;
			if (b == Block.CobblestoneSlab) return 8/16f;
			if (b == Block.Snow) return 2/16f;
			return 1;
		}
		
		public static bool FullBright(byte b) {
			return b == Block.Lava || b == Block.StillLava
				|| b == Block.Magma || b == Block.Fire;
		}
		
		public static float FogDensity(byte b) {
			if (b == Block.Water || b == Block.StillWater)
				return 0.1f;
			if (b == Block.Lava || b == Block.StillLava)
				return 2f;
			return 0;
		}
		
		public static FastColour FogColour(byte b) {
			if (b == Block.Water || b == Block.StillWater)
				return new FastColour(5, 5, 51);
			if (b == Block.Lava || b == Block.StillLava)
				return new FastColour(153, 25, 0);
			return default(FastColour);
		}
		
		public static CollideType Collide(byte b) {
			if (b >= Block.Water && b <= Block.StillLava)
				return CollideType.SwimThrough;
			if (b == Block.Snow || b == Block.Air || Draw(b) == DrawType.Sprite)
				return CollideType.WalkThrough;
			return CollideType.Solid;
		}
		
		public static bool BlocksLight(byte b) {
			return !(b == Block.Glass || b == Block.Leaves 
			         || b == Block.Air || Draw(b) == DrawType.Sprite);
		}

		public static SoundType StepSound(byte b) {
			if (b == Block.Glass) return SoundType.Stone;
			if (b == Block.Rope) return SoundType.Cloth;			
			if (Draw(b) == DrawType.Sprite) return SoundType.None;
			return DigSound(b);
		}
		
		
		public static byte Draw(byte b) {
			if (b == Block.Air || b == Block.Invalid) return DrawType.Gas;
			if (b == Block.Leaves) return DrawType.TransparentThick;

			if (b == Block.Ice || b == Block.Water || b == Block.StillWater) 
				return DrawType.Translucent;
			if (b == Block.Glass || b == Block.Leaves || b == Block.Snow)
				return DrawType.Transparent;
			if (b == Block.Slab || b == Block.CobblestoneSlab)
				return DrawType.Transparent;
			
			if (b >= Block.Dandelion && b <= Block.RedMushroom)
				return DrawType.Sprite;
			if (b == Block.Sapling || b == Block.Rope || b == Block.Fire)
				return DrawType.Sprite;
			return DrawType.Opaque;
		}		

		public static SoundType DigSound(byte b) {
			if (b >= Block.Red && b <= Block.White) 
				return SoundType.Cloth;
			if (b >= Block.LightPink && b <= Block.Turquoise) 
				return SoundType.Cloth;
			
			if (b == Block.Bookshelf || b == Block.Wood 
			   || b == Block.Log || b == Block.Crate || b == Block.Fire)
				return SoundType.Wood;
			
			if (b == Block.Rope) return SoundType.Cloth;
			if (b == Block.Sand) return SoundType.Sand;
			if (b == Block.Snow) return SoundType.Snow;
			if (b == Block.Glass) return SoundType.Glass;
			if (b == Block.Dirt || b == Block.Gravel)
				return SoundType.Gravel;
			
			if (b == Block.Grass || b == Block.Sapling || b == Block.TNT
			   || b == Block.Leaves || b == Block.Sponge)
				return SoundType.Grass;
			
			if (b >= Block.Dandelion && b <= Block.RedMushroom)
				return SoundType.Grass;
			if (b >= Block.Water && b <= Block.StillLava)
				return SoundType.None;
			if (b >= Block.Stone && b <= Block.StoneBrick)
				return SoundType.Stone;
			return SoundType.None;
		}
	}
}