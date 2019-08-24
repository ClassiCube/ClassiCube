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

#define CHAT_MAX_STATUS Array_Elems(Chat_Status)
#define CHAT_MAX_BOTTOMRIGHT Array_Elems(Chat_BottomRight)
#define CHAT_MAX_CLIENTSTATUS Array_Elems(Chat_ClientStatus)

struct HUDScreen {
	Screen_Layout
	struct HotbarWidget hotbar;
	/* player list state */
	struct PlayerListWidget playerList;
	FontDesc playerFont;
	bool showingList, wasShowingList;
	/* chat state */
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

bool Screen_FKey(void* elem, int key)               { return false; }
bool Screen_FKeyPress(void* elem, char keyChar)     { return false; }
bool Screen_FMouseScroll(void* elem, float delta)   { return false; }
bool Screen_FMouse(void* elem, int x, int y, int btn) { return false; }
bool Screen_FMouseMove(void* elem, int x, int y)    { return false; }

bool Screen_TKeyPress(void* elem, char keyChar)     { return true; }
bool Screen_TKey(void* s, int key)                  { return true; }
bool Screen_TMouseScroll(void* screen, float delta) { return true; }
bool Screen_TMouse(void* screen, int x, int y, int btn) { return true; }
bool Screen_TMouseMove(void* elem, int x, int y)    { return true; }
static void Screen_NullFunc(void* screen) { }

CC_NOINLINE static bool IsOnlyHudActive(void) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		if (s->grabsInput && s != Gui_HUD) return false;
	}
	return true;
}


/*########################################################################################################################*
*-----------------------------------------------------InventoryScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct InventoryScreen {
	Screen_Layout
	FontDesc font;
	struct TableWidget table;
	bool releasedInv, deferredSelect;
} InventoryScreen_Instance;

static void InventoryScreen_OnBlockChanged(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	TableWidget_OnInventoryChanged(&s->table);
}

static void InventoryScreen_ContextLost(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Font_Free(&s->font);
	Elem_TryFree(&s->table);
}

static void InventoryScreen_ContextRecreated(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Drawer2D_MakeFont(&s->font, 16, FONT_STYLE_NORMAL);
	TableWidget_Recreate(&s->table);
}

static void InventoryScreen_MoveToSelected(struct InventoryScreen* s) {
	struct TableWidget* table = &s->table;
	TableWidget_SetBlockTo(table, Inventory_SelectedBlock);
	TableWidget_Recreate(table);

	s->deferredSelect = false;
	/* User is holding invalid block */
	if (table->selectedIndex == -1) {
		TableWidget_MakeDescTex(table, Inventory_SelectedBlock);
	}
}

static void InventoryScreen_Init(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	
	TableWidget_Create(&s->table);
	s->table.font         = &s->font;
	s->table.blocksPerRow = Game_PureClassic ? 9 : 10;
	TableWidget_RecreateBlocks(&s->table);

	/* Can't immediately move to selected here, because cursor grabbed  */
	/* status might be toggled after InventoryScreen_Init() is called. */
	/* That causes the cursor to be moved back to the middle of the window. */
	s->deferredSelect = true;

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
	Event_UnregisterVoid(&BlockEvents.PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&BlockEvents.BlockDefChanged,    s, InventoryScreen_OnBlockChanged);
}

static bool InventoryScreen_KeyDown(void* screen, Key key) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct TableWidget* table = &s->table;

	if (key == KeyBinds[KEYBIND_INVENTORY] && s->releasedInv) {
		Gui_Remove(screen);
	} else if (key == KEY_ENTER && table->selectedIndex != -1) {
		Inventory_SetSelectedBlock(table->blocks[table->selectedIndex]);
		Gui_Remove(screen);
	} else if (Elem_HandlesKeyDown(table, key)) {
	} else {
		struct HUDScreen* hud = (struct HUDScreen*)Gui_HUD;
		return Elem_HandlesKeyDown(&hud->hotbar, key);
	}
	return true;
}

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
		if (!hotbar) Gui_Remove(screen);
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

static const struct ScreenVTABLE InventoryScreen_VTABLE = {
	InventoryScreen_Init,      InventoryScreen_Render,  InventoryScreen_Free,
	InventoryScreen_KeyDown,   InventoryScreen_KeyUp,   Screen_TKeyPress,
	InventoryScreen_MouseDown, InventoryScreen_MouseUp, InventoryScreen_MouseMove, InventoryScreen_MouseScroll,
	InventoryScreen_OnResize,  InventoryScreen_ContextLost, InventoryScreen_ContextRecreated
};
void InventoryScreen_Show(void) {
	struct InventoryScreen* s = &InventoryScreen_Instance;
	s->grabsInput = true;
	s->closable   = true;

	s->VTABLE = &InventoryScreen_VTABLE;
	Gui_Replace((struct Screen*)s, GUI_PRIORITY_INVENTORY);
}


/*########################################################################################################################*
*-------------------------------------------------------StatusScreen------------------------------------------------------*
*#########################################################################################################################*/
static struct StatusScreen {
	Screen_Layout
	FontDesc font;
	struct TextWidget line1, line2;
	struct TextAtlas posAtlas;
	double accumulator;
	int frames, fps;
	bool speed, halfSpeed, noclip, fly, canSpeed;
	int lastFov;
} StatusScreen_Instance;

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

static void StatusScreen_ContextLost(void* screen) {
	struct StatusScreen* s = (struct StatusScreen*)screen;
	Font_Free(&s->font);
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
	Drawer2D_MakeFont(&s->font, 16, FONT_STYLE_NORMAL);
	
	y = 2;
	TextWidget_Make(line1, ANCHOR_MIN, ANCHOR_MIN, 2, y);
	line1->reducePadding = true;
	StatusScreen_Update(s, 1.0);

	y += line1->height;
	TextAtlas_Make(&s->posAtlas, &chars, &s->font, &prefix);
	s->posAtlas.tex.Y = y;

	y += s->posAtlas.tex.Height;
	TextWidget_Make(line2, ANCHOR_MIN, ANCHOR_MIN, 2, y);
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

static void StatusScreen_Render(void* screen, double delta) {
	struct StatusScreen* s = (struct StatusScreen*)screen;
	StatusScreen_Update(s, delta);
	if (Game_HideGui) return;

	/* TODO: If Game_ShowFps is off and not classic mode, we should just return here */
	Gfx_SetTexturing(true);
	if (Gui_ShowFPS) Elem_Render(&s->line1, delta);

	if (Game_ClassicMode) {
		Elem_Render(&s->line2, delta);
	} else if (IsOnlyHudActive() && Gui_ShowFPS) {
		if (StatusScreen_HacksChanged(s)) { StatusScreen_UpdateHackState(s); }
		StatusScreen_DrawPosition(s);
		Elem_Render(&s->line2, delta);
	}
	Gfx_SetTexturing(false);
}

static const struct ScreenVTABLE StatusScreen_VTABLE = {
	Screen_NullFunc, StatusScreen_Render, Screen_NullFunc,
	Screen_FKey,     Screen_FKey,         Screen_FKeyPress,
	Screen_FMouse,   Screen_FMouse,       Screen_FMouseMove, Screen_FMouseScroll,
	Screen_NullFunc, StatusScreen_ContextLost, StatusScreen_ContextRecreated
};
void StatusScreen_Show(void) {
	struct StatusScreen* s = &StatusScreen_Instance;
	s->VTABLE  = &StatusScreen_VTABLE;
	Gui_Status =(struct Screen*)s;
	Gui_Replace((struct Screen*)s, GUI_PRIORITY_STATUS);
}


/*########################################################################################################################*
*------------------------------------------------------LoadingScreen------------------------------------------------------*
*#########################################################################################################################*/
static struct LoadingScreen {
	Screen_Layout
	FontDesc font;
	float progress;
	
	struct TextWidget title, message;
	String titleStr, messageStr;
	const char* lastState;

	char _titleBuffer[STRING_SIZE];
	char _messageBuffer[STRING_SIZE];
} LoadingScreen_Instance;

static void LoadingScreen_SetTitle(struct LoadingScreen* s) {
	TextWidget_Set(&s->title, &s->titleStr, &s->font);
}
static void LoadingScreen_SetMessage(struct LoadingScreen* s) {
	TextWidget_Set(&s->message, &s->messageStr, &s->font);
}

static void LoadingScreen_MapLoading(void* screen, float progress) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	s->progress = progress;
}

static void LoadingScreen_OnResize(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	if (!s->title.VTABLE) return;
	Widget_Reposition(&s->title);
	Widget_Reposition(&s->message);
}

static void LoadingScreen_ContextLost(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Font_Free(&s->font);
	if (!s->title.VTABLE) return;

	Elem_Free(&s->title);
	Elem_Free(&s->message);
}

static void LoadingScreen_ContextRecreated(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Drawer2D_MakeFont(&s->font, 16, FONT_STYLE_NORMAL);
	LoadingScreen_SetTitle(s);
	LoadingScreen_SetMessage(s);
}

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

	TextWidget_Make(&s->title,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -31);
	TextWidget_Make(&s->message, ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  17);

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

	Elem_Render(&s->title,   delta);
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
	Event_UnregisterFloat(&WorldEvents.Loading, s, LoadingScreen_MapLoading);
}

CC_NOINLINE static void LoadingScreen_ShowCommon(const String* title, const String* message) {
	struct LoadingScreen* s = &LoadingScreen_Instance;
	s->lastState = NULL;
	s->progress  = 0.0f;

	String_InitArray(s->titleStr,   s->_titleBuffer);
	String_AppendString(&s->titleStr,   title);
	String_InitArray(s->messageStr, s->_messageBuffer);
	String_AppendString(&s->messageStr, message);
	
	s->grabsInput  = true;
	s->blocksWorld = true;
	Gui_Replace((struct Screen*)s, 
		Game_ClassicMode ? GUI_PRIORITY_OLDLOADING : GUI_PRIORITY_LOADING);
}

static const struct ScreenVTABLE LoadingScreen_VTABLE = {
	LoadingScreen_Init, LoadingScreen_Render, LoadingScreen_Free,
	Screen_TKey,        Screen_TKey,          Screen_TKeyPress,
	Screen_TMouse,      Screen_TMouse,        Screen_TMouseMove,  Screen_TMouseScroll,
	LoadingScreen_OnResize, LoadingScreen_ContextLost, LoadingScreen_ContextRecreated
};
void LoadingScreen_Show(const String* title, const String* message) {
	LoadingScreen_Instance.VTABLE = &LoadingScreen_VTABLE;
	LoadingScreen_ShowCommon(title, message);
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
		Gen_Done = true;
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

	Gui_Remove(LoadingScreen_UNSAFE_RawPointer);
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

static const struct ScreenVTABLE GeneratingScreen_VTABLE = {
	GeneratingScreen_Init, GeneratingScreen_Render, LoadingScreen_Free,
	Screen_TKey,           Screen_TKey,             Screen_TKeyPress,
	Screen_TMouse,         Screen_TMouse,           Screen_FMouseMove,  Screen_TMouseScroll,
	LoadingScreen_OnResize, LoadingScreen_ContextLost, LoadingScreen_ContextRecreated
};
void GeneratingScreen_Show(void) {
	static const String title   = String_FromConst("Generating level");
	static const String message = String_FromConst("Generating..");

	LoadingScreen_Instance.VTABLE = &GeneratingScreen_VTABLE;
	LoadingScreen_ShowCommon(&title, &message);
}


/*########################################################################################################################*
*--------------------------------------------------------HUDScreen--------------------------------------------------------*
*#########################################################################################################################*/
static struct HUDScreen HUDScreen_Instance;
#define CH_EXTENT 16

static int HUDScreen_BottomOffset(void) { return HUDScreen_Instance.hotbar.height; }
static int HUDScreen_InputUsedHeight(struct HUDScreen* s) {
	if (s->altText.height == 0) {
		return s->input.base.height + 20;
	} else {
		return (Window_Height - s->altText.y) + 5;
	}
}

static void HUDScreen_UpdateAltTextY(struct HUDScreen* s) {
	struct InputWidget* input = &s->input.base;
	int height = max(input->height + input->yOffset, HUDScreen_BottomOffset());
	height += input->yOffset;

	s->altText.yOffset = height;
	Widget_Reposition(&s->altText);
}

static String HUDScreen_GetChat(void* obj, int i) {
	i += HUDScreen_Instance.chatIndex;

	if (i >= 0 && i < Chat_Log.count) {
		return StringsBuffer_UNSAFE_Get(&Chat_Log, i);
	}
	return String_Empty;
}

static String HUDScreen_GetStatus(void* obj, int i)       { return Chat_Status[i]; }
static String HUDScreen_GetBottomRight(void* obj, int i)  { return Chat_BottomRight[2 - i]; }
static String HUDScreen_GetClientStatus(void* obj, int i) { return Chat_ClientStatus[i]; }

static void HUDScreen_FreeChatFonts(struct HUDScreen* s) {
	Font_Free(&s->chatFont);
	Font_Free(&s->announcementFont);
}

static void HUDScreen_InitChatFonts(struct HUDScreen* s) {
	int size;

	size = (int)(8  * Game_GetChatScale());
	Math_Clamp(size, 8, 60);
	Drawer2D_MakeFont(&s->chatFont, size, FONT_STYLE_NORMAL);

	size = (int)(16 * Game_GetChatScale());
	Math_Clamp(size, 8, 60);
	Drawer2D_MakeFont(&s->announcementFont, size, FONT_STYLE_NORMAL);
}

static void HUDScreen_ChatUpdateFont(struct HUDScreen* s) {
	ChatInputWidget_SetFont(&s->input, &s->chatFont);
	TextGroupWidget_SetFont(&s->status, &s->chatFont);
	TextGroupWidget_SetFont(&s->bottomRight, &s->chatFont);
	TextGroupWidget_SetFont(&s->chat, &s->chatFont);
	TextGroupWidget_SetFont(&s->clientStatus, &s->chatFont);
}

static void HUDScreen_ChatUpdateLayout(struct HUDScreen* s) {
	int yOffset = HUDScreen_BottomOffset() + 15;
	Widget_SetLocation(&s->input.base, ANCHOR_MIN, ANCHOR_MAX, 5, 5);
	Widget_SetLocation(&s->altText,    ANCHOR_MIN, ANCHOR_MAX, 5, 5);
	HUDScreen_UpdateAltTextY(s);

	Widget_SetLocation(&s->status,       ANCHOR_MAX, ANCHOR_MIN,  0,       0);
	Widget_SetLocation(&s->bottomRight,  ANCHOR_MAX, ANCHOR_MAX,  0, yOffset);
	Widget_SetLocation(&s->chat,         ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
	Widget_SetLocation(&s->clientStatus, ANCHOR_MIN, ANCHOR_MAX, 10, yOffset);
}

static void HUDScreen_ChatInit(struct HUDScreen* s) {
	ChatInputWidget_Create(&s->input);
	SpecialInputWidget_Create(&s->altText, &s->chatFont, &s->input.base);

	TextGroupWidget_Create(&s->status, CHAT_MAX_STATUS,
							s->statusTextures, HUDScreen_GetStatus);
	TextGroupWidget_Create(&s->bottomRight, CHAT_MAX_BOTTOMRIGHT, 
							s->bottomRightTextures, HUDScreen_GetBottomRight);
	TextGroupWidget_Create(&s->chat, Gui_Chatlines,
							s->chatTextures, HUDScreen_GetChat);
	TextGroupWidget_Create(&s->clientStatus, CHAT_MAX_CLIENTSTATUS,
							s->clientStatusTextures, HUDScreen_GetClientStatus);
	TextWidget_Make(&s->announcement, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -Window_Height / 4);

	s->status.collapsible[0]       = true; /* Texture pack download status */
	s->clientStatus.collapsible[0] = true;
	s->clientStatus.collapsible[1] = true;

	s->chat.underlineUrls = !Game_ClassicMode;
	s->chatIndex = Chat_Log.count - Gui_Chatlines;
}

static void HUDScreen_Redraw(struct HUDScreen* s) {
	TextGroupWidget_RedrawAll(&s->chat);
	TextWidget_Set(&s->announcement, &Chat_Announcement, &s->announcementFont);
	TextGroupWidget_RedrawAll(&s->status);
	TextGroupWidget_RedrawAll(&s->bottomRight);
	TextGroupWidget_RedrawAll(&s->clientStatus);

	if (s->grabsInput) InputWidget_UpdateText(&s->input.base);
	SpecialInputWidget_Redraw(&s->altText);
}

static void HUDScreen_UpdateChatYOffset(struct HUDScreen* s, bool force) {
	int height = HUDScreen_InputUsedHeight(s);
	if (force || height != s->inputOldHeight) {
		int bottomOffset = HUDScreen_BottomOffset() + 15;
		s->clientStatus.yOffset = max(bottomOffset, height);
		Widget_Reposition(&s->clientStatus);
		
		s->chat.yOffset = s->clientStatus.yOffset + s->clientStatus.height;
		Widget_Reposition(&s->chat);
		s->inputOldHeight = height;
	}
}

static int HUDScreen_ClampChatIndex(int index) {
	int maxIndex = Chat_Log.count - Gui_Chatlines;
	int minIndex = min(0, maxIndex);
	Math_Clamp(index, minIndex, maxIndex);
	return index;
}

static void HUDScreen_ScrollChatBy(struct HUDScreen* s, int delta) {
	int newIndex = HUDScreen_ClampChatIndex(s->chatIndex + delta);
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

static void HUDScreen_EnterChatInput(struct HUDScreen* s, bool close) {
	struct InputWidget* input;
	int defaultIndex;

	s->grabsInput = false;
	Camera_CheckFocus();
	if (close) InputWidget_Clear(&s->input.base);

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

static void HUDScreen_UpdateTexpackStatus(struct HUDScreen* s) {
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

static void HUDScreen_ColCodeChanged(void* screen, int code) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	double caretAcc;
	if (Gfx.LostContext) return;

	SpecialInputWidget_UpdateCols(&s->altText);
	TextGroupWidget_RedrawAllWithCol(&s->chat,         code);
	TextGroupWidget_RedrawAllWithCol(&s->status,       code);
	TextGroupWidget_RedrawAllWithCol(&s->bottomRight,  code);
	TextGroupWidget_RedrawAllWithCol(&s->clientStatus, code);

	/* Some servers have plugins that redefine colours constantly */
	/* Preserve caret accumulator so caret blinking stays consistent */
	caretAcc = s->input.base.caretAccumulator;
	InputWidget_UpdateText(&s->input.base);
	s->input.base.caretAccumulator = caretAcc;
}

static void HUDScreen_ChatReceived(void* screen, const String* msg, int type) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
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
		HUDScreen_UpdateChatYOffset(s, true);
	}
}

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

static void HUDScreen_DrawChatBackground(struct HUDScreen* s) {
	int usedHeight = TextGroupWidget_UsedHeight(&s->chat);
	int x = s->chat.x;
	int y = s->chat.y + s->chat.height - usedHeight;

	int width  = max(s->clientStatus.width, s->chat.width);
	int height = usedHeight + s->clientStatus.height;

	if (height > 0) {
		PackedCol backCol = PACKEDCOL_CONST(0, 0, 0, 127);
		Gfx_Draw2DFlat(x - 5, y - 5, width + 10, height + 10, backCol);
	}
}

static void HUDScreen_DrawChat(struct HUDScreen* s, double delta) {
	struct Texture tex;
	TimeMS now;
	int i, logIdx;

	HUDScreen_UpdateTexpackStatus(s);
	if (!Game_PureClassic) { Elem_Render(&s->status, delta); }
	Elem_Render(&s->bottomRight, delta);

	HUDScreen_UpdateChatYOffset(s, false);
	Elem_Render(&s->clientStatus, delta);

	now = DateTime_CurrentUTC_MS();
	if (s->grabsInput) {
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
	if (s->grabsInput) {
		Elem_Render(&s->input.base, delta);
		if (s->altText.active) {
			Elem_Render(&s->altText, delta);
		}
	}

	if (s->announcement.tex.ID && now > Chat_AnnouncementReceived + (5 * 1000)) {
		Elem_TryFree(&s->announcement);
	}
}

static void HUDScreen_ContextLost(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	Font_Free(&s->playerFont);
	HUDScreen_FreeChatFonts(s);

	Elem_TryFree(&s->chat);
	Elem_TryFree(&s->input.base);
	Elem_TryFree(&s->altText);
	Elem_TryFree(&s->status);
	Elem_TryFree(&s->bottomRight);
	Elem_TryFree(&s->clientStatus);
	Elem_TryFree(&s->announcement);

	s->wasShowingList = s->showingList;
	if (s->showingList) { Elem_TryFree(&s->playerList); }
	s->showingList    = false;
}

static void HUDScreen_RemakePlayerList(struct HUDScreen* s) {
	bool extended = Server.SupportsExtPlayerList && !Gui_ClassicTabList;
	if (!s->wasShowingList) return;
	PlayerListWidget_Create(&s->playerList, &s->playerFont, !extended);
	s->showingList = true;

	Elem_Init(&s->playerList);
	Widget_Reposition(&s->playerList);
}

static void HUDScreen_ContextRecreated(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	int size = Drawer2D_BitmappedText ? 16 : 11;
	Drawer2D_MakeFont(&s->playerFont, size, FONT_STYLE_NORMAL);
	HUDScreen_InitChatFonts(s);
	HUDScreen_ChatUpdateFont(s);
	HUDScreen_ChatUpdateLayout(s);

	HUDScreen_Redraw(s);
	HUDScreen_UpdateChatYOffset(s, true);
	HUDScreen_RemakePlayerList(s);
	Widget_Reposition(&s->hotbar);
}

static void HUDScreen_OnResize(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	HUDScreen_ChatUpdateLayout(s);
	Widget_Reposition(&s->hotbar);
	if (s->showingList) { Widget_Reposition(&s->playerList); }
}

static bool HUDScreen_KeyPress(void* screen, char keyChar) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	if (!s->grabsInput) return false;

	if (s->suppressNextPress) {
		s->suppressNextPress = false;
		return false;
	}

	InputWidget_Append(&s->input.base, keyChar);
	HUDScreen_UpdateAltTextY(s);
	return true;
}

static bool HUDScreen_KeyDown(void* screen, Key key) {
	static const String slash = String_FromConst("/");
	struct HUDScreen* s = (struct HUDScreen*)screen;
	Key playerListKey = KeyBinds[KEYBIND_PLAYER_LIST];
	bool handlesList  = playerListKey != KEY_TAB || !Gui_TabAutocomplete || !s->grabsInput;

	if (key == playerListKey && handlesList) {
		if (!s->showingList && !Server.IsSinglePlayer) {
			s->wasShowingList = true;
			HUDScreen_RemakePlayerList(s);
		}
		return true;
	}

	s->suppressNextPress = false;
	/* Handle chat text input */
	if (s->grabsInput) {
#ifdef CC_BUILD_WEB
		/* See reason for this in InputHandler_KeyUp */
		if (key == KeyBinds[KEYBIND_SEND_CHAT] || key == KEY_KP_ENTER) {
			HUDScreen_EnterChatInput(s, false);
#else
		if (key == KeyBinds[KEYBIND_SEND_CHAT] || key == KEY_KP_ENTER || key == KEY_ESCAPE) {
			HUDScreen_EnterChatInput(s, key == KEY_ESCAPE);
#endif
		} else if (key == KEY_PAGEUP) {
			HUDScreen_ScrollChatBy(s, -Gui_Chatlines);
		} else if (key == KEY_PAGEDOWN) {
			HUDScreen_ScrollChatBy(s, +Gui_Chatlines);
		} else {
			Elem_HandlesKeyDown(&s->input.base, key);
			HUDScreen_UpdateAltTextY(s);
		}
		return key < KEY_F1 || key > KEY_F35;
	}

	if (key == KeyBinds[KEYBIND_CHAT]) {
		HUDScreen_OpenInput(&String_Empty);
	} else if (key == KEY_SLASH) {
		HUDScreen_OpenInput(&slash);
	} else if (key == KeyBinds[KEYBIND_INVENTORY]) {
		InventoryScreen_Show();
	} else {
		return Elem_HandlesKeyDown(&s->hotbar, key);
	}
	return true;
}

static bool HUDScreen_KeyUp(void* screen, Key key) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	if (key == KeyBinds[KEYBIND_PLAYER_LIST] && s->showingList) {
		s->showingList    = false;
		s->wasShowingList = false;
		Elem_TryFree(&s->playerList);
		return true;
	}

	if (!s->grabsInput) return Elem_HandlesKeyUp(&s->hotbar, key);
#ifdef CC_BUILD_WEB
	/* See reason for this in InputHandler_KeyUp */
	if (key == KEY_ESCAPE) HUDScreen_EnterChatInput(s, true);
#endif

	if (Server.SupportsFullCP437 && key == KeyBinds[KEYBIND_EXT_INPUT]) {
		if (!Window_Focused) return true;
		SpecialInputWidget_SetActive(&s->altText, !s->altText.active);
	}
	return true;
}

static bool HUDScreen_MouseScroll(void* screen, float delta) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	int steps;
	if (!s->grabsInput) return false;

	steps = Utils_AccumulateWheelDelta(&s->chatAcc, delta);
	HUDScreen_ScrollChatBy(s, -steps);
	return true;
}

static bool HUDScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	String text; char textBuffer[STRING_SIZE * 4];
	struct HUDScreen* s = (struct HUDScreen*)screen;
	int height, chatY;

	if (btn != MOUSE_LEFT || !s->grabsInput) return false;

	/* player clicks on name in tab list */
	/* TODO: Move to PlayerListWidget */
	if (s->showingList) {
		String_InitArray(text, textBuffer);
		PlayerListWidget_GetNameUnder(&s->playerList, x, y, &text);

		if (text.length) {
			String_Append(&text, ' ');
			HUDScreen_AppendInput(&text);
			return true;
		}
	}

	if (Game_HideGui) return false;

	if (!Widget_Contains(&s->chat, x, y)) {
		if (s->altText.active && Widget_Contains(&s->altText, x, y)) {
			Elem_HandlesMouseDown(&s->altText, x, y, btn);
			HUDScreen_UpdateAltTextY(s);
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
		UrlWarningOverlay_Show(&text);
	} else if (Gui_ClickableChat) {
		InputWidget_AppendString(&s->input.base, &text);
		HUDScreen_UpdateAltTextY(s);
	}
	return true;
}

static void HUDScreen_Init(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	s->wasShowingList = false;
	HotbarWidget_Create(&s->hotbar);
	HUDScreen_ChatInit(s);

	Event_RegisterChat(&ChatEvents.ChatReceived,  s, HUDScreen_ChatReceived);
	Event_RegisterInt(&ChatEvents.ColCodeChanged, s, HUDScreen_ColCodeChanged);
}

static void HUDScreen_Render(void* screen, double delta) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	bool showMinimal;

	if (Game_HideGui && s->grabsInput) {
		Gfx_SetTexturing(true);
		Elem_Render(&s->input.base, delta);
		Gfx_SetTexturing(false);
	}
	if (Game_HideGui) return;
	showMinimal = Gui_GetBlocksWorld() != NULL;

	if (!s->showingList && !showMinimal) {
		Gfx_SetTexturing(true);
		HUDScreen_DrawCrosshairs();
		Gfx_SetTexturing(false);
	}
	if (s->grabsInput && !Game_PureClassic) {
		HUDScreen_DrawChatBackground(s);
	}

	Gfx_SetTexturing(true);
	if (!showMinimal) { Elem_Render(&s->hotbar, delta); }
	HUDScreen_DrawChat(s, delta);

	if (s->showingList && IsOnlyHudActive()) {
		s->playerList.active = s->grabsInput;
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
	Event_UnregisterChat(&ChatEvents.ChatReceived,  s, HUDScreen_ChatReceived);
	Event_UnregisterInt(&ChatEvents.ColCodeChanged, s, HUDScreen_ColCodeChanged);
}

static const struct ScreenVTABLE HUDScreen_VTABLE = {
	HUDScreen_Init,      HUDScreen_Render, HUDScreen_Free,
	HUDScreen_KeyDown,   HUDScreen_KeyUp,  HUDScreen_KeyPress,
	HUDScreen_MouseDown, Screen_FMouse,    Screen_FMouseMove,  HUDScreen_MouseScroll,
	HUDScreen_OnResize,  HUDScreen_ContextLost, HUDScreen_ContextRecreated
};
void HUDScreen_Show(void) {
	struct HUDScreen* s = &HUDScreen_Instance;
	s->wasShowingList     = false;
	s->inputOldHeight     = -1;
	s->lastDownloadStatus = Int32_MinValue;

	s->VTABLE = &HUDScreen_VTABLE;
	Gui_HUD   = (struct Screen*)s;
	Gui_Replace((struct Screen*)s, GUI_PRIORITY_HUD);
}

void HUDScreen_OpenInput(const String* text) {
	struct HUDScreen* s  = &HUDScreen_Instance;
	s->suppressNextPress = true;
	s->grabsInput        = true;
	Camera_CheckFocus();

	String_Copy(&s->input.base.text, text);
	InputWidget_UpdateText(&s->input.base);
}

void HUDScreen_AppendInput(const String* text) {
	struct HUDScreen* s = &HUDScreen_Instance;
	InputWidget_AppendString(&s->input.base, text);
}

void HUDScreen_SetChatlines(int lines) {
	struct HUDScreen* s = &HUDScreen_Instance;
	Elem_Free(&s->chat);
	s->chatIndex += s->chat.lines - lines;
	s->chat.lines = lines;
	TextGroupWidget_RedrawAll(&s->chat);
}

struct Widget* HUDScreen_GetHotbar(void) {
	return (struct Widget*)&HUDScreen_Instance.hotbar;
}


/*########################################################################################################################*
*----------------------------------------------------DisconnectScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct DisconnectScreen {
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
} DisconnectScreen_Instance;

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
	Font_Free(&s->titleFont);
	Font_Free(&s->messageFont);
	if (!s->title.VTABLE) return;

	Elem_Free(&s->title);
	Elem_Free(&s->message);
	Elem_Free(&s->reconnect);
}

static void DisconnectScreen_ContextRecreated(void* screen) {
	String msg; char msgBuffer[STRING_SIZE];
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	Drawer2D_MakeFont(&s->titleFont,   16, FONT_STYLE_BOLD);
	Drawer2D_MakeFont(&s->messageFont, 16, FONT_STYLE_NORMAL);

	TextWidget_Set(&s->title,   &s->titleStr,   &s->titleFont);
	TextWidget_Set(&s->message, &s->messageStr, &s->messageFont);

	String_InitArray(msg, msgBuffer);
	DisconnectScreen_ReconnectMessage(s, &msg);
	ButtonWidget_Set(&s->reconnect, &msg, &s->titleFont);
}

static void DisconnectScreen_Init(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	TextWidget_Make(&s->title,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);
	TextWidget_Make(&s->message, ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  10);

	ButtonWidget_Make(&s->reconnect, 300, NULL, 
					ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 80);
	s->reconnect.disabled = !s->canReconnect;

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

static void DisconnectScreen_Free(void* screen) { Game_SetFpsLimit(Game_FpsLimit); }

static void DisconnectScreen_OnResize(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	if (!s->title.VTABLE) return;
	Widget_Reposition(&s->title);
	Widget_Reposition(&s->message);
	Widget_Reposition(&s->reconnect);
}

static bool DisconnectScreen_KeyDown(void* s, Key key) { return key < KEY_F1 || key > KEY_F35; }

static bool DisconnectScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	struct ButtonWidget* w = &s->reconnect;

	if (btn != MOUSE_LEFT) return true;

	if (!w->disabled && Widget_Contains(w, x, y)) {
		Gui_Remove((struct Screen*)s);
		Gui_ShowDefault();
		Server.BeginConnect();
	}
	return true;
}

static bool DisconnectScreen_MouseMove(void* screen, int x, int y) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	struct ButtonWidget* w     = &s->reconnect;

	w->active = !w->disabled && Widget_Contains(w, x, y);
	return true;
}

static const struct ScreenVTABLE DisconnectScreen_VTABLE = {
	DisconnectScreen_Init,      DisconnectScreen_Render, DisconnectScreen_Free,
	DisconnectScreen_KeyDown,   Screen_TKey,             Screen_TKeyPress,
	DisconnectScreen_MouseDown, Screen_TMouse,           DisconnectScreen_MouseMove, Screen_TMouseScroll,
	DisconnectScreen_OnResize,  DisconnectScreen_ContextLost, DisconnectScreen_ContextRecreated
};
void DisconnectScreen_Show(const String* title, const String* message) {
	static const String kick = String_FromConst("Kicked ");
	static const String ban  = String_FromConst("Banned ");
	String why; char whyBuffer[STRING_SIZE];
	struct DisconnectScreen* s = &DisconnectScreen_Instance;

	s->grabsInput  = true;
	s->blocksWorld = true;

	String_InitArray(s->titleStr,   s->_titleBuffer);
	String_AppendString(&s->titleStr,   title);
	String_InitArray(s->messageStr, s->_messageBuffer);
	String_AppendString(&s->messageStr, message);

	String_InitArray(why, whyBuffer);
	String_AppendColorless(&why, message);
	
	s->canReconnect = !(String_CaselessStarts(&why, &kick) || String_CaselessStarts(&why, &ban));
	s->VTABLE       = &DisconnectScreen_VTABLE;

	/* Get rid of all other menus instead of just hiding to reduce GPU usage */
	while (Gui_ScreensCount) Gui_Remove(Gui_Screens[0]);
	Gui_Replace((struct Screen*)s, GUI_PRIORITY_DISCONNECT);
}
