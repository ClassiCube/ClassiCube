#include "Inventory.h"
#include "Funcs.h"
#include "Game.h"
#include "Block.h"
#include "Events.h"

bool Inventory_CanChangeSelected(void) {
	if (!Inventory_CanChangeHeldBlock) {
		//game.Chat.Add("&e/client: &cThe server has forbidden you from changing your held block.");
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
	return b == BlockID_DoubleSlab || b == BlockID_Bedrock ||
		b == BlockID_Grass || Block_IsLiquid(b);
}

BlockID Inventory_DefaultMapping(Int32 i) {
#if USE16_BIT
	if ((i >= Block_CpeCount && i < 256) || i == BlockID_Air) return BlockID_Invalid;
#else
	if (i >= BLOCK_CPE_COUNT || i == BlockID_Air) return BlockID_Invalid;
#endif
	if (!Game_ClassicMode) return (BlockID)i;

	if (i >= 25 && i <= 40) {
		return (BlockID)(BlockID_Red + (i - 25));
	}
	if (i >= 18 && i <= 21) {
		return (BlockID)(BlockID_Dandelion + (i - 18));
	}

	switch (i) {
		/* First row */
	case 3: return BlockID_Cobblestone;
	case 4: return BlockID_Brick;
	case 5: return BlockID_Dirt;
	case 6: return BlockID_Wood;

		/* Second row */
	case 12: return BlockID_Log;
	case 13: return BlockID_Leaves;
	case 14: return BlockID_Glass;
	case 15: return BlockID_Slab;
	case 16: return BlockID_MossyRocks;
	case 17: return BlockID_Sapling;

		/* Third row */
	case 22: return BlockID_Sand;
	case 23: return BlockID_Gravel;
	case 24: return BlockID_Sponge;

		/* Fifth row */
	case 41: return BlockID_CoalOre;
	case 42: return BlockID_IronOre;
	case 43: return BlockID_GoldOre;
	case 44: return BlockID_DoubleSlab;
	case 45: return BlockID_Iron;
	case 46: return BlockID_Gold;
	case 47: return BlockID_Bookshelf;
	case 48: return BlockID_TNT;
	}
	return (BlockID)i;
}

void Inventory_SetDefaultMapping(void) {
	Int32 i;
	for (i = 0; i < Array_NumElements(Inventory_Map); i++) {
		Inventory_Map[i] = (BlockID)i;
	}
	for (i = 0; i < Array_NumElements(Inventory_Map); i++) {
		BlockID mapping = Inventory_DefaultMapping(i);
		if (Game_PureClassic && Inventory_IsHackBlock(mapping)) {
			mapping = BlockID_Invalid;
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
	for (i = 0; i < Array_NumElements(Inventory_Map); i++) {
		if (Inventory_Map[i] != block) continue;
		Inventory_Map[i] = Inventory_DefaultMapping(i);
	}
}

void Inventory_Remove(BlockID block) {
	Int32 i;
	for (i = 0; i < Array_NumElements(Inventory_Map); i++) {
		if (Inventory_Map[i] != block) continue;
		Inventory_Map[i] = BlockID_Invalid;
	}
}

void Inventory_PushToFreeSlots(Int32 i) {
	BlockID block = Inventory_Map[i];
	Int32 j;
	/* The slot was already pushed out in the past
	TODO: find a better way of fixing this */
	for (j = 1; j < Array_NumElements(Inventory_Map); j++) {
		if (j != i && Inventory_Map[j] == block) return;
	}

	for (j = block; j < Array_NumElements(Inventory_Map); j++) {
		if (Inventory_Map[j] == BlockID_Invalid) {
			Inventory_Map[j] = block; return;
		}
	}
	for (j = 1; j < block; j++) {
		if (Inventory_Map[j] == BlockID_Invalid) {
			Inventory_Map[j] = block; return;
		}
	}
}

void Inventory_Insert(Int32 i, BlockID block) {
	if (Inventory_Map[i] == block) return;
	/* Need to push the old block to a different slot if different block. */
	if (Inventory_Map[i] != BlockID_Invalid) Inventory_PushToFreeSlots(i);
	Inventory_Map[i] = block;
}


IGameComponent Inventory_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init  = Inventory_SetDefaultMapping;
	comp.Reset = Inventory_SetDefaultMapping;
	return comp;
}