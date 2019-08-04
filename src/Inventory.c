#include "Inventory.h"
#include "Funcs.h"
#include "Game.h"
#include "Block.h"
#include "Event.h"
#include "Chat.h"
#include "GameStructs.h"

struct _InventoryData Inventory;

bool Inventory_CheckChangeSelected(void) {
	if (!Inventory.CanChangeSelected) {
		Chat_AddRaw("&cThe server has forbidden you from changing your held block.");
		return false;
	}
	return true;
}

void Inventory_SetSelectedIndex(int index) {
	if (!Inventory_CheckChangeSelected()) return;
	Inventory.SelectedIndex = index;
	Event_RaiseVoid(&UserEvents.HeldBlockChanged);
}

void Inventory_SetHotbarIndex(int index) {
	if (!Inventory_CheckChangeSelected() || Game_ClassicMode) return;
	Inventory.Offset = index * INVENTORY_BLOCKS_PER_HOTBAR;
	Event_RaiseVoid(&UserEvents.HeldBlockChanged);
}

void Inventory_SetSelectedBlock(BlockID block) {
	int i;
	if (!Inventory_CheckChangeSelected()) return;

	/* Swap with currently selected block if given block is already in the hotbar */
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		if (Inventory_Get(i) != block) continue;
		Inventory_Set(i, Inventory_SelectedBlock);
		break;
	}

	Inventory_Set(Inventory.SelectedIndex, block);
	Event_RaiseVoid(&UserEvents.HeldBlockChanged);
}

void Inventory_SetBlockAtIndex(BlockID block, int index) {
	int i;
	if (!Inventory_CheckChangeSelected()) return;

	/* Swap with currently selected block if given block is already in the hotbar */
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		if (Inventory_Get(i) != block) continue;
		Inventory_Set(i, Inventory_Get(index));
		break;
	}

	Inventory_Set(index, block);
	if (Inventory.SelectedIndex == index) {
		Event_RaiseVoid(&UserEvents.HeldBlockChanged);
	}
}

static const uint8_t classicInventory[42] = {
	BLOCK_STONE, BLOCK_COBBLE, BLOCK_BRICK, BLOCK_DIRT, BLOCK_WOOD, BLOCK_LOG, BLOCK_LEAVES, BLOCK_GLASS, BLOCK_SLAB,
	BLOCK_MOSSY_ROCKS, BLOCK_SAPLING, BLOCK_DANDELION, BLOCK_ROSE, BLOCK_BROWN_SHROOM, BLOCK_RED_SHROOM, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_SPONGE,
	BLOCK_RED, BLOCK_ORANGE, BLOCK_YELLOW, BLOCK_LIME, BLOCK_GREEN, BLOCK_TEAL, BLOCK_AQUA, BLOCK_CYAN, BLOCK_BLUE,
	BLOCK_INDIGO, BLOCK_VIOLET, BLOCK_MAGENTA, BLOCK_PINK, BLOCK_BLACK, BLOCK_GRAY, BLOCK_WHITE, BLOCK_COAL_ORE, BLOCK_IRON_ORE,
	BLOCK_GOLD_ORE, BLOCK_IRON, BLOCK_GOLD, BLOCK_BOOKSHELF, BLOCK_TNT, BLOCK_OBSIDIAN,
};
static const uint8_t classicHacksInventory[49] = {
	BLOCK_STONE, BLOCK_GRASS, BLOCK_COBBLE, BLOCK_BRICK, BLOCK_DIRT, BLOCK_WOOD, BLOCK_BEDROCK, BLOCK_WATER, BLOCK_STILL_WATER, BLOCK_LAVA,
	BLOCK_STILL_LAVA, BLOCK_LOG, BLOCK_LEAVES, BLOCK_GLASS, BLOCK_SLAB, BLOCK_MOSSY_ROCKS, BLOCK_SAPLING, BLOCK_DANDELION, BLOCK_ROSE, BLOCK_BROWN_SHROOM,
	BLOCK_RED_SHROOM, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_SPONGE, BLOCK_RED, BLOCK_ORANGE, BLOCK_YELLOW, BLOCK_LIME, BLOCK_GREEN, BLOCK_TEAL,
	BLOCK_AQUA, BLOCK_CYAN, BLOCK_BLUE, BLOCK_INDIGO, BLOCK_VIOLET, BLOCK_MAGENTA, BLOCK_PINK, BLOCK_BLACK, BLOCK_GRAY, BLOCK_WHITE,
	BLOCK_COAL_ORE, BLOCK_IRON_ORE, BLOCK_GOLD_ORE, BLOCK_DOUBLE_SLAB, BLOCK_IRON, BLOCK_GOLD, BLOCK_BOOKSHELF, BLOCK_TNT, BLOCK_OBSIDIAN,
};

static BlockID Inventory_DefaultMapping(int slot) {
	if (Game_PureClassic) {
		if (slot < 9 * 4 + 6)  return classicInventory[slot];
	} else if (Game_ClassicMode) {
		if (slot < 10 * 4 + 9) return classicHacksInventory[slot];
	} else if (slot < BLOCK_MAX_CPE) {
		return (BlockID)(slot + 1);
	}
	return BLOCK_AIR;
}

void Inventory_ApplyDefaultMapping(void) {
	int slot;
	for (slot = 0; slot < Array_Elems(Inventory.Map); slot++) {
		Inventory.Map[slot] = Inventory_DefaultMapping(slot);
	}
}

void Inventory_AddDefault(BlockID block) {
	int slot;
	if (block >= BLOCK_CPE_COUNT) {
		Inventory.Map[block - 1] = block; return;
	}
	
	for (slot = 0; slot < BLOCK_MAX_CPE; slot++) {
		if (Inventory_DefaultMapping(slot) != block) continue;
		Inventory.Map[slot] = block;
		return;
	}
}

void Inventory_Remove(BlockID block) {
	int slot;
	for (slot = 0; slot < Array_Elems(Inventory.Map); slot++) {
		if (Inventory.Map[slot] == block) Inventory.Map[slot] = BLOCK_AIR;
	}
}


/*########################################################################################################################*
*--------------------------------------------------Inventory component----------------------------------------------------*
*#########################################################################################################################*/
static void Inventory_Reset(void) {
	Inventory_ApplyDefaultMapping();
	Inventory.CanChangeSelected = true;
}

static void Inventory_Init(void) {
	BlockID* inv = Inventory.Table;
	Inventory_Reset();
	
	inv[0] = BLOCK_STONE;  inv[1] = BLOCK_COBBLE; inv[2] = BLOCK_BRICK;
	inv[3] = BLOCK_DIRT;   inv[4] = BLOCK_WOOD;   inv[5] = BLOCK_LOG;
	inv[6] = BLOCK_LEAVES; inv[7] = BLOCK_GRASS;  inv[8] = BLOCK_SLAB;
}

struct IGameComponent Inventory_Component = {
	Inventory_Init,  /* Init  */
	NULL,            /* Free  */
	Inventory_Reset, /* Reset */
};
