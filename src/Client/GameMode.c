#include "GameMode.h"
#include "Inventory.h"
#include "Widgets.h"
#include "Game.h"
#include "Screens.h"
#include "Block.h"
#include "Event.h"

void GameMode_Init(void) {
	BlockID* inv = Inventory_Table;
	inv[0] = BLOCK_STONE;  inv[1] = BLOCK_COBBLE; inv[2] = BLOCK_BRICK;
	inv[3] = BLOCK_DIRT;   inv[4] = BLOCK_WOOD;   inv[5] = BLOCK_LOG;
	inv[6] = BLOCK_LEAVES; inv[7] = BLOCK_GRASS;  inv[8] = BLOCK_SLAB;
}

IGameComponent GameMode_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = GameMode_Init;
	return comp;
}

bool GameMode_HandlesKeyDown(Key key) {
	Screen* activeScreen = Gui_GetActiveScreen();
	if (key == KeyBind_Get(KeyBind_Inventory) && activeScreen == Gui_HUD) {
		Screen* screen = InventoryScreen_MakeInstance();
		Gui_SetNewScreen(screen);
		return true;
	} else if (key == KeyBind_Get(KeyBind_DropBlock) && !Game_ClassicMode) {
		if (Inventory_CanChangeSelected() && Inventory_SelectedBlock != BLOCK_AIR) {
			/* Don't assign SelectedIndex directly, because we don't want held block
			   switching positions if they already have air in their inventory hotbar. */
			Inventory_Set(Inventory_SelectedIndex, BLOCK_AIR);
			Event_RaiseVoid(&UserEvents_HeldBlockChanged);
		}
		return true;
	}
	return false;
}

bool GameMode_PickingLeft(void) {
	/* always play delete animations, even if we aren't picking a block */
	game.HeldBlockRenderer.ClickAnim(true);
	return false;
}
bool GameMode_PickingRight(void) { return false; }

void GameMode_PickLeft(BlockID old) {
	Vector3I pos = Game_SelectedPos.BlockPos;
	Game_UpdateBlock(pos.X, pos.Y, pos.Z, BLOCK_AIR);
	Event_RaiseBlock(&UserEvents_BlockChanged, pos, old, BLOCK_AIR);
}

void GameMode_PickMiddle(BlockID old) {
	if (Block_Draw[old] == DRAW_GAS) return;
	if (!(Block_CanPlace[old] || Block_CanDelete[old])) return;
	if (!Inventory_CanChangeSelected() || Inventory_SelectedBlock == old) return;
	UInt32 i;

	/* Is the currently selected block an empty slot */
	if (Inventory_Get(Inventory_SelectedIndex) == BLOCK_AIR) {
		Inventory_SetSelectedBlock(old); return;
	}

	/* Try to replace same block */	
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		if (Inventory_Get(i) != old) continue;
		Inventory_SetSelectedIndex(i); return;
	}

	/* Try to replace empty slots */
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		if (Inventory_Get(i) != BLOCK_AIR) continue;
		Inventory_Set(i, old);
		Inventory_SetSelectedIndex(i); return;
	}

	/* Finally, replace the currently selected block */
	Inventory_SetSelectedBlock(old);
}

void GameMode_PickRight(BlockID old, BlockID block) {
	Vector3I pos = Game_SelectedPos.TranslatedPos;
	Game_UpdateBlock(pos.X, pos.Y, pos.Z, block);
	Event_RaiseBlock(&UserEvents_BlockChanged, pos, old, block);
}

HotbarWidget GameMode_Hotbar;
HotbarWidget* GameMode_MakeHotbar(void) { return &GameMode_Hotbar; }
void GameMode_BeginFrame(Real64 delta) { }
void GameMode_EndFrame(Real64 delta) { }