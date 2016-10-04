// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp {
	
	/// <summary> Enumeration of all blocks in Minecraft Classic, including CPE ones. </summary>
	public static class Block {
		
		public const byte Air = 0;
		public const byte Stone = 1;
		public const byte Grass = 2;
		public const byte Dirt = 3;
		public const byte Cobblestone = 4;
		public const byte Wood = 5;
		public const byte Sapling = 6;
		public const byte Bedrock = 7;
		public const byte Water = 8;
		public const byte StillWater = 9;
		public const byte Lava = 10;
		public const byte StillLava = 11;
		public const byte Sand = 12;
		public const byte Gravel = 13;
		public const byte GoldOre = 14;
		public const byte IronOre = 15;
		public const byte CoalOre = 16;
		public const byte Log = 17;
		public const byte Leaves = 18;
		public const byte Sponge = 19;
		public const byte Glass = 20;
		public const byte Red = 21;
		public const byte Orange = 22;
		public const byte Yellow = 23;
		public const byte Lime = 24;
		public const byte Green = 25;
		public const byte Teal = 26;
		public const byte Aqua = 27;
		public const byte Cyan = 28;
		public const byte Blue = 29;
		public const byte Indigo = 30;
		public const byte Violet = 31;
		public const byte Magenta = 32;
		public const byte Pink = 33;
		public const byte Black = 34;
		public const byte Gray = 35;
		public const byte White = 36;
		public const byte Dandelion = 37;
		public const byte Rose = 38;
		public const byte BrownMushroom = 39;
		public const byte RedMushroom = 40;
		public const byte Gold = 41;
		public const byte Iron = 42;
		public const byte DoubleSlab = 43;
		public const byte Slab = 44;
		public const byte Brick = 45;
		public const byte TNT = 46;
		public const byte Bookshelf = 47;
		public const byte MossyRocks = 48;
		public const byte Obsidian = 49;
		
		public const byte CobblestoneSlab = 50;
		public const byte Rope = 51;
		public const byte Sandstone = 52;
		public const byte Snow = 53;
		public const byte Fire = 54;
		public const byte LightPink = 55;
		public const byte ForestGreen = 56;
		public const byte Brown = 57;
		public const byte DeepBlue = 58;
		public const byte Turquoise = 59;
		public const byte Ice = 60;
		public const byte CeramicTile = 61;
		public const byte Magma = 62;
		public const byte Pillar = 63;
		public const byte Crate = 64;
		public const byte StoneBrick = 65;
		
		public const string Names = "Air Stone Grass Dirt Cobblestone Wood Sapling Bedrock Water StillWater Lava" +
			" StillLava Sand Gravel GoldOre IronOre CoalOre Log Leaves Sponge Glass Red Orange Yellow Lime Green" +
			" Teal Aqua Cyan Blue Indigo Violet Magenta Pink Black Gray White Dandelion Rose BrownMushroom RedMushroom" +
			" Gold Iron DoubleSlab Slab Brick TNT Bookshelf MossyRocks Obsidian CobblestoneSlab Rope Sandstone" +
			" Snow Fire LightPink ForestGreen Brown DeepBlue Turquoise Ice CeramicTile Magma Pillar Crate StoneBrick";		
				
		public const byte MaxOriginalBlock = Block.Obsidian;
		public const int OriginalCount = MaxOriginalBlock + 1;
		public const byte MaxCpeBlock = Block.StoneBrick;
		public const int CpeCount = MaxCpeBlock + 1;
		public const byte MaxDefinedBlock = byte.MaxValue;
		public const int Count = MaxDefinedBlock + 1;
	}
}