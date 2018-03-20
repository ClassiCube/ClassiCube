// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using BlockRaw = System.Byte;

namespace ClassicalSharp {
	
	/// <summary> Enumeration of all blocks in Minecraft Classic, including CPE ones. </summary>
	public static class Block {

#pragma warning	disable 1591
		public const BlockRaw Air = 0;
		public const BlockRaw Stone = 1;
		public const BlockRaw Grass = 2;
		public const BlockRaw Dirt = 3;
		public const BlockRaw Cobblestone = 4;
		public const BlockRaw Wood = 5;
		public const BlockRaw Sapling = 6;
		public const BlockRaw Bedrock = 7;
		public const BlockRaw Water = 8;
		public const BlockRaw StillWater = 9;
		public const BlockRaw Lava = 10;
		public const BlockRaw StillLava = 11;
		public const BlockRaw Sand = 12;
		public const BlockRaw Gravel = 13;
		public const BlockRaw GoldOre = 14;
		public const BlockRaw IronOre = 15;
		public const BlockRaw CoalOre = 16;
		public const BlockRaw Log = 17;
		public const BlockRaw Leaves = 18;
		public const BlockRaw Sponge = 19;
		public const BlockRaw Glass = 20;
		public const BlockRaw Red = 21;
		public const BlockRaw Orange = 22;
		public const BlockRaw Yellow = 23;
		public const BlockRaw Lime = 24;
		public const BlockRaw Green = 25;
		public const BlockRaw Teal = 26;
		public const BlockRaw Aqua = 27;
		public const BlockRaw Cyan = 28;
		public const BlockRaw Blue = 29;
		public const BlockRaw Indigo = 30;
		public const BlockRaw Violet = 31;
		public const BlockRaw Magenta = 32;
		public const BlockRaw Pink = 33;
		public const BlockRaw Black = 34;
		public const BlockRaw Gray = 35;
		public const BlockRaw White = 36;
		public const BlockRaw Dandelion = 37;
		public const BlockRaw Rose = 38;
		public const BlockRaw BrownMushroom = 39;
		public const BlockRaw RedMushroom = 40;
		public const BlockRaw Gold = 41;
		public const BlockRaw Iron = 42;
		public const BlockRaw DoubleSlab = 43;
		public const BlockRaw Slab = 44;
		public const BlockRaw Brick = 45;
		public const BlockRaw TNT = 46;
		public const BlockRaw Bookshelf = 47;
		public const BlockRaw MossyRocks = 48;
		public const BlockRaw Obsidian = 49;
		
		public const BlockRaw CobblestoneSlab = 50;
		public const BlockRaw Rope = 51;
		public const BlockRaw Sandstone = 52;
		public const BlockRaw Snow = 53;
		public const BlockRaw Fire = 54;
		public const BlockRaw LightPink = 55;
		public const BlockRaw ForestGreen = 56;
		public const BlockRaw Brown = 57;
		public const BlockRaw DeepBlue = 58;
		public const BlockRaw Turquoise = 59;
		public const BlockRaw Ice = 60;
		public const BlockRaw CeramicTile = 61;
		public const BlockRaw Magma = 62;
		public const BlockRaw Pillar = 63;
		public const BlockRaw Crate = 64;
		public const BlockRaw StoneBrick = 65;
#pragma warning restore 1591
		
		public const string RawNames = "Air Stone Grass Dirt Cobblestone Wood Sapling Bedrock Water StillWater Lava" +
			" StillLava Sand Gravel GoldOre IronOre CoalOre Log Leaves Sponge Glass Red Orange Yellow Lime Green" +
			" Teal Aqua Cyan Blue Indigo Violet Magenta Pink Black Gray White Dandelion Rose BrownMushroom RedMushroom" +
			" Gold Iron DoubleSlab Slab Brick TNT Bookshelf MossyRocks Obsidian CobblestoneSlab Rope Sandstone" +
			" Snow Fire LightPink ForestGreen Brown DeepBlue Turquoise Ice CeramicTile Magma Pillar Crate StoneBrick";		
		
		/// <summary> Max block ID used in original classic. </summary>
		public const BlockRaw MaxOriginalBlock = Block.Obsidian;
		
		/// <summary> Number of blocks in original classic. </summary>
		public const int OriginalCount = MaxOriginalBlock + 1;
		
		/// <summary> Max block ID used in original classic plus CPE blocks. </summary>
		public const BlockRaw MaxCpeBlock = Block.StoneBrick;

		/// <summary> Number of blocks in original classic plus CPE blocks. </summary>		
		public const int CpeCount = MaxCpeBlock + 1;
		
		/// <summary> Number of blocks in original classic plus CPE blocks plus block definitions. </summary>
		public const int DefaultCount = 256;
	}
}