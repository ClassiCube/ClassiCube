#ifndef CC_INVENTORY_H
#define CC_INVENTORY_H
#include "Typedefs.h"
#include "GameStructs.h"
#include "BlockID.h"

/* Manages inventory hotbar, and ordering of blocks in the inventory menu.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define INVENTORY_BLOCKS_PER_HOTBAR 9
#define INVENTORY_HOTBARS 9
/* Stores the blocks for all hotbars. */
BlockID Inventory_Table[INVENTORY_HOTBARS * INVENTORY_BLOCKS_PER_HOTBAR];
/* Mapping of indices in inventory menu to block IDs. */
BlockID Inventory_Map[BLOCK_COUNT];
IGameComponent Inventory_MakeComponent(void);

/* Index of selected block within the current hotbar. */
Int32 Inventory_SelectedIndex;
/* Offset within Inventory_Table for the first block of the current hotbar. */
Int32 Inventory_Offset;
/* Gets block at given index within the current hotbar. */
#define Inventory_Get(idx) (Inventory_Table[Inventory_Offset + (idx)])
/* Sets block at given index within the current hotbar */
#define Inventory_Set(idx, block) Inventory_Table[Inventory_Offset + (idx)] = block
/* Block currently selected by the player. */
#define Inventory_SelectedBlock Inventory_Get(Inventory_SelectedIndex)
/* Whether the player is allowed to change the held / selected block. */
bool Inventory_CanChangeHeldBlock;

/* Gets whether player can change selected block, showing message in chat if not. */
bool Inventory_CanChangeSelected(void);
/* Sets index of the selected block within the current hotbar.
Fails if the server has forbidden user from changing the held block. */
void Inventory_SetSelectedIndex(Int32 index);
/* Sets offset within Inventory_Table for the first block of the current hotbar.
Fails if the server has forbidden user from changing the held block. */
void Inventory_SetOffset(Int32 offset);
/* Sets the block currently selected by the player.
Fails if the server has forbidden user from changing the held block. */
void Inventory_SetSelectedBlock(BlockID block);
/* Sets default mapping for every block. */
void Inventory_SetDefaultMapping(void);

void Inventory_AddDefault(BlockID block);
void Inventory_Reset(BlockID block);
void Inventory_Remove(BlockID block);
void Inventory_Insert(Int32 i, BlockID block);
#endif