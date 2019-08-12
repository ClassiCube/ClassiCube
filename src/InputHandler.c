#include "InputHandler.h"
#include "Utils.h"
#include "Server.h"
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
#include "Protocol.h"
#include "AxisLinesRenderer.h"

static bool input_buttonsDown[3];
static int input_pickingId = -1;
static short normViewDists[10]   = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
static short classicViewDists[4] = { 8, 32, 128, 512 };
static TimeMS input_lastClick;
static float input_fovIndex = -1.0f;
#ifdef CC_BUILD_WEB
static bool suppressEscape;
#endif

bool InputHandler_IsMousePressed(MouseButton button) {
	if (Mouse_Pressed[button]) return true;

	/* Key --> mouse mappings */
	if (button == MOUSE_LEFT   && KeyBind_IsPressed(KEYBIND_MOUSE_LEFT))   return true;
	if (button == MOUSE_MIDDLE && KeyBind_IsPressed(KEYBIND_MOUSE_MIDDLE)) return true;
	if (button == MOUSE_RIGHT  && KeyBind_IsPressed(KEYBIND_MOUSE_RIGHT))  return true;
	return false;
}

static void InputHandler_ButtonStateUpdate(MouseButton button, bool pressed) {
	struct Entity* p;
	/* defer getting the targeted entity, as it's a costly operation */
	if (input_pickingId == -1) {
		p = &LocalPlayer_Instance.Base;
		input_pickingId = Entities_GetCloset(p);
	}

	input_buttonsDown[button] = pressed;
	CPE_SendPlayerClick(button, pressed, (EntityID)input_pickingId, &Game_SelectedPos);	
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
	if (oldScreen && oldScreen->grabsInput) {
		input_lastClick = DateTime_CurrentUTC_MS();
	}

	if (Server.SupportsPlayerClick) {
		input_pickingId = -1;
		InputHandler_ButtonStateChanged(MOUSE_LEFT,   false);
		InputHandler_ButtonStateChanged(MOUSE_RIGHT,  false);
		InputHandler_ButtonStateChanged(MOUSE_MIDDLE, false);
	}
}

static bool InputHandler_IsShutdown(Key key) {
	if (key == KEY_F4 && Key_IsAltPressed()) return true;

	/* On OSX, Cmd+Q should also terminate the process */
#ifdef CC_BUILD_OSX
	return key == 'Q' && Key_IsWinPressed();
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

static void InputHandler_CycleDistanceForwards(const short* viewDists, int count) {
	int i, dist;
	for (i = 0; i < count; i++) {
		dist = viewDists[i];

		if (dist > Game_UserViewDistance) {
			Game_UserSetViewDistance(dist); return;
		}
	}
	Game_UserSetViewDistance(viewDists[0]);
}

static void InputHandler_CycleDistanceBackwards(const short* viewDists, int count) {
	int i, dist;
	for (i = count - 1; i >= 0; i--) {
		dist = viewDists[i];

		if (dist < Game_UserViewDistance) {
			Game_UserSetViewDistance(dist); return;
		}
	}
	Game_UserSetViewDistance(viewDists[count - 1]);
}

bool InputHandler_SetFOV(int fov) {
	struct HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanUseThirdPersonCamera) return false;

	Game_ZoomFov = fov;
	Game_SetFov(fov);
	return true;
}

static bool InputHandler_DoFovZoom(float deltaPrecise) {
	struct HacksComp* h;
	if (!KeyBind_IsPressed(KEYBIND_ZOOM_SCROLL)) return false;

	h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanUseThirdPersonCamera) return false;

	if (input_fovIndex == -1.0f) input_fovIndex = (float)Game_ZoomFov;
	input_fovIndex -= deltaPrecise * 5.0f;

	Math_Clamp(input_fovIndex, 1.0f, Game_DefaultFov);
	return InputHandler_SetFOV((int)input_fovIndex);
}

static void InputHandler_CheckZoomFov(void* obj) {
	struct HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanUseThirdPersonCamera) Game_SetFov(Game_DefaultFov);
}

static bool InputHandler_HandleNonClassicKey(Key key) {
	if (key == KeyBinds[KEYBIND_HIDE_GUI]) {
		Game_HideGui = !Game_HideGui;
	} else if (key == KeyBinds[KEYBIND_SMOOTH_CAMERA]) {
		InputHandler_Toggle(key, &Camera.Smooth,
			"  &eSmooth camera is &aenabled",
			"  &eSmooth camera is &cdisabled");
	} else if (key == KeyBinds[KEYBIND_AXIS_LINES]) {
		InputHandler_Toggle(key, &AxisLinesRenderer_Enabled,
			"  &eAxis lines (&4X&e, &2Y&e, &1Z&e) now show",
			"  &eAxis lines no longer show");
	} else if (key == KeyBinds[KEYBIND_AUTOROTATE]) {
		InputHandler_Toggle(key, &AutoRotate_Enabled,
			"  &eAuto rotate is &aenabled",
			"  &eAuto rotate is &cdisabled");
	} else if (key == KeyBinds[KEYBIND_THIRD_PERSON]) {
		Camera_CycleActive();
	} else if (key == KeyBinds[KEYBIND_DROP_BLOCK]) {
		if (Inventory_CheckChangeSelected() && Inventory_SelectedBlock != BLOCK_AIR) {
			/* Don't assign SelectedIndex directly, because we don't want held block
			switching positions if they already have air in their inventory hotbar. */
			Inventory_Set(Inventory.SelectedIndex, BLOCK_AIR);
			Event_RaiseVoid(&UserEvents.HeldBlockChanged);
		}
	} else if (key == KeyBinds[KEYBIND_IDOVERLAY]) {
		if (Gui_OverlaysCount) return true;
		Gui_ShowOverlay(TexIdsOverlay_MakeInstance());
	} else if (key == KeyBinds[KEYBIND_BREAK_LIQUIDS]) {
		InputHandler_Toggle(key, &Game_BreakableLiquids,
			"  &eBreakable liquids is &aenabled",
			"  &eBreakable liquids is &cdisabled");
	} else {
		return false;
	}
	return true;
}

static bool InputHandler_HandleCoreKey(Key key) {
	struct Screen* active = Gui_GetActiveScreen();

	if (key == KeyBinds[KEYBIND_HIDE_FPS]) {
		Gui_ShowFPS = !Gui_ShowFPS;
	} else if (key == KeyBinds[KEYBIND_FULLSCREEN]) {
		int state = Window_GetWindowState();

		if (state == WINDOW_STATE_FULLSCREEN) {
			Window_ExitFullscreen();
		} else if (state != WINDOW_STATE_MINIMISED) {
			Window_EnterFullscreen();
		}
	} else if (key == KeyBinds[KEYBIND_FOG]) {
		short* viewDists = Gui_ClassicMenu ? classicViewDists : normViewDists;
		int count = Gui_ClassicMenu ? Array_Elems(classicViewDists) : Array_Elems(normViewDists);

		if (Key_IsShiftPressed()) {
			InputHandler_CycleDistanceBackwards(viewDists, count);
		} else {
			InputHandler_CycleDistanceForwards(viewDists, count);
		}
	} else if (key == KeyBinds[KEYBIND_INVENTORY] && active == Gui_HUD) {
		InventoryScreen_Show();
	} else if (key == KEY_F5 && Game_ClassicMode) {
		int weather = Env.Weather == WEATHER_SUNNY ? WEATHER_RAINY : WEATHER_SUNNY;
		Env_SetWeather(weather);
	} else {
		if (Game_ClassicMode) return false;
		return InputHandler_HandleNonClassicKey(key);
	}
	return true;
}

static bool InputHandler_TouchesSolid(BlockID b) { return Blocks.Collide[b] == COLLIDE_SOLID; }
static bool InputHandler_PushbackPlace(struct AABB* blockBB) {
	struct Entity* p        = &LocalPlayer_Instance.Base;
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	Face closestFace;
	bool insideMap;

	Vec3 pos = p->Position;
	struct AABB playerBB;
	struct LocationUpdate update;

	/* Offset position by the closest face */
	closestFace = Game_SelectedPos.Closest;
	if (closestFace == FACE_XMAX) {
		pos.X = blockBB->Max.X + 0.5f;
	} else if (closestFace == FACE_ZMAX) {
		pos.Z = blockBB->Max.Z + 0.5f;
	} else if (closestFace == FACE_XMIN) {
		pos.X = blockBB->Min.X - 0.5f;
	} else if (closestFace == FACE_ZMIN) {
		pos.Z = blockBB->Min.Z - 0.5f;
	} else if (closestFace == FACE_YMAX) {
		pos.Y = blockBB->Min.Y + 1 + ENTITY_ADJUSTMENT;
	} else if (closestFace == FACE_YMIN) {
		pos.Y = blockBB->Min.Y - p->Size.Y - ENTITY_ADJUSTMENT;
	}

	/* Exclude exact map boundaries, otherwise player can get stuck outside map */
	/* Being vertically above the map is acceptable though */
	insideMap =
		pos.X > 0.0f && pos.Y >= 0.0f && pos.Z > 0.0f &&
		pos.X < World.Width && pos.Z < World.Length;
	if (!insideMap) return false;

	AABB_Make(&playerBB, &pos, &p->Size);
	if (!hacks->Noclip && Entity_TouchesAny(&playerBB, InputHandler_TouchesSolid)) {
		/* Don't put player inside another block */
		return false;
	}

	LocationUpdate_MakePos(&update, pos, false);
	p->VTABLE->SetLocation(p, &update, false);
	return true;
}

static bool InputHandler_IntersectsOthers(Vec3 pos, BlockID block) {
	struct AABB blockBB, entityBB;
	struct Entity* entity;
	int id;

	Vec3_Add(&blockBB.Min, &pos, &Blocks.MinBB[block]);
	Vec3_Add(&blockBB.Max, &pos, &Blocks.MaxBB[block]);
	
	for (id = 0; id < ENTITIES_SELF_ID; id++) {
		entity = Entities.List[id];
		if (!entity) continue;

		Entity_GetBounds(entity, &entityBB);
		entityBB.Min.Y += 1.0f / 32.0f; /* when player is exactly standing on top of ground */
		if (AABB_Intersects(&entityBB, &blockBB)) return true;
	}
	return false;
}

static bool InputHandler_CheckIsFree(BlockID block) {
	struct Entity* p        = &LocalPlayer_Instance.Base;
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;

	Vec3 pos, nextPos;
	struct AABB blockBB, playerBB;
	struct LocationUpdate update;

	/* Non solid blocks (e.g. water/flowers) can always be placed on players */
	if (Blocks.Collide[block] != COLLIDE_SOLID) return true;

	IVec3_ToVec3(&pos, &Game_SelectedPos.TranslatedPos);
	if (InputHandler_IntersectsOthers(pos, block)) return false;
	
	nextPos = LocalPlayer_Instance.Interp.Next.Pos;
	Vec3_Add(&blockBB.Min, &pos, &Blocks.MinBB[block]);
	Vec3_Add(&blockBB.Max, &pos, &Blocks.MaxBB[block]);

	/* NOTE: Need to also test against next position here, otherwise player can 
	fall through the block at feet as collision is performed against nextPos */
	Entity_GetBounds(p, &playerBB);
	playerBB.Min.Y = min(nextPos.Y, playerBB.Min.Y);

	if (hacks->Noclip || !AABB_Intersects(&playerBB, &blockBB)) return true;
	if (hacks->CanPushbackBlocks && hacks->PushbackPlacing && hacks->Enabled) {
		return InputHandler_PushbackPlace(&blockBB);
	}

	playerBB.Min.Y += 0.25f + ENTITY_ADJUSTMENT;
	if (AABB_Intersects(&playerBB, &blockBB)) return false;

	/* Push player upwards when they are jumping and trying to place a block underneath them */
	nextPos.Y = pos.Y + Blocks.MaxBB[block].Y + ENTITY_ADJUSTMENT;
	LocationUpdate_MakePos(&update, nextPos, false);
	p->VTABLE->SetLocation(p, &update, false);
	return true;
}

void InputHandler_PickBlocks(bool cooldown, bool left, bool middle, bool right) {
	TimeMS now = DateTime_CurrentUTC_MS();
	int delta  = (int)(now - input_lastClick);

	IVec3 pos;
	BlockID old, cur, block;
	int i;

	if (cooldown && delta < 250) return; /* 4 times per second */
	input_lastClick = now;
	if (Gui_GetInputGrab()) return;

	if (Server.SupportsPlayerClick) {
		input_pickingId = -1;
		InputHandler_ButtonStateChanged(MOUSE_LEFT,   left);
		InputHandler_ButtonStateChanged(MOUSE_RIGHT,  right);
		InputHandler_ButtonStateChanged(MOUSE_MIDDLE, middle);
	}

	if (left) {
		/* always play delete animations, even if we aren't picking a block */
		HeldBlockRenderer_ClickAnim(true);

		pos = Game_SelectedPos.BlockPos;
		if (!Game_SelectedPos.Valid || !World_Contains(pos.X, pos.Y, pos.Z)) return;

		old = World_GetBlock(pos.X, pos.Y, pos.Z);
		if (Blocks.Draw[old] == DRAW_GAS || !Blocks.CanDelete[old]) return;

		Game_ChangeBlock(pos.X, pos.Y, pos.Z, BLOCK_AIR);
		Event_RaiseBlock(&UserEvents.BlockChanged, pos, old, BLOCK_AIR);
	} else if (right) {
		pos = Game_SelectedPos.TranslatedPos;
		if (!Game_SelectedPos.Valid || !World_Contains(pos.X, pos.Y, pos.Z)) return;

		old   = World_GetBlock(pos.X, pos.Y, pos.Z);
		block = Inventory_SelectedBlock;
		if (AutoRotate_Enabled) block = AutoRotate_RotateBlock(block);

		if (Game_CanPick(old) || !Blocks.CanPlace[block]) return;
		/* air-ish blocks can only replace over other air-ish blocks */
		if (Blocks.Draw[block] == DRAW_GAS && Blocks.Draw[old] != DRAW_GAS) return;
		if (!InputHandler_CheckIsFree(block)) return;

		Game_ChangeBlock(pos.X, pos.Y, pos.Z, block);
		Event_RaiseBlock(&UserEvents.BlockChanged, pos, old, block);
	} else if (middle) {
		pos = Game_SelectedPos.BlockPos;
		if (!World_Contains(pos.X, pos.Y, pos.Z)) return;

		cur = World_GetBlock(pos.X, pos.Y, pos.Z);
		if (Blocks.Draw[cur] == DRAW_GAS) return;
		if (!(Blocks.CanPlace[cur] || Blocks.CanDelete[cur])) return;
		if (!Inventory_CheckChangeSelected() || Inventory_SelectedBlock == cur) return;

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
	struct Widget* widget;
	bool hotbar;
	if (Elem_HandlesMouseScroll(active, delta)) return;

	hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (!hotbar && Camera.Active->Zoom(delta)) return;
	if (InputHandler_DoFovZoom(delta) || !Inventory.CanChangeSelected) return;

	widget = HUDScreen_GetHotbar();
	Elem_HandlesMouseScroll(widget, delta);
}

static void InputHandler_MouseMove(void* obj, int xDelta, int yDelta) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		if (Elem_HandlesMouseMove(s, Mouse_X, Mouse_Y)) return;
	}
}

static void InputHandler_MouseDown(void* obj, int btn) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		if (Elem_HandlesMouseDown(s, Mouse_X, Mouse_Y, btn)) {
			input_lastClick = DateTime_CurrentUTC_MS(); return;
		}
	}

	InputHandler_PickBlocks(false, btn == MOUSE_LEFT, 
			  btn == MOUSE_MIDDLE, btn == MOUSE_RIGHT);
}

static void InputHandler_MouseUp(void* obj, int btn) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		if (Elem_HandlesMouseUp(s, Mouse_X, Mouse_Y, btn)) return;
	}

	if (Server.SupportsPlayerClick && btn <= MOUSE_MIDDLE) {
		input_pickingId = -1;
		InputHandler_ButtonStateChanged(btn, false);
	}
}

static bool InputHandler_SimulateMouse(Key key, bool pressed) {
	Key left   = KeyBinds[KEYBIND_MOUSE_LEFT];
	Key middle = KeyBinds[KEYBIND_MOUSE_MIDDLE];
	Key right  = KeyBinds[KEYBIND_MOUSE_RIGHT];
	MouseButton btn;
	if (!(key == left || key == middle || key == right)) return false;

	btn = key == left ? MOUSE_LEFT : key == middle ? MOUSE_MIDDLE : MOUSE_RIGHT;
	if (pressed) { InputHandler_MouseDown(NULL, btn); }
	else {         InputHandler_MouseUp(NULL,   btn); }
	return true;
}

static void InputHandler_KeyDown(void* obj, int key, bool was) {
	struct Screen* active;
	int idx;
	struct HotkeyData* hkey;
	String text;

	if (!was && InputHandler_SimulateMouse(key, true)) return;
	active = Gui_GetActiveScreen();

#ifndef CC_BUILD_WEB
	if (key == KEY_ESCAPE && active->closable) {
		/* Don't want holding down escape to go in and out of pause menu */
		if (!was) Gui_Close(active); 
		return;
	}
#endif

	if (InputHandler_IsShutdown(key)) {
		/* TODO: Do we need a separate exit function in Game class? */
		Window_Close(); return;
	} else if (key == KeyBinds[KEYBIND_SCREENSHOT] && !was) {
		Game_ScreenshotRequested = true; return;
	} else if (Elem_HandlesKeyDown(active, key, was)) {
		return;
	} else if ((key == KEY_ESCAPE || key == KEY_PAUSE) && !Gui_GetInputGrab()) {
#ifdef CC_BUILD_WEB
		/* Can't do this in KeyUp, because pressing escape without having */
		/* explicitly disabled mouse lock means a KeyUp event isn't sent. */
		/* But switching to pause screen disables mouse lock, causing a KeyUp */
		/* event to be sent, triggering the active->closable case which immediately
		/* closes the pause screen. Hence why the next KeyUp must be supressed. */
		suppressEscape = true;
#endif
		PauseScreen_Show(); return;
	}

	/* These should not be triggered multiple times when holding down */
	if (was) return;
	if (InputHandler_HandleCoreKey(key)) {
	} else if (LocalPlayer_HandlesKey(key)) {
	} else {
		idx = Hotkeys_FindPartial(key);
		if (idx == -1) return;

		hkey = &HotkeysList[idx];
		text = StringsBuffer_UNSAFE_Get(&HotkeysText, hkey->TextIndex);

		if (!hkey->StaysOpen) {
			Chat_Send(&text, false);
		} else if (!Gui_GetInputGrab()) {
			HUDScreen_OpenInput(&text);
		}
	}
}

static void InputHandler_KeyUp(void* obj, int key) {
	struct Screen* active;
	if (InputHandler_SimulateMouse(key, false)) return;

	if (key == KeyBinds[KEYBIND_ZOOM_SCROLL]) Game_SetFov(Game_DefaultFov);
	active = Gui_GetActiveScreen();

#ifdef CC_BUILD_WEB
	/* When closing menus (which reacquires mouse focus) in key down, */
	/* this still leaves the cursor visible. But if this is instead */
	/* done in key up, the cursor disappears as expected. */
	if (key == KEY_ESCAPE && active->closable) {
		if (suppressEscape) { suppressEscape = false; return; }
		Gui_Close(active); return;
	}
#endif
	Elem_HandlesKeyUp(active, key);
}

static void InputHandler_KeyPress(void* obj, int keyChar) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		if (s->VTABLE->HandlesKeyPress(s, keyChar)) return;
	}
}

void InputHandler_Init(void) {
	Event_RegisterFloat(&MouseEvents.Wheel,     NULL, InputHandler_MouseWheel);
	Event_RegisterMouseMove(&MouseEvents.Moved, NULL, InputHandler_MouseMove);
	Event_RegisterInt(&MouseEvents.Down,        NULL, InputHandler_MouseDown);
	Event_RegisterInt(&MouseEvents.Up,          NULL, InputHandler_MouseUp);
	Event_RegisterInt(&KeyEvents.Down,          NULL, InputHandler_KeyDown);
	Event_RegisterInt(&KeyEvents.Up,            NULL, InputHandler_KeyUp);
	Event_RegisterInt(&KeyEvents.Press,         NULL, InputHandler_KeyPress);

	Event_RegisterVoid(&UserEvents.HackPermissionsChanged, NULL, InputHandler_CheckZoomFov);
	KeyBind_Init();
	Hotkeys_Init();
}
