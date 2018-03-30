#ifndef CC_INVENTORY_H
#define CC_INVENTORY_H
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

Int32 Inventory_SelectedIndex;
Int32 Inventory_Offset;
#define Inventory_Get(idx) (Inventory_Table[Inventory_Offset + (idx)])
#define Inventory_Set(idx, block) Inventory_Table[Inventory_Offset + (idx)] = block
#define Inventory_SelectedBlock Inventory_Get(Inventory_SelectedIndex)
bool Inventory_CanChangeHeldBlock, Inventory_CanPick;

bool Inventory_CanChangeSelected(void);
void Inventory_SetSelectedIndex(Int32 index);
void Inventory_SetOffset(Int32 offset);
void Inventory_SetSelectedBlock(BlockID block);
void Inventory_SetDefaultMapping(void);

void Inventory_AddDefault(BlockID block);
void Inventory_Reset(BlockID block);
void Inventory_Remove(BlockID block);
void Inventory_Insert(Int32 i, BlockID block);
#endif