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
#include "Generator.h"
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
	FontDesc font;
	struct TableWidget table;
	bool releasedInv, deferredSelect;
};

struct StatusScreen {
	Screen_Layout
	FontDesc font;
	struct TextWidget line1, line2;
	struct TextAtlas posAtlas;
	double accumulator;
	int frames, fps;
	bool speed, halfSpeed, noclip, fly, canSpeed;
	int lastFov;
};

struct HUDScreen {
	Screen_Layout
	struct Screen* chat;
	struct HotbarWidget hotbar;
	struct PlayerListWidget playerList;
	FontDesc playerFont;
	bool showingList, wasShowingList;
};

struct LoadingScreen {
	Screen_Layout
	FontDesc font;
	float progress;
	
	struct TextWidget title, message;
	String titleStr, messageStr;
	const char* lastState;

	char _titleBuffer[STRING_SIZE];
	char _messageBuffer[STRING_SIZE];
};

#define CHAT_MAX_STATUS Array_Elems(Chat_Status)
#define CHAT_MAX_BOTTOMRIGHT Array_Elems(Chat_BottomRight)
#define CHAT_MAX_CLIENTSTATUS Array_Elems(Chat_ClientStatus)

struct ChatScreen {
	Screen_Layout
	int inputOldHeight;
	float chatAcc;
	bool suppressNextPress;
	int chatIndex;
	int lastDownloadStatus;
	FontDesc chatFont, announcementFont;
	struct TextWidget announcement;
	struct ChatInputWidget input;
	struct TextGroupWidget status, bottomRight, chat, clientStatus;
	struct SpecialInputWidget altText;

	struct Texture statusTextures[CHAT_MAX_STATUS];
	struct Texture bottomRightTextures[CHAT_MAX_BOTTOMRIGHT];
	struct Texture clientStatusTextures[CHAT_MAX_CLIENTSTATUS];
	struct Texture chatTextures[TEXTGROUPWIDGET_MAX_LINES];
};

struct DisconnectScreen {
	Screen_Layout
	TimeMS initTime;
	bool canReconnect, lastActive;
	int lastSecsLeft;
	struct ButtonWidget reconnect;

	FontDesc titleFont, messageFont;
	struct TextWidget title, message;
	char _titleBuffer[STRING_SIZE];
	char _messageBuffer[STRING_SIZE];
	String titleStr, messageStr;
};

static bool Screen_Mouse(void* elem, int x, int y, MouseButton btn) { return false; }
static bool Screen_MouseMove(void* elem, int x, int y) { return false; }


/*########################################################################################################################*
*-----------------------------------------------------InventoryScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct InventoryScreen InventoryScreen_Instance;
static void InventoryScreen_OnBlockChanged(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	TableWidget_OnInventoryChanged(&s->table);
}

static void InventoryScreen_ContextLost(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Elem_TryFree(&s->table);
}

static void InventoryScreen_ContextRecreated(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Elem_Recreate(&s->table);
}

static void InventoryScreen_MoveToSelected(struct InventoryScreen* s) {
	struct TableWidget* table = &s->table;
	TableWidget_SetBlockTo(table, Inventory_SelectedBlock);
	Elem_Recreate(table);

	s->deferredSelect = false;
	/* User is holding invalid block */
	if (table->selectedIndex == -1) {
		TableWidget_MakeDescTex(table, Inventory_SelectedBlock);
	}
}

static void InventoryScreen_Init(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Drawer2D_MakeFont(&s->font, 16, FONT_STYLE_NORMAL);

	TableWidget_Create(&s->table);
	s->table.font = s->font;
	s->table.elementsPerRow = Game_PureClassic ? 9 : 10;
	Elem_Init(&s->table);

	/* Can't immediately move to selected here, because cursor visibility 
	   might be toggled after Init() is called. This causes the cursor to 
	   be moved back to the middle of the window. */
	s->deferredSelect = true;
	Screen_CommonInit(s);

	Event_RegisterVoid(&BlockEvents.PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_RegisterVoid(&BlockEvents.BlockDefChanged,    s, InventoryScreen_OnBlockChanged);
}

static void InventoryScreen_Render(void* screen, double delta) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	if (s->deferredSelect) InventoryScreen_MoveToSelected(s);
	Elem_Render(&s->table, delta);
}

static void InventoryScreen_OnResize(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Widget_Reposition(&s->table);
}

static void InventoryScreen_Free(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Font_Free(&s->font);
	Screen_CommonFree(s);
	
	Event_UnregisterVoid(&BlockEvents.PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&BlockEvents.BlockDefChanged,    s, InventoryScreen_OnBlockChanged);
}

static bool InventoryScreen_KeyDown(void* screen, Key key, bool was) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct TableWidget* table = &s->table;

	if (key == KeyBinds[KEYBIND_INVENTORY] && s->releasedInv) {
		Gui_Close(screen);
	} else if (key == KEY_ENTER && table->selectedIndex != -1) {
		Inventory_SetSelectedBlock(table->elements[table->selectedIndex]);
		Gui_Close(screen);
	} else if (Elem_HandlesKeyDown(table, key, was)) {
	} else {
		struct HUDScreen* hud = (struct HUDScreen*)Gui_HUD;
		return Elem_HandlesKeyDown(&hud->hotbar, key, was);
	}
	return true;
}

static bool InventoryScreen_KeyPress(void* elem, char keyChar) { return true; }
static bool InventoryScreen_KeyUp(void* screen, Key key) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct HUDScreen* hud;

	if (key == KeyBinds[KEYBIND_INVENTORY]) {
		s->releasedInv = true; return true;
	}

	hud = (struct HUDScreen*)Gui_HUD;
	return Elem_HandlesKeyUp(&hud->hotbar, key);
}

static bool InventoryScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct TableWidget* table = &s->table;
	struct HUDScreen* hud     = (struct HUDScreen*)Gui_HUD;
	bool handled, hotbar;

	if (table->scroll.draggingMouse || Elem_HandlesMouseDown(&hud->hotbar, x, y, btn)) return true;
	handled = Elem_HandlesMouseDown(table, x, y, btn);

	if ((!handled || table->pendingClose) && btn == MOUSE_LEFT) {
		hotbar = Key_IsControlPressed() || Key_IsShiftPressed();
		if (!hotbar) Gui_Close(screen);
	}
	return true;
}

static bool InventoryScreen_MouseUp(void* screen, int x, int y, MouseButton btn) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct TableWidget* table = &s->table;
	return Elem_HandlesMouseUp(table, x, y, btn);
}

static bool InventoryScreen_MouseMove(void* screen, int x, int y) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct TableWidget* table = &s->table;
	return Elem_HandlesMouseMove(table, x, y);
}

static bool InventoryScreen_MouseScroll(void* screen, float delta) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct TableWidget* table = &s->table;

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
	s->handlesAllInput = true;
	s->closable        = true;

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
	s->fps = (int)(s->frames / s->accumulator);
	String_Format1(status, "%i fps, ", &s->fps);

	if (Game_ClassicMode) {
		String_Format1(status, "%i chunk updates", &Game.ChunkUpdates);
	} else {
		if (Game.ChunkUpdates) {
			String_Format1(status, "%i chunks/s, ", &Game.ChunkUpdates);
		}

		indices = ICOUNT(Game_Vertices);
		String_Format1(status, "%i vertices", &indices);

		ping = Ping_AveragePingMS();
		if (ping) String_Format1(status, ", ping %i ms", &ping);
	}
}

static void StatusScreen_DrawPosition(struct StatusScreen* s) {
	VertexP3fT2fC4b vertices[4 * 64];
	VertexP3fT2fC4b* ptr = vertices;
	PackedCol col = PACKEDCOL_WHITE;

	struct TextAtlas* atlas = &s->posAtlas;
	struct Texture tex;
	IVec3 pos;
	int count;	

	/* Make "Position: " prefix */
	tex = atlas->tex; 
	tex.X = 2; tex.Width = atlas->offset;
	Gfx_Make2DQuad(&tex, col, &ptr);

	IVec3_Floor(&pos, &LocalPlayer_Instance.Base.Position);
	atlas->curX = atlas->offset + 2;

	/* Make (X, Y, Z) suffix */
	TextAtlas_Add(atlas, 13, &ptr);
	TextAtlas_AddInt(atlas, pos.X, &ptr);
	TextAtlas_Add(atlas, 11, &ptr);
	TextAtlas_AddInt(atlas, pos.Y, &ptr);
	TextAtlas_Add(atlas, 11, &ptr);
	TextAtlas_AddInt(atlas, pos.Z, &ptr);
	TextAtlas_Add(atlas, 14, &ptr);

	Gfx_BindTexture(atlas->tex.ID);
	/* TODO: Do we need to use a separate VB here? */
	count = (int)(ptr - vertices);
	Gfx_UpdateDynamicVb_IndexedTris(Models.Vb, vertices, count);
}

static bool StatusScreen_HacksChanged(struct StatusScreen* s) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	return hacks->Speeding != s->speed || hacks->HalfSpeeding != s->halfSpeed || hacks->Flying != s->fly
		|| hacks->Noclip != s->noclip  || Game_Fov != s->lastFov || hacks->CanSpeed != s->canSpeed;
}

static void StatusScreen_UpdateHackState(struct StatusScreen* s) {
	String status; char statusBuffer[STRING_SIZE * 2];
	struct HacksComp* hacks;
	bool speeding;

	hacks = &LocalPlayer_Instance.Hacks;
	s->speed = hacks->Speeding; s->halfSpeed = hacks->HalfSpeeding; s->fly = hacks->Flying;
	s->noclip = hacks->Noclip;  s->lastFov = Game_Fov; s->canSpeed = hacks->CanSpeed;

	String_InitArray(status, statusBuffer);
	if (Game_Fov != Game_DefaultFov) {
		String_Format1(&status, "Zoom fov %i  ", &Game_Fov);
	}
	speeding = (hacks->Speeding || hacks->HalfSpeeding) && hacks->CanSpeed;

	if (hacks->Flying) String_AppendConst(&status, "Fly ON   ");
	if (speeding)      String_AppendConst(&status, "Speed ON   ");
	if (hacks->Noclip) String_AppendConst(&status, "Noclip ON   ");

	TextWidget_Set(&s->line2, &status, &s->font);
}

static void StatusScreen_Update(struct StatusScreen* s, double delta) {
	String status; char statusBuffer[STRING_SIZE * 2];

	s->frames++;
	s->accumulator += delta;
	if (s->accumulator < 1.0) return;

	String_InitArray(status, statusBuffer);
	StatusScreen_MakeText(s, &status);

	TextWidget_Set(&s->line1, &status, &s->font);
	s->accumulator = 0.0;
	s->frames = 0;
	Game.ChunkUpdates = 0;
}

static void StatusScreen_OnResize(void* screen) { }
static void StatusScreen_ContextLost(void* screen) {
	struct StatusScreen* s = (struct StatusScreen*)screen;
	TextAtlas_Free(&s->posAtlas);
	Elem_TryFree(&s->line1);
	Elem_TryFree(&s->line2);
}

static void StatusScreen_ContextRecreated(void* screen) {	
	static const String chars   = String_FromConst("0123456789-, ()");
	static const String prefix  = String_FromConst("Position: ");
	static const String version = String_FromConst("0.30");

	struct StatusScreen* s   = (struct StatusScreen*)screen;
	struct TextWidget* line1 = &s->line1;
	struct TextWidget* line2 = &s->line2;
	int y;

	y = 2;
	TextWidget_Make(line1);
	Widget_SetLocation(line1, ANCHOR_MIN, ANCHOR_MIN, 2, y);
	line1->reducePadding = true;
	StatusScreen_Update(s, 1.0);

	y += line1->height;
	TextAtlas_Make(&s->posAtlas, &chars, &s->font, &prefix);
	s->posAtlas.tex.Y = y;

	y += s->posAtlas.tex.Height;
	TextWidget_Make(line2);
	Widget_SetLocation(line2, ANCHOR_MIN, ANCHOR_MIN, 2, y);
	line2->reducePadding = true;

	if (Game_ClassicMode) {
		/* Swap around so 0.30 version is at top */
		line2->yOffset = 2;
		line1->yOffset = s->posAtlas.tex.Y;
		TextWidget_Set(line2, &version, &s->font);

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
	struct StatusScreen* s = (struct StatusScreen*)screen;
	Drawer2D_MakeFont(&s->font, 16, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);
}

static void StatusScreen_Render(void* screen, double delta) {
	struct StatusScreen* s = (struct StatusScreen*)screen;
	StatusScreen_Update(s, delta);
	if (Game_HideGui) return;

	/* TODO: If Game_ShowFps is off and not classic mode, we should just return here */
	Gfx_SetTexturing(true);
	if (Gui_ShowFPS) Elem_Render(&s->line1, delta);

	if (Game_ClassicMode) {
		Elem_Render(&s->line2, delta);
	} else if (!Gui_Active && Gui_ShowFPS) {
		if (StatusScreen_HacksChanged(s)) { StatusScreen_UpdateHackState(s); }
		StatusScreen_DrawPosition(s);
		Elem_Render(&s->line2, delta);
	}
	Gfx_SetTexturing(false);
}

static void StatusScreen_Free(void* screen) {
	struct StatusScreen* s = (struct StatusScreen*)screen;
	Font_Free(&s->font);
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
	s->handlesAllInput = false;

	s->VTABLE = &StatusScreen_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*------------------------------------------------------LoadingScreen------------------------------------------------------*
*#########################################################################################################################*/
static struct LoadingScreen LoadingScreen_Instance;
static void LoadingScreen_SetTitle(struct LoadingScreen* s) {
	Elem_TryFree(&s->title);
	TextWidget_Create(&s->title, &s->titleStr, &s->font);
	Widget_SetLocation(&s->title, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -31);
}

static void LoadingScreen_SetMessage(struct LoadingScreen* s) {
	Elem_TryFree(&s->message);
	TextWidget_Create(&s->message, &s->messageStr, &s->font);
	Widget_SetLocation(&s->message, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 17);
}

static void LoadingScreen_MapLoading(void* screen, float progress) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	s->progress = progress;
}

static void LoadingScreen_OnResize(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Widget_Reposition(&s->title);
	Widget_Reposition(&s->message);
}

static void LoadingScreen_ContextLost(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Elem_TryFree(&s->title);
	Elem_TryFree(&s->message);
}

static void LoadingScreen_ContextRecreated(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
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
	if (Gui_HUD->handlesAllInput) { Elem_HandlesMouseDown(Gui_HUD, x, y, btn); }
	return true;
}

static bool LoadingScreen_MouseUp(void* screen, int x, int y, MouseButton btn) { return true; }
static bool LoadingScreen_MouseMove(void* screen, int x, int y) { return true; }
static bool LoadingScreen_MouseScroll(void* screen, float delta) { return true; }

static void LoadingScreen_UpdateBackgroundVB(VertexP3fT2fC4b* vertices, int count, int atlasIndex, bool* bound) {
	if (!(*bound)) {
		*bound = true;
		Gfx_BindTexture(Atlas1D.TexIds[atlasIndex]);
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

	loc = Block_Tex(BLOCK_DIRT, FACE_YMAX);
	tex.ID    = GFX_NULL;
	Tex_SetRect(tex, 0,0, Window_Width,LOADING_TILE_SIZE);
	tex.uv    = Atlas1D_TexRec(loc, 1, &atlasIndex);
	tex.uv.U2 = (float)Window_Width / LOADING_TILE_SIZE;
	
	for (y = 0; y < Window_Height; y += LOADING_TILE_SIZE) {
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
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Drawer2D_MakeFont(&s->font, 16, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);

	Gfx_SetFog(false);
	Event_RegisterFloat(&WorldEvents.Loading, s, LoadingScreen_MapLoading);
}

#define PROG_BAR_WIDTH 200
#define PROG_BAR_HEIGHT 4
static void LoadingScreen_Render(void* screen, double delta) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	PackedCol backCol = PACKEDCOL_CONST(128, 128, 128, 255);
	PackedCol progCol = PACKEDCOL_CONST(128, 255, 128, 255);
	int progWidth;
	int x, y;

	Gfx_SetTexturing(true);
	LoadingScreen_DrawBackground();

	Elem_Render(&s->title, delta);
	Elem_Render(&s->message, delta);
	Gfx_SetTexturing(false);

	x = Gui_CalcPos(ANCHOR_CENTRE,  0, PROG_BAR_WIDTH,  Window_Width);
	y = Gui_CalcPos(ANCHOR_CENTRE, 34, PROG_BAR_HEIGHT, Window_Height);
	progWidth = (int)(PROG_BAR_WIDTH * s->progress);

	Gfx_Draw2DFlat(x, y, PROG_BAR_WIDTH, PROG_BAR_HEIGHT, backCol);
	Gfx_Draw2DFlat(x, y, progWidth,      PROG_BAR_HEIGHT, progCol);
}

static void LoadingScreen_Free(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Font_Free(&s->font);
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
	s->lastState = NULL;
	s->VTABLE    = &LoadingScreen_VTABLE;
	s->progress  = 0.0f;

	String_InitArray(s->titleStr, s->_titleBuffer);
	String_AppendString(&s->titleStr, title);
	String_InitArray(s->messageStr, s->_messageBuffer);
	String_AppendString(&s->messageStr, message);
	
	s->handlesAllInput = true;
	s->blocksWorld     = true;
	s->renderHUDOver   = true;
	return (struct Screen*)s;
}
struct Screen* LoadingScreen_UNSAFE_RawPointer = (struct Screen*)&LoadingScreen_Instance;


/*########################################################################################################################*
*--------------------------------------------------GeneratingMapScreen----------------------------------------------------*
*#########################################################################################################################*/
static void GeneratingScreen_Init(void* screen) {
	Gen_Done = false;
	LoadingScreen_Init(screen);
	Event_RaiseVoid(&WorldEvents.NewMap);

	Gen_Blocks = (BlockRaw*)Mem_TryAlloc(World.Volume, 1);
	if (!Gen_Blocks) {
		Window_ShowDialog("Out of memory", "Not enough free memory to generate a map that large.\nTry a smaller size.");
		Gui_CloseActive();
	} else if (Gen_Vanilla) {
		Thread_Start(NotchyGen_Generate, true);
	} else {
		Thread_Start(FlatgrassGen_Generate, true);
	}
}

static void GeneratingScreen_EndGeneration(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct LocationUpdate update;
	float x, z;

	Gui_CloseActive();
	Gen_Done = false;

	if (!Gen_Blocks) { Chat_AddRaw("&cFailed to generate the map."); return; }
	World_SetNewMap(Gen_Blocks, World.Width, World.Height, World.Length);
	Gen_Blocks = NULL;

	x = (World.Width / 2) + 0.5f; z = (World.Length / 2) + 0.5f;
	p->Spawn = Respawn_FindSpawnPosition(x, z, p->Base.Size);

	LocationUpdate_MakePosAndOri(&update, p->Spawn, 0.0f, 0.0f, false);
	p->Base.VTABLE->SetLocation(&p->Base, &update, false);

	Camera.CurrentPos = Camera.Active->GetPosition(0.0f);
	Event_RaiseVoid(&WorldEvents.MapLoaded);
}

static void GeneratingScreen_Render(void* screen, double delta) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	const volatile char* state;

	LoadingScreen_Render(s, delta);
	if (Gen_Done) { GeneratingScreen_EndGeneration(); return; }

	state       = Gen_CurrentState;
	s->progress = Gen_CurrentProgress;
	if (state == s->lastState) return;

	s->messageStr.length = 0;
	String_AppendConst(&s->messageStr, (const char*)state);
	LoadingScreen_SetMessage(s);
}

static struct ScreenVTABLE GeneratingScreen_VTABLE = {
	GeneratingScreen_Init,   GeneratingScreen_Render, LoadingScreen_Free,      Gui_DefaultRecreate,
	LoadingScreen_KeyDown,   LoadingScreen_KeyUp,     LoadingScreen_KeyPress,
	LoadingScreen_MouseDown, LoadingScreen_MouseUp,   LoadingScreen_MouseMove, LoadingScreen_MouseScroll,
	LoadingScreen_OnResize,  LoadingScreen_ContextLost, LoadingScreen_ContextRecreated,
};
struct Screen* GeneratingScreen_MakeInstance(void) {
	static const String title   = String_FromConst("Generating level");
	static const String message = String_FromConst("Generating..");

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

static int ChatScreen_BottomOffset(void) { return ((struct HUDScreen*)Gui_HUD)->hotbar.height; }
static int ChatScreen_InputUsedHeight(struct ChatScreen* s) {
	if (s->altText.height == 0) {
		return s->input.base.height + 20;
	} else {
		return (Window_Height - s->altText.y) + 5;
	}
}

static void ChatScreen_UpdateAltTextY(struct ChatScreen* s) {
	struct InputWidget* input = &s->input.base;
	int height = max(input->height + input->yOffset, ChatScreen_BottomOffset());
	height += input->yOffset;

	s->altText.tex.Y = Window_Height - (height + s->altText.tex.Height);
	s->altText.y     = s->altText.tex.Y;
}

static void ChatScreen_SetHandlesAllInput(struct ChatScreen* s, bool handles) {
	s->handlesAllInput       = handles;
	Gui_HUD->handlesAllInput = handles;
	Camera_CheckFocus();
}

static void ChatScreen_OpenInput(struct ChatScreen* s, const String* initialText) {
	s->suppressNextPress = true;
	ChatScreen_SetHandlesAllInput(s, true);

	String_Copy(&s->input.base.text, initialText);
	Elem_Recreate(&s->input.base);
}

static String ChatScreen_GetChat(void* obj, int i) {
	i += *((int*)obj); /* argument is offset into chat */

	if (i >= 0 && i < Chat_Log.count) {
		return StringsBuffer_UNSAFE_Get(&Chat_Log, i);
	}
	return String_Empty;
}

static String ChatScreen_GetStatus(void* obj, int i)       { return Chat_Status[i]; }
static String ChatScreen_GetBottomRight(void* obj, int i)  { return Chat_BottomRight[2 - i]; }
static String ChatScreen_GetClientStatus(void* obj, int i) { return Chat_ClientStatus[i]; }

static void ChatScreen_ConstructWidgets(struct ChatScreen* s) {
	int yOffset = ChatScreen_BottomOffset() + 15;
	ChatInputWidget_Create(&s->input, &s->chatFont);
	Widget_SetLocation(&s->input.base, ANCHOR_MIN, ANCHOR_MAX, 5, 5);

	SpecialInputWidget_Create(&s->altText, &s->chatFont, &s->input.base);
	Elem_Init(&s->altText);
	ChatScreen_UpdateAltTextY(s);

	TextGroupWidget_Create(&s->status, CHAT_MAX_STATUS, &s->chatFont, 
							s->statusTextures, ChatScreen_GetStatus);
	Widget_SetLocation(&s->status, ANCHOR_MAX, ANCHOR_MIN, 0, 0);
	Elem_Init(&s->status);
	TextGroupWidget_SetUsePlaceHolder(&s->status, 0, false);

	TextGroupWidget_Create(&s->bottomRight, CHAT_MAX_BOTTOMRIGHT, &s->chatFont, 
							s->bottomRightTextures, ChatScreen_GetBottomRight);
	Widget_SetLocation(&s->bottomRight, ANCHOR_MAX, ANCHOR_MAX, 0, yOffset);
	Elem_Init(&s->bottomRight);

	TextGroupWidget_Create(&s->chat, Gui_Chatlines, &s->chatFont, 
							s->chatTextures, ChatScreen_GetChat);
	s->chat.underlineUrls = !Game_ClassicMode;
	s->chat.getLineObj    = &s->chatIndex;
	Widget_SetLocation(&s->chat, ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Elem_Init(&s->chat);

	TextGroupWidget_Create(&s->clientStatus, CHAT_MAX_CLIENTSTATUS, &s->chatFont,
							s->clientStatusTextures, ChatScreen_GetClientStatus);
	Widget_SetLocation(&s->clientStatus, ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Elem_Init(&s->clientStatus);

	TextWidget_Create(&s->announcement, &String_Empty, &s->announcementFont);
	Widget_SetLocation(&s->announcement, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -Window_Height / 4);
}

static void ChatScreen_SetInitialMessages(struct ChatScreen* s) {
	s->chatIndex = Chat_Log.count - Gui_Chatlines;
	TextGroupWidget_RedrawAll(&s->chat);
	TextWidget_Set(&s->announcement, &Chat_Announcement, &s->announcementFont);

	TextGroupWidget_RedrawAll(&s->status);
	TextGroupWidget_RedrawAll(&s->bottomRight);
	TextGroupWidget_RedrawAll(&s->clientStatus);

	if (s->handlesAllInput) {
		ChatScreen_OpenInput(s, &ChatScreen_InputStr);
	}
}

static void ChatScreen_CheckOtherStatuses(struct ChatScreen* s) {
	static const String texPack = String_FromConst("texturePack");
	struct HttpRequest request;
	int progress;
	bool hasRequest;
	String identifier;
	
	hasRequest = Http_GetCurrent(&request, &progress);
	identifier = String_FromRawArray(request.ID);	

	/* Is terrain/texture pack currently being downloaded? */
	if (!hasRequest || !String_Equals(&identifier, &texPack)) {
		if (s->status.textures[0].ID) {
			Chat_Status[0].length = 0;
			TextGroupWidget_Redraw(&s->status, 0);
		}
		s->lastDownloadStatus = Int32_MinValue;
		return;
	}

	if (progress == s->lastDownloadStatus) return;
	s->lastDownloadStatus = progress;
	Chat_Status[0].length = 0;

	if (progress == ASYNC_PROGRESS_MAKING_REQUEST) {
		String_AppendConst(&Chat_Status[0], "&eRetrieving texture pack..");
	} else if (progress == ASYNC_PROGRESS_FETCHING_DATA) {
		String_AppendConst(&Chat_Status[0], "&eDownloading texture pack");
	} else if (progress >= 0 && progress <= 100) {
		String_Format1(&Chat_Status[0], "&eDownloading texture pack (&7%i&e%%)", &progress);
	}
	TextGroupWidget_Redraw(&s->status, 0);
}

static void ChatScreen_RenderBackground(struct ChatScreen* s) {
	int usedHeight = TextGroupWidget_UsedHeight(&s->chat);
	int x = s->chat.x;
	int y = s->chat.y + s->chat.height - usedHeight;

	int width  = max(s->clientStatus.width, s->chat.width);
	int height = usedHeight + TextGroupWidget_UsedHeight(&s->clientStatus);

	if (height > 0) {
		PackedCol backCol = PACKEDCOL_CONST(0, 0, 0, 127);
		Gfx_Draw2DFlat(x - 5, y - 5, width + 10, height + 10, backCol);
	}
}

static void ChatScreen_UpdateChatYOffset(struct ChatScreen* s, bool force) {
	int height = ChatScreen_InputUsedHeight(s);
	if (force || height != s->inputOldHeight) {
		int bottomOffset = ChatScreen_BottomOffset() + 15;
		s->clientStatus.yOffset = max(bottomOffset, height);
		Widget_Reposition(&s->clientStatus);

		s->chat.yOffset = s->clientStatus.yOffset + TextGroupWidget_UsedHeight(&s->clientStatus);
		Widget_Reposition(&s->chat);
		s->inputOldHeight = height;
	}
}

static void ChatElem_Recreate(struct TextGroupWidget* group, char code) {
	String line;
	int i, j;

	for (i = 0; i < group->lines; i++) {
		line = TextGroupWidget_UNSAFE_Get(group, i);
		if (!line.length) continue;

		for (j = 0; j < line.length - 1; j++) {
			if (line.buffer[j] == '&' && line.buffer[j + 1] == code) {
				TextGroupWidget_Redraw(group, i); 
				break;
			}
		}
	}
}

static int ChatScreen_ClampIndex(int index) {
	int maxIndex = Chat_Log.count - Gui_Chatlines;
	int minIndex = min(0, maxIndex);
	Math_Clamp(index, minIndex, maxIndex);
	return index;
}

static void ChatScreen_ScrollHistoryBy(struct ChatScreen* s, int delta) {
	int newIndex = ChatScreen_ClampIndex(s->chatIndex + delta);
	delta = newIndex - s->chatIndex;

	while (delta) {
		if (delta < 0) {
			/* scrolling up to oldest */
			s->chatIndex--; delta++;
			TextGroupWidget_ShiftDown(&s->chat);
		} else {
			/* scrolling down to newest */
			s->chatIndex++; delta--;
			TextGroupWidget_ShiftUp(&s->chat);
		}
	}
}

static void ChatScreen_EnterInput(struct ChatScreen* s, bool close) {
	struct InputWidget* input;
	int defaultIndex;

	ChatScreen_SetHandlesAllInput(s, false);
	if (close) InputWidget_Clear(&s->input.base);
	ChatScreen_InputStr.length = 0;

	input = &s->input.base;
	input->OnPressedEnter(input);
	SpecialInputWidget_SetActive(&s->altText, false);

	/* Reset chat when user has scrolled up in chat history */
	defaultIndex = Chat_Log.count - Gui_Chatlines;
	if (s->chatIndex != defaultIndex) {
		s->chatIndex = defaultIndex;
		TextGroupWidget_RedrawAll(&s->chat);
	}
}

static bool ChatScreen_KeyDown(void* screen, Key key, bool was) {
	static const String slash = String_FromConst("/");
	struct ChatScreen* s = (struct ChatScreen*)screen;
	s->suppressNextPress = false;

	/* Handle text input bar */
	if (s->handlesAllInput) {
#ifdef CC_BUILD_WEB
		/* See reason for this in InputHandler_KeyUp */
		if (key == KeyBinds[KEYBIND_SEND_CHAT] || key == KEY_KP_ENTER) {
			ChatScreen_EnterInput(s, false); 
#else
		if (key == KeyBinds[KEYBIND_SEND_CHAT] || key == KEY_KP_ENTER || key == KEY_ESCAPE) {
			ChatScreen_EnterInput(s, key == KEY_ESCAPE);		
#endif
		} else if (key == KEY_PAGEUP) {
			ChatScreen_ScrollHistoryBy(s, -Gui_Chatlines);
		} else if (key == KEY_PAGEDOWN) {
			ChatScreen_ScrollHistoryBy(s, +Gui_Chatlines);
		} else {
			Elem_HandlesKeyDown(&s->input.base, key, was);
			ChatScreen_UpdateAltTextY(s);
		}
		return key < KEY_F1 || key > KEY_F35;
	}

	if (key == KeyBinds[KEYBIND_CHAT]) {
		ChatScreen_OpenInput(s, &String_Empty);
	} else if (key == KEY_SLASH) {
		ChatScreen_OpenInput(s, &slash);
	} else {
		return false;
	}
	return true;
}

static bool ChatScreen_KeyUp(void* screen, Key key) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (!s->handlesAllInput) return false;

#ifdef CC_BUILD_WEB
	/* See reason for this in InputHandler_KeyUp */
	if (key == KEY_ESCAPE) ChatScreen_EnterInput(s, true);
#endif

	if (Server.SupportsFullCP437 && key == KeyBinds[KEYBIND_EXT_INPUT]) {
		if (!Window_Focused) return true;
		SpecialInputWidget_SetActive(&s->altText, !s->altText.active);
	}
	return true;
}

static bool ChatScreen_KeyPress(void* screen, char keyChar) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	bool handled;
	if (!s->handlesAllInput) return false;

	if (s->suppressNextPress) {
		s->suppressNextPress = false;
		return false;
	}

	handled = Elem_HandlesKeyPress(&s->input.base, keyChar);
	ChatScreen_UpdateAltTextY(s);
	return handled;
}

static bool ChatScreen_MouseScroll(void* screen, float delta) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	int steps;
	if (!s->handlesAllInput) return false;

	steps = Utils_AccumulateWheelDelta(&s->chatAcc, delta);
	ChatScreen_ScrollHistoryBy(s, -steps);
	return true;
}

static bool ChatScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	String text; char textBuffer[STRING_SIZE * 4];
	struct ChatScreen* s = (struct ChatScreen*)screen;
	int height, chatY;
	if (!s->handlesAllInput || Game_HideGui) return false;

	if (!Widget_Contains(&s->chat, x, y)) {
		if (s->altText.active && Widget_Contains(&s->altText, x, y)) {
			Elem_HandlesMouseDown(&s->altText, x, y, btn);
			ChatScreen_UpdateAltTextY(s);
			return true;
		}
		Elem_HandlesMouseDown(&s->input.base, x, y, btn);
		return true;
	}

	height = TextGroupWidget_UsedHeight(&s->chat);
	chatY  = s->chat.y + s->chat.height - height;
	if (!Gui_Contains(s->chat.x, chatY, s->chat.width, height, x, y)) return false;

	String_InitArray(text, textBuffer);
	TextGroupWidget_GetSelected(&s->chat, &text, x, y);
	if (!text.length) return false;

	if (Utils_IsUrlPrefix(&text)) {
		Gui_ShowOverlay(UrlWarningOverlay_MakeInstance(&text));
	} else if (Gui_ClickableChat) {
		InputWidget_AppendString(&s->input.base, &text);
	}
	return true;
}

static void ChatScreen_ColCodeChanged(void* screen, int code) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	double caretAcc;
	if (Gfx.LostContext) return;

	SpecialInputWidget_UpdateCols(&s->altText);
	ChatElem_Recreate(&s->chat, code);
	ChatElem_Recreate(&s->status, code);
	ChatElem_Recreate(&s->bottomRight, code);
	ChatElem_Recreate(&s->clientStatus, code);

	/* Some servers have plugins that redefine colours constantly */
	/* Preserve caret accumulator so caret blinking stays consistent */
	caretAcc = s->input.base.caretAccumulator;
	Elem_Recreate(&s->input.base);
	s->input.base.caretAccumulator = caretAcc;
}

static void ChatScreen_ChatReceived(void* screen, const String* msg, int type) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (Gfx.LostContext) return;

	if (type == MSG_TYPE_NORMAL) {
		s->chatIndex++;
		if (!Gui_Chatlines) return;
		TextGroupWidget_ShiftUp(&s->chat);
	} else if (type >= MSG_TYPE_STATUS_1 && type <= MSG_TYPE_STATUS_3) {
		/* Status[0] is for texture pack downloading message */
		TextGroupWidget_Redraw(&s->status, 1 + (type - MSG_TYPE_STATUS_1));
	} else if (type >= MSG_TYPE_BOTTOMRIGHT_1 && type <= MSG_TYPE_BOTTOMRIGHT_3) {
		/* Bottom3 is top most line, so need to redraw index 0 */
		TextGroupWidget_Redraw(&s->bottomRight, 2 - (type - MSG_TYPE_BOTTOMRIGHT_1));
	} else if (type == MSG_TYPE_ANNOUNCEMENT) {
		TextWidget_Set(&s->announcement, msg, &s->announcementFont);
	} else if (type >= MSG_TYPE_CLIENTSTATUS_1 && type <= MSG_TYPE_CLIENTSTATUS_2) {
		TextGroupWidget_Redraw(&s->clientStatus, type - MSG_TYPE_CLIENTSTATUS_1);
		ChatScreen_UpdateChatYOffset(s, true);
	}
}

static void ChatScreen_ContextLost(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (s->handlesAllInput) {
		String_Copy(&ChatScreen_InputStr, &s->input.base.text);
		Camera_CheckFocus();
	}

	Elem_TryFree(&s->chat);
	Elem_TryFree(&s->input.base);
	Elem_TryFree(&s->altText);
	Elem_TryFree(&s->status);
	Elem_TryFree(&s->bottomRight);
	Elem_TryFree(&s->clientStatus);
	Elem_TryFree(&s->announcement);
}

static void ChatScreen_ContextRecreated(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	ChatScreen_ConstructWidgets(s);
	ChatScreen_SetInitialMessages(s);
	ChatScreen_UpdateChatYOffset(s, true);
}

static void ChatScreen_OnResize(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	bool active = s->altText.active;
	Elem_Recreate(s);
	SpecialInputWidget_SetActive(&s->altText, active);
}

static void ChatScreen_Init(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	int fontSize, largeSize;

	fontSize  = (int)(8  * Game_GetChatScale());
	Math_Clamp(fontSize, 8, 60);
	largeSize = (int)(16 * Game_GetChatScale());
	Math_Clamp(largeSize, 8, 60);

	Drawer2D_MakeFont(&s->chatFont,         fontSize,  FONT_STYLE_NORMAL);
	Drawer2D_MakeFont(&s->announcementFont, largeSize, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);

	Event_RegisterChat(&ChatEvents.ChatReceived,  s, ChatScreen_ChatReceived);
	Event_RegisterInt(&ChatEvents.ColCodeChanged, s, ChatScreen_ColCodeChanged);
}

static void ChatScreen_Render(void* screen, double delta) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	struct Texture tex;
	TimeMS now;
	int i, y, logIdx;

	ChatScreen_CheckOtherStatuses(s);
	if (!Game_PureClassic) { Elem_Render(&s->status, delta); }
	Elem_Render(&s->bottomRight, delta);

	ChatScreen_UpdateChatYOffset(s, false);
	y = s->clientStatus.y + s->clientStatus.height;
	for (i = 0; i < s->clientStatus.lines; i++) {
		tex = s->clientStatus.textures[i];
		if (!tex.ID) continue;

		y -= tex.Height; tex.Y = y;
		Texture_Render(&tex);
	}

	now = DateTime_CurrentUTC_MS();
	if (s->handlesAllInput) {
		Elem_Render(&s->chat, delta);
	} else {
		/* Only render recent chat */
		for (i = 0; i < s->chat.lines; i++) {
			tex    = s->chat.textures[i];
			logIdx = s->chatIndex + i;
			if (!tex.ID) continue;

			if (logIdx < 0 || logIdx >= Chat_Log.count) continue;
			if (Chat_GetLogTime(logIdx) + (10 * 1000) >= now) Texture_Render(&tex);
		}
	}

	Elem_Render(&s->announcement, delta);
	if (s->handlesAllInput) {
		Elem_Render(&s->input.base, delta);
		if (s->altText.active) {
			Elem_Render(&s->altText, delta);
		}
	}

	if (s->announcement.texture.ID && now > Chat_AnnouncementReceived + (5 * 1000)) {
		Elem_TryFree(&s->announcement);
	}
}

static void ChatScreen_Free(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	Font_Free(&s->chatFont);
	Font_Free(&s->announcementFont);
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
	s->inputOldHeight     = -1;
	s->lastDownloadStatus = Int32_MinValue;

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

	extent = (int)(CH_EXTENT * Game_Scale(Window_Height / 480.0f));
	tex.ID = Gui_IconsTex;
	tex.X  = (Window_Width  / 2) - extent;
	tex.Y  = (Window_Height / 2) - extent;

	tex.Width  = extent * 2;
	tex.Height = extent * 2;
	Texture_Render(&tex);
}

static void HUDScreen_ContextLost(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	Elem_TryFree(&s->hotbar);

	s->wasShowingList = s->showingList;
	if (s->showingList) { Elem_TryFree(&s->playerList); }
	s->showingList = false;
}

static void HUDScreen_ContextRecreated(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	bool extended;

	Elem_TryFree(&s->hotbar);
	Elem_Init(&s->hotbar);

	if (!s->wasShowingList) return;
	extended = Server.SupportsExtPlayerList && !Gui_ClassicTabList;
	PlayerListWidget_Create(&s->playerList, &s->playerFont, !extended);
	s->showingList = true;

	Elem_Init(&s->playerList);
	Widget_Reposition(&s->playerList);
}

static void HUDScreen_OnResize(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	Screen_OnResize(s->chat);

	Widget_Reposition(&s->hotbar);
	if (s->showingList) { Widget_Reposition(&s->playerList); }
}

static bool HUDScreen_KeyPress(void* screen, char keyChar) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	return Elem_HandlesKeyPress(s->chat, keyChar); 
}

static bool HUDScreen_KeyDown(void* screen, Key key, bool was) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	Key playerListKey = KeyBinds[KEYBIND_PLAYER_LIST];
	bool handles = playerListKey != KEY_TAB || !Gui_TabAutocomplete || !s->chat->handlesAllInput;

	if (key == playerListKey && handles) {
		if (!s->showingList && !Server.IsSinglePlayer) {
			s->wasShowingList = true;
			HUDScreen_ContextRecreated(s);
		}
		return true;
	}

	return Elem_HandlesKeyDown(s->chat, key, was) || Elem_HandlesKeyDown(&s->hotbar, key, was);
}

static bool HUDScreen_KeyUp(void* screen, Key key) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	if (key == KeyBinds[KEYBIND_PLAYER_LIST] && s->showingList) {
		s->showingList    = false;
		s->wasShowingList = false;
		Elem_TryFree(&s->playerList);
		return true;
	}

	return Elem_HandlesKeyUp(s->chat, key) || Elem_HandlesKeyUp(&s->hotbar, key);
}

static bool HUDScreen_MouseScroll(void* screen, float delta) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	return Elem_HandlesMouseScroll(s->chat, delta);
}

static bool HUDScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	String name; char nameBuffer[STRING_SIZE + 1];
	struct HUDScreen* s = (struct HUDScreen*)screen;
	struct ChatScreen* chat;	

	if (btn != MOUSE_LEFT || !s->handlesAllInput) return false;
	chat = (struct ChatScreen*)s->chat;
	if (!s->showingList) { return Elem_HandlesMouseDown(chat, x, y, btn); }

	String_InitArray(name, nameBuffer);
	PlayerListWidget_GetNameUnder(&s->playerList, x, y, &name);
	if (!name.length) { return Elem_HandlesMouseDown(chat, x, y, btn); }

	String_Append(&name, ' ');
	if (chat->handlesAllInput) { InputWidget_AppendString(&chat->input.base, &name); }
	return true;
}

static void HUDScreen_Init(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	int size = Drawer2D_BitmappedText ? 16 : 11;
	Drawer2D_MakeFont(&s->playerFont, size, FONT_STYLE_NORMAL);

	ChatScreen_MakeInstance();
	s->chat = (struct Screen*)&ChatScreen_Instance;
	Elem_Init(s->chat);

	HotbarWidget_Create(&s->hotbar);
	s->wasShowingList = false;
	Screen_CommonInit(s);
}

static void HUDScreen_Render(void* screen, double delta) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	struct ChatScreen* chat = (struct ChatScreen*)s->chat;
	bool showMinimal;

	if (Game_HideGui && chat->handlesAllInput) {
		Gfx_SetTexturing(true);
		Elem_Render(&chat->input.base, delta);
		Gfx_SetTexturing(false);
	}
	if (Game_HideGui) return;
	showMinimal = Gui_GetActiveScreen()->blocksWorld;

	if (!s->showingList && !showMinimal) {
		Gfx_SetTexturing(true);
		HUDScreen_DrawCrosshairs();
		Gfx_SetTexturing(false);
	}
	if (chat->handlesAllInput && !Game_PureClassic) {
		ChatScreen_RenderBackground(chat);
	}

	Gfx_SetTexturing(true);
	if (!showMinimal) { Elem_Render(&s->hotbar, delta); }
	Elem_Render(s->chat, delta);

	if (s->showingList && Gui_GetActiveScreen() == (struct Screen*)s) {
		s->playerList.active = s->chat->handlesAllInput;
		Elem_Render(&s->playerList, delta);
		/* NOTE: Should usually be caught by KeyUp, but just in case. */
		if (!KeyBind_IsPressed(KEYBIND_PLAYER_LIST)) {
			Elem_TryFree(&s->playerList);
			s->showingList = false;
		}
	}
	Gfx_SetTexturing(false);
}

static void HUDScreen_Free(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	Font_Free(&s->playerFont);
	Elem_TryFree(s->chat);
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
	s->wasShowingList = false;
	s->VTABLE = &HUDScreen_VTABLE;
	return (struct Screen*)s;
}

void HUDScreen_OpenInput(struct Screen* hud, const String* text) {
	struct Screen* chat = ((struct HUDScreen*)hud)->chat;
	ChatScreen_OpenInput((struct ChatScreen*)chat, text);
}

void HUDScreen_AppendInput(struct Screen* hud, const String* text) {
	struct Screen* chat = ((struct HUDScreen*)hud)->chat;
	struct ChatInputWidget* widget = &((struct ChatScreen*)chat)->input;
	InputWidget_AppendString(&widget->base, text);
}

struct Widget* HUDScreen_GetHotbar(struct Screen* hud) {
	struct HUDScreen* s = (struct HUDScreen*)hud;
	return (struct Widget*)(&s->hotbar);
}


/*########################################################################################################################*
*----------------------------------------------------DisconnectScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct DisconnectScreen DisconnectScreen_Instance;
#define DISCONNECT_DELAY_MS 5000
static void DisconnectScreen_ReconnectMessage(struct DisconnectScreen* s, String* msg) {
	if (s->canReconnect) {
		int elapsedMS = (int)(DateTime_CurrentUTC_MS() - s->initTime);
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

	elapsedMS = (int)(DateTime_CurrentUTC_MS() - s->initTime);
	secsLeft  = (DISCONNECT_DELAY_MS - elapsedMS) / MILLIS_PER_SEC;
	if (secsLeft < 0) secsLeft = 0;
	if (s->lastSecsLeft == secsLeft && s->reconnect.active == s->lastActive) return;

	String_InitArray(msg, msgBuffer);
	DisconnectScreen_ReconnectMessage(s, &msg);
	ButtonWidget_Set(&s->reconnect, &msg, &s->titleFont);

	s->reconnect.disabled = secsLeft != 0;
	s->lastSecsLeft = secsLeft;
	s->lastActive   = s->reconnect.active;
}

static void DisconnectScreen_ContextLost(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	Elem_TryFree(&s->title);
	Elem_TryFree(&s->message);
	Elem_TryFree(&s->reconnect);
}

static void DisconnectScreen_ContextRecreated(void* screen) {
	String msg; char msgBuffer[STRING_SIZE];
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;

	TextWidget_Create(&s->title, &s->titleStr, &s->titleFont);
	Widget_SetLocation(&s->title, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);

	TextWidget_Create(&s->message, &s->messageStr, &s->messageFont);
	Widget_SetLocation(&s->message, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 10);

	String_InitArray(msg, msgBuffer);
	DisconnectScreen_ReconnectMessage(s, &msg);

	ButtonWidget_Create(&s->reconnect, 300, &msg, &s->titleFont, NULL);
	Widget_SetLocation(&s->reconnect, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 80);
	s->reconnect.disabled = !s->canReconnect;
}

static void DisconnectScreen_Init(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	Drawer2D_MakeFont(&s->titleFont,   16, FONT_STYLE_BOLD);
	Drawer2D_MakeFont(&s->messageFont, 16, FONT_STYLE_NORMAL);
	Screen_CommonInit(s);

	/* NOTE: changing VSync can't be done within frame, causes crash on some GPUs */
	Gfx_SetFpsLimit(Game_FpsLimit == FPS_LIMIT_VSYNC, 1000 / 5.0f);

	s->initTime     = DateTime_CurrentUTC_MS();
	s->lastSecsLeft = DISCONNECT_DELAY_MS / MILLIS_PER_SEC;
}

static void DisconnectScreen_Render(void* screen, double delta) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	PackedCol top    = PACKEDCOL_CONST(64, 32, 32, 255);
	PackedCol bottom = PACKEDCOL_CONST(80, 16, 16, 255);

	if (s->canReconnect) { DisconnectScreen_UpdateDelayLeft(s, delta); }
	Gfx_Draw2DGradient(0, 0, Window_Width, Window_Height, top, bottom);

	Gfx_SetTexturing(true);
	Elem_Render(&s->title, delta);
	Elem_Render(&s->message, delta);

	if (s->canReconnect) { Elem_Render(&s->reconnect, delta); }
	Gfx_SetTexturing(false);
}

static void DisconnectScreen_Free(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	Font_Free(&s->titleFont);
	Font_Free(&s->messageFont);
	Screen_CommonFree(s);
	Game_SetFpsLimit(Game_FpsLimit);
}

static void DisconnectScreen_OnResize(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	Widget_Reposition(&s->title);
	Widget_Reposition(&s->message);
	Widget_Reposition(&s->reconnect);
}

static bool DisconnectScreen_KeyDown(void* s, Key key, bool was) { return key < KEY_F1 || key > KEY_F35; }
static bool DisconnectScreen_KeyPress(void* s, char keyChar) { return true; }
static bool DisconnectScreen_KeyUp(void* s, Key key) { return true; }

static bool DisconnectScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	struct ButtonWidget* w = &s->reconnect;

	if (btn != MOUSE_LEFT) return true;
	if (!w->disabled && Widget_Contains(w, x, y)) Server.BeginConnect();
	return true;
}

static bool DisconnectScreen_MouseMove(void* screen, int x, int y) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	struct ButtonWidget* w     = &s->reconnect;

	w->active = !w->disabled && Widget_Contains(w, x, y);
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
	static const String kick = String_FromConst("Kicked ");
	static const String ban  = String_FromConst("Banned ");
	String why; char whyBuffer[STRING_SIZE];
	struct DisconnectScreen* s = &DisconnectScreen_Instance;

	s->handlesAllInput = true;
	s->blocksWorld     = true;
	s->hidesHUD        = true;

	String_InitArray(s->titleStr, s->_titleBuffer);
	String_AppendString(&s->titleStr, title);
	String_InitArray(s->messageStr, s->_messageBuffer);
	String_AppendString(&s->messageStr, message);

	String_InitArray(why, whyBuffer);
	String_AppendColorless(&why, message);
	
	s->canReconnect = !(String_CaselessStarts(&why, &kick) || String_CaselessStarts(&why, &ban));
	s->VTABLE = &DisconnectScreen_VTABLE;
	return (struct Screen*)s;
}
