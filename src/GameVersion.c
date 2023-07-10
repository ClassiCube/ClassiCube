#include "Game.h"
#include "Protocol.h"
#include "Block.h"
#include "Options.h"
#include "Inventory.h"

static const cc_uint8 v7_inventory[] = {
	BLOCK_STONE, BLOCK_COBBLE, BLOCK_BRICK, BLOCK_DIRT, BLOCK_WOOD, BLOCK_LOG, BLOCK_LEAVES, BLOCK_GLASS, BLOCK_SLAB,
	BLOCK_MOSSY_ROCKS, BLOCK_SAPLING, BLOCK_DANDELION, BLOCK_ROSE, BLOCK_BROWN_SHROOM, BLOCK_RED_SHROOM, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_SPONGE,
	BLOCK_RED, BLOCK_ORANGE, BLOCK_YELLOW, BLOCK_LIME, BLOCK_GREEN, BLOCK_TEAL, BLOCK_AQUA, BLOCK_CYAN, BLOCK_BLUE,
	BLOCK_INDIGO, BLOCK_VIOLET, BLOCK_MAGENTA, BLOCK_PINK, BLOCK_BLACK, BLOCK_GRAY, BLOCK_WHITE, BLOCK_COAL_ORE, BLOCK_IRON_ORE,
	BLOCK_GOLD_ORE, BLOCK_IRON, BLOCK_GOLD, BLOCK_BOOKSHELF, BLOCK_TNT, BLOCK_OBSIDIAN,
};
static const cc_uint8 v6_inventory[] = {
	BLOCK_STONE, BLOCK_COBBLE, BLOCK_DIRT, BLOCK_WOOD, BLOCK_LOG, BLOCK_LEAVES, BLOCK_SAPLING, BLOCK_DANDELION,
	BLOCK_ROSE, BLOCK_BROWN_SHROOM, BLOCK_RED_SHROOM, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_GLASS, BLOCK_SPONGE, BLOCK_GOLD,
	BLOCK_RED, BLOCK_ORANGE, BLOCK_YELLOW, BLOCK_LIME, BLOCK_GREEN, BLOCK_TEAL, BLOCK_AQUA, BLOCK_CYAN, BLOCK_BLUE,
	BLOCK_INDIGO, BLOCK_VIOLET, BLOCK_MAGENTA, BLOCK_PINK, BLOCK_BLACK, BLOCK_GRAY, BLOCK_WHITE,
};
static const cc_uint8 v5_inventory[] = {
	BLOCK_STONE, BLOCK_COBBLE, BLOCK_DIRT, BLOCK_WOOD, BLOCK_LOG, BLOCK_LEAVES, 
	BLOCK_SAPLING, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_GLASS, BLOCK_SPONGE,
};
static const cc_uint8 v4_inventory[] = {
	BLOCK_STONE, BLOCK_COBBLE, BLOCK_DIRT, BLOCK_WOOD, BLOCK_LOG, BLOCK_LEAVES, 
	BLOCK_SAPLING, BLOCK_SAND, BLOCK_GRAVEL,
};

static const cc_uint8 v7_hotbar[INVENTORY_BLOCKS_PER_HOTBAR] = {
	BLOCK_STONE, BLOCK_COBBLE, BLOCK_BRICK, BLOCK_DIRT, BLOCK_WOOD, BLOCK_LOG, BLOCK_LEAVES, BLOCK_GLASS, BLOCK_SLAB
};
static const cc_uint8 v6_hotbar[INVENTORY_BLOCKS_PER_HOTBAR] = {
	BLOCK_STONE, BLOCK_COBBLE, BLOCK_DIRT, BLOCK_WOOD, BLOCK_LOG, BLOCK_LEAVES, BLOCK_SAPLING, BLOCK_DANDELION, BLOCK_ROSE
};
static const cc_uint8 v5_hotbar[INVENTORY_BLOCKS_PER_HOTBAR] = {
	BLOCK_STONE, BLOCK_DIRT, BLOCK_SPONGE, BLOCK_WOOD, BLOCK_SAPLING, BLOCK_LOG, BLOCK_LEAVES, BLOCK_GLASS, BLOCK_GRAVEL
};
static const cc_uint8 v4_hotbar[INVENTORY_BLOCKS_PER_HOTBAR] = {
	BLOCK_STONE, BLOCK_DIRT, BLOCK_COBBLE, BLOCK_WOOD, BLOCK_SAPLING, BLOCK_LOG, BLOCK_LEAVES, BLOCK_SAND, BLOCK_GRAVEL
};

static const struct GameVersion version_cpe  = { 
	"0.30",     true, VERSION_CPE, 
	PROTOCOL_0030, BLOCK_MAX_CPE, 
	10, sizeof(v7_inventory), NULL,         v7_hotbar,
	"texpacks/default.zip"
};
static const struct GameVersion version_0030 = {
	"0.30",    false, VERSION_0030,
	PROTOCOL_0030, BLOCK_OBSIDIAN, 
	 9, sizeof(v7_inventory), v7_inventory, v7_hotbar,
	 "texpacks/default.zip"
};
static const struct GameVersion version_0023 = {
	"0.0.23a", false, VERSION_0023,
	PROTOCOL_0020, BLOCK_GOLD, 
	 8, sizeof(v6_inventory), v6_inventory, v6_hotbar,
	 "texpacks/default_0023.zip"
};
static const struct GameVersion version_0019 = {
	"0.0.19a", false, VERSION_0019,
	PROTOCOL_0019, BLOCK_GLASS, 
	 6, sizeof(v5_inventory), v5_inventory, v5_hotbar,
	 "texpacks/default_0023.zip"
};
static const struct GameVersion version_0017 = {
	"0.0.17a", false, VERSION_0017,
	PROTOCOL_0017, BLOCK_LEAVES, 
	 6, sizeof(v4_inventory), v4_inventory, v4_hotbar,
	 "texpacks/default_0023.zip"
};

void GameVersion_Load(void) {
	cc_bool hasCPE = !Game_ClassicMode && Options_GetBool(OPT_CPE, true);
	int version    = Options_GetInt(OPT_GAME_VERSION, VERSION_0017, VERSION_0030, VERSION_0030);
	const struct GameVersion* ver = &version_cpe;

	if (hasCPE) {
		/* defaults to CPE already */
	} else if (version == VERSION_0030) {
		ver = &version_0030;
	} else if (version == VERSION_0023) {
		ver = &version_0023;
	} else if (version == VERSION_0019) {
		ver = &version_0019;
	} else if (version == VERSION_0017) {
		ver = &version_0017;
	}

	Game_Version = *ver;
}
