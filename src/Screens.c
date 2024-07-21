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
#include "Input.h"
#include "Utils.h"
#include "Options.h"
#include "InputHandler.h"

#define CHAT_MAX_STATUS Array_Elems(Chat_Status)
#define CHAT_MAX_BOTTOMRIGHT Array_Elems(Chat_BottomRight)
#define CHAT_MAX_CLIENTSTATUS Array_Elems(Chat_ClientStatus)

int Screen_FInput(void* s, int key, struct InputDevice* device) { return false; }
int Screen_FKeyPress(void* s, char keyChar)     { return false; }
int Screen_FText(void* s, const cc_string* str) { return false; }
int Screen_FMouseScroll(void* s, float delta)   { return false; }
int Screen_FPointer(void* s, int id, int x, int y) { return false; }

int Screen_TInput(void* s, int key, struct InputDevice* device) { return true; }
int Screen_TKeyPress(void* s, char keyChar)     { return true; }
int Screen_TText(void* s, const cc_string* str) { return true; }
int Screen_TMouseScroll(void* s, float delta)   { return true; }
int Screen_TPointer(void* s, int id, int x, int y) { return true; }

void Screen_NullFunc(void* screen) { }
void Screen_NullUpdate(void* screen, float delta) { }

/* TODO: Remove these */
struct HUDScreen;
struct ChatScreen;
static struct HUDScreen*  Gui_HUD;
static struct ChatScreen* Gui_Chat;
static cc_bool tablist_active;

static cc_bool InventoryScreen_IsHotbarActive(void);
CC_NOINLINE static cc_bool IsOnlyChatActive(void) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		if (s->grabsInput && s != (struct Screen*)Gui_Chat) return false;
	}
	return true;
}


/*########################################################################################################################*
*--------------------------------------------------------HUDScreen--------------------------------------------------------*
*#########################################################################################################################*/
static struct HUDScreen {
	Screen_Body
	struct FontDesc font;
	struct TextWidget line1, line2;
	struct TextAtlas posAtlas;
	float accumulator;
	int frames, posCount;
	cc_bool hacksChanged;
	float lastSpeed;
	int lastFov;
	int lastX, lastY, lastZ;
	struct HotbarWidget hotbar;
} HUDScreen_Instance;

/* Each integer can be at most 10 digits + minus prefix */
#define POSITION_VAL_CHARS 11
/* [PREFIX] [(] [X] [,] [Y] [,] [Z] [)] */
#define POSITION_HUD_CHARS (1 + 1 + POSITION_VAL_CHARS + 1 + POSITION_VAL_CHARS + 1 + POSITION_VAL_CHARS + 1)
#define HUD_MAX_VERTICES (4 + TEXTWIDGET_MAX * 2 + HOTBAR_MAX_VERTICES + POSITION_HUD_CHARS * 4)

static void HUDScreen_RemakeLine1(struct HUDScreen* s) {
	cc_string status; char statusBuffer[STRING_SIZE * 2];
	int indices, ping, fps;
	float real_fps;

	String_InitArray(status, statusBuffer);
	/* Don't remake texture when FPS isn't being shown */
	if (!Gui.ShowFPS && s->line1.tex.ID) return;
	fps = s->accumulator == 0 ? 1 : (int)(s->frames / s->accumulator);

	if (Gfx.ReducedPerfMode || (Gfx.ReducedPerfModeCooldown > 0)) {
		String_AppendConst(&status, "(low perf mode), ");
		Gfx.ReducedPerfModeCooldown--;
	} else if (fps == 0) {
		/* Running at less than 1 FPS.. */
		real_fps = s->frames / s->accumulator;
		String_Format1(&status, "%f1 fps, ", &real_fps);
	} else {
		String_Format1(&status, "%i fps, ", &fps);
	}

	if (Game_ClassicMode) {
		String_Format1(&status, "%i chunk updates", &Game.ChunkUpdates);
	} else {
		if (Game.ChunkUpdates) {
			String_Format1(&status, "%i chunks/s, ", &Game.ChunkUpdates);
		}

		indices = ICOUNT(Game_Vertices);
		String_Format1(&status, "%i vertices", &indices);

		ping = Ping_AveragePingMS();
		if (ping) String_Format1(&status, ", ping %i ms", &ping);
	}
	TextWidget_Set(&s->line1, &status, &s->font);
	s->dirty = true;
}

static void HUDScreen_BuildPosition(struct HUDScreen* s, struct VertexTextured* data) {
	struct VertexTextured* cur = data;
	struct TextAtlas* atlas = &s->posAtlas;
	struct Texture tex;
	IVec3 pos;

	/* Make "Position: " prefix */
	tex = atlas->tex; 
	tex.x     = 2 + DisplayInfo.ContentOffsetX;
	tex.width = atlas->offset;
	Gfx_Make2DQuad(&tex, PACKEDCOL_WHITE, &cur);

	IVec3_Floor(&pos, &Entities.CurPlayer->Base.Position);
	atlas->curX = tex.x + tex.width;

	/* Make (X, Y, Z) suffix */
	TextAtlas_Add(atlas,       13, &cur);
	TextAtlas_AddInt(atlas, pos.x, &cur);
	TextAtlas_Add(atlas,       11, &cur);
	TextAtlas_AddInt(atlas, pos.y, &cur);
	TextAtlas_Add(atlas,       11, &cur);
	TextAtlas_AddInt(atlas, pos.z, &cur);
	TextAtlas_Add(atlas,       14, &cur);

	s->lastX = pos.x;
	s->lastY = pos.y;
	s->lastZ = pos.z;

	s->posCount = (int)(cur - data);
}

static cc_bool HUDScreen_HasHacksChanged(struct HUDScreen* s) {
	struct HacksComp* hacks = &Entities.CurPlayer->Hacks;
	float speed = HacksComp_CalcSpeedFactor(hacks, hacks->CanSpeed);
	return speed != s->lastSpeed || Camera.Fov != s->lastFov || s->hacksChanged;
}

static void HUDScreen_RemakeLine2(struct HUDScreen* s) {
	cc_string status; char statusBuffer[STRING_SIZE * 2];
	struct HacksComp* hacks = &Entities.CurPlayer->Hacks;
	float speed;
	s->dirty = true;

	if (Game_ClassicMode) {
		TextWidget_SetConst(&s->line2, Game_Version.Name, &s->font);
		return;
	}

	speed = HacksComp_CalcSpeedFactor(hacks, hacks->CanSpeed);
	s->lastSpeed = speed; s->lastFov = Camera.Fov;
	s->hacksChanged = false;

	String_InitArray(status, statusBuffer);
	if (Camera.Fov != Camera.DefaultFov) {
		String_Format1(&status, "Zoom fov %i  ", &Camera.Fov);
	}

	if (hacks->Flying) String_AppendConst(&status, "Fly ON   ");
	if (speed)         String_Format1(&status, "Speed %f1x   ", &speed);
	if (hacks->Noclip) String_AppendConst(&status, "Noclip ON   ");

	TextWidget_Set(&s->line2, &status, &s->font);
}


static void HUDScreen_ContextLost(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	Font_Free(&s->font);
	Screen_ContextLost(screen);

	TextAtlas_Free(&s->posAtlas);
	Elem_Free(&s->hotbar);
	Elem_Free(&s->line1);
	Elem_Free(&s->line2);
}

static void HUDScreen_ContextRecreated(void* screen) {	
	static const cc_string chars  = String_FromConst("0123456789-, ()");
	static const cc_string prefix = String_FromConst("Position: ");

	struct HUDScreen* s = (struct HUDScreen*)screen;
	Screen_UpdateVb(s);

	Font_Make(&s->font, 16, FONT_FLAGS_PADDING);
	Font_SetPadding(&s->font, 2);
	HotbarWidget_SetFont(&s->hotbar, &s->font);

	HUDScreen_RemakeLine1(s);
	TextAtlas_Make(&s->posAtlas, &chars, &s->font, &prefix);
	HUDScreen_RemakeLine2(s);
}

int HUDScreen_LayoutHotbar(void) {
	struct HUDScreen* s = &HUDScreen_Instance;
	s->hotbar.scale     = Gui_GetHotbarScale();
	Widget_Layout(&s->hotbar);
	return s->hotbar.height;
}

static void HUDScreen_Layout(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	struct TextWidget* line1 = &s->line1;
	struct TextWidget* line2 = &s->line2;
	int posY;

	Widget_SetLocation(line1, ANCHOR_MIN, ANCHOR_MIN, 
						2 + DisplayInfo.ContentOffsetX, 2 + DisplayInfo.ContentOffsetY);
	posY = line1->y + line1->height;
	s->posAtlas.tex.y = posY;
	Widget_SetLocation(line2, ANCHOR_MIN, ANCHOR_MIN, 
						2 + DisplayInfo.ContentOffsetX, 0);

	if (Game_ClassicMode) {
		/* Swap around so 0.30 version is at top */
		line2->yOffset = line1->yOffset;
		line1->yOffset = posY;
		Widget_Layout(line1);
	} else {
		/* We can't use y in TextWidget_Make because that DPI scales it */
		line2->yOffset = posY + s->posAtlas.tex.height;
	}

	HUDScreen_LayoutHotbar();
	Widget_Layout(line2);
}

static int HUDScreen_KeyDown(void* screen, int key, struct InputDevice* device) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	return Elem_HandlesKeyDown(&s->hotbar, key, device);
}

static void HUDScreen_InputUp(void* screen, int key, struct InputDevice* device) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	if (!InventoryScreen_IsHotbarActive()) return;
	Elem_OnInputUp(&s->hotbar, key, device);
}

static int HUDscreen_PointerDown(void* screen, int id, int x, int y) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	if (Gui_TouchUI || Gui.InputGrab) {
		return Elem_HandlesPointerDown(&s->hotbar, id, x, y);
	}
	return false;
}

static void HUDScreen_PointerUp(void *screen, int id, int x, int y) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	if (!Gui_TouchUI) return;
	Elem_OnPointerUp(&s->hotbar, id, x, y);
}

static int HUDScreen_PointerMove(void *screen, int id, int x, int y) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	if (!Gui_TouchUI) return false;
	return Elem_HandlesPointerMove(&s->hotbar, id, x, y);
}

static int HUDscreen_MouseScroll(void* screen, float delta) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	/* The default scrolling behaviour (e.g. camera, zoom) needs to be checked */
	/*   BEFORE the hotbar is scrolled, but AFTER chat (maybe) handles scrolling. */
	/* Therefore need to check the default behaviour here, hacky as that may be. */
	if (Input_HandleMouseWheel(delta)) return false;

	if (!Inventory.CanChangeSelected)  return false;
	return Elem_HandlesMouseScroll(&s->hotbar, delta);
}

static void HUDScreen_HacksChanged(void* obj) {
	((struct HUDScreen*)obj)->hacksChanged = true;
}

static void HUDScreen_NeedRedrawing(void* obj) {
	((struct HUDScreen*)obj)->dirty = true;
}

static void HUDScreen_Init(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	s->maxVertices      = HUD_MAX_VERTICES;

	HotbarWidget_Create(&s->hotbar);
	TextWidget_Init(&s->line1);
	TextWidget_Init(&s->line2);
	
	s->line1.flags  |= WIDGET_FLAG_MAINSCREEN;
	s->line2.flags  |= WIDGET_FLAG_MAINSCREEN;

	Event_Register_(&UserEvents.HacksStateChanged, s, HUDScreen_HacksChanged);
	Event_Register_(&TextureEvents.AtlasChanged,   s, HUDScreen_NeedRedrawing);
	Event_Register_(&BlockEvents.BlockDefChanged,  s, HUDScreen_NeedRedrawing);
}

static void HUDScreen_Free(void* screen) {
	Event_Unregister_(&UserEvents.HacksStateChanged, screen, HUDScreen_HacksChanged);
	Event_Unregister_(&TextureEvents.AtlasChanged,   screen, HUDScreen_NeedRedrawing);
	Event_Unregister_(&BlockEvents.BlockDefChanged,  screen, HUDScreen_NeedRedrawing);
}

static void HUDScreen_UpdateFPS(struct HUDScreen* s, float delta) {
	s->frames++;
	s->accumulator += delta;
	if (s->accumulator < 1.0f) return;

	HUDScreen_RemakeLine1(s);
	s->accumulator    = 0.0f;
	s->frames         = 0;
	Game.ChunkUpdates = 0;
}

static void HUDScreen_Update(void* screen, float delta) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	IVec3 pos;

	HUDScreen_UpdateFPS(s,          delta);
	HotbarWidget_Update(&s->hotbar, delta);
	if (Game_ClassicMode) return;

	if (IsOnlyChatActive() && Gui.ShowFPS) {
		if (HUDScreen_HasHacksChanged(s)) HUDScreen_RemakeLine2(s);
	}

	IVec3_Floor(&pos, &Entities.CurPlayer->Base.Position);
	if (pos.x != s->lastX || pos.y != s->lastY || pos.z != s->lastZ) {
		s->dirty = true;
	}
}

#define CH_EXTENT 16
static void HUDScreen_BuildCrosshairsMesh(struct VertexTextured** ptr) {
	/* Only top quarter of icons.png is used */
	static struct Texture tex = { 0, Tex_Rect(0,0,0,0), Tex_UV(0.0f,0.0f, 15/256.0f,15/64.0f) };
	int extent;

	extent = (int)(CH_EXTENT * Gui_GetCrosshairScale());
	tex.x  = (Window_Main.Width  / 2) - extent;
	tex.y  = (Window_Main.Height / 2) - extent;

	tex.width  = extent * 2;
	tex.height = extent * 2;
	Gfx_Make2DQuad(&tex, PACKEDCOL_WHITE, ptr);
}

static void HUDScreen_BuildMesh(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	struct VertexTextured* data;
	struct VertexTextured** ptr;

	data = Screen_LockVb(s);
	ptr  = &data;

	HUDScreen_BuildCrosshairsMesh(ptr);
	Widget_BuildMesh(&s->line1,  ptr);
	Widget_BuildMesh(&s->line2,  ptr);
	Widget_BuildMesh(&s->hotbar, ptr);

	if (!Game_ClassicMode) 
		HUDScreen_BuildPosition(s, data);
	Gfx_UnlockDynamicVb(s->vb);
}

static void HUDScreen_Render(void* screen, float delta) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	if (Game_HideGui) return;

	Gfx_3DS_SetRenderScreen(TOP_SCREEN);

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindDynamicVb(s->vb);
	if (Gui.ShowFPS) Widget_Render2(&s->line1, 4);

	if (Game_ClassicMode) {
		Widget_Render2(&s->line2, 8);
	} else if (IsOnlyChatActive() && Gui.ShowFPS) {
		Widget_Render2(&s->line2, 8);
		Gfx_BindTexture(s->posAtlas.tex.ID);
		Gfx_DrawVb_IndexedTris_Range(s->posCount, 12 + HOTBAR_MAX_VERTICES);
		/* TODO swap these two lines back */
	}

	if (!Gui_GetBlocksWorld()) {
		Gfx_BindDynamicVb(s->vb);
		Widget_Render2(&s->hotbar, 12);

		if (Gui.IconsTex && !tablist_active) {
			Gfx_BindTexture(Gui.IconsTex);
			Gfx_BindDynamicVb(s->vb); /* Have to rebind for mobile right now... */
			Gfx_DrawVb_IndexedTris(4);
		}
	}

	Gfx_3DS_SetRenderScreen(BOTTOM_SCREEN);
}

static const struct ScreenVTABLE HUDScreen_VTABLE = {
	HUDScreen_Init,        HUDScreen_Update,    HUDScreen_Free,
	HUDScreen_Render,      HUDScreen_BuildMesh,
	HUDScreen_KeyDown,     HUDScreen_InputUp,   Screen_FKeyPress, Screen_FText,
	HUDscreen_PointerDown, HUDScreen_PointerUp, HUDScreen_PointerMove,  HUDscreen_MouseScroll,
	HUDScreen_Layout,      HUDScreen_ContextLost, HUDScreen_ContextRecreated
};
void HUDScreen_Show(void) {
	struct HUDScreen* s = &HUDScreen_Instance;
	s->VTABLE = &HUDScreen_VTABLE;
	Gui_HUD   = s;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_HUD);
}


/*########################################################################################################################*
*----------------------------------------------------TabListOverlay-----------------------------------------------------*
*#########################################################################################################################*/
#define GROUP_NAME_ID UInt16_MaxValue
#define LIST_COLUMN_PADDING 5
#define LIST_NAMES_PER_COLUMN 16
#define TABLIST_MAX_ENTRIES (TABLIST_MAX_NAMES * 2)
typedef int (*TabListEntryCompare)(int x, int y);

static struct TabListOverlay {
	Screen_Body
	int x, y, width, height;
	cc_bool classic, staysOpen;
	int usedCount, elementOffset;
	struct TextWidget title;
	struct FontDesc font;
	TabListEntryCompare compare;
	cc_uint16 ids[TABLIST_MAX_ENTRIES];
	struct Texture textures[TABLIST_MAX_ENTRIES];
} TabListOverlay_Instance;
#define TABLIST_MAX_VERTICES (TEXTWIDGET_MAX + 4 * TABLIST_MAX_ENTRIES)

static void TabListOverlay_DrawText(struct Texture* tex, struct TabListOverlay* s, const cc_string* name) {
	cc_string tmp; char tmpBuffer[STRING_SIZE];
	struct DrawTextArgs args;

	if (Game_PureClassic) {
		String_InitArray(tmp, tmpBuffer);
		String_AppendColorless(&tmp, name);
	} else {
		tmp = *name;
	}

	DrawTextArgs_Make(&args, &tmp, &s->font, !s->classic);
	Drawer2D_MakeTextTexture(tex, &args);
}

static int TabListOverlay_GetColumnWidth(struct TabListOverlay* s, int column) {
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(s->usedCount, i + LIST_NAMES_PER_COLUMN);
	int maxWidth = 0;

	for (; i < end; i++) 
	{
		maxWidth = max(maxWidth, s->textures[i].width);
	}
	return maxWidth + LIST_COLUMN_PADDING + s->elementOffset;
}

static int TabListOverlay_GetColumnHeight(struct TabListOverlay* s, int column) {
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(s->usedCount, i + LIST_NAMES_PER_COLUMN);
	int height = 0;

	for (; i < end; i++) 
	{
		height += s->textures[i].height + 1;
	}
	return height;
}

static void TabListOverlay_SetColumnPos(struct TabListOverlay* s, int column, int x, int y) {
	struct Texture tex;
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(s->usedCount, i + LIST_NAMES_PER_COLUMN);

	for (; i < end; i++) 
	{
		tex = s->textures[i];
		tex.x = x; tex.y = y - 10;

		y += tex.height + 1;
		/* offset player names a bit, compared to group name */
		if (!s->classic && s->ids[i] != GROUP_NAME_ID) {
			tex.x += s->elementOffset;
		}
		s->textures[i] = tex;
	}
}

static void TabListOverlay_Layout(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	int minWidth, minHeight, paddingX, paddingY;
	int i, x, y, width = 0, height = 0;
	int columns = Math_CeilDiv(s->usedCount, LIST_NAMES_PER_COLUMN);

	for (i = 0; i < columns; i++) 
	{
		width += TabListOverlay_GetColumnWidth(s,  i);
		y      = TabListOverlay_GetColumnHeight(s, i);
		height = max(height, y);
	}

	minWidth = Display_ScaleX(480);
	width    = max(width, minWidth);
	paddingX = Display_ScaleX(10);
	paddingY = Display_ScaleY(10);

	width  += paddingX * 2;
	height += paddingY * 2;

	y    = Window_UI.Height / 4 - height / 2;
	s->x = Gui_CalcPos(ANCHOR_CENTRE,          0, width , Window_UI.Width );
	s->y = Gui_CalcPos(ANCHOR_CENTRE, -max(0, y), height, Window_UI.Height);

	x = s->x + paddingX;
	y = s->y + paddingY;

	for (i = 0; i < columns; i++) 
	{
		TabListOverlay_SetColumnPos(s, i, x, y);
		x += TabListOverlay_GetColumnWidth(s, i);
	}

	s->y -= (s->title.height + paddingY);
	s->width  = width;
	minHeight = Display_ScaleY(300);
	s->height = max(minHeight, height + s->title.height);

	s->title.horAnchor = ANCHOR_CENTRE;
	s->title.yOffset   = s->y + paddingY / 2;
	Widget_Layout(&s->title);
}

static void TabListOverlay_AddName(struct TabListOverlay* s, EntityID id, int index) {
	cc_string name;
	/* insert at end of list */
	if (index == -1) { index = s->usedCount; s->usedCount++; }

	name = TabList_UNSAFE_GetList(id);
	s->ids[index] = id;
	TabListOverlay_DrawText(&s->textures[index], s, &name);
}

static void TabListOverlay_DeleteAt(struct TabListOverlay* s, int i) {
	Gfx_DeleteTexture(&s->textures[i].ID);

	for (; i < s->usedCount - 1; i++)
	{
		s->ids[i]      = s->ids[i + 1];
		s->textures[i] = s->textures[i + 1];
	}

	s->usedCount--;
	s->ids[s->usedCount]         = 0;
	s->textures[s->usedCount].ID = 0;
}

static void TabListOverlay_AddGroup(struct TabListOverlay* s, int id, int* index) {
	cc_string group;
	int i;
	group = TabList_UNSAFE_GetGroup(id);

	for (i = Array_Elems(s->ids) - 1; i > (*index); i--) 
	{
		s->ids[i]      = s->ids[i - 1];
		s->textures[i] = s->textures[i - 1];
	}
	
	s->ids[*index] = GROUP_NAME_ID;
	s->textures[*index].ID = 0; /* TODO: TEMP HACK! */
	TabListOverlay_DrawText(&s->textures[*index], s, &group);

	(*index)++;
	s->usedCount++;
}

static int TabListOverlay_GetGroupCount(struct TabListOverlay* s, int id, int i) {
	cc_string group, curGroup;
	int count;
	group = TabList_UNSAFE_GetGroup(id);

	for (count = 0; i < s->usedCount; i++, count++)
	{
		curGroup = TabList_UNSAFE_GetGroup(s->ids[i]);
		if (!String_CaselessEquals(&group, &curGroup)) break;
	}
	return count;
}

static int TabListOverlay_PlayerCompare(int x, int y) {
	cc_string xName; char xNameBuffer[STRING_SIZE];
	cc_string yName; char yNameBuffer[STRING_SIZE];
	cc_uint8 xRank, yRank;
	cc_string xNameRaw, yNameRaw;

	xRank = TabList.GroupRanks[x];
	yRank = TabList.GroupRanks[y];
	if (xRank != yRank) return (xRank < yRank ? -1 : 1);
	
	String_InitArray(xName, xNameBuffer);
	xNameRaw = TabList_UNSAFE_GetList(x);
	String_AppendColorless(&xName, &xNameRaw);

	String_InitArray(yName, yNameBuffer);
	yNameRaw = TabList_UNSAFE_GetList(y);
	String_AppendColorless(&yName, &yNameRaw);

	return String_Compare(&xName, &yName);
}

static int TabListOverlay_GroupCompare(int x, int y) {
	cc_string xGroup, yGroup;
	/* TODO: should we use colourless comparison? ClassicalSharp sorts groups with colours */
	xGroup = TabList_UNSAFE_GetGroup(x);
	yGroup = TabList_UNSAFE_GetGroup(y);
	return String_Compare(&xGroup, &yGroup);
}

static void TabListOverlay_QuickSort(int left, int right) {
	struct Texture* values = TabListOverlay_Instance.textures; struct Texture value;
	cc_uint16* keys        = TabListOverlay_Instance.ids; cc_uint16 key;
	TabListEntryCompare compareEntries = TabListOverlay_Instance.compare;

	while (left < right) {
		int i = left, j = right;
		int pivot = keys[(i + j) / 2];

		/* partition the list */
		while (i <= j) {
			while (compareEntries(pivot, keys[i]) > 0) i++;
			while (compareEntries(pivot, keys[j]) < 0) j--;
			QuickSort_Swap_KV_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(TabListOverlay_QuickSort)
	}
}

static void TabListOverlay_SortEntries(struct TabListOverlay* s) {
	int i, id, count;
	if (!s->usedCount) return;

	if (s->classic) {
		TabListOverlay_Instance.compare = TabListOverlay_PlayerCompare;
		TabListOverlay_QuickSort(0, s->usedCount - 1);
		return;
	}

	/* Sort the list by group */
	/* Loop backwards, since DeleteAt() reduces NamesCount */
	for (i = s->usedCount - 1; i >= 0; i--)
	{
		if (s->ids[i] != GROUP_NAME_ID) continue;
		TabListOverlay_DeleteAt(s, i);
	}
	TabListOverlay_Instance.compare = TabListOverlay_GroupCompare;
	TabListOverlay_QuickSort(0, s->usedCount - 1);

	/* Sort the entries in each group */
	TabListOverlay_Instance.compare = TabListOverlay_PlayerCompare;
	for (i = 0; i < s->usedCount; )
	{
		id = s->ids[i];
		TabListOverlay_AddGroup(s, id, &i);

		count = TabListOverlay_GetGroupCount(s, id, i);
		TabListOverlay_QuickSort(i, i + (count - 1));
		i += count;
	}
}

static void TabListOverlay_SortAndLayout(struct TabListOverlay* s) {
	TabListOverlay_SortEntries(s);
	TabListOverlay_Layout(s);
	s->dirty = true;
}

static void TabListOverlay_Add(void* obj, int id) {
	struct TabListOverlay* s = (struct TabListOverlay*)obj;
	TabListOverlay_AddName(s, id, -1);
	TabListOverlay_SortAndLayout(s);
}

static void TabListOverlay_Update(void* obj, int id) {
	struct TabListOverlay* s = (struct TabListOverlay*)obj;
	int i;
	for (i = 0; i < s->usedCount; i++)
	{
		if (s->ids[i] != id) continue;
		Gfx_DeleteTexture(&s->textures[i].ID);

		TabListOverlay_AddName(s, id, i);
		TabListOverlay_SortAndLayout(s);
		return;
	}
}

static void TabListOverlay_Remove(void* obj, int id) {
	struct TabListOverlay* s = (struct TabListOverlay*)obj;
	int i;
	for (i = 0; i < s->usedCount; i++)
	{
		if (s->ids[i] != id) continue;

		TabListOverlay_DeleteAt(s, i);
		TabListOverlay_SortAndLayout(s);
		return;
	}
}

static int TabListOverlay_PointerDown(void* screen, int id, int x, int y) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	cc_string text; char textBuffer[STRING_SIZE * 4];
	struct Texture tex;
	cc_string player;
	int i;

	if (!((struct Screen*)Gui_Chat)->grabsInput) return false;
	String_InitArray(text, textBuffer);

	for (i = 0; i < s->usedCount; i++)
	{
		if (!s->textures[i].ID || s->ids[i] == GROUP_NAME_ID) continue;
		tex = s->textures[i];
		if (!Gui_Contains(tex.x, tex.y, tex.width, tex.height, x, y)) continue;

		player = TabList_UNSAFE_GetPlayer(s->ids[i]);
		String_Format1(&text, "%s ", &player);
		ChatScreen_AppendInput(&text);
		return TOUCH_TYPE_GUI;
	}
	return false;
}

static void TabListOverlay_KeyUp(void* screen, int key, struct InputDevice* device) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	if (!InputBind_Claims(BIND_TABLIST, key, device) || s->staysOpen) return;
	Gui_Remove((struct Screen*)s);
}

static void TabListOverlay_ContextLost(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	int i;
	for (i = 0; i < s->usedCount; i++)
	{
		Gfx_DeleteTexture(&s->textures[i].ID);
	}

	Elem_Free(&s->title);
	Font_Free(&s->font);
	Screen_ContextLost(screen);
}

static void TabListOverlay_ContextRecreated(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	int size, id;

	size = Drawer2D.BitmappedText ? 16 : 11;
	Font_Make(&s->font, size, FONT_FLAGS_PADDING);
	s->usedCount = 0;

	TextWidget_SetConst(&s->title, "Connected players:", &s->font);
	Font_SetPadding(&s->font, 1);
	Screen_UpdateVb(screen);

	/* TODO: Just recreate instead of this? maybe */
	for (id = 0; id < TABLIST_MAX_NAMES; id++) 
	{
		if (!TabList.NameOffsets[id]) continue;
		TabListOverlay_AddName(s, (EntityID)id, -1);
	}
	TabListOverlay_SortAndLayout(s); /* TODO: Not do layout here too */
}

static void TabListOverlay_BuildMesh(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	struct Screen*   grabbed = Gui_GetInputGrab();
	struct VertexTextured* v;
	struct Texture tex;
	int i;
	
	v = (struct VertexTextured*)Gfx_LockDynamicVb(s->vb,
										VERTEX_FORMAT_TEXTURED, TEXTWIDGET_MAX + s->usedCount * 4);
	Widget_BuildMesh(&s->title, &v);

	for (i = 0; i < s->usedCount; i++)
	{
		if (!s->textures[i].ID) continue;
		tex = s->textures[i];

		if (grabbed && s->ids[i] != GROUP_NAME_ID) {
			if (Gui_ContainsPointers(tex.x, tex.y, tex.width, tex.height)) tex.x += 4;
		}
		Gfx_Make2DQuad(&tex, PACKEDCOL_WHITE, &v);
	}
	Gfx_UnlockDynamicVb(s->vb);
}

static void TabListOverlay_Render(void* screen, float delta) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	int i, offset = 0;
	PackedCol topCol    = PackedCol_Make( 0,  0,  0, 180);
	PackedCol bottomCol = PackedCol_Make(50, 50, 50, 205);

	if (Game_HideGui || !IsOnlyChatActive()) return;

	Gfx_3DS_SetRenderScreen(TOP_SCREEN);

	Gfx_Draw2DGradient(s->x, s->y, s->width, s->height, topCol, bottomCol);

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindDynamicVb(s->vb);
	offset = Widget_Render2(&s->title, offset);

	for (i = 0; i < s->usedCount; i++)
	{
		if (!s->textures[i].ID) continue;
		Gfx_BindTexture(s->textures[i].ID);

		Gfx_DrawVb_IndexedTris_Range(4, offset);
		offset += 4;
	}

	Gfx_3DS_SetRenderScreen(BOTTOM_SCREEN);
}

static void TabListOverlay_Free(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	tablist_active = false;
	Event_Unregister_(&TabListEvents.Added,   s, TabListOverlay_Add);
	Event_Unregister_(&TabListEvents.Changed, s, TabListOverlay_Update);
	Event_Unregister_(&TabListEvents.Removed, s, TabListOverlay_Remove);
}

static void TabListOverlay_Init(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	tablist_active   = true;
	s->classic       = Gui.ClassicTabList || !Server.SupportsExtPlayerList;
	s->elementOffset = s->classic ? 0 : 10;
	s->maxVertices   = TABLIST_MAX_VERTICES;
	TextWidget_Init(&s->title);

	Event_Register_(&TabListEvents.Added,   s, TabListOverlay_Add);
	Event_Register_(&TabListEvents.Changed, s, TabListOverlay_Update);
	Event_Register_(&TabListEvents.Removed, s, TabListOverlay_Remove);
}

static const struct ScreenVTABLE TabListOverlay_VTABLE = {
	TabListOverlay_Init,        Screen_NullUpdate,     TabListOverlay_Free,
	TabListOverlay_Render,      TabListOverlay_BuildMesh,
	Screen_FInput,              TabListOverlay_KeyUp,  Screen_FKeyPress, Screen_FText,
	TabListOverlay_PointerDown, Screen_PointerUp,      Screen_FPointer,  Screen_FMouseScroll,
	TabListOverlay_Layout, TabListOverlay_ContextLost, TabListOverlay_ContextRecreated
};
void TabListOverlay_Show(cc_bool staysOpen) {
	struct TabListOverlay* s  = &TabListOverlay_Instance;
	s->VTABLE    = &TabListOverlay_VTABLE;
	s->staysOpen = staysOpen;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_TABLIST);
}


/*########################################################################################################################*
*--------------------------------------------------------ChatScreen-------------------------------------------------------*
*#########################################################################################################################*/
static struct ChatScreen {
	Screen_Body
	float chatAcc;
	cc_bool suppressNextPress;
	int chatIndex, paddingX, paddingY;
	int lastDownloadStatus;
	struct FontDesc chatFont, announcementFont, bigAnnouncementFont, smallAnnouncementFont;
	struct TextWidget announcement, bigAnnouncement, smallAnnouncement;
	struct ChatInputWidget input;
	struct TextGroupWidget status, bottomRight, chat, clientStatus;
	struct SpecialInputWidget altText;
#ifdef CC_BUILD_TOUCH
	struct ButtonWidget send, cancel, more;
#endif

	struct Texture statusTextures[CHAT_MAX_STATUS];
	struct Texture bottomRightTextures[CHAT_MAX_BOTTOMRIGHT];
	struct Texture clientStatusTextures[CHAT_MAX_CLIENTSTATUS];
	struct Texture chatTextures[GUI_MAX_CHATLINES];
} ChatScreen_Instance;

static void ChatScreen_UpdateChatYOffsets(struct ChatScreen* s) {
	int pad, y;
	/* Determining chat Y requires us to know hotbar's position */
	HUDScreen_LayoutHotbar();
		
	y = min(s->input.base.y, Gui_HUD->hotbar.y);
	y -= s->input.base.yOffset; /* add some padding */
	s->altText.yOffset = Window_UI.Height - y;
	Widget_Layout(&s->altText);

	pad = s->altText.active ? 5 : 10;
	s->clientStatus.yOffset = Window_UI.Height - s->altText.y + pad;
	Widget_Layout(&s->clientStatus);
	s->chat.yOffset = s->clientStatus.yOffset + s->clientStatus.height;
	Widget_Layout(&s->chat);
}

static void ChatScreen_OnInputTextChanged(void* elem) {
	ChatScreen_UpdateChatYOffsets(&ChatScreen_Instance);
}

static cc_string ChatScreen_GetChat(int i) {
	i += ChatScreen_Instance.chatIndex;

	if (i >= 0 && i < Chat_Log.count) {
		return StringsBuffer_UNSAFE_Get(&Chat_Log, i);
	}
	return String_Empty;
}

static cc_string ChatScreen_GetStatus(int i)       { return Chat_Status[i]; }
static cc_string ChatScreen_GetBottomRight(int i)  { return Chat_BottomRight[2 - i]; }
static cc_string ChatScreen_GetClientStatus(int i) { return Chat_ClientStatus[i]; }

static void ChatScreen_FreeChatFonts(struct ChatScreen* s) {
	Font_Free(&s->chatFont);
	Font_Free(&s->announcementFont);
	Font_Free(&s->bigAnnouncementFont);
	Font_Free(&s->smallAnnouncementFont);
}

static cc_bool ChatScreen_ChatUpdateFont(struct ChatScreen* s) {
	int size = (int)(8  * Gui_GetChatScale());
	Math_Clamp(size, 8, 64);

	/* don't recreate font if possible */
	/* TODO: Add function for this, don't use Display_ScaleY (Drawer2D_SameFontSize ??) */
	if (Display_ScaleY(size) == s->chatFont.size) return false;
	ChatScreen_FreeChatFonts(s);
	Font_Make(&s->chatFont, size, FONT_FLAGS_PADDING);

	size = (int)(16 * Gui_GetChatScale());
	Math_Clamp(size, 8, 64);
	Font_Make(&s->announcementFont, size, FONT_FLAGS_NONE);
	size = (int)(24 * Gui_GetChatScale());
	Math_Clamp(size, 8, 64);
	Font_Make(&s->bigAnnouncementFont, size, FONT_FLAGS_NONE);
	size = (int)(8 * Gui_GetChatScale());
	Math_Clamp(size, 8, 64);
	Font_Make(&s->smallAnnouncementFont, size, FONT_FLAGS_NONE);

	ChatInputWidget_SetFont(&s->input,        &s->chatFont);
	TextGroupWidget_SetFont(&s->status,       &s->chatFont);
	TextGroupWidget_SetFont(&s->bottomRight,  &s->chatFont);
	TextGroupWidget_SetFont(&s->chat,         &s->chatFont);
	TextGroupWidget_SetFont(&s->clientStatus, &s->chatFont);
	return true;
}

static void ChatScreen_Redraw(struct ChatScreen* s) {
	TextGroupWidget_RedrawAll(&s->chat);
	TextWidget_Set(&s->announcement, &Chat_Announcement, &s->announcementFont);
	TextWidget_Set(&s->bigAnnouncement, &Chat_BigAnnouncement, &s->bigAnnouncementFont);
	TextWidget_Set(&s->smallAnnouncement, &Chat_SmallAnnouncement, &s->smallAnnouncementFont);
	TextGroupWidget_RedrawAll(&s->status);
	TextGroupWidget_RedrawAll(&s->bottomRight);
	TextGroupWidget_RedrawAll(&s->clientStatus);

	if (s->grabsInput) InputWidget_UpdateText(&s->input.base);
	SpecialInputWidget_Redraw(&s->altText);
}

static int ChatScreen_ClampChatIndex(int index) {
	int maxIndex = Chat_Log.count - Gui.Chatlines;
	int minIndex = min(0, maxIndex);
	Math_Clamp(index, minIndex, maxIndex);
	return index;
}

static void ChatScreen_ScrollChatBy(struct ChatScreen* s, int delta) {
	int newIndex = ChatScreen_ClampChatIndex(s->chatIndex + delta);
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

static void ChatScreen_EnterChatInput(struct ChatScreen* s, cc_bool close) {
	struct InputWidget* input;
	int defaultIndex;

	s->grabsInput = false;
	Gui_UpdateInputGrab();
	OnscreenKeyboard_Close();
	if (close) InputWidget_Clear(&s->input.base);

	input = &s->input.base;
	input->OnPressedEnter(input);
	SpecialInputWidget_SetActive(&s->altText, false);
	ChatScreen_UpdateChatYOffsets(s);

	/* Reset chat when user has scrolled up in chat history */
	defaultIndex = Chat_Log.count - Gui.Chatlines;
	if (s->chatIndex != defaultIndex) {
		s->chatIndex = defaultIndex;
		TextGroupWidget_RedrawAll(&s->chat);
	}
}

static void ChatScreen_UpdateTexpackStatus(struct ChatScreen* s) {
	int progress = Http_CheckProgress(TexturePack_ReqID);
	cc_string msg; char msgBuffer[STRING_SIZE];
	if (progress == s->lastDownloadStatus) return;

	s->lastDownloadStatus = progress;
	String_InitArray(msg, msgBuffer);

	if (progress == HTTP_PROGRESS_MAKING_REQUEST) {
		String_AppendConst(&msg, "&eRetrieving texture pack..");
	} else if (progress == HTTP_PROGRESS_FETCHING_DATA) {
		String_AppendConst(&msg, "&eDownloading texture pack");
	} else if (progress >= 0 && progress <= 100) {
		String_Format1(&msg, "&eDownloading texture pack (&7%i&e%%)", &progress);
	}
	Chat_AddOf(&msg, MSG_TYPE_EXTRASTATUS_1);
}

static void ChatScreen_ColCodeChanged(void* screen, int code) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	float caretAcc;
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

static void ChatScreen_ChatReceived(void* screen, const cc_string* msg, int type) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (Gfx.LostContext) return;
	s->dirty = true;

	if (type == MSG_TYPE_NORMAL) {
		s->chatIndex++;
		if (!Gui.Chatlines) return;
		TextGroupWidget_ShiftUp(&s->chat);
	} else if (type >= MSG_TYPE_STATUS_1 && type <= MSG_TYPE_STATUS_3) {
		/* Status[0] is for texture pack downloading message */
		/* Status[1] is for reduced performance mode message */
		TextGroupWidget_Redraw(&s->status, 2 + (type - MSG_TYPE_STATUS_1));
	} else if (type >= MSG_TYPE_BOTTOMRIGHT_1 && type <= MSG_TYPE_BOTTOMRIGHT_3) {
		/* Bottom3 is top most line, so need to redraw index 0 */
		TextGroupWidget_Redraw(&s->bottomRight, 2 - (type - MSG_TYPE_BOTTOMRIGHT_1));
	} else if (type == MSG_TYPE_ANNOUNCEMENT) {
		TextWidget_Set(&s->announcement, msg, &s->announcementFont);
	} else if (type == MSG_TYPE_BIGANNOUNCEMENT) {
		TextWidget_Set(&s->bigAnnouncement, msg, &s->bigAnnouncementFont);
	} else if (type == MSG_TYPE_SMALLANNOUNCEMENT) {
		TextWidget_Set(&s->smallAnnouncement, msg, &s->smallAnnouncementFont);
	} else if (type >= MSG_TYPE_CLIENTSTATUS_1 && type <= MSG_TYPE_CLIENTSTATUS_2) {
		TextGroupWidget_Redraw(&s->clientStatus, type - MSG_TYPE_CLIENTSTATUS_1);
		ChatScreen_UpdateChatYOffsets(s);
	} else if (type >= MSG_TYPE_EXTRASTATUS_1 && type <= MSG_TYPE_EXTRASTATUS_2) {
		/* Status[0] is for texture pack downloading message */
		/* Status[1] is for reduced performance mode message */
		TextGroupWidget_Redraw(&s->status, type - MSG_TYPE_EXTRASTATUS_1);
	} 
}


static void ChatScreen_Update(void* screen, float delta) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	double now = Game.Time;

	/* Destroy announcement texture before even rendering it at all, */
	/* otherwise changing texture pack shows announcement for one frame */
	if (s->announcement.tex.ID && now > Chat_AnnouncementReceived + 5) {
		Elem_Free(&s->announcement);
	}

	if (s->bigAnnouncement.tex.ID && now > Chat_BigAnnouncementReceived + 5) {
		Elem_Free(&s->bigAnnouncement);
	}

	if (s->smallAnnouncement.tex.ID && now > Chat_SmallAnnouncementReceived + 5) {
		Elem_Free(&s->smallAnnouncement);
	}
}

static void ChatScreen_DrawChatBackground(struct ChatScreen* s) {
	int usedHeight = TextGroupWidget_UsedHeight(&s->chat);
	int x = s->chat.x;
	int y = s->chat.y + s->chat.height - usedHeight;

	int width  = max(s->clientStatus.width, s->chat.width);
	int height = usedHeight + s->clientStatus.height;

	if (height > 0) {
		PackedCol backCol = PackedCol_Make(0, 0, 0, 127);
		Gfx_Draw2DFlat( x - s->paddingX,          y - s->paddingY, 
					width + s->paddingX * 2, height + s->paddingY * 2, backCol);
	}
}

static void ChatScreen_DrawChat(struct ChatScreen* s, float delta) {
	struct Texture tex;
	double now;
	int i, logIdx;

	ChatScreen_UpdateTexpackStatus(s);
	if (!Game_PureClassic) { Elem_Render(&s->status, delta); }
	Elem_Render(&s->bottomRight, delta);
	Elem_Render(&s->clientStatus, delta);

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindDynamicVb(s->vb);
	now = Game.Time;

	if (s->grabsInput) {
		Widget_Render2(&s->chat, 0);
	} else {
		/* Only render recent chat */
		for (i = 0; i < s->chat.lines; i++) {
			tex    = s->chat.textures[i];
			logIdx = s->chatIndex + i;
			if (!tex.ID) continue;

			if (logIdx < 0 || logIdx >= Chat_Log.count) continue;
			/* Only draw chat within last 10 seconds */
			if (Chat_GetLogTime(logIdx) + 10 < now) continue;
			
			Gfx_BindTexture(tex.ID);
			Gfx_DrawVb_IndexedTris_Range(4, i * 4);
		}
	}

	Elem_Render(&s->announcement, delta);
	Elem_Render(&s->bigAnnouncement, delta);
	Elem_Render(&s->smallAnnouncement, delta);

	if (s->grabsInput) {
		Elem_Render(&s->input.base, delta);
		if (s->altText.active) {
			Elem_Render(&s->altText, delta);
		}

#ifdef CC_BUILD_TOUCH
		if (!Gui.TouchUI) return;
		Gfx_3DS_SetRenderScreen(BOTTOM_SCREEN);
		Elem_Render(&s->more,   delta);
		Elem_Render(&s->send,   delta);
		Elem_Render(&s->cancel, delta);
		Gfx_3DS_SetRenderScreen(TOP_SCREEN);
#endif
	}
}

static void ChatScreen_ContextLost(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	ChatScreen_FreeChatFonts(s);
	Screen_ContextLost(s);

	Elem_Free(&s->chat);
	Elem_Free(&s->input.base);
	Elem_Free(&s->altText);
	Elem_Free(&s->status);
	Elem_Free(&s->bottomRight);
	Elem_Free(&s->clientStatus);
	Elem_Free(&s->announcement);
	Elem_Free(&s->bigAnnouncement);
	Elem_Free(&s->smallAnnouncement);

#ifdef CC_BUILD_TOUCH
	Elem_Free(&s->more);
	Elem_Free(&s->send);
	Elem_Free(&s->cancel);
#endif
}

static void ChatScreen_ContextRecreated(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	struct FontDesc font;
	ChatScreen_ChatUpdateFont(s);
	ChatScreen_Redraw(s);
	Screen_UpdateVb(s);

#ifdef CC_BUILD_TOUCH
	if (!Gui.TouchUI) return;
	Gui_MakeTitleFont(&font);
	ButtonWidget_SetConst(&s->more,   "More",   &font);
	ButtonWidget_SetConst(&s->send,   "Send",   &font);
	ButtonWidget_SetConst(&s->cancel, "Cancel", &font);
	Font_Free(&font);
#endif
}

static int ChatScreen_CalcMaxVertices(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	struct TextGroupWidget* chat = &s->chat;
	/* In case chatlines is 0 */
	return max(4, chat->VTABLE->GetMaxVertices(chat));
}

static void ChatScreen_BuildMesh(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	struct VertexTextured* data;
	struct VertexTextured** ptr;

	data = Screen_LockVb(s);
	ptr  = &data;

	Widget_BuildMesh(&s->chat, ptr);
	Gfx_UnlockDynamicVb(s->vb);
}

static void ChatScreen_Layout(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (ChatScreen_ChatUpdateFont(s)) ChatScreen_Redraw(s);

	s->paddingX = Display_ScaleX(5);
	s->paddingY = Display_ScaleY(5);

	Widget_SetLocation(&s->input.base,   ANCHOR_MIN, ANCHOR_MAX,  5, 5);
	Widget_SetLocation(&s->altText,      ANCHOR_MIN, ANCHOR_MAX,  5, 5);
	Widget_SetLocation(&s->status,       ANCHOR_MAX, ANCHOR_MIN,  0, 0);
	Widget_SetLocation(&s->bottomRight,  ANCHOR_MAX, ANCHOR_MAX,  0, 0);
	Widget_SetLocation(&s->chat,         ANCHOR_MIN, ANCHOR_MAX, 10, 0);
	Widget_SetLocation(&s->clientStatus, ANCHOR_MIN, ANCHOR_MAX, 10, 0);
	ChatScreen_UpdateChatYOffsets(s);

	/* Can't use Widget_SetLocation because it DPI scales input */
	s->bottomRight.yOffset = HUDScreen_LayoutHotbar() + Display_ScaleY(15);
	Widget_Layout(&s->bottomRight);

	Widget_SetLocation(&s->announcement, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 0);
	s->announcement.yOffset = -Window_UI.Height / 4;
	Widget_Layout(&s->announcement);

	Widget_SetLocation(&s->bigAnnouncement, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 0);
	s->bigAnnouncement.yOffset = -Window_UI.Height / 16;
	Widget_Layout(&s->bigAnnouncement);

	Widget_SetLocation(&s->smallAnnouncement, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 0);
	s->smallAnnouncement.yOffset = Window_UI.Height / 20;
	Widget_Layout(&s->smallAnnouncement);

#ifdef CC_BUILD_TOUCH
	if (Window_Main.SoftKeyboard == SOFT_KEYBOARD_SHIFT) {
		Widget_SetLocation(&s->send,   ANCHOR_MAX, ANCHOR_MAX, 10,  60);
		Widget_SetLocation(&s->cancel, ANCHOR_MAX, ANCHOR_MAX, 10,  10);
		Widget_SetLocation(&s->more,   ANCHOR_MAX, ANCHOR_MAX, 10, 110);
	} else {
		Widget_SetLocation(&s->send,   ANCHOR_MAX, ANCHOR_MIN, 10,  10);
		Widget_SetLocation(&s->cancel, ANCHOR_MAX, ANCHOR_MIN, 10,  60);
		Widget_SetLocation(&s->more,   ANCHOR_MAX, ANCHOR_MIN, 10, 110);
	}
#endif
}

static int ChatScreen_KeyPress(void* screen, char keyChar) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (!s->grabsInput) return false;

	if (s->suppressNextPress) {
		s->suppressNextPress = false;
		return false;
	}

	InputWidget_Append(&s->input.base, keyChar);
	return true;
}

static int ChatScreen_TextChanged(void* screen, const cc_string* str) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (!s->grabsInput) return false;

	InputWidget_SetText(&s->input.base, str);
	return true;
}

static int ChatScreen_KeyDown(void* screen, int key, struct InputDevice* device) {
	static const cc_string slash = String_FromConst("/");
	struct ChatScreen* s = (struct ChatScreen*)screen;
	int playerListKey    = KeyBind_Mappings[BIND_TABLIST].button1;
	cc_bool handlesList  = playerListKey != CCKEY_TAB || !Gui.TabAutocomplete || !s->grabsInput;

	if (InputBind_Claims(BIND_TABLIST, key, device) && handlesList) {
		if (!tablist_active && !Server.IsSinglePlayer) {
			TabListOverlay_Show(false);
		}
		return true;
	}

	s->suppressNextPress = false;
	/* Handle chat text input */
	if (s->grabsInput) {
#ifdef CC_BUILD_WEB
		/* See reason for this in HandleInputUp */
		if (InputBind_Claims(BIND_SEND_CHAT, key, device) || key == CCKEY_KP_ENTER) {
			ChatScreen_EnterChatInput(s, false);
#else
		if (InputBind_Claims(BIND_SEND_CHAT, key, device) || key == CCKEY_KP_ENTER || key == device->escapeButton) {
			ChatScreen_EnterChatInput(s, key == device->escapeButton);
#endif
		} else if (key == device->pageUpButton) {
			ChatScreen_ScrollChatBy(s, -Gui.Chatlines);
		} else if (key == device->pageDownButton) {
			ChatScreen_ScrollChatBy(s, +Gui.Chatlines);
		} else if (key == CCWHEEL_UP) {
			ChatScreen_ScrollChatBy(s, -1);
		} else if (key == CCWHEEL_DOWN) {
			ChatScreen_ScrollChatBy(s, +1);
		} else {
			Elem_HandlesKeyDown(&s->input.base, key, device);
		}
		return key < CCKEY_F1 || key > CCKEY_F24;
	}

	if (InputBind_Claims(BIND_CHAT, key, device)) {
		ChatScreen_OpenInput(&String_Empty);
	} else if (key == CCKEY_SLASH) {
		ChatScreen_OpenInput(&slash);
	} else if (InputBind_Claims(BIND_INVENTORY, key, device)) {
		InventoryScreen_Show();
	} else {
		return false;
	}
	return true;
}

static void ChatScreen_ToggleAltInput(struct ChatScreen* s) {
	SpecialInputWidget_SetActive(&s->altText, !s->altText.active);
	ChatScreen_UpdateChatYOffsets(s);
}

static void ChatScreen_KeyUp(void* screen, int key, struct InputDevice* device) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (!s->grabsInput || (struct Screen*)s != Gui.InputGrab) return;

#ifdef CC_BUILD_WEB
	/* See reason for this in HandleInputUp */
	if (key == CCKEY_ESCAPE) ChatScreen_EnterChatInput(s, true);
#endif

	if (Server.SupportsFullCP437 && InputBind_Claims(BIND_EXT_INPUT, key, device)) {
		if (!Window_Main.Focused) return;
		ChatScreen_ToggleAltInput(s);
	}
}

static int ChatScreen_MouseScroll(void* screen, float delta) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	return s->grabsInput;
}

static int ChatScreen_PointerDown(void* screen, int id, int x, int y) {
	cc_string text; char textBuffer[STRING_SIZE * 4];
	struct ChatScreen* s = (struct ChatScreen*)screen;
	int height, chatY, i;
	if (Game_HideGui) return false;

	if (!s->grabsInput) {
		if (!Gui_TouchUI) return false;
		String_InitArray(text, textBuffer);

		/* Should be able to click on links with touch */
		i = TextGroupWidget_GetSelected(&s->chat, &text, x, y);
		if (!Utils_IsUrlPrefix(&text)) return false;

		if (Chat_GetLogTime(s->chatIndex + i) + 10 < Game.Time) return false;
		UrlWarningOverlay_Show(&text); return TOUCH_TYPE_GUI;
	}

#ifdef CC_BUILD_TOUCH
	if (Gui.TouchUI) {
		if (Widget_Contains(&s->send, x, y)) {
			ChatScreen_EnterChatInput(s, false); return TOUCH_TYPE_GUI;
		}
		if (Widget_Contains(&s->cancel, x, y)) {
			ChatScreen_EnterChatInput(s, true); return TOUCH_TYPE_GUI;
		}
		if (Widget_Contains(&s->more, x, y)) {
			ChatScreen_ToggleAltInput(s); return TOUCH_TYPE_GUI;
		}
	}
#endif

	if (!Widget_Contains(&s->chat, x, y)) {
		if (s->altText.active && Widget_Contains(&s->altText, x, y)) {
			Elem_HandlesPointerDown(&s->altText, id, x, y);
			ChatScreen_UpdateChatYOffsets(s);
			return TOUCH_TYPE_GUI;
		}
		Elem_HandlesPointerDown(&s->input.base, id, x, y);
		return TOUCH_TYPE_GUI;
	}

	height = TextGroupWidget_UsedHeight(&s->chat);
	chatY  = s->chat.y + s->chat.height - height;
	if (!Gui_Contains(s->chat.x, chatY, s->chat.width, height, x, y)) return false;

	String_InitArray(text, textBuffer);
	TextGroupWidget_GetSelected(&s->chat, &text, x, y);
	if (!text.length) return false;

	if (Utils_IsUrlPrefix(&text) && Process_OpenSupported) {
		UrlWarningOverlay_Show(&text);
	} else if (Gui.ClickableChat) {
		ChatScreen_AppendInput(&text);
	}
	return TOUCH_TYPE_GUI;
}

static void ChatScreen_Init(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	ChatInputWidget_Create(&s->input);
	s->input.base.OnTextChanged = ChatScreen_OnInputTextChanged;
	SpecialInputWidget_Create(&s->altText, &s->chatFont, &s->input.base);

	TextGroupWidget_Create(&s->status, CHAT_MAX_STATUS,
							s->statusTextures, ChatScreen_GetStatus);
	TextGroupWidget_Create(&s->bottomRight, CHAT_MAX_BOTTOMRIGHT, 
							s->bottomRightTextures, ChatScreen_GetBottomRight);
	TextGroupWidget_Create(&s->chat, Gui.Chatlines,
							s->chatTextures, ChatScreen_GetChat);
	TextGroupWidget_Create(&s->clientStatus, CHAT_MAX_CLIENTSTATUS,
							s->clientStatusTextures, ChatScreen_GetClientStatus);
	TextWidget_Init(&s->announcement);
	TextWidget_Init(&s->bigAnnouncement);
	TextWidget_Init(&s->smallAnnouncement);

	s->status.collapsible[0]       = true; /* Texture pack downloading status */
	s->status.collapsible[1]       = true; /* Reduced performance mode status */
	s->clientStatus.collapsible[0] = true;
	s->clientStatus.collapsible[1] = true;

	s->chat.underlineUrls = !Game_ClassicMode;
	s->chatIndex = Chat_Log.count - Gui.Chatlines;

	Event_Register_(&ChatEvents.ChatReceived,   s, ChatScreen_ChatReceived);
	Event_Register_(&ChatEvents.ColCodeChanged, s, ChatScreen_ColCodeChanged);
	
	s->maxVertices = ChatScreen_CalcMaxVertices(s);
	
	/* For dual screen builds, chat is still rendered on the main game screen */
	s->input.base.flags   |= WIDGET_FLAG_MAINSCREEN;
	s->altText.flags      |= WIDGET_FLAG_MAINSCREEN;
	s->status.flags       |= WIDGET_FLAG_MAINSCREEN;
	s->bottomRight.flags  |= WIDGET_FLAG_MAINSCREEN;
	s->chat.flags         |= WIDGET_FLAG_MAINSCREEN;
	s->clientStatus.flags |= WIDGET_FLAG_MAINSCREEN;

	s->bottomRight.flags       |= WIDGET_FLAG_MAINSCREEN;
	s->announcement.flags      |= WIDGET_FLAG_MAINSCREEN;
	s->bigAnnouncement.flags   |= WIDGET_FLAG_MAINSCREEN;
	s->smallAnnouncement.flags |= WIDGET_FLAG_MAINSCREEN;

#ifdef CC_BUILD_TOUCH
	ButtonWidget_Init(&s->send,   100, NULL);
	ButtonWidget_Init(&s->cancel, 100, NULL);
	ButtonWidget_Init(&s->more,   100, NULL);
#endif
}

static void ChatScreen_Render(void* screen, float delta) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	Gfx_3DS_SetRenderScreen(TOP_SCREEN);

	if (Game_HideGui && s->grabsInput) {
		Elem_Render(&s->input.base, delta);
	}
	if (!Game_HideGui) {
		if (s->grabsInput && !Gui.ClassicChat) {
			ChatScreen_DrawChatBackground(s);
		}

		ChatScreen_DrawChat(s, delta);
	}
	Gfx_3DS_SetRenderScreen(BOTTOM_SCREEN);
}

static void ChatScreen_Free(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	Event_Unregister_(&ChatEvents.ChatReceived,   s, ChatScreen_ChatReceived);
	Event_Unregister_(&ChatEvents.ColCodeChanged, s, ChatScreen_ColCodeChanged);
}

static const struct ScreenVTABLE ChatScreen_VTABLE = {
	ChatScreen_Init,        ChatScreen_Update, ChatScreen_Free,
	ChatScreen_Render,      ChatScreen_BuildMesh,
	ChatScreen_KeyDown,     ChatScreen_KeyUp,  ChatScreen_KeyPress, ChatScreen_TextChanged,
	ChatScreen_PointerDown, Screen_PointerUp,  Screen_FPointer,     ChatScreen_MouseScroll,
	ChatScreen_Layout, ChatScreen_ContextLost, ChatScreen_ContextRecreated
};
void ChatScreen_Show(void) {
	struct ChatScreen* s  = &ChatScreen_Instance;
	s->lastDownloadStatus = HTTP_PROGRESS_NOT_WORKING_ON;

	s->VTABLE = &ChatScreen_VTABLE;
	Gui_Chat  = s;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_CHAT);
}

void ChatScreen_OpenInput(const cc_string* text) {
	struct ChatScreen* s = &ChatScreen_Instance;
	struct OpenKeyboardArgs args;
	s->suppressNextPress = true;
	s->grabsInput        = true;

	Gui_UpdateInputGrab();
	String_Copy(&s->input.base.text, text);

	OpenKeyboardArgs_Init(&args, text, KEYBOARD_TYPE_TEXT | KEYBOARD_FLAG_SEND);
	args.placeholder = "Enter chat";
	args.multiline   = true;
	OnscreenKeyboard_Open(&args);

	Widget_SetDisabled(&s->input.base, args.opaque);
	InputWidget_UpdateText(&s->input.base);
}

void ChatScreen_AppendInput(const cc_string* text) {
	struct ChatScreen* s = &ChatScreen_Instance;
	InputWidget_AppendText(&s->input.base, text);
}

void ChatScreen_SetChatlines(int lines) {
	struct ChatScreen* s = &ChatScreen_Instance;
	Elem_Free(&s->chat);
	s->chatIndex += s->chat.lines - lines;
	s->chat.lines = lines;
	TextGroupWidget_RedrawAll(&s->chat);

	s->maxVertices = ChatScreen_CalcMaxVertices(s);
	Screen_UpdateVb(s);
	s->dirty = true;
}


/*########################################################################################################################*
*-----------------------------------------------------InventoryScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct InventoryScreen {
	Screen_Body
	struct FontDesc font;
	struct TableWidget table;
	struct TextWidget title;
	cc_bool releasedInv, deferredSelect;
} InventoryScreen;

static struct Widget* inventory_widgets[2];


static void InventoryScreen_GetTitleText(cc_string* desc, BlockID block) {
	cc_string name;
	int block_ = block;
	if (Game_PureClassic) { String_AppendConst(desc, "Select block"); return; }
	if (block == BLOCK_AIR) return;

	name = Block_UNSAFE_GetName(block);
	String_AppendString(desc, &name);
	if (Game_ClassicMode) return;

	String_Format1(desc, " (ID %i&f", &block_);
	if (!Blocks.CanPlace[block])  { String_AppendConst(desc,  ", place &cNo&f"); }
	if (!Blocks.CanDelete[block]) { String_AppendConst(desc, ", delete &cNo&f"); }
	String_Append(desc, ')');
}

static void InventoryScreen_UpdateTitle(struct InventoryScreen* s, BlockID block) {
	cc_string desc; char descBuffer[STRING_SIZE * 2];

	String_InitArray(desc, descBuffer);
	InventoryScreen_GetTitleText(&desc, block);
	TextWidget_Set(&s->title, &desc, &s->font);
	s->dirty = true;
}

static void InventoryScreen_OnUpdateTitle(BlockID block) {
	InventoryScreen_UpdateTitle(&InventoryScreen, block);
}


static void InventoryScreen_OnBlockChanged(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	TableWidget_OnInventoryChanged(&s->table);
}

static void InventoryScreen_NeedRedrawing(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	s->dirty = true;
}

static void InventoryScreen_ContextLost(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Font_Free(&s->font);
	Screen_ContextLost(s);
	s->table.vb = 0;
}

static void InventoryScreen_ContextRecreated(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Screen_UpdateVb(s);
	s->table.vb = s->vb;

	Gui_MakeBodyFont(&s->font);
	TableWidget_RecreateTitle(&s->table, true);
}

static void InventoryScreen_MoveToSelected(struct InventoryScreen* s) {
	struct TableWidget* table = &s->table;
	s->deferredSelect = false;

	if (Game_ClassicMode) {
		/* Accuracy: Original classic preserves selected block across inventory menu opens */
		TableWidget_SetToIndex(table, table->selectedIndex);
		TableWidget_RecreateTitle(table, true);
	} else {
		TableWidget_SetToBlock(table, Inventory_SelectedBlock);

		if (table->selectedIndex == -1) {
			/* Hidden block in inventory - display title for it still */
			InventoryScreen_OnUpdateTitle(Inventory_SelectedBlock);
		} else {
			TableWidget_RecreateTitle(table, true);
		}
	}
}

static void InventoryScreen_Init(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	s->widgets     = inventory_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(inventory_widgets);
	
	TextWidget_Add(s,  &s->title);
	TableWidget_Add(s, &s->table, 22 * Options_GetFloat(OPT_INV_SCROLLBAR_SCALE, 0, 10, 1));
	s->table.blocksPerRow = Inventory.BlocksPerRow;
	s->table.UpdateTitle   = InventoryScreen_OnUpdateTitle;
	TableWidget_RecreateBlocks(&s->table);

	/* Can't immediately move to selected here, because cursor grabbed  */
	/*  status might be toggled *after* InventoryScreen_Init() is called */
	/* That causes the cursor to be moved back to the middle of the window */
	s->deferredSelect = true;

	Event_Register_(&TextureEvents.AtlasChanged,     s, InventoryScreen_NeedRedrawing);
	Event_Register_(&BlockEvents.PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_Register_(&BlockEvents.BlockDefChanged,    s, InventoryScreen_OnBlockChanged);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static void InventoryScreen_Free(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;

	Event_Unregister_(&TextureEvents.AtlasChanged,     s, InventoryScreen_NeedRedrawing);
	Event_Unregister_(&BlockEvents.PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_Unregister_(&BlockEvents.BlockDefChanged,    s, InventoryScreen_OnBlockChanged);
}

static void InventoryScreen_Update(void* screen, float delta) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	if (s->deferredSelect) InventoryScreen_MoveToSelected(s);
}

static void InventoryScreen_Render(void* screen, float delta) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Widget_Render2(&s->table, TEXTWIDGET_MAX);
	Widget_Render2(&s->title,              0);
}

static void InventoryScreen_Layout(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	s->table.scale = Gui_GetInventoryScale();
	Widget_SetLocation(&s->table, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 0);

	Widget_SetLocation(&s->title, ANCHOR_CENTRE, ANCHOR_MIN, 0, 0);
	/* use Table(Y) directly instead of s->title->height ??? */
	s->title.yOffset = s->table.y - s->title.height - 3;
	Widget_Layout(&s->title); /* Needed for yOffset */
}

static int InventoryScreen_KeyDown(void* screen, int key, struct InputDevice* device) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct TableWidget* table = &s->table;

	/* Accuracy: Original classic doesn't close inventory menu when B is pressed */
	if (InputBind_Claims(BIND_INVENTORY, key, device) && s->releasedInv && !Game_ClassicMode) {
		Gui_Remove((struct Screen*)s);
	} else if (InputDevice_IsEnter(key, device) && table->selectedIndex != -1) {
		Inventory_SetSelectedBlock(table->blocks[table->selectedIndex]);
		Gui_Remove((struct Screen*)s);
	} else if (Elem_HandlesKeyDown(table, key, device)) {
	} else {
		return Elem_HandlesKeyDown(&HUDScreen_Instance.hotbar, key, device);
	}
	return true;
}

static cc_bool InventoryScreen_IsHotbarActive(void) {
	struct Screen* grabbed = Gui.InputGrab;
	/* Only toggle hotbar when inventory or no grab screen is open */
	return !grabbed || grabbed == (struct Screen*)&InventoryScreen;
}

static void InventoryScreen_KeyUp(void* screen, int key, struct InputDevice* device) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	if (InputBind_Claims(BIND_INVENTORY, key, device)) s->releasedInv = true;
}

static int InventoryScreen_PointerDown(void* screen, int id, int x, int y) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct TableWidget* table = &s->table;
	cc_bool handled, hotbar;

	if (table->scroll.draggingId == id) return TOUCH_TYPE_GUI;
	if (HUDscreen_PointerDown(Gui_HUD, id, x, y)) return TOUCH_TYPE_GUI;
	handled = Elem_HandlesPointerDown(table, id, x, y);

	if (!handled || table->pendingClose) {
		hotbar = Input_IsCtrlPressed() || Input_IsShiftPressed();
		if (!hotbar) Gui_Remove((struct Screen*)s);
	}
	return TOUCH_TYPE_GUI;
}

static void InventoryScreen_PointerUp(void* screen, int id, int x, int y) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Elem_OnPointerUp(&s->table, id, x, y);
}

static int InventoryScreen_PointerMove(void* screen, int id, int x, int y) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	return Elem_HandlesPointerMove(&s->table, id, x, y);
}

static int InventoryScreen_MouseScroll(void* screen, float delta) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;

	cc_bool hotbar = Input_IsAltPressed() || Input_IsCtrlPressed() || Input_IsShiftPressed();
	if (hotbar) return false;
	return Elem_HandlesMouseScroll(&s->table, delta);
}

static int InventoryScreen_PadAxis(void* screen, int axis, float x, float y) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;

	return Elem_HandlesPadAxis(&s->table, axis, x, y);
}

static const struct ScreenVTABLE InventoryScreen_VTABLE = {
	InventoryScreen_Init,        InventoryScreen_Update,    InventoryScreen_Free,
	InventoryScreen_Render,      Screen_BuildMesh,
	InventoryScreen_KeyDown,     InventoryScreen_KeyUp,     Screen_TKeyPress,            Screen_TText,
	InventoryScreen_PointerDown, InventoryScreen_PointerUp, InventoryScreen_PointerMove, InventoryScreen_MouseScroll,
	InventoryScreen_Layout,  InventoryScreen_ContextLost, InventoryScreen_ContextRecreated,
	InventoryScreen_PadAxis
};
void InventoryScreen_Show(void) {
	struct InventoryScreen* s = &InventoryScreen;
	s->grabsInput = true;
	s->closable   = true;

	s->VTABLE = &InventoryScreen_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_INVENTORY);
}


/*########################################################################################################################*
*------------------------------------------------------LoadingScreen------------------------------------------------------*
*#########################################################################################################################*/
static struct LoadingScreen {
	Screen_Body
	struct FontDesc font;
	float progress; 
	int rows;
	
	int progX, progY, progWidth, progHeight;
	struct TextWidget title, message;
	cc_string titleStr, messageStr;
	const char* lastState;

	char _titleBuffer[STRING_SIZE];
	char _messageBuffer[STRING_SIZE];
} LoadingScreen;

static struct Widget* loading_widgets[2];
#define LOADING_TILE_SIZE 64

static void LoadingScreen_SetTitle(struct LoadingScreen* s) {
	TextWidget_Set(&s->title, &s->titleStr, &s->font);
	s->dirty = true;
}
static void LoadingScreen_SetMessage(struct LoadingScreen* s) {
	TextWidget_Set(&s->message, &s->messageStr, &s->font);
	s->dirty = true;
}

static void LoadingScreen_CalcMaxVertices(struct LoadingScreen* s) {
	s->rows = Math_CeilDiv(Window_UI.Height, LOADING_TILE_SIZE);
	s->maxVertices = Screen_CalcDefaultMaxVertices(s) + s->rows * 4;
}

static void LoadingScreen_Layout(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	int oldRows, y;
	Widget_SetLocation(&s->title,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -31);
	Widget_SetLocation(&s->message, ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  17);
	y = Display_ScaleY(34);

	s->progWidth  = Display_ScaleX(200);
	s->progX      = Gui_CalcPos(ANCHOR_CENTRE, 0, s->progWidth,  Window_UI.Width);
	s->progHeight = Display_ScaleY(4);
	s->progY      = Gui_CalcPos(ANCHOR_CENTRE, y, s->progHeight, Window_UI.Height);

	oldRows = s->rows;
	LoadingScreen_CalcMaxVertices(s);
	if (oldRows == s->rows) return;
	Screen_UpdateVb(s);
}

static void LoadingScreen_ContextLost(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Font_Free(&s->font);
	Screen_ContextLost(screen);
}

static void LoadingScreen_ContextRecreated(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Gui_MakeBodyFont(&s->font);
	LoadingScreen_SetTitle(s);
	LoadingScreen_SetMessage(s);
	Screen_UpdateVb(s);
}

static void LoadingScreen_BuildMesh(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	struct VertexTextured* data;
	struct VertexTextured** ptr;
	struct Texture tex;
	TextureLoc loc;
	int atlasIndex, i;

	data = Screen_LockVb(s);
	ptr  = &data;

	loc       = Block_Tex(BLOCK_DIRT, FACE_YMAX);
	Tex_SetRect(tex, 0,0, Window_UI.Width,LOADING_TILE_SIZE);
	tex.uv    = Atlas1D_TexRec(loc, 1, &atlasIndex);
	tex.uv.u2 = (float)Window_UI.Width / LOADING_TILE_SIZE;
	
	for (i = 0; i < s->rows; i++) {
		tex.y = i * LOADING_TILE_SIZE;
		Gfx_Make2DQuad(&tex, PackedCol_Make(64, 64, 64, 255), ptr);
	}

	Widget_BuildMesh(&s->title,   ptr);
	Widget_BuildMesh(&s->message, ptr);
	Gfx_UnlockDynamicVb(s->vb);
}

static void LoadingScreen_MapLoading(void* screen, float progress) {
	((struct LoadingScreen*)screen)->progress = progress;
}

static void LoadingScreen_MapLoaded(void* screen) {
	Gui_Remove((struct Screen*)screen);
}

static void LoadingScreen_Init(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	s->widgets     = loading_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(loading_widgets);

	TextWidget_Add(s, &s->title);
	TextWidget_Add(s, &s->message);

	LoadingScreen_CalcMaxVertices(s);
	Gfx_SetFog(false);
	Event_Register_(&WorldEvents.Loading,   s, LoadingScreen_MapLoading);
	Event_Register_(&WorldEvents.MapLoaded, s, LoadingScreen_MapLoaded);
}

static void LoadingScreen_Render(void* screen, float delta) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	int offset, filledWidth;
	TextureLoc loc;

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindDynamicVb(s->vb);

	/* Draw background dirt */
	offset = 0;
	if (s->rows) {
		loc = Block_Tex(BLOCK_DIRT, FACE_YMAX);
		Atlas1D_Bind(Atlas1D_Index(loc));
		Gfx_DrawVb_IndexedTris(s->rows * 4);
		offset = s->rows * 4;
	}

	offset = Widget_Render2(&s->title,   offset);
	offset = Widget_Render2(&s->message, offset);

	filledWidth = (int)(s->progWidth * s->progress);
	Gfx_Draw2DFlat(s->progX, s->progY, s->progWidth, 
					s->progHeight, PackedCol_Make(128, 128, 128, 255));
	Gfx_Draw2DFlat(s->progX, s->progY, filledWidth,  
					s->progHeight, PackedCol_Make(128, 255, 128, 255));
}

static void LoadingScreen_Free(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Event_Unregister_(&WorldEvents.Loading,   s, LoadingScreen_MapLoading);
	Event_Unregister_(&WorldEvents.MapLoaded, s, LoadingScreen_MapLoaded);
}

CC_NOINLINE static void LoadingScreen_ShowCommon(const cc_string* title, const cc_string* message) {
	struct LoadingScreen* s = &LoadingScreen;
	s->lastState = NULL;
	s->progress  = 0.0f;

	String_InitArray(s->titleStr,   s->_titleBuffer);
	String_AppendString(&s->titleStr,   title);
	String_InitArray(s->messageStr, s->_messageBuffer);
	String_AppendString(&s->messageStr, message);
	
	s->grabsInput  = true;
	s->blocksWorld = true;
	Gui_Add((struct Screen*)s, 
		Game_ClassicMode ? GUI_PRIORITY_OLDLOADING : GUI_PRIORITY_LOADING);
}

static const struct ScreenVTABLE LoadingScreen_VTABLE = {
	LoadingScreen_Init,   Screen_NullUpdate, LoadingScreen_Free, 
	LoadingScreen_Render, LoadingScreen_BuildMesh,
	Screen_TInput,        Screen_InputUp,    Screen_TKeyPress,   Screen_TText,
	Screen_TPointer,      Screen_PointerUp,  Screen_TPointer,    Screen_TMouseScroll,
	LoadingScreen_Layout, LoadingScreen_ContextLost, LoadingScreen_ContextRecreated
};
void LoadingScreen_Show(const cc_string* title, const cc_string* message) {
	LoadingScreen.VTABLE = &LoadingScreen_VTABLE;
	LoadingScreen_ShowCommon(title, message);
}


/*########################################################################################################################*
*--------------------------------------------------GeneratingMapScreen----------------------------------------------------*
*#########################################################################################################################*/
static void GeneratingScreen_AtlasChanged(void* obj) {
	LoadingScreen.dirty = true; /* Dirt texture may have changed */
}

static void GeneratingScreen_Init(void* screen) {
	LoadingScreen_Init(screen);
	Event_Register_(&TextureEvents.AtlasChanged,   NULL, GeneratingScreen_AtlasChanged);
}
static void GeneratingScreen_Free(void* screen) {
	LoadingScreen_Free(screen);
	Event_Unregister_(&TextureEvents.AtlasChanged, NULL, GeneratingScreen_AtlasChanged);
}

static void GeneratingScreen_EndGeneration(void) {
	struct LocationUpdate update;
	World_SetNewMap(Gen_Blocks, World.Width, World.Height, World.Length);
	if (!Gen_Blocks) { Chat_AddRaw("&cFailed to generate the map."); return; }

	Gen_Blocks = NULL;
	World.Seed = Gen_Seed;

	LocalPlayer_CalcDefaultSpawn(Entities.CurPlayer, &update);
	LocalPlayers_MoveToSpawn(&update);
}

static void GeneratingScreen_Update(void* screen, float delta) {
	struct LoadingScreen* s    = (struct LoadingScreen*)screen;
	const char* state = (const char*)Gen_CurrentState;
	if (state == s->lastState) return;
	s->lastState = state;

	s->messageStr.length = 0;
	String_AppendConst(&s->messageStr, state);
	LoadingScreen_SetMessage(s);
}

static void GeneratingScreen_Render(void* screen, float delta) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	s->progress = Gen_CurrentProgress;
	LoadingScreen_Render(s, delta);
	if (Gen_IsDone()) GeneratingScreen_EndGeneration();
}

static const struct ScreenVTABLE GeneratingScreen_VTABLE = {
	GeneratingScreen_Init,   GeneratingScreen_Update, GeneratingScreen_Free,
	GeneratingScreen_Render, LoadingScreen_BuildMesh,
	Screen_TInput,           Screen_InputUp,    Screen_TKeyPress,   Screen_TText,
	Screen_TPointer,         Screen_PointerUp,  Screen_FPointer,    Screen_TMouseScroll,
	LoadingScreen_Layout, LoadingScreen_ContextLost, LoadingScreen_ContextRecreated
};
void GeneratingScreen_Show(void) {
	static const cc_string title   = String_FromConst("Generating level");
	static const cc_string message = String_FromConst("Generating..");

	LoadingScreen.VTABLE = &GeneratingScreen_VTABLE;
	LoadingScreen_ShowCommon(&title, &message);
}


/*########################################################################################################################*
*----------------------------------------------------DisconnectScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct DisconnectScreen {
	Screen_Body
	double initTime;
	cc_bool canReconnect, lastActive;
	int lastSecsLeft;
	struct ButtonWidget reconnect, quit;

	struct FontDesc titleFont, messageFont;
	struct TextWidget title, message;
	char _titleBuffer[STRING_SIZE * 2];
	char _messageBuffer[STRING_SIZE];
	cc_string titleStr, messageStr;
} DisconnectScreen;

static struct Widget* disconnect_widgets[4];
#define DISCONNECT_DELAY_SECS 5

static void DisconnectScreen_Layout(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	Widget_SetLocation(&s->title,     ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);
	Widget_SetLocation(&s->message,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  10);
	Widget_SetLocation(&s->reconnect, ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  80);
	Widget_SetLocation(&s->quit,      ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 130);
}

static void DisconnectScreen_UpdateReconnect(struct DisconnectScreen* s) {
	cc_string msg; char msgBuffer[STRING_SIZE];
	int elapsed, secsLeft;
	String_InitArray(msg, msgBuffer);

	if (s->canReconnect) {
		elapsed  = (int)(Game.Time - s->initTime);
		secsLeft = DISCONNECT_DELAY_SECS - elapsed;

		if (secsLeft > 0) {
			String_Format1(&msg, "Reconnect in %i", &secsLeft);
		}
		Widget_SetDisabled(&s->reconnect, secsLeft > 0);
	}

	if (!msg.length) String_AppendConst(&msg, "Reconnect");
	ButtonWidget_Set(&s->reconnect, &msg, &s->titleFont);
}

static void DisconnectScreen_ContextLost(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	Font_Free(&s->titleFont);
	Font_Free(&s->messageFont);
	Screen_ContextLost(screen);
}

static void DisconnectScreen_ContextRecreated(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	Screen_UpdateVb(screen);

	Gui_MakeTitleFont(&s->titleFont);
	Gui_MakeBodyFont(&s->messageFont);
	TextWidget_Set(&s->title,   &s->titleStr,   &s->titleFont);
	TextWidget_Set(&s->message, &s->messageStr, &s->messageFont);

	DisconnectScreen_UpdateReconnect(s);
	ButtonWidget_SetConst(&s->quit, "Quit game", &s->titleFont);
}

static void DisconnectScreen_OnReconnect(void* s, void* w) {
	Gui_Remove((struct Screen*)s);
	Gui_ShowDefault();
	Server.BeginConnect();
}

static void DisconnectScreen_OnQuit(void* s, void* w) { 
	Window_RequestClose(); 
}

static void DisconnectScreen_Init(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	s->widgets      = disconnect_widgets;
	s->numWidgets   = 0;
	s->maxWidgets   = Array_Elems(disconnect_widgets);

	TextWidget_Add(s, &s->title);
	TextWidget_Add(s, &s->message);

	ButtonWidget_Add(s, &s->reconnect, 300, DisconnectScreen_OnReconnect);
	ButtonWidget_Add(s, &s->quit,      300, DisconnectScreen_OnQuit);
	if (!s->canReconnect) s->reconnect.flags = WIDGET_FLAG_DISABLED;

	Game_SetMinFrameTime(1000 / 5.0f);

	s->initTime     = Game.Time;
	s->lastSecsLeft = DISCONNECT_DELAY_SECS;
	s->maxVertices  = Screen_CalcDefaultMaxVertices(s);
}

static void DisconnectScreen_Update(void* screen, float delta) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	int elapsed, secsLeft;

	if (!s->canReconnect) return;
	elapsed  = (int)(Game.Time - s->initTime);
	secsLeft = DISCONNECT_DELAY_SECS - elapsed;

	if (secsLeft < 0) secsLeft = 0;
	if (s->lastSecsLeft == secsLeft && s->reconnect.active == s->lastActive) return;
	DisconnectScreen_UpdateReconnect(s);

	s->lastSecsLeft = secsLeft;
	s->lastActive   = s->reconnect.active;
	s->dirty        = true;
}

static void DisconnectScreen_Render(void* screen, float delta) {
	PackedCol top    = PackedCol_Make(64, 32, 32, 255);
	PackedCol bottom = PackedCol_Make(80, 16, 16, 255);
	Gfx_Draw2DGradient(0, 0, Window_UI.Width, Window_UI.Height, top, bottom);

	Screen_Render2Widgets(screen, delta);
}

static void DisconnectScreen_Free(void* screen) { Game_SetFpsLimit(Game_FpsLimit); }

static const struct ScreenVTABLE DisconnectScreen_VTABLE = {
	DisconnectScreen_Init,   DisconnectScreen_Update, DisconnectScreen_Free,
	DisconnectScreen_Render, Screen_BuildMesh,
	Menu_InputDown,          Screen_InputUp,          Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,        Screen_PointerUp,        Menu_PointerMove, Screen_TMouseScroll,
	DisconnectScreen_Layout, DisconnectScreen_ContextLost, DisconnectScreen_ContextRecreated
};
void DisconnectScreen_Show(const cc_string* title, const cc_string* message) {
	static const cc_string kick = String_FromConst("Kicked ");
	static const cc_string ban  = String_FromConst("Banned ");
	cc_string why; char whyBuffer[STRING_SIZE];
	struct DisconnectScreen* s = &DisconnectScreen;
	int i;

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

	Gui_Add((struct Screen*)s, GUI_PRIORITY_DISCONNECT);
	/* Remove other screens instead of just drawing over them to reduce GPU usage */
	for (i = Gui.ScreensCount - 1; i >= 0; i--) 
	{
		if (Gui_Screens[i] == (struct Screen*)s) continue;
		Gui_Remove(Gui_Screens[i]);
	}
}
