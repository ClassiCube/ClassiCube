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


typedef struct InventoryScreen_ {
	Screen_Layout
	FontDesc Font;
	TableWidget Table;
	bool ReleasedInv;
} InventoryScreen;

typedef struct StatusScreen_ {
	Screen_Layout
	FontDesc Font;
	TextWidget Status, HackStates;
	TextAtlas PosAtlas;
	Real64 Accumulator;
	Int32 Frames, FPS;
	bool Speed, HalfSpeed, Noclip, Fly, CanSpeed;
	Int32 LastFov;
} StatusScreen;

typedef struct HUDScreen_ {
	Screen_Layout
	Screen* Chat;
	HotbarWidget Hotbar;
	PlayerListWidget PlayerList;
	FontDesc PlayerFont;
	bool ShowingList, WasShowingList;
} HUDScreen;

typedef struct LoadingScreen_ {
	Screen_Layout
	FontDesc Font;
	Real32 Progress;
	
	TextWidget Title, Message;
	UInt8 TitleBuffer[String_BufferSize(STRING_SIZE)];
	UInt8 MessageBuffer[String_BufferSize(STRING_SIZE)];
} LoadingScreen;

typedef struct GeneratingMapScreen_ {
	LoadingScreen Base;
	const UInt8* LastState;
} GeneratingMapScreen;

#define CHATSCREEN_MAX_STATUS 5
#define CHATSCREEN_MAX_GROUP 3
typedef struct ChatScreen_ {
	Screen_Layout
	Screen* HUD;
	Int32 InputOldHeight;
	Real32 ChatAcc;
	bool SuppressNextPress;
	Int32 ChatIndex;
	Int32 LastDownloadStatus;
	FontDesc ChatFont, ChatUrlFont, AnnouncementFont;
	TextWidget Announcement;
	ChatInputWidget Input;
	TextGroupWidget Status, BottomRight, Chat, ClientStatus;
	SpecialInputWidget AltText;

	/* needed for lost contexts, to restore chat typed in */
	UInt8 ChatInInputBuffer[String_BufferSize(INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN)];

	Texture Status_Textures[CHATSCREEN_MAX_STATUS];
	Texture BottomRight_Textures[CHATSCREEN_MAX_GROUP];
	Texture ClientStatus_Textures[CHATSCREEN_MAX_GROUP];
	Texture Chat_Textures[TEXTGROUPWIDGET_MAX_LINES];
	UInt8 Status_Buffer[String_BufferSize(CHATSCREEN_MAX_STATUS * TEXTGROUPWIDGET_LEN)];
	UInt8 BottomRight_Buffer[String_BufferSize(CHATSCREEN_MAX_GROUP * TEXTGROUPWIDGET_LEN)];
	UInt8 ClientStatus_Buffer[String_BufferSize(CHATSCREEN_MAX_GROUP * TEXTGROUPWIDGET_LEN)];
	UInt8 Chat_Buffer[String_BufferSize(TEXTGROUPWIDGET_MAX_LINES * TEXTGROUPWIDGET_LEN)];
} ChatScreen;

typedef struct DisconnectScreen_ {
	Screen_Layout
	Int64 InitTime, ClearTime;
	bool CanReconnect, LastActive;
	Int32 LastSecsLeft;
	ButtonWidget Reconnect;

	FontDesc TitleFont, MessageFont;
	TextWidget Title, Message;
	UInt8 TitleBuffer[String_BufferSize(STRING_SIZE)];
	UInt8 MessageBuffer[String_BufferSize(STRING_SIZE)];
} DisconnectScreen;


/*########################################################################################################################*
*-----------------------------------------------------InventoryScreen-----------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE InventoryScreen_VTABLE;
InventoryScreen InventoryScreen_Instance;
void InventoryScreen_OnBlockChanged(void* obj) {
	InventoryScreen* screen = (InventoryScreen*)obj;
	TableWidget_OnInventoryChanged(&screen->Table);
}

void InventoryScreen_ContextLost(void* obj) {
	InventoryScreen* screen = (InventoryScreen*)obj;
	Elem_TryFree(&screen->Table);
}

void InventoryScreen_ContextRecreated(void* obj) {
	InventoryScreen* screen = (InventoryScreen*)obj;
	Elem_Recreate(&screen->Table);
}

void InventoryScreen_Init(GuiElement* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Platform_MakeFont(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);

	TableWidget_Create(&screen->Table);
	screen->Table.Font = screen->Font;
	screen->Table.ElementsPerRow = Game_PureClassic ? 9 : 10;
	Elem_Init(&screen->Table);

	/* User is holding invalid block */
	if (screen->Table.SelectedIndex == -1) {
		TableWidget_MakeDescTex(&screen->Table, Inventory_SelectedBlock);
	}

	Key_KeyRepeat = true;
	Event_RegisterVoid(&BlockEvents_PermissionsChanged, screen, InventoryScreen_OnBlockChanged);
	Event_RegisterVoid(&BlockEvents_BlockDefChanged,    screen, InventoryScreen_OnBlockChanged);	
	Event_RegisterVoid(&GfxEvents_ContextLost,          screen, InventoryScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,     screen, InventoryScreen_ContextRecreated);
}

void InventoryScreen_Render(GuiElement* elem, Real64 delta) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Elem_Render(&screen->Table, delta);
}

void InventoryScreen_OnResize(GuiElement* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Widget_Reposition(&screen->Table);
}

void InventoryScreen_Free(GuiElement* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Platform_FreeFont(&screen->Font);
	Elem_TryFree(&screen->Table);

	Key_KeyRepeat = false;
	Event_UnregisterVoid(&BlockEvents_PermissionsChanged, screen, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&BlockEvents_BlockDefChanged,    screen, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,          screen, InventoryScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,     screen, InventoryScreen_ContextRecreated);
}

bool InventoryScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;

	if (key == KeyBind_Get(KeyBind_PauseOrExit)) {
		Gui_ReplaceActive(NULL);
	} else if (key == KeyBind_Get(KeyBind_Inventory) && screen->ReleasedInv) {
		Gui_ReplaceActive(NULL);
	} else if (key == Key_Enter && table->SelectedIndex != -1) {
		Inventory_SetSelectedBlock(table->Elements[table->SelectedIndex]);
		Gui_ReplaceActive(NULL);
	} else if (Elem_HandlesKeyDown(table, key)) {
	} else {
		HUDScreen* hud = (HUDScreen*)Gui_HUD;
		return Elem_HandlesKeyDown(&hud->Hotbar, key);
	}
	return true;
}

bool InventoryScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	if (key == KeyBind_Get(KeyBind_Inventory)) {
		screen->ReleasedInv = true; return true;
	}

	HUDScreen* hud = (HUDScreen*)Gui_HUD;
	return Elem_HandlesKeyUp(&hud->Hotbar, key);
}

bool InventoryScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;
	HUDScreen* hud = (HUDScreen*)Gui_HUD;
	if (table->Scroll.DraggingMouse || Elem_HandlesMouseDown(&hud->Hotbar, x, y, btn)) return true;

	bool handled = Elem_HandlesMouseDown(table, x, y, btn);
	if ((!handled || table->PendingClose) && btn == MouseButton_Left) {
		bool hotbar = Key_IsControlPressed() || Key_IsShiftPressed();
		if (!hotbar) Gui_ReplaceActive(NULL);
	}
	return true;
}

bool InventoryScreen_HandlesMouseUp(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;
	return Elem_HandlesMouseUp(table, x, y, btn);
}

bool InventoryScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;
	return Elem_HandlesMouseMove(table, x, y);
}

bool InventoryScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;

	bool hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (hotbar) return false;
	return Elem_HandlesMouseScroll(table, delta);
}

Screen* InventoryScreen_MakeInstance(void) {
	InventoryScreen* screen = &InventoryScreen_Instance;
	Platform_MemSet(screen, 0, sizeof(InventoryScreen));
	screen->VTABLE = &InventoryScreen_VTABLE;
	Screen_Reset((Screen*)screen);
	screen->HandlesAllInput = true;

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
	return (Screen*)screen;
}
extern Screen* InventoryScreen_UNSAFE_RawPointer = (Screen*)&InventoryScreen_Instance;


/*########################################################################################################################*
*-------------------------------------------------------StatusScreen------------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE StatusScreen_VTABLE;
StatusScreen StatusScreen_Instance;
void StatusScreen_MakeText(StatusScreen* screen, STRING_TRANSIENT String* status) {
	screen->FPS = (Int32)(screen->Frames / screen->Accumulator);
	String_Format1(status, "%i fps, ", &screen->FPS);

	if (Game_ClassicMode) {
		String_Format1(status, "%i chunk updates", &Game_ChunkUpdates);
	} else {
		if (Game_ChunkUpdates > 0) {
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

void StatusScreen_DrawPosition(StatusScreen* screen) {
	TextAtlas* atlas = &screen->PosAtlas;
	VertexP3fT2fC4b vertices[4 * 64];
	VertexP3fT2fC4b* ptr = vertices;

	Texture tex = atlas->Tex; tex.X = 2; tex.Width = (UInt16)atlas->Offset;
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

bool StatusScreen_HacksChanged(StatusScreen* screen) {
	HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	return hacks->Speeding != screen->Speed || hacks->HalfSpeeding != screen->HalfSpeed || hacks->Flying != screen->Fly
		|| hacks->Noclip != screen->Noclip  || Game_Fov != screen->LastFov || hacks->CanSpeed != screen->CanSpeed;
}

void StatusScreen_UpdateHackState(StatusScreen* screen) {
	HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	screen->Speed = hacks->Speeding; screen->HalfSpeed = hacks->HalfSpeeding; screen->Fly = hacks->Flying;
	screen->Noclip = hacks->Noclip; screen->LastFov = Game_Fov; screen->CanSpeed = hacks->CanSpeed;

	UInt8 statusBuffer[String_BufferSize(STRING_SIZE * 2)];
	String status = String_InitAndClearArray(statusBuffer);

	if (Game_Fov != Game_DefaultFov) {
		String_Format1(&status, "Zoom fov %i  ", &Game_Fov);
	}
	if (hacks->Flying) String_AppendConst(&status, "Fly ON   ");

	bool speeding = (hacks->Speeding || hacks->HalfSpeeding) && (hacks->CanSpeed || hacks->BaseHorSpeed > 1);
	if (speeding) String_AppendConst(&status, "Speed ON   ");
	if (hacks->Noclip) String_AppendConst(&status, "Noclip ON   ");

	TextWidget_SetText(&screen->HackStates, &status);
}

void StatusScreen_Update(StatusScreen* screen, Real64 delta) {
	screen->Frames++;
	screen->Accumulator += delta;
	if (screen->Accumulator < 1.0) return;

	UInt8 statusBuffer[String_BufferSize(STRING_SIZE * 2)];
	String status = String_InitAndClearArray(statusBuffer);
	StatusScreen_MakeText(screen, &status);

	TextWidget_SetText(&screen->Status, &status);
	screen->Accumulator = 0.0;
	screen->Frames = 0;
	Game_ChunkUpdates = 0;
}

void StatusScreen_OnResize(GuiElement* elem) { }
void StatusScreen_FontChanged(void* obj) {
	StatusScreen* screen = (StatusScreen*)obj;
	Elem_Recreate(screen);
}

void StatusScreen_ContextLost(void* obj) {
	StatusScreen* screen = (StatusScreen*)obj;
	TextAtlas_Free(&screen->PosAtlas);
	Elem_TryFree(&screen->Status);
	Elem_TryFree(&screen->HackStates);
}

void StatusScreen_ContextRecreated(void* obj) {
	StatusScreen* screen = (StatusScreen*)obj;

	TextWidget* status = &screen->Status; TextWidget_Make(status, &screen->Font);
	Widget_SetLocation((Widget*)status, ANCHOR_MIN, ANCHOR_MIN, 2, 2);
	status->ReducePadding = true;
	Elem_Init(status);
	StatusScreen_Update(screen, 1.0);

	String chars = String_FromConst("0123456789-, ()");
	String prefix = String_FromConst("Position: ");
	TextAtlas_Make(&screen->PosAtlas, &chars, &screen->Font, &prefix);
	screen->PosAtlas.Tex.Y = (Int16)(status->Height + 2);

	Int32 yOffset = status->Height + screen->PosAtlas.Tex.Height + 2;
	TextWidget* hacks = &screen->HackStates; TextWidget_Make(hacks, &screen->Font);
	Widget_SetLocation((Widget*)hacks, ANCHOR_MIN, ANCHOR_MIN, 2, yOffset);
	hacks->ReducePadding = true;
	Elem_Init(hacks);
	StatusScreen_UpdateHackState(screen);
}

void StatusScreen_Init(GuiElement* elem) {
	StatusScreen* screen = (StatusScreen*)elem;
	Platform_MakeFont(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);
	StatusScreen_ContextRecreated(screen);

	Event_RegisterVoid(&ChatEvents_FontChanged,     screen, StatusScreen_FontChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, StatusScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, StatusScreen_ContextRecreated);
}

void StatusScreen_Render(GuiElement* elem, Real64 delta) {
	StatusScreen* screen = (StatusScreen*)elem;
	StatusScreen_Update(screen, delta);
	if (Game_HideGui || !Game_ShowFPS) return;

	Gfx_SetTexturing(true);
	Elem_Render(&screen->Status, delta);

	if (!Game_ClassicMode && Gui_Active == NULL) {
		if (StatusScreen_HacksChanged(screen)) { StatusScreen_UpdateHackState(screen); }
		StatusScreen_DrawPosition(screen);
		Elem_Render(&screen->HackStates, delta);
	}
	Gfx_SetTexturing(false);
}

void StatusScreen_Free(GuiElement* elem) {
	StatusScreen* screen = (StatusScreen*)elem;
	Platform_FreeFont(&screen->Font);
	StatusScreen_ContextLost(screen);

	Event_UnregisterVoid(&ChatEvents_FontChanged,     screen, StatusScreen_FontChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, StatusScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, StatusScreen_ContextRecreated);
}

Screen* StatusScreen_MakeInstance(void) {
	StatusScreen* screen = &StatusScreen_Instance;
	Platform_MemSet(screen, 0, sizeof(StatusScreen));
	screen->VTABLE = &StatusScreen_VTABLE;
	Screen_Reset((Screen*)screen);

	screen->OnResize       = StatusScreen_OnResize;
	screen->VTABLE->Init   = StatusScreen_Init;
	screen->VTABLE->Render = StatusScreen_Render;
	screen->VTABLE->Free   = StatusScreen_Free;
	return (Screen*)screen;
}

void StatusScreen_Ready(void) { Elem_Init(&StatusScreen_Instance); }
IGameComponent StatusScreen_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Ready = StatusScreen_Ready;
	return comp;
}


/*########################################################################################################################*
*------------------------------------------------------LoadingScreen------------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE LoadingScreen_VTABLE;
LoadingScreen LoadingScreen_Instance;
void LoadingScreen_SetTitle(LoadingScreen* screen) {
	String title = String_FromRawArray(screen->TitleBuffer);
	Elem_TryFree(&screen->Title);

	TextWidget_Create(&screen->Title, &title, &screen->Font);
	Widget_SetLocation((Widget*)(&screen->Title), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -80);
}

void LoadingScreen_SetMessage(LoadingScreen* screen) {
	String message = String_FromRawArray(screen->MessageBuffer);
	Elem_TryFree(&screen->Message);

	TextWidget_Create(&screen->Message, &message, &screen->Font);
	Widget_SetLocation((Widget*)(&screen->Message), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);
}

void LoadingScreen_MapLoading(void* obj, Real32 progress) {
	LoadingScreen* screen = (LoadingScreen*)obj;
	screen->Progress = progress;
}

void LoadingScreen_OnResize(GuiElement* elem) {
	LoadingScreen* screen = (LoadingScreen*)elem;
	Widget_Reposition(&screen->Title);
	Widget_Reposition(&screen->Message);
}

void LoadingScreen_ContextLost(void* obj) {
	LoadingScreen* screen = (LoadingScreen*)obj;
	Elem_TryFree(&screen->Title);
	Elem_TryFree(&screen->Message);
}

void LoadingScreen_ContextRecreated(void* obj) {
	LoadingScreen* screen = (LoadingScreen*)obj;
	if (Gfx_LostContext) return;
	LoadingScreen_SetTitle(screen);
	LoadingScreen_SetMessage(screen);
}

bool LoadingScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	if (key == Key_Tab) return true;
	return Elem_HandlesKeyDown(Gui_HUD, key);
}

bool LoadingScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) {
	return Elem_HandlesKeyPress(Gui_HUD, key);
}

bool LoadingScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	if (key == Key_Tab) return true;
	return Elem_HandlesKeyUp(Gui_HUD, key);
}

bool LoadingScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) { return true; }
bool LoadingScreen_HandlesMouseUp(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) { return true; }
bool LoadingScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) { return true; }
bool LoadingScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) { return true; }

void LoadingScreen_UpdateBackgroundVB(VertexP3fT2fC4b* vertices, Int32 count, Int32 atlasIndex, bool* bound) {
	if (!(*bound)) {
		*bound = true;
		Gfx_BindTexture(Atlas1D_TexIds[atlasIndex]);
	}

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	/* TODO: Do we need to use a separate VB here? */
	GfxCommon_UpdateDynamicVb_IndexedTris(ModelCache_Vb, vertices, count);
}

#define LOADING_TILE_SIZE 64
void LoadingScreen_DrawBackground(void) {
	VertexP3fT2fC4b vertices[144];
	VertexP3fT2fC4b* ptr = vertices;
	Int32 count = 0, atlasIndex = 0, y = 0;
	PackedCol col = PACKEDCOL_CONST(64, 64, 64, 255);

	TextureLoc texLoc = Block_GetTexLoc(BLOCK_DIRT, FACE_YMAX);
	TextureRec rec = Atlas1D_TexRec(texLoc, 1, &atlasIndex);
	Texture tex = Texture_FromRec(0, 0, 0, Game_Width, LOADING_TILE_SIZE, rec);		
	tex.U2 = (Real32)Game_Width / (Real32)LOADING_TILE_SIZE;

	bool bound = false;
	while (y < Game_Height) {
		tex.Y = (Int16)y;
		y += LOADING_TILE_SIZE;		
		GfxCommon_Make2DQuad(&tex, col, &ptr);
		count += 4;

		if (count < Array_Elems(vertices)) continue;
		LoadingScreen_UpdateBackgroundVB(vertices, count, atlasIndex, &bound);
		count = 0;
		ptr = vertices;
	}

	if (count == 0) return;
	LoadingScreen_UpdateBackgroundVB(vertices, count, atlasIndex, &bound);
}

void LoadingScreen_Init(GuiElement* elem) {
	LoadingScreen* screen = (LoadingScreen*)elem;
	Gfx_SetFog(false);
	LoadingScreen_ContextRecreated(screen);

	Event_RegisterReal(&WorldEvents_MapLoading,   screen, LoadingScreen_MapLoading);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, LoadingScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, LoadingScreen_ContextRecreated);
}

#define PROG_BAR_WIDTH 220
#define PROG_BAR_HEIGHT 10
void LoadingScreen_Render(GuiElement* elem, Real64 delta) {
	LoadingScreen* screen = (LoadingScreen*)elem;
	Gfx_SetTexturing(true);
	LoadingScreen_DrawBackground();
	Elem_Render(&screen->Title, delta);
	Elem_Render(&screen->Message, delta);

	Gfx_SetTexturing(false);

	Int32 x = Game_Width  / 2 - PROG_BAR_WIDTH  / 2;
	Int32 y = Game_Height / 2 - PROG_BAR_HEIGHT / 2;
	Int32 progWidth = (Int32)(PROG_BAR_WIDTH * screen->Progress);

	PackedCol backCol = PACKEDCOL_CONST(128, 128, 128, 255);
	PackedCol progCol = PACKEDCOL_CONST(128, 255, 128, 255);
	GfxCommon_Draw2DFlat(x, y, PROG_BAR_WIDTH, PROG_BAR_HEIGHT, backCol);
	GfxCommon_Draw2DFlat(x, y, progWidth,      PROG_BAR_HEIGHT, progCol);
}

void LoadingScreen_Free(GuiElement* elem) {
	LoadingScreen* screen = (LoadingScreen*)elem;
	Platform_FreeFont(&screen->Font);
	LoadingScreen_ContextLost(screen);

	Event_UnregisterReal(&WorldEvents_MapLoading,   screen, LoadingScreen_MapLoading);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, LoadingScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, LoadingScreen_ContextRecreated);
}

void LoadingScreen_Make(LoadingScreen* screen, GuiElementVTABLE* vtable, STRING_PURE String* title, STRING_PURE String* message) {
	Platform_MemSet(screen, 0, sizeof(LoadingScreen));
	screen->VTABLE = vtable;
	Screen_Reset((Screen*)screen);
	screen->HandlesAllInput = true;

	screen->VTABLE->HandlesKeyDown     = LoadingScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp       = LoadingScreen_HandlesKeyUp;
	screen->VTABLE->HandlesMouseDown   = LoadingScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseUp     = LoadingScreen_HandlesMouseUp;
	screen->VTABLE->HandlesMouseMove   = LoadingScreen_HandlesMouseMove;
	screen->VTABLE->HandlesMouseScroll = LoadingScreen_HandlesMouseScroll;

	screen->OnResize       = LoadingScreen_OnResize;
	screen->VTABLE->Init   = LoadingScreen_Init;
	screen->VTABLE->Render = LoadingScreen_Render;
	screen->VTABLE->Free   = LoadingScreen_Free;

	String titleScreen = String_InitAndClearArray(screen->TitleBuffer);
	String_AppendString(&titleScreen, title);
	String messageScreen = String_InitAndClearArray(screen->MessageBuffer);
	String_AppendString(&messageScreen, message);

	Platform_MakeFont(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);
	screen->BlocksWorld     = true;
	screen->RenderHUDOver   = true;
}

Screen* LoadingScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message) {
	LoadingScreen* screen = &LoadingScreen_Instance;
	LoadingScreen_Make(screen, &LoadingScreen_VTABLE, title, message);
	return (Screen*)screen;
}
extern Screen* LoadingScreen_UNSAFE_RawPointer = (Screen*)&LoadingScreen_Instance;


/*########################################################################################################################*
*--------------------------------------------------GeneratingMapScreen----------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE GeneratingMapScreen_VTABLE;
GeneratingMapScreen GeneratingMapScreen_Instance;
void GeneratingScreen_Render(GuiElement* elem, Real64 delta) {
	GeneratingMapScreen* screen = (GeneratingMapScreen*)elem;
	LoadingScreen_Render(elem, delta);
	if (Gen_Done) { ServerConnection_EndGeneration(); return; }

	const volatile UInt8* state = Gen_CurrentState;
	screen->Base.Progress = Gen_CurrentProgress;
	if (state == screen->LastState) return;

	String message = String_InitAndClearArray(screen->Base.MessageBuffer);
	String_AppendConst(&message, state);
	LoadingScreen_SetMessage(&screen->Base);
}

Screen* GeneratingScreen_MakeInstance(void) {
	GeneratingMapScreen* screen = &GeneratingMapScreen_Instance;
	String title   = String_FromConst("Generating level");
	String message = String_FromConst("Generating..");
	LoadingScreen_Make(&screen->Base, &GeneratingMapScreen_VTABLE, &title, &message);

	screen->Base.VTABLE->Render = GeneratingScreen_Render;
	screen->LastState = NULL;
	return (Screen*)screen;
}


/*########################################################################################################################*
*--------------------------------------------------------ChatScreen-------------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE ChatScreen_VTABLE;
ChatScreen ChatScreen_Instance;
Int32 ChatScreen_BottomOffset(void) { return ((HUDScreen*)Gui_HUD)->Hotbar.Height; }
Int32 ChatScreen_InputUsedHeight(ChatScreen* screen) {
	if (screen->AltText.Height == 0) {
		return screen->Input.Base.Height + 20;
	} else {
		return (Game_Height - screen->AltText.Y) + 5;
	}
}

void ChatScreen_UpdateAltTextY(ChatScreen* screen) {
	InputWidget* input = &screen->Input.Base;
	Int32 height = max(input->Height + input->YOffset, ChatScreen_BottomOffset());
	height += input->YOffset;

	screen->AltText.Tex.Y = Game_Height - (height + screen->AltText.Tex.Height);
	screen->AltText.Y = screen->AltText.Tex.Y;
}

void ChatScreen_SetHandlesAllInput(ChatScreen* screen, bool handles) {
	screen->HandlesAllInput  = handles;
	Gui_HUD->HandlesAllInput = handles;
}

void ChatScreen_OpenInput(ChatScreen* screen, STRING_PURE String* initialText) {
	Game_SetCursorVisible(true);
	screen->SuppressNextPress = true;
	ChatScreen_SetHandlesAllInput(screen, true);
	Key_KeyRepeat = true;

	String_Set(&screen->Input.Base.Text, initialText);
	Elem_Recreate(&screen->Input.Base);
}

void ChatScreen_ResetChat(ChatScreen* screen) {
	Elem_TryFree(&screen->Chat);
	Int32 i;
	for (i = screen->ChatIndex; i < screen->ChatIndex + Game_ChatLines; i++) {
		if (i >= 0 && i < Chat_Log.Count) {
			String msg = StringsBuffer_UNSAFE_Get(&Chat_Log, i);
			TextGroupWidget_PushUpAndReplaceLast(&screen->Chat, &msg);
		}
	}
}

void ChatScreen_ConstructWidgets(ChatScreen* screen) {
#define ChatScreen_MakeGroup(widget, lines, textures, buffer) TextGroupWidget_Create(widget, lines, &screen->ChatFont, &screen->ChatUrlFont, textures, buffer);
	Int32 yOffset = ChatScreen_BottomOffset() + 15;

	ChatInputWidget_Create(&screen->Input, &screen->ChatFont);
	Widget_SetLocation((Widget*)(&screen->Input.Base), ANCHOR_MIN, ANCHOR_MAX, 5, 5);

	SpecialInputWidget_Create(&screen->AltText, &screen->ChatFont, &screen->Input.Base);
	Elem_Init(&screen->AltText);
	ChatScreen_UpdateAltTextY(screen);

	ChatScreen_MakeGroup(&screen->Status, CHATSCREEN_MAX_STATUS, screen->Status_Textures, screen->Status_Buffer);
	Widget_SetLocation((Widget*)(&screen->Status), ANCHOR_MAX, ANCHOR_MIN, 0, 0);
	Elem_Init(&screen->Status);
	TextGroupWidget_SetUsePlaceHolder(&screen->Status, 0, false);
	TextGroupWidget_SetUsePlaceHolder(&screen->Status, 1, false);

	ChatScreen_MakeGroup(&screen->BottomRight, CHATSCREEN_MAX_GROUP, screen->BottomRight_Textures, screen->BottomRight_Buffer);
	Widget_SetLocation((Widget*)(&screen->BottomRight), ANCHOR_MAX, ANCHOR_MAX, 0, yOffset);
	Elem_Init(&screen->BottomRight);

	ChatScreen_MakeGroup(&screen->Chat, Game_ChatLines, screen->Chat_Textures, screen->Chat_Buffer);
	Widget_SetLocation((Widget*)(&screen->Chat), ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Elem_Init(&screen->Chat);

	ChatScreen_MakeGroup(&screen->ClientStatus, CHATSCREEN_MAX_GROUP, screen->ClientStatus_Textures, screen->ClientStatus_Buffer);
	Widget_SetLocation((Widget*)(&screen->ClientStatus), ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Elem_Init(&screen->ClientStatus);

	String empty = String_MakeNull();
	TextWidget_Create(&screen->Announcement, &empty, &screen->AnnouncementFont);
	Widget_SetLocation((Widget*)(&screen->Announcement), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -Game_Height / 4);
}

void ChatScreen_SetInitialMessages(ChatScreen* screen) {
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

	String chatInInput = String_FromRawArray(screen->ChatInInputBuffer);
	if (chatInInput.length > 0) {
		ChatScreen_OpenInput(screen, &chatInInput);
		String_Clear(&chatInInput);
	}
}

void ChatScreen_CheckOtherStatuses(ChatScreen* screen) {
	AsyncRequest request;
	Int32 progress;
	bool hasRequest = AsyncDownloader_GetInProgress(&request, &progress);

	String id = String_FromRawArray(request.ID);
	String terrain = String_FromConst("terrain");
	String texPack = String_FromConst("texturePack");

	/* Is terrain / texture pack currently being downloaded? */
	if (!hasRequest || !(String_Equals(&id, &terrain) || String_Equals(&id, &texPack))) {
		if (screen->Status.Textures[1].ID != NULL) {
			String empty = String_MakeNull();
			TextGroupWidget_SetText(&screen->Status, 1, &empty);
		}
		screen->LastDownloadStatus = Int32_MinValue;
		return;
	}

	if (progress == screen->LastDownloadStatus) return;
	screen->LastDownloadStatus = progress;
	UInt8 strBuffer[String_BufferSize(STRING_SIZE)];
	String str = String_InitAndClearArray(strBuffer);

	if (progress == -2) {
		String_AppendConst(&str, "&eRetrieving texture pack..");
	} else if (progress == -1) {
		String_AppendConst(&str, "&eDownloading texture pack");
	} else if (progress >= 0 && progress <= 100) {
		String_AppendConst(&str, "&eDownloading texture pack (&7");
		String_AppendInt32(&str, progress);
		String_AppendConst(&str, "&e%)");
	}
	TextGroupWidget_SetText(&screen->Status, 1, &str);
}

void ChatScreen_RenderBackground(ChatScreen* screen) {
	Int32 usedHeight = TextGroupWidget_UsedHeight(&screen->Chat);
	Int32 y = screen->Chat.Y + screen->Chat.Height - usedHeight - 5;
	Int32 x = screen->Chat.X - 5;
	Int32 width = max(screen->ClientStatus.Width, screen->Chat.Width) + 10;

	Int32 boxHeight = usedHeight + TextGroupWidget_UsedHeight(&screen->ClientStatus);
	if (boxHeight > 0) {
		PackedCol backCol = PACKEDCOL_CONST(0, 0, 0, 127);
		GfxCommon_Draw2DFlat(x, y, width, boxHeight + 10, backCol);
	}
}

void ChatScreen_UpdateChatYOffset(ChatScreen* screen, bool force) {
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

void ChatElem_Recreate(TextGroupWidget* group, UInt8 code) {
	Int32 i, j;
	UInt8 lineBuffer[String_BufferSize(TEXTGROUPWIDGET_LEN)];
	String line = String_InitAndClearArray(lineBuffer);

	for (i = 0; i < group->LinesCount; i++) {
		TextGroupWidget_GetText(group, i, &line);
		if (line.length == 0) continue;

		for (j = 0; j < line.length - 1; j++) {
			if (line.buffer[j] == '&' && line.buffer[j + 1] == code) {
				TextGroupWidget_SetText(group, i, &line); 
				break;
			}
		}
	}
}

Int32 ChatScreen_ClampIndex(Int32 index) {
	Int32 maxIndex = Chat_Log.Count - Game_ChatLines;
	Int32 minIndex = min(0, maxIndex);
	Math_Clamp(index, minIndex, maxIndex);
	return index;
}

void ChatScreen_ScrollHistoryBy(ChatScreen* screen, Int32 delta) {
	Int32 newIndex = ChatScreen_ClampIndex(screen->ChatIndex + delta);
	if (newIndex == screen->ChatIndex) return;

	screen->ChatIndex = newIndex;
	ChatScreen_ResetChat(screen);
}

bool ChatScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	ChatScreen* screen = (ChatScreen*)elem;
	screen->SuppressNextPress = false;
	if (screen->HandlesAllInput) { /* text input bar */
		if (key == KeyBind_Get(KeyBind_SendChat) || key == Key_KeypadEnter || key == KeyBind_Get(KeyBind_PauseOrExit)) {
			ChatScreen_SetHandlesAllInput(screen, false);
			Game_SetCursorVisible(false);
			Camera_Active->RegrabMouse();
			Key_KeyRepeat = false;

			if (key == KeyBind_Get(KeyBind_PauseOrExit)) {
				InputWidget_Clear(&screen->Input.Base);
			}
			elem = (GuiElement*)(&screen->Input);
			screen->Input.Base.OnPressedEnter(elem);
			SpecialInputWidget_SetActive(&screen->AltText, false);

			/* Do we need to move all chat down? */
			Int32 resetIndex = Chat_Log.Count - Game_ChatLines;
			if (screen->ChatIndex != resetIndex) {
				screen->ChatIndex = ChatScreen_ClampIndex(resetIndex);
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

bool ChatScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	ChatScreen* screen = (ChatScreen*)elem;
	if (!screen->HandlesAllInput) return false;

	if (ServerConnection_SupportsFullCP437 && key == KeyBind_Get(KeyBind_ExtInput)) {
		if (Window_GetFocused()) {
			bool active = !screen->AltText.Active;
			SpecialInputWidget_SetActive(&screen->AltText, active);
		}
	}
	return true;
}

bool ChatScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) {
	ChatScreen* screen = (ChatScreen*)elem;
	if (!screen->HandlesAllInput) return false;
	if (screen->SuppressNextPress) {
		screen->SuppressNextPress = false;
		return false;
	}

	bool handled = Elem_HandlesKeyPress(&screen->Input.Base, key);
	ChatScreen_UpdateAltTextY(screen);
	return handled;
}

bool ChatScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) {
	ChatScreen* screen = (ChatScreen*)elem;
	if (!screen->HandlesAllInput) return false;

	Int32 steps = Utils_AccumulateWheelDelta(&screen->ChatAcc, delta);
	ChatScreen_ScrollHistoryBy(screen, -steps);
	return true;
}

bool ChatScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	ChatScreen* screen = (ChatScreen*)elem;
	if (!screen->HandlesAllInput || Game_HideGui) return false;

	if (!Widget_Contains((Widget*)(&screen->Chat), x, y)) {
		if (screen->AltText.Active && Widget_Contains((Widget*)(&screen->AltText), x, y)) {
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

	UInt8 textBuffer[String_BufferSize(TEXTGROUPWIDGET_LEN)];
	String text = String_InitAndClearArray(textBuffer);
	TextGroupWidget_GetSelected(&screen->Chat, &text, x, y);
	if (text.length == 0) return false;

	UInt8 urlBuffer[String_BufferSize(TEXTGROUPWIDGET_LEN)];
	String url = String_InitAndClearArray(urlBuffer);
	String_AppendColorless(&url, &text);

	if (Utils_IsUrlPrefix(&url, 0)) {
		Screen* overlay = UrlWarningOverlay_MakeInstance(&url);
		Gui_ShowOverlay(overlay, false);
	} else if (Game_ClickableChat) {
		InputWidget_AppendString(&screen->Input.Base, &text);
	}
	return true;
}

void ChatScreen_ColCodeChanged(void* obj, Int32 code) {
	ChatScreen* screen = (ChatScreen*)obj;
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

void ChatScreen_ChatReceived(void* obj, String* msg, Int32 type) {
	if (Gfx_LostContext) return;
	ChatScreen* screen = (ChatScreen*)obj;

	if (type == MSG_TYPE_NORMAL) {
		screen->ChatIndex++;
		if (Game_ChatLines == 0) return;

		Int32 index = screen->ChatIndex + (Game_ChatLines - 1);
		String chatMsg = StringsBuffer_UNSAFE_Get(&Chat_Log, index);
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

void ChatScreen_ContextLost(void* obj) {
	ChatScreen* screen = (ChatScreen*)obj;
	String chatInInput = String_InitAndClearArray(screen->ChatInInputBuffer);

	if (screen->HandlesAllInput) {
		String_AppendString(&chatInInput, &screen->Input.Base.Text);
		Game_SetCursorVisible(false);
	}

	Elem_TryFree(&screen->Chat);
	Elem_TryFree(&screen->Input.Base);
	Elem_TryFree(&screen->AltText);
	Elem_TryFree(&screen->Status);
	Elem_TryFree(&screen->BottomRight);
	Elem_TryFree(&screen->ClientStatus);
	Elem_TryFree(&screen->Announcement);
}

void ChatScreen_ContextRecreated(void* obj) {
	ChatScreen* screen = (ChatScreen*)obj;
	ChatScreen_ConstructWidgets(screen);
	ChatScreen_SetInitialMessages(screen);
}

void ChatScreen_FontChanged(void* obj) {
	ChatScreen* screen = (ChatScreen*)obj;
	if (!Drawer2D_UseBitmappedChat) return;
	Elem_Recreate(screen);
	ChatScreen_UpdateChatYOffset(screen, true);
}

void ChatScreen_OnResize(GuiElement* elem) {
	ChatScreen* screen = (ChatScreen*)elem;
	bool active = screen->AltText.Active;
	Elem_Recreate(screen);
	SpecialInputWidget_SetActive(&screen->AltText, active);
}

void ChatScreen_Init(GuiElement* elem) {
	ChatScreen* screen = (ChatScreen*)elem;

	Int32 fontSize = (Int32)(8 * Game_GetChatScale());
	Math_Clamp(fontSize, 8, 60);
	Platform_MakeFont(&screen->ChatFont, &Game_FontName, fontSize, FONT_STYLE_NORMAL);
	Platform_MakeFont(&screen->ChatUrlFont, &Game_FontName, fontSize, FONT_STYLE_UNDERLINE);

	fontSize = (Int32)(16 * Game_GetChatScale());
	Math_Clamp(fontSize, 8, 60);
	Platform_MakeFont(&screen->AnnouncementFont, &Game_FontName, fontSize, FONT_STYLE_NORMAL);
	ChatScreen_ContextRecreated(elem);

	Event_RegisterChat(&ChatEvents_ChatReceived,    screen, ChatScreen_ChatReceived);
	Event_RegisterVoid(&ChatEvents_FontChanged,     screen, ChatScreen_FontChanged);
	Event_RegisterInt(&ChatEvents_ColCodeChanged, screen, ChatScreen_ColCodeChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, ChatScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, ChatScreen_ContextRecreated);
}

void ChatScreen_Render(GuiElement* elem, Real64 delta) {
	ChatScreen* screen = (ChatScreen*)elem;
	ChatScreen_CheckOtherStatuses(screen);
	if (!Game_PureClassic) { Elem_Render(&screen->Status, delta); }
	Elem_Render(&screen->BottomRight, delta);

	ChatScreen_UpdateChatYOffset(screen, false);
	Int32 i, y = screen->ClientStatus.Y + screen->ClientStatus.Height;
	for (i = 0; i < screen->ClientStatus.LinesCount; i++) {
		Texture tex = screen->ClientStatus.Textures[i];
		if (tex.ID == NULL) continue;

		y -= tex.Height; tex.Y = y;
		Texture_Render(&tex);
	}

	DateTime now; Platform_CurrentUTCTime(&now);
	if (screen->HandlesAllInput) {
		Elem_Render(&screen->Chat, delta);
	} else {
		Int64 nowMS = DateTime_TotalMs(&now);
		/* Only render recent chat */
		for (i = 0; i < screen->Chat.LinesCount; i++) {
			Texture tex = screen->Chat.Textures[i];
			Int32 logIdx = screen->ChatIndex + i;
			if (tex.ID == NULL) continue;
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

	if (screen->Announcement.Texture.ID != NULL && DateTime_MsBetween(&Chat_Announcement.Received, &now) > 5 * 1000) {
		Elem_TryFree(&screen->Announcement);
	}
}

void ChatScreen_Free(GuiElement* elem) {
	ChatScreen* screen = (ChatScreen*)elem;
	ChatScreen_ContextLost(elem);
	Platform_FreeFont(&screen->ChatFont);
	Platform_FreeFont(&screen->ChatUrlFont);
	Platform_FreeFont(&screen->AnnouncementFont);

	Event_UnregisterChat(&ChatEvents_ChatReceived,    screen, ChatScreen_ChatReceived);
	Event_UnregisterVoid(&ChatEvents_FontChanged,     screen, ChatScreen_FontChanged);
	Event_UnregisterInt(&ChatEvents_ColCodeChanged, screen, ChatScreen_ColCodeChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, ChatScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, ChatScreen_ContextRecreated);
}

Screen* ChatScreen_MakeInstance(void) {
	ChatScreen* screen = &ChatScreen_Instance;
	Platform_MemSet(screen, 0, sizeof(ChatScreen));
	screen->VTABLE = &ChatScreen_VTABLE;
	Screen_Reset((Screen*)screen);

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
	return (Screen*)screen;
}


/*########################################################################################################################*
*--------------------------------------------------------HUDScreen--------------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE HUDScreenVTABLE;
HUDScreen HUDScreen_Instance;
#define CH_EXTENT 16
#define CH_WEIGHT 2
void HUDScreen_DrawCrosshairs(void) {
	if (Gui_IconsTex == NULL) return;
	TextureRec chRec = { 0.0f, 0.0f, 15.0f / 256.0f, 15 / 256.0f };

	Int32 extent = (Int32)(CH_EXTENT * Game_Scale(Game_Height / 480.0f));
	Int32 chX = (Game_Width / 2) - extent, chY = (Game_Height / 2) - extent;
	Texture chTex = Texture_FromRec(Gui_IconsTex, chX, chY, extent * 2, extent * 2, chRec);
	Texture_Render(&chTex);
}

void HUDScreen_FreePlayerList(HUDScreen* screen) {
	screen->WasShowingList = screen->ShowingList;
	if (screen->ShowingList) { Elem_TryFree(&screen->PlayerList); }
	screen->ShowingList = false;
}

void HUDScreen_ContextLost(void* obj) {
	HUDScreen* screen = (HUDScreen*)obj;
	HUDScreen_FreePlayerList(screen);
	Elem_TryFree(&screen->Hotbar);
}

void HUDScreen_ContextRecreated(void* obj) {
	HUDScreen* screen = (HUDScreen*)obj;
	Elem_TryFree(&screen->Hotbar);
	Elem_Init(&screen->Hotbar);

	if (!screen->WasShowingList) return;
	bool extended = ServerConnection_SupportsExtPlayerList && !Game_UseClassicTabList;
	PlayerListWidget_Create(&screen->PlayerList, &screen->PlayerFont, !extended);
	screen->ShowingList = true;

	Elem_Init(&screen->PlayerList);
	Widget_Reposition(&screen->PlayerList);
}


void HUDScreen_OnResize(GuiElement* elem) {
	HUDScreen* screen = (HUDScreen*)elem;
	screen->Chat->OnResize((GuiElement*)screen->Chat);

	Widget_Reposition(&screen->Hotbar);
	if (screen->ShowingList) {
		Widget_Reposition(&screen->PlayerList);
	}
}

void HUDScreen_OnNewMap(void* obj) {
	HUDScreen* screen = (HUDScreen*)obj;
	HUDScreen_FreePlayerList(screen);
}

bool HUDScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) {
	HUDScreen* screen = (HUDScreen*)elem;
	return Elem_HandlesKeyPress(screen->Chat, key); 
}

bool HUDScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	HUDScreen* screen = (HUDScreen*)elem;
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

bool HUDScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	HUDScreen* screen = (HUDScreen*)elem;
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

bool HUDScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) {
	HUDScreen* screen = (HUDScreen*)elem;
	return Elem_HandlesMouseScroll(screen->Chat, delta);
}

bool HUDScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	HUDScreen* screen = (HUDScreen*)elem;
	if (btn != MouseButton_Left || !screen->HandlesAllInput) return false;

	elem = (GuiElement*)screen->Chat;
	if (!screen->ShowingList) { return elem->VTABLE->HandlesMouseDown(elem, x, y, btn); }

	UInt8 nameBuffer[String_BufferSize(STRING_SIZE + 1)];
	String name = String_InitAndClearArray(nameBuffer);
	PlayerListWidget_GetNameUnder(&screen->PlayerList, x, y, &name);
	if (name.length == 0) { return elem->VTABLE->HandlesMouseDown(elem, x, y, btn); }

	String_Append(&name, ' ');
	ChatScreen* chat = (ChatScreen*)screen->Chat;
	if (chat->HandlesAllInput) { InputWidget_AppendString(&chat->Input.Base, &name); }
	return true;
}

void HUDScreen_Init(GuiElement* elem) {
	HUDScreen* screen = (HUDScreen*)elem;
	UInt16 size = Drawer2D_UseBitmappedChat ? 16 : 11;
	Platform_MakeFont(&screen->PlayerFont, &Game_FontName, size, FONT_STYLE_NORMAL);

	HotbarWidget_Create(&screen->Hotbar);
	Elem_Init(&screen->Hotbar);

	ChatScreen_MakeInstance();
	screen->Chat = (Screen*)&ChatScreen_Instance;
	Elem_Init(screen->Chat);

	Event_RegisterVoid(&WorldEvents_NewMap,         screen, HUDScreen_OnNewMap);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, HUDScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, HUDScreen_ContextRecreated);
}

void HUDScreen_Render(GuiElement* elem, Real64 delta) {
	HUDScreen* screen = (HUDScreen*)elem;
	ChatScreen* chat = (ChatScreen*)screen->Chat;
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

	if (screen->ShowingList && Gui_GetActiveScreen() == (Screen*)screen) {
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

void HUDScreen_Free(GuiElement* elem) {
	HUDScreen* screen = (HUDScreen*)elem;
	Platform_FreeFont(&screen->PlayerFont);
	Elem_TryFree(screen->Chat);
	HUDScreen_ContextLost(screen);

	Event_UnregisterVoid(&WorldEvents_NewMap,         screen, HUDScreen_OnNewMap);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, HUDScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, HUDScreen_ContextRecreated);
}

Screen* HUDScreen_MakeInstance(void) {
	HUDScreen* screen = &HUDScreen_Instance;
	Platform_MemSet(screen, 0, sizeof(HUDScreen));
	screen->VTABLE = &HUDScreenVTABLE;
	Screen_Reset((Screen*)screen);

	screen->OnResize       = HUDScreen_OnResize;
	screen->VTABLE->Init   = HUDScreen_Init;
	screen->VTABLE->Render = HUDScreen_Render;
	screen->VTABLE->Free   = HUDScreen_Free;

	screen->VTABLE->HandlesKeyDown     = HUDScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp       = HUDScreen_HandlesKeyUp;
	screen->VTABLE->HandlesKeyPress    = HUDScreen_HandlesKeyPress;
	screen->VTABLE->HandlesMouseDown   = HUDScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseScroll = HUDScreen_HandlesMouseScroll;
	return (Screen*)screen;
}

void HUDScreen_Ready(void) { Elem_Init(&HUDScreen_Instance); }
IGameComponent HUDScreen_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Ready = HUDScreen_Ready;
	return comp;
}

void HUDScreen_OpenInput(Screen* hud, STRING_PURE String* text) {
	Screen* chat = ((HUDScreen*)hud)->Chat;
	ChatScreen_OpenInput((ChatScreen*)chat, text);
}

void HUDScreen_AppendInput(Screen* hud, STRING_PURE String* text) {
	Screen* chat = ((HUDScreen*)hud)->Chat;
	ChatInputWidget* widget = &((ChatScreen*)chat)->Input;
	InputWidget_AppendString(&widget->Base, text);
}

Widget* HUDScreen_GetHotbar(Screen* hud) {
	HUDScreen* screen = (HUDScreen*)hud;
	return (Widget*)(&screen->Hotbar);
}


/*########################################################################################################################*
*----------------------------------------------------DisconnectScreen-----------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE DisconnectScreen_VTABLE;
DisconnectScreen DisconnectScreen_Instance;
#define DISCONNECT_DELAY_MS 5000
void DisconnectScreen_ReconnectMessage(DisconnectScreen* screen, STRING_TRANSIENT String* msg) {
	if (screen->CanReconnect) {
		DateTime now; Platform_CurrentUTCTime(&now);
		Int32 elapsedMS = (Int32)(DateTime_TotalMs(&now) - screen->InitTime);
		Int32 secsLeft = (DISCONNECT_DELAY_MS - elapsedMS) / DATETIME_MILLISECS_PER_SECOND;

		if (secsLeft > 0) {
			String_Format1(msg, "Reconnect in %i", &secsLeft);
			return;
		}
	}
	String_AppendConst(msg, "Reconnect");
}

void DisconnectScreen_Redraw(DisconnectScreen* screen, Real64 delta) {
	PackedCol top    = PACKEDCOL_CONST(64, 32, 32, 255);
	PackedCol bottom = PACKEDCOL_CONST(80, 16, 16, 255);
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, top, bottom);

	Gfx_SetTexturing(true);
	Elem_Render(&screen->Title, delta);
	Elem_Render(&screen->Message, delta);
	if (screen->CanReconnect) { Elem_Render(&screen->Reconnect, delta); }
	Gfx_SetTexturing(false);
}

void DisconnectScreen_UpdateDelayLeft(DisconnectScreen* screen, Real64 delta) {
	DateTime now; Platform_CurrentUTCTime(&now);
	Int32 elapsedMS = (Int32)(DateTime_TotalMs(&now) - screen->InitTime);
	Int32 secsLeft = (DISCONNECT_DELAY_MS - elapsedMS) / DATETIME_MILLISECS_PER_SECOND;
	if (secsLeft < 0) secsLeft = 0;
	if (screen->LastSecsLeft == secsLeft && screen->Reconnect.Active == screen->LastActive) return;

	UInt8 msgBuffer[String_BufferSize(STRING_SIZE)];
	String msg = String_InitAndClearArray(msgBuffer);
	DisconnectScreen_ReconnectMessage(screen, &msg);
	ButtonWidget_SetText(&screen->Reconnect, &msg);
	screen->Reconnect.Disabled = secsLeft != 0;

	DisconnectScreen_Redraw(screen, delta);
	screen->LastSecsLeft = secsLeft;
	screen->LastActive   = screen->Reconnect.Active;
	screen->ClearTime = DateTime_TotalMs(&now) + 500;
}

void DisconnectScreen_ContextLost(void* obj) {
	DisconnectScreen* screen = (DisconnectScreen*)obj;
	Elem_TryFree(&screen->Title);
	Elem_TryFree(&screen->Message);
	Elem_TryFree(&screen->Reconnect);
}

void DisconnectScreen_ContextRecreated(void* obj) {
	DisconnectScreen* screen = (DisconnectScreen*)obj;
	if (Gfx_LostContext) return;
	DateTime now; Platform_CurrentUTCTime(&now);
	screen->ClearTime = DateTime_TotalMs(&now) + 500;

	String title = String_FromRawArray(screen->TitleBuffer);
	TextWidget_Create(&screen->Title, &title, &screen->TitleFont);
	Widget_SetLocation((Widget*)(&screen->Title), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);

	String message = String_FromRawArray(screen->MessageBuffer);
	TextWidget_Create(&screen->Message, &message, &screen->MessageFont);
	Widget_SetLocation((Widget*)(&screen->Message), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 10);

	UInt8 msgBuffer[String_BufferSize(STRING_SIZE)];
	String msg = String_InitAndClearArray(msgBuffer);
	DisconnectScreen_ReconnectMessage(screen, &msg);

	ButtonWidget_Create(&screen->Reconnect, 300, &msg, &screen->TitleFont, NULL);
	Widget_SetLocation((Widget*)(&screen->Reconnect), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 80);
	screen->Reconnect.Disabled = !screen->CanReconnect;
}

void DisconnectScreen_Init(GuiElement* elem) {
	DisconnectScreen* screen = (DisconnectScreen*)elem;
	Game_SkipClear = true;
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, DisconnectScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, DisconnectScreen_ContextRecreated);

	DisconnectScreen_ContextRecreated(screen);
	DateTime now; Platform_CurrentUTCTime(&now);
	screen->InitTime = DateTime_TotalMs(&now);
	screen->LastSecsLeft = DISCONNECT_DELAY_MS / DATETIME_MILLISECS_PER_SECOND;
}

void DisconnectScreen_Render(GuiElement* elem, Real64 delta) {
	DisconnectScreen* screen = (DisconnectScreen*)elem;
	if (screen->CanReconnect) {
		DisconnectScreen_UpdateDelayLeft(screen, delta);
	}

	/* NOTE: We need to make sure that both the front and back buffers have
	definitely been drawn over, so we redraw the background multiple times. */
	DateTime now; Platform_CurrentUTCTime(&now);
	if (DateTime_TotalMs(&now) < screen->ClearTime) {
		DisconnectScreen_Redraw(screen, delta);
	}
}

void DisconnectScreen_Free(GuiElement* elem) {
	DisconnectScreen* screen = (DisconnectScreen*)elem;
	Game_SkipClear = false;
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, DisconnectScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, DisconnectScreen_ContextRecreated);

	DisconnectScreen_ContextLost(screen);
	Platform_FreeFont(&screen->TitleFont);
	Platform_FreeFont(&screen->MessageFont);
}

void DisconnectScreen_OnResize(GuiElement* elem) {
	DisconnectScreen* screen = (DisconnectScreen*)elem;
	Widget_Reposition(&screen->Title);
	Widget_Reposition(&screen->Message);
	Widget_Reposition(&screen->Reconnect);
	DateTime now; Platform_CurrentUTCTime(&now);
	screen->ClearTime = DateTime_TotalMs(&now) + 500;
}

bool DisconnectScreen_HandlesKeyDown(GuiElement* elem, Key key) { return key < Key_F1 || key > Key_F35; }
bool DisconnectScreen_HandlesKeyPress(GuiElement* elem, UInt8 keyChar) { return true; }
bool DisconnectScreen_HandlesKeyUp(GuiElement* elem, Key key) { return true; }

bool DisconnectScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	DisconnectScreen* screen = (DisconnectScreen*)elem;
	ButtonWidget* widget = &screen->Reconnect;
	if (btn != MouseButton_Left) return true;

	if (!widget->Disabled && Widget_Contains((Widget*)widget, x, y)) {
		UInt8 connectBuffer[String_BufferSize(STRING_SIZE)];
		String connect = String_FromConst(connectBuffer);
		String empty = String_MakeNull();
		String_Format2(&connect, "Connecting to %s: %i..", &Game_IPAddress, &Game_Port);

		Gui_ReplaceActive(LoadingScreen_MakeInstance(&connect, &empty));
		ServerConnection_Connect(&Game_IPAddress, Game_Port);
	}
	return true;
}

bool DisconnectScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	DisconnectScreen* screen = (DisconnectScreen*)elem;
	ButtonWidget* widget = &screen->Reconnect;
	widget->Active = !widget->Disabled && Widget_Contains((Widget*)widget, x, y);
	return true;
}

bool DisconnectScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) { return true; }
bool DisconnectScreen_HandlesMouseUp(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) { return true; }

Screen* DisconnectScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message) {
	DisconnectScreen* screen = &DisconnectScreen_Instance;
	Platform_MemSet(screen, 0, sizeof(DisconnectScreen));
	screen->VTABLE = &DisconnectScreen_VTABLE;
	Screen_Reset((Screen*)screen);
	screen->HandlesAllInput = true;

	String titleScreen = String_InitAndClearArray(screen->TitleBuffer);
	String_AppendString(&titleScreen, title);
	String messageScreen = String_InitAndClearArray(screen->MessageBuffer);
	String_AppendString(&messageScreen, message);

	UInt8 reasonBuffer[String_BufferSize(STRING_SIZE)];
	String reason = String_InitAndClearArray(reasonBuffer);
	String_AppendColorless(&reason, message);

	String kick = String_FromConst("Kicked ");
	String ban  = String_FromConst("Banned ");
	screen->CanReconnect = !(String_StartsWith(&reason, &kick) || String_StartsWith(&reason, &ban));

	Platform_MakeFont(&screen->TitleFont,   &Game_FontName, 16, FONT_STYLE_BOLD);
	Platform_MakeFont(&screen->MessageFont, &Game_FontName, 16, FONT_STYLE_NORMAL);
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
	screen->VTABLE->HandlesMouseScroll = DisconnectScreen_HandlesMouseScroll;
	return (Screen*)screen;
}