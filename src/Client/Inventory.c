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

bool Inventory_IsHackBlock(BlockID b) {
	return b == BLOCK_DOUBLE_SLAB || b == BLOCK_BEDROCK || b == BLOCK_GRASS || Block_IsLiquid[b];
}

BlockID Inventory_DefaultMapping(Int32 i) {
#if USE16_BIT
	if ((i >= Block_CpeCount && i < 256) || i == BLOCK_AIR) return BLOCK_AIR;
#else
	if (i >= BLOCK_CPE_COUNT || i == BLOCK_AIR) return BLOCK_AIR;
#endif
	if (!Game_ClassicMode) return (BlockID)i;

	if (i >= 25 && i <= 40) {
		return (BlockID)(BLOCK_RED + (i - 25));
	}
	if (i >= 18 && i <= 21) {
		return (BlockID)(BLOCK_DANDELION + (i - 18));
	}

	switch (i) {
		/* First row */
	case 3: return BLOCK_COBBLE;
	case 4: return BLOCK_BRICK;
	case 5: return BLOCK_DIRT;
	case 6: return BLOCK_WOOD;

		/* Second row */
	case 12: return BLOCK_LOG;
	case 13: return BLOCK_LEAVES;
	case 14: return BLOCK_GLASS;
	case 15: return BLOCK_SLAB;
	case 16: return BLOCK_MOSSY_ROCKS;
	case 17: return BLOCK_SAPLING;

		/* Third row */
	case 22: return BLOCK_SAND;
	case 23: return BLOCK_GRAVEL;
	case 24: return BLOCK_SPONGE;

		/* Fifth row */
	case 41: return BLOCK_COAL_ORE;
	case 42: return BLOCK_IRON_ORE;
	case 43: return BLOCK_GOLD_ORE;
	case 44: return BLOCK_DOUBLE_SLAB;
	case 45: return BLOCK_IRON;
	case 46: return BLOCK_GOLD;
	case 47: return BLOCK_BOOKSHELF;
	case 48: return BLOCK_TNT;
	}
	return (BlockID)i;
}

void Inventory_SetDefaultMapping(void) {
	Int32 i;
	for (i = 0; i < Array_Elems(Inventory_Map); i++) {
		Inventory_Map[i] = (BlockID)i;
	}
	for (i = 0; i < Array_Elems(Inventory_Map); i++) {
		BlockID mapping = Inventory_DefaultMapping(i);
		if (Game_PureClassic && Inventory_IsHackBlock(mapping)) {
			mapping = BLOCK_AIR;
		}
		if (mapping != i) Inventory_Map[i] = mapping;
	}
}

void Inventory_AddDefault(BlockID block) {
	if (block >= BLOCK_CPE_COUNT || Inventory_DefaultMapping(block) == block) {
		Inventory_Map[block] = block;
		return;
	}

	Int32 i;
	for (i = 0; i < BLOCK_CPE_COUNT; i++) {
		if (Inventory_DefaultMapping(i) != block) continue;
		Inventory_Map[i] = block;
		return;
	}
}

void Inventory_Reset(BlockID block) {
	Int32 i;
	for (i = 0; i < Array_Elems(Inventory_Map); i++) {
		if (Inventory_Map[i] != block) continue;
		Inventory_Map[i] = Inventory_DefaultMapping(i);
	}
}

void Inventory_Remove(BlockID block) {
	Int32 i;
	for (i = 0; i < Array_Elems(Inventory_Map); i++) {
		if (Inventory_Map[i] != block) continue;
		Inventory_Map[i] = BLOCK_AIR;
	}
}

void Inventory_PushToFreeSlots(Int32 i) {
	BlockID block = Inventory_Map[i];
	Int32 j;
	/* The slot was already pushed out in the past
	TODO: find a better way of fixing this */
	for (j = 1; j < Array_Elems(Inventory_Map); j++) {
		if (j != i && Inventory_Map[j] == block) return;
	}

	for (j = block; j < Array_Elems(Inventory_Map); j++) {
		if (Inventory_Map[j] == BLOCK_AIR) {
			Inventory_Map[j] = block; return;
		}
	}
	for (j = 1; j < block; j++) {
		if (Inventory_Map[j] == BLOCK_AIR) {
			Inventory_Map[j] = block; return;
		}
	}
}

void Inventory_Insert(Int32 i, BlockID block) {
	if (Inventory_Map[i] == block) return;
	/* Need to push the old block to a different slot if different block. */
	if (Inventory_Map[i] != BLOCK_AIR) Inventory_PushToFreeSlots(i);
	Inventory_Map[i] = block;
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