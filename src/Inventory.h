#ifndef CC_INVENTORY_H
#define CC_INVENTORY_H
#include "Core.h"
#include "BlockID.h"

/* Manages inventory hotbar, and ordering of blocks in the inventory menu.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Inventory_Component;

#define INVENTORY_BLOCKS_PER_HOTBAR 9
#define INVENTORY_HOTBARS 9
/* Stores the currently bound blocks for all hotbars. */
extern BlockID Inventory_Table[INVENTORY_HOTBARS * INVENTORY_BLOCKS_PER_HOTBAR];
/* Mapping of indices in inventory menu to block IDs. */
extern BlockID Inventory_Map[BLOCK_COUNT];

/* Currently selected index within a hotbar. */
extern int Inventory_SelectedIndex;
/* Currently selected hotbar. */
extern int Inventory_Offset;
/* Gets the block at the nth index in the current hotbar. */
#define Inventory_Get(idx) (Inventory_Table[Inventory_Offset + (idx)])
/* Sets the block at the nth index in the current hotbar. */
#define Inventory_Set(idx, block) Inventory_Table[Inventory_Offset + (idx)] = block
/* Gets the currently selected block. */
#define Inventory_SelectedBlock Inventory_Get(Inventory_SelectedIndex)
/* Whether the user is allowed to change selected/held block. */
extern bool Inventory_CanChangeSelected;
/* Whether the user can use the inventory at all. */
/* NOTE: false prevents the user from deleting/picking/placing blocks. */
extern bool Inventory_CanUse;

/* Checks if the user can change their selected/held block. */
/* NOTE: Shows a message in chat if they are unable to. */
bool Inventory_CheckChangeSelected(void);
/* Attempts to set the currently selected index in a hotbar. */
void Inventory_SetSelectedIndex(int index);
/* Attempts to set the currently active hotbar. */
void Inventory_SetHotbarIndex(int index);
/* Attempts to set the block for the selected index in the current hotbar. */
/* NOTE: If another slot is already this block, the selected index is instead changed. */
void Inventory_SetSelectedBlock(BlockID block);
/* Sets all slots to contain their default associated block. */
/* NOTE: The order of default blocks may not be in order of ID. */
void Inventory_ApplyDefaultMapping(void);

/* Inserts the given block at its default slot in the inventory. */
/* NOTE: Replaces (doesn't move) the block that was at that slot before. */
void Inventory_AddDefault(BlockID block);
/* Removes any slots with the given block from the inventory. */
void Inventory_Remove(BlockID block);
#endif
