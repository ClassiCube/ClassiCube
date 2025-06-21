#include "Inventory.h"
#include "Funcs.h"
#include "Game.h"
#include "Block.h"
#include "Event.h"
#include "Chat.h"
#include "Protocol.h"

struct _InventoryData Inventory;

cc_bool Inventory_CheckChangeSelected(void) {
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

void Inventory_SwitchHotbar(void) {
	int index = Inventory.Offset == 0 ? 1 : 0;
	Inventory_SetHotbarIndex(index);
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
	CPE_SendNotifyAction(NOTIFY_ACTION_BLOCK_LIST_SELECTED, block);
}

void Inventory_PickBlock(BlockID block) {
	int i;
	if (!Inventory_CheckChangeSelected() || Inventory_SelectedBlock == block) return;

	/* Vanilla classic client doesn't let you select these blocks */
	if (Game_PureClassic) {
		if (block == BLOCK_GRASS)       block = BLOCK_DIRT;
		if (block == BLOCK_DOUBLE_SLAB) block = BLOCK_SLAB;
	}

	/* Try to replace same block */
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		if (Inventory_Get(i) != block) continue;
		Inventory_SetSelectedIndex(i); return;
	}

	if (AutoRotate_Enabled) {
		/* Try to replace existing autorotate variant */
		for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
			if (AutoRotate_BlocksShareGroup(Inventory_Get(i), block)) {
				Inventory_SetSelectedIndex(i);
				Inventory_SetSelectedBlock(block);
				return;
			}
		}
	}

	/* Is the currently selected block an empty slot? */
	if (Inventory_SelectedBlock == BLOCK_AIR) {
		Inventory_SetSelectedBlock(block); return;
	}

	/* Try to replace empty slots */
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		if (Inventory_Get(i) != BLOCK_AIR) continue;
		Inventory_Set(i, block);
		Inventory_SetSelectedIndex(i); return;
	}

	/* Finally, replace the currently selected block */
	Inventory_SetSelectedBlock(block);
}

/* Returns default block that should go in the given inventory slot */
static BlockID DefaultMapping(int slot) {
	if (Game_ClassicMode) {
		if (slot < Game_Version.InventorySize) return Game_Version.Inventory[slot];
	} else if (slot < Game_Version.MaxCoreBlock) {
		return (BlockID)(slot + 1);
	}
	return BLOCK_AIR;
}

void Inventory_ResetMapping(void) {
	int slot;
	for (slot = 0; slot < Array_Elems(Inventory.Map); slot++) {
		Inventory.Map[slot] = DefaultMapping(slot);
	}
}

void Inventory_AddDefault(BlockID block) {
	int slot;
	if (block > BLOCK_MAX_CPE) {
		Inventory.Map[block - 1] = block; return;
	}
	
	for (slot = 0; slot < BLOCK_MAX_CPE; slot++) {
		if (DefaultMapping(slot) != block) continue;
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
static void OnReset(void) {
	Inventory_ResetMapping();
	Inventory.CanChangeSelected = true;
}

static void OnInit(void) {
	int i;
	BlockID* inv = Inventory.Table;
	OnReset();
	Inventory.BlocksPerRow = Game_Version.BlocksPerRow;
	
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		inv[i] = Game_Version.Hotbar[i];
	}
}

struct IGameComponent Inventory_Component = {
	OnInit,  /* Init  */
	NULL,    /* Free  */
	OnReset, /* Reset */
};
