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
#include "Utils.h"

#define CHAT_MAX_STATUS Array_Elems(Chat_Status)
#define CHAT_MAX_BOTTOMRIGHT Array_Elems(Chat_BottomRight)
#define CHAT_MAX_CLIENTSTATUS Array_Elems(Chat_ClientStatus)

int Screen_FInput(void* s, int key)             { return false; }
int Screen_FKeyPress(void* s, char keyChar)     { return false; }
int Screen_FText(void* s, const cc_string* str) { return false; }
int Screen_FMouseScroll(void* s, float delta)   { return false; }
int Screen_FPointer(void* s, int id, int x, int y) { return false; }

int Screen_TInput(void* s, int key)             { return true; }
int Screen_TKeyPress(void* s, char keyChar)     { return true; }
int Screen_TText(void* s, const cc_string* str) { return true; }
int Screen_TMouseScroll(void* s, float delta)   { return true; }
int Screen_TPointer(void* s, int id, int x, int y) { return true; }

void Screen_NullFunc(void* screen) { }
void Screen_NullUpdate(void* screen, double delta) { }
int  Screen_InputDown(void* screen, int key) { return key < KEY_F1 || key > KEY_F24; }

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
	double accumulator;
	int frames, fps;
	cc_bool speed, halfSpeed, canSpeed, hacksChanged;
	int lastFov;
	struct HotbarWidget hotbar;
} HUDScreen_Instance;

static void HUDScreen_MakeText(struct HUDScreen* s, cc_string* status) {
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

static void HUDScreen_DrawPosition(struct HUDScreen* s) {
	struct VertexTextured vertices[4 * 64];
	struct VertexTextured* ptr = vertices;
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

static cc_bool HUDScreen_HasHacksChanged(struct HUDScreen* s) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	return hacks->Speeding != s->speed   || hacks->HalfSpeeding != s->halfSpeed
			|| Game_Fov    != s->lastFov || hacks->CanSpeed != s->canSpeed || s->hacksChanged;
}

static void HUDScreen_UpdateHackState(struct HUDScreen* s) {
	cc_string status; char statusBuffer[STRING_SIZE * 2];
	struct HacksComp* hacks;
	cc_bool speeding;

	hacks = &LocalPlayer_Instance.Hacks;
	s->speed   = hacks->Speeding; s->halfSpeed = hacks->HalfSpeeding;
	s->lastFov = Game_Fov;         s->canSpeed = hacks->CanSpeed;
	s->hacksChanged = false;

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

static void HUDScreen_Update(void* screen, double delta) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	cc_string status; char statusBuffer[STRING_SIZE * 2];

	s->frames++;
	s->accumulator += delta;
	if (s->accumulator < 1.0) return;

	String_InitArray(status, statusBuffer);
	HUDScreen_MakeText(s, &status);

	TextWidget_Set(&s->line1, &status, &s->font);
	s->accumulator = 0.0;
	s->frames = 0;
	Game.ChunkUpdates = 0;
}

static void HUDScreen_ContextLost(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	Font_Free(&s->font);
	TextAtlas_Free(&s->posAtlas);
	Elem_Free(&s->hotbar);
	Elem_Free(&s->line1);
	Elem_Free(&s->line2);
}

static void HUDScreen_ContextRecreated(void* screen) {	
	static const cc_string chars  = String_FromConst("0123456789-, ()");
	static const cc_string prefix = String_FromConst("Position: ");

	struct HUDScreen* s      = (struct HUDScreen*)screen;
	struct TextWidget* line1 = &s->line1;
	struct TextWidget* line2 = &s->line2;

	Drawer2D_MakeFont(&s->font, 16, FONT_FLAGS_NONE);
	Font_SetPadding(&s->font, 2);
	HotbarWidget_SetFont(&s->hotbar, &s->font);

	HUDScreen_Update(s, 1.0);
	TextAtlas_Make(&s->posAtlas, &chars, &s->font, &prefix);

	if (Game_ClassicMode) {
		TextWidget_SetConst(line2, "0.30", &s->font);
	} else {
		HUDScreen_UpdateHackState(s);
	}
}

static void HUDScreen_BuildMesh(void* screen) { }

static void HUDScreen_Layout(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	struct TextWidget* line1 = &s->line1;
	struct TextWidget* line2 = &s->line2;
	int posY;

	Widget_SetLocation(line1, ANCHOR_MIN, ANCHOR_MIN, 2, 2);
	posY = line1->y + line1->height;
	s->posAtlas.tex.Y = posY;
	Widget_SetLocation(line2, ANCHOR_MIN, ANCHOR_MIN, 2, 0);

	if (Game_ClassicMode) {
		/* Swap around so 0.30 version is at top */
		line2->yOffset = line1->yOffset;
		line1->yOffset = posY;
		Widget_Layout(line1);
	} else {
		/* We can't use y in TextWidget_Make because that DPI scales it */
		line2->yOffset = posY + s->posAtlas.tex.Height;
	}

	Widget_Layout(&s->hotbar);
	Widget_Layout(line2);
}

static int HUDScreen_KeyDown(void* screen, int key) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	return Elem_HandlesKeyDown(&s->hotbar, key);
}

static int HUDScreen_KeyUp(void* screen, int key) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	return Elem_HandlesKeyUp(&s->hotbar, key);
}

static int HUDscreen_PointerDown(void* screen, int id, int x, int y) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
#ifdef CC_BUILD_TOUCH
	if (Input_TouchMode || Gui_GetInputGrab()) {
		return Elem_HandlesPointerDown(&s->hotbar, id, x, y);
	}
#else
	if (Gui_GetInputGrab()) return Elem_HandlesPointerDown(&s->hotbar, id, x, y);
#endif
	return false;
}

static void HUDScreen_HacksChanged(void* obj) {
	((struct HUDScreen*)obj)->hacksChanged = true;
}

static void HUDScreen_Init(void* screen) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	HotbarWidget_Create(&s->hotbar);
	TextWidget_Init(&s->line1);
	TextWidget_Init(&s->line2);
	Event_Register_(&UserEvents.HacksStateChanged, screen, HUDScreen_HacksChanged);
}

static void HUDScreen_Render(void* screen, double delta) {
	struct HUDScreen* s = (struct HUDScreen*)screen;
	if (Game_HideGui) return;

	/* TODO: If Game_ShowFps is off and not classic mode, we should just return here */
	Gfx_SetTexturing(true);
	if (Gui.ShowFPS) Elem_Render(&s->line1, delta);

	if (Game_ClassicMode) {
		Elem_Render(&s->line2, delta);
	} else if (IsOnlyChatActive() && Gui.ShowFPS) {
		if (HUDScreen_HasHacksChanged(s)) HUDScreen_UpdateHackState(s);
		HUDScreen_DrawPosition(s);
		Elem_Render(&s->line2, delta);
	}

	if (!Gui_GetBlocksWorld()) Elem_Render(&s->hotbar, delta);
	Gfx_SetTexturing(false);
}

static void HUDScreen_Free(void* screen) {
	Event_Unregister_(&UserEvents.HacksStateChanged, screen, HUDScreen_HacksChanged);
}

static const struct ScreenVTABLE HUDScreen_VTABLE = {
	HUDScreen_Init,        HUDScreen_Update,    HUDScreen_Free,
	HUDScreen_Render,      HUDScreen_BuildMesh,
	HUDScreen_KeyDown,     HUDScreen_KeyUp,     Screen_FKeyPress, Screen_FText,
	HUDscreen_PointerDown, Screen_FPointer,     Screen_FPointer,  Screen_FMouseScroll,
	HUDScreen_Layout,      HUDScreen_ContextLost, HUDScreen_ContextRecreated
};
void HUDScreen_Show(void) {
	struct HUDScreen* s = &HUDScreen_Instance;
	s->VTABLE = &HUDScreen_VTABLE;
	Gui_HUD   = s;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_HUD);
}

struct Widget* HUDScreen_GetHotbar(void) {
	return (struct Widget*)&HUDScreen_Instance.hotbar;
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
	cc_bool active, classic;
	int namesCount, elementOffset;
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
	int end = min(s->namesCount, i + LIST_NAMES_PER_COLUMN);
	int maxWidth = 0;

	for (; i < end; i++) {
		maxWidth = max(maxWidth, s->textures[i].Width);
	}
	return maxWidth + LIST_COLUMN_PADDING + s->elementOffset;
}

static int TabListOverlay_GetColumnHeight(struct TabListOverlay* s, int column) {
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(s->namesCount, i + LIST_NAMES_PER_COLUMN);
	int height = 0;

	for (; i < end; i++) {
		height += s->textures[i].Height + 1;
	}
	return height;
}

static void TabListOverlay_SetColumnPos(struct TabListOverlay* s, int column, int x, int y) {
	struct Texture tex;
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(s->namesCount, i + LIST_NAMES_PER_COLUMN);

	for (; i < end; i++) {
		tex = s->textures[i];
		tex.X = x; tex.Y = y - 10;

		y += tex.Height + 1;
		/* offset player names a bit, compared to group name */
		if (!s->classic && s->ids[i] != GROUP_NAME_ID) {
			tex.X += s->elementOffset;
		}
		s->textures[i] = tex;
	}
}

static void TabListOverlay_Layout(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	int minWidth, minHeight, paddingX, paddingY;
	int i, x, y, width = 0, height = 0;
	int columns = Math_CeilDiv(s->namesCount, LIST_NAMES_PER_COLUMN);

	for (i = 0; i < columns; i++) {
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

	y    = WindowInfo.Height / 4 - height / 2;
	s->x = Gui_CalcPos(ANCHOR_CENTRE,          0, width , WindowInfo.Width );
	s->y = Gui_CalcPos(ANCHOR_CENTRE, -max(0, y), height, WindowInfo.Height);

	x = s->x + paddingX;
	y = s->y + paddingY;

	for (i = 0; i < columns; i++) {
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
	if (index == -1) { index = s->namesCount; s->namesCount++; }

	name = TabList_UNSAFE_GetList(id);
	s->ids[index] = id;
	TabListOverlay_DrawText(&s->textures[index], s, &name);
}

static void TabListOverlay_DeleteAt(struct TabListOverlay* s, int i) {
	Gfx_DeleteTexture(&s->textures[i].ID);

	for (; i < s->namesCount - 1; i++) {
		s->ids[i]      = s->ids[i + 1];
		s->textures[i] = s->textures[i + 1];
	}

	s->namesCount--;
	s->ids[s->namesCount]         = 0;
	s->textures[s->namesCount].ID = 0;
}

static void TabListOverlay_AddGroup(struct TabListOverlay* s, int id, int* index) {
	cc_string group;
	int i;
	group = TabList_UNSAFE_GetGroup(id);

	for (i = Array_Elems(s->ids) - 1; i > (*index); i--) {
		s->ids[i]      = s->ids[i - 1];
		s->textures[i] = s->textures[i - 1];
	}
	
	s->ids[*index] = GROUP_NAME_ID;
	TabListOverlay_DrawText(&s->textures[*index], s, &group);

	(*index)++;
	s->namesCount++;
}

static int TabListOverlay_GetGroupCount(struct TabListOverlay* s, int id, int i) {
	cc_string group, curGroup;
	int count;
	group = TabList_UNSAFE_GetGroup(id);

	for (count = 0; i < s->namesCount; i++, count++) {
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
	if (!s->namesCount) return;

	if (s->classic) {
		TabListOverlay_Instance.compare = TabListOverlay_PlayerCompare;
		TabListOverlay_QuickSort(0, s->namesCount - 1);
		return;
	}

	/* Sort the list by group */
	/* Loop backwards, since DeleteAt() reduces NamesCount */
	for (i = s->namesCount - 1; i >= 0; i--) {
		if (s->ids[i] != GROUP_NAME_ID) continue;
		TabListOverlay_DeleteAt(s, i);
	}
	TabListOverlay_Instance.compare = TabListOverlay_GroupCompare;
	TabListOverlay_QuickSort(0, s->namesCount - 1);

	/* Sort the entries in each group */
	TabListOverlay_Instance.compare = TabListOverlay_PlayerCompare;
	for (i = 0; i < s->namesCount; ) {
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
}

static void TabListOverlay_Add(void* obj, int id) {
	struct TabListOverlay* s = (struct TabListOverlay*)obj;
	TabListOverlay_AddName(s, id, -1);
	TabListOverlay_SortAndLayout(s);
}

static void TabListOverlay_Update(void* obj, int id) {
	struct TabListOverlay* s = (struct TabListOverlay*)obj;
	struct Texture tex;
	int i;

	for (i = 0; i < s->namesCount; i++) {
		if (s->ids[i] != id) continue;
		tex = s->textures[i];

		Gfx_DeleteTexture(&tex.ID);
		TabListOverlay_AddName(s, id, i);
		TabListOverlay_SortAndLayout(s);
		return;
	}
}

static void TabListOverlay_Remove(void* obj, int id) {
	struct TabListOverlay* s = (struct TabListOverlay*)obj;
	int i;
	for (i = 0; i < s->namesCount; i++) {
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

	for (i = 0; i < s->namesCount; i++) {
		if (!s->textures[i].ID || s->ids[i] == GROUP_NAME_ID) continue;
		tex = s->textures[i];
		if (!Gui_Contains(tex.X, tex.Y, tex.Width, tex.Height, x, y)) continue;

		player = TabList_UNSAFE_GetPlayer(s->ids[i]);
		String_Format1(&text, "%s ", &player);
		ChatScreen_AppendInput(&text);
		return true;
	}
	return false;
}

static int TabListOverlay_KeyUp(void* screen, int key) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	if (key != KeyBinds[KEYBIND_PLAYER_LIST]) return false;

	Gui_Remove((struct Screen*)s);
	return true;
}

static void TabListOverlay_ContextLost(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	int i;
	for (i = 0; i < s->namesCount; i++) {
		Gfx_DeleteTexture(&s->textures[i].ID);
	}

	Elem_Free(&s->title);
	Font_Free(&s->font);
}

static void TabListOverlay_ContextRecreated(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	int size, id;

	size = Drawer2D_BitmappedText ? 16 : 11;
	Drawer2D_MakeFont(&s->font, size, FONT_FLAGS_NONE);
	s->namesCount = 0;

	TextWidget_SetConst(&s->title, "Connected players:", &s->font);
	Font_SetPadding(&s->font, 1);

	/* TODO: Just recreate instead of this? maybe */
	for (id = 0; id < TABLIST_MAX_NAMES; id++) {
		if (!TabList.NameOffsets[id]) continue;
		TabListOverlay_AddName(s, (EntityID)id, -1);
	}
	TabListOverlay_SortAndLayout(s); /* TODO: Not do layout here too */
}

static void TabListOverlay_BuildMesh(void* screen) { }

static void TabListOverlay_Render(void* screen, double delta) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	struct TextWidget* title = &s->title;
	struct Screen* grabbed;
	struct Texture tex;
	int i;
	PackedCol topCol    = PackedCol_Make( 0,  0,  0, 180);
	PackedCol bottomCol = PackedCol_Make(50, 50, 50, 205);

	if (Game_HideGui || !IsOnlyChatActive()) return;
	Gfx_SetTexturing(false);
	Gfx_Draw2DGradient(s->x, s->y, s->width, s->height, topCol, bottomCol);

	Gfx_SetTexturing(true);
	Elem_Render(title, delta);
	grabbed = Gui_GetInputGrab();

	for (i = 0; i < s->namesCount; i++) {
		if (!s->textures[i].ID) continue;
		tex = s->textures[i];
		
		if (grabbed && s->ids[i] != GROUP_NAME_ID) {
			if (Gui_ContainsPointers(tex.X, tex.Y, tex.Width, tex.Height)) tex.X += 4;
		}
		Texture_Render(&tex);
	}
	Gfx_SetTexturing(false);

	/* NOTE: Should usually be caught by KeyUp, but just in case. */
	if (KeyBind_IsPressed(KEYBIND_PLAYER_LIST)) return;
	Gui_Remove((struct Screen*)s);
}

static void TabListOverlay_Free(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	s->active = false;
	Event_Unregister_(&TabListEvents.Added,   s, TabListOverlay_Add);
	Event_Unregister_(&TabListEvents.Changed, s, TabListOverlay_Update);
	Event_Unregister_(&TabListEvents.Removed, s, TabListOverlay_Remove);
}

static void TabListOverlay_Init(void* screen) {
	struct TabListOverlay* s = (struct TabListOverlay*)screen;
	s->active        = true;
	s->classic       = Gui.ClassicTabList || !Server.SupportsExtPlayerList;
	s->elementOffset = s->classic ? 0 : 10;
	TextWidget_Init(&s->title);

	Event_Register_(&TabListEvents.Added,   s, TabListOverlay_Add);
	Event_Register_(&TabListEvents.Changed, s, TabListOverlay_Update);
	Event_Register_(&TabListEvents.Removed, s, TabListOverlay_Remove);
}

static const struct ScreenVTABLE TabListOverlay_VTABLE = {
	TabListOverlay_Init,        Screen_NullUpdate,     TabListOverlay_Free,
	TabListOverlay_Render,      TabListOverlay_BuildMesh,
	Screen_FInput,              TabListOverlay_KeyUp,  Screen_FKeyPress, Screen_FText,
	TabListOverlay_PointerDown, Screen_FPointer,       Screen_FPointer,  Screen_FMouseScroll,
	TabListOverlay_Layout, TabListOverlay_ContextLost, TabListOverlay_ContextRecreated
};
void TabListOverlay_Show(void) {
	struct TabListOverlay* s  = &TabListOverlay_Instance;
	s->VTABLE = &TabListOverlay_VTABLE;
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
	struct FontDesc chatFont, announcementFont;
	struct TextWidget announcement;
	struct ChatInputWidget input;
	struct TextGroupWidget status, bottomRight, chat, clientStatus;
	struct SpecialInputWidget altText;
#ifdef CC_BUILD_TOUCH
	struct ButtonWidget send, cancel;
#endif

	struct Texture statusTextures[CHAT_MAX_STATUS];
	struct Texture bottomRightTextures[CHAT_MAX_BOTTOMRIGHT];
	struct Texture clientStatusTextures[CHAT_MAX_CLIENTSTATUS];
	struct Texture chatTextures[TEXTGROUPWIDGET_MAX_LINES];
} ChatScreen_Instance;
#define CH_EXTENT 16

static void ChatScreen_UpdateChatYOffsets(struct ChatScreen* s) {
	int pad, y;

	/* Determining chat Y requires us to know hotbar's position */
	/* But HUD is lower priority, so it gets laid out AFTER chat */
	/* Hence use this hack to resize HUD first */
	HUDScreen_Layout(Gui_HUD);
		
	y = min(s->input.base.y, Gui_HUD->hotbar.y);
	y -= s->input.base.yOffset; /* add some padding */
	s->altText.yOffset = WindowInfo.Height - y;
	Widget_Layout(&s->altText);

	pad = s->altText.active ? 5 : 10;
	s->clientStatus.yOffset = WindowInfo.Height - s->altText.y + pad;
	Widget_Layout(&s->clientStatus);
	s->chat.yOffset = s->clientStatus.yOffset + s->clientStatus.height;
	Widget_Layout(&s->chat);
}

static void ChatScreen_OnInputTextChanged(void* elem) {
	ChatScreen_UpdateChatYOffsets(Gui_Chat);
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
}

static cc_bool ChatScreen_ChatUpdateFont(struct ChatScreen* s) {
	int size = (int)(8  * Gui_GetChatScale());
	Math_Clamp(size, 8, 60);

	/* don't recreate font if possible */
	/* TODO: Add function for this, don't use Display_ScaleY (Drawer2D_SameFontSize ??) */
	if (Display_ScaleY(size) == s->chatFont.size) return false;
	ChatScreen_FreeChatFonts(s);
	Drawer2D_MakeFont(&s->chatFont, size, FONT_FLAGS_NONE);

	size = (int)(16 * Gui_GetChatScale());
	Math_Clamp(size, 8, 60);
	Drawer2D_MakeFont(&s->announcementFont, size, FONT_FLAGS_NONE);

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
	Camera_CheckFocus();
	Window_CloseKeyboard();
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
	if (progress == s->lastDownloadStatus) return;

	s->lastDownloadStatus = progress;
	Chat_Status[0].length = 0;

	if (progress == HTTP_PROGRESS_MAKING_REQUEST) {
		String_AppendConst(&Chat_Status[0], "&eRetrieving texture pack..");
	} else if (progress == HTTP_PROGRESS_FETCHING_DATA) {
		String_AppendConst(&Chat_Status[0], "&eDownloading texture pack");
	} else if (progress >= 0 && progress <= 100) {
		String_Format1(&Chat_Status[0], "&eDownloading texture pack (&7%i&e%%)", &progress);
	}
	TextGroupWidget_Redraw(&s->status, 0);
}

static void ChatScreen_ColCodeChanged(void* screen, int code) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
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

static void ChatScreen_ChatReceived(void* screen, const cc_string* msg, int type) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (Gfx.LostContext) return;

	if (type == MSG_TYPE_NORMAL) {
		s->chatIndex++;
		if (!Gui.Chatlines) return;
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
		ChatScreen_UpdateChatYOffsets(s);
	}
}

static void ChatScreen_DrawCrosshairs(void) {
	static struct Texture tex = { 0, Tex_Rect(0,0,0,0), Tex_UV(0.0f,0.0f, 15/256.0f,15/256.0f) };
	int extent;
	if (!Gui.IconsTex) return;

	extent = (int)(CH_EXTENT * Gui_Scale(WindowInfo.Height / 480.0f));
	tex.ID = Gui.IconsTex;
	tex.X  = (WindowInfo.Width  / 2) - extent;
	tex.Y  = (WindowInfo.Height / 2) - extent;

	tex.Width  = extent * 2;
	tex.Height = extent * 2;
	Texture_Render(&tex);
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

static void ChatScreen_DrawChat(struct ChatScreen* s, double delta) {
	struct Texture tex;
	double now;
	int i, logIdx;

	ChatScreen_UpdateTexpackStatus(s);
	if (!Game_PureClassic) { Elem_Render(&s->status, delta); }
	Elem_Render(&s->bottomRight, delta);
	Elem_Render(&s->clientStatus, delta);

	now = Game.Time;
	if (s->grabsInput) {
		Elem_Render(&s->chat, delta);
	} else {
		/* Only render recent chat */
		for (i = 0; i < s->chat.lines; i++) {
			tex    = s->chat.textures[i];
			logIdx = s->chatIndex + i;
			if (!tex.ID) continue;

			if (logIdx < 0 || logIdx >= Chat_Log.count) continue;
			if (Chat_LogTime[logIdx] + 10 >= now) Texture_Render(&tex);
		}
	}

	/* Destroy announcement texture before even rendering it at all, */
	/* otherwise changing texture pack shows announcement for one frame */
	if (s->announcement.tex.ID && now > Chat_AnnouncementReceived + 5) {
		Elem_Free(&s->announcement);
	}
	Elem_Render(&s->announcement, delta);

	if (s->grabsInput) {
		Elem_Render(&s->input.base, delta);
		if (s->altText.active) {
			Elem_Render(&s->altText, delta);
		}

#ifdef CC_BUILD_TOUCH
		if (!Input_TouchMode) return;
		Elem_Render(&s->send,   delta);
		Elem_Render(&s->cancel, delta);
#endif
	}
}

static void ChatScreen_ContextLost(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	ChatScreen_FreeChatFonts(s);

	Elem_Free(&s->chat);
	Elem_Free(&s->input.base);
	Elem_Free(&s->altText);
	Elem_Free(&s->status);
	Elem_Free(&s->bottomRight);
	Elem_Free(&s->clientStatus);
	Elem_Free(&s->announcement);

#ifdef CC_BUILD_TOUCH
	if (!Input_TouchMode) return;
	Elem_Free(&s->send);
	Elem_Free(&s->cancel);
#endif
}

static void ChatScreen_ContextRecreated(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	struct FontDesc font;
	ChatScreen_ChatUpdateFont(s);
	ChatScreen_Redraw(s);

#ifdef CC_BUILD_TOUCH
	if (!Input_TouchMode) return;
	Drawer2D_MakeFont(&font, 16, FONT_FLAGS_BOLD);
	ButtonWidget_SetConst(&s->send,   "Send",   &font);
	ButtonWidget_SetConst(&s->cancel, "Cancel", &font);
	Font_Free(&font);
#endif
}

static void ChatScreen_BuildMesh(void* screen) { }

static void ChatScreen_Layout(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	/* See comment in ChatScreen_UpdateChatYOffsets */
	HUDScreen_Layout(Gui_HUD);
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
	s->bottomRight.yOffset = Gui_HUD->hotbar.height + Display_ScaleY(15);
	Widget_Layout(&s->bottomRight);

	Widget_SetLocation(&s->announcement, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 0);
	s->announcement.yOffset = -WindowInfo.Height / 4;
	Widget_Layout(&s->announcement);

#ifdef CC_BUILD_TOUCH
	if (!Input_TouchMode) return;
	Widget_SetLocation(&s->send,   ANCHOR_MAX, ANCHOR_MIN, 10, 10);
	Widget_SetLocation(&s->cancel, ANCHOR_MAX, ANCHOR_MIN, 10, 60);
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
#ifdef CC_BUILD_TOUCH
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (!s->grabsInput) return false;

	InputWidget_SetText(&s->input.base, str);
#endif
	return true;
}

static int ChatScreen_KeyDown(void* screen, int key) {
	static const cc_string slash = String_FromConst("/");
	struct ChatScreen* s = (struct ChatScreen*)screen;
	int playerListKey   = KeyBinds[KEYBIND_PLAYER_LIST];
	cc_bool handlesList = playerListKey != KEY_TAB || !Gui.TabAutocomplete || !s->grabsInput;

	if (key == playerListKey && handlesList) {
		if (!TabListOverlay_Instance.active && !Server.IsSinglePlayer) {
			TabListOverlay_Show();
		}
		return true;
	}

	s->suppressNextPress = false;
	/* Handle chat text input */
	if (s->grabsInput) {
#ifdef CC_BUILD_WEB
		/* See reason for this in HandleInputUp */
		if (key == KeyBinds[KEYBIND_SEND_CHAT] || key == KEY_KP_ENTER) {
			ChatScreen_EnterChatInput(s, false);
#else
		if (key == KeyBinds[KEYBIND_SEND_CHAT] || key == KEY_KP_ENTER || key == KEY_ESCAPE) {
			ChatScreen_EnterChatInput(s, key == KEY_ESCAPE);
#endif
		} else if (key == KEY_PAGEUP) {
			ChatScreen_ScrollChatBy(s, -Gui.Chatlines);
		} else if (key == KEY_PAGEDOWN) {
			ChatScreen_ScrollChatBy(s, +Gui.Chatlines);
		} else {
			Elem_HandlesKeyDown(&s->input.base, key);
		}
		return key < KEY_F1 || key > KEY_F24;
	}

	if (key == KeyBinds[KEYBIND_CHAT]) {
		ChatScreen_OpenInput(&String_Empty);
	} else if (key == KEY_SLASH) {
		ChatScreen_OpenInput(&slash);
	} else if (key == KeyBinds[KEYBIND_INVENTORY]) {
		InventoryScreen_Show();
	} else {
		return false;
	}
	return true;
}

static int ChatScreen_KeyUp(void* screen, int key) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	if (!s->grabsInput) return false;
#ifdef CC_BUILD_WEB
	/* See reason for this in HandleInputUp */
	if (key == KEY_ESCAPE) ChatScreen_EnterChatInput(s, true);
#endif

	if (Server.SupportsFullCP437 && key == KeyBinds[KEYBIND_EXT_INPUT]) {
		if (!WindowInfo.Focused) return true;
		SpecialInputWidget_SetActive(&s->altText, !s->altText.active);
		ChatScreen_UpdateChatYOffsets(s);
	}
	return true;
}

static int ChatScreen_MouseScroll(void* screen, float delta) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	int steps;
	if (!s->grabsInput) return false;

	steps = Utils_AccumulateWheelDelta(&s->chatAcc, delta);
	ChatScreen_ScrollChatBy(s, -steps);
	return true;
}

static int ChatScreen_PointerDown(void* screen, int id, int x, int y) {
	cc_string text; char textBuffer[STRING_SIZE * 4];
	struct ChatScreen* s = (struct ChatScreen*)screen;
	int height, chatY;

	if (!s->grabsInput || Game_HideGui) return false;

#ifdef CC_BUILD_TOUCH
	if (Widget_Contains(&s->send, x, y)) {
		ChatScreen_EnterChatInput(s, false); return true;
	}
	if (Widget_Contains(&s->cancel, x, y)) {
		ChatScreen_EnterChatInput(s, true); return true;
	}
#endif

	if (!Widget_Contains(&s->chat, x, y)) {
		if (s->altText.active && Widget_Contains(&s->altText, x, y)) {
			Elem_HandlesPointerDown(&s->altText, id, x, y);
			ChatScreen_UpdateChatYOffsets(s);
			return true;
		}
		Elem_HandlesPointerDown(&s->input.base, id, x, y);
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
	} else if (Gui.ClickableChat) {
		ChatScreen_AppendInput(&text);
	}
	return true;
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

	s->status.collapsible[0]       = true; /* Texture pack download status */
	s->clientStatus.collapsible[0] = true;
	s->clientStatus.collapsible[1] = true;

	s->chat.underlineUrls = !Game_ClassicMode;
	s->chatIndex = Chat_Log.count - Gui.Chatlines;

	Event_Register_(&ChatEvents.ChatReceived,   s, ChatScreen_ChatReceived);
	Event_Register_(&ChatEvents.ColCodeChanged, s, ChatScreen_ColCodeChanged);

#ifdef CC_BUILD_TOUCH
	if (!Input_TouchMode) return;
	ButtonWidget_Init(&s->send,   100, NULL);
	ButtonWidget_Init(&s->cancel, 100, NULL);
#endif
}

static void ChatScreen_Render(void* screen, double delta) {
	struct ChatScreen* s = (struct ChatScreen*)screen;

	if (Game_HideGui && s->grabsInput) {
		Gfx_SetTexturing(true);
		Elem_Render(&s->input.base, delta);
		Gfx_SetTexturing(false);
	}
	if (Game_HideGui) return;

	if (!TabListOverlay_Instance.active && !Gui_GetBlocksWorld()) {
		Gfx_SetTexturing(true);
		ChatScreen_DrawCrosshairs();
		Gfx_SetTexturing(false);
	}
	if (s->grabsInput && !Gui.ClassicChat) {
		ChatScreen_DrawChatBackground(s);
	}

	Gfx_SetTexturing(true);
	ChatScreen_DrawChat(s, delta);
	Gfx_SetTexturing(false);
}

static void ChatScreen_Free(void* screen) {
	struct ChatScreen* s = (struct ChatScreen*)screen;
	Event_Unregister_(&ChatEvents.ChatReceived,   s, ChatScreen_ChatReceived);
	Event_Unregister_(&ChatEvents.ColCodeChanged, s, ChatScreen_ColCodeChanged);
}

static const struct ScreenVTABLE ChatScreen_VTABLE = {
	ChatScreen_Init,        Screen_NullUpdate, ChatScreen_Free,    
	ChatScreen_Render,      ChatScreen_BuildMesh,
	ChatScreen_KeyDown,     ChatScreen_KeyUp,  ChatScreen_KeyPress, ChatScreen_TextChanged,
	ChatScreen_PointerDown, Screen_FPointer,   Screen_FPointer,     ChatScreen_MouseScroll,
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
	s->suppressNextPress = true;
	s->grabsInput        = true;
	Camera_CheckFocus();
	Window_OpenKeyboard(text, KEYBOARD_TYPE_TEXT);

	String_Copy(&s->input.base.text, text);
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
}


/*########################################################################################################################*
*-----------------------------------------------------InventoryScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct InventoryScreen {
	Screen_Body
	struct FontDesc font;
	struct TableWidget table;
	cc_bool releasedInv, deferredSelect;
} InventoryScreen_Instance;

static void InventoryScreen_OnBlockChanged(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	TableWidget_OnInventoryChanged(&s->table);
}

static void InventoryScreen_ContextLost(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Font_Free(&s->font);
	Elem_Free(&s->table);
}

static void InventoryScreen_ContextRecreated(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Drawer2D_MakeFont(&s->font, 16, FONT_FLAGS_NONE);
	TableWidget_Recreate(&s->table);
}

static void InventoryScreen_BuildMesh(void* screen) { }

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
	s->table.blocksPerRow = Inventory.BlocksPerRow;
	TableWidget_RecreateBlocks(&s->table);

	/* Can't immediately move to selected here, because cursor grabbed  */
	/* status might be toggled after InventoryScreen_Init() is called. */
	/* That causes the cursor to be moved back to the middle of the window. */
	s->deferredSelect = true;

	Event_Register_(&BlockEvents.PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_Register_(&BlockEvents.BlockDefChanged,    s, InventoryScreen_OnBlockChanged);
}

static void InventoryScreen_Render(void* screen, double delta) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	if (s->deferredSelect) InventoryScreen_MoveToSelected(s);
	Elem_Render(&s->table, delta);
}

static void InventoryScreen_Layout(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Widget_Layout(&s->table);
}

static void InventoryScreen_Free(void* screen) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	Event_Unregister_(&BlockEvents.PermissionsChanged, s, InventoryScreen_OnBlockChanged);
	Event_Unregister_(&BlockEvents.BlockDefChanged,    s, InventoryScreen_OnBlockChanged);
}

static int InventoryScreen_KeyDown(void* screen, int key) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct TableWidget* table = &s->table;

	if (key == KeyBinds[KEYBIND_INVENTORY] && s->releasedInv) {
		Gui_Remove((struct Screen*)s);
	} else if (key == KEY_ENTER && table->selectedIndex != -1) {
		Inventory_SetSelectedBlock(table->blocks[table->selectedIndex]);
		Gui_Remove((struct Screen*)s);
	} else if (Elem_HandlesKeyDown(table, key)) {
	} else {
		return Elem_HandlesKeyDown(&Gui_HUD->hotbar, key);
	}
	return true;
}

static int InventoryScreen_KeyUp(void* screen, int key) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;

	if (key == KeyBinds[KEYBIND_INVENTORY]) {
		s->releasedInv = true; return true;
	}
	return Elem_HandlesKeyUp(&Gui_HUD->hotbar, key);
}

static int InventoryScreen_PointerDown(void* screen, int id, int x, int y) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	struct TableWidget* table = &s->table;
	cc_bool handled, hotbar;

	if (table->scroll.draggingId == id) return true;
	if (HUDscreen_PointerDown(Gui_HUD, id, x, y)) return true;
	handled = Elem_HandlesPointerDown(table, id, x, y);

	if (!handled || table->pendingClose) {
		hotbar = Key_IsControlPressed() || Key_IsShiftPressed();
		if (!hotbar) Gui_Remove((struct Screen*)s);
	}
	return true;
}

static int InventoryScreen_PointerUp(void* screen, int id, int x, int y) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	return Elem_HandlesPointerUp(&s->table, id, x, y);
}

static int InventoryScreen_PointerMove(void* screen, int id, int x, int y) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;
	return Elem_HandlesPointerMove(&s->table, id, x, y);
}

static int InventoryScreen_MouseScroll(void* screen, float delta) {
	struct InventoryScreen* s = (struct InventoryScreen*)screen;

	cc_bool hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (hotbar) return false;
	return Elem_HandlesMouseScroll(&s->table, delta);
}

static const struct ScreenVTABLE InventoryScreen_VTABLE = {
	InventoryScreen_Init,        Screen_NullUpdate,         InventoryScreen_Free, 
	InventoryScreen_Render,      InventoryScreen_BuildMesh,
	InventoryScreen_KeyDown,     InventoryScreen_KeyUp,     Screen_TKeyPress,            Screen_TText,
	InventoryScreen_PointerDown, InventoryScreen_PointerUp, InventoryScreen_PointerMove, InventoryScreen_MouseScroll,
	InventoryScreen_Layout,  InventoryScreen_ContextLost, InventoryScreen_ContextRecreated
};
void InventoryScreen_Show(void) {
	struct InventoryScreen* s = &InventoryScreen_Instance;
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
#define LOADING_MAX_VERTICES (2 * TEXTWIDGET_MAX)
#define LOADING_TILE_SIZE 64

static struct Widget* loading_widgets[2] = {
	(struct Widget*)&LoadingScreen.title, (struct Widget*)&LoadingScreen.message
};

static void LoadingScreen_SetTitle(struct LoadingScreen* s) {
	TextWidget_Set(&s->title, &s->titleStr, &s->font);
	s->dirty = true;
}
static void LoadingScreen_SetMessage(struct LoadingScreen* s) {
	TextWidget_Set(&s->message, &s->messageStr, &s->font);
	s->dirty = true;
}

static void LoadingScreen_CalcMaxVertices(struct LoadingScreen* s) {
	s->rows = Math_CeilDiv(WindowInfo.Height, LOADING_TILE_SIZE);
	s->maxVertices = LOADING_MAX_VERTICES + s->rows * 4;
}

static void LoadingScreen_Layout(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	int oldRows, y;
	Widget_SetLocation(&s->title,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -31);
	Widget_SetLocation(&s->message, ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  17);
	y = Display_ScaleY(34);

	s->progWidth  = Display_ScaleX(200);
	s->progX      = Gui_CalcPos(ANCHOR_CENTRE, 0, s->progWidth, WindowInfo.Width);
	s->progHeight = Display_ScaleY(4);
	s->progY      = Gui_CalcPos(ANCHOR_CENTRE, y, s->progHeight, WindowInfo.Height);

	oldRows = s->rows;
	LoadingScreen_CalcMaxVertices(s);
	if (oldRows == s->rows) return;

	Gfx_DeleteDynamicVb(&s->vb);
	Screen_CreateVb(s);
}

static void LoadingScreen_ContextLost(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Font_Free(&s->font);
	Screen_ContextLost(screen);
}

static void LoadingScreen_ContextRecreated(void* screen) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	Drawer2D_MakeFont(&s->font, 16, FONT_FLAGS_NONE);
	LoadingScreen_SetTitle(s);
	LoadingScreen_SetMessage(s);
	Screen_CreateVb(s);
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
	Tex_SetRect(tex, 0,0, WindowInfo.Width,LOADING_TILE_SIZE);
	tex.uv    = Atlas1D_TexRec(loc, 1, &atlasIndex);
	tex.uv.U2 = (float)WindowInfo.Width / LOADING_TILE_SIZE;
	
	for (i = 0; i < s->rows; i++) {
		tex.Y = i * LOADING_TILE_SIZE;
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
	TextWidget_Init(&s->title);
	TextWidget_Init(&s->message);
	s->widgets     = loading_widgets;
	s->numWidgets  = Array_Elems(loading_widgets);

	LoadingScreen_CalcMaxVertices(s);
	Gfx_SetFog(false);
	Event_Register_(&WorldEvents.Loading,   s, LoadingScreen_MapLoading);
	Event_Register_(&WorldEvents.MapLoaded, s, LoadingScreen_MapLoaded);
}

static void LoadingScreen_Render(void* screen, double delta) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	int offset, filledWidth;
	TextureLoc loc;

	Gfx_SetTexturing(true);
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindDynamicVb(s->vb);

	/* Draw background dirt */
	offset = 0;
	if (s->rows) {
		loc = Block_Tex(BLOCK_DIRT, FACE_YMAX);
		Gfx_BindTexture(Atlas1D.TexIds[Atlas1D_Index(loc)]);
		Gfx_DrawVb_IndexedTris(s->rows * 4);
		offset = s->rows * 4;
	}

	offset = Widget_Render2(&s->title,   offset);
	offset = Widget_Render2(&s->message, offset);
	Gfx_SetTexturing(false);

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
	Screen_TInput,        Screen_TInput,     Screen_TKeyPress,   Screen_TText,
	Screen_TPointer,      Screen_TPointer,   Screen_TPointer,    Screen_TMouseScroll,
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
	Gen_Done = false;
	LoadingScreen_Init(screen);

	Gen_Blocks = (BlockRaw*)Mem_TryAlloc(World.Volume, 1);
	if (!Gen_Blocks) {
		Window_ShowDialog("Out of memory", "Not enough free memory to generate a map that large.\nTry a smaller size.");
		Gen_Done = true;
	} else if (Gen_Vanilla) {
		Thread_Start(NotchyGen_Generate, true);
	} else {
		Thread_Start(FlatgrassGen_Generate, true);
	}
	Event_Register_(&TextureEvents.AtlasChanged,   NULL, GeneratingScreen_AtlasChanged);
}
static void GeneratingScreen_Free(void* screen) {
	LoadingScreen_Free(screen);
	Event_Unregister_(&TextureEvents.AtlasChanged, NULL, GeneratingScreen_AtlasChanged);
}

static void GeneratingScreen_EndGeneration(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	float x, z;

	Gen_Done   = false;
	World_SetNewMap(Gen_Blocks, World.Width, World.Height, World.Length);
	if (!Gen_Blocks) { Chat_AddRaw("&cFailed to generate the map."); return; }
	Gen_Blocks = NULL;

	x = (World.Width / 2) + 0.5f; z = (World.Length / 2) + 0.5f;
	p->Spawn = Respawn_FindSpawnPosition(x, z, p->Base.Size);

	p->SpawnYaw   = 0.0f;
	p->SpawnPitch = 0.0f;
	LocalPlayer_MoveToSpawn();
}

static void GeneratingScreen_Update(void* screen, double delta) {
	struct LoadingScreen* s    = (struct LoadingScreen*)screen;
	const char* state = (const char*)Gen_CurrentState;
	if (state == s->lastState) return;
	s->lastState = state;

	s->messageStr.length = 0;
	String_AppendConst(&s->messageStr, state);
	LoadingScreen_SetMessage(s);
}

static void GeneratingScreen_Render(void* screen, double delta) {
	struct LoadingScreen* s = (struct LoadingScreen*)screen;
	s->progress = Gen_CurrentProgress;
	LoadingScreen_Render(s, delta);
	if (Gen_Done) GeneratingScreen_EndGeneration();
}

static const struct ScreenVTABLE GeneratingScreen_VTABLE = {
	GeneratingScreen_Init,   GeneratingScreen_Update, GeneratingScreen_Free,
	GeneratingScreen_Render, LoadingScreen_BuildMesh,
	Screen_TInput,           Screen_TInput,     Screen_TKeyPress,   Screen_TText,
	Screen_TPointer,         Screen_TPointer,   Screen_FPointer,    Screen_TMouseScroll,
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
	char _titleBuffer[STRING_SIZE];
	char _messageBuffer[STRING_SIZE];
	cc_string titleStr, messageStr;
} DisconnectScreen;

static struct Widget* disconnect_widgets[4] = {
	(struct Widget*)&DisconnectScreen.title, 
	(struct Widget*)&DisconnectScreen.message,
	(struct Widget*)&DisconnectScreen.reconnect,
	(struct Widget*)&DisconnectScreen.quit
};
#define DISCONNECT_MAX_VERTICES (2 * TEXTWIDGET_MAX + 2 * BUTTONWIDGET_MAX)
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
		s->reconnect.disabled = secsLeft > 0;
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
	Screen_CreateVb(screen);

	Drawer2D_MakeFont(&s->titleFont,   16, FONT_FLAGS_BOLD);
	Drawer2D_MakeFont(&s->messageFont, 16, FONT_FLAGS_NONE);
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
static void DisconnectScreen_OnQuit(void* s, void* w) { Window_Close(); }

static void DisconnectScreen_Init(void* screen) {
	struct DisconnectScreen* s = (struct DisconnectScreen*)screen;
	TextWidget_Init(&s->title);
	TextWidget_Init(&s->message);

	ButtonWidget_Init(&s->reconnect, 300, DisconnectScreen_OnReconnect);
	ButtonWidget_Init(&s->quit,      300, DisconnectScreen_OnQuit);
	s->reconnect.disabled = !s->canReconnect;
	s->maxVertices  = DISCONNECT_MAX_VERTICES;

	/* NOTE: changing VSync can't be done within frame, causes crash on some GPUs */
	Gfx_SetFpsLimit(Game_FpsLimit == FPS_LIMIT_VSYNC, 1000 / 5.0f);

	s->initTime     = Game.Time;
	s->lastSecsLeft = DISCONNECT_DELAY_SECS;
	s->widgets      = disconnect_widgets;
	s->numWidgets   = Array_Elems(disconnect_widgets);
}

static void DisconnectScreen_Update(void* screen, double delta) {
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

static void DisconnectScreen_Render(void* screen, double delta) {
	PackedCol top    = PackedCol_Make(64, 32, 32, 255);
	PackedCol bottom = PackedCol_Make(80, 16, 16, 255);
	Gfx_Draw2DGradient(0, 0, WindowInfo.Width, WindowInfo.Height, top, bottom);

	Gfx_SetTexturing(true);
	Screen_Render2Widgets(screen, delta);
	Gfx_SetTexturing(false);
}

static void DisconnectScreen_Free(void* screen) { Game_SetFpsLimit(Game_FpsLimit); }

static const struct ScreenVTABLE DisconnectScreen_VTABLE = {
	DisconnectScreen_Init,   DisconnectScreen_Update, DisconnectScreen_Free,
	DisconnectScreen_Render, Screen_BuildMesh,
	Screen_InputDown,        Screen_TInput,           Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,        Screen_TPointer,         Menu_PointerMove, Screen_TMouseScroll,
	DisconnectScreen_Layout, DisconnectScreen_ContextLost, DisconnectScreen_ContextRecreated
};
void DisconnectScreen_Show(const cc_string* title, const cc_string* message) {
	static const cc_string kick = String_FromConst("Kicked ");
	static const cc_string ban  = String_FromConst("Banned ");
	cc_string why; char whyBuffer[STRING_SIZE];
	struct DisconnectScreen* s = &DisconnectScreen;

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

	/* Remove all screens instead of just drawing over them to reduce GPU usage */
	Gui_RemoveAll();
	Gui_Add((struct Screen*)s, GUI_PRIORITY_DISCONNECT);
}


/*########################################################################################################################*
*--------------------------------------------------------TouchScreen------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_TOUCH
#define TOUCH_MAX_BTNS 4
struct TouchBindDesc {
	const char* text;
	cc_uint8 bind, width;
	cc_int16 x, y;
	Widget_LeftClick OnClick;
};

static struct TouchScreen {
	Screen_Body
	const struct TouchBindDesc* descs;
	int numDescs;
	struct FontDesc font;
	struct ThumbstickWidget thumbstick;
	struct ButtonWidget btns[TOUCH_MAX_BTNS];
} TouchScreen;

static struct Widget* touch_widgets[1 + TOUCH_MAX_BTNS] = {
	(struct Widget*)&TouchScreen.thumbstick, (struct Widget*)&TouchScreen.btns[0],
	(struct Widget*)&TouchScreen.btns[1],    (struct Widget*)&TouchScreen.btns[2],
	(struct Widget*)&TouchScreen.btns[3]
};
#define TOUCH_MAX_VERTICES (THUMBSTICKWIDGET_MAX + TOUCH_MAX_BTNS * BUTTONWIDGET_MAX)

static void TouchScreen_UpdateModeText(void* screen);
static void TouchScreen_ModeClick(void* s, void* w) { 
	Input_Placing = !Input_Placing; 
	TouchScreen_UpdateModeText(s);
}
static void TouchScreen_MoreClick(void* s, void* w) { TouchMoreScreen_Show(); }

static const struct TouchBindDesc normDescs[3] = {
	{ "Jump", KEYBIND_JUMP,     100,  50,  90, NULL                  },
	{ "",     KEYBIND_COUNT,    100,  50,  50, TouchScreen_ModeClick },
	{ "More", KEYBIND_COUNT,    100,  50,  10, TouchScreen_MoreClick },
};
static const struct TouchBindDesc hackDescs[4] = {
	{ "Up",   KEYBIND_FLY_UP,   100,  50, 130, NULL                  },
	{ "Down", KEYBIND_FLY_DOWN, 100,  50,  90, NULL                  },
	{ "",     KEYBIND_COUNT,    100,  50,  50, TouchScreen_ModeClick },
	{ "More", KEYBIND_COUNT,    100,  50,  10, TouchScreen_MoreClick },
};

static void TouchScreen_InitButtons(struct TouchScreen* s) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	const struct TouchBindDesc* desc;
	int i;

	if (hacks->Flying || hacks->Noclip) {
		s->descs    = hackDescs;
		s->numDescs = Array_Elems(hackDescs);
	} else {
		s->descs    = normDescs;
		s->numDescs = Array_Elems(normDescs);
	}
	s->numWidgets = 1 + s->numDescs;

	for (i = 0; i < s->numDescs; i++) {
		desc = &s->descs[i];
		ButtonWidget_Init(&s->btns[i], desc->width, desc->OnClick);
	}
}

static void TouchScreen_HacksChanged(void* screen) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	/* InitButtons changes number of widgets, hence */
	/* must destroy graphics resources BEFORE that */
	Screen_ContextLost(s);
	TouchScreen_InitButtons(s);
	Gui_Refresh((struct Screen*)s);
}

static void TouchScreen_ContextLost(void* screen) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	Font_Free(&s->font);
	Screen_ContextLost(screen);
}

static void TouchScreen_UpdateModeText(void* screen) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	ButtonWidget_SetConst(&s->btns[s->numDescs - 2], 
							Input_Placing ? "Place" : "Delete", &s->font);
}

static void TouchScreen_ContextRecreated(void* screen) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	const struct TouchBindDesc* desc;
	int i;
	Screen_CreateVb(screen);
	Drawer2D_MakeFont(&s->font, 16, FONT_FLAGS_BOLD);

	for (i = 0; i < s->numDescs; i++) {
		desc = &s->descs[i];
		ButtonWidget_SetConst(&s->btns[i], desc->text, &s->font);
	}
	TouchScreen_UpdateModeText(s);
	/* TODO: this is pretty nasty hacky. rewrite! */
}

static void TouchScreen_Render(void* screen, double delta) {
	if (Gui_GetInputGrab()) return;

	Gfx_SetTexturing(true);
	Screen_Render2Widgets(screen, delta);
	Gfx_SetTexturing(false);
}

static int TouchScreen_PointerDown(void* screen, int id, int x, int y) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	int i;
	//Chat_Add1("POINTER DOWN: %i", &id);
	if (Gui_GetInputGrab()) return false;

	if (Widget_Contains(&s->thumbstick, x, y)) {
		s->thumbstick.active |= id; return true;
	}

	for (i = 0; i < s->numDescs; i++) {
		if (!Widget_Contains(&s->btns[i], x, y)) continue;

		if (s->descs[i].bind < KEYBIND_COUNT) {
			Input_SetPressed(KeyBinds[s->descs[i].bind], true);
		} else {
			s->btns[i].MenuClick(screen, &s->btns[i]);
		}
		s->btns[i].active |= id;
		return true;
	}
	return false;
}

static int TouchScreen_PointerUp(void* screen, int id, int x, int y) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	int i;
	//Chat_Add1("POINTER UP: %i", &id);
	s->thumbstick.active &= ~id;

	for (i = 0; i < s->numDescs; i++) {
		if (!(s->btns[i].active & id)) continue;

		if (s->descs[i].bind < KEYBIND_COUNT) {
			Input_SetPressed(KeyBinds[s->descs[i].bind], false);
		}
		s->btns[i].active &= ~id;
		return true;
	}
	return false;
}

static void TouchScreen_GetMovement(float* xMoving, float* zMoving) {
	ThumbstickWidget_GetMovement(&TouchScreen.thumbstick, xMoving, zMoving);
}

struct LocalPlayerInput touchInput;
static void TouchScreen_Init(void* screen) {
	struct TouchScreen* s = (struct TouchScreen*)screen;

	s->widgets     = touch_widgets;
	s->maxVertices = TOUCH_MAX_VERTICES;
	Event_Register_(&UserEvents.HacksStateChanged, screen, TouchScreen_HacksChanged);

	TouchScreen_InitButtons(s);
	ThumbstickWidget_Init(&s->thumbstick);
	touchInput.GetMovement = TouchScreen_GetMovement;
	LocalPlayer_Instance.input.next = &touchInput;
}

static void TouchScreen_Layout(void* screen) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	const struct TouchBindDesc* desc;
	int i, height;

	/* Need to align these relative to the hotbar */
	HUDScreen_Layout(Gui_HUD);
	height = Gui_HUD->hotbar.height;

	for (i = 0; i < s->numDescs; i++) {
		desc = &s->descs[i];
		Widget_SetLocation(&s->btns[i], ANCHOR_MAX, ANCHOR_MAX, desc->x, desc->y);
		s->btns[i].yOffset += height;
		Widget_Layout(&s->btns[i]);
	}

	Widget_SetLocation(&s->thumbstick, ANCHOR_MIN, ANCHOR_MAX, 30, 5);
	s->thumbstick.yOffset += height;
	Widget_Layout(&s->thumbstick);
}

static void TouchScreen_Free(void* s) {
	Event_Unregister_(&UserEvents.HacksStateChanged, s, TouchScreen_HacksChanged);
}

static const struct ScreenVTABLE TouchScreen_VTABLE = {
	TouchScreen_Init,        Screen_NullUpdate,     TouchScreen_Free,
	TouchScreen_Render,      Screen_BuildMesh,
	Screen_FInput,           Screen_FInput,         Screen_FKeyPress, Screen_FText,
	TouchScreen_PointerDown, TouchScreen_PointerUp, Screen_FPointer,  Screen_FMouseScroll,
	TouchScreen_Layout,      TouchScreen_ContextLost, TouchScreen_ContextRecreated
};
void TouchScreen_Show(void) {
	struct TouchScreen* s = &TouchScreen;
	s->VTABLE = &TouchScreen_VTABLE;

	if (!Input_TouchMode) return;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_TOUCH);
}
#endif
