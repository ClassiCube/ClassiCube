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
	String TitleStr, MessageStr;
	const char* LastState;

	char __TitleBuffer[STRING_SIZE];
	char __MessageBuffer[STRING_SIZE];
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
	UInt64 InitTime, ClearTime;
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
struct InventoryScreen InventoryScreen_Instance;
static void InventoryScreen_OnBlockChanged(void* screen) {
	struct InventoryScreen* s = screen;
	TableWidget_OnInventoryChanged(&s->Table);
}

static void InventoryScreen_ContextLost(void* screen) {
	struct InventoryScreen* s = screen;
	Elem_TryFree(&s->Table);
}

static void InventoryScreen_ContextRecreated(void* screen) {
	struct InventoryScreen* s = screen;
	Elem_Recreate(&s->Table);
}

static void InventoryScreen_MoveToSelected(struct InventoryScreen* s) {
	struct TableWidget* table = &s->Table;
	TableWidget_SetBlockTo(table, Inventory_SelectedBlock);
	Elem_Recreate(table);

	s->DeferredSelect = false;
	/* User is holding invalid block */
	if (table->SelectedIndex == -1) {
		TableWidget_MakeDescTex(table, Inventory_SelectedBlock);
	}
}

static void InventoryScreen_Init(void* screen) {
	struct InventoryScreen* s = screen;
	Font_Make(&s->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);

	TableWidget_Create(&s->Table);
	s->Table.Font = s->Font;
	s->Table.ElementsPerRow = Game_PureClassic ? 9 : 10;
	Elem_Init(&s->Table);

	/* Can't immediately move to selected here, because cursor visibility 
	   might be toggled after Init() is called. This causes the cursor to 
	   be moved back to the middle of the window. */
	s->DeferredSelect = true;
	Key_KeyRepeat     = true;
	Screen_CommonInit(s);

	Event_RegisterVoid(&BlockEvents_PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_RegisterVoid(&BlockEvents_BlockDefChanged,    s, InventoryScreen_OnBlockChanged);
}

static void InventoryScreen_Render(void* screen, Real64 delta) {
	struct InventoryScreen* s = screen;
	if (s->DeferredSelect) InventoryScreen_MoveToSelected(s);
	Elem_Render(&s->Table, delta);
}

static void InventoryScreen_OnResize(void* screen) {
	struct InventoryScreen* s = screen;
	Widget_Reposition(&s->Table);
}

static void InventoryScreen_Free(void* screen) {
	struct InventoryScreen* s = screen;
	Font_Free(&s->Font);
	Key_KeyRepeat = false;
	Screen_CommonFree(s);
	
	Event_UnregisterVoid(&BlockEvents_PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&BlockEvents_BlockDefChanged,    s, InventoryScreen_OnBlockChanged);
}

static bool InventoryScreen_KeyDown(void* screen, Key key) {
	struct InventoryScreen* s = screen;
	struct TableWidget* table = &s->Table;

	if (key == KeyBind_Get(KeyBind_PauseOrExit)) {
		Gui_ReplaceActive(NULL);
	} else if (key == KeyBind_Get(KeyBind_Inventory) && s->ReleasedInv) {
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

static bool InventoryScreen_KeyUp(void* screen, Key key) {
	struct InventoryScreen* s = screen;
	if (key == KeyBind_Get(KeyBind_Inventory)) {
		s->ReleasedInv = true; return true;
	}

	struct HUDScreen* hud = (struct HUDScreen*)Gui_HUD;
	return Elem_HandlesKeyUp(&hud->Hotbar, key);
}

static bool InventoryScreen_MouseDown(void* screen, Int32 x, Int32 y, MouseButton btn) {
	struct InventoryScreen* s = screen;
	struct TableWidget* table = &s->Table;
	struct HUDScreen* hud     = (struct HUDScreen*)Gui_HUD;
	if (table->Scroll.DraggingMouse || Elem_HandlesMouseDown(&hud->Hotbar, x, y, btn)) return true;

	bool handled = Elem_HandlesMouseDown(table, x, y, btn);
	if ((!handled || table->PendingClose) && btn == MouseButton_Left) {
		bool hotbar = Key_IsControlPressed() || Key_IsShiftPressed();
		if (!hotbar) Gui_ReplaceActive(NULL);
	}
	return true;
}

static bool InventoryScreen_MouseUp(void* screen, Int32 x, Int32 y, MouseButton btn) {
	struct InventoryScreen* s = screen;
	struct TableWidget* table = &s->Table;
	return Elem_HandlesMouseUp(table, x, y, btn);
}

static bool InventoryScreen_MouseMove(void* screen, Int32 x, Int32 y) {
	struct InventoryScreen* s = screen;
	struct TableWidget* table = &s->Table;
	return Elem_HandlesMouseMove(table, x, y);
}

static bool InventoryScreen_MouseScroll(void* screen, Real32 delta) {
	struct InventoryScreen* s = screen;
	struct TableWidget* table = &s->Table;

	bool hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (hotbar) return false;
	return Elem_HandlesMouseScroll(table, delta);
}

struct ScreenVTABLE InventoryScreen_VTABLE = {
	InventoryScreen_Init,      InventoryScreen_Render,  InventoryScreen_Free,      Gui_DefaultRecreate,
	InventoryScreen_KeyDown,   InventoryScreen_KeyUp,   Gui_DefaultKeyPress,
	InventoryScreen_MouseDown, InventoryScreen_MouseUp, InventoryScreen_MouseMove, InventoryScreen_MouseScroll,
	InventoryScreen_OnResize,  InventoryScreen_ContextLost, InventoryScreen_ContextRecreated,
};
struct Screen* InventoryScreen_MakeInstance(void) {
	struct InventoryScreen* s = &InventoryScreen_Instance;
	s->HandlesAllInput = true;

	s->VTABLE = &InventoryScreen_VTABLE;
	return (struct Screen*)s;
}
struct Screen* InventoryScreen_UNSAFE_RawPointer = (struct Screen*)&InventoryScreen_Instance;


/*########################################################################################################################*
*-------------------------------------------------------StatusScreen------------------------------------------------------*
*#########################################################################################################################*/
struct StatusScreen StatusScreen_Instance;
static void StatusScreen_MakeText(struct StatusScreen* s, STRING_TRANSIENT String* status) {
	s->FPS = (Int32)(s->Frames / s->Accumulator);
	String_Format1(status, "%i fps, ", &s->FPS);

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

static void StatusScreen_DrawPosition(struct StatusScreen* s) {
	struct TextAtlas* atlas = &s->PosAtlas;
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

static bool StatusScreen_HacksChanged(struct StatusScreen* s) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	return hacks->Speeding != s->Speed || hacks->HalfSpeeding != s->HalfSpeed || hacks->Flying != s->Fly
		|| hacks->Noclip != s->Noclip  || Game_Fov != s->LastFov || hacks->CanSpeed != s->CanSpeed;
}

static void StatusScreen_UpdateHackState(struct StatusScreen* s) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	s->Speed = hacks->Speeding; s->HalfSpeed = hacks->HalfSpeeding; s->Fly = hacks->Flying;
	s->Noclip = hacks->Noclip;  s->LastFov = Game_Fov; s->CanSpeed = hacks->CanSpeed;

	char statusBuffer[STRING_SIZE * 2];
	String status = String_FromArray(statusBuffer);

	if (Game_Fov != Game_DefaultFov) {
		String_Format1(&status, "Zoom fov %i  ", &Game_Fov);
	}
	if (hacks->Flying) String_AppendConst(&status, "Fly ON   ");

	bool speeding = (hacks->Speeding || hacks->HalfSpeeding) && hacks->CanSpeed;
	if (speeding)      String_AppendConst(&status, "Speed ON   ");
	if (hacks->Noclip) String_AppendConst(&status, "Noclip ON   ");

	TextWidget_Set(&s->HackStates, &status, &s->Font);
}

static void StatusScreen_Update(struct StatusScreen* s, Real64 delta) {
	s->Frames++;
	s->Accumulator += delta;
	if (s->Accumulator < 1.0) return;

	char statusBuffer[STRING_SIZE * 2];
	String status = String_FromArray(statusBuffer);
	StatusScreen_MakeText(s, &status);

	TextWidget_Set(&s->Status, &status, &s->Font);
	s->Accumulator = 0.0;
	s->Frames = 0;
	Game_ChunkUpdates = 0;
}

static void StatusScreen_OnResize(void* screen) { }
static void StatusScreen_FontChanged(void* screen) {
	struct StatusScreen* s = screen;
	Elem_Recreate(s);
}

static void StatusScreen_ContextLost(void* screen) {
	struct StatusScreen* s = screen;
	TextAtlas_Free(&s->PosAtlas);
	Elem_TryFree(&s->Status);
	Elem_TryFree(&s->HackStates);
}

static void StatusScreen_ContextRecreated(void* screen) {
	struct StatusScreen* s = screen;
	String chars  = String_FromConst("0123456789-, ()");
	String prefix = String_FromConst("Position: ");

	struct TextWidget* status = &s->Status;
	TextWidget_Make(status);
	Widget_SetLocation(status, ANCHOR_MIN, ANCHOR_MIN, 2, 2);
	status->ReducePadding = true;
	StatusScreen_Update(s, 1.0);

	Int32 elemHeight = status->Height + 2;
	TextAtlas_Make(&s->PosAtlas, &chars, &s->Font, &prefix);
	s->PosAtlas.Tex.Y = elemHeight;

	struct TextWidget* hacks = &s->HackStates; 
	TextWidget_Make(hacks);
	Widget_SetLocation(hacks, ANCHOR_MIN, ANCHOR_MIN, 2, elemHeight * 2);
	hacks->ReducePadding = true;
	StatusScreen_UpdateHackState(s);
}

static void StatusScreen_Init(void* screen) {
	struct StatusScreen* s = screen;
	Font_Make(&s->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);
	Event_RegisterVoid(&ChatEvents_FontChanged, s, StatusScreen_FontChanged);
}

static void StatusScreen_Render(void* screen, Real64 delta) {
	struct StatusScreen* s = screen;
	StatusScreen_Update(s, delta);
	if (Game_HideGui || !Game_ShowFPS) return;

	Gfx_SetTexturing(true);
	Elem_Render(&s->Status, delta);

	if (!Game_ClassicMode && !Gui_Active) {
		if (StatusScreen_HacksChanged(s)) { StatusScreen_UpdateHackState(s); }
		StatusScreen_DrawPosition(s);
		Elem_Render(&s->HackStates, delta);
	}
	Gfx_SetTexturing(false);
}

static void StatusScreen_Free(void* screen) {
	struct StatusScreen* s = screen;
	Font_Free(&s->Font);
	Screen_CommonFree(s);
	Event_UnregisterVoid(&ChatEvents_FontChanged, s, StatusScreen_FontChanged);
}

struct ScreenVTABLE StatusScreen_VTABLE = {
	StatusScreen_Init,     StatusScreen_Render, StatusScreen_Free,    Gui_DefaultRecreate,
	Gui_DefaultKey,        Gui_DefaultKey,      Gui_DefaultKeyPress,
	Gui_DefaultMouse,      Gui_DefaultMouse,    Gui_DefaultMouseMove, Gui_DefaultMouseScroll,
	StatusScreen_OnResize, StatusScreen_ContextLost, StatusScreen_ContextRecreated,
};
struct Screen* StatusScreen_MakeInstance(void) {
	struct StatusScreen* s = &StatusScreen_Instance;
	s->HandlesAllInput = false;

	s->VTABLE = &StatusScreen_VTABLE;
	return (struct Screen*)s;
}

static void StatusScreen_Ready(void) { Elem_Init(&StatusScreen_Instance); }
void StatusScreen_MakeComponent(struct IGameComponent* comp) {
	comp->Ready = StatusScreen_Ready;
}


/*########################################################################################################################*
*------------------------------------------------------LoadingScreen------------------------------------------------------*
*#########################################################################################################################*/
struct LoadingScreen LoadingScreen_Instance;
static void LoadingScreen_SetTitle(struct LoadingScreen* s) {
	Elem_TryFree(&s->Title);
	TextWidget_Create(&s->Title, &s->TitleStr, &s->Font);
	Widget_SetLocation(&s->Title, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -31);
}

static void LoadingScreen_SetMessage(struct LoadingScreen* s) {
	Elem_TryFree(&s->Message);
	TextWidget_Create(&s->Message, &s->MessageStr, &s->Font);
	Widget_SetLocation(&s->Message, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 17);
}

static void LoadingScreen_MapLoading(void* screen, Real32 progress) {
	struct LoadingScreen* s = screen;
	s->Progress = progress;
}

static void LoadingScreen_OnResize(void* screen) {
	struct LoadingScreen* s = screen;
	Widget_Reposition(&s->Title);
	Widget_Reposition(&s->Message);
}

static void LoadingScreen_ContextLost(void* screen) {
	struct LoadingScreen* s = screen;
	Elem_TryFree(&s->Title);
	Elem_TryFree(&s->Message);
}

static void LoadingScreen_ContextRecreated(void* screen) {
	struct LoadingScreen* s = screen;
	LoadingScreen_SetTitle(s);
	LoadingScreen_SetMessage(s);
}

static bool LoadingScreen_KeyDown(void* sceen, Key key) {
	if (key == Key_Tab) return true;
	return Elem_HandlesKeyDown(Gui_HUD, key);
}

static bool LoadingScreen_KeyPress(void* scren, char keyChar) {
	return Elem_HandlesKeyPress(Gui_HUD, keyChar);
}

static bool LoadingScreen_KeyUp(void* screen, Key key) {
	if (key == Key_Tab) return true;
	return Elem_HandlesKeyUp(Gui_HUD, key);
}

static bool LoadingScreen_MouseDown(void* screen, Int32 x, Int32 y, MouseButton btn) {
	if (Gui_HUD->HandlesAllInput) { Elem_HandlesMouseDown(Gui_HUD, x, y, btn); }
	return true;
}

static bool LoadingScreen_MouseUp(void* screen, Int32 x, Int32 y, MouseButton btn) { return true; }
static bool LoadingScreen_MouseMove(void* screen, Int32 x, Int32 y) { return true; }
static bool LoadingScreen_MouseScroll(void* screen, Real32 delta) { return true; }

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

static void LoadingScreen_Init(void* screen) {
	struct LoadingScreen* s = screen;
	Font_Make(&s->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);

	Gfx_SetFog(false);
	Event_RegisterReal(&WorldEvents_Loading, s, LoadingScreen_MapLoading);
}

#define PROG_BAR_WIDTH 200
#define PROG_BAR_HEIGHT 4
static void LoadingScreen_Render(void* screen, Real64 delta) {
	struct LoadingScreen* s = screen;
	Gfx_SetTexturing(true);
	LoadingScreen_DrawBackground();

	Elem_Render(&s->Title, delta);
	Elem_Render(&s->Message, delta);
	Gfx_SetTexturing(false);

	Int32 x = Gui_CalcPos(ANCHOR_CENTRE,  0, PROG_BAR_WIDTH,  Game_Width);
	Int32 y = Gui_CalcPos(ANCHOR_CENTRE, 34, PROG_BAR_HEIGHT, Game_Height);
	Int32 progWidth = (Int32)(PROG_BAR_WIDTH * s->Progress);

	PackedCol backCol = PACKEDCOL_CONST(128, 128, 128, 255);
	PackedCol progCol = PACKEDCOL_CONST(128, 255, 128, 255);
	GfxCommon_Draw2DFlat(x, y, PROG_BAR_WIDTH, PROG_BAR_HEIGHT, backCol);
	GfxCommon_Draw2DFlat(x, y, progWidth,      PROG_BAR_HEIGHT, progCol);
}

static void LoadingScreen_Free(void* screen) {
	struct LoadingScreen* s = screen;
	Font_Free(&s->Font);
	Screen_CommonFree(s);
	Event_UnregisterReal(&WorldEvents_Loading, s, LoadingScreen_MapLoading);
}

struct ScreenVTABLE LoadingScreen_VTABLE = {
	LoadingScreen_Init,      LoadingScreen_Render,  LoadingScreen_Free,      Gui_DefaultRecreate,
	LoadingScreen_KeyDown,   LoadingScreen_KeyUp,   LoadingScreen_KeyPress,
	LoadingScreen_MouseDown, LoadingScreen_MouseUp, LoadingScreen_MouseMove, LoadingScreen_MouseScroll,
	LoadingScreen_OnResize,  LoadingScreen_ContextLost, LoadingScreen_ContextRecreated,
};
struct Screen* LoadingScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message) {
	struct LoadingScreen* s = &LoadingScreen_Instance;
	s->LastState = NULL;
	s->VTABLE    = &LoadingScreen_VTABLE;

	String title_copy = String_FromArray(s->__TitleBuffer);
	String_AppendString(&title_copy, title);
	s->TitleStr  = title_copy;

	String message_copy = String_FromArray(s->__MessageBuffer);
	String_AppendString(&message_copy, message);
	s->MessageStr  = message_copy;
	
	s->HandlesAllInput = true;
	s->BlocksWorld     = true;
	s->RenderHUDOver   = true;
	return (struct Screen*)s;
}
struct Screen* LoadingScreen_UNSAFE_RawPointer = (struct Screen*)&LoadingScreen_Instance;


/*########################################################################################################################*
*--------------------------------------------------GeneratingMapScreen----------------------------------------------------*
*#########################################################################################################################*/
static void GeneratingScreen_Init(void* screen) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);
	Gen_Done = false;
	LoadingScreen_Init(screen);

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

static void GeneratingScreen_Render(void* screen, Real64 delta) {
	struct LoadingScreen* s = screen;
	LoadingScreen_Render(s, delta);
	if (Gen_Done) { GeneratingScreen_EndGeneration(); return; }

	const volatile char* state = Gen_CurrentState;
	s->Progress = Gen_CurrentProgress;
	if (state == s->LastState) return;

	String message = s->MessageStr;
	message.length = 0;
	String_AppendConst(&message, state);
	LoadingScreen_SetMessage(s);
}

struct ScreenVTABLE GeneratingScreen_VTABLE = {
	GeneratingScreen_Init,   GeneratingScreen_Render, LoadingScreen_Free,      Gui_DefaultRecreate,
	LoadingScreen_KeyDown,   LoadingScreen_KeyUp,     LoadingScreen_KeyPress,
	LoadingScreen_MouseDown, LoadingScreen_MouseUp,   LoadingScreen_MouseMove, LoadingScreen_MouseScroll,
	LoadingScreen_OnResize,  LoadingScreen_ContextLost, LoadingScreen_ContextRecreated,
};
struct Screen* GeneratingScreen_MakeInstance(void) {
	String title   = String_FromConst("Generating level");
	String message = String_FromConst("Generating..");

	struct Screen* s = LoadingScreen_MakeInstance(&title, &message);
	s->VTABLE = &GeneratingScreen_VTABLE;
	return s;
}


/*########################################################################################################################*
*--------------------------------------------------------ChatScreen-------------------------------------------------------*
*#########################################################################################################################*/
struct ChatScreen ChatScreen_Instance;
static Int32 ChatScreen_BottomOffset(void) { return ((struct HUDScreen*)Gui_HUD)->Hotbar.Height; }
static Int32 ChatScreen_InputUsedHeight(struct ChatScreen* s) {
	if (s->AltText.Height == 0) {
		return s->Input.Base.Height + 20;
	} else {
		return (Game_Height - s->AltText.Y) + 5;
	}
}

static void ChatScreen_UpdateAltTextY(struct ChatScreen* s) {
	struct InputWidget* input = &s->Input.Base;
	Int32 height = max(input->Height + input->YOffset, ChatScreen_BottomOffset());
	height += input->YOffset;

	s->AltText.Tex.Y = Game_Height - (height + s->AltText.Tex.Height);
	s->AltText.Y     = s->AltText.Tex.Y;
}

static void ChatScreen_SetHandlesAllInput(struct ChatScreen* s, bool handles) {
	s->HandlesAllInput       = handles;
	Gui_HUD->HandlesAllInput = handles;
	Gui_CalcCursorVisible();
}

static void ChatScreen_OpenInput(struct ChatScreen* s, STRING_PURE String* initialText) {
	s->SuppressNextPress = true;
	ChatScreen_SetHandlesAllInput(s, true);
	Key_KeyRepeat = true;

	String_Copy(&s->Input.Base.Text, initialText);
	Elem_Recreate(&s->Input.Base);
}

static void ChatScreen_ResetChat(struct ChatScreen* s) {
	Elem_TryFree(&s->Chat);
	Int32 i;
	for (i = s->ChatIndex; i < s->ChatIndex + Game_ChatLines; i++) {
		if (i >= 0 && i < Chat_Log.Count) {
			String msg = StringsBuffer_UNSAFE_Get(&Chat_Log, i);
			TextGroupWidget_PushUpAndReplaceLast(&s->Chat, &msg);
		}
	}
}

static void ChatScreen_ConstructWidgets(struct ChatScreen* s) {
#define ChatScreen_MakeGroup(widget, lines, textures, buffer) TextGroupWidget_Create(widget, lines, &s->ChatFont, &s->ChatUrlFont, textures, buffer);
	Int32 yOffset = ChatScreen_BottomOffset() + 15;

	ChatInputWidget_Create(&s->Input, &s->ChatFont);
	Widget_SetLocation(&s->Input.Base, ANCHOR_MIN, ANCHOR_MAX, 5, 5);

	SpecialInputWidget_Create(&s->AltText, &s->ChatFont, &s->Input.Base);
	Elem_Init(&s->AltText);
	ChatScreen_UpdateAltTextY(s);

	ChatScreen_MakeGroup(&s->Status, CHATSCREEN_MAX_STATUS, s->Status_Textures, s->Status_Buffer);
	Widget_SetLocation(&s->Status, ANCHOR_MAX, ANCHOR_MIN, 0, 0);
	Elem_Init(&s->Status);
	TextGroupWidget_SetUsePlaceHolder(&s->Status, 0, false);
	TextGroupWidget_SetUsePlaceHolder(&s->Status, 1, false);

	ChatScreen_MakeGroup(&s->BottomRight, CHATSCREEN_MAX_GROUP, s->BottomRight_Textures, s->BottomRight_Buffer);
	Widget_SetLocation(&s->BottomRight, ANCHOR_MAX, ANCHOR_MAX, 0, yOffset);
	Elem_Init(&s->BottomRight);

	ChatScreen_MakeGroup(&s->Chat, Game_ChatLines, s->Chat_Textures, s->Chat_Buffer);
	Widget_SetLocation(&s->Chat, ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Elem_Init(&s->Chat);

	ChatScreen_MakeGroup(&s->ClientStatus, CHATSCREEN_MAX_GROUP, s->ClientStatus_Textures, s->ClientStatus_Buffer);
	Widget_SetLocation(&s->ClientStatus, ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Elem_Init(&s->ClientStatus);

	String empty = String_MakeNull();
	TextWidget_Create(&s->Announcement, &empty, &s->AnnouncementFont);
	Widget_SetLocation(&s->Announcement, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -Game_Height / 4);
}

static void ChatScreen_SetInitialMessages(struct ChatScreen* s) {
	s->ChatIndex = Chat_Log.Count - Game_ChatLines;
	ChatScreen_ResetChat(s);
	String msg;
#define ChatScreen_Set(group, idx, src) msg = String_FromRawArray(src.Buffer); TextGroupWidget_SetText(group, idx, &msg);

	ChatScreen_Set(&s->Status, 2, Chat_Status[0]);
	ChatScreen_Set(&s->Status, 3, Chat_Status[1]);
	ChatScreen_Set(&s->Status, 4, Chat_Status[2]);

	ChatScreen_Set(&s->BottomRight, 2, Chat_BottomRight[0]);
	ChatScreen_Set(&s->BottomRight, 1, Chat_BottomRight[1]);
	ChatScreen_Set(&s->BottomRight, 0, Chat_BottomRight[2]);

	msg = String_FromRawArray(Chat_Announcement.Buffer);
	TextWidget_Set(&s->Announcement, &msg, &s->AnnouncementFont);

	Int32 i;
	for (i = 0; i < s->ClientStatus.LinesCount; i++) {
		ChatScreen_Set(&s->ClientStatus, i, Chat_ClientStatus[i]);
	}

	if (s->HandlesAllInput) {
		ChatScreen_OpenInput(s, &s->ChatInInputStr);
		s->ChatInInputStr.length = 0;
	}
}

static void ChatScreen_CheckOtherStatuses(struct ChatScreen* s) {
	struct AsyncRequest request;
	Int32 progress;
	bool hasRequest = AsyncDownloader_GetCurrent(&request, &progress);

	String id = String_FromRawArray(request.ID);
	String texPack = String_FromConst("texturePack");

	/* Is terrain / texture pack currently being downloaded? */
	if (!hasRequest || !String_Equals(&id, &texPack)) {
		if (s->Status.Textures[1].ID) {
			String empty = String_MakeNull();
			TextGroupWidget_SetText(&s->Status, 1, &empty);
		}
		s->LastDownloadStatus = Int32_MinValue;
		return;
	}

	if (progress == s->LastDownloadStatus) return;
	s->LastDownloadStatus = progress;
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
	TextGroupWidget_SetText(&s->Status, 1, &str);
}

static void ChatScreen_RenderBackground(struct ChatScreen* s) {
	Int32 usedHeight = TextGroupWidget_UsedHeight(&s->Chat);
	Int32 x = s->Chat.X;
	Int32 y = s->Chat.Y + s->Chat.Height - usedHeight;

	Int32 width  = max(s->ClientStatus.Width, s->Chat.Width);
	Int32 height = usedHeight + TextGroupWidget_UsedHeight(&s->ClientStatus);

	if (height > 0) {
		PackedCol backCol = PACKEDCOL_CONST(0, 0, 0, 127);
		GfxCommon_Draw2DFlat(x - 5, y - 5, width + 10, height + 10, backCol);
	}
}

static void ChatScreen_UpdateChatYOffset(struct ChatScreen* s, bool force) {
	Int32 height = ChatScreen_InputUsedHeight(s);
	if (force || height != s->InputOldHeight) {
		Int32 bottomOffset = ChatScreen_BottomOffset() + 15;
		s->ClientStatus.YOffset = max(bottomOffset, height);
		Widget_Reposition(&s->ClientStatus);

		s->Chat.YOffset = s->ClientStatus.YOffset + TextGroupWidget_UsedHeight(&s->ClientStatus);
		Widget_Reposition(&s->Chat);
		s->InputOldHeight = height;
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

static void ChatScreen_ScrollHistoryBy(struct ChatScreen* s, Int32 delta) {
	Int32 newIndex = ChatScreen_ClampIndex(s->ChatIndex + delta);
	if (newIndex == s->ChatIndex) return;

	s->ChatIndex = newIndex;
	ChatScreen_ResetChat(s);
}

static bool ChatScreen_KeyDown(void* screen, Key key) {
	struct ChatScreen* s = screen;
	s->SuppressNextPress = false;

	if (s->HandlesAllInput) { /* text input bar */
		if (key == KeyBind_Get(KeyBind_SendChat) || key == Key_KeypadEnter || key == KeyBind_Get(KeyBind_PauseOrExit)) {
			ChatScreen_SetHandlesAllInput(s, false);
			Key_KeyRepeat = false;

			if (key == KeyBind_Get(KeyBind_PauseOrExit)) {
				InputWidget_Clear(&s->Input.Base);
			}

			struct InputWidget* input = &s->Input.Base;
			input->OnPressedEnter(input);
			SpecialInputWidget_SetActive(&s->AltText, false);

			/* Reset chat when user has scrolled up in chat history */
			Int32 defaultIndex = Chat_Log.Count - Game_ChatLines;
			if (s->ChatIndex != defaultIndex) {
				s->ChatIndex = ChatScreen_ClampIndex(defaultIndex);
				ChatScreen_ResetChat(s);
			}
		} else if (key == Key_PageUp) {
			ChatScreen_ScrollHistoryBy(s, -Game_ChatLines);
		} else if (key == Key_PageDown) {
			ChatScreen_ScrollHistoryBy(s, +Game_ChatLines);
		} else {
			Elem_HandlesKeyDown(&s->Input.Base, key);
			ChatScreen_UpdateAltTextY(s);
		}
		return key < Key_F1 || key > Key_F35;
	}

	if (key == KeyBind_Get(KeyBind_Chat)) {
		String empty = String_MakeNull();
		ChatScreen_OpenInput(s, &empty);
	} else if (key == Key_Slash) {
		String slash = String_FromConst("/");
		ChatScreen_OpenInput(s, &slash);
	} else {
		return false;
	}
	return true;
}

static bool ChatScreen_KeyUp(void* screen, Key key) {
	struct ChatScreen* s = screen;
	if (!s->HandlesAllInput) return false;

	if (ServerConnection_SupportsFullCP437 && key == KeyBind_Get(KeyBind_ExtInput)) {
		if (Window_Focused) {
			bool active = !s->AltText.Active;
			SpecialInputWidget_SetActive(&s->AltText, active);
		}
	}
	return true;
}

static bool ChatScreen_KeyPress(void* screen, char keyChar) {
	struct ChatScreen* s = screen;
	if (!s->HandlesAllInput) return false;

	if (s->SuppressNextPress) {
		s->SuppressNextPress = false;
		return false;
	}

	bool handled = Elem_HandlesKeyPress(&s->Input.Base, keyChar);
	ChatScreen_UpdateAltTextY(s);
	return handled;
}

static bool ChatScreen_MouseScroll(void* screen, Real32 delta) {
	struct ChatScreen* s = screen;
	if (!s->HandlesAllInput) return false;

	Int32 steps = Utils_AccumulateWheelDelta(&s->ChatAcc, delta);
	ChatScreen_ScrollHistoryBy(s, -steps);
	return true;
}

static bool ChatScreen_MouseDown(void* screen, Int32 x, Int32 y, MouseButton btn) {
	struct ChatScreen* s = screen;
	if (!s->HandlesAllInput || Game_HideGui) return false;

	if (!Widget_Contains(&s->Chat, x, y)) {
		if (s->AltText.Active && Widget_Contains(&s->AltText, x, y)) {
			Elem_HandlesMouseDown(&s->AltText, x, y, btn);
			ChatScreen_UpdateAltTextY(s);
			return true;
		}
		Elem_HandlesMouseDown(&s->Input.Base, x, y, btn);
		return true;
	}

	Int32 height = TextGroupWidget_UsedHeight(&s->Chat);
	Int32 chatY = s->Chat.Y + s->Chat.Height - height;
	if (!Gui_Contains(s->Chat.X, chatY, s->Chat.Width, height, x, y)) return false;

	char textBuffer[TEXTGROUPWIDGET_LEN];
	String text = String_FromArray(textBuffer);
	TextGroupWidget_GetSelected(&s->Chat, &text, x, y);
	if (!text.length) return false;

	if (Utils_IsUrlPrefix(&text, 0)) {
		struct Screen* overlay = UrlWarningOverlay_MakeInstance(&text);
		Gui_ShowOverlay(overlay, false);
	} else if (Game_ClickableChat) {
		InputWidget_AppendString(&s->Input.Base, &text);
	}
	return true;
}

static void ChatScreen_ColCodeChanged(void* screen, Int32 code) {
	struct ChatScreen* s = screen;
	if (Gfx_LostContext) return;

	SpecialInputWidget_UpdateCols(&s->AltText);
	ChatElem_Recreate(&s->Chat, code);
	ChatElem_Recreate(&s->Status, code);
	ChatElem_Recreate(&s->BottomRight, code);
	ChatElem_Recreate(&s->ClientStatus, code);

	/* Some servers have plugins that redefine colours constantly */
	/* Preserve caret accumulator so caret blinking stays consistent */
	Real64 caretAcc = s->Input.Base.CaretAccumulator;
	Elem_Recreate(&s->Input.Base);
	s->Input.Base.CaretAccumulator = caretAcc;
}

static void ChatScreen_ChatReceived(void* screen, String* msg, Int32 type) {
	struct ChatScreen* s = screen;
	if (Gfx_LostContext) return;

	if (type == MSG_TYPE_NORMAL) {
		s->ChatIndex++;
		if (!Game_ChatLines) return;

		Int32 i = s->ChatIndex + (Game_ChatLines - 1);
		String chatMsg = *msg;

		if (i < Chat_Log.Count) { chatMsg = StringsBuffer_UNSAFE_Get(&Chat_Log, i); }
		TextGroupWidget_PushUpAndReplaceLast(&s->Chat, &chatMsg);
	} else if (type >= MSG_TYPE_STATUS_1 && type <= MSG_TYPE_STATUS_3) {
		TextGroupWidget_SetText(&s->Status, 2 + (type - MSG_TYPE_STATUS_1), msg);
	} else if (type >= MSG_TYPE_BOTTOMRIGHT_1 && type <= MSG_TYPE_BOTTOMRIGHT_3) {
		TextGroupWidget_SetText(&s->BottomRight, 2 - (type - MSG_TYPE_BOTTOMRIGHT_1), msg);
	} else if (type == MSG_TYPE_ANNOUNCEMENT) {
		TextWidget_Set(&s->Announcement, msg, &s->AnnouncementFont);
	} else if (type >= MSG_TYPE_CLIENTSTATUS_1 && type <= MSG_TYPE_CLIENTSTATUS_3) {
		TextGroupWidget_SetText(&s->ClientStatus, type - MSG_TYPE_CLIENTSTATUS_1, msg);
		ChatScreen_UpdateChatYOffset(s, true);
	}
}

static void ChatScreen_ContextLost(void* screen) {
	struct ChatScreen* s = screen;
	if (s->HandlesAllInput) {
		String_Copy(&s->ChatInInputStr, &s->Input.Base.Text);
		Gui_CalcCursorVisible();
	}

	Elem_TryFree(&s->Chat);
	Elem_TryFree(&s->Input.Base);
	Elem_TryFree(&s->AltText);
	Elem_TryFree(&s->Status);
	Elem_TryFree(&s->BottomRight);
	Elem_TryFree(&s->ClientStatus);
	Elem_TryFree(&s->Announcement);
}

static void ChatScreen_ContextRecreated(void* screen) {
	struct ChatScreen* s = screen;
	ChatScreen_ConstructWidgets(s);
	ChatScreen_SetInitialMessages(s);
}

static void ChatScreen_FontChanged(void* screen) {
	struct ChatScreen* s = screen;
	if (!Drawer2D_UseBitmappedChat) return;
	Elem_Recreate(s);
	ChatScreen_UpdateChatYOffset(s, true);
}

static void ChatScreen_OnResize(void* screen) {
	struct ChatScreen* s = screen;
	bool active = s->AltText.Active;
	Elem_Recreate(s);
	SpecialInputWidget_SetActive(&s->AltText, active);
}

static void ChatScreen_Init(void* screen) {
	struct ChatScreen* s = screen;
	String str = String_FromArray(s->__ChatInInputBuffer);
	s->ChatInInputStr = str;

	Int32 fontSize = (Int32)(8 * Game_GetChatScale());
	Math_Clamp(fontSize, 8, 60);
	Int32 announceSize = (Int32)(16 * Game_GetChatScale());
	Math_Clamp(announceSize, 8, 60);

	Font_Make(&s->ChatFont,         &Game_FontName, fontSize,     FONT_STYLE_NORMAL);
	Font_Make(&s->ChatUrlFont,      &Game_FontName, fontSize,     FONT_STYLE_UNDERLINE);
	Font_Make(&s->AnnouncementFont, &Game_FontName, announceSize, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);

	Event_RegisterChat(&ChatEvents_ChatReceived,    s, ChatScreen_ChatReceived);
	Event_RegisterVoid(&ChatEvents_FontChanged,     s, ChatScreen_FontChanged);
	Event_RegisterInt(&ChatEvents_ColCodeChanged,   s, ChatScreen_ColCodeChanged);
}

static void ChatScreen_Render(void* screen, Real64 delta) {
	struct ChatScreen* s = screen;
	ChatScreen_CheckOtherStatuses(s);
	if (!Game_PureClassic) { Elem_Render(&s->Status, delta); }
	Elem_Render(&s->BottomRight, delta);

	ChatScreen_UpdateChatYOffset(s, false);
	Int32 i, y = s->ClientStatus.Y + s->ClientStatus.Height;
	for (i = 0; i < s->ClientStatus.LinesCount; i++) {
		struct Texture tex = s->ClientStatus.Textures[i];
		if (!tex.ID) continue;

		y -= tex.Height; tex.Y = y;
		Texture_Render(&tex);
	}

	UInt64 now = DateTime_CurrentUTC_MS();
	if (s->HandlesAllInput) {
		Elem_Render(&s->Chat, delta);
	} else {
		/* Only render recent chat */
		for (i = 0; i < s->Chat.LinesCount; i++) {
			struct Texture tex = s->Chat.Textures[i];
			Int32 logIdx = s->ChatIndex + i;
			if (!tex.ID) continue;
			if (logIdx < 0 || logIdx >= Chat_Log.Count) continue;

			if (Chat_GetLogTime(logIdx) + (10 * 1000) >= now) Texture_Render(&tex);
		}
	}

	Elem_Render(&s->Announcement, delta);
	if (s->HandlesAllInput) {
		Elem_Render(&s->Input.Base, delta);
		if (s->AltText.Active) {
			Elem_Render(&s->AltText, delta);
		}
	}

	if (s->Announcement.Texture.ID && Chat_Announcement.Received + (5 * 1000) >= now) {
		Elem_TryFree(&s->Announcement);
	}
}

static void ChatScreen_Free(void* screen) {
	struct ChatScreen* s = screen;
	Font_Free(&s->ChatFont);
	Font_Free(&s->ChatUrlFont);
	Font_Free(&s->AnnouncementFont);
	Screen_CommonFree(s);

	Event_UnregisterChat(&ChatEvents_ChatReceived,    s, ChatScreen_ChatReceived);
	Event_UnregisterVoid(&ChatEvents_FontChanged,     s, ChatScreen_FontChanged);
	Event_UnregisterInt(&ChatEvents_ColCodeChanged,   s, ChatScreen_ColCodeChanged);
}

struct ScreenVTABLE ChatScreen_VTABLE = {
	ChatScreen_Init,      ChatScreen_Render, ChatScreen_Free,      Gui_DefaultRecreate,
	ChatScreen_KeyDown,   ChatScreen_KeyUp,  ChatScreen_KeyPress,
	ChatScreen_MouseDown, Gui_DefaultMouse,  Gui_DefaultMouseMove, ChatScreen_MouseScroll,
	ChatScreen_OnResize,  ChatScreen_ContextLost, ChatScreen_ContextRecreated,
};
struct Screen* ChatScreen_MakeInstance(void) {
	struct ChatScreen* s = &ChatScreen_Instance;
	s->InputOldHeight     = -1;
	s->LastDownloadStatus = Int32_MinValue;
	s->HUD = Gui_HUD;

	s->VTABLE = &ChatScreen_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*--------------------------------------------------------HUDScreen--------------------------------------------------------*
*#########################################################################################################################*/
struct HUDScreen HUDScreen_Instance;
#define CH_EXTENT 16

static void HUDScreen_DrawCrosshairs(void) {
	if (!Gui_IconsTex) return;

	Int32 extent = (Int32)(CH_EXTENT * Game_Scale(Game_Height / 480.0f)), size = extent * 2;
	Int32 chX = (Game_Width / 2) - extent, chY = (Game_Height / 2) - extent;

	struct Texture tex = { Gui_IconsTex, TEX_RECT(chX,chY, size, size), TEX_UV(0.0f,0.0f, 15/256.0f,15/256.0f) };
	Texture_Render(&tex);
}

static void HUDScreen_ContextLost(void* screen) {
	struct HUDScreen* s = screen;
	Elem_TryFree(&s->Hotbar);

	s->WasShowingList = s->ShowingList;
	if (s->ShowingList) { Elem_TryFree(&s->PlayerList); }
	s->ShowingList = false;
}

static void HUDScreen_ContextRecreated(void* screen) {
	struct HUDScreen* s = screen;
	Elem_TryFree(&s->Hotbar);
	Elem_Init(&s->Hotbar);

	if (!s->WasShowingList) return;
	bool extended = ServerConnection_SupportsExtPlayerList && !Game_UseClassicTabList;
	PlayerListWidget_Create(&s->PlayerList, &s->PlayerFont, !extended);
	s->ShowingList = true;

	Elem_Init(&s->PlayerList);
	Widget_Reposition(&s->PlayerList);
}

static void HUDScreen_OnResize(void* screen) {
	struct HUDScreen* s = screen;
	Screen_OnResize(s->Chat);

	Widget_Reposition(&s->Hotbar);
	if (s->ShowingList) { Widget_Reposition(&s->PlayerList); }
}

static bool HUDScreen_KeyPress(void* screen, char keyChar) {
	struct HUDScreen* s = screen;
	return Elem_HandlesKeyPress(s->Chat, keyChar); 
}

static bool HUDScreen_KeyDown(void* screen, Key key) {
	struct HUDScreen* s = screen;
	Key playerListKey = KeyBind_Get(KeyBind_PlayerList);
	bool handles = playerListKey != Key_Tab || !Game_TabAutocomplete || !s->Chat->HandlesAllInput;

	if (key == playerListKey && handles) {
		if (!s->ShowingList && !ServerConnection_IsSinglePlayer) {
			s->WasShowingList = true;
			HUDScreen_ContextRecreated(s);
		}
		return true;
	}

	return Elem_HandlesKeyDown(s->Chat, key) || Elem_HandlesKeyDown(&s->Hotbar, key);
}

static bool HUDScreen_KeyUp(void* screen, Key key) {
	struct HUDScreen* s = screen;
	if (key == KeyBind_Get(KeyBind_PlayerList) && s->ShowingList) {
		s->ShowingList    = false;
		s->WasShowingList = false;
		Elem_TryFree(&s->PlayerList);
		return true;
	}

	return Elem_HandlesKeyUp(s->Chat, key) || Elem_HandlesKeyUp(&s->Hotbar, key);
}

static bool HUDScreen_MouseScroll(void* screen, Real32 delta) {
	struct HUDScreen* s = screen;
	return Elem_HandlesMouseScroll(s->Chat, delta);
}

static bool HUDScreen_MouseDown(void* screen, Int32 x, Int32 y, MouseButton btn) {
	struct HUDScreen* s = screen;
	if (btn != MouseButton_Left || !s->HandlesAllInput) return false;
	struct ChatScreen* chat = (struct ChatScreen*)s->Chat;
	if (!s->ShowingList) { return Elem_HandlesMouseDown(chat, x, y, btn); }

	char nameBuffer[STRING_SIZE + 1];
	String name = String_FromArray(nameBuffer);
	PlayerListWidget_GetNameUnder(&s->PlayerList, x, y, &name);
	if (!name.length) { return Elem_HandlesMouseDown(chat, x, y, btn); }

	String_Append(&name, ' ');
	if (chat->HandlesAllInput) { InputWidget_AppendString(&chat->Input.Base, &name); }
	return true;
}

static void HUDScreen_Init(void* screen) {
	struct HUDScreen* s = screen;
	UInt16 size = Drawer2D_UseBitmappedChat ? 16 : 11;
	Font_Make(&s->PlayerFont, &Game_FontName, size, FONT_STYLE_NORMAL);

	ChatScreen_MakeInstance();
	s->Chat = (struct Screen*)&ChatScreen_Instance;
	Elem_Init(s->Chat);

	HotbarWidget_Create(&s->Hotbar);
	s->WasShowingList = false;
	Screen_CommonInit(s);
}

static void HUDScreen_Render(void* screen, Real64 delta) {
	struct HUDScreen* s = screen;
	struct ChatScreen* chat = (struct ChatScreen*)s->Chat;
	if (Game_HideGui && chat->HandlesAllInput) {
		Gfx_SetTexturing(true);
		Elem_Render(&chat->Input.Base, delta);
		Gfx_SetTexturing(false);
	}
	if (Game_HideGui) return;
	bool showMinimal = Gui_GetActiveScreen()->BlocksWorld;

	if (!s->ShowingList && !showMinimal) {
		Gfx_SetTexturing(true);
		HUDScreen_DrawCrosshairs();
		Gfx_SetTexturing(false);
	}
	if (chat->HandlesAllInput && !Game_PureClassic) {
		ChatScreen_RenderBackground(chat);
	}

	Gfx_SetTexturing(true);
	if (!showMinimal) { Elem_Render(&s->Hotbar, delta); }
	Elem_Render(s->Chat, delta);

	if (s->ShowingList && Gui_GetActiveScreen() == (struct Screen*)s) {
		s->PlayerList.Active = s->Chat->HandlesAllInput;
		Elem_Render(&s->PlayerList, delta);
		/* NOTE: Should usually be caught by KeyUp, but just in case. */
		if (!KeyBind_IsPressed(KeyBind_PlayerList)) {
			Elem_TryFree(&s->PlayerList);
			s->ShowingList = false;
		}
	}
	Gfx_SetTexturing(false);
}

static void HUDScreen_Free(void* screen) {
	struct HUDScreen* s = screen;
	Font_Free(&s->PlayerFont);
	Elem_TryFree(s->Chat);
	Screen_CommonFree(s);
}

struct ScreenVTABLE HUDScreen_VTABLE = {
	HUDScreen_Init,      HUDScreen_Render, HUDScreen_Free,       Gui_DefaultRecreate,
	HUDScreen_KeyDown,   HUDScreen_KeyUp,  HUDScreen_KeyPress,
	HUDScreen_MouseDown, Gui_DefaultMouse, Gui_DefaultMouseMove, HUDScreen_MouseScroll,
	HUDScreen_OnResize,  HUDScreen_ContextLost, HUDScreen_ContextRecreated,
};
struct Screen* HUDScreen_MakeInstance(void) {
	struct HUDScreen* s = &HUDScreen_Instance;
	s->WasShowingList = false;
	s->VTABLE = &HUDScreen_VTABLE;
	return (struct Screen*)s;
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
	struct HUDScreen* s = (struct HUDScreen*)hud;
	return (struct Widget*)(&s->Hotbar);
}


/*########################################################################################################################*
*----------------------------------------------------DisconnectScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct DisconnectScreen DisconnectScreen_Instance;
#define DISCONNECT_DELAY_MS 5000
static void DisconnectScreen_ReconnectMessage(struct DisconnectScreen* s, STRING_TRANSIENT String* msg) {
	if (s->CanReconnect) {
		Int32 elapsedMS = (Int32)(DateTime_CurrentUTC_MS() - s->InitTime);
		Int32 secsLeft = (DISCONNECT_DELAY_MS - elapsedMS) / DATETIME_MILLIS_PER_SEC;

		if (secsLeft > 0) {
			String_Format1(msg, "Reconnect in %i", &secsLeft); return;
		}
	}
	String_AppendConst(msg, "Reconnect");
}

static void DisconnectScreen_Redraw(struct DisconnectScreen* s, Real64 delta) {
	PackedCol top    = PACKEDCOL_CONST(64, 32, 32, 255);
	PackedCol bottom = PACKEDCOL_CONST(80, 16, 16, 255);
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, top, bottom);

	Gfx_SetTexturing(true);
	Elem_Render(&s->Title, delta);
	Elem_Render(&s->Message, delta);
	if (s->CanReconnect) { Elem_Render(&s->Reconnect, delta); }
	Gfx_SetTexturing(false);
}

static void DisconnectScreen_UpdateDelayLeft(struct DisconnectScreen* s, Real64 delta) {
	Int32 elapsedMS = (Int32)(DateTime_CurrentUTC_MS() - s->InitTime);
	Int32 secsLeft = (DISCONNECT_DELAY_MS - elapsedMS) / DATETIME_MILLIS_PER_SEC;
	if (secsLeft < 0) secsLeft = 0;

	if (s->LastSecsLeft == secsLeft && s->Reconnect.Active == s->LastActive) return;

	char msgBuffer[STRING_SIZE];
	String msg = String_FromArray(msgBuffer);
	DisconnectScreen_ReconnectMessage(s, &msg);
	ButtonWidget_Set(&s->Reconnect, &msg, &s->TitleFont);
	s->Reconnect.Disabled = secsLeft != 0;

	DisconnectScreen_Redraw(s, delta);
	s->LastSecsLeft = secsLeft;
	s->LastActive   = s->Reconnect.Active;
	s->ClearTime    = DateTime_CurrentUTC_MS() + 500;
}

static void DisconnectScreen_ContextLost(void* screen) {
	struct DisconnectScreen* s = screen;
	Elem_TryFree(&s->Title);
	Elem_TryFree(&s->Message);
	Elem_TryFree(&s->Reconnect);
}

static void DisconnectScreen_ContextRecreated(void* screen) {
	struct DisconnectScreen* s = screen;
	s->ClearTime = DateTime_CurrentUTC_MS() + 500;

	TextWidget_Create(&s->Title, &s->TitleStr, &s->TitleFont);
	Widget_SetLocation(&s->Title, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);

	TextWidget_Create(&s->Message, &s->MessageStr, &s->MessageFont);
	Widget_SetLocation(&s->Message, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 10);

	char msgBuffer[STRING_SIZE];
	String msg = String_FromArray(msgBuffer);
	DisconnectScreen_ReconnectMessage(s, &msg);

	ButtonWidget_Create(&s->Reconnect, 300, &msg, &s->TitleFont, NULL);
	Widget_SetLocation(&s->Reconnect, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 80);
	s->Reconnect.Disabled = !s->CanReconnect;
}

static void DisconnectScreen_Init(void* screen) {
	struct DisconnectScreen* s = screen;
	Font_Make(&s->TitleFont,   &Game_FontName, 16, FONT_STYLE_BOLD);
	Font_Make(&s->MessageFont, &Game_FontName, 16, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);

	Game_SkipClear = true;
	s->InitTime     = DateTime_CurrentUTC_MS();
	s->LastSecsLeft = DISCONNECT_DELAY_MS / DATETIME_MILLIS_PER_SEC;
}

static void DisconnectScreen_Render(void* screen, Real64 delta) {
	struct DisconnectScreen* s = screen;
	if (s->CanReconnect) { DisconnectScreen_UpdateDelayLeft(s, delta); }

	/* NOTE: We need to make sure that both the front and back buffers have
	definitely been drawn over, so we redraw the background multiple times. */
	if (DateTime_CurrentUTC_MS() < s->ClearTime) {
		DisconnectScreen_Redraw(s, delta);
	}
}

static void DisconnectScreen_Free(void* screen) {
	struct DisconnectScreen* s = screen;
	Font_Free(&s->TitleFont);
	Font_Free(&s->MessageFont);
	Screen_CommonFree(s);
	Game_SkipClear = false;
}

static void DisconnectScreen_OnResize(void* screen) {
	struct DisconnectScreen* s = screen;
	Widget_Reposition(&s->Title);
	Widget_Reposition(&s->Message);
	Widget_Reposition(&s->Reconnect);
	s->ClearTime = DateTime_CurrentUTC_MS() + 500;
}

static bool DisconnectScreen_KeyDown(void* s, Key key) { return key < Key_F1 || key > Key_F35; }
static bool DisconnectScreen_KeyPress(void* s, char keyChar) { return true; }
static bool DisconnectScreen_KeyUp(void* s, Key key) { return true; }

static bool DisconnectScreen_MouseDown(void* screen, Int32 x, Int32 y, MouseButton btn) {
	struct DisconnectScreen* s = screen;
	struct ButtonWidget* w = &s->Reconnect;
	if (btn != MouseButton_Left) return true;

	if (!w->Disabled && Widget_Contains(w, x, y)) {
		char connectBuffer[STRING_SIZE];
		String title   = String_FromArray(connectBuffer);
		String message = String_MakeNull();
		String_Format2(&title, "Connecting to %s:%i..", &Game_IPAddress, &Game_Port);

		Gui_ReplaceActive(LoadingScreen_MakeInstance(&title, &message));
		ServerConnection_BeginConnect();
	}
	return true;
}

static bool DisconnectScreen_MouseMove(void* screen, Int32 x, Int32 y) {
	struct DisconnectScreen* s = screen;
	struct ButtonWidget* w = &s->Reconnect;
	w->Active = !w->Disabled && Widget_Contains(w, x, y);
	return true;
}

static bool DisconnectScreen_MouseScroll(void* screen, Real32 delta) { return true; }
static bool DisconnectScreen_MouseUp(void* screen, Int32 x, Int32 y, MouseButton btn) { return true; }

struct ScreenVTABLE DisconnectScreen_VTABLE = {
	DisconnectScreen_Init,      DisconnectScreen_Render,  DisconnectScreen_Free,      Gui_DefaultRecreate,
	DisconnectScreen_KeyDown,   DisconnectScreen_KeyUp,   DisconnectScreen_KeyPress,
	DisconnectScreen_MouseDown, DisconnectScreen_MouseUp, DisconnectScreen_MouseMove, DisconnectScreen_MouseScroll,
	DisconnectScreen_OnResize,  DisconnectScreen_ContextLost, DisconnectScreen_ContextRecreated
};
struct Screen* DisconnectScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message) {
	struct DisconnectScreen* s = &DisconnectScreen_Instance;
	s->HandlesAllInput = true;
	s->BlocksWorld     = true;
	s->HidesHUD        = true;

	String title_copy = String_FromArray(s->__TitleBuffer);
	String_AppendString(&title_copy, title);
	s->TitleStr = title_copy;

	String message_copy = String_FromArray(s->__MessageBuffer);
	String_AppendString(&message_copy, message);
	s->MessageStr = message_copy;

	char whyBuffer[STRING_SIZE];
	String why = String_FromArray(whyBuffer);
	String_AppendColorless(&why, message);

	String kick = String_FromConst("Kicked ");
	String ban  = String_FromConst("Banned ");
	s->CanReconnect = 
		!(String_CaselessStarts(&why, &kick) || String_CaselessStarts(&why, &ban));

	s->VTABLE = &DisconnectScreen_VTABLE;
	return (struct Screen*)s;
}
