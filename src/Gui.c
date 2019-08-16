#include "Gui.h"
#include "Window.h"
#include "Game.h"
#include "Graphics.h"
#include "Event.h"
#include "Drawer2D.h"
#include "ExtMath.h"
#include "Screens.h"
#include "Camera.h"
#include "InputHandler.h"
#include "Logger.h"
#include "Platform.h"
#include "Bitmap.h"

bool Gui_ClassicTexture, Gui_ClassicTabList, Gui_ClassicMenu;
int  Gui_Chatlines;
bool Gui_ClickableChat, Gui_TabAutocomplete, Gui_ShowFPS;

GfxResourceID Gui_GuiTex, Gui_GuiClassicTex, Gui_IconsTex;
struct Screen* Gui_Status;
struct Screen* Gui_HUD;
struct Screen* Gui_Active;
struct Screen* Gui_Overlays[GUI_MAX_OVERLAYS];
struct Screen* Gui_Screens[GUI_MAX_SCREENS];
int Gui_ScreensCount, Gui_OverlaysCount;
static uint8_t priorities[GUI_MAX_SCREENS];

void Widget_SetLocation(void* widget, uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset) {
	struct Widget* w = (struct Widget*)widget;
	w->horAnchor = horAnchor; w->verAnchor = verAnchor;
	w->xOffset   = xOffset;   w->yOffset   = yOffset;
	Widget_Reposition(w);
}

void Widget_CalcPosition(void* widget) {
	struct Widget* w = (struct Widget*)widget;
	w->x = Gui_CalcPos(w->horAnchor, w->xOffset, w->width , Window_Width );
	w->y = Gui_CalcPos(w->verAnchor, w->yOffset, w->height, Window_Height);
}

void Widget_Reset(void* widget) {
	struct Widget* w = (struct Widget*)widget;
	w->active   = false;
	w->disabled = false;
	w->x = 0; w->y = 0;
	w->width = 0; w->height = 0;
	w->horAnchor = ANCHOR_MIN;
	w->verAnchor = ANCHOR_MIN;
	w->xOffset = 0; w->yOffset = 0;
	w->MenuClick = NULL;
}

bool Widget_Contains(void* widget, int x, int y) {
	struct Widget* w = (struct Widget*)widget;
	return Gui_Contains(w->x, w->y, w->width, w->height, x, y);
}


/*########################################################################################################################*
*----------------------------------------------------------Gui------------------------------------------------------------*
*#########################################################################################################################*/
int Gui_CalcPos(uint8_t anchor, int offset, int size, int axisLen) {
	if (anchor == ANCHOR_MIN) return offset;
	if (anchor == ANCHOR_MAX) return axisLen - size - offset;

	if (anchor == ANCHOR_CENTRE_MIN) return (axisLen / 2) + offset;
	if (anchor == ANCHOR_CENTRE_MAX) return (axisLen / 2) - size - offset;
	return (axisLen - size) / 2 + offset;
}

bool Gui_Contains(int recX, int recY, int width, int height, int x, int y) {
	return x >= recX && y >= recY && x < (recX + width) && y < (recY + height);
}

static void Gui_ContextLost(void* obj) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		s->VTABLE->ContextLost(s);
	}
}

static void Gui_ContextRecreated(void* obj) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		s->VTABLE->ContextRecreated(s);
	}
}

static void Gui_LoadOptions(void) {
	Gui_Chatlines       = Options_GetInt(OPT_CHATLINES, 0, 30, 12);
	Gui_ClickableChat   = Options_GetBool(OPT_CLICKABLE_CHAT,   !Game_ClassicMode);
	Gui_TabAutocomplete = Options_GetBool(OPT_TAB_AUTOCOMPLETE, !Game_ClassicMode);

	Gui_ClassicTexture = Options_GetBool(OPT_CLASSIC_GUI, true)      || Game_ClassicMode;
	Gui_ClassicTabList = Options_GetBool(OPT_CLASSIC_TABLIST, false) || Game_ClassicMode;
	Gui_ClassicMenu    = Options_GetBool(OPT_CLASSIC_OPTIONS, false) || Game_ClassicMode;

	Gui_ShowFPS = Options_GetBool(OPT_SHOW_FPS, true);
}

static void Gui_FontChanged(void* obj) { Gui_RefreshAll(); }

static void Gui_FileChanged(void* obj, struct Stream* stream, const String* name) {
	if (String_CaselessEqualsConst(name, "gui.png")) {
		Game_UpdateTexture(&Gui_GuiTex, stream, name, NULL);
	} else if (String_CaselessEqualsConst(name, "gui_classic.png")) {
		Game_UpdateTexture(&Gui_GuiClassicTex, stream, name, NULL);
	} else if (String_CaselessEqualsConst(name, "icons.png")) {
		Game_UpdateTexture(&Gui_IconsTex, stream, name, NULL);
	}
}

static void Gui_Init(void) {
	Event_RegisterVoid(&ChatEvents.FontChanged,     NULL, Gui_FontChanged);
	Event_RegisterEntry(&TextureEvents.FileChanged, NULL, Gui_FileChanged);
	Event_RegisterVoid(&GfxEvents.ContextLost,      NULL, Gui_ContextLost);
	Event_RegisterVoid(&GfxEvents.ContextRecreated, NULL, Gui_ContextRecreated);
	Gui_LoadOptions();

	StatusScreen_Show();
	HUDScreen_Show();
}

static void Gui_Reset(void) {
	int i;
	for (i = 0; i < Gui_OverlaysCount; i++) {
		Elem_TryFree(Gui_Overlays[i]);
	}
	Gui_OverlaysCount = 0;
}

static void Gui_Free(void) {
	Event_UnregisterVoid(&ChatEvents.FontChanged,     NULL, Gui_FontChanged);
	Event_UnregisterEntry(&TextureEvents.FileChanged, NULL, Gui_FileChanged);
	Event_UnregisterVoid(&GfxEvents.ContextLost,      NULL, Gui_ContextLost);
	Event_UnregisterVoid(&GfxEvents.ContextRecreated, NULL, Gui_ContextRecreated);

	while (Gui_ScreensCount) Gui_Remove(Gui_Screens[0]);

	Gfx_DeleteTexture(&Gui_GuiTex);
	Gfx_DeleteTexture(&Gui_GuiClassicTex);
	Gfx_DeleteTexture(&Gui_IconsTex);
	Gui_Reset();
}

struct IGameComponent Gui_Component = {
	Gui_Init,  /* Init  */
	Gui_Free,  /* Free  */
	Gui_Reset, /* Reset */
	NULL, /* OnNewMap */
	NULL, /* OnNewMapLoaded */
};

struct Screen* Gui_GetActiveScreen(void) {
	return Gui_OverlaysCount ? Gui_Overlays[0] : Gui_GetUnderlyingScreen();
}

struct Screen* Gui_GetUnderlyingScreen(void) {
	return Gui_Active ? Gui_Active : Gui_HUD;
}

void Gui_FreeActive(void) {
	if (Gui_Active) { Elem_TryFree(Gui_Active); }
}
void Gui_Close(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	if (s) { Elem_TryFree(s); }
	if (s == Gui_Active) Gui_SetActive(NULL);
}

void Gui_CloseActive(void) { Gui_Close(Gui_Active); }

void Gui_SetActive(struct Screen* screen) {
	InputHandler_ScreenChanged(Gui_Active, screen);
	Gui_Active = screen;

	if (screen) { 
		Elem_Init(screen);
		/* for selecting active button etc */
		Elem_HandlesMouseMove(screen, Mouse_X, Mouse_Y);
	}
	Camera_CheckFocus();
}

void Gui_RefreshAll(void) { 
	Gui_ContextLost(NULL);
	Gui_ContextRecreated(NULL);
}

void Gui_RefreshHud(void) {
	Gui_HUD->VTABLE->ContextLost(Gui_HUD);
	Gui_HUD->VTABLE->ContextRecreated(Gui_HUD);
}

int Gui_Index(struct Screen* s) {
	int i;
	for (i = 0; i < Gui_ScreensCount; i++) {
		if (Gui_Screens[i] == s) return i;
	}
	return -1;
}

static void Gui_AddCore(struct Screen* s, int priority) {
	int i, j;
	if (Gui_ScreensCount >= GUI_MAX_SCREENS) Logger_Abort("Hit max screens");

	for (i = 0; i < Gui_ScreensCount; i++) {
		if (priority <= priorities[i]) continue;

		/* Shift lower priority screens right */
		for (j = Gui_ScreensCount; j > i; j--) {
			Gui_Screens[j] = Gui_Screens[j - 1];
			priorities[j]  = priorities[j - 1];
		}
		break;
	}

	Gui_Screens[i] = s;
	priorities[i]  = priority;
	Gui_ScreensCount++;

	s->VTABLE->Init(s);
	s->VTABLE->ContextRecreated(s);
	/* for selecting active button etc */
	s->VTABLE->HandlesMouseMove(s, Mouse_X, Mouse_Y);
}

static void Gui_RemoveCore(struct Screen* s) {
	int i = Gui_Index(s);
	if (i == -1) return;

	for (; i < Gui_ScreensCount - 1; i++) {
		Gui_Screens[i] = Gui_Screens[i + 1];
		priorities[i]  = priorities[i  + 1];
	}
	Gui_ScreensCount--;

	s->VTABLE->ContextLost(s);
	s->VTABLE->Free(s);
}

void Gui_Add(struct Screen* s, int priority) {
	Gui_AddCore(s, priority);
	Camera_CheckFocus();
}

void Gui_Remove(struct Screen* s) {
	Gui_RemoveCore(s);
	Camera_CheckFocus();
}

void Gui_Replace(struct Screen* s, int priority) {
	int i;
	Gui_RemoveCore(s);
	/* Backwards loop since removing changes count and gui_screens */
	for (i = Gui_ScreensCount - 1; i >= 0; i--) {
		if (priorities[i] == priority) Gui_RemoveCore(Gui_Screens[i]);
	}

	Gui_AddCore(s, priority);
	Camera_CheckFocus();
}

struct Screen* Gui_GetInputGrab(void) {
	int i;
	for (i = 0; i < Gui_ScreensCount; i++) {
		if (Gui_Screens[i]->grabsInput) return Gui_Screens[i];
	}
	return NULL;
}

struct Screen* Gui_GetBlocksWorld(void) {
	int i;
	for (i = 0; i < Gui_ScreensCount; i++) {
		if (Gui_Screens[i]->blocksWorld) return Gui_Screens[i];
	}
	return NULL;
}

struct Screen* Gui_GetClosable(void) {
	int i;
	for (i = 0; i < Gui_ScreensCount; i++) {
		if (Gui_Screens[i]->closable) return Gui_Screens[i];
	}
	return NULL;
}

int Gui_IndexOverlay(const void* screen) {
	int i;

	for (i = 0; i < Gui_OverlaysCount; i++) {
		if (Gui_Overlays[i] == screen) return i;
	}
	return -1;
}

void Gui_RemoveOverlay(const void* screen) {
	int i = Gui_IndexOverlay(screen);
	if (i == -1) return;

	for (; i < Gui_OverlaysCount - 1; i++) {
		Gui_Overlays[i] = Gui_Overlays[i + 1];
	}

	Gui_OverlaysCount--;
	Gui_Overlays[Gui_OverlaysCount] = NULL;
	Camera_CheckFocus();
}

void Gui_RenderGui(double delta) {
	struct Screen* s;
	int i;
	Gfx_Mode2D(Game.Width, Game.Height);

	/* Draw back to front so highest priority screen is on top */
	for (i = Gui_ScreensCount - 1; i >= 0; i--) {
		s = Gui_Screens[i];
		if (!s->hidden) Elem_Render(s, delta);
	}
	Gfx_Mode3D();
}

void Gui_OnResize(void) {
	int i;
	for (i = 0; i < Gui_ScreensCount; i++) {
		Screen_OnResize(Gui_Screens[i]);
	}
}


/*########################################################################################################################*
*-------------------------------------------------------TextAtlas---------------------------------------------------------*
*#########################################################################################################################*/
void TextAtlas_Make(struct TextAtlas* atlas, const String* chars, const FontDesc* font, const String* prefix) {
	struct DrawTextArgs args; 
	Size2D size;
	Bitmap bmp;
	int i, width;

	DrawTextArgs_Make(&args, prefix, font, true);
	size = Drawer2D_MeasureText(&args);
	atlas->offset = size.Width;
	
	width = atlas->offset;
	for (i = 0; i < chars->length; i++) {
		args.text = String_UNSAFE_Substring(chars, i, 1);
		size = Drawer2D_MeasureText(&args);

		atlas->widths[i]  = size.Width;
		atlas->offsets[i] = width;
		/* add 1 pixel of padding */
		width += size.Width + 1;
	}

	Bitmap_AllocateClearedPow2(&bmp, width, size.Height);
	{
		args.text = *prefix;
		Drawer2D_DrawText(&bmp, &args, 0, 0);	

		for (i = 0; i < chars->length; i++) {
			args.text = String_UNSAFE_Substring(chars, i, 1);
			Drawer2D_DrawText(&bmp, &args, atlas->offsets[i], 0);
		}
		Drawer2D_Make2DTexture(&atlas->tex, &bmp, size, 0, 0);
	}	
	Mem_Free(bmp.Scan0);

	Drawer2D_ReducePadding_Tex(&atlas->tex, font->Size, 4);
	atlas->uScale = 1.0f / (float)bmp.Width;
	atlas->tex.uv.U2 = atlas->offset * atlas->uScale;
	atlas->tex.Width = atlas->offset;	
}

void TextAtlas_Free(struct TextAtlas* atlas) { Gfx_DeleteTexture(&atlas->tex.ID); }

void TextAtlas_Add(struct TextAtlas* atlas, int charI, VertexP3fT2fC4b** vertices) {
	struct Texture part = atlas->tex;
	int width       = atlas->widths[charI];
	PackedCol white = PACKEDCOL_WHITE;

	part.X  = atlas->curX; part.Width = width;
	part.uv.U1 = atlas->offsets[charI] * atlas->uScale;
	part.uv.U2 = part.uv.U1 + width    * atlas->uScale;

	atlas->curX += width;	
	Gfx_Make2DQuad(&part, white, vertices);
}

void TextAtlas_AddInt(struct TextAtlas* atlas, int value, VertexP3fT2fC4b** vertices) {
	char digits[STRING_INT_CHARS];
	int i, count;

	if (value < 0) {
		TextAtlas_Add(atlas, 10, vertices); value = -value; /* - sign */
	}
	count = String_MakeUInt32((uint32_t)value, digits);

	for (i = count - 1; i >= 0; i--) {
		TextAtlas_Add(atlas, digits[i] - '0' , vertices);
	}
}
