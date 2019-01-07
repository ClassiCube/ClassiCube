#include "Screens.h"
#include "Widgets.h"
#include "Game.h"
#include "Event.h"
#include "Platform.h"
#include "Inventory.h"
#include "Drawer2D.h"
#include "Graphics.h"
#include "Funcs.h"
#include "TexturePack.h"
#include "Model.h"
#include "MapGenerator.h"
#include "Server.h"
#include "Chat.h"
#include "ExtMath.h"
#include "Window.h"
#include "Camera.h"
#include "Http.h"
#include "Block.h"
#include "Menus.h"
#include "World.h"

struct InventoryScreen {
	Screen_Layout
	FontDesc Font;
	struct TableWidget Table;
	bool ReleasedInv, DeferredSelect;
};

struct StatusScreen {
	Screen_Layout
	FontDesc Font;
	struct TextWidget Line1, Line2;
	struct TextAtlas PosAtlas;
	double Accumulator;
	int Frames, FPS;
	bool Speed, HalfSpeed, Noclip, Fly, CanSpeed;
	int LastFov;
};

struct HUDScreen {
	Screen_Layout
	struct Screen* Chat;
	struct HotbarWidget Hotbar;
	struct PlayerListWidget PlayerList;
	FontDesc PlayerFont;
	bool ShowingList, WasShowingList;
};

struct LoadingScreen {
	Screen_Layout
	FontDesc Font;
	float Progress;
	
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
	int InputOldHeight;
	float ChatAcc;
	bool SuppressNextPress;
	int ChatIndex;
	int LastDownloadStatus;
	FontDesc ChatFont, AnnouncementFont;
	struct TextWidget Announcement;
	struct ChatInputWidget Input;
	struct TextGroupWidget Status, BottomRight, Chat, ClientStatus;
	struct SpecialInputWidget AltText;

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
	TimeMS InitTime;
	bool CanReconnect, LastActive;
	int LastSecsLeft;
	struct ButtonWidget Reconnect;

	FontDesc TitleFont, MessageFont;
	struct TextWidget Title, Message;
	char __TitleBuffer[STRING_SIZE];
	char __MessageBuffer[STRING_SIZE];
	String TitleStr, MessageStr;
};

static bool Screen_Mouse(void* elem, int x, int y, MouseButton btn) { return false; }
static bool Screen_MouseMove(void* elem, int x, int y) { return false; }


/*########################################################################################################################*
*-----------------------------------------------------InventoryScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct InventoryScreen InventoryScreen_Instance;
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
	Drawer2D_MakeFont(&s->Font, 16, FONT_STYLE_NORMAL);

	TableWidget_Create(&s->Table);
	s->Table.Font = s->Font;
	s->Table.ElementsPerRow = Game_PureClassic ? 9 : 10;
	Elem_Init(&s->Table);

	/* Can't immediately move to selected here, because cursor visibility 
	   might be toggled after Init() is called. This causes the cursor to 
	   be moved back to the middle of the window. */
	s->DeferredSelect = true;
	Screen_CommonInit(s);

	Event_RegisterVoid(&BlockEvents.PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_RegisterVoid(&BlockEvents.BlockDefChanged,    s, InventoryScreen_OnBlockChanged);
}

static void InventoryScreen_Render(void* screen, double delta) {
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
	Screen_CommonFree(s);
	
	Event_UnregisterVoid(&BlockEvents.PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&BlockEvents.BlockDefChanged,    s, InventoryScreen_OnBlockChanged);
}

static bool InventoryScreen_KeyDown(void* screen, Key key, bool was) {
	struct InventoryScreen* s = screen;
	struct TableWidget* table = &s->Table;

	if (key == KEY_ESCAPE) {
		Gui_CloseActive();
	} else if (key == KeyBind_Get(KEYBIND_INVENTORY) && s->ReleasedInv) {
		Gui_CloseActive();
	} else if (key == KEY_ENTER && table->SelectedIndex != -1) {
		Inventory_SetSelectedBlock(table->Elements[table->SelectedIndex]);
		Gui_CloseActive();
	} else if (Elem_HandlesKeyDown(table, key, was)) {
	} else {
		struct HUDScreen* hud = (struct HUDScreen*)Gui_HUD;
		return Elem_HandlesKeyDown(&hud->Hotbar, key, was);
	}
	return true;
}

static bool InventoryScreen_KeyPress(void* elem, char keyChar) { return true; }
static bool InventoryScreen_KeyUp(void* screen, Key key) {
	struct InventoryScreen* s = screen;
	struct HUDScreen* hud;

	if (key == KeyBind_Get(KEYBIND_INVENTORY)) {
		s->ReleasedInv = true; return true;
	}

	hud = (struct HUDScreen*)Gui_HUD;
	return Elem_HandlesKeyUp(&hud->Hotbar, key);
}

static bool InventoryScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	struct InventoryScreen* s = screen;
	struct TableWidget* table = &s->Table;
	struct HUDScreen* hud     = (struct HUDScreen*)Gui_HUD;
	bool handled, hotbar;

	if (table->Scroll.DraggingMouse || Elem_HandlesMouseDown(&hud->Hotbar, x, y, btn)) return true;
	handled = Elem_HandlesMouseDown(table, x, y, btn);

	if ((!handled || table->PendingClose) && btn == MOUSE_LEFT) {
		hotbar = Key_IsControlPressed() || Key_IsShiftPressed();
		if (!hotbar) Gui_CloseActive();
	}
	return true;
}

static bool InventoryScreen_MouseUp(void* screen, int x, int y, MouseButton btn) {
	struct InventoryScreen* s = screen;
	struct TableWidget* table = &s->Table;
	return Elem_HandlesMouseUp(table, x, y, btn);
}

static bool InventoryScreen_MouseMove(void* screen, int x, int y) {
	struct InventoryScreen* s = screen;
	struct TableWidget* table = &s->Table;
	return Elem_HandlesMouseMove(table, x, y);
}

static bool InventoryScreen_MouseScroll(void* screen, float delta) {
	struct InventoryScreen* s = screen;
	struct TableWidget* table = &s->Table;

	bool hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (hotbar) return false;
	return Elem_HandlesMouseScroll(table, delta);
}

static struct ScreenVTABLE InventoryScreen_VTABLE = {
	InventoryScreen_Init,      InventoryScreen_Render,  InventoryScreen_Free,      Gui_DefaultRecreate,
	InventoryScreen_KeyDown,   InventoryScreen_KeyUp,   InventoryScreen_KeyPress,
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
static struct StatusScreen StatusScreen_Instance;
static void StatusScreen_MakeText(struct StatusScreen* s, String* status) {
	int indices, ping;
	s->FPS = (int)(s->Frames / s->Accumulator);
	String_Format1(status, "%i fps, ", &s->FPS);

	if (Game_ClassicMode) {
		String_Format1(status, "%i chunk updates", &Game_ChunkUpdates);
	} else {
		if (Game_ChunkUpdates) {
			String_Format1(status, "%i chunks/s, ", &Game_ChunkUpdates);
		}

		indices = ICOUNT(Game_Vertices);
		String_Format1(status, "%i vertices", &indices);

		ping = PingList_AveragePingMs();
		if (ping) String_Format1(status, ", ping %i ms", &ping);
	}
}

static void StatusScreen_DrawPosition(struct StatusScreen* s) {
	VertexP3fT2fC4b vertices[4 * 64];
	VertexP3fT2fC4b* ptr = vertices;
	PackedCol col = PACKEDCOL_WHITE;

	struct TextAtlas* atlas = &s->PosAtlas;
	struct Texture tex;
	Vector3I pos;
	int count;	

	/* Make "Position: " prefix */
	tex = atlas->Tex; 
	tex.X = 2; tex.Width = atlas->Offset;
	Gfx_Make2DQuad(&tex, col, &ptr);

	Vector3I_Floor(&pos, &LocalPlayer_Instance.Base.Position);
	atlas->CurX = atlas->Offset + 2;

	/* Make (X, Y, Z) suffix */
	TextAtlas_Add(atlas, 13, &ptr);
	TextAtlas_AddInt(atlas, pos.X, &ptr);
	TextAtlas_Add(atlas, 11, &ptr);
	TextAtlas_AddInt(atlas, pos.Y, &ptr);
	TextAtlas_Add(atlas, 11, &ptr);
	TextAtlas_AddInt(atlas, pos.Z, &ptr);
	TextAtlas_Add(atlas, 14, &ptr);

	Gfx_BindTexture(atlas->Tex.ID);
	/* TODO: Do we need to use a separate VB here? */
	count = (int)(ptr - vertices);
	Gfx_UpdateDynamicVb_IndexedTris(Models.Vb, vertices, count);
}

static bool StatusScreen_HacksChanged(struct StatusScreen* s) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	return hacks->Speeding != s->Speed || hacks->HalfSpeeding != s->HalfSpeed || hacks->Flying != s->Fly
		|| hacks->Noclip != s->Noclip  || Game_Fov != s->LastFov || hacks->CanSpeed != s->CanSpeed;
}

static void StatusScreen_UpdateHackState(struct StatusScreen* s) {
	String status; char statusBuffer[STRING_SIZE * 2];
	struct HacksComp* hacks;
	bool speeding;

	hacks = &LocalPlayer_Instance.Hacks;
	s->Speed = hacks->Speeding; s->HalfSpeed = hacks->HalfSpeeding; s->Fly = hacks->Flying;
	s->Noclip = hacks->Noclip;  s->LastFov = Game_Fov; s->CanSpeed = hacks->CanSpeed;

	String_InitArray(status, statusBuffer);
	if (Game_Fov != Game_DefaultFov) {
		String_Format1(&status, "Zoom fov %i  ", &Game_Fov);
	}
	speeding = (hacks->Speeding || hacks->HalfSpeeding) && hacks->CanSpeed;

	if (hacks->Flying) String_AppendConst(&status, "Fly ON   ");
	if (speeding)      String_AppendConst(&status, "Speed ON   ");
	if (hacks->Noclip) String_AppendConst(&status, "Noclip ON   ");

	TextWidget_Set(&s->Line2, &status, &s->Font);
}

static void StatusScreen_Update(struct StatusScreen* s, double delta) {
	String status; char statusBuffer[STRING_SIZE * 2];

	s->Frames++;
	s->Accumulator += delta;
	if (s->Accumulator < 1.0) return;

	String_InitArray(status, statusBuffer);
	StatusScreen_MakeText(s, &status);

	TextWidget_Set(&s->Line1, &status, &s->Font);
	s->Accumulator = 0.0;
	s->Frames = 0;
	Game_ChunkUpdates = 0;
}

static void StatusScreen_OnResize(void* screen) { }
static void StatusScreen_ContextLost(void* screen) {
	struct StatusScreen* s = screen;
	TextAtlas_Free(&s->PosAtlas);
	Elem_TryFree(&s->Line1);
	Elem_TryFree(&s->Line2);
}

static void StatusScreen_ContextRecreated(void* screen) {	
	const static String chars   = String_FromConst("0123456789-, ()");
	const static String prefix  = String_FromConst("Position: ");
	const static String version = String_FromConst("0.30");

	struct StatusScreen* s = screen;
	struct TextWidget* line1 = &s->Line1;
	struct TextWidget* line2 = &s->Line2;
	int y;

	y = 2;
	TextWidget_Make(line1);
	Widget_SetLocation(line1, ANCHOR_MIN, ANCHOR_MIN, 2, y);
	line1->ReducePadding = true;
	StatusScreen_Update(s, 1.0);

	y += line1->Height;
	TextAtlas_Make(&s->PosAtlas, &chars, &s->Font, &prefix);
	s->PosAtlas.Tex.Y = y;

	y += s->PosAtlas.Tex.Height;
	TextWidget_Make(line2);
	Widget_SetLocation(line2, ANCHOR_MIN, ANCHOR_MIN, 2, y);
	line2->ReducePadding = true;

	if (Game_ClassicMode) {
		/* Swap around so 0.30 version is at top */
		line2->YOffset = 2;
		line1->YOffset = s->PosAtlas.Tex.Y;
		TextWidget_Set(line2, &version, &s->Font);

		Widget_Reposition(line1);
		Widget_Reposition(line2);
	} else {
		StatusScreen_UpdateHackState(s);
	}
}

static bool StatusScreen_KeyDown(void* elem, Key key, bool was) { return false; }
static bool StatusScreen_KeyUp(void* elem, Key key) { return false; }
static bool StatusScreen_KeyPress(void* elem, char keyChar) { return false; }
static bool StatusScreen_MouseScroll(void* elem, float delta) { return false; }

static void StatusScreen_Init(void* screen) {
	struct StatusScreen* s = screen;
	Drawer2D_MakeFont(&s->Font, 16, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);
}

static void StatusScreen_Render(void* screen, double delta) {
	struct StatusScreen* s = screen;
	StatusScreen_Update(s, delta);
	if (Game_HideGui) return;

	/* TODO: If Game_ShowFps is off and not classic mode, we should just return here */
	Gfx_SetTexturing(true);
	if (Gui_ShowFPS) Elem_Render(&s->Line1, delta);

	if (Game_ClassicMode) {
		Elem_Render(&s->Line2, delta);
	} else if (!Gui_Active && Gui_ShowFPS) {
		if (StatusScreen_HacksChanged(s)) { StatusScreen_UpdateHackState(s); }
		StatusScreen_DrawPosition(s);
		Elem_Render(&s->Line2, delta);
	}
	Gfx_SetTexturing(false);
}

static void StatusScreen_Free(void* screen) {
	struct StatusScreen* s = screen;
	Font_Free(&s->Font);
	Screen_CommonFree(s);
}

static struct ScreenVTABLE StatusScreen_VTABLE = {
	StatusScreen_Init,    StatusScreen_Render, StatusScreen_Free,     Gui_DefaultRecreate,
	StatusScreen_KeyDown, StatusScreen_KeyUp,  StatusScreen_KeyPress,
	Screen_Mouse,         Screen_Mouse,        Screen_MouseMove,      StatusScreen_MouseScroll,
	StatusScreen_OnResize, StatusScreen_ContextLost, StatusScreen_ContextRecreated,
};
struct Screen* StatusScreen_MakeInstance(void) {
	struct StatusScreen* s = &StatusScreen_Instance;
	s->HandlesAllInput = false;

	s->VTABLE = &StatusScreen_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*------------------------------------------------------LoadingScreen------------------------------------------------------*
*#########################################################################################################################*/
static struct LoadingScreen LoadingScreen_Instance;
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

static void LoadingScreen_MapLoading(void* screen, float progress) {
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

static bool LoadingScreen_KeyDown(void* sceen, Key key, bool was) {
	if (key == KEY_TAB) return true;
	return Elem_HandlesKeyDown(Gui_HUD, key, was);
}

static bool LoadingScreen_KeyPress(void* scren, char keyChar) {
	return Elem_HandlesKeyPress(Gui_HUD, keyChar);
}

static bool LoadingScreen_KeyUp(void* screen, Key key) {
	if (key == KEY_TAB) return true;
	return Elem_HandlesKeyUp(Gui_HUD, key);
}

static bool LoadingScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	if (Gui_HUD->HandlesAllInput) { Elem_HandlesMouseDown(Gui_HUD, x, y, btn); }
	return true;
}

static bool LoadingScreen_MouseUp(void* screen, int x, int y, MouseButton btn) { return true; }
static bool LoadingScreen_MouseMove(void* screen, int x, int y) { return true; }
static bool LoadingScreen_MouseScroll(void* screen, float delta) { return true; }

static void LoadingScreen_UpdateBackgroundVB(VertexP3fT2fC4b* vertices, int count, int atlasIndex, bool* bound) {
	if (!(*bound)) {
		*bound = true;
		Gfx_BindTexture(Atlas1D_TexIds[atlasIndex]);
	}

	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FT2FC4B);
	/* TODO: Do we need to use a separate VB here? */
	Gfx_UpdateDynamicVb_IndexedTris(Models.Vb, vertices, count);
}

#define LOADING_TILE_SIZE 64
static void LoadingScreen_DrawBackground(void) {
	VertexP3fT2fC4b vertices[144];
	VertexP3fT2fC4b* ptr = vertices;
	PackedCol col = PACKEDCOL_CONST(64, 64, 64, 255);

	struct Texture tex;
	TextureLoc loc;
	int count  = 0, atlasIndex, y;
	bool bound = false;

	loc = Block_GetTex(BLOCK_DIRT, FACE_YMAX);
	tex.ID    = GFX_NULL;
	Tex_SetRect(tex, 0,0, Game_Width,LOADING_TILE_SIZE);
	tex.uv    = Atlas1D_TexRec(loc, 1, &atlasIndex);
	tex.uv.U2 = (float)Game_Width / LOADING_TILE_SIZE;
	
	for (y = 0; y < Game_Height; y += LOADING_TILE_SIZE) {
		tex.Y = y;
		Gfx_Make2DQuad(&tex, col, &ptr);
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
	Drawer2D_MakeFont(&s->Font, 16, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);

	Gfx_SetFog(false);
	Event_RegisterFloat(&WorldEvents.Loading, s, LoadingScreen_MapLoading);
}

#define PROG_BAR_WIDTH 200
#define PROG_BAR_HEIGHT 4
static void LoadingScreen_Render(void* screen, double delta) {
	struct LoadingScreen* s = screen;
	PackedCol backCol = PACKEDCOL_CONST(128, 128, 128, 255);
	PackedCol progCol = PACKEDCOL_CONST(128, 255, 128, 255);
	int progWidth;
	int x, y;

	Gfx_SetTexturing(true);
	LoadingScreen_DrawBackground();

	Elem_Render(&s->Title, delta);
	Elem_Render(&s->Message, delta);
	Gfx_SetTexturing(false);

	x = Gui_CalcPos(ANCHOR_CENTRE,  0, PROG_BAR_WIDTH,  Game_Width);
	y = Gui_CalcPos(ANCHOR_CENTRE, 34, PROG_BAR_HEIGHT, Game_Height);
	progWidth = (int)(PROG_BAR_WIDTH * s->Progress);

	Gfx_Draw2DFlat(x, y, PROG_BAR_WIDTH, PROG_BAR_HEIGHT, backCol);
	Gfx_Draw2DFlat(x, y, progWidth,      PROG_BAR_HEIGHT, progCol);
}

static void LoadingScreen_Free(void* screen) {
	struct LoadingScreen* s = screen;
	Font_Free(&s->Font);
	Screen_CommonFree(s);
	Event_UnregisterFloat(&WorldEvents.Loading, s, LoadingScreen_MapLoading);
}

static struct ScreenVTABLE LoadingScreen_VTABLE = {
	LoadingScreen_Init,      LoadingScreen_Render,  LoadingScreen_Free,      Gui_DefaultRecreate,
	LoadingScreen_KeyDown,   LoadingScreen_KeyUp,   LoadingScreen_KeyPress,
	LoadingScreen_MouseDown, LoadingScreen_MouseUp, LoadingScreen_MouseMove, LoadingScreen_MouseScroll,
	LoadingScreen_OnResize,  LoadingScreen_ContextLost, LoadingScreen_ContextRecreated,
};
struct Screen* LoadingScreen_MakeInstance(const String* title, const String* message) {
	struct LoadingScreen* s = &LoadingScreen_Instance;
	s->LastState = NULL;
	s->VTABLE    = &LoadingScreen_VTABLE;
	s->Progress  = 0.0f;

	String_InitArray(s->TitleStr, s->__TitleBuffer);
	String_AppendString(&s->TitleStr, title);
	String_InitArray(s->MessageStr, s->__MessageBuffer);
	String_AppendString(&s->MessageStr, message);
	
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
	Event_RaiseVoid(&WorldEvents.NewMap);
	Gen_Done = false;
	LoadingScreen_Init(screen);

	if (Gen_Vanilla) {
		Thread_Start(&NotchyGen_Generate, true);
	} else {
		Thread_Start(&FlatgrassGen_Generate, true);
	}
}

static void GeneratingScreen_EndGeneration(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct LocationUpdate update;
	float x, z;

	Gui_CloseActive();
	Gen_Done = false;

	if (!Gen_Blocks) {
		Chat_AddRaw("&cFailed to generate the map."); return;
	}

	World_BlocksSize = Gen_Width * Gen_Height * Gen_Length;
	World_SetNewMap(Gen_Blocks, World_BlocksSize, Gen_Width, Gen_Height, Gen_Length);
	Gen_Blocks = NULL;

	x = (World_Width / 2) + 0.5f; z = (World_Length / 2) + 0.5f;
	p->Spawn = Respawn_FindSpawnPosition(x, z, p->Base.Size);

	LocationUpdate_MakePosAndOri(&update, p->Spawn, 0.0f, 0.0f, false);
	p->Base.VTABLE->SetLocation(&p->Base, &update, false);

	Camera.CurrentPos = Camera.Active->GetPosition(0.0f);
	Event_RaiseVoid(&WorldEvents.MapLoaded);
}

static void GeneratingScreen_Render(void* screen, double delta) {
	struct LoadingScreen* s = screen;
	const volatile char* state;

	LoadingScreen_Render(s, delta);
	if (Gen_Done) { GeneratingScreen_EndGeneration(); return; }

	state       = Gen_CurrentState;
	s->Progress = Gen_CurrentProgress;
	if (state == s->LastState) return;

	s->MessageStr.length = 0;
	String_AppendConst(&s->MessageStr, state);
	LoadingScreen_SetMessage(s);
}

static struct ScreenVTABLE GeneratingScreen_VTABLE = {
	GeneratingScreen_Init,   GeneratingScreen_Render, LoadingScreen_Free,      Gui_DefaultRecreate,
	LoadingScreen_KeyDown,   LoadingScreen_KeyUp,     LoadingScreen_KeyPress,
	LoadingScreen_MouseDown, LoadingScreen_MouseUp,   LoadingScreen_MouseMove, LoadingScreen_MouseScroll,
	LoadingScreen_OnResize,  LoadingScreen_ContextLost, LoadingScreen_ContextRecreated,
};
struct Screen* GeneratingScreen_MakeInstance(void) {
	const static String title   = String_FromConst("Generating level");
	const static String message = String_FromConst("Generating..");

	struct Screen* s = LoadingScreen_MakeInstance(&title, &message);
	s->VTABLE = &GeneratingScreen_VTABLE;
	return s;
}


/*########################################################################################################################*
*--------------------------------------------------------ChatScreen-------------------------------------------------------*
*#########################################################################################################################*/
static struct ChatScreen ChatScreen_Instance;
/* needed for lost contexts, to restore chat typed in */
static char ChatScreen_InputBuffer[INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN];
static String ChatScreen_InputStr = String_FromArray(ChatScreen_InputBuffer);

static int ChatScreen_BottomOffset(void) { return ((struct HUDScreen*)Gui_HUD)->Hotbar.Height; }
static int ChatScreen_InputUsedHeight(struct ChatScreen* s) {
	if (s->AltText.Height == 0) {
		return s->Input.Base.Height + 20;
	} else {
		return (Game_Height - s->AltText.Y) + 5;
	}
}

static void ChatScreen_UpdateAltTextY(struct ChatScreen* s) {
	struct InputWidget* input = &s->Input.Base;
	int height = max(input->Height + input->YOffset, ChatScreen_BottomOffset());
	height += input->YOffset;

	s->AltText.Tex.Y = Game_Height - (height + s->AltText.Tex.Height);
	s->AltText.Y     = s->AltText.Tex.Y;
}

static void ChatScreen_SetHandlesAllInput(struct ChatScreen* s, bool handles) {
	s->HandlesAllInput       = handles;
	Gui_HUD->HandlesAllInput = handles;
	Gui_CalcCursorVisible();
}

static void ChatScreen_OpenInput(struct ChatScreen* s, const String* initialText) {
	s->SuppressNextPress = true;
	ChatScreen_SetHandlesAllInput(s, true);

	String_Copy(&s->Input.Base.Text, initialText);
	Elem_Recreate(&s->Input.Base);
}

static void ChatScreen_ResetChat(struct ChatScreen* s) {
	String msg;
	int i;
	Elem_TryFree(&s->Chat);

	for (i = s->ChatIndex; i < s->ChatIndex + Gui_Chatlines; i++) {
		if (i >= 0 && i < Chat_Log.Count) {
			msg = StringsBuffer_UNSAFE_Get(&Chat_Log, i);
			TextGroupWidget_PushUpAndReplaceLast(&s->Chat, &msg);
		}
	}
}

static void ChatScreen_ConstructWidgets(struct ChatScreen* s) {
#define ChatScreen_MakeGroup(widget, lines, textures, buffer) TextGroupWidget_Create(widget, lines, &s->ChatFont, textures, buffer);
	int yOffset = ChatScreen_BottomOffset() + 15;

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

	ChatScreen_MakeGroup(&s->Chat, Gui_Chatlines, s->Chat_Textures, s->Chat_Buffer);
	Widget_SetLocation(&s->Chat, ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Elem_Init(&s->Chat);

	ChatScreen_MakeGroup(&s->ClientStatus, CHATSCREEN_MAX_GROUP, s->ClientStatus_Textures, s->ClientStatus_Buffer);
	Widget_SetLocation(&s->ClientStatus, ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Elem_Init(&s->ClientStatus);

	TextWidget_Create(&s->Announcement, &String_Empty, &s->AnnouncementFont);
	Widget_SetLocation(&s->Announcement, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -Game_Height / 4);
}

static void ChatScreen_SetInitialMessages(struct ChatScreen* s) {
	int i;

	s->ChatIndex = Chat_Log.Count - Gui_Chatlines;
	ChatScreen_ResetChat(s);

	TextGroupWidget_SetText(&s->Status, 2, &Chat_Status[0]);
	TextGroupWidget_SetText(&s->Status, 3, &Chat_Status[1]);
	TextGroupWidget_SetText(&s->Status, 4, &Chat_Status[2]);

	TextGroupWidget_SetText(&s->BottomRight, 2, &Chat_BottomRight[0]);
	TextGroupWidget_SetText(&s->BottomRight, 1, &Chat_BottomRight[1]);
	TextGroupWidget_SetText(&s->BottomRight, 0, &Chat_BottomRight[2]);

	TextWidget_Set(&s->Announcement, &Chat_Announcement, &s->AnnouncementFont);
	
	for (i = 0; i < s->ClientStatus.LinesCount; i++) {
		TextGroupWidget_SetText(&s->ClientStatus, i, &Chat_ClientStatus[i]);
	}

	if (s->HandlesAllInput) {
		ChatScreen_OpenInput(s, &ChatScreen_InputStr);
	}
}

static void ChatScreen_CheckOtherStatuses(struct ChatScreen* s) {
	const static String texPack = String_FromConst("texturePack");
	String str; char strBuffer[STRING_SIZE];
	struct HttpRequest request;
	int progress;
	bool hasRequest;
	String identifier;
	
	hasRequest = Http_GetCurrent(&request, &progress);
	identifier = String_FromRawArray(request.ID);	

	/* Is terrain / texture pack currently being downloaded? */
	if (!hasRequest || !String_Equals(&identifier, &texPack)) {
		if (s->Status.Textures[1].ID) {
			TextGroupWidget_SetText(&s->Status, 1, &String_Empty);
		}
		s->LastDownloadStatus = Int32_MinValue;
		return;
	}

	if (progress == s->LastDownloadStatus) return;
	s->LastDownloadStatus = progress;
	String_InitArray(str, strBuffer);

	if (progress == ASYNC_PROGRESS_MAKING_REQUEST) {
		String_AppendConst(&str, "&eRetrieving texture pack..");
	} else if (progress == ASYNC_PROGRESS_FETCHING_DATA) {
		String_AppendConst(&str, "&eDownloading texture pack");
	} else if (progress >= 0 && progress <= 100) {
		String_AppendConst(&str, "&eDownloading texture pack (&7");
		String_AppendInt(&str, progress);
		String_AppendConst(&str, "&e%)");
	}
	TextGroupWidget_SetText(&s->Status, 1, &str);
}

static void ChatScreen_RenderBackground(struct ChatScreen* s) {
	int usedHeight = TextGroupWidget_UsedHeight(&s->Chat);
	int x = s->Chat.X;
	int y = s->Chat.Y + s->Chat.Height - usedHeight;

	int width  = max(s->ClientStatus.Width, s->Chat.Width);
	int height = usedHeight + TextGroupWidget_UsedHeight(&s->ClientStatus);

	if (height > 0) {
		PackedCol backCol = PACKEDCOL_CONST(0, 0, 0, 127);
		Gfx_Draw2DFlat(x - 5, y - 5, width + 10, height + 10, backCol);
	}
}

static void ChatScreen_UpdateChatYOffset(struct ChatScreen* s, bool force) {
	int height = ChatScreen_InputUsedHeight(s);
	if (force || height != s->InputOldHeight) {
		int bottomOffset = ChatScreen_BottomOffset() + 15;
		s->ClientStatus.YOffset = max(bottomOffset, height);
		Widget_Reposition(&s->ClientStatus);

		s->Chat.YOffset = s->ClientStatus.YOffset + TextGroupWidget_UsedHeight(&s->ClientStatus);
		Widget_Reposition(&s->Chat);
		s->InputOldHeight = height;
	}
}

static void ChatElem_Recreate(struct TextGroupWidget* group, char code) {
	String line; char lineBuffer[TEXTGROUPWIDGET_LEN];
	int i, j;

	String_InitArray(line, lineBuffer);
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

static int ChatScreen_ClampIndex(int index) {
	int maxIndex = Chat_Log.Count - Gui_Chatlines;
	int minIndex = min(0, maxIndex);
	Math_Clamp(index, minIndex, maxIndex);
	return index;
}

static void ChatScreen_ScrollHistoryBy(struct ChatScreen* s, int delta) {
	int newIndex = ChatScreen_ClampIndex(s->ChatIndex + delta);
	if (newIndex == s->ChatIndex) return;

	s->ChatIndex = newIndex;
	ChatScreen_ResetChat(s);
}

static bool ChatScreen_KeyDown(void* screen, Key key, bool was) {
	const static String slash  = String_FromConst("/");
	struct ChatScreen* s = screen;
	struct InputWidget* input;
	int defaultIndex;

	s->SuppressNextPress = false;
	/* Handle text input bar */
	if (s->HandlesAllInput) {
		if (key == KeyBind_Get(KEYBIND_SEND_CHAT) || key == KEY_KP_ENTER || key == KEY_ESCAPE) {
			ChatScreen_SetHandlesAllInput(s, false);

			if (key == KEY_ESCAPE) InputWidget_Clear(&s->Input.Base);
			ChatScreen_InputStr.length = 0;

			input = &s->Input.Base;
			input->OnPressedEnter(input);
			SpecialInputWidget_SetActive(&s->AltText, false);

			/* Reset chat when user has scrolled up in chat history */
			defaultIndex = Chat_Log.Count - Gui_Chatlines;
			if (s->ChatIndex != defaultIndex) {
				s->ChatIndex = ChatScreen_ClampIndex(defaultIndex);
				ChatScreen_ResetChat(s);
			}
		} else if (key == KEY_PAGEUP) {
			ChatScreen_ScrollHistoryBy(s, -Gui_Chatlines);
		} else if (key == KEY_PAGEDOWN) {
			ChatScreen_ScrollHistoryBy(s, +Gui_Chatlines);
		} else {
			Elem_HandlesKeyDown(&s->Input.Base, key, was);
			ChatScreen_UpdateAltTextY(s);
		}
		return key < KEY_F1 || key > KEY_F35;
	}

	if (key == KeyBind_Get(KEYBIND_CHAT)) {
		ChatScreen_OpenInput(s, &String_Empty);
	} else if (key == KEY_SLASH) {
		ChatScreen_OpenInput(s, &slash);
	} else {
		return false;
	}
	return true;
}

static bool ChatScreen_KeyUp(void* screen, Key key) {
	struct ChatScreen* s = screen;
	if (!s->HandlesAllInput) return false;

	if (Server.SupportsFullCP437 && key == KeyBind_Get(KEYBIND_EXT_INPUT)) {
		if (!Window_Focused) return true;
		SpecialInputWidget_SetActive(&s->AltText, !s->AltText.Active);
	}
	return true;
}

static bool ChatScreen_KeyPress(void* screen, char keyChar) {
	struct ChatScreen* s = screen;
	bool handled;
	if (!s->HandlesAllInput) return false;

	if (s->SuppressNextPress) {
		s->SuppressNextPress = false;
		return false;
	}

	handled = Elem_HandlesKeyPress(&s->Input.Base, keyChar);
	ChatScreen_UpdateAltTextY(s);
	return handled;
}

static bool ChatScreen_MouseScroll(void* screen, float delta) {
	struct ChatScreen* s = screen;
	int steps;
	if (!s->HandlesAllInput) return false;

	steps = Utils_AccumulateWheelDelta(&s->ChatAcc, delta);
	ChatScreen_ScrollHistoryBy(s, -steps);
	return true;
}

static bool ChatScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	String text; char textBuffer[STRING_SIZE * 4];
	struct ChatScreen* s = screen;
	struct Screen* overlay;
	int height, chatY;
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

	height = TextGroupWidget_UsedHeight(&s->Chat);
	chatY  = s->Chat.Y + s->Chat.Height - height;
	if (!Gui_Contains(s->Chat.X, chatY, s->Chat.Width, height, x, y)) return false;

	String_InitArray(text, textBuffer);
	TextGroupWidget_GetSelected(&s->Chat, &text, x, y);
	if (!text.length) return false;

	if (Utils_IsUrlPrefix(&text, 0)) {
		overlay = UrlWarningOverlay_MakeInstance(&text);
		Gui_ShowOverlay(overlay, false);
	} else if (Gui_ClickableChat) {
		InputWidget_AppendString(&s->Input.Base, &text);
	}
	return true;
}

static void ChatScreen_ColCodeChanged(void* screen, int code) {
	struct ChatScreen* s = screen;
	double caretAcc;
	if (Gfx_LostContext) return;

	SpecialInputWidget_UpdateCols(&s->AltText);
	ChatElem_Recreate(&s->Chat, code);
	ChatElem_Recreate(&s->Status, code);
	ChatElem_Recreate(&s->BottomRight, code);
	ChatElem_Recreate(&s->ClientStatus, code);

	/* Some servers have plugins that redefine colours constantly */
	/* Preserve caret accumulator so caret blinking stays consistent */
	caretAcc = s->Input.Base.CaretAccumulator;
	Elem_Recreate(&s->Input.Base);
	s->Input.Base.CaretAccumulator = caretAcc;
}

static void ChatScreen_ChatReceived(void* screen, const String* msg, int type) {
	struct ChatScreen* s = screen;
	String chatMsg;
	int i;
	if (Gfx_LostContext) return;

	if (type == MSG_TYPE_NORMAL) {
		s->ChatIndex++;
		if (!Gui_Chatlines) return;

		chatMsg = *msg;
		i = s->ChatIndex + (Gui_Chatlines - 1);

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
		String_Copy(&ChatScreen_InputStr, &s->Input.Base.Text);
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
	int fontSize, largeSize;

	fontSize  = (int)(8  * Game_GetChatScale());
	Math_Clamp(fontSize, 8, 60);
	largeSize = (int)(16 * Game_GetChatScale());
	Math_Clamp(largeSize, 8, 60);

	Drawer2D_MakeFont(&s->ChatFont,         fontSize,  FONT_STYLE_NORMAL);
	Drawer2D_MakeFont(&s->AnnouncementFont, largeSize, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);

	Event_RegisterChat(&ChatEvents.ChatReceived,  s, ChatScreen_ChatReceived);
	Event_RegisterInt(&ChatEvents.ColCodeChanged, s, ChatScreen_ColCodeChanged);
}

static void ChatScreen_Render(void* screen, double delta) {
	struct ChatScreen* s = screen;
	struct Texture tex;
	TimeMS now;
	int i, y, logIdx;

	ChatScreen_CheckOtherStatuses(s);
	if (!Game_PureClassic) { Elem_Render(&s->Status, delta); }
	Elem_Render(&s->BottomRight, delta);

	ChatScreen_UpdateChatYOffset(s, false);
	y = s->ClientStatus.Y + s->ClientStatus.Height;
	for (i = 0; i < s->ClientStatus.LinesCount; i++) {
		tex = s->ClientStatus.Textures[i];
		if (!tex.ID) continue;

		y -= tex.Height; tex.Y = y;
		Texture_Render(&tex);
	}

	now = DateTime_CurrentUTC_MS();
	if (s->HandlesAllInput) {
		Elem_Render(&s->Chat, delta);
	} else {
		/* Only render recent chat */
		for (i = 0; i < s->Chat.LinesCount; i++) {
			tex    = s->Chat.Textures[i];
			logIdx = s->ChatIndex + i;
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

	if (s->Announcement.Texture.ID && now > Chat_AnnouncementReceived + (5 * 1000)) {
		Elem_TryFree(&s->Announcement);
	}
}

static void ChatScreen_Free(void* screen) {
	struct ChatScreen* s = screen;
	Font_Free(&s->ChatFont);
	Font_Free(&s->AnnouncementFont);
	Screen_CommonFree(s);

	Event_UnregisterChat(&ChatEvents.ChatReceived,  s, ChatScreen_ChatReceived);
	Event_UnregisterInt(&ChatEvents.ColCodeChanged, s, ChatScreen_ColCodeChanged);
}

static struct ScreenVTABLE ChatScreen_VTABLE = {
	ChatScreen_Init,      ChatScreen_Render, ChatScreen_Free,     Gui_DefaultRecreate,
	ChatScreen_KeyDown,   ChatScreen_KeyUp,  ChatScreen_KeyPress,
	ChatScreen_MouseDown, Screen_Mouse,      Screen_MouseMove,    ChatScreen_MouseScroll,
	ChatScreen_OnResize,  ChatScreen_ContextLost, ChatScreen_ContextRecreated,
};
struct Screen* ChatScreen_MakeInstance(void) {
	struct ChatScreen* s = &ChatScreen_Instance;
	s->InputOldHeight     = -1;
	s->LastDownloadStatus = Int32_MinValue;

	s->VTABLE = &ChatScreen_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*--------------------------------------------------------HUDScreen--------------------------------------------------------*
*#########################################################################################################################*/
static struct HUDScreen HUDScreen_Instance;
#define CH_EXTENT 16

static void HUDScreen_DrawCrosshairs(void) {
	static struct Texture tex = { GFX_NULL, Tex_Rect(0,0,0,0), Tex_UV(0.0f,0.0f, 15/256.0f,15/256.0f) };
	int extent;
	if (!Gui_IconsTex) return;

	extent = (int)(CH_EXTENT * Game_Scale(Game_Height / 480.0f));
	tex.ID = Gui_IconsTex;
	tex.X  = (Game_Width  / 2) - extent;
	tex.Y  = (Game_Height / 2) - extent;

	tex.Width  = extent * 2;
	tex.Height = extent * 2;
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
	bool extended;

	Elem_TryFree(&s->Hotbar);
	Elem_Init(&s->Hotbar);

	if (!s->WasShowingList) return;
	extended = Server.SupportsExtPlayerList && !Gui_ClassicTabList;
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

static bool HUDScreen_KeyDown(void* screen, Key key, bool was) {
	struct HUDScreen* s = screen;
	Key playerListKey = KeyBind_Get(KEYBIND_PLAYER_LIST);
	bool handles = playerListKey != KEY_TAB || !Gui_TabAutocomplete || !s->Chat->HandlesAllInput;

	if (key == playerListKey && handles) {
		if (!s->ShowingList && !Server.IsSinglePlayer) {
			s->WasShowingList = true;
			HUDScreen_ContextRecreated(s);
		}
		return true;
	}

	return Elem_HandlesKeyDown(s->Chat, key, was) || Elem_HandlesKeyDown(&s->Hotbar, key, was);
}

static bool HUDScreen_KeyUp(void* screen, Key key) {
	struct HUDScreen* s = screen;
	if (key == KeyBind_Get(KEYBIND_PLAYER_LIST) && s->ShowingList) {
		s->ShowingList    = false;
		s->WasShowingList = false;
		Elem_TryFree(&s->PlayerList);
		return true;
	}

	return Elem_HandlesKeyUp(s->Chat, key) || Elem_HandlesKeyUp(&s->Hotbar, key);
}

static bool HUDScreen_MouseScroll(void* screen, float delta) {
	struct HUDScreen* s = screen;
	return Elem_HandlesMouseScroll(s->Chat, delta);
}

static bool HUDScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	String name; char nameBuffer[STRING_SIZE + 1];
	struct HUDScreen* s = screen;
	struct ChatScreen* chat;	

	if (btn != MOUSE_LEFT || !s->HandlesAllInput) return false;
	chat = (struct ChatScreen*)s->Chat;
	if (!s->ShowingList) { return Elem_HandlesMouseDown(chat, x, y, btn); }

	String_InitArray(name, nameBuffer);
	PlayerListWidget_GetNameUnder(&s->PlayerList, x, y, &name);
	if (!name.length) { return Elem_HandlesMouseDown(chat, x, y, btn); }

	String_Append(&name, ' ');
	if (chat->HandlesAllInput) { InputWidget_AppendString(&chat->Input.Base, &name); }
	return true;
}

static void HUDScreen_Init(void* screen) {
	struct HUDScreen* s = screen;
	int size = Drawer2D_BitmappedText ? 16 : 11;
	Drawer2D_MakeFont(&s->PlayerFont, size, FONT_STYLE_NORMAL);

	ChatScreen_MakeInstance();
	s->Chat = (struct Screen*)&ChatScreen_Instance;
	Elem_Init(s->Chat);

	HotbarWidget_Create(&s->Hotbar);
	s->WasShowingList = false;
	Screen_CommonInit(s);
}

static void HUDScreen_Render(void* screen, double delta) {
	struct HUDScreen* s = screen;
	struct ChatScreen* chat = (struct ChatScreen*)s->Chat;
	bool showMinimal;

	if (Game_HideGui && chat->HandlesAllInput) {
		Gfx_SetTexturing(true);
		Elem_Render(&chat->Input.Base, delta);
		Gfx_SetTexturing(false);
	}
	if (Game_HideGui) return;
	showMinimal = Gui_GetActiveScreen()->BlocksWorld;

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
		if (!KeyBind_IsPressed(KEYBIND_PLAYER_LIST)) {
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

static struct ScreenVTABLE HUDScreen_VTABLE = {
	HUDScreen_Init,      HUDScreen_Render, HUDScreen_Free,     Gui_DefaultRecreate,
	HUDScreen_KeyDown,   HUDScreen_KeyUp,  HUDScreen_KeyPress,
	HUDScreen_MouseDown, Screen_Mouse,     Screen_MouseMove,   HUDScreen_MouseScroll,
	HUDScreen_OnResize,  HUDScreen_ContextLost, HUDScreen_ContextRecreated,
};
struct Screen* HUDScreen_MakeInstance(void) {
	struct HUDScreen* s = &HUDScreen_Instance;
	s->WasShowingList = false;
	s->VTABLE = &HUDScreen_VTABLE;
	return (struct Screen*)s;
}

void HUDScreen_OpenInput(struct Screen* hud, const String* text) {
	struct Screen* chat = ((struct HUDScreen*)hud)->Chat;
	ChatScreen_OpenInput((struct ChatScreen*)chat, text);
}

void HUDScreen_AppendInput(struct Screen* hud, const String* text) {
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
static struct DisconnectScreen DisconnectScreen_Instance;
#define DISCONNECT_DELAY_MS 5000
static void DisconnectScreen_ReconnectMessage(struct DisconnectScreen* s, String* msg) {
	if (s->CanReconnect) {
		int elapsedMS = (int)(DateTime_CurrentUTC_MS() - s->InitTime);
		int secsLeft = (DISCONNECT_DELAY_MS - elapsedMS) / MILLIS_PER_SEC;

		if (secsLeft > 0) {
			String_Format1(msg, "Reconnect in %i", &secsLeft); return;
		}
	}
	String_AppendConst(msg, "Reconnect");
}

static void DisconnectScreen_UpdateDelayLeft(struct DisconnectScreen* s, double delta) {
	String msg; char msgBuffer[STRING_SIZE];
	int elapsedMS, secsLeft;

	elapsedMS = (int)(DateTime_CurrentUTC_MS() - s->InitTime);
	secsLeft  = (DISCONNECT_DELAY_MS - elapsedMS) / MILLIS_PER_SEC;
	if (secsLeft < 0) secsLeft = 0;
	if (s->LastSecsLeft == secsLeft && s->Reconnect.Active == s->LastActive) return;

	String_InitArray(msg, msgBuffer);
	DisconnectScreen_ReconnectMessage(s, &msg);
	ButtonWidget_Set(&s->Reconnect, &msg, &s->TitleFont);

	s->Reconnect.Disabled = secsLeft != 0;
	s->LastSecsLeft = secsLeft;
	s->LastActive   = s->Reconnect.Active;
}

static void DisconnectScreen_ContextLost(void* screen) {
	struct DisconnectScreen* s = screen;
	Elem_TryFree(&s->Title);
	Elem_TryFree(&s->Message);
	Elem_TryFree(&s->Reconnect);
}

static void DisconnectScreen_ContextRecreated(void* screen) {
	String msg; char msgBuffer[STRING_SIZE];
	struct DisconnectScreen* s = screen;

	TextWidget_Create(&s->Title, &s->TitleStr, &s->TitleFont);
	Widget_SetLocation(&s->Title, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);

	TextWidget_Create(&s->Message, &s->MessageStr, &s->MessageFont);
	Widget_SetLocation(&s->Message, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 10);

	String_InitArray(msg, msgBuffer);
	DisconnectScreen_ReconnectMessage(s, &msg);

	ButtonWidget_Create(&s->Reconnect, 300, &msg, &s->TitleFont, NULL);
	Widget_SetLocation(&s->Reconnect, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 80);
	s->Reconnect.Disabled = !s->CanReconnect;
}

static void DisconnectScreen_Init(void* screen) {
	struct DisconnectScreen* s = screen;
	Drawer2D_MakeFont(&s->TitleFont,   16, FONT_STYLE_BOLD);
	Drawer2D_MakeFont(&s->MessageFont, 16, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);

	/* NOTE: changing VSync can't be done within frame, causes crash on some GPUs */
	game_limitMs = 1000 / 5.0f;

	s->InitTime     = DateTime_CurrentUTC_MS();
	s->LastSecsLeft = DISCONNECT_DELAY_MS / MILLIS_PER_SEC;
}

static void DisconnectScreen_Render(void* screen, double delta) {
	struct DisconnectScreen* s = screen;
	PackedCol top    = PACKEDCOL_CONST(64, 32, 32, 255);
	PackedCol bottom = PACKEDCOL_CONST(80, 16, 16, 255);

	if (s->CanReconnect) { DisconnectScreen_UpdateDelayLeft(s, delta); }
	Gfx_Draw2DGradient(0, 0, Game_Width, Game_Height, top, bottom);

	Gfx_SetTexturing(true);
	Elem_Render(&s->Title, delta);
	Elem_Render(&s->Message, delta);

	if (s->CanReconnect) { Elem_Render(&s->Reconnect, delta); }
	Gfx_SetTexturing(false);
}

static void DisconnectScreen_Free(void* screen) {
	struct DisconnectScreen* s = screen;
	Font_Free(&s->TitleFont);
	Font_Free(&s->MessageFont);
	Screen_CommonFree(s);

	game_limitMs = Game_CalcLimitMillis(Game_FpsLimit);
}

static void DisconnectScreen_OnResize(void* screen) {
	struct DisconnectScreen* s = screen;
	Widget_Reposition(&s->Title);
	Widget_Reposition(&s->Message);
	Widget_Reposition(&s->Reconnect);
}

static bool DisconnectScreen_KeyDown(void* s, Key key, bool was) { return key < KEY_F1 || key > KEY_F35; }
static bool DisconnectScreen_KeyPress(void* s, char keyChar) { return true; }
static bool DisconnectScreen_KeyUp(void* s, Key key) { return true; }

static bool DisconnectScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	String title; char titleBuffer[STRING_SIZE];
	struct DisconnectScreen* s = screen;
	struct ButtonWidget* w = &s->Reconnect;

	if (btn != MOUSE_LEFT) return true;
	if (!w->Disabled && Widget_Contains(w, x, y)) {
		String_InitArray(title, titleBuffer);
		String_Format2(&title, "Connecting to %s:%i..", &Game_IPAddress, &Game_Port);

		Gui_FreeActive();
		Gui_SetActive(LoadingScreen_MakeInstance(&title, &String_Empty));
		Server.BeginConnect();
	}
	return true;
}

static bool DisconnectScreen_MouseMove(void* screen, int x, int y) {
	struct DisconnectScreen* s = screen;
	struct ButtonWidget* w     = &s->Reconnect;

	w->Active = !w->Disabled && Widget_Contains(w, x, y);
	return true;
}

static bool DisconnectScreen_MouseScroll(void* screen, float delta) { return true; }
static bool DisconnectScreen_MouseUp(void* screen, int x, int y, MouseButton btn) { return true; }

static struct ScreenVTABLE DisconnectScreen_VTABLE = {
	DisconnectScreen_Init,      DisconnectScreen_Render,  DisconnectScreen_Free,      Gui_DefaultRecreate,
	DisconnectScreen_KeyDown,   DisconnectScreen_KeyUp,   DisconnectScreen_KeyPress,
	DisconnectScreen_MouseDown, DisconnectScreen_MouseUp, DisconnectScreen_MouseMove, DisconnectScreen_MouseScroll,
	DisconnectScreen_OnResize,  DisconnectScreen_ContextLost, DisconnectScreen_ContextRecreated
};
struct Screen* DisconnectScreen_MakeInstance(const String* title, const String* message) {
	const static String kick = String_FromConst("Kicked ");
	const static String ban  = String_FromConst("Banned ");
	String why; char whyBuffer[STRING_SIZE];
	struct DisconnectScreen* s = &DisconnectScreen_Instance;

	s->HandlesAllInput = true;
	s->BlocksWorld     = true;
	s->HidesHUD        = true;

	String_InitArray(s->TitleStr, s->__TitleBuffer);
	String_AppendString(&s->TitleStr, title);
	String_InitArray(s->MessageStr, s->__MessageBuffer);
	String_AppendString(&s->MessageStr, message);

	String_InitArray(why, whyBuffer);
	String_AppendColorless(&why, message);
	
	s->CanReconnect = !(String_CaselessStarts(&why, &kick) || String_CaselessStarts(&why, &ban));
	s->VTABLE = &DisconnectScreen_VTABLE;
	return (struct Screen*)s;
}
