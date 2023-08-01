#ifndef CC_INVENTORY_H
#define CC_INVENTORY_H
#include "Core.h"
#include "BlockID.h"

/* Manages inventory hotbar, and ordering of blocks in the inventory menu.
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Inventory_Component;

/* Number of blocks in each hotbar */
#define INVENTORY_BLOCKS_PER_HOTBAR 9
/* Number of hotbars that can be selected between */
#define INVENTORY_HOTBARS 9
#define HOTBAR_MAX_INDEX (INVENTORY_BLOCKS_PER_HOTBAR - 1)

CC_VAR extern struct _InventoryData {
	/* Stores the currently bound blocks for all hotbars. */
	BlockID Table[INVENTORY_HOTBARS * INVENTORY_BLOCKS_PER_HOTBAR];
	/* Mapping of indices in inventory menu to block IDs. */
	BlockID Map[BLOCK_COUNT];
	/* Currently selected index within a hotbar. */
	int SelectedIndex;
	/* Currently selected hotbar. */
	int Offset;
	/* Whether the user is allowed to change selected/held block. */
	cc_bool CanChangeSelected;
	/* Number of blocks in each row in inventory menu. */
	cc_uint8 BlocksPerRow;
} Inventory;

/* Gets the block at the nth index in the current hotbar. */
#define Inventory_Get(idx) (Inventory.Table[Inventory.Offset + (idx)])
/* Sets the block at the nth index in the current hotbar. */
#define Inventory_Set(idx, block) Inventory.Table[Inventory.Offset + (idx)] = block
/* Gets the currently selected block. */
#define Inventory_SelectedBlock Inventory_Get(Inventory.SelectedIndex)

/* Checks if the user can change their selected/held block. */
/* NOTE: Shows a message in chat if they are unable to. */
cc_bool Inventory_CheckChangeSelected(void);
/* Attempts to set the currently selected index in a hotbar. */
void Inventory_SetSelectedIndex(int index);
/* Attempts to set the currently active hotbar. */
void Inventory_SetHotbarIndex(int index);
void Inventory_SwitchHotbar(void);
/* Attempts to set the block for the selected index in the current hotbar. */
/* NOTE: If another slot is already this block, the selected index is instead changed. */
void Inventory_SetSelectedBlock(BlockID block);
/* Attempts to set the selected block in a user-friendly manner. */
/* e.g. this method tries to replace empty slots before other blocks */
void Inventory_PickBlock(BlockID block);
/* Sets all slots to contain their default associated block. */
/* NOTE: The order of default blocks may not be in order of ID. */
void Inventory_ResetMapping(void);

/* Inserts the given block at its default slot in the inventory. */
/* NOTE: Replaces (doesn't move) the block that was at that slot before. */
void Inventory_AddDefault(BlockID block);
/* Removes any slots with the given block from the inventory. */
void Inventory_Remove(BlockID block);
#endif
