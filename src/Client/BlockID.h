#ifndef CC_BLOCKID_H
#define CC_BLOCKID_H
/* List of all core/standard block IDs
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Classic blocks */
#define BLOCK_AIR 0
#define BLOCK_STONE 1
#define BLOCK_GRASS 2
#define BLOCK_DIRT 3
#define BLOCK_COBBLE 4
#define BLOCK_WOOD 5
#define BLOCK_SAPLING 6
#define BLOCK_BEDROCK 7
#define BLOCK_WATER 8
#define BLOCK_STILL_WATER 9
#define BLOCK_LAVA 10
#define BLOCK_STILL_LAVA 11
#define BLOCK_SAND 12
#define BLOCK_GRAVEL 13
#define BLOCK_GOLD_ORE 14
#define BLOCK_IRON_ORE 15
#define BLOCK_COAL_ORE 16
#define BLOCK_LOG 17
#define BLOCK_LEAVES 18
#define BLOCK_SPONGE 19
#define BLOCK_GLASS 20
#define BLOCK_RED 21
#define BLOCK_ORANGE 22
#define BLOCK_YELLOW 23
#define BLOCK_LIME 24
#define BLOCK_GREEN 25
#define BLOCK_TEAL 26
#define BLOCK_AQUA 27
#define BLOCK_CYAN 28
#define BLOCK_BLUE 29
#define BLOCK_INDIGO 30
#define BLOCK_VIOLET 31
#define BLOCK_MAGENTA 32
#define BLOCK_PINK 33
#define BLOCK_BLACK 34
#define BLOCK_GRAY 35
#define BLOCK_WHITE 36
#define BLOCK_DANDELION 37
#define BLOCK_ROSE 38
#define BLOCK_BROWN_SHROOM 39
#define BLOCK_RED_SHROOM 40
#define BLOCK_GOLD 41
#define BLOCK_IRON 42
#define BLOCK_DOUBLE_SLAB 43
#define BLOCK_SLAB 44
#define BLOCK_BRICK 45
#define BLOCK_TNT 46
#define BLOCK_BOOKSHELF 47
#define BLOCK_MOSSY_ROCKS 48
#define BLOCK_OBSIDIAN 49

/* CPE blocks */
#define BLOCK_COBBLE_SLAB 50
#define BLOCK_ROPE 51
#define BLOCK_SANDSTONE 52
#define BLOCK_SNOW 53
#define BLOCK_FIRE 54
#define BLOCK_LIGHT_PINK 55
#define BLOCK_FOREST_GREEN 56
#define BLOCK_BROWN 57
#define BLOCK_DEEP_BLUE 58
#define BLOCK_TURQUOISE 59
#define BLOCK_ICE 60
#define BLOCK_CERAMIC_TILE 61
#define BLOCK_MAGMA 62
#define BLOCK_PILLAR 63
#define BLOCK_CRATE 64
#define BLOCK_STONE_BRICK 65

/* Max block ID used in original classic */
#define BLOCK_MAX_ORIGINAL BLOCK_OBSIDIAN
/* Number of blocks in original classic. */
#define BLOCK_ORIGINAL_COUNT (BLOCK_MAX_ORIGINAL + 1)
/* Max block ID used in original classic plus CPE blocks. */
#define BLOCK_MAX_CPE BLOCK_STONE_BRICK
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
#define BLOCK_INVALID BLOCK_MAX_DEFINED
#endif