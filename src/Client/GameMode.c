#include "GameMode.h"
#include "Inventory.h"
#include "Widgets.h"
#include "Game.h"
#include "Screens.h"
#include "Block.h"
#include "Events.h"

void GameMode_Init(void) {
	BlockID* inv = Inventory_Table;
	inv[0] = BlockID_Stone;  inv[1] = BlockID_Cobblestone; inv[2] = BlockID_Brick;
	inv[3] = BlockID_Dirt;   inv[4] = BlockID_Wood;        inv[5] = BlockID_Log;
	inv[6] = BlockID_Leaves; inv[7] = BlockID_Grass;       inv[8] = BlockID_Slab;
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
		if (Inventory_CanChangeSelected()) {
			/* Don't assign SelectedIndex directly, because we don't want held block
			   switching positions if they already have air in their inventory hotbar. */
			Inventory_Set(Inventory_SelectedIndex, BlockID_Air);
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
	Game_UpdateBlock(pos.X, pos.Y, pos.Z, BlockID_Air);
	Event_RaiseBlock(&UserEvents_BlockChanged, pos, old, BlockID_Air);
}

void GameMode_PickMiddle(BlockID old) {
	if (Block_Draw[old] == DrawType_Gas) return;
	if (!(Block_CanPlace[old] || Block_CanDelete[old])) return;
	if (!Inventory_CanChangeSelected() || Inventory_SelectedBlock == old) return;
	UInt32 i;

	/* Is the currently selected block an empty slot */
	if (Inventory_Get(Inventory_SelectedIndex) == BlockID_Air) {
		Inventory_SetSelectedBlock(old); return;
	}

	/* Try to replace same block */	
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		if (Inventory_Get(i) != old) continue;
		Inventory_SetSelectedIndex(i); return;
	}

	/* Try to replace empty slots */
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		if (Inventory_Get(i) != BlockID_Air) continue;
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