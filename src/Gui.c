#include "Gui.h"
#include "String.h"
#include "Window.h"
#include "Game.h"
#include "Graphics.h"
#include "Event.h"
#include "Drawer2D.h"
#include "ExtMath.h"
#include "Screens.h"
#include "Camera.h"
#include "Input.h"
#include "Logger.h"
#include "Platform.h"
#include "Bitmap.h"
#include "Options.h"
#include "Funcs.h"

struct _GuiData Gui;
struct HUDScreen*  Gui_HUD;
struct ChatScreen* Gui_Chat;
struct Screen* Gui_Screens[GUI_MAX_SCREENS];
static cc_uint8 priorities[GUI_MAX_SCREENS];


/*########################################################################################################################*
*----------------------------------------------------------Gui------------------------------------------------------------*
*#########################################################################################################################*/
static CC_NOINLINE int GetWindowScale(void) {
	float windowScale = min(WindowInfo.Width / 640.0f, WindowInfo.Height / 480.0f);
	return 1 + (int)windowScale;
}

float Gui_Scale(float value) {
	return (float)((int)(value * 10 + 0.5f)) / 10.0f;
}

float Gui_GetHotbarScale(void) {
	return Gui_Scale(GetWindowScale() * Gui.RawHotbarScale);
}

float Gui_GetInventoryScale(void) {
	return Gui_Scale(GetWindowScale() * (Gui.RawInventoryScale * 0.5f));
}

float Gui_GetChatScale(void) {
	return Gui_Scale(GetWindowScale() * Gui.RawChatScale);
}

int Gui_CalcPos(cc_uint8 anchor, int offset, int size, int axisLen) {
	if (anchor == ANCHOR_MIN) return offset;
	if (anchor == ANCHOR_MAX) return axisLen - size - offset;

	if (anchor == ANCHOR_CENTRE_MIN) return (axisLen / 2) + offset;
	if (anchor == ANCHOR_CENTRE_MAX) return (axisLen / 2) - size - offset;
	return (axisLen - size) / 2 + offset;
}

int Gui_Contains(int recX, int recY, int width, int height, int x, int y) {
	return x >= recX && y >= recY && x < (recX + width) && y < (recY + height);
}

int Gui_ContainsPointers(int x, int y, int width, int height) {
	int i, px, py;
	for (i = 0; i < Pointers_Count; i++) {
		px = Pointers[i].x;	py = Pointers[i].y;

		if (px >= x && py >= y && px < (x + width) && py < (y + height)) return true;
	}
	return false;
}

void Gui_ShowDefault(void) {
	HUDScreen_Show();
	ChatScreen_Show();
#ifdef CC_BUILD_TOUCH
	TouchScreen_Show();
#endif
}

static void Gui_LoadOptions(void) {
	Gui.DefaultLines    = Game_ClassicMode ? 10 : 12;
	Gui.Chatlines       = Options_GetInt(OPT_CHATLINES, 0, 30, Gui.DefaultLines);
	Gui.ClickableChat   = Options_GetBool(OPT_CLICKABLE_CHAT,   true) && !Game_ClassicMode;
	Gui.TabAutocomplete = Options_GetBool(OPT_TAB_AUTOCOMPLETE, true) && !Game_ClassicMode;

	Gui.ClassicTexture = Options_GetBool(OPT_CLASSIC_GUI, true)      || Game_ClassicMode;
	Gui.ClassicTabList = Options_GetBool(OPT_CLASSIC_TABLIST, false) || Game_ClassicMode;
	Gui.ClassicMenu    = Options_GetBool(OPT_CLASSIC_OPTIONS, false) || Game_ClassicMode;
	Gui.ClassicChat    = Options_GetBool(OPT_CLASSIC_CHAT, false)    || Game_PureClassic;
	Gui.ShowFPS        = Options_GetBool(OPT_SHOW_FPS, true);
	
	Gui.RawInventoryScale = Options_GetFloat(OPT_INVENTORY_SCALE, 0.25f, 5.0f, 1.0f);
	Gui.RawHotbarScale    = Options_GetFloat(OPT_HOTBAR_SCALE,    0.25f, 5.0f, 1.0f);
	Gui.RawChatScale      = Options_GetFloat(OPT_CHAT_SCALE,      0.35f, 5.0f, 1.0f);
}

static void LoseAllScreens(void) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		s->VTABLE->ContextLost(s);
	}
}

static void OnContextRecreated(void* obj) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		s->VTABLE->ContextRecreated(s);
	}
}

static void OnResize(void* obj) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		s->VTABLE->Layout(s);
		s->dirty = true;
	}
}

void Gui_RefreshAll(void) { 
	LoseAllScreens();
	OnContextRecreated(NULL);
	OnResize(NULL);
}

void Gui_RemoveAll(void) {
	while (Gui.ScreensCount) Gui_Remove(Gui_Screens[0]);
}

void Gui_RefreshChat(void) { Gui_Refresh((struct Screen*)Gui_Chat); }
void Gui_Refresh(struct Screen* s) {
	s->VTABLE->ContextLost(s);
	s->VTABLE->ContextRecreated(s);
	s->VTABLE->Layout(s);
	s->dirty = true;
}

static void Gui_AddCore(struct Screen* s, int priority) {
	int i, j;
	if (Gui.ScreensCount >= GUI_MAX_SCREENS) Logger_Abort("Hit max screens");

	for (i = 0; i < Gui.ScreensCount; i++) {
		if (priority <= priorities[i]) continue;

		/* Shift lower priority screens right */
		for (j = Gui.ScreensCount; j > i; j--) {
			Gui_Screens[j] = Gui_Screens[j - 1];
			priorities[j]  = priorities[j - 1];
		}
		break;
	}

	Gui_Screens[i] = s;
	priorities[i]  = priority;
	Gui.ScreensCount++;

	s->dirty = true;
	s->VTABLE->Init(s);
	s->VTABLE->ContextRecreated(s);
	s->VTABLE->Layout(s);

	/* for selecting active button etc */
	for (i = 0; i < Pointers_Count; i++) {
		s->VTABLE->HandlesPointerMove(s, i, Pointers[i].x, Pointers[i].y);
	}
}

/* Returns index of the given screen in the screens list, -1 if not */
static int IndexOfScreen(struct Screen* s) {
	int i;
	for (i = 0; i < Gui.ScreensCount; i++) {
		if (Gui_Screens[i] == s) return i;
	}
	return -1;
}

static void Gui_RemoveCore(struct Screen* s) {
	int i = IndexOfScreen(s);
	if (i == -1) return;

	for (; i < Gui.ScreensCount - 1; i++) {
		Gui_Screens[i] = Gui_Screens[i + 1];
		priorities[i]  = priorities[i  + 1];
	}
	Gui.ScreensCount--;

	s->VTABLE->ContextLost(s);
	s->VTABLE->Free(s);
}

CC_NOINLINE static void Gui_OnScreensChanged(void) {
	Camera_CheckFocus();
	InputHandler_OnScreensChanged();
}

void Gui_Remove(struct Screen* s) {
	Gui_RemoveCore(s);
	Gui_OnScreensChanged();
}

void Gui_Add(struct Screen* s, int priority) {
	int i;
	Gui_RemoveCore(s);
	/* Backwards loop since removing changes count and gui_screens */
	for (i = Gui.ScreensCount - 1; i >= 0; i--) {
		if (priorities[i] == priority) Gui_RemoveCore(Gui_Screens[i]);
	}

	Gui_AddCore(s, priority);
	Gui_OnScreensChanged();
}

struct Screen* Gui_GetInputGrab(void) {
	int i;
	for (i = 0; i < Gui.ScreensCount; i++) {
		if (Gui_Screens[i]->grabsInput) return Gui_Screens[i];
	}
	return NULL;
}

struct Screen* Gui_GetBlocksWorld(void) {
	int i;
	for (i = 0; i < Gui.ScreensCount; i++) {
		if (Gui_Screens[i]->blocksWorld) return Gui_Screens[i];
	}
	return NULL;
}

struct Screen* Gui_GetClosable(void) {
	int i;
	for (i = 0; i < Gui.ScreensCount; i++) {
		if (Gui_Screens[i]->closable) return Gui_Screens[i];
	}
	return NULL;
}

void Gui_RenderGui(double delta) {
	struct Screen* s;
	int i;

	/* Draw back to front so highest priority screen is on top */
	for (i = Gui.ScreensCount - 1; i >= 0; i--) {
		s = Gui_Screens[i];
		s->VTABLE->Update(s, delta);

		if (s->dirty) { s->VTABLE->BuildMesh(s); s->dirty = false; }
		s->VTABLE->Render(s, delta);
	}
}


/*########################################################################################################################*
*-------------------------------------------------------TextAtlas---------------------------------------------------------*
*#########################################################################################################################*/
void TextAtlas_Make(struct TextAtlas* atlas, const cc_string* chars, struct FontDesc* font, const cc_string* prefix) {
	struct DrawTextArgs args; 
	int width, height;
	struct Bitmap bmp;
	int i, charWidth;

	Gfx_DeleteTexture(&atlas->tex.ID);
	DrawTextArgs_Make(&args, prefix, font, true);
	width = Drawer2D_TextWidth(&args);
	atlas->offset = width;
	
	for (i = 0; i < chars->length; i++) {
		args.text = String_UNSAFE_Substring(chars, i, 1);
		charWidth = Drawer2D_TextWidth(&args);

		atlas->widths[i]  = charWidth;
		atlas->offsets[i] = width;
		/* add 1 pixel of padding */
		width += charWidth + 1;
	}
	height = Drawer2D_TextHeight(&args);

	Bitmap_AllocateClearedPow2(&bmp, width, height);
	{
		args.text = *prefix;
		Drawer2D_DrawText(&bmp, &args, 0, 0);	

		for (i = 0; i < chars->length; i++) {
			args.text = String_UNSAFE_Substring(chars, i, 1);
			Drawer2D_DrawText(&bmp, &args, atlas->offsets[i], 0);
		}
		Drawer2D_MakeTexture(&atlas->tex, &bmp, width, height);
	}	
	Mem_Free(bmp.scan0);

	atlas->uScale = 1.0f / (float)bmp.width;
	atlas->tex.uv.U2 = atlas->offset * atlas->uScale;
	atlas->tex.Width = atlas->offset;	
}

void TextAtlas_Free(struct TextAtlas* atlas) { Gfx_DeleteTexture(&atlas->tex.ID); }

void TextAtlas_Add(struct TextAtlas* atlas, int charI, struct VertexTextured** vertices) {
	struct Texture part = atlas->tex;
	int width       = atlas->widths[charI];
	PackedCol white = PACKEDCOL_WHITE;

	part.X  = atlas->curX; part.Width = width;
	part.uv.U1 = atlas->offsets[charI] * atlas->uScale;
	part.uv.U2 = part.uv.U1 + width    * atlas->uScale;

	atlas->curX += width;	
	Gfx_Make2DQuad(&part, white, vertices);
}

void TextAtlas_AddInt(struct TextAtlas* atlas, int value, struct VertexTextured** vertices) {
	char digits[STRING_INT_CHARS];
	int i, count;

	if (value < 0) {
		TextAtlas_Add(atlas, 10, vertices); value = -value; /* - sign */
	}
	count = String_MakeUInt32((cc_uint32)value, digits);

	for (i = count - 1; i >= 0; i--) {
		TextAtlas_Add(atlas, digits[i] - '0' , vertices);
	}
}


/*########################################################################################################################*
*-------------------------------------------------------Widget base-------------------------------------------------------*
*#########################################################################################################################*/
void Widget_SetLocation(void* widget, cc_uint8 horAnchor, cc_uint8 verAnchor, int xOffset, int yOffset) {
	struct Widget* w = (struct Widget*)widget;
	w->horAnchor = horAnchor; w->verAnchor = verAnchor;
	w->xOffset = Display_ScaleX(xOffset);
	w->yOffset = Display_ScaleY(yOffset);
	Widget_Layout(w);
}

void Widget_CalcPosition(void* widget) {
	struct Widget* w = (struct Widget*)widget;
	w->x = Gui_CalcPos(w->horAnchor, w->xOffset, w->width , WindowInfo.Width );
	w->y = Gui_CalcPos(w->verAnchor, w->yOffset, w->height, WindowInfo.Height);
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

int Widget_Contains(void* widget, int x, int y) {
	struct Widget* w = (struct Widget*)widget;
	return Gui_Contains(w->x, w->y, w->width, w->height, x, y);
}


/*########################################################################################################################*
*-------------------------------------------------------Screen base-------------------------------------------------------*
*#########################################################################################################################*/
void Screen_RenderWidgets(void* screen, double delta) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i;

	for (i = 0; i < s->numWidgets; i++) {
		if (!widgets[i]) continue;
		Elem_Render(widgets[i], delta);
	}
}

void Screen_Render2Widgets(void* screen, double delta) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i, offset = 0;

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindDynamicVb(s->vb);

	for (i = 0; i < s->numWidgets; i++) {
		if (!widgets[i]) continue;
		offset = Widget_Render2(widgets[i], offset);
	}
}

void Screen_Layout(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i;

	for (i = 0; i < s->numWidgets; i++) {
		if (!widgets[i]) continue;
		Widget_Layout(widgets[i]);
	}
}

void Screen_ContextLost(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i;
	Gfx_DeleteDynamicVb(&s->vb);

	for (i = 0; i < s->numWidgets; i++) {
		if (!widgets[i]) continue;
		Elem_Free(widgets[i]);
	}
}

void Screen_CreateVb(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	s->vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_TEXTURED, s->maxVertices);
}

struct VertexTextured* Screen_LockVb(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	return (struct VertexTextured*)Gfx_LockDynamicVb(s->vb,
										VERTEX_FORMAT_TEXTURED, s->maxVertices);
}

void Screen_BuildMesh(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	struct VertexTextured* data;
	struct VertexTextured** ptr;
	int i;

	data = Screen_LockVb(s);
	ptr  = &data;

	for (i = 0; i < s->numWidgets; i++) {
		if (!widgets[i]) continue;
		Widget_BuildMesh(widgets[i], ptr);
	}
	Gfx_UnlockDynamicVb(s->vb);
}


/*########################################################################################################################*
*------------------------------------------------------Gui component------------------------------------------------------*
*#########################################################################################################################*/
static void OnFontChanged(void* obj) { Gui_RefreshAll(); }

static void OnFileChanged(void* obj, struct Stream* stream, const cc_string* name) {
	if (String_CaselessEqualsConst(name, "gui.png")) {
		Game_UpdateTexture(&Gui.GuiTex, stream, name, NULL);
	} else if (String_CaselessEqualsConst(name, "gui_classic.png")) {
		Game_UpdateTexture(&Gui.GuiClassicTex, stream, name, NULL);
	} else if (String_CaselessEqualsConst(name, "icons.png")) {
		Game_UpdateTexture(&Gui.IconsTex, stream, name, NULL);
	} else if (String_CaselessEqualsConst(name, "touch.png")) {
		Game_UpdateTexture(&Gui.TouchTex, stream, name, NULL);
	}
}

static void OnKeyPress(void* obj, int keyChar) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		if (s->VTABLE->HandlesKeyPress(s, keyChar)) return;
	}
}

#ifdef CC_BUILD_TOUCH
static void OnTextChanged(void* obj, const cc_string* str) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		if (s->VTABLE->HandlesTextChanged(s, str)) return;
	}
}
#endif

static void OnContextLost(void* obj) {
	LoseAllScreens();
	if (Gfx.ManagedTextures) return;

	Gfx_DeleteTexture(&Gui.GuiTex);
	Gfx_DeleteTexture(&Gui.GuiClassicTex);
	Gfx_DeleteTexture(&Gui.IconsTex);
	Gfx_DeleteTexture(&Gui.TouchTex);
}

static void OnInit(void) {
	Gui.Screens = Gui_Screens; /* for plugins */
	Event_Register_(&ChatEvents.FontChanged,     NULL, OnFontChanged);
	Event_Register_(&TextureEvents.FileChanged,  NULL, OnFileChanged);
	Event_Register_(&GfxEvents.ContextLost,      NULL, OnContextLost);
	Event_Register_(&GfxEvents.ContextRecreated, NULL, OnContextRecreated);
	Event_Register_(&InputEvents.Press,          NULL, OnKeyPress);
#ifdef CC_BUILD_TOUCH
	Event_Register_(&InputEvents.TextChanged,    NULL, OnTextChanged);
#endif

	Event_Register_(&WindowEvents.Resized,       NULL, OnResize);
	Gui_LoadOptions();
	Gui_ShowDefault();
}

static void OnReset(void) {
	/* TODO:Should we reset all screens here.. ? */
}

static void OnFree(void) {
	Gui_RemoveAll();
	OnContextLost(NULL);
	OnReset();
}

struct IGameComponent Gui_Component = {
	OnInit,  /* Init  */
	OnFree,  /* Free  */
	OnReset, /* Reset */
	NULL, /* OnNewMap */
	NULL, /* OnNewMapLoaded */
};
