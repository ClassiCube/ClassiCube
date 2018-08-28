#include "Screens.h"
#include "Widgets.h"
#include "Game.h"
#include "Event.h"
#include "GraphicsCommon.h"
#include "Platform.h"
#include "Inventory.h"
#include "Drawer2D.h"
#include "GraphicsAPI.h"
#include "Funcs.h"
#include "TerrainAtlas.h"
#include "ModelCache.h"
#include "MapGenerator.h"
#include "ServerConnection.h"
#include "Chat.h"
#include "ExtMath.h"
#include "Window.h"
#include "Camera.h"
#include "AsyncDownloader.h"
#include "Block.h"
#include "Menus.h"
#include "World.h"

struct InventoryScreen {
	Screen_Layout
	struct FontDesc Font;
	struct TableWidget Table;
	bool ReleasedInv, DeferredSelect;
};

struct StatusScreen {
	Screen_Layout
	struct FontDesc Font;
	struct TextWidget Status, HackStates;
	struct TextAtlas PosAtlas;
	Real64 Accumulator;
	Int32 Frames, FPS;
	bool Speed, HalfSpeed, Noclip, Fly, CanSpeed;
	Int32 LastFov;
};

struct HUDScreen {
	Screen_Layout
	struct Screen* Chat;
	struct HotbarWidget Hotbar;
	struct PlayerListWidget PlayerList;
	struct FontDesc PlayerFont;
	bool ShowingList, WasShowingList;
};

struct LoadingScreen {
	Screen_Layout
	struct FontDesc Font;
	Real32 Progress;
	
	struct TextWidget Title, Message;
	char __TitleBuffer[STRING_SIZE];
	char __MessageBuffer[STRING_SIZE];
	String TitleStr, MessageStr;
};

struct GeneratingScreen {
	struct LoadingScreen Base;
	const char* LastState;
};

#define CHATSCREEN_MAX_STATUS 5
#define CHATSCREEN_MAX_GROUP 3
struct ChatScreen {
	Screen_Layout
	struct Screen* HUD;
	Int32 InputOldHeight;
	Real32 ChatAcc;
	bool SuppressNextPress;
	Int32 ChatIndex;
	Int32 LastDownloadStatus;
	struct FontDesc ChatFont, ChatUrlFont, AnnouncementFont;
	struct TextWidget Announcement;
	struct ChatInputWidget Input;
	struct TextGroupWidget Status, BottomRight, Chat, ClientStatus;
	struct SpecialInputWidget AltText;

	/* needed for lost contexts, to restore chat typed in */
	char __ChatInInputBuffer[INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN];
	String ChatInInputStr;

	struct Texture Status_Textures[CHATSCREEN_MAX_STATUS];
	struct Texture BottomRight_Textures[CHATSCREEN_MAX_GROUP];
	struct Texture ClientStatus_Textures[CHATSCREEN_MAX_GROUP];
	struct Texture Chat_Textures[TEXTGROUPWIDGET_MAX_LINES];
	char Status_Buffer[CHATSCREEN_MAX_STATUS * TEXTGROUPWIDGET_LEN];
	char BottomRight_Buffer[CHATSCREEN_MAX_GROUP * TEXTGROUPWIDGET_LEN];
	char ClientStatus_Buffer[CHATSCREEN_MAX_GROUP * TEXTGROUPWIDGET_LEN];
	char Chat_Buffer[TEXTGROUPWIDGET_MAX_LINES * TEXTGROUPWIDGET_LEN];
};

struct DisconnectScreen {
	Screen_Layout
	Int64 InitTime, ClearTime;
	bool CanReconnect, LastActive;
	Int32 LastSecsLeft;
	struct ButtonWidget Reconnect;

	struct FontDesc TitleFont, MessageFont;
	struct TextWidget Title, Message;
	char __TitleBuffer[STRING_SIZE];
	char __MessageBuffer[STRING_SIZE];
	String TitleStr, MessageStr;
};


/*########################################################################################################################*
*-----------------------------------------------------InventoryScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE InventoryScreen_VTABLE;
struct InventoryScreen InventoryScreen_Instance;
static void InventoryScreen_OnBlockChanged(void* obj) {
	struct InventoryScreen* screen = (struct InventoryScreen*)obj;
	TableWidget_OnInventoryChanged(&screen->Table);
}

static void InventoryScreen_ContextLost(void* obj) {
	struct InventoryScreen* screen = (struct InventoryScreen*)obj;
	Elem_TryFree(&screen->Table);
}

static void InventoryScreen_ContextRecreated(void* obj) {
	struct InventoryScreen* screen = (struct InventoryScreen*)obj;
	Elem_Recreate(&screen->Table);
}

static void InventoryScreen_MoveToSelected(struct InventoryScreen* screen) {
	struct TableWidget* table = &screen->Table;
	TableWidget_SetBlockTo(table, Inventory_SelectedBlock);
	Elem_Recreate(table);
	screen->DeferredSelect = false;

	/* User is holding invalid block */
	if (table->SelectedIndex == -1) {
		TableWidget_MakeDescTex(table, Inventory_SelectedBlock);
	}
}

static void InventoryScreen_Init(struct GuiElem* elem) {
	struct InventoryScreen* screen = (struct InventoryScreen*)elem;
	Font_Make(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);

	TableWidget_Create(&screen->Table);
	screen->Table.Font = screen->Font;
	screen->Table.ElementsPerRow = Game_PureClassic ? 9 : 10;
	Elem_Init(&screen->Table);

	/* Can't immediately move to selected here, because cursor visibility 
	   might be toggled after Init() is called. This causes the cursor to 
	   be moved back to the middle of the window. */
	screen->DeferredSelect = true;

	Key_KeyRepeat = true;
	Event_RegisterVoid(&BlockEvents_PermissionsChanged, screen, InventoryScreen_OnBlockChanged);
	Event_RegisterVoid(&BlockEvents_BlockDefChanged,    screen, InventoryScreen_OnBlockChanged);	
	Event_RegisterVoid(&GfxEvents_ContextLost,          screen, InventoryScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,     screen, InventoryScreen_ContextRecreated);
}

static void InventoryScreen_Render(struct GuiElem* elem, Real64 delta) {
	struct InventoryScreen* screen = (struct InventoryScreen*)elem;
	if (screen->DeferredSelect) InventoryScreen_MoveToSelected(screen);
	Elem_Render(&screen->Table, delta);
}

static void InventoryScreen_OnResize(struct GuiElem* elem) {
	struct InventoryScreen* screen = (struct InventoryScreen*)elem;
	Widget_Reposition(&screen->Table);
}

static void InventoryScreen_Free(struct GuiElem* elem) {
	struct InventoryScreen* screen = (struct InventoryScreen*)elem;
	Font_Free(&screen->Font);
	Elem_TryFree(&screen->Table);

	Key_KeyRepeat = false;
	Event_UnregisterVoid(&BlockEvents_PermissionsChanged, screen, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&BlockEvents_BlockDefChanged,    screen, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,          screen, InventoryScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,     screen, InventoryScreen_ContextRecreated);
}

static bool InventoryScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct InventoryScreen* screen = (struct InventoryScreen*)elem;
	struct TableWidget* table = &screen->Table;

	if (key == KeyBind_Get(KeyBind_PauseOrExit)) {
		Gui_ReplaceActive(NULL);
	} else if (key == KeyBind_Get(KeyBind_Inventory) && screen->ReleasedInv) {
		Gui_ReplaceActive(NULL);
	} else if (key == Key_Enter && table->SelectedIndex != -1) {
		Inventory_SetSelectedBlock(table->Elements[table->SelectedIndex]);
		Gui_ReplaceActive(NULL);
	} else if (Elem_HandlesKeyDown(table, key)) {
	} else {
		struct HUDScreen* hud = (struct HUDScreen*)Gui_HUD;
		return Elem_HandlesKeyDown(&hud->Hotbar, key);
	}
	return true;
}

static bool InventoryScreen_HandlesKeyUp(struct GuiElem* elem, Key key) {
	struct InventoryScreen* screen = (struct InventoryScreen*)elem;
	if (key == KeyBind_Get(KeyBind_Inventory)) {
		screen->ReleasedInv = true; return true;
	}

	struct HUDScreen* hud = (struct HUDScreen*)Gui_HUD;
	return Elem_HandlesKeyUp(&hud->Hotbar, key);
}

static bool InventoryScreen_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct InventoryScreen* screen = (struct InventoryScreen*)elem;
	struct TableWidget* table = &screen->Table;
	struct HUDScreen* hud = (struct HUDScreen*)Gui_HUD;
	if (table->Scroll.DraggingMouse || Elem_HandlesMouseDown(&hud->Hotbar, x, y, btn)) return true;

	bool handled = Elem_HandlesMouseDown(table, x, y, btn);
	if ((!handled || table->PendingClose) && btn == MouseButton_Left) {
		bool hotbar = Key_IsControlPressed() || Key_IsShiftPressed();
		if (!hotbar) Gui_ReplaceActive(NULL);
	}
	return true;
}

static bool InventoryScreen_HandlesMouseUp(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct InventoryScreen* screen = (struct InventoryScreen*)elem;
	struct TableWidget* table = &screen->Table;
	return Elem_HandlesMouseUp(table, x, y, btn);
}

static bool InventoryScreen_HandlesMouseMove(struct GuiElem* elem, Int32 x, Int32 y) {
	struct InventoryScreen* screen = (struct InventoryScreen*)elem;
	struct TableWidget* table = &screen->Table;
	return Elem_HandlesMouseMove(table, x, y);
}

static bool InventoryScreen_HandlesMouseScroll(struct GuiElem* elem, Real32 delta) {
	struct InventoryScreen* screen = (struct InventoryScreen*)elem;
	struct TableWidget* table = &screen->Table;

	bool hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (hotbar) return false;
	return Elem_HandlesMouseScroll(table, delta);
}

struct Screen* InventoryScreen_MakeInstance(void) {
	struct InventoryScreen* screen = &InventoryScreen_Instance;
	Mem_Set(screen, 0, sizeof(struct InventoryScreen));
	screen->VTABLE = &InventoryScreen_VTABLE;
	Screen_Reset((struct Screen*)screen);

	screen->VTABLE->HandlesKeyDown     = InventoryScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp       = InventoryScreen_HandlesKeyUp;
	screen->VTABLE->HandlesMouseDown   = InventoryScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseUp     = InventoryScreen_HandlesMouseUp;
	screen->VTABLE->HandlesMouseMove   = InventoryScreen_HandlesMouseMove;
	screen->VTABLE->HandlesMouseScroll = InventoryScreen_HandlesMouseScroll;

	screen->OnResize       = InventoryScreen_OnResize;
	screen->VTABLE->Init   = InventoryScreen_Init;
	screen->VTABLE->Render = InventoryScreen_Render;
	screen->VTABLE->Free   = InventoryScreen_Free;
	return (struct Screen*)screen;
}
struct Screen* InventoryScreen_UNSAFE_RawPointer = (struct Screen*)&InventoryScreen_Instance;


/*########################################################################################################################*
*-------------------------------------------------------StatusScreen------------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE StatusScreen_VTABLE;
struct StatusScreen StatusScreen_Instance;
static void StatusScreen_MakeText(struct StatusScreen* screen, STRING_TRANSIENT String* status) {
	screen->FPS = (Int32)(screen->Frames / screen->Accumulator);
	String_Format1(status, "%i fps, ", &screen->FPS);

	if (Game_ClassicMode) {
		String_Format1(status, "%i chunk updates", &Game_ChunkUpdates);
	} else {
		if (Game_ChunkUpdates) {
			String_Format1(status, "%i chunks/s, ", &Game_ChunkUpdates);
		}
		Int32 indices = ICOUNT(Game_Vertices);
		String_Format1(status, "%i vertices", &indices);

		Int32 ping = PingList_AveragePingMs();
		if (ping) {
			String_Format1(status, ", ping %i ms", &ping);
		}
	}
}

static void StatusScreen_DrawPosition(struct StatusScreen* screen) {
	struct TextAtlas* atlas = &screen->PosAtlas;
	VertexP3fT2fC4b vertices[4 * 64];
	VertexP3fT2fC4b* ptr = vertices;

	struct Texture tex = atlas->Tex; tex.X = 2; tex.Width = atlas->Offset;
	PackedCol col = PACKEDCOL_WHITE;
	GfxCommon_Make2DQuad(&tex, col, &ptr);

	Vector3I pos; Vector3I_Floor(&pos, &LocalPlayer_Instance.Base.Position);
	atlas->CurX = atlas->Offset + 2;

	TextAtlas_Add(atlas, 13, &ptr);
	TextAtlas_AddInt(atlas, pos.X, &ptr);
	TextAtlas_Add(atlas, 11, &ptr);
	TextAtlas_AddInt(atlas, pos.Y, &ptr);
	TextAtlas_Add(atlas, 11, &ptr);
	TextAtlas_AddInt(atlas, pos.Z, &ptr);
	TextAtlas_Add(atlas, 14, &ptr);

	Gfx_BindTexture(atlas->Tex.ID);
	/* TODO: Do we need to use a separate VB here? */
	Int32 count = (Int32)(ptr - vertices);
	GfxCommon_UpdateDynamicVb_IndexedTris(ModelCache_Vb, vertices, count);
}

static bool StatusScreen_HacksChanged(struct StatusScreen* screen) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	return hacks->Speeding != screen->Speed || hacks->HalfSpeeding != screen->HalfSpeed || hacks->Flying != screen->Fly
		|| hacks->Noclip != screen->Noclip  || Game_Fov != screen->LastFov || hacks->CanSpeed != screen->CanSpeed;
}

static void StatusScreen_UpdateHackState(struct StatusScreen* screen) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	screen->Speed = hacks->Speeding; screen->HalfSpeed = hacks->HalfSpeeding; screen->Fly = hacks->Flying;
	screen->Noclip = hacks->Noclip; screen->LastFov = Game_Fov; screen->CanSpeed = hacks->CanSpeed;

	char statusBuffer[STRING_SIZE * 2];
	String status = String_FromArray(statusBuffer);

	if (Game_Fov != Game_DefaultFov) {
		String_Format1(&status, "Zoom fov %i  ", &Game_Fov);
	}
	if (hacks->Flying) String_AppendConst(&status, "Fly ON   ");

	bool speeding = (hacks->Speeding || hacks->HalfSpeeding) && hacks->CanSpeed;
	if (speeding)      String_AppendConst(&status, "Speed ON   ");
	if (hacks->Noclip) String_AppendConst(&status, "Noclip ON   ");

	TextWidget_SetText(&screen->HackStates, &status);
}

static void StatusScreen_Update(struct StatusScreen* screen, Real64 delta) {
	screen->Frames++;
	screen->Accumulator += delta;
	if (screen->Accumulator < 1.0) return;

	char statusBuffer[STRING_SIZE * 2];
	String status = String_FromArray(statusBuffer);
	StatusScreen_MakeText(screen, &status);

	TextWidget_SetText(&screen->Status, &status);
	screen->Accumulator = 0.0;
	screen->Frames = 0;
	Game_ChunkUpdates = 0;
}

static void StatusScreen_OnResize(struct GuiElem* elem) { }
static void StatusScreen_FontChanged(void* obj) {
	struct StatusScreen* screen = (struct StatusScreen*)obj;
	Elem_Recreate(screen);
}

static void StatusScreen_ContextLost(void* obj) {
	struct StatusScreen* screen = (struct StatusScreen*)obj;
	TextAtlas_Free(&screen->PosAtlas);
	Elem_TryFree(&screen->Status);
	Elem_TryFree(&screen->HackStates);
}

static void StatusScreen_ContextRecreated(void* obj) {
	struct StatusScreen* screen = (struct StatusScreen*)obj;

	struct TextWidget* status = &screen->Status; TextWidget_Make(status, &screen->Font);
	Widget_SetLocation((struct Widget*)status, ANCHOR_MIN, ANCHOR_MIN, 2, 2);
	status->ReducePadding = true;
	Elem_Init(status);
	StatusScreen_Update(screen, 1.0);

	String chars = String_FromConst("0123456789-, ()");
	String prefix = String_FromConst("Position: ");
	TextAtlas_Make(&screen->PosAtlas, &chars, &screen->Font, &prefix);
	screen->PosAtlas.Tex.Y = status->Height + 2;

	Int32 yOffset = status->Height + screen->PosAtlas.Tex.Height + 2;
	struct TextWidget* hacks = &screen->HackStates; TextWidget_Make(hacks, &screen->Font);
	Widget_SetLocation((struct Widget*)hacks, ANCHOR_MIN, ANCHOR_MIN, 2, yOffset);
	hacks->ReducePadding = true;
	Elem_Init(hacks);
	StatusScreen_UpdateHackState(screen);
}

static void StatusScreen_Init(struct GuiElem* elem) {
	struct StatusScreen* screen = (struct StatusScreen*)elem;
	Font_Make(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);
	StatusScreen_ContextRecreated(screen);

	Event_RegisterVoid(&ChatEvents_FontChanged,     screen, StatusScreen_FontChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, StatusScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, StatusScreen_ContextRecreated);
}

static void StatusScreen_Render(struct GuiElem* elem, Real64 delta) {
	struct StatusScreen* screen = (struct StatusScreen*)elem;
	StatusScreen_Update(screen, delta);
	if (Game_HideGui || !Game_ShowFPS) return;

	Gfx_SetTexturing(true);
	Elem_Render(&screen->Status, delta);

	if (!Game_ClassicMode && !Gui_Active) {
		if (StatusScreen_HacksChanged(screen)) { StatusScreen_UpdateHackState(screen); }
		StatusScreen_DrawPosition(screen);
		Elem_Render(&screen->HackStates, delta);
	}
	Gfx_SetTexturing(false);
}

static void StatusScreen_Free(struct GuiElem* elem) {
	struct StatusScreen* screen = (struct StatusScreen*)elem;
	Font_Free(&screen->Font);
	StatusScreen_ContextLost(screen);

	Event_UnregisterVoid(&ChatEvents_FontChanged,     screen, StatusScreen_FontChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, StatusScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, StatusScreen_ContextRecreated);
}

struct Screen* StatusScreen_MakeInstance(void) {
	struct StatusScreen* screen = &StatusScreen_Instance;
	Mem_Set(screen, 0, sizeof(struct StatusScreen));
	screen->VTABLE = &StatusScreen_VTABLE;
	Screen_Reset((struct Screen*)screen);
	screen->HandlesAllInput = false;

	screen->OnResize       = StatusScreen_OnResize;
	screen->VTABLE->Init   = StatusScreen_Init;
	screen->VTABLE->Render = StatusScreen_Render;
	screen->VTABLE->Free   = StatusScreen_Free;
	return (struct Screen*)screen;
}

static void StatusScreen_Ready(void) { Elem_Init(&StatusScreen_Instance); }
void StatusScreen_MakeComponent(struct IGameComponent* comp) {
	comp->Ready = StatusScreen_Ready;
}


/*########################################################################################################################*
*------------------------------------------------------LoadingScreen------------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE LoadingScreen_VTABLE;
struct LoadingScreen LoadingScreen_Instance;
static void LoadingScreen_SetTitle(struct LoadingScreen* screen) {
	Elem_TryFree(&screen->Title);
	TextWidget_Create(&screen->Title, &screen->TitleStr, &screen->Font);
	Widget_SetLocation((struct Widget*)(&screen->Title), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -31);
}

static void LoadingScreen_SetMessage(struct LoadingScreen* screen) {
	Elem_TryFree(&screen->Message);
	TextWidget_Create(&screen->Message, &screen->MessageStr, &screen->Font);
	Widget_SetLocation((struct Widget*)(&screen->Message), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 17);
}

static void LoadingScreen_MapLoading(void* obj, Real32 progress) {
	struct LoadingScreen* screen = (struct LoadingScreen*)obj;
	screen->Progress = progress;
}

static void LoadingScreen_OnResize(struct GuiElem* elem) {
	struct LoadingScreen* screen = (struct LoadingScreen*)elem;
	Widget_Reposition(&screen->Title);
	Widget_Reposition(&screen->Message);
}

static void LoadingScreen_ContextLost(void* obj) {
	struct LoadingScreen* screen = (struct LoadingScreen*)obj;
	Elem_TryFree(&screen->Title);
	Elem_TryFree(&screen->Message);
}

static void LoadingScreen_ContextRecreated(void* obj) {
	struct LoadingScreen* screen = (struct LoadingScreen*)obj;
	if (Gfx_LostContext) return;
	LoadingScreen_SetTitle(screen);
	LoadingScreen_SetMessage(screen);
}

static bool LoadingScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	if (key == Key_Tab) return true;
	return Elem_HandlesKeyDown(Gui_HUD, key);
}

static bool LoadingScreen_HandlesKeyPress(struct GuiElem* elem, UInt8 key) {
	return Elem_HandlesKeyPress(Gui_HUD, key);
}

static bool LoadingScreen_HandlesKeyUp(struct GuiElem* elem, Key key) {
	if (key == Key_Tab) return true;
	return Elem_HandlesKeyUp(Gui_HUD, key);
}

static bool LoadingScreen_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	if (Gui_HUD->HandlesAllInput) { Elem_HandlesMouseDown(Gui_HUD, x, y, btn); }
	return true;
}

static bool LoadingScreen_HandlesMouseUp(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) { return true; }
static bool LoadingScreen_HandlesMouseMove(struct GuiElem* elem, Int32 x, Int32 y) { return true; }
static bool LoadingScreen_HandlesMouseScroll(struct GuiElem* elem, Real32 delta) { return true; }

static void LoadingScreen_UpdateBackgroundVB(VertexP3fT2fC4b* vertices, Int32 count, Int32 atlasIndex, bool* bound) {
	if (!(*bound)) {
		*bound = true;
		Gfx_BindTexture(Atlas1D_TexIds[atlasIndex]);
	}

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	/* TODO: Do we need to use a separate VB here? */
	GfxCommon_UpdateDynamicVb_IndexedTris(ModelCache_Vb, vertices, count);
}

#define LOADING_TILE_SIZE 64
static void LoadingScreen_DrawBackground(void) {
	VertexP3fT2fC4b vertices[144];
	VertexP3fT2fC4b* ptr = vertices;
	Int32 count = 0, atlasIndex = 0, y = 0;
	PackedCol col = PACKEDCOL_CONST(64, 64, 64, 255);

	TextureLoc texLoc = Block_GetTexLoc(BLOCK_DIRT, FACE_YMAX);
	struct TextureRec rec = Atlas1D_TexRec(texLoc, 1, &atlasIndex);

	Real32 u2 = (Real32)Game_Width / (Real32)LOADING_TILE_SIZE;
	struct Texture tex = { NULL, TEX_RECT(0,0, Game_Width,LOADING_TILE_SIZE), TEX_UV(0,rec.V1, u2,rec.V2) };

	bool bound = false;
	while (y < Game_Height) {
		tex.Y = y;
		y += LOADING_TILE_SIZE;
		GfxCommon_Make2DQuad(&tex, col, &ptr);
		count += 4;

		if (count < Array_Elems(vertices)) continue;
		LoadingScreen_UpdateBackgroundVB(vertices, count, atlasIndex, &bound);
		count = 0;
		ptr = vertices;
	}

	if (!count) return;
	LoadingScreen_UpdateBackgroundVB(vertices, count, atlasIndex, &bound);
}

static void LoadingScreen_Init(struct GuiElem* elem) {
	struct LoadingScreen* screen = (struct LoadingScreen*)elem;
	Gfx_SetFog(false);
	LoadingScreen_ContextRecreated(screen);

	Event_RegisterReal(&WorldEvents_Loading,        screen, LoadingScreen_MapLoading);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, LoadingScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, LoadingScreen_ContextRecreated);
}

#define PROG_BAR_WIDTH 200
#define PROG_BAR_HEIGHT 4
static void LoadingScreen_Render(struct GuiElem* elem, Real64 delta) {
	struct LoadingScreen* screen = (struct LoadingScreen*)elem;
	Gfx_SetTexturing(true);
	LoadingScreen_DrawBackground();
	Elem_Render(&screen->Title, delta);
	Elem_Render(&screen->Message, delta);
	Gfx_SetTexturing(false);

	Int32 x = Gui_CalcPos(ANCHOR_CENTRE,  0, PROG_BAR_WIDTH,  Game_Width);
	Int32 y = Gui_CalcPos(ANCHOR_CENTRE, 34, PROG_BAR_HEIGHT, Game_Height);
	Int32 progWidth = (Int32)(PROG_BAR_WIDTH * screen->Progress);

	PackedCol backCol = PACKEDCOL_CONST(128, 128, 128, 255);
	PackedCol progCol = PACKEDCOL_CONST(128, 255, 128, 255);
	GfxCommon_Draw2DFlat(x, y, PROG_BAR_WIDTH, PROG_BAR_HEIGHT, backCol);
	GfxCommon_Draw2DFlat(x, y, progWidth,      PROG_BAR_HEIGHT, progCol);
}

static void LoadingScreen_Free(struct GuiElem* elem) {
	struct LoadingScreen* screen = (struct LoadingScreen*)elem;
	Font_Free(&screen->Font);
	LoadingScreen_ContextLost(screen);

	Event_UnregisterReal(&WorldEvents_Loading,        screen, LoadingScreen_MapLoading);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, LoadingScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, LoadingScreen_ContextRecreated);
}

static void LoadingScreen_Make(struct LoadingScreen* screen, struct GuiElementVTABLE* vtable, STRING_PURE String* title, STRING_PURE String* message) {
	Mem_Set(screen, 0, sizeof(struct LoadingScreen));
	screen->VTABLE = vtable;
	Screen_Reset((struct Screen*)screen);

	screen->VTABLE->HandlesKeyDown     = LoadingScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyPress    = LoadingScreen_HandlesKeyPress;
	screen->VTABLE->HandlesKeyUp       = LoadingScreen_HandlesKeyUp;
	screen->VTABLE->HandlesMouseDown   = LoadingScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseUp     = LoadingScreen_HandlesMouseUp;
	screen->VTABLE->HandlesMouseMove   = LoadingScreen_HandlesMouseMove;
	screen->VTABLE->HandlesMouseScroll = LoadingScreen_HandlesMouseScroll;

	screen->OnResize       = LoadingScreen_OnResize;
	screen->VTABLE->Init   = LoadingScreen_Init;
	screen->VTABLE->Render = LoadingScreen_Render;
	screen->VTABLE->Free   = LoadingScreen_Free;

	String title_copy = String_FromArray(screen->__TitleBuffer);
	String_AppendString(&title_copy, title);
	screen->TitleStr = title_copy;

	String message_copy = String_FromArray(screen->__MessageBuffer);
	String_AppendString(&message_copy, message);
	screen->MessageStr = message_copy;

	Font_Make(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);
	screen->BlocksWorld     = true;
	screen->RenderHUDOver   = true;
}

struct Screen* LoadingScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message) {
	struct LoadingScreen* screen = &LoadingScreen_Instance;
	LoadingScreen_Make(screen, &LoadingScreen_VTABLE, title, message);
	return (struct Screen*)screen;
}
struct Screen* LoadingScreen_UNSAFE_RawPointer = (struct Screen*)&LoadingScreen_Instance;


/*########################################################################################################################*
*--------------------------------------------------GeneratingMapScreen----------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE GeneratingScreen_VTABLE;
struct GeneratingScreen GeneratingScreen_Instance;
static void GeneratingScreen_Init(struct GuiElem* elem) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);
	Gen_Done = false;
	LoadingScreen_Init(elem);

	void* threadHandle;
	if (Gen_Vanilla) {
		threadHandle = Thread_Start(&NotchyGen_Generate);
	} else {
		threadHandle = Thread_Start(&FlatgrassGen_Generate);
	}
	/* don't leak thread handle here */
	Thread_FreeHandle(threadHandle);
}

static void GeneratingScreen_EndGeneration(void) {
	Gui_ReplaceActive(NULL);
	Gen_Done = false;

	if (!Gen_Blocks) {
		Chat_AddRaw("&cFailed to generate the map."); return;
	}

	World_BlocksSize = Gen_Width * Gen_Height * Gen_Length;
	World_SetNewMap(Gen_Blocks, World_BlocksSize, Gen_Width, Gen_Height, Gen_Length);
	Gen_Blocks = NULL;

	struct LocalPlayer* p = &LocalPlayer_Instance;
	Real32 x = (World_Width / 2) + 0.5f, z = (World_Length / 2) + 0.5f;
	p->Spawn = Respawn_FindSpawnPosition(x, z, p->Base.Size);

	struct LocationUpdate update; LocationUpdate_MakePosAndOri(&update, p->Spawn, 0.0f, 0.0f, false);
	p->Base.VTABLE->SetLocation(&p->Base, &update, false);
	Game_CurrentCameraPos = Camera_Active->GetPosition(0.0f);
	Event_RaiseVoid(&WorldEvents_MapLoaded);
}

static void GeneratingScreen_Render(struct GuiElem* elem, Real64 delta) {
	struct GeneratingScreen* screen = (struct GeneratingScreen*)elem;
	LoadingScreen_Render(elem, delta);
	if (Gen_Done) { GeneratingScreen_EndGeneration(); return; }

	const volatile char* state = Gen_CurrentState;
	screen->Base.Progress = Gen_CurrentProgress;
	if (state == screen->LastState) return;

	String message = screen->Base.MessageStr;
	message.length = 0;
	String_AppendConst(&message, state);
	LoadingScreen_SetMessage(&screen->Base);
}

struct Screen* GeneratingScreen_MakeInstance(void) {
	struct GeneratingScreen* screen = &GeneratingScreen_Instance;
	String title   = String_FromConst("Generating level");
	String message = String_FromConst("Generating..");
	LoadingScreen_Make(&screen->Base, &GeneratingScreen_VTABLE, &title, &message);

	screen->Base.VTABLE->Init   = GeneratingScreen_Init;
	screen->Base.VTABLE->Render = GeneratingScreen_Render;
	screen->LastState = NULL;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*--------------------------------------------------------ChatScreen-------------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE ChatScreen_VTABLE;
struct ChatScreen ChatScreen_Instance;
static Int32 ChatScreen_BottomOffset(void) { return ((struct HUDScreen*)Gui_HUD)->Hotbar.Height; }
static Int32 ChatScreen_InputUsedHeight(struct ChatScreen* screen) {
	if (screen->AltText.Height == 0) {
		return screen->Input.Base.Height + 20;
	} else {
		return (Game_Height - screen->AltText.Y) + 5;
	}
}

static void ChatScreen_UpdateAltTextY(struct ChatScreen* screen) {
	struct InputWidget* input = &screen->Input.Base;
	Int32 height = max(input->Height + input->YOffset, ChatScreen_BottomOffset());
	height += input->YOffset;

	screen->AltText.Tex.Y = Game_Height - (height + screen->AltText.Tex.Height);
	screen->AltText.Y = screen->AltText.Tex.Y;
}

static void ChatScreen_SetHandlesAllInput(struct ChatScreen* screen, bool handles) {
	screen->HandlesAllInput  = handles;
	Gui_HUD->HandlesAllInput = handles;
	Gui_CalcCursorVisible();
}

static void ChatScreen_OpenInput(struct ChatScreen* screen, STRING_PURE String* initialText) {
	screen->SuppressNextPress = true;
	ChatScreen_SetHandlesAllInput(screen, true);
	Key_KeyRepeat = true;

	String_Copy(&screen->Input.Base.Text, initialText);
	Elem_Recreate(&screen->Input.Base);
}

static void ChatScreen_ResetChat(struct ChatScreen* screen) {
	Elem_TryFree(&screen->Chat);
	Int32 i;
	for (i = screen->ChatIndex; i < screen->ChatIndex + Game_ChatLines; i++) {
		if (i >= 0 && i < Chat_Log.Count) {
			String msg = StringsBuffer_UNSAFE_Get(&Chat_Log, i);
			TextGroupWidget_PushUpAndReplaceLast(&screen->Chat, &msg);
		}
	}
}

static void ChatScreen_ConstructWidgets(struct ChatScreen* screen) {
#define ChatScreen_MakeGroup(widget, lines, textures, buffer) TextGroupWidget_Create(widget, lines, &screen->ChatFont, &screen->ChatUrlFont, textures, buffer);
	Int32 yOffset = ChatScreen_BottomOffset() + 15;

	ChatInputWidget_Create(&screen->Input, &screen->ChatFont);
	Widget_SetLocation((struct Widget*)(&screen->Input.Base), ANCHOR_MIN, ANCHOR_MAX, 5, 5);

	SpecialInputWidget_Create(&screen->AltText, &screen->ChatFont, &screen->Input.Base);
	Elem_Init(&screen->AltText);
	ChatScreen_UpdateAltTextY(screen);

	ChatScreen_MakeGroup(&screen->Status, CHATSCREEN_MAX_STATUS, screen->Status_Textures, screen->Status_Buffer);
	Widget_SetLocation((struct Widget*)(&screen->Status), ANCHOR_MAX, ANCHOR_MIN, 0, 0);
	Elem_Init(&screen->Status);
	TextGroupWidget_SetUsePlaceHolder(&screen->Status, 0, false);
	TextGroupWidget_SetUsePlaceHolder(&screen->Status, 1, false);

	ChatScreen_MakeGroup(&screen->BottomRight, CHATSCREEN_MAX_GROUP, screen->BottomRight_Textures, screen->BottomRight_Buffer);
	Widget_SetLocation((struct Widget*)(&screen->BottomRight), ANCHOR_MAX, ANCHOR_MAX, 0, yOffset);
	Elem_Init(&screen->BottomRight);

	ChatScreen_MakeGroup(&screen->Chat, Game_ChatLines, screen->Chat_Textures, screen->Chat_Buffer);
	Widget_SetLocation((struct Widget*)(&screen->Chat), ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Elem_Init(&screen->Chat);

	ChatScreen_MakeGroup(&screen->ClientStatus, CHATSCREEN_MAX_GROUP, screen->ClientStatus_Textures, screen->ClientStatus_Buffer);
	Widget_SetLocation((struct Widget*)(&screen->ClientStatus), ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Elem_Init(&screen->ClientStatus);

	String empty = String_MakeNull();
	TextWidget_Create(&screen->Announcement, &empty, &screen->AnnouncementFont);
	Widget_SetLocation((struct Widget*)(&screen->Announcement), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -Game_Height / 4);
}

static void ChatScreen_SetInitialMessages(struct ChatScreen* screen) {
	screen->ChatIndex = Chat_Log.Count - Game_ChatLines;
	ChatScreen_ResetChat(screen);
	String msg;
#define ChatScreen_Set(group, idx, src) msg = String_FromRawArray(src.Buffer); TextGroupWidget_SetText(group, idx, &msg);

	ChatScreen_Set(&screen->Status, 2, Chat_Status[0]);
	ChatScreen_Set(&screen->Status, 3, Chat_Status[1]);
	ChatScreen_Set(&screen->Status, 4, Chat_Status[2]);

	ChatScreen_Set(&screen->BottomRight, 2, Chat_BottomRight[0]);
	ChatScreen_Set(&screen->BottomRight, 1, Chat_BottomRight[1]);
	ChatScreen_Set(&screen->BottomRight, 0, Chat_BottomRight[2]);

	msg = String_FromRawArray(Chat_Announcement.Buffer);
	TextWidget_SetText(&screen->Announcement, &msg);

	Int32 i;
	for (i = 0; i < screen->ClientStatus.LinesCount; i++) {
		ChatScreen_Set(&screen->ClientStatus, i, Chat_ClientStatus[i]);
	}

	if (screen->HandlesAllInput) {
		ChatScreen_OpenInput(screen, &screen->ChatInInputStr);
		screen->ChatInInputStr.length = 0;
	}
}

static void ChatScreen_CheckOtherStatuses(struct ChatScreen* screen) {
	struct AsyncRequest request;
	Int32 progress;
	bool hasRequest = AsyncDownloader_GetCurrent(&request, &progress);

	String id = String_FromRawArray(request.ID);
	String texPack = String_FromConst("texturePack");

	/* Is terrain / texture pack currently being downloaded? */
	if (!hasRequest || !String_Equals(&id, &texPack)) {
		if (screen->Status.Textures[1].ID) {
			String empty = String_MakeNull();
			TextGroupWidget_SetText(&screen->Status, 1, &empty);
		}
		screen->LastDownloadStatus = Int32_MinValue;
		return;
	}

	if (progress == screen->LastDownloadStatus) return;
	screen->LastDownloadStatus = progress;
	char strBuffer[STRING_SIZE];
	String str = String_FromArray(strBuffer);

	if (progress == ASYNC_PROGRESS_MAKING_REQUEST) {
		String_AppendConst(&str, "&eRetrieving texture pack..");
	} else if (progress == ASYNC_PROGRESS_FETCHING_DATA) {
		String_AppendConst(&str, "&eDownloading texture pack");
	} else if (progress >= 0 && progress <= 100) {
		String_AppendConst(&str, "&eDownloading texture pack (&7");
		String_AppendInt32(&str, progress);
		String_AppendConst(&str, "&e%)");
	}
	TextGroupWidget_SetText(&screen->Status, 1, &str);
}

static void ChatScreen_RenderBackground(struct ChatScreen* screen) {
	Int32 usedHeight = TextGroupWidget_UsedHeight(&screen->Chat);
	Int32 x = screen->Chat.X;
	Int32 y = screen->Chat.Y + screen->Chat.Height - usedHeight;

	Int32 width = max(screen->ClientStatus.Width, screen->Chat.Width);
	Int32 height = usedHeight + TextGroupWidget_UsedHeight(&screen->ClientStatus);

	if (height > 0) {
		PackedCol backCol = PACKEDCOL_CONST(0, 0, 0, 127);
		GfxCommon_Draw2DFlat(x - 5, y - 5, width + 10, height + 10, backCol);
	}
}

static void ChatScreen_UpdateChatYOffset(struct ChatScreen* screen, bool force) {
	Int32 height = ChatScreen_InputUsedHeight(screen);
	if (force || height != screen->InputOldHeight) {
		Int32 bottomOffset = ChatScreen_BottomOffset() + 15;
		screen->ClientStatus.YOffset = max(bottomOffset, height);
		Widget_Reposition(&screen->ClientStatus);

		screen->Chat.YOffset = screen->ClientStatus.YOffset + TextGroupWidget_UsedHeight(&screen->ClientStatus);
		Widget_Reposition(&screen->Chat);
		screen->InputOldHeight = height;
	}
}

static void ChatElem_Recreate(struct TextGroupWidget* group, char code) {
	Int32 i, j;
	char lineBuffer[TEXTGROUPWIDGET_LEN];
	String line = String_FromArray(lineBuffer);

	for (i = 0; i < group->LinesCount; i++) {
		TextGroupWidget_GetText(group, i, &line);
		if (!line.length) continue;

		for (j = 0; j < line.length - 1; j++) {
			if (line.buffer[j] == '&' && line.buffer[j + 1] == code) {
				TextGroupWidget_SetText(group, i, &line); 
				break;
			}
		}
	}
}

static Int32 ChatScreen_ClampIndex(Int32 index) {
	Int32 maxIndex = Chat_Log.Count - Game_ChatLines;
	Int32 minIndex = min(0, maxIndex);
	Math_Clamp(index, minIndex, maxIndex);
	return index;
}

static void ChatScreen_ScrollHistoryBy(struct ChatScreen* screen, Int32 delta) {
	Int32 newIndex = ChatScreen_ClampIndex(screen->ChatIndex + delta);
	if (newIndex == screen->ChatIndex) return;

	screen->ChatIndex = newIndex;
	ChatScreen_ResetChat(screen);
}

static bool ChatScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct ChatScreen* screen = (struct ChatScreen*)elem;
	screen->SuppressNextPress = false;
	if (screen->HandlesAllInput) { /* text input bar */
		if (key == KeyBind_Get(KeyBind_SendChat) || key == Key_KeypadEnter || key == KeyBind_Get(KeyBind_PauseOrExit)) {
			ChatScreen_SetHandlesAllInput(screen, false);
			Key_KeyRepeat = false;

			if (key == KeyBind_Get(KeyBind_PauseOrExit)) {
				InputWidget_Clear(&screen->Input.Base);
			}
			elem = (struct GuiElem*)(&screen->Input);
			screen->Input.Base.OnPressedEnter(elem);
			SpecialInputWidget_SetActive(&screen->AltText, false);

			/* Reset chat when user has scrolled up in chat history */
			Int32 defaultIndex = Chat_Log.Count - Game_ChatLines;
			if (screen->ChatIndex != defaultIndex) {
				screen->ChatIndex = ChatScreen_ClampIndex(defaultIndex);
				ChatScreen_ResetChat(screen);
			}
		} else if (key == Key_PageUp) {
			ChatScreen_ScrollHistoryBy(screen, -Game_ChatLines);
		} else if (key == Key_PageDown) {
			ChatScreen_ScrollHistoryBy(screen, +Game_ChatLines);
		} else {
			Elem_HandlesKeyDown(&screen->Input.Base, key);
			ChatScreen_UpdateAltTextY(screen);
		}
		return key < Key_F1 || key > Key_F35;
	}

	if (key == KeyBind_Get(KeyBind_Chat)) {
		String empty = String_MakeNull();
		ChatScreen_OpenInput(screen, &empty);
	} else if (key == Key_Slash) {
		String slash = String_FromConst("/");
		ChatScreen_OpenInput(screen, &slash);
	} else {
		return false;
	}
	return true;
}

static bool ChatScreen_HandlesKeyUp(struct GuiElem* elem, Key key) {
	struct ChatScreen* screen = (struct ChatScreen*)elem;
	if (!screen->HandlesAllInput) return false;

	if (ServerConnection_SupportsFullCP437 && key == KeyBind_Get(KeyBind_ExtInput)) {
		if (Window_Focused) {
			bool active = !screen->AltText.Active;
			SpecialInputWidget_SetActive(&screen->AltText, active);
		}
	}
	return true;
}

static bool ChatScreen_HandlesKeyPress(struct GuiElem* elem, UInt8 key) {
	struct ChatScreen* screen = (struct ChatScreen*)elem;
	if (!screen->HandlesAllInput) return false;
	if (screen->SuppressNextPress) {
		screen->SuppressNextPress = false;
		return false;
	}

	bool handled = Elem_HandlesKeyPress(&screen->Input.Base, key);
	ChatScreen_UpdateAltTextY(screen);
	return handled;
}

static bool ChatScreen_HandlesMouseScroll(struct GuiElem* elem, Real32 delta) {
	struct ChatScreen* screen = (struct ChatScreen*)elem;
	if (!screen->HandlesAllInput) return false;

	Int32 steps = Utils_AccumulateWheelDelta(&screen->ChatAcc, delta);
	ChatScreen_ScrollHistoryBy(screen, -steps);
	return true;
}

static bool ChatScreen_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct ChatScreen* screen = (struct ChatScreen*)elem;
	if (!screen->HandlesAllInput || Game_HideGui) return false;

	if (!Widget_Contains((struct Widget*)(&screen->Chat), x, y)) {
		if (screen->AltText.Active && Widget_Contains((struct Widget*)(&screen->AltText), x, y)) {
			Elem_HandlesMouseDown(&screen->AltText, x, y, btn);
			ChatScreen_UpdateAltTextY(screen);
			return true;
		}
		Elem_HandlesMouseDown(&screen->Input.Base, x, y, btn);
		return true;
	}

	Int32 height = TextGroupWidget_UsedHeight(&screen->Chat);
	Int32 chatY = screen->Chat.Y + screen->Chat.Height - height;
	if (!Gui_Contains(screen->Chat.X, chatY, screen->Chat.Width, height, x, y)) return false;

	char textBuffer[TEXTGROUPWIDGET_LEN];
	String text = String_FromArray(textBuffer);
	TextGroupWidget_GetSelected(&screen->Chat, &text, x, y);
	if (!text.length) return false;

	if (Utils_IsUrlPrefix(&text, 0)) {
		struct Screen* overlay = UrlWarningOverlay_MakeInstance(&text);
		Gui_ShowOverlay(overlay, false);
	} else if (Game_ClickableChat) {
		InputWidget_AppendString(&screen->Input.Base, &text);
	}
	return true;
}

static void ChatScreen_ColCodeChanged(void* obj, Int32 code) {
	struct ChatScreen* screen = (struct ChatScreen*)obj;
	if (Gfx_LostContext) return;

	SpecialInputWidget_UpdateCols(&screen->AltText);
	ChatElem_Recreate(&screen->Chat, code);
	ChatElem_Recreate(&screen->Status, code);
	ChatElem_Recreate(&screen->BottomRight, code);
	ChatElem_Recreate(&screen->ClientStatus, code);

	/* Some servers have plugins that redefine colours constantly */
	/* Preserve caret accumulator so caret blinking stays consistent */
	Real64 caretAcc = screen->Input.Base.CaretAccumulator;
	Elem_Recreate(&screen->Input.Base);
	screen->Input.Base.CaretAccumulator = caretAcc;
}

static void ChatScreen_ChatReceived(void* obj, String* msg, Int32 type) {
	if (Gfx_LostContext) return;
	struct ChatScreen* screen = (struct ChatScreen*)obj;

	if (type == MSG_TYPE_NORMAL) {
		screen->ChatIndex++;
		if (!Game_ChatLines) return;

		Int32 i = screen->ChatIndex + (Game_ChatLines - 1);
		String chatMsg = *msg;

		if (i < Chat_Log.Count) { chatMsg = StringsBuffer_UNSAFE_Get(&Chat_Log, i); }
		TextGroupWidget_PushUpAndReplaceLast(&screen->Chat, &chatMsg);
	} else if (type >= MSG_TYPE_STATUS_1 && type <= MSG_TYPE_STATUS_3) {
		TextGroupWidget_SetText(&screen->Status, 2 + (type - MSG_TYPE_STATUS_1), msg);
	} else if (type >= MSG_TYPE_BOTTOMRIGHT_1 && type <= MSG_TYPE_BOTTOMRIGHT_3) {
		TextGroupWidget_SetText(&screen->BottomRight, 2 - (type - MSG_TYPE_BOTTOMRIGHT_1), msg);
	} else if (type == MSG_TYPE_ANNOUNCEMENT) {
		TextWidget_SetText(&screen->Announcement, msg);
	} else if (type >= MSG_TYPE_CLIENTSTATUS_1 && type <= MSG_TYPE_CLIENTSTATUS_3) {
		TextGroupWidget_SetText(&screen->ClientStatus, type - MSG_TYPE_CLIENTSTATUS_1, msg);
		ChatScreen_UpdateChatYOffset(screen, true);
	}
}

static void ChatScreen_ContextLost(void* obj) {
	struct ChatScreen* screen = (struct ChatScreen*)obj;
	if (screen->HandlesAllInput) {
		String_Copy(&screen->ChatInInputStr, &screen->Input.Base.Text);
		Gui_CalcCursorVisible();
	}

	Elem_TryFree(&screen->Chat);
	Elem_TryFree(&screen->Input.Base);
	Elem_TryFree(&screen->AltText);
	Elem_TryFree(&screen->Status);
	Elem_TryFree(&screen->BottomRight);
	Elem_TryFree(&screen->ClientStatus);
	Elem_TryFree(&screen->Announcement);
}

static void ChatScreen_ContextRecreated(void* obj) {
	struct ChatScreen* screen = (struct ChatScreen*)obj;
	ChatScreen_ConstructWidgets(screen);
	ChatScreen_SetInitialMessages(screen);
}

static void ChatScreen_FontChanged(void* obj) {
	struct ChatScreen* screen = (struct ChatScreen*)obj;
	if (!Drawer2D_UseBitmappedChat) return;
	Elem_Recreate(screen);
	ChatScreen_UpdateChatYOffset(screen, true);
}

static void ChatScreen_OnResize(struct GuiElem* elem) {
	struct ChatScreen* screen = (struct ChatScreen*)elem;
	bool active = screen->AltText.Active;
	Elem_Recreate(screen);
	SpecialInputWidget_SetActive(&screen->AltText, active);
}

static void ChatScreen_Init(struct GuiElem* elem) {
	struct ChatScreen* screen = (struct ChatScreen*)elem;
	String str = String_FromArray(screen->__ChatInInputBuffer);
	screen->ChatInInputStr = str;

	Int32 fontSize = (Int32)(8 * Game_GetChatScale());
	Math_Clamp(fontSize, 8, 60);
	Font_Make(&screen->ChatFont, &Game_FontName, fontSize, FONT_STYLE_NORMAL);
	Font_Make(&screen->ChatUrlFont, &Game_FontName, fontSize, FONT_STYLE_UNDERLINE);

	fontSize = (Int32)(16 * Game_GetChatScale());
	Math_Clamp(fontSize, 8, 60);
	Font_Make(&screen->AnnouncementFont, &Game_FontName, fontSize, FONT_STYLE_NORMAL);
	ChatScreen_ContextRecreated(elem);

	Event_RegisterChat(&ChatEvents_ChatReceived,    screen, ChatScreen_ChatReceived);
	Event_RegisterVoid(&ChatEvents_FontChanged,     screen, ChatScreen_FontChanged);
	Event_RegisterInt(&ChatEvents_ColCodeChanged, screen, ChatScreen_ColCodeChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, ChatScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, ChatScreen_ContextRecreated);
}

static void ChatScreen_Render(struct GuiElem* elem, Real64 delta) {
	struct ChatScreen* screen = (struct ChatScreen*)elem;
	ChatScreen_CheckOtherStatuses(screen);
	if (!Game_PureClassic) { Elem_Render(&screen->Status, delta); }
	Elem_Render(&screen->BottomRight, delta);

	ChatScreen_UpdateChatYOffset(screen, false);
	Int32 i, y = screen->ClientStatus.Y + screen->ClientStatus.Height;
	for (i = 0; i < screen->ClientStatus.LinesCount; i++) {
		struct Texture tex = screen->ClientStatus.Textures[i];
		if (!tex.ID) continue;

		y -= tex.Height; tex.Y = y;
		Texture_Render(&tex);
	}

	DateTime now; DateTime_CurrentUTC(&now);
	if (screen->HandlesAllInput) {
		Elem_Render(&screen->Chat, delta);
	} else {
		Int64 nowMS = DateTime_TotalMs(&now);
		/* Only render recent chat */
		for (i = 0; i < screen->Chat.LinesCount; i++) {
			struct Texture tex = screen->Chat.Textures[i];
			Int32 logIdx = screen->ChatIndex + i;
			if (!tex.ID) continue;
			if (logIdx < 0 || logIdx >= Chat_Log.Count) continue;

			Int64 received; Chat_GetLogTime(logIdx, &received);
			if ((nowMS - received) <= 10 * 1000) Texture_Render(&tex);
		}
	}

	Elem_Render(&screen->Announcement, delta);
	if (screen->HandlesAllInput) {
		Elem_Render(&screen->Input.Base, delta);
		if (screen->AltText.Active) {
			Elem_Render(&screen->AltText, delta);
		}
	}

	if (screen->Announcement.Texture.ID && DateTime_MsBetween(&Chat_Announcement.Received, &now) > 5 * 1000) {
		Elem_TryFree(&screen->Announcement);
	}
}

static void ChatScreen_Free(struct GuiElem* elem) {
	struct ChatScreen* screen = (struct ChatScreen*)elem;
	ChatScreen_ContextLost(elem);
	Font_Free(&screen->ChatFont);
	Font_Free(&screen->ChatUrlFont);
	Font_Free(&screen->AnnouncementFont);

	Event_UnregisterChat(&ChatEvents_ChatReceived,    screen, ChatScreen_ChatReceived);
	Event_UnregisterVoid(&ChatEvents_FontChanged,     screen, ChatScreen_FontChanged);
	Event_UnregisterInt(&ChatEvents_ColCodeChanged, screen, ChatScreen_ColCodeChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, ChatScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, ChatScreen_ContextRecreated);
}

struct Screen* ChatScreen_MakeInstance(void) {
	struct ChatScreen* screen = &ChatScreen_Instance;
	Mem_Set(screen, 0, sizeof(struct ChatScreen));
	screen->VTABLE = &ChatScreen_VTABLE;
	Screen_Reset((struct Screen*)screen);
	screen->HandlesAllInput = false;

	screen->InputOldHeight = -1;
	screen->LastDownloadStatus = Int32_MinValue;
	screen->HUD = Gui_HUD;

	screen->OnResize       = ChatScreen_OnResize;
	screen->VTABLE->Init   = ChatScreen_Init;
	screen->VTABLE->Render = ChatScreen_Render;
	screen->VTABLE->Free   = ChatScreen_Free;

	screen->VTABLE->HandlesKeyDown     = ChatScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp       = ChatScreen_HandlesKeyUp;
	screen->VTABLE->HandlesKeyPress    = ChatScreen_HandlesKeyPress;
	screen->VTABLE->HandlesMouseDown   = ChatScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseScroll = ChatScreen_HandlesMouseScroll;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*--------------------------------------------------------HUDScreen--------------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE HUDScreenVTABLE;
struct HUDScreen HUDScreen_Instance;
#define CH_EXTENT 16

static void HUDScreen_DrawCrosshairs(void) {
	if (!Gui_IconsTex) return;

	Int32 extent = (Int32)(CH_EXTENT * Game_Scale(Game_Height / 480.0f)), size = extent * 2;
	Int32 chX = (Game_Width / 2) - extent, chY = (Game_Height / 2) - extent;

	struct Texture tex = { Gui_IconsTex, TEX_RECT(chX,chY, size, size), TEX_UV(0.0f,0.0f, 15/256.0f,15/256.0f) };
	Texture_Render(&tex);
}

static void HUDScreen_ContextLost(void* obj) {
	struct HUDScreen* screen = (struct HUDScreen*)obj;
	Elem_TryFree(&screen->Hotbar);

	screen->WasShowingList = screen->ShowingList;
	if (screen->ShowingList) { Elem_TryFree(&screen->PlayerList); }
	screen->ShowingList = false;
}

static void HUDScreen_ContextRecreated(void* obj) {
	struct HUDScreen* screen = (struct HUDScreen*)obj;
	Elem_TryFree(&screen->Hotbar);
	Elem_Init(&screen->Hotbar);

	if (!screen->WasShowingList) return;
	bool extended = ServerConnection_SupportsExtPlayerList && !Game_UseClassicTabList;
	PlayerListWidget_Create(&screen->PlayerList, &screen->PlayerFont, !extended);
	screen->ShowingList = true;

	Elem_Init(&screen->PlayerList);
	Widget_Reposition(&screen->PlayerList);
}


static void HUDScreen_OnResize(struct GuiElem* elem) {
	struct HUDScreen* screen = (struct HUDScreen*)elem;
	screen->Chat->OnResize((struct GuiElem*)screen->Chat);

	Widget_Reposition(&screen->Hotbar);
	if (screen->ShowingList) {
		Widget_Reposition(&screen->PlayerList);
	}
}

static bool HUDScreen_HandlesKeyPress(struct GuiElem* elem, UInt8 key) {
	struct HUDScreen* screen = (struct HUDScreen*)elem;
	return Elem_HandlesKeyPress(screen->Chat, key); 
}

static bool HUDScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct HUDScreen* screen = (struct HUDScreen*)elem;
	Key playerListKey = KeyBind_Get(KeyBind_PlayerList);
	bool handles = playerListKey != Key_Tab || !Game_TabAutocomplete || !screen->Chat->HandlesAllInput;

	if (key == playerListKey && handles) {
		if (!screen->ShowingList && !ServerConnection_IsSinglePlayer) {
			screen->WasShowingList = true;
			HUDScreen_ContextRecreated(screen);
		}
		return true;
	}

	return Elem_HandlesKeyDown(screen->Chat, key) || Elem_HandlesKeyDown(&screen->Hotbar, key);
}

static bool HUDScreen_HandlesKeyUp(struct GuiElem* elem, Key key) {
	struct HUDScreen* screen = (struct HUDScreen*)elem;
	if (key == KeyBind_Get(KeyBind_PlayerList)) {
		if (screen->ShowingList) {
			screen->ShowingList = false;
			screen->WasShowingList = false;
			Elem_TryFree(&screen->PlayerList);
			return true;
		}
	}

	return Elem_HandlesKeyUp(screen->Chat, key) || Elem_HandlesKeyUp(&screen->Hotbar, key);
}

static bool HUDScreen_HandlesMouseScroll(struct GuiElem* elem, Real32 delta) {
	struct HUDScreen* screen = (struct HUDScreen*)elem;
	return Elem_HandlesMouseScroll(screen->Chat, delta);
}

static bool HUDScreen_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct HUDScreen* screen = (struct HUDScreen*)elem;
	if (btn != MouseButton_Left || !screen->HandlesAllInput) return false;

	elem = (struct GuiElem*)screen->Chat;
	if (!screen->ShowingList) { return elem->VTABLE->HandlesMouseDown(elem, x, y, btn); }

	char nameBuffer[STRING_SIZE + 1];
	String name = String_FromArray(nameBuffer);
	PlayerListWidget_GetNameUnder(&screen->PlayerList, x, y, &name);
	if (!name.length) { return elem->VTABLE->HandlesMouseDown(elem, x, y, btn); }

	String_Append(&name, ' ');
	struct ChatScreen* chat = (struct ChatScreen*)screen->Chat;
	if (chat->HandlesAllInput) { InputWidget_AppendString(&chat->Input.Base, &name); }
	return true;
}

static void HUDScreen_Init(struct GuiElem* elem) {
	struct HUDScreen* screen = (struct HUDScreen*)elem;
	UInt16 size = Drawer2D_UseBitmappedChat ? 16 : 11;
	Font_Make(&screen->PlayerFont, &Game_FontName, size, FONT_STYLE_NORMAL);

	HotbarWidget_Create(&screen->Hotbar);
	Elem_Init(&screen->Hotbar);

	ChatScreen_MakeInstance();
	screen->Chat = (struct Screen*)&ChatScreen_Instance;
	Elem_Init(screen->Chat);

	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, HUDScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, HUDScreen_ContextRecreated);
}

static void HUDScreen_Render(struct GuiElem* elem, Real64 delta) {
	struct HUDScreen* screen = (struct HUDScreen*)elem;
	struct ChatScreen* chat = (struct ChatScreen*)screen->Chat;
	if (Game_HideGui && chat->HandlesAllInput) {
		Gfx_SetTexturing(true);
		Elem_Render(&chat->Input.Base, delta);
		Gfx_SetTexturing(false);
	}
	if (Game_HideGui) return;
	bool showMinimal = Gui_GetActiveScreen()->BlocksWorld;

	if (!screen->ShowingList && !showMinimal) {
		Gfx_SetTexturing(true);
		HUDScreen_DrawCrosshairs();
		Gfx_SetTexturing(false);
	}
	if (chat->HandlesAllInput && !Game_PureClassic) {
		ChatScreen_RenderBackground(chat);
	}

	Gfx_SetTexturing(true);
	if (!showMinimal) { Elem_Render(&screen->Hotbar, delta); }
	Elem_Render(screen->Chat, delta);

	if (screen->ShowingList && Gui_GetActiveScreen() == (struct Screen*)screen) {
		screen->PlayerList.Active = screen->Chat->HandlesAllInput;
		Elem_Render(&screen->PlayerList, delta);
		/* NOTE: Should usually be caught by KeyUp, but just in case. */
		if (!KeyBind_IsPressed(KeyBind_PlayerList)) {
			Elem_TryFree(&screen->PlayerList);
			screen->ShowingList = false;
		}
	}
	Gfx_SetTexturing(false);
}

static void HUDScreen_Free(struct GuiElem* elem) {
	struct HUDScreen* screen = (struct HUDScreen*)elem;
	Font_Free(&screen->PlayerFont);
	Elem_TryFree(screen->Chat);
	HUDScreen_ContextLost(screen);

	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, HUDScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, HUDScreen_ContextRecreated);
}

struct Screen* HUDScreen_MakeInstance(void) {
	struct HUDScreen* screen = &HUDScreen_Instance;
	Mem_Set(screen, 0, sizeof(struct HUDScreen));
	screen->VTABLE = &HUDScreenVTABLE;
	Screen_Reset((struct Screen*)screen);
	screen->HandlesAllInput = false;

	screen->OnResize       = HUDScreen_OnResize;
	screen->VTABLE->Init   = HUDScreen_Init;
	screen->VTABLE->Render = HUDScreen_Render;
	screen->VTABLE->Free   = HUDScreen_Free;

	screen->VTABLE->HandlesKeyDown     = HUDScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp       = HUDScreen_HandlesKeyUp;
	screen->VTABLE->HandlesKeyPress    = HUDScreen_HandlesKeyPress;
	screen->VTABLE->HandlesMouseDown   = HUDScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseScroll = HUDScreen_HandlesMouseScroll;
	return (struct Screen*)screen;
}

static void HUDScreen_Ready(void) { Elem_Init(&HUDScreen_Instance); }
void HUDScreen_MakeComponent(struct IGameComponent* comp) {
	comp->Ready = HUDScreen_Ready;
}

void HUDScreen_OpenInput(struct Screen* hud, STRING_PURE String* text) {
	struct Screen* chat = ((struct HUDScreen*)hud)->Chat;
	ChatScreen_OpenInput((struct ChatScreen*)chat, text);
}

void HUDScreen_AppendInput(struct Screen* hud, STRING_PURE String* text) {
	struct Screen* chat = ((struct HUDScreen*)hud)->Chat;
	struct ChatInputWidget* widget = &((struct ChatScreen*)chat)->Input;
	InputWidget_AppendString(&widget->Base, text);
}

struct Widget* HUDScreen_GetHotbar(struct Screen* hud) {
	struct HUDScreen* screen = (struct HUDScreen*)hud;
	return (struct Widget*)(&screen->Hotbar);
}


/*########################################################################################################################*
*----------------------------------------------------DisconnectScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE DisconnectScreen_VTABLE;
struct DisconnectScreen DisconnectScreen_Instance;
#define DISCONNECT_DELAY_MS 5000
static void DisconnectScreen_ReconnectMessage(struct DisconnectScreen* screen, STRING_TRANSIENT String* msg) {
	if (screen->CanReconnect) {
		DateTime now; DateTime_CurrentUTC(&now);
		Int32 elapsedMS = (Int32)(DateTime_TotalMs(&now) - screen->InitTime);
		Int32 secsLeft = (DISCONNECT_DELAY_MS - elapsedMS) / DATETIME_MILLISECS_PER_SECOND;

		if (secsLeft > 0) {
			String_Format1(msg, "Reconnect in %i", &secsLeft);
			return;
		}
	}
	String_AppendConst(msg, "Reconnect");
}

static void DisconnectScreen_Redraw(struct DisconnectScreen* screen, Real64 delta) {
	PackedCol top    = PACKEDCOL_CONST(64, 32, 32, 255);
	PackedCol bottom = PACKEDCOL_CONST(80, 16, 16, 255);
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, top, bottom);

	Gfx_SetTexturing(true);
	Elem_Render(&screen->Title, delta);
	Elem_Render(&screen->Message, delta);
	if (screen->CanReconnect) { Elem_Render(&screen->Reconnect, delta); }
	Gfx_SetTexturing(false);
}

static void DisconnectScreen_UpdateDelayLeft(struct DisconnectScreen* screen, Real64 delta) {
	DateTime now; DateTime_CurrentUTC(&now);
	Int32 elapsedMS = (Int32)(DateTime_TotalMs(&now) - screen->InitTime);
	Int32 secsLeft = (DISCONNECT_DELAY_MS - elapsedMS) / DATETIME_MILLISECS_PER_SECOND;
	if (secsLeft < 0) secsLeft = 0;
	if (screen->LastSecsLeft == secsLeft && screen->Reconnect.Active == screen->LastActive) return;

	char msgBuffer[STRING_SIZE];
	String msg = String_FromArray(msgBuffer);
	DisconnectScreen_ReconnectMessage(screen, &msg);
	ButtonWidget_SetText(&screen->Reconnect, &msg);
	screen->Reconnect.Disabled = secsLeft != 0;

	DisconnectScreen_Redraw(screen, delta);
	screen->LastSecsLeft = secsLeft;
	screen->LastActive   = screen->Reconnect.Active;
	screen->ClearTime = DateTime_TotalMs(&now) + 500;
}

static void DisconnectScreen_ContextLost(void* obj) {
	struct DisconnectScreen* screen = (struct DisconnectScreen*)obj;
	Elem_TryFree(&screen->Title);
	Elem_TryFree(&screen->Message);
	Elem_TryFree(&screen->Reconnect);
}

static void DisconnectScreen_ContextRecreated(void* obj) {
	struct DisconnectScreen* screen = (struct DisconnectScreen*)obj;
	if (Gfx_LostContext) return;
	DateTime now; DateTime_CurrentUTC(&now);
	screen->ClearTime = DateTime_TotalMs(&now) + 500;

	TextWidget_Create(&screen->Title, &screen->TitleStr, &screen->TitleFont);
	Widget_SetLocation((struct Widget*)(&screen->Title), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);

	TextWidget_Create(&screen->Message, &screen->MessageStr, &screen->MessageFont);
	Widget_SetLocation((struct Widget*)(&screen->Message), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 10);

	char msgBuffer[STRING_SIZE];
	String msg = String_FromArray(msgBuffer);
	DisconnectScreen_ReconnectMessage(screen, &msg);

	ButtonWidget_Create(&screen->Reconnect, 300, &msg, &screen->TitleFont, NULL);
	Widget_SetLocation((struct Widget*)(&screen->Reconnect), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 80);
	screen->Reconnect.Disabled = !screen->CanReconnect;
}

static void DisconnectScreen_Init(struct GuiElem* elem) {
	struct DisconnectScreen* screen = (struct DisconnectScreen*)elem;
	Game_SkipClear = true;
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, DisconnectScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, DisconnectScreen_ContextRecreated);

	DisconnectScreen_ContextRecreated(screen);
	DateTime now; DateTime_CurrentUTC(&now);
	screen->InitTime = DateTime_TotalMs(&now);
	screen->LastSecsLeft = DISCONNECT_DELAY_MS / DATETIME_MILLISECS_PER_SECOND;
}

static void DisconnectScreen_Render(struct GuiElem* elem, Real64 delta) {
	struct DisconnectScreen* screen = (struct DisconnectScreen*)elem;
	if (screen->CanReconnect) {
		DisconnectScreen_UpdateDelayLeft(screen, delta);
	}

	/* NOTE: We need to make sure that both the front and back buffers have
	definitely been drawn over, so we redraw the background multiple times. */
	DateTime now; DateTime_CurrentUTC(&now);
	if (DateTime_TotalMs(&now) < screen->ClearTime) {
		DisconnectScreen_Redraw(screen, delta);
	}
}

static void DisconnectScreen_Free(struct GuiElem* elem) {
	struct DisconnectScreen* screen = (struct DisconnectScreen*)elem;
	Game_SkipClear = false;
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, DisconnectScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, DisconnectScreen_ContextRecreated);

	DisconnectScreen_ContextLost(screen);
	Font_Free(&screen->TitleFont);
	Font_Free(&screen->MessageFont);
}

static void DisconnectScreen_OnResize(struct GuiElem* elem) {
	struct DisconnectScreen* screen = (struct DisconnectScreen*)elem;
	Widget_Reposition(&screen->Title);
	Widget_Reposition(&screen->Message);
	Widget_Reposition(&screen->Reconnect);
	DateTime now; DateTime_CurrentUTC(&now);
	screen->ClearTime = DateTime_TotalMs(&now) + 500;
}

static bool DisconnectScreen_HandlesKeyDown(struct GuiElem* elem, Key key) { return key < Key_F1 || key > Key_F35; }
static bool DisconnectScreen_HandlesKeyPress(struct GuiElem* elem, UInt8 keyChar) { return true; }
static bool DisconnectScreen_HandlesKeyUp(struct GuiElem* elem, Key key) { return true; }

static bool DisconnectScreen_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct DisconnectScreen* screen = (struct DisconnectScreen*)elem;
	struct ButtonWidget* widget = &screen->Reconnect;
	if (btn != MouseButton_Left) return true;

	if (!widget->Disabled && Widget_Contains((struct Widget*)widget, x, y)) {
		char connectBuffer[STRING_SIZE];
		String title   = String_FromArray(connectBuffer);
		String message = String_MakeNull();
		String_Format2(&title, "Connecting to %s:%i..", &Game_IPAddress, &Game_Port);

		Gui_ReplaceActive(LoadingScreen_MakeInstance(&title, &message));
		ServerConnection_BeginConnect();
	}
	return true;
}

static bool DisconnectScreen_HandlesMouseMove(struct GuiElem* elem, Int32 x, Int32 y) {
	struct DisconnectScreen* screen = (struct DisconnectScreen*)elem;
	struct ButtonWidget* widget = &screen->Reconnect;
	widget->Active = !widget->Disabled && Widget_Contains((struct Widget*)widget, x, y);
	return true;
}

static bool DisconnectScreen_HandlesMouseScroll(struct GuiElem* elem, Real32 delta) { return true; }
static bool DisconnectScreen_HandlesMouseUp(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) { return true; }

struct Screen* DisconnectScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message) {
	struct DisconnectScreen* screen = &DisconnectScreen_Instance;
	Mem_Set(screen, 0, sizeof(struct DisconnectScreen));
	screen->VTABLE = &DisconnectScreen_VTABLE;
	Screen_Reset((struct Screen*)screen);

	String title_copy = String_FromArray(screen->__TitleBuffer);
	String_AppendString(&title_copy, title);
	screen->TitleStr = title_copy;

	String message_copy = String_FromArray(screen->__MessageBuffer);
	String_AppendString(&message_copy, message);
	screen->MessageStr = message_copy;

	char whyBuffer[STRING_SIZE];
	String why = String_FromArray(whyBuffer);
	String_AppendColorless(&why, message);

	String kick = String_FromConst("Kicked ");
	String ban  = String_FromConst("Banned ");
	screen->CanReconnect = 
		!(String_CaselessStarts(&why, &kick) || String_CaselessStarts(&why, &ban));

	Font_Make(&screen->TitleFont,   &Game_FontName, 16, FONT_STYLE_BOLD);
	Font_Make(&screen->MessageFont, &Game_FontName, 16, FONT_STYLE_NORMAL);
	screen->BlocksWorld     = true;
	screen->HidesHUD        = true;

	screen->OnResize       = DisconnectScreen_OnResize;
	screen->VTABLE->Init   = DisconnectScreen_Init;
	screen->VTABLE->Render = DisconnectScreen_Render;
	screen->VTABLE->Free   = DisconnectScreen_Free;

	screen->VTABLE->HandlesKeyDown     = DisconnectScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp       = DisconnectScreen_HandlesKeyUp;
	screen->VTABLE->HandlesKeyPress    = DisconnectScreen_HandlesKeyPress;
	screen->VTABLE->HandlesMouseDown   = DisconnectScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseMove   = DisconnectScreen_HandlesMouseMove;
	screen->VTABLE->HandlesMouseScroll = DisconnectScreen_HandlesMouseScroll;
	screen->VTABLE->HandlesMouseUp     = DisconnectScreen_HandlesMouseUp;
	return (struct Screen*)screen;
}
