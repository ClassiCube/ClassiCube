#ifndef CC_BLOCKID_H
#define CC_BLOCKID_H
/* List of all core/standard block IDs
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Classic blocks */
#define BlockID_Air 0
#define BlockID_Stone 1
#define BlockID_Grass 2
#define BlockID_Dirt 3
#define BlockID_Cobblestone 4
#define BlockID_Wood 5
#define BlockID_Sapling 6
#define BlockID_Bedrock 7
#define BlockID_Water 8
#define BlockID_StillWater 9
#define BlockID_Lava 10
#define BlockID_StillLava 11
#define BlockID_Sand 12
#define BlockID_Gravel 13
#define BlockID_GoldOre 14
#define BlockID_IronOre 15
#define BlockID_CoalOre 16
#define BlockID_Log 17
#define BlockID_Leaves 18
#define BlockID_Sponge 19
#define BlockID_Glass 20
#define BlockID_Red 21
#define BlockID_Orange 22
#define BlockID_Yellow 23
#define BlockID_Lime 24
#define BlockID_Green 25
#define BlockID_Teal 26
#define BlockID_Aqua 27
#define BlockID_Cyan 28
#define BlockID_Blue 29
#define BlockID_Indigo 30
#define BlockID_Violet 31
#define BlockID_Magenta 32
#define BlockID_Pink 33
#define BlockID_Black 34
#define BlockID_Gray 35
#define BlockID_White 36
#define BlockID_Dandelion 37
#define BlockID_Rose 38
#define BlockID_BrownMushroom 39
#define BlockID_RedMushroom 40
#define BlockID_Gold 41
#define BlockID_Iron 42
#define BlockID_DoubleSlab 43
#define BlockID_Slab 44
#define BlockID_Brick 45
#define BlockID_TNT 46
#define BlockID_Bookshelf 47
#define BlockID_MossyRocks 48
#define BlockID_Obsidian 49

/* CPE blocks */
#define BlockID_CobblestoneSlab 50
#define BlockID_Rope 51
#define BlockID_Sandstone 52
#define BlockID_Snow 53
#define BlockID_Fire 54
#define BlockID_LightPink 55
#define BlockID_ForestGreen 56
#define BlockID_Brown 57
#define BlockID_DeepBlue 58
#define BlockID_Turquoise 59
#define BlockID_Ice 60
#define BlockID_CeramicTile 61
#define BlockID_Magma 62
#define BlockID_Pillar 63
#define BlockID_Crate 64
#define BlockID_StoneBrick 65

/* Max block ID used in original classic */
#define BLOCK_MAX_ORIGINAL BlockID_Obsidian
/* Number of blocks in original classic. */
#define BLOCK_ORIGINAL_COUNT (BLOCK_MAX_ORIGINAL + 1)
/* Max block ID used in original classic plus CPE blocks. */
#define BLOCK_MAX_CPE BlockID_StoneBrick
/* Number of blocks in original classic plus CPE blocks. */
#define BLOCK_CPE_COUNT (BLOCK_MAX_CPE + 1)

#if USE16_BIT
#define BLOCK_MAX_DEFINED 0xFFF
#else
#define BLOCK_MAX_DEFINED 0xFF
#endif

#define BLOCK_RAW_NAMES "Air Stone Grass Dirt Cobblestone Wood Sapling Bedrock Water StillWater Lava"\
" StillLava Sand Gravel GoldOre IronOre CoalOre Log Leaves Sponge Glass Red Orange Yellow Lime Green Teal"\
" Aqua Cyan Blue Indigo Violet Magenta Pink Black Gray White Dandelion Rose BrownMushroom RedMushroom Gold"\
" Iron DoubleSlab Slab Brick TNT Bookshelf MossyRocks Obsidian CobblestoneSlab Rope Sandstone Snow Fire LightPink"\
" ForestGreen Brown DeepBlue Turquoise Ice CeramicTile Magma Pillar Crate StoneBrick"

#define BLOCK_COUNT (BLOCK_MAX_DEFINED + 1)
#define BlockID_Invalid BLOCK_MAX_DEFINED
#endif