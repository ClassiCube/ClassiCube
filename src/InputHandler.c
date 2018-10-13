#include "InputHandler.h"
#include "Utils.h"
#include "ServerConnection.h"
#include "HeldBlockRenderer.h"
#include "Game.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Camera.h"
#include "Inventory.h"
#include "World.h"
#include "Event.h"
#include "Window.h"
#include "Entity.h"
#include "Chat.h"
#include "Funcs.h"
#include "Screens.h"
#include "Block.h"
#include "Menus.h"
#include "Gui.h"

bool input_buttonsDown[3];
int input_pickingId = -1;
int16_t input_normViewDists[10] = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
int16_t input_classicViewDists[4] = { 8, 32, 128, 512 };
TimeMS input_lastClick;
float input_fovIndex = -1.0f;

bool InputHandler_IsMousePressed(MouseButton button) {
	if (Mouse_Pressed[button]) return true;

	/* Key --> mouse mappings */
	if (button == MouseButton_Left   && KeyBind_IsPressed(KeyBind_MouseLeft))   return true;
	if (button == MouseButton_Middle && KeyBind_IsPressed(KeyBind_MouseMiddle)) return true;
	if (button == MouseButton_Right  && KeyBind_IsPressed(KeyBind_MouseRight))  return true;
	return false;
}

static void InputHandler_ButtonStateUpdate(MouseButton button, bool pressed) {
	/* defer getting the targeted entity as it's a costly operation */
	if (input_pickingId == -1) {
		struct Entity* p = &LocalPlayer_Instance.Base;
		input_pickingId = Entities_GetCloset(p);
	}

	EntityID id = (EntityID)input_pickingId;
	ServerConnection_SendPlayerClick(button, pressed, id, &Game_SelectedPos);
	input_buttonsDown[button] = pressed;
}

static void InputHandler_ButtonStateChanged(MouseButton button, bool pressed) {
	if (pressed) {
		/* Can send multiple Pressed events */
		InputHandler_ButtonStateUpdate(button, true);
	} else {
		if (!input_buttonsDown[button]) return;
		InputHandler_ButtonStateUpdate(button, false);
	}
}

void InputHandler_ScreenChanged(struct Screen* oldScreen, struct Screen* newScreen) {
	if (oldScreen && oldScreen->HandlesAllInput) {
		input_lastClick = DateTime_CurrentUTC_MS();
	}

	if (ServerConnection_SupportsPlayerClick) {
		input_pickingId = -1;
		InputHandler_ButtonStateChanged(MouseButton_Left,   false);
		InputHandler_ButtonStateChanged(MouseButton_Right,  false);
		InputHandler_ButtonStateChanged(MouseButton_Middle, false);
	}
}

static bool InputHandler_IsShutdown(Key key) {
	if (key == Key_F4 && Key_IsAltPressed()) return true;

	/* On OSX, Cmd+Q should also terminate the process. */
#ifdef CC_BUILD_OSX
	return key == Key_Q && Key_IsWinPressed();
#else
	return false;
#endif
}

static void InputHandler_Toggle(Key key, bool* target, const char* enableMsg, const char* disableMsg) {
	*target = !(*target);
	if (*target) {
		Chat_Add2("%c. &ePress &a%c &eto disable.",   enableMsg,  Key_Names[key]);
	} else {
		Chat_Add2("%c. &ePress &a%c &eto re-enable.", disableMsg, Key_Names[key]);
	}
}

static void InputHandler_CycleDistanceForwards(int16_t* viewDists, int count) {
	int i;
	for (i = 0; i < count; i++) {
		int dist = viewDists[i];
		if (dist > Game_UserViewDistance) {
			Game_UserSetViewDistance(dist); return;
		}
	}
	Game_UserSetViewDistance(viewDists[0]);
}

static void InputHandler_CycleDistanceBackwards(int16_t* viewDists, int count) {
	int i;
	for (i = count - 1; i >= 0; i--) {
		int dist = viewDists[i];
		if (dist < Game_UserViewDistance) {
			Game_UserSetViewDistance(dist); return;
		}
	}
	Game_UserSetViewDistance(viewDists[count - 1]);
}

bool InputHandler_SetFOV(int fov, bool setZoom) {
	if (Game_Fov == fov) return true;
	struct HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanAnyHacks || !h->CanUseThirdPersonCamera) return false;

	Game_Fov = fov;
	if (setZoom) Game_ZoomFov = fov;
	Game_UpdateProjection();
	return true;
}

static bool InputHandler_DoFovZoom(float deltaPrecise) {
	if (!KeyBind_IsPressed(KeyBind_ZoomScrolling)) return false;
	struct HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanAnyHacks || !h->CanUseThirdPersonCamera) return false;

	if (input_fovIndex == -1.0f) input_fovIndex = (float)Game_ZoomFov;
	input_fovIndex -= deltaPrecise * 5.0f;

	Math_Clamp(input_fovIndex, 1.0f, Game_DefaultFov);
	return InputHandler_SetFOV((int)input_fovIndex, true);
}

static bool InputHandler_HandleNonClassicKey(Key key) {
	if (key == KeyBind_Get(KeyBind_HideGui)) {
		Game_HideGui = !Game_HideGui;
	} else if (key == KeyBind_Get(KeyBind_SmoothCamera)) {
		InputHandler_Toggle(key, &Game_SmoothCamera,
			"  &eSmooth camera is &aenabled",
			"  &eSmooth camera is &cdisabled");
	} else if (key == KeyBind_Get(KeyBind_AxisLines)) {
		InputHandler_Toggle(key, &Game_ShowAxisLines,
			"  &eAxis lines (&4X&e, &2Y&e, &1Z&e) now show",
			"  &eAxis lines no longer show");
	} else if (key == KeyBind_Get(KeyBind_Autorotate)) {
		InputHandler_Toggle(key, &Game_AutoRotate,
			"  &eAuto rotate is &aenabled",
			"  &eAuto rotate is &cdisabled");
	} else if (key == KeyBind_Get(KeyBind_ThirdPerson)) {
		Camera_CycleActive();
	} else if (key == KeyBind_Get(KeyBind_DropBlock)) {
		if (Inventory_CanChangeSelected() && Inventory_SelectedBlock != BLOCK_AIR) {
			/* Don't assign SelectedIndex directly, because we don't want held block
			switching positions if they already have air in their inventory hotbar. */
			Inventory_Set(Inventory_SelectedIndex, BLOCK_AIR);
			Event_RaiseVoid(&UserEvents_HeldBlockChanged);
		}
	} else if (key == KeyBind_Get(KeyBind_IDOverlay)) {
		if (Gui_OverlaysCount) return true;
		struct Screen* overlay = TexIdsOverlay_MakeInstance();
		Gui_ShowOverlay(overlay, false);
	} else if (key == KeyBind_Get(KeyBind_BreakableLiquids)) {
		InputHandler_Toggle(key, &Game_BreakableLiquids,
			"  &eBreakable liquids is &aenabled",
			"  &eBreakable liquids is &cdisabled");
	} else {
		return false;
	}
	return true;
}

static bool InputHandler_HandleCoreKey(Key key) {
	struct Screen* activeScreen = Gui_GetActiveScreen();

	if (key == KeyBind_Get(KeyBind_HideFps)) {
		Game_ShowFPS = !Game_ShowFPS;
	} else if (key == KeyBind_Get(KeyBind_Fullscreen)) {
		uint8_t state = Window_GetWindowState();
		if (state != WINDOW_STATE_MAXIMISED) {
			bool fullscreen = state == WINDOW_STATE_FULLSCREEN;
			Window_SetWindowState(fullscreen ? WINDOW_STATE_NORMAL : WINDOW_STATE_FULLSCREEN);
		}
	} else if (key == KeyBind_Get(KeyBind_ToggleFog)) {
		int16_t* viewDists = Game_UseClassicOptions ? input_classicViewDists : input_normViewDists;
		int count = Game_UseClassicOptions ? Array_Elems(input_classicViewDists) : Array_Elems(input_normViewDists);

		if (Key_IsShiftPressed()) {
			InputHandler_CycleDistanceBackwards(viewDists, count);
		} else {
			InputHandler_CycleDistanceForwards(viewDists, count);
		}
	} else if ((key == KeyBind_Get(KeyBind_PauseOrExit) || key == Key_Pause) && !activeScreen->HandlesAllInput) {
		Gui_FreeActive();
		Gui_SetActive(PauseScreen_MakeInstance());
	} else if (key == KeyBind_Get(KeyBind_Inventory) && activeScreen == Gui_HUD) {
		Gui_FreeActive();
		Gui_SetActive(InventoryScreen_MakeInstance());
	} else if (key == Key_F5 && Game_ClassicMode) {
		int weather = Env_Weather == WEATHER_SUNNY ? WEATHER_RAINY : WEATHER_SUNNY;
		Env_SetWeather(weather);
	} else {
		if (Game_ClassicMode) return false;
		return InputHandler_HandleNonClassicKey(key);
	}
	return true;
}

static bool InputHandler_TouchesSolid(BlockID b) { return Block_Collide[b] == COLLIDE_SOLID; }
static bool InputHandler_PushbackPlace(struct AABB* blockBB) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	Vector3 curPos = p->Position, adjPos = p->Position;

	/* Offset position by the closest face */
	Face closestFace = Game_SelectedPos.ClosestFace;
	if (closestFace == FACE_XMAX) {
		adjPos.X = blockBB->Max.X + 0.5f;
	} else if (closestFace == FACE_ZMAX) {
		adjPos.Z = blockBB->Max.Z + 0.5f;
	} else if (closestFace == FACE_XMIN) {
		adjPos.X = blockBB->Min.X - 0.5f;
	} else if (closestFace == FACE_ZMIN) {
		adjPos.Z = blockBB->Min.Z - 0.5f;
	} else if (closestFace == FACE_YMAX) {
		adjPos.Y = blockBB->Min.Y + 1 + ENTITY_ADJUSTMENT;
	} else if (closestFace == FACE_YMIN) {
		adjPos.Y = blockBB->Min.Y - p->Size.Y - ENTITY_ADJUSTMENT;
	}

	/* exclude exact map boundaries, otherwise player can get stuck outside map */
	bool validPos =
		adjPos.X > 0.0f && adjPos.Y >= 0.0f && adjPos.Z > 0.0f &&
		adjPos.X < World_Width && adjPos.Z < World_Length;
	if (!validPos) return false;

	p->Position = adjPos;
	struct AABB bounds; Entity_GetBounds(p, &bounds);
	if (!hacks->Noclip && Entity_TouchesAny(&bounds, InputHandler_TouchesSolid)) {
		p->Position = curPos;
		return false;
	}

	p->Position = curPos;
	struct LocationUpdate update; LocationUpdate_MakePos(&update, adjPos, false);
	p->VTABLE->SetLocation(p, &update, false);
	return true;
}

static bool InputHandler_IntersectsOthers(Vector3 pos, BlockID block) {
	struct AABB blockBB;
	Vector3_Add(&blockBB.Min, &pos, &Block_MinBB[block]);
	Vector3_Add(&blockBB.Max, &pos, &Block_MaxBB[block]);

	int id;
	for (id = 0; id < ENTITIES_SELF_ID; id++) {
		struct Entity* entity = Entities_List[id];
		if (!entity) continue;

		struct AABB bounds; Entity_GetBounds(entity, &bounds);
		bounds.Min.Y += 1.0f / 32.0f; /* when player is exactly standing on top of ground */
		if (AABB_Intersects(&bounds, &blockBB)) return true;
	}
	return false;
}

static bool InputHandler_CheckIsFree(BlockID block) {
	Vector3 pos; Vector3I_ToVector3(&pos, &Game_SelectedPos.TranslatedPos);
	struct Entity* p = &LocalPlayer_Instance.Base;
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;

	if (Block_Collide[block] != COLLIDE_SOLID) return true;
	if (InputHandler_IntersectsOthers(pos, block)) return false;
	Vector3 nextPos = LocalPlayer_Instance.Interp.Next.Pos;

	struct AABB blockBB;
	Vector3_Add(&blockBB.Min, &pos, &Block_MinBB[block]);
	Vector3_Add(&blockBB.Max, &pos, &Block_MaxBB[block]);

	/* NOTE: Need to also test against nextPos here, otherwise player can 
	fall through the block at feet as collision is performed against nextPos */
	struct AABB localBB; AABB_Make(&localBB, &p->Position, &p->Size);
	localBB.Min.Y = min(nextPos.Y, localBB.Min.Y);

	if (hacks->Noclip || !AABB_Intersects(&localBB, &blockBB)) return true;
	if (hacks->CanPushbackBlocks && hacks->PushbackPlacing && hacks->Enabled) {
		return InputHandler_PushbackPlace(&blockBB);
	}

	localBB.Min.Y += 0.25f + ENTITY_ADJUSTMENT;
	if (AABB_Intersects(&localBB, &blockBB)) return false;

	/* Push player upwards when they are jumping and trying to place a block underneath them */
	nextPos.Y = pos.Y + Block_MaxBB[block].Y + ENTITY_ADJUSTMENT;
	struct LocationUpdate update; LocationUpdate_MakePos(&update, nextPos, false);
	p->VTABLE->SetLocation(p, &update, false);
	return true;
}

void InputHandler_PickBlocks(bool cooldown, bool left, bool middle, bool right) {
	TimeMS now = DateTime_CurrentUTC_MS();
	int delta = (int)(now - input_lastClick);
	if (cooldown && delta < 250) return; /* 4 times per second */

	input_lastClick = now;
	if (ServerConnection_SupportsPlayerClick && !Gui_GetActiveScreen()->HandlesAllInput) {
		input_pickingId = -1;
		InputHandler_ButtonStateChanged(MouseButton_Left,   left);
		InputHandler_ButtonStateChanged(MouseButton_Right,  right);
		InputHandler_ButtonStateChanged(MouseButton_Middle, middle);
	}

	if (Gui_GetActiveScreen()->HandlesAllInput || !Inventory_CanPick) return;

	if (left) {
		/* always play delete animations, even if we aren't picking a block */
		HeldBlockRenderer_ClickAnim(true);

		Vector3I p = Game_SelectedPos.BlockPos;
		if (!Game_SelectedPos.Valid || !World_IsValidPos_3I(p)) return;

		BlockID old = World_GetBlock(p.X, p.Y, p.Z);
		if (Block_Draw[old] == DRAW_GAS || !Block_CanDelete[old]) return;

		Game_UpdateBlock(p.X, p.Y, p.Z, BLOCK_AIR);
		Event_RaiseBlock(&UserEvents_BlockChanged, p, old, BLOCK_AIR);
	} else if (right) {
		Vector3I p = Game_SelectedPos.TranslatedPos;
		if (!Game_SelectedPos.Valid || !World_IsValidPos_3I(p)) return;

		BlockID old = World_GetBlock(p.X, p.Y, p.Z);
		BlockID block = Inventory_SelectedBlock;
		if (Game_AutoRotate) { block = AutoRotate_RotateBlock(block); }

		if (Game_CanPick(old) || !Block_CanPlace[block]) return;
		/* air-ish blocks can only replace over other air-ish blocks */
		if (Block_Draw[block] == DRAW_GAS && Block_Draw[old] != DRAW_GAS) return;
		if (!InputHandler_CheckIsFree(block)) return;

		Game_UpdateBlock(p.X, p.Y, p.Z, block);
		Event_RaiseBlock(&UserEvents_BlockChanged, p, old, block);
	} else if (middle) {
		Vector3I p = Game_SelectedPos.BlockPos;
		if (!Game_SelectedPos.Valid || !World_IsValidPos_3I(p)) return;

		BlockID cur = World_GetBlock(p.X, p.Y, p.Z);
		if (Block_Draw[cur] == DRAW_GAS) return;
		if (!(Block_CanPlace[cur] || Block_CanDelete[cur])) return;
		if (!Inventory_CanChangeSelected() || Inventory_SelectedBlock == cur) return;
		int i;

		/* Is the currently selected block an empty slot? */
		if (Inventory_SelectedBlock == BLOCK_AIR) {
			Inventory_SetSelectedBlock(cur); return;
		}

		/* Try to replace same block */
		for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
			if (Inventory_Get(i) != cur) continue;
			Inventory_SetSelectedIndex(i); return;
		}

		/* Try to replace empty slots */
		for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
			if (Inventory_Get(i) != BLOCK_AIR) continue;
			Inventory_Set(i, cur);
			Inventory_SetSelectedIndex(i); return;
		}

		/* Finally, replace the currently selected block */
		Inventory_SetSelectedBlock(cur);
	}
}

static void InputHandler_MouseWheel(void* obj, float delta) {
	struct Screen* active = Gui_GetActiveScreen();
	if (Elem_HandlesMouseScroll(active, delta)) return;

	bool hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (!hotbar && Camera_Active->Zoom(delta)) return;
	if (InputHandler_DoFovZoom(delta) || !Inventory_CanChangeHeldBlock) return;

	struct Widget* hotbarW = HUDScreen_GetHotbar(Gui_HUD);
	Elem_HandlesMouseScroll(hotbarW, delta);
}

static void InputHandler_MouseMove(void* obj, int xDelta, int yDelta) {
	struct Screen* active = Gui_GetActiveScreen();
	Elem_HandlesMouseMove(active, Mouse_X, Mouse_Y);
}

static void InputHandler_MouseDown(void* obj, int button) {
	struct Screen* active = Gui_GetActiveScreen();
	if (!Elem_HandlesMouseDown(active, Mouse_X, Mouse_Y, button)) {
		bool left   = button == MouseButton_Left;
		bool middle = button == MouseButton_Middle;
		bool right  = button == MouseButton_Right;
		InputHandler_PickBlocks(false, left, middle, right);
	} else {
		input_lastClick = DateTime_CurrentUTC_MS();
	}
}

static void InputHandler_MouseUp(void* obj, int button) {
	struct Screen* active = Gui_GetActiveScreen();
	if (!Elem_HandlesMouseUp(active, Mouse_X, Mouse_Y, button)) {
		if (ServerConnection_SupportsPlayerClick && button <= MouseButton_Middle) {
			input_pickingId = -1;
			InputHandler_ButtonStateChanged(button, false);
		}
	}
}

static bool InputHandler_SimulateMouse(Key key, bool pressed) {
	Key left   = KeyBind_Get(KeyBind_MouseLeft);
	Key middle = KeyBind_Get(KeyBind_MouseMiddle);
	Key right  = KeyBind_Get(KeyBind_MouseRight);
	if (!(key == left || key == middle || key == right)) return false;

	MouseButton btn = key == left ? MouseButton_Left : key == middle ? MouseButton_Middle : MouseButton_Right;
	if (pressed) { InputHandler_MouseDown(NULL, btn); }
	else {         InputHandler_MouseUp(NULL,   btn); }
	return true;
}

static void InputHandler_KeyDown(void* obj, int key) {
	if (InputHandler_SimulateMouse(key, true)) return;
	struct Screen* active = Gui_GetActiveScreen();

	if (InputHandler_IsShutdown(key)) {
		/* TODO: Do we need a separate exit function in Game class? */
		Window_Close();
	} else if (key == KeyBind_Get(KeyBind_Screenshot)) {
		Game_ScreenshotRequested = true;
	} else if (Elem_HandlesKeyDown(active, key)) {
	} else if (InputHandler_HandleCoreKey(key)) {
	} else if (LocalPlayer_HandlesKey(key)) {
	} else {
		int idx = Hotkeys_FindPartial(key);
		if (idx == -1) return;

		struct HotkeyData* hkey = &HotkeysList[idx];
		String text = StringsBuffer_UNSAFE_Get(&HotkeysText, hkey->TextIndex);

		if (!hkey->StaysOpen) {
			Chat_Send(&text, false);
		} else if (!Gui_Active) {
			HUDScreen_OpenInput(Gui_HUD, &text);
		}
	}
}

static void InputHandler_KeyUp(void* obj, int key) {
	if (InputHandler_SimulateMouse(key, false)) return;

	if (key == KeyBind_Get(KeyBind_ZoomScrolling)) {
		InputHandler_SetFOV(Game_DefaultFov, false);
	}

	struct Screen* active = Gui_GetActiveScreen();
	Elem_HandlesKeyUp(active, key);
}

static void InputHandler_KeyPress(void* obj, int keyChar) {
	struct Screen* active = Gui_GetActiveScreen();
	Elem_HandlesKeyPress(active, keyChar);
}

void InputHandler_Init(void) {
	Event_RegisterFloat(&MouseEvents_Wheel,      NULL, InputHandler_MouseWheel);
	Event_RegisterMouseMove(&MouseEvents_Moved, NULL, InputHandler_MouseMove);
	Event_RegisterInt(&MouseEvents_Down,        NULL, InputHandler_MouseDown);
	Event_RegisterInt(&MouseEvents_Up,          NULL, InputHandler_MouseUp);
	Event_RegisterInt(&KeyEvents_Down,          NULL, InputHandler_KeyDown);
	Event_RegisterInt(&KeyEvents_Up,            NULL, InputHandler_KeyUp);
	Event_RegisterInt(&KeyEvents_Press,         NULL, InputHandler_KeyPress);

	KeyBind_Init();
	Hotkeys_Init();
}
