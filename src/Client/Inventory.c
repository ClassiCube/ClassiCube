#include "Inventory.h"
#include "Funcs.h"
#include "Game.h"
#include "Block.h"
#include "Event.h"
#include "Chat.h"

bool Inventory_CanChangeSelected(void) {
	if (!Inventory_CanChangeHeldBlock) {
		String msg = String_FromConst("&e/client: &cThe server has forbidden you from changing your held block.");
		Chat_Add(&msg);
		return false;
	}
	return true;
}

void Inventory_SetSelectedIndex(Int32 index) {
	if (!Inventory_CanChangeSelected()) return;
	Inventory_SelectedIndex = index;
	Event_RaiseVoid(&UserEvents_HeldBlockChanged);
}

void Inventory_SetOffset(Int32 offset) {
	if (!Inventory_CanChangeSelected() || Game_ClassicMode) return;
	Inventory_Offset = offset;
	Event_RaiseVoid(&UserEvents_HeldBlockChanged);
}

void Inventory_SetSelectedBlock(BlockID block) {
	if (!Inventory_CanChangeSelected()) return;

	/* Swap with the current, if the new block is already in the hotbar */
	Int32 i;
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		if (Inventory_Get(i) != block) continue;
		Inventory_Set(i, Inventory_SelectedBlock);
		break;
	}

	Inventory_Set(Inventory_SelectedIndex, block);
	Event_RaiseVoid(&UserEvents_HeldBlockChanged);
}

BlockID inv_classicTable[] = {
	BLOCK_STONE, BLOCK_COBBLE, BLOCK_BRICK, BLOCK_DIRT, BLOCK_WOOD, BLOCK_LOG, BLOCK_LEAVES, BLOCK_GLASS, BLOCK_SLAB,
	BLOCK_MOSSY_ROCKS, BLOCK_SAPLING, BLOCK_DANDELION, BLOCK_ROSE, BLOCK_BROWN_SHROOM, BLOCK_RED_SHROOM, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_SPONGE,
	BLOCK_RED, BLOCK_ORANGE, BLOCK_YELLOW, BLOCK_LIME, BLOCK_GREEN, BLOCK_TEAL, BLOCK_AQUA, BLOCK_CYAN, BLOCK_BLUE,
	BLOCK_INDIGO, BLOCK_VIOLET, BLOCK_MAGENTA, BLOCK_PINK, BLOCK_BLACK, BLOCK_GRAY, BLOCK_WHITE, BLOCK_COAL_ORE, BLOCK_IRON_ORE,
	BLOCK_GOLD_ORE, BLOCK_IRON, BLOCK_GOLD, BLOCK_BOOKSHELF, BLOCK_TNT, BLOCK_OBSIDIAN,
};
BlockID inv_classicHacksTable[] = {
	BLOCK_STONE, BLOCK_GRASS, BLOCK_COBBLE, BLOCK_BRICK, BLOCK_DIRT, BLOCK_WOOD, BLOCK_BEDROCK, BLOCK_WATER, BLOCK_STILL_WATER, BLOCK_LAVA,
	BLOCK_STILL_LAVA, BLOCK_LOG, BLOCK_LEAVES, BLOCK_GLASS, BLOCK_SLAB, BLOCK_MOSSY_ROCKS, BLOCK_SAPLING, BLOCK_DANDELION, BLOCK_ROSE, BLOCK_BROWN_SHROOM,
	BLOCK_RED_SHROOM, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_SPONGE, BLOCK_RED, BLOCK_ORANGE, BLOCK_YELLOW, BLOCK_LIME, BLOCK_GREEN, BLOCK_TEAL,
	BLOCK_AQUA, BLOCK_CYAN, BLOCK_BLUE, BLOCK_INDIGO, BLOCK_VIOLET, BLOCK_MAGENTA, BLOCK_PINK, BLOCK_BLACK, BLOCK_GRAY, BLOCK_WHITE,
	BLOCK_COAL_ORE, BLOCK_IRON_ORE, BLOCK_GOLD_ORE, BLOCK_DOUBLE_SLAB, BLOCK_IRON, BLOCK_GOLD, BLOCK_BOOKSHELF, BLOCK_TNT, BLOCK_OBSIDIAN,
};

BlockID Inventory_DefaultMapping(int slot) {
	if (Game_PureClassic) {
		if (slot < 9 * 4 + 6)  return inv_classicTable[slot];
	} else if (Game_ClassicMode) {
		if (slot < 10 * 4 + 9) return inv_classicHacksTable[slot];
	} else if (slot < BLOCK_MAX_CPE) {
		return (BlockID)(slot + 1);
	}
	return BLOCK_AIR;
}

void Inventory_SetDefaultMapping(void) {
	Int32 slot;
	for (slot = 0; slot < Array_Elems(Inventory_Map); slot++) {
		Inventory_Map[slot] = Inventory_DefaultMapping(slot);
	}
}

void Inventory_AddDefault(BlockID block) {
	if (block >= BLOCK_CPE_COUNT) {
		Inventory_Map[block - 1] = block; return;
	}

	Int32 slot;
	for (slot = 0; slot < BLOCK_MAX_CPE; slot++) {
		if (Inventory_DefaultMapping(slot) != block) continue;
		Inventory_Map[slot] = block;
		return;
	}
}

void Inventory_Remove(BlockID block) {
	Int32 slot;
	for (slot = 0; slot < Array_Elems(Inventory_Map); slot++) {
		if (Inventory_Map[slot] == block) Inventory_Map[slot] = BLOCK_AIR;
	}
}

void Inventory_ResetState(void) {
	Inventory_SetDefaultMapping();
	Inventory_CanChangeHeldBlock = true;
	Inventory_CanPick = true;
}

IGameComponent Inventory_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init  = Inventory_ResetState;
	comp.Reset = Inventory_ResetState;
	return comp;
}