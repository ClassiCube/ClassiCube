#ifndef CC_SURVIVALINV_H
#define CC_SURVIVALINV_H
#include "Core.h"
#include "BlockID.h"
CC_BEGIN_HEADER

/* Survival inventory: 36-slot container (9 hotbar + 27 main) with stack counts.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;

#define SURINV_HOTBAR_SLOTS  9
#define SURINV_MAIN_SLOTS    27
#define SURINV_TOTAL_SLOTS   36
#define SURINV_MAX_STACK     64

struct SurvivalSlot { BlockID id; int count; };
extern struct SurvivalSlot SurvivalInv_Slots[SURINV_TOTAL_SLOTS];

/* Adds count of block to inventory; returns false if inventory is full */
cc_bool SurvivalInv_Add(BlockID block, int count);
/* Removes count items from slot idx */
void    SurvivalInv_Remove(int slotIdx, int count);
/* Removes count of block from wherever it appears in inventory */
void    SurvivalInv_RemoveBlock(BlockID block, int count);
/* Removes one item from the slot matching the currently selected hotbar index */
void    SurvivalInv_ConsumeSelected(void);
/* Number of items in the slot matching the currently selected hotbar index */
int     SurvivalInv_SelectedCount(void);
/* Returns total count of the given block across all slots */
int     SurvivalInv_Count(BlockID block);
/* Empties every slot. Does NOT touch the live hotbar (call SyncHotbar for that) */
void    SurvivalInv_Clear(void);
/* Copies hotbar slots 0-8 into Inventory.Table so the HUD stays in sync */
void    SurvivalInv_SyncHotbar(void);

void SurvivalInv_Show(void);
void SurvivalInv_Hide(void);

extern struct IGameComponent SurvivalInv_Component;

CC_END_HEADER
#endif
