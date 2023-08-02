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
#include "Menus.h"
#include "Funcs.h"
#include "Server.h"
#include "TexturePack.h"

struct _GuiData Gui;
struct Screen* Gui_Screens[GUI_MAX_SCREENS];
static cc_uint8 priorities[GUI_MAX_SCREENS];


/*########################################################################################################################*
*----------------------------------------------------------Gui------------------------------------------------------------*
*#########################################################################################################################*/
static CC_NOINLINE int GetWindowScale(void) {
	float widthScale  = WindowInfo.Width  / 640.0f;
	float heightScale = WindowInfo.Height / 480.0f;

	/* Use larger UI scaling on mobile */
	/* TODO move this DPI scaling elsewhere.,. */
	if (!Input_TouchMode) {
		widthScale  /= DisplayInfo.ScaleX;
		heightScale /= DisplayInfo.ScaleY;
	}
	return 1 + (int)(min(widthScale, heightScale));
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

void Gui_MakeTitleFont(struct FontDesc* font) { Font_Make(font, 16, FONT_FLAGS_BOLD); }
void Gui_MakeBodyFont(struct FontDesc* font)  { Font_Make(font, 16, FONT_FLAGS_NONE); }

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
	for (i = 0; i < Pointers_Count; i++) 
	{
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

static void LoadOptions(void) {
	Gui.DefaultLines    = Game_ClassicMode ? 10 : 12;
	Gui.Chatlines       = Options_GetInt(OPT_CHATLINES, 0, GUI_MAX_CHATLINES, Gui.DefaultLines);
	Gui.ClickableChat   = !Game_ClassicMode && Options_GetBool(OPT_CLICKABLE_CHAT,   !Input_TouchMode);
	Gui.TabAutocomplete = !Game_ClassicMode && Options_GetBool(OPT_TAB_AUTOCOMPLETE, true);

	Gui.ClassicTexture   = Options_GetBool(OPT_CLASSIC_GUI,        true) || Game_ClassicMode;
	Gui.ClassicTabList   = Options_GetBool(OPT_CLASSIC_TABLIST,   false) || Game_ClassicMode;
	Gui.ClassicMenu      = Options_GetBool(OPT_CLASSIC_OPTIONS,   false) || Game_ClassicMode;
	Gui.ClassicChat      = Options_GetBool(OPT_CLASSIC_CHAT,      false) || Game_PureClassic;
	Gui.ClassicInventory = Options_GetBool(OPT_CLASSIC_INVENTORY, false) || Game_ClassicMode;
	Gui.ShowFPS          = Options_GetBool(OPT_SHOW_FPS, true);
	
	Gui.RawInventoryScale = Options_GetFloat(OPT_INVENTORY_SCALE, 0.25f, 5.0f, 1.0f);
	Gui.RawHotbarScale    = Options_GetFloat(OPT_HOTBAR_SCALE,    0.25f, 5.0f, 1.0f);
	Gui.RawChatScale      = Options_GetFloat(OPT_CHAT_SCALE,      0.25f, 5.0f, 1.0f);
	Gui.RawTouchScale     = Options_GetFloat(OPT_TOUCH_SCALE,     0.25f, 5.0f, 1.0f);
}

static void LoseAllScreens(void) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui.ScreensCount; i++) 
	{
		s = Gui_Screens[i];
		s->VTABLE->ContextLost(s);
	}
}

static void OnContextRecreated(void* obj) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui.ScreensCount; i++) 
	{
		s = Gui_Screens[i];
		s->VTABLE->ContextRecreated(s);
		s->dirty = true;
	}
}

static void OnResize(void* obj) { Gui_LayoutAll(); }
void Gui_LayoutAll(void) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui.ScreensCount; i++) 
	{
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

void Gui_Refresh(struct Screen* s) {
	s->VTABLE->ContextLost(s);
	s->VTABLE->ContextRecreated(s);
	s->VTABLE->Layout(s);
	s->dirty = true;
}

static void Gui_AddCore(struct Screen* s, int priority) {
	int i, j;
	if (Gui.ScreensCount >= GUI_MAX_SCREENS) Logger_Abort("Hit max screens");

	for (i = 0; i < Gui.ScreensCount; i++) 
	{
		if (priority <= priorities[i]) continue;

		/* Shift lower priority screens right */
		for (j = Gui.ScreensCount; j > i; j--) 
		{
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
	for (i = 0; i < Pointers_Count; i++) 
	{
		s->VTABLE->HandlesPointerMove(s, i, Pointers[i].x, Pointers[i].y);
	}
}

/* Returns index of the given screen in the screens list, -1 if not */
static int IndexOfScreen(struct Screen* s) {
	int i;
	for (i = 0; i < Gui.ScreensCount; i++)
	{
		if (Gui_Screens[i] == s) return i;
	}
	return -1;
}

void Gui_RemoveCore(struct Screen* s) {
	int i = IndexOfScreen(s);
	if (i == -1) return;

	for (; i < Gui.ScreensCount - 1; i++) 
	{
		Gui_Screens[i] = Gui_Screens[i + 1];
		priorities[i]  = priorities[i  + 1];
	}
	Gui.ScreensCount--;

	s->VTABLE->ContextLost(s);
	s->VTABLE->Free(s);
}

CC_NOINLINE static void Gui_OnScreensChanged(void) {
	Gui_UpdateInputGrab();
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
	for (i = Gui.ScreensCount - 1; i >= 0; i--) 
	{
		if (priorities[i] == priority) Gui_RemoveCore(Gui_Screens[i]);
	}

	Gui_AddCore(s, priority);
	Gui_OnScreensChanged();
}

struct Screen* Gui_GetInputGrab(void) {
	int i;
	for (i = 0; i < Gui.ScreensCount; i++) 
	{
		if (Gui_Screens[i]->grabsInput) return Gui_Screens[i];
	}
	return NULL;
}

struct Screen* Gui_GetBlocksWorld(void) {
	int i;
	for (i = 0; i < Gui.ScreensCount; i++) 
	{
		if (Gui_Screens[i]->blocksWorld) return Gui_Screens[i];
	}
	return NULL;
}

struct Screen* Gui_GetClosable(void) {
	int i;
	for (i = 0; i < Gui.ScreensCount; i++) 
	{
		if (Gui_Screens[i]->closable) return Gui_Screens[i];
	}
	return NULL;
}

void Gui_UpdateInputGrab(void) {
	Gui.InputGrab = Gui_GetInputGrab();
	Camera_CheckFocus();
}

void Gui_ShowPauseMenu(void) {
	if (Gui.ClassicMenu) {
		ClassicPauseScreen_Show();
	} else {
		PauseScreen_Show();
	}
}

void Gui_RenderGui(double delta) {
	struct Screen* s;
	int i;

	/* Draw back to front so highest priority screen is on top */
	for (i = Gui.ScreensCount - 1; i >= 0; i--) 
	{
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
	struct Context2D ctx;
	int width, height;
	int i, charWidth;

	Gfx_DeleteTexture(&atlas->tex.ID);
	DrawTextArgs_Make(&args, prefix, font, true);
	width = Drawer2D_TextWidth(&args);
	atlas->offset = width;
	
	for (i = 0; i < chars->length; i++) 
	{
		args.text = String_UNSAFE_Substring(chars, i, 1);
		charWidth = Drawer2D_TextWidth(&args);

		atlas->widths[i]  = charWidth;
		atlas->offsets[i] = width;
		/* add 1 pixel of padding */
		width += charWidth + 1;
	}
	height = Drawer2D_TextHeight(&args);

	Context2D_Alloc(&ctx, width, height);
	{
		args.text = *prefix;
		Context2D_DrawText(&ctx, &args, 0, 0);

		for (i = 0; i < chars->length; i++) 
		{
			args.text = String_UNSAFE_Substring(chars, i, 1);
			Context2D_DrawText(&ctx, &args, atlas->offsets[i], 0);
		}
		Context2D_MakeTexture(&atlas->tex, &ctx);
	}	
	Context2D_Free(&ctx);

	atlas->uScale = 1.0f / (float)ctx.bmp.width;
	atlas->tex.uv.U2 = atlas->offset * atlas->uScale;
	atlas->tex.Width = atlas->offset;	
}

void TextAtlas_Free(struct TextAtlas* atlas) { Gfx_DeleteTexture(&atlas->tex.ID); }

void TextAtlas_Add(struct TextAtlas* atlas, int charI, struct VertexTextured** vertices) {
	struct Texture part = atlas->tex;
	int width = atlas->widths[charI];

	part.X  = atlas->curX; part.Width = width;
	part.uv.U1 = atlas->offsets[charI] * atlas->uScale;
	part.uv.U2 = part.uv.U1 + width    * atlas->uScale;

	atlas->curX += width;	
	Gfx_Make2DQuad(&part, PACKEDCOL_WHITE, vertices);
}

void TextAtlas_AddInt(struct TextAtlas* atlas, int value, struct VertexTextured** vertices) {
	char digits[STRING_INT_CHARS];
	int i, count;

	if (value < 0) {
		TextAtlas_Add(atlas, 10, vertices); value = -value; /* - sign */
	}
	count = String_MakeUInt32((cc_uint32)value, digits);

	for (i = count - 1; i >= 0; i--) 
	{
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
	w->flags    = 0;
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

void Widget_SetDisabled(void* widget, int disabled) {
	struct Widget* w = (struct Widget*)widget;

	if (disabled) {
		w->flags |=  WIDGET_FLAG_DISABLED;
	} else {
		w->flags &= ~WIDGET_FLAG_DISABLED;
	}
}


/*########################################################################################################################*
*-------------------------------------------------------Screen base-------------------------------------------------------*
*#########################################################################################################################*/
void Screen_Render2Widgets(void* screen, double delta) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i, offset = 0;

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindDynamicVb(s->vb);

	for (i = 0; i < s->numWidgets; i++) 
	{
		if (!widgets[i]) continue;
		offset = Widget_Render2(widgets[i], offset);
	}
}

void Screen_UpdateVb(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	Gfx_RecreateDynamicVb(&s->vb, VERTEX_FORMAT_TEXTURED, s->maxVertices);
}

struct VertexTextured* Screen_LockVb(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	return (struct VertexTextured*)Gfx_LockDynamicVb(s->vb,
										VERTEX_FORMAT_TEXTURED, s->maxVertices);
}

int Screen_DoPointerDown(void* screen, int id, int x, int y) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i, count = s->numWidgets;

	/* iterate backwards (because last elements rendered are shown over others) */
	for (i = count - 1; i >= 0; i--) 
	{
		struct Widget* w = widgets[i];
		if (!w || !Widget_Contains(w, x, y)) continue;
		if (w->flags & WIDGET_FLAG_DISABLED) return i;

		if (w->MenuClick) {
			w->MenuClick(s, w);
		} else {
			Elem_HandlesPointerDown(w, id, x, y);
		}
		return i;
	}
	return -1;
}

int Screen_Index(void* screen, void* widget) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i;

	struct Widget* w = (struct Widget*)widget;
	for (i = 0; i < s->numWidgets; i++) 
	{
		if (widgets[i] == w) return i;
	}
	return -1;
}

void Screen_BuildMesh(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	struct VertexTextured* data;
	struct VertexTextured** ptr;
	int i;

	data = Screen_LockVb(s);
	ptr  = &data;

	for (i = 0; i < s->numWidgets; i++) 
	{
		if (!widgets[i]) continue;
		Widget_BuildMesh(widgets[i], ptr);
	}
	Gfx_UnlockDynamicVb(s->vb);
}

void Screen_Layout(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i;

	for (i = 0; i < s->numWidgets; i++) 
	{
		if (!widgets[i]) continue;
		Widget_Layout(widgets[i]);
	}
}

void Screen_ContextLost(void* screen) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i;
	Gfx_DeleteDynamicVb(&s->vb);

	for (i = 0; i < s->numWidgets; i++) 
	{
		if (!widgets[i]) continue;
		Elem_Free(widgets[i]);
	}
}

int  Screen_InputDown(void* screen, int key) { return key < CCKEY_F1 || key > CCKEY_F24; }
void Screen_InputUp(void*   screen, int key) { }
void Screen_PointerUp(void* s, int id, int x, int y) { }


/*########################################################################################################################*
*------------------------------------------------------Gui component------------------------------------------------------*
*#########################################################################################################################*/
static void GuiPngProcess(struct Stream* stream, const cc_string* name) {
	Game_UpdateTexture(&Gui.GuiTex, stream, name, NULL);
}
static struct TextureEntry gui_entry = { "gui.png", GuiPngProcess };

static void GuiClassicPngProcess(struct Stream* stream, const cc_string* name) {
	Game_UpdateTexture(&Gui.GuiClassicTex, stream, name, NULL);
}
static struct TextureEntry guiClassic_entry = { "gui_classic.png", GuiClassicPngProcess };

static void IconsPngProcess(struct Stream* stream, const cc_string* name) {
	Game_UpdateTexture(&Gui.IconsTex, stream, name, NULL);
}
static struct TextureEntry icons_entry = { "icons.png", IconsPngProcess };

static void TouchPngProcess(struct Stream* stream, const cc_string* name) {
	Game_UpdateTexture(&Gui.TouchTex, stream, name, NULL);
}
static struct TextureEntry touch_entry = { "touch.png", TouchPngProcess };


static void OnFontChanged(void* obj) { Gui_RefreshAll(); }

static void OnKeyPress(void* obj, int cp) {
	struct Screen* s;
	int i; 
	char c;
	if (!Convert_TryCodepointToCP437(cp, &c)) return;

	for (i = 0; i < Gui.ScreensCount; i++) 
	{
		s = Gui_Screens[i];
		if (s->VTABLE->HandlesKeyPress(s, c)) return;
	}
}

#ifdef CC_BUILD_TOUCH
static void OnTextChanged(void* obj, const cc_string* str) {
	struct Screen* s;
	int i;

	for (i = 0; i < Gui.ScreensCount; i++) 
	{
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
	TextureEntry_Register(&gui_entry);
	TextureEntry_Register(&guiClassic_entry);
	TextureEntry_Register(&icons_entry);
	TextureEntry_Register(&touch_entry);

	Event_Register_(&ChatEvents.FontChanged,     NULL, OnFontChanged);
	Event_Register_(&GfxEvents.ContextLost,      NULL, OnContextLost);
	Event_Register_(&GfxEvents.ContextRecreated, NULL, OnContextRecreated);
	Event_Register_(&InputEvents.Press,          NULL, OnKeyPress);
	Event_Register_(&WindowEvents.Resized,       NULL, OnResize);

#ifdef CC_BUILD_TOUCH
	Event_Register_(&InputEvents.TextChanged,    NULL, OnTextChanged);

	#define DEFAULT_SP_ONSCREEN (ONSCREEN_BTN_FLY | ONSCREEN_BTN_SPEED)
	#define DEFAULT_MP_ONSCREEN (ONSCREEN_BTN_FLY | ONSCREEN_BTN_SPEED | ONSCREEN_BTN_CHAT)
	Gui._onscreenButtons = Options_GetInt(OPT_TOUCH_BUTTONS, 0, Int32_MaxValue,
											Server.IsSinglePlayer ? DEFAULT_SP_ONSCREEN : DEFAULT_MP_ONSCREEN);
#endif

	LoadOptions();
	Gui_ShowDefault();
}

static void OnReset(void) {
	/* TODO:Should we reset all screens here.. ? */
}

static void OnFree(void) {
	while (Gui.ScreensCount) Gui_Remove(Gui_Screens[0]);

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
