#include "InputHandler.h"
#include "Utils.h"
#include "ServerConnection.h"
#include "Game.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Camera.h"
#include "Inventory.h"
#include "World.h"
#include "Event.h"
#include "GameMode.h"
#include "Window.h"
#include "Entity.h"
#include "Chat.h"
#include "Funcs.h"
#include "Screens.h"
#include "Block.h"

bool input_buttonsDown[3];
Int32 input_pickingId = -1;
Int32 input_normViewDists[] = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
Int32 input_classicViewDists[] = { 8, 32, 128, 512 };
Key input_lastKey;
DateTime input_lastClick;
Real32 input_fovIndex = -1.0f;

bool InputHandler_IsMousePressed(MouseButton button) {
	if (Mouse_IsPressed(button)) return true;

	/* Key --> mouse mappings */
	if (button == MouseButton_Left   && KeyBind_IsPressed(KeyBind_MouseLeft))   return true;
	if (button == MouseButton_Middle && KeyBind_IsPressed(KeyBind_MouseMiddle)) return true;
	if (button == MouseButton_Right  && KeyBind_IsPressed(KeyBind_MouseRight))  return true;
	return false;
}

void InputHandler_ButtonStateUpdate(MouseButton button, bool pressed) {
	/* defer getting the targeted entity as it's a costly operation */
	if (input_pickingId == -1) {
		Entity* p = &LocalPlayer_Instance.Base;
		input_pickingId = Entities_GetCloset(p);
	}

	EntityID id = (EntityID)input_pickingId;
	ServerConnection_SendPlayerClick(button, pressed, id, &Game_SelectedPos);
	input_buttonsDown[button] = pressed;
}

void InputHandler_ButtonStateChanged(MouseButton button, bool pressed) {
	if (pressed) {
		/* Can send multiple Pressed events */
		InputHandler_ButtonStateUpdate(button, true);
	} else {
		if (!input_buttonsDown[button]) return;
		InputHandler_ButtonStateUpdate(button, false);
	}
}

void InputHandler_ScreenChanged(Screen* oldScreen, Screen* newScreen) {
	if (oldScreen != NULL && oldScreen->HandlesAllInput) {
		input_lastClick = Platform_CurrentUTCTime();
	}

	if (ServerConnection_SupportsPlayerClick) {
		input_pickingId = -1;
		InputHandler_ButtonStateChanged(MouseButton_Left,   false);
		InputHandler_ButtonStateChanged(MouseButton_Right,  false);
		InputHandler_ButtonStateChanged(MouseButton_Middle, false);
	}
}

bool InputHandler_IsShutdown(Key key, Key last) {
	if (key == Key_F4 && (last == Key_AltLeft || last == Key_AltRight)) return true;

	/* On OSX, Cmd+Q should also terminate the process. */
#if CC_BUILD_OSX
	return key == Key_Q && (last == Key_WinLeft || last == Key_WinRight);
#else
	return false;
#endif
}

void InputHandler_Toggle(Key key, bool* target, const UInt8* enableMsg, const UInt8* disableMsg) {
	*target = !(*target);
	UInt8 msgBuffer[String_BufferSize(STRING_SIZE * 2)];
	String msg = String_InitAndClearArray(msgBuffer);

	String_AppendConst(&msg, (*target) ? enableMsg : disableMsg);
	String_AppendConst(&msg, ". &ePress &a");
	String_AppendConst(&msg, Key_Names[key]);
	String_AppendConst(&msg, (*target) ? " &eto disable." : " &eto re-enable.");
	Chat_Add(&msg);
}

void InputHandler_CycleDistanceForwards(Int32* viewDists, Int32 count) {
	Int32 i;
	for (i = 0; i < count; i++) {
		Int32 dist = viewDists[i];
		if (dist > Game_UserViewDistance) {
			Game_SetViewDistance(dist, true); return;
		}
	}
	Game_SetViewDistance(viewDists[0], true);
}

void InputHandler_CycleDistanceBackwards(Int32* viewDists, Int32 count) {
	Int32 i;
	for (i = count - 1; i >= 0; i--) {
		Int32 dist = viewDists[i];
		if (dist < Game_UserViewDistance) {
			Game_SetViewDistance(dist, true); return;
		}
	}
	Game_SetViewDistance(viewDists[count - 1], true);
}

bool InputHandler_SetFOV(Int32 fov, bool setZoom) {
	if (Game_Fov == fov) return true;
	HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanAnyHacks || !h->CanUseThirdPersonCamera) return false;

	Game_Fov = fov;
	if (setZoom) Game_ZoomFov = fov;
	Game_UpdateProjection();
	return true;
}

bool InputHandler_DoFovZoom(Real32 deltaPrecise) {
	if (!KeyBind_IsPressed(KeyBind_ZoomScrolling)) return false;
	HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanAnyHacks || !h->CanUseThirdPersonCamera) return false;

	if (input_fovIndex == -1.0f) input_fovIndex = Game_ZoomFov;
	input_fovIndex -= deltaPrecise * 5.0f;

	Math_Clamp(input_fovIndex, 1.0f, Game_DefaultFov);
	return InputHandler_SetFOV((Int32)input_fovIndex, true);
}

bool InputHandler_HandleCoreKey(Key key) {
	if (key == KeyBind_Get(KeyBind_HideGui)) {
		Game_HideGui = !Game_HideGui;
	} else if (key == KeyBind_Get(KeyBind_HideFps)) {
		Game_ShowFPS = !Game_ShowFPS;
	} else if (key == KeyBind_Get(KeyBind_Fullscreen)) {
		UInt8 state = Window_GetWindowState();
		if (state != WINDOW_STATE_MAXIMISED) {
			bool fullscreen = state == WINDOW_STATE_FULLSCREEN;
			Window_SetWindowState(fullscreen ? WINDOW_STATE_NORMAL : WINDOW_STATE_FULLSCREEN);
		}
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
	} else if (key == KeyBind_Get(KeyBind_ToggleFog)) {
		Int32* viewDists = Game_UseClassicOptions ? input_classicViewDists : input_normViewDists;
		Int32 count = Game_UseClassicOptions ? Array_Elems(input_classicViewDists) : Array_Elems(input_normViewDists);

		if (Key_IsShiftPressed()) {
			InputHandler_CycleDistanceBackwards(viewDists, count);
		} else {
			InputHandler_CycleDistanceForwards(viewDists, count);
		}
	} else if ((key == KeyBind_Get(KeyBind_PauseOrExit) || key == Key_Pause) && World_Blocks != NULL) {
		Screen* screen = PauseScreen_MakeInstance();
		Gui_SetNewScreen(screen);
	} else if (GameMode_HandlesKeyDown(key)) {
	} else if (key == KeyBind_Get(KeyBind_IDOverlay)) {
		if (Gui_OverlaysCount > 0) return true;
		Gui_ShowOverlay(new TexIdsOverlay(), false);
	} else {
		return false;
	}
	return true;
}

bool InputHandler_TouchesSolid(BlockID b) { return Block_Collide[b] == COLLIDE_SOLID; }
bool InputHandler_PushbackPlace(AABB* blockBB) {
	Entity* p = &LocalPlayer_Instance.Base;
	HacksComp* hacks = &LocalPlayer_Instance.Hacks;
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
	AABB bounds; Entity_GetBounds(p, &bounds);
	if (!hacks->Noclip && Entity_TouchesAny(&bounds, InputHandler_TouchesSolid)) {
		p->Position = curPos;
		return false;
	}

	p->Position = curPos;
	LocationUpdate update; LocationUpdate_MakePos(&update, adjPos, false);
	p->VTABLE->SetLocation(p, &update, false);
	return true;
}

bool InputHandler_IntersectsOthers(Vector3 pos, BlockID block) {
	AABB blockBB;
	Vector3_Add(&blockBB.Min, &pos, &Block_MinBB[block]);
	Vector3_Add(&blockBB.Max, &pos, &Block_MaxBB[block]);

	Int32 id;
	for (id = 0; id < ENTITIES_SELF_ID; id++) {
		Entity* entity = Entities_List[id];
		if (entity == NULL) continue;

		AABB bounds; Entity_GetBounds(entity, &bounds);
		bounds.Min.Y += 1.0f / 32.0f; /* when player is exactly standing on top of ground */
		if (AABB_Intersects(&bounds, &blockBB)) return true;
	}
	return false;
}

bool InputHandler_CheckIsFree(BlockID block) {
	Vector3 pos; Vector3I_ToVector3(&pos, &Game_SelectedPos.TranslatedPos);
	Entity* p = &LocalPlayer_Instance.Base;
	HacksComp* hacks = &LocalPlayer_Instance.Hacks;

	if (Block_Collide[block] != COLLIDE_SOLID) return true;
	if (InputHandler_IntersectsOthers(pos, block)) return false;
	Vector3 nextPos = LocalPlayer_Instance.Interp.Next.Pos;

	AABB blockBB;
	Vector3_Add(&blockBB.Min, &pos, &Block_MinBB[block]);
	Vector3_Add(&blockBB.Max, &pos, &Block_MaxBB[block]);

	/* NOTE: Need to also test against nextPos here, otherwise player can 
	fall through the block at feet as collision is performed against nextPos */
	AABB localBB; AABB_Make(&localBB, &p->Position, &p->Size);
	localBB.Min.Y = min(nextPos.Y, localBB.Min.Y);

	if (hacks->Noclip || !AABB_Intersects(&localBB, &blockBB)) return true;
	if (hacks->CanPushbackBlocks && hacks->PushbackPlacing && hacks->Enabled) {
		return InputHandler_PushbackPlace(&blockBB);
	}

	localBB.Min.Y += 0.25f + ENTITY_ADJUSTMENT;
	if (AABB_Intersects(&localBB, &blockBB)) return false;

	/* Push player upwards when they are jumping and trying to place a block underneath them */
	nextPos.Y = pos.Y + Block_MaxBB[block].Y + ENTITY_ADJUSTMENT;
	LocationUpdate update; LocationUpdate_MakePos(&update, nextPos, false);
	p->VTABLE->SetLocation(p, &update, false);
	return true;
}

void InputHandler_PickBlocks(bool cooldown, bool left, bool middle, bool right) {
	DateTime now = Platform_CurrentUTCTime();
	Int64 delta = DateTime_MsBetween(&input_lastClick, &now);
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
		if (GameMode_PickingLeft()) return;
		Vector3I pos = Game_SelectedPos.BlockPos;
		if (!Game_SelectedPos.Valid || !World_IsValidPos_3I(pos)) return;

		BlockID old = World_GetBlock_3I(pos);
		if (Block_Draw[old] == DRAW_GAS || !Block_CanDelete[old]) return;
		GameMode_PickLeft(old);
	} else if (right) {
		if (GameMode_PickingRight()) return;
		Vector3I pos = Game_SelectedPos.TranslatedPos;
		if (!Game_SelectedPos.Valid || !World_IsValidPos_3I(pos)) return;

		BlockID old = World_GetBlock_3I(pos);
		BlockID block = Inventory_SelectedBlock;
		if (Game_AutoRotate) { block = AutoRotate_RotateBlock(block); }

		if (Game_CanPick(old) || !Block_CanPlace[block]) return;
		/* air-ish blocks can only replace over other air-ish blocks */
		if (Block_Draw[block] == DRAW_GAS && Block_Draw[old] != DRAW_GAS) return;

		if (!InputHandler_CheckIsFree(block)) return;
		GameMode_PickRight(old, block);
	} else if (middle) {
		Vector3I pos = Game_SelectedPos.BlockPos;
		if (!Game_SelectedPos.Valid || !World_IsValidPos_3I(pos)) return;

		BlockID old = World_GetBlock_3I(pos);
		GameMode_PickMiddle(old);
	}
}

void InputHandler_MouseWheel(void* obj, Real32 delta) {
	GuiElement* active = (GuiElement*)Gui_GetActiveScreen();
	if (active->VTABLE->HandlesMouseScroll(active, delta)) return;

	bool hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (!hotbar && Camera_ActiveCamera->Zoom(delta)) return;
	if (InputHandler_DoFovZoom(delta) || !Inventory_CanChangeHeldBlock) return;

	game.Gui.hudScreen.hotbar.HandlesMouseScroll(delta);
}

void InputHandler_MouseMove(void* obj, Int32 xDelta, Int32 yDelta) {
	GuiElement* active = (GuiElement*)Gui_GetActiveScreen();
	active->VTABLE->HandlesMouseMove(active, Mouse_X, Mouse_Y);
}

void InputHandler_MouseDown(void* obj, MouseButton button) {
	GuiElement* active = (GuiElement*)Gui_GetActiveScreen();
	if (!active->VTABLE->HandlesMouseDown(active, Mouse_X, Mouse_Y, button)) {
		bool left   = button == MouseButton_Left;
		bool middle = button == MouseButton_Middle;
		bool right  = button == MouseButton_Right;
		InputHandler_PickBlocks(false, left, middle, right);
	} else {
		input_lastClick = Platform_CurrentUTCTime();
	}
}

void InputHandler_MouseUp(void* obj, MouseButton button) {
	GuiElement* active = (GuiElement*)Gui_GetActiveScreen();
	if (!active->VTABLE->HandlesMouseUp(active, Mouse_X, Mouse_Y, button)) {
		if (ServerConnection_SupportsPlayerClick && button <= MouseButton_Middle) {
			input_pickingId = -1;
			InputHandler_ButtonStateChanged(button, false);
		}
	}
}

bool InputHandler_SimulateMouse(Key key, bool pressed) {
	Key left   = KeyBind_Get(KeyBind_MouseLeft);
	Key middle = KeyBind_Get(KeyBind_MouseMiddle);
	Key right  = KeyBind_Get(KeyBind_MouseRight);
	if (!(key == left || key == middle || key == right)) return false;

	MouseButton btn = key == left ? MouseButton_Left : key == middle ? MouseButton_Middle : MouseButton_Right;
	if (pressed) { InputHandler_MouseDown(NULL, btn); }
	else {         InputHandler_MouseUp(NULL,   btn); }
	return true;
}

void InputHandler_KeyDown(void* obj, Key key) {
	if (InputHandler_SimulateMouse(key, true)) return;
	GuiElement* active = (GuiElement*)Gui_GetActiveScreen();

	Key last = input_lastKey; input_lastKey = key;
	if (InputHandler_IsShutdown(key, last)) {
		/* TODO: Do we need a separate exit function in Game class? */
		Window_Close();
	} else if (key == KeyBind_Get(KeyBind_Screenshot)) {
		Game_ScreenshotRequested = true;
	} else if (active->VTABLE->HandlesKeyDown(active, key)) {
	} else if (InputHandler_HandleCoreKey(key)) {
	} else if (LocalPlayer_Instance.Input.Handles(key)) {
	} else {
		UInt8 textBuffer[String_BufferSize(STRING_SIZE)];
		String text = String_InitAndClearArray(textBuffer);
		bool more;
		if (!Hotkeys_IsHotkey(key, &text, &more)) return;

		if (!more) {
			ServerConnection_SendChat(&text);
		} else if (Gui_Active == NULL) {
			HUDScreen_OpenInput(Gui_HUD, &text);
		}
	}
}

void InputHandler_KeyUp(void* obj, Key key) {
	if (InputHandler_SimulateMouse(key, false)) return;

	if (key == KeyBind_Get(KeyBind_ZoomScrolling)) {
		InputHandler_SetFOV(Game_DefaultFov, false);
	}

	GuiElement* active = (GuiElement*)Gui_GetActiveScreen();
	active->VTABLE->HandlesKeyUp(active, key);
}

void InputHandler_KeyPress(void* obj, Int32 keyChar) {
	GuiElement* active = (GuiElement*)Gui_GetActiveScreen();
	active->VTABLE->HandlesKeyPress(active, (UInt8)keyChar);
}

void InputHandler_Init(void) {
	Event_RegisterReal32(&MouseEvents_Wheel,    NULL, InputHandler_MouseWheel);
	Event_RegisterMouseMove(&MouseEvents_Moved, NULL, InputHandler_MouseMove);
	Event_RegisterInt32(&MouseEvents_Down,      NULL, InputHandler_MouseDown);
	Event_RegisterInt32(&MouseEvents_Up,        NULL, InputHandler_MouseUp);
	Event_RegisterInt32(&KeyEvents_Down,        NULL, InputHandler_KeyDown);
	Event_RegisterInt32(&KeyEvents_Up,          NULL, InputHandler_KeyUp);
	Event_RegisterInt32(&KeyEvents_Press,       NULL, InputHandler_KeyPress);

	KeyBind_Init();
	Hotkeys_Init();
}