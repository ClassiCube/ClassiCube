#include "LBackend.h"
#if defined CC_BUILD_WEB
/* Web backend doesn't use the launcher */
#elif defined CC_BUILD_WIN_TEST
/* Testing windows UI backend */
#include "LBackend_Win.c"
#elif defined CC_BUILD_IOS
/* iOS uses custom UI backend */
#else
#include "Launcher.h"
#include "Drawer2D.h"
#include "Window.h"
#include "LWidgets.h"
#include "String.h"
#include "Gui.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "ExtMath.h"
#include "Window.h"
#include "Funcs.h"
#include "LWeb.h"
#include "Platform.h"
#include "LScreens.h"
#include "Input.h"
#include "Utils.h"
#include "Event.h"

struct FontDesc titleFont, textFont, hintFont, logoFont, rowFont;
/* Contains the pixels that are drawn to the window */
static struct Context2D framebuffer;
/* The area/region of the window that needs to be redrawn and presented to the screen. */
/* If width is 0, means no area needs to be redrawn. */
static Rect2D dirty_rect;

static cc_uint8 pendingRedraw;
#define REDRAW_ALL  0x02
#define REDRAW_SOME 0x01

static int xBorder, xBorder2, xBorder3, xBorder4;
static int yBorder, yBorder2, yBorder3, yBorder4;
static int xInputOffset, yInputOffset, inputExpand;
static int caretOffset, caretWidth, caretHeight;
static int scrollbarWidth, dragPad, gridlineWidth, gridlineHeight;
static int hdrYOffset, hdrYPadding, rowYOffset, rowYPadding;
static int cellXOffset, cellXPadding, cellMinWidth;
static int flagXOffset, flagYOffset;

static void HookEvents(void);
void LBackend_Init(void) {
	xBorder = Display_ScaleX(1); xBorder2 = xBorder * 2; xBorder3 = xBorder * 3; xBorder4 = xBorder * 4;
	yBorder = Display_ScaleY(1); yBorder2 = yBorder * 2; yBorder3 = yBorder * 3; yBorder4 = yBorder * 4;

	xInputOffset = Display_ScaleX(5);
	yInputOffset = Display_ScaleY(2);
	inputExpand  = Display_ScaleX(20);

	caretOffset  = Display_ScaleY(5);
	caretWidth   = Display_ScaleX(10);
	caretHeight  = Display_ScaleY(2);

	scrollbarWidth = Display_ScaleX(10);
	dragPad        = Display_ScaleX(8);
	gridlineWidth  = Display_ScaleX(2);
	gridlineHeight = Display_ScaleY(2);

	hdrYOffset   = Display_ScaleY(3);
	hdrYPadding  = Display_ScaleY(5);
	rowYOffset   = Display_ScaleY(3);
	rowYPadding  = Display_ScaleY(1);

	cellXOffset  = Display_ScaleX(6);
	cellXPadding = Display_ScaleX(5);
	cellMinWidth = Display_ScaleX(20);
	flagXOffset  = Display_ScaleX(2);
	flagYOffset  = Display_ScaleY(6);

	Font_Make(&titleFont, 16, FONT_FLAGS_BOLD);
	Font_Make(&textFont,  14, FONT_FLAGS_NONE);
	Font_Make(&hintFont,  12, FONT_FLAGS_NONE);
	HookEvents();
}

void LBackend_Free(void) {
	Font_Free(&titleFont);
	Font_Free(&textFont);
	Font_Free(&hintFont);
	Font_Free(&logoFont);
	Font_Free(&rowFont);
}

void LBackend_UpdateLogoFont(void) {
	Font_Free(&logoFont);
	Launcher_MakeLogoFont(&logoFont);
}
void LBackend_DrawLogo(struct Context2D* ctx, const char* title) {
	Launcher_DrawLogo(&logoFont, title, ctx);
}

static void OnPointerMove(void* obj, int idx);
void LBackend_SetScreen(struct LScreen* s) {
	int i;
	/* for hovering over active button etc */
	for (i = 0; i < Pointers_Count; i++) {
		OnPointerMove(s, i);
	}
}

void LBackend_CloseScreen(struct LScreen* s) { }

static void LBackend_LayoutDimensions(struct LWidget* w) {
	const struct LLayout* l = w->layouts + 2;
	while (l->type)
	{
		switch (l->type)
		{
		case LLAYOUT_WIDTH:
			w->width  = WindowInfo.Width  - w->x - Display_ScaleX(l->offset);
			w->width  = max(1, w->width);
			break;
		case LLAYOUT_HEIGHT:
			w->height = WindowInfo.Height - w->y - Display_ScaleY(l->offset);
			w->height = max(1, w->height);
			break;
		}
		l++;
	}
}

void LBackend_LayoutWidget(struct LWidget* w) {
	const struct LLayout* l = w->layouts;

	w->x = Gui_CalcPos(l[0].type & 0xFF, Display_ScaleX(l[0].offset), w->width,  WindowInfo.Width);
	w->y = Gui_CalcPos(l[1].type & 0xFF, Display_ScaleY(l[1].offset), w->height, WindowInfo.Height);

	/* e.g. Table widget needs adjusts width/height based on window */
	if (l[1].type & LLAYOUT_EXTRA)
		LBackend_LayoutDimensions(w);

	if (w->type != LWIDGET_TABLE) return;
	LBackend_TableReposition((struct LTable*)w);
}

void LBackend_MarkDirty(void* widget) {
	struct LWidget* w = (struct LWidget*)widget;
	pendingRedraw |= REDRAW_SOME;
	w->dirty = true;
}

/* Marks the entire window as needing to be redrawn. */
static CC_NOINLINE void MarkAllDirty(void) {
	dirty_rect.X = 0; dirty_rect.Width  = framebuffer.width;
	dirty_rect.Y = 0; dirty_rect.Height = framebuffer.height;
}

/* Marks the given area/region as needing to be redrawn. */
static CC_NOINLINE void MarkAreaDirty(int x, int y, int width, int height) {
	int x1, y1, x2, y2;
	if (!Drawer2D_Clamp(&framebuffer, &x, &y, &width, &height)) return;

	/* union with existing dirty area */
	if (dirty_rect.Width) {
		x1 = min(x, dirty_rect.X);
		y1 = min(y, dirty_rect.Y);

		x2 = max(x +  width, dirty_rect.X + dirty_rect.Width);
		y2 = max(y + height, dirty_rect.Y + dirty_rect.Height);

		x = x1; width  = x2 - x1;
		y = y1; height = y2 - y1;
	}

	dirty_rect.X = x; dirty_rect.Width  = width;
	dirty_rect.Y = y; dirty_rect.Height = height;
}

void LBackend_InitFramebuffer(void) {
	struct Bitmap bmp;
	bmp.width  = max(WindowInfo.Width,  1);
	bmp.height = max(WindowInfo.Height, 1);

	Window_AllocFramebuffer(&bmp);
	Context2D_Wrap(&framebuffer, &bmp);
}

void LBackend_FreeFramebuffer(void) {
	Window_FreeFramebuffer(&framebuffer.bmp);
}


/*########################################################################################################################*
*------------------------------------------------------Base drawing-------------------------------------------------------*
*#########################################################################################################################*/
static void DrawBoxBounds(BitmapCol color, int x, int y, int width, int height) {
	Context2D_Clear(&framebuffer, color,
		x,                   y, 
		width,               yBorder);
	Context2D_Clear(&framebuffer, color,
		x,                   y + height - yBorder,
		width,               yBorder);
	Context2D_Clear(&framebuffer, color,
		x,                   y, 
		xBorder,             height);
	Context2D_Clear(&framebuffer, color,
		x + width - xBorder, y, 
		xBorder,             height);
}

static CC_NOINLINE void DrawWidget(struct LWidget* w) {
	w->last.X = w->x; w->last.Width  = w->width;
	w->last.Y = w->y; w->last.Height = w->height;

	w->dirty = false;
	w->VTABLE->Draw(w);
	MarkAreaDirty(w->x, w->y, w->width, w->height);
}

static CC_NOINLINE void RedrawAll(void) {
	struct LScreen* s = Launcher_Active;
	int i;
	s->DrawBackground(s, &framebuffer);
	
	for (i = 0; i < s->numWidgets; i++) {
		DrawWidget(s->widgets[i]);
	}
	MarkAllDirty();
}

static CC_NOINLINE void RedrawDirty(void) {
	struct LScreen* s = Launcher_Active;
	struct LWidget* w;
	int i;
	
	for (i = 0; i < s->numWidgets; i++) {
		w = s->widgets[i];
		if (!w->dirty) continue;

		/* check if widget might need redrawing of background behind */
		if (!w->opaque || w->last.Width > w->width || w->last.Height > w->height) {
			s->ResetArea(&framebuffer,
						  w->last.X, w->last.Y, w->last.Width, w->last.Height);
			MarkAreaDirty(w->last.X, w->last.Y, w->last.Width, w->last.Height);
		}
		DrawWidget(w);
	}
}

static CC_NOINLINE void DoRedraw(void) {
	if (pendingRedraw & REDRAW_ALL) {
		RedrawAll();
		pendingRedraw = 0;
	} else if (pendingRedraw & REDRAW_SOME) {
		RedrawDirty();
		pendingRedraw = 0;
	}
}

void LBackend_Redraw(void) {
	pendingRedraw = REDRAW_ALL;
	MarkAllDirty();
}
void LBackend_ThemeChanged(void) { LBackend_Redraw(); }

void LBackend_Tick(void) {
	DoRedraw();
	if (!dirty_rect.Width) return;

	Window_DrawFramebuffer(dirty_rect);
	dirty_rect.X = 0; dirty_rect.Width   = 0;
	dirty_rect.Y = 0; dirty_rect.Height  = 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Event handling------------------------------------------------------*
*#########################################################################################################################*/
static void ReqeustRedraw(void* obj)  { LBackend_Redraw(); }
static void RedrawContents(void* obj) { DoRedraw(); }

CC_NOINLINE static struct LWidget* GetWidgetAt(struct LScreen* s, int idx) {
	struct LWidget* w;
	int i, x = Pointers[idx].x, y = Pointers[idx].y;

	for (i = 0; i < s->numWidgets; i++) {
		w = s->widgets[i];
		if (Gui_Contains(w->x, w->y, w->width, w->height, x, y)) return w;
	}
	return NULL;
}

static void OnPointerDown(void* obj, int idx) {
	struct LScreen* s = Launcher_Active;
	struct LWidget* over;
	struct LWidget* prev;

	if (!s) return;
	over = GetWidgetAt(s, idx);
	prev = s->selectedWidget;

	if (prev && over != prev) LScreen_UnselectWidget(s, idx, prev);
	if (over) LScreen_SelectWidget(s, idx, over, over == prev);
}

static void OnPointerUp(void* obj, int idx) {
	struct LScreen* s = Launcher_Active;
	struct LWidget* over;
	struct LWidget* prev;

	if (!s) return;
	over = GetWidgetAt(s, idx);
	prev = s->selectedWidget;

	/* if user moves mouse away, it doesn't count */
	if (over != prev) {
		LScreen_UnselectWidget(s, idx, prev);
	} else if (over && over->OnClick) {
		over->OnClick(over);
	}
	/* TODO eliminate this hack */
	s->MouseUp(s, idx);
}

static void OnPointerMove(void* obj, int idx) {
	struct LScreen* s = Launcher_Active;
	struct LWidget* over;
	struct LWidget* prev;
	cc_bool overSame;

	if (!s) return;
	over = GetWidgetAt(s, idx);
	prev = s->hoveredWidget;
	overSame = prev == over;

	if (prev && !overSame) {
		prev->hovered    = false;
		s->hoveredWidget = NULL;

		if (prev->OnUnhover) prev->OnUnhover(prev);
		if (prev->VTABLE->MouseLeft) prev->VTABLE->MouseLeft(prev);
	}

	if (over) {
		over->hovered    = true;
		s->hoveredWidget = over;

		if (over->OnHover) over->OnHover(over);
		if (!over->VTABLE->MouseMove) return;
		over->VTABLE->MouseMove(over, idx, overSame);
	}
}

static void OnKeyPress(void* obj, int cp) {
	struct LWidget* selected;
	char c;
	if (!Convert_TryCodepointToCP437(cp, &c)) return;

	selected = Launcher_Active->selectedWidget;
	if (!selected) return;

	if (!selected->VTABLE->KeyPress) return;
	selected->VTABLE->KeyPress(selected, c);
}

static void OnTextChanged(void* obj, const cc_string* str) {
	struct LWidget* selected = Launcher_Active->selectedWidget;
	if (!selected) return;

	if (!selected->VTABLE->TextChanged) return;
	selected->VTABLE->TextChanged(selected, str);
}

static void HookEvents(void) {
	Event_Register_(&PointerEvents.Down,  NULL, OnPointerDown);
	Event_Register_(&PointerEvents.Up,    NULL, OnPointerUp);
	Event_Register_(&PointerEvents.Moved, NULL, OnPointerMove);
	
	Event_Register_(&InputEvents.Press,         NULL, OnKeyPress);
	Event_Register_(&InputEvents.TextChanged,   NULL, OnTextChanged);

	Event_Register_(&WindowEvents.RedrawNeeded, NULL, ReqeustRedraw);
	Event_Register_(&WindowEvents.Redrawing,    NULL, RedrawContents);
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_ButtonInit(struct LButton* w, int width, int height) {	
	w->width  = Display_ScaleX(width);
	w->height = Display_ScaleY(height);
}

void LBackend_ButtonUpdate(struct LButton* w) {
	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &w->text, &titleFont, true);
	LBackend_MarkDirty(w);

	w->_textWidth  = Drawer2D_TextWidth(&args);
	w->_textHeight = Drawer2D_TextHeight(&args);
}

void LBackend_ButtonDraw(struct LButton* w) {
	struct DrawTextArgs args;
	int xOffset, yOffset;
	cc_bool active = w->hovered || w->selected;

	LButton_DrawBackground(&framebuffer, w->x, w->y, w->width, w->height, active);
	xOffset = w->width  - w->_textWidth;
	yOffset = w->height - w->_textHeight;
	DrawTextArgs_Make(&args, &w->text, &titleFont, true);

	if (!active) Drawer2D.Colors['f'] = Drawer2D.Colors['7'];
	Context2D_DrawText(&framebuffer, &args, 
					  w->x + xOffset / 2, w->y + yOffset / 2);

	if (!active) Drawer2D.Colors['f'] = Drawer2D.Colors['F'];
}


/*########################################################################################################################*
*-----------------------------------------------------CheckboxWidget------------------------------------------------------*
*#########################################################################################################################*/
#define CB_SIZE  24
#define CB_OFFSET 8

static void LCheckbox_OnClick(void* w) {
	struct LCheckbox* cb = (struct LCheckbox*)w;
	LBackend_MarkDirty(cb);

	cb->value = !cb->value;
	if (cb->ValueChanged) cb->ValueChanged(cb);
}

void LBackend_CheckboxInit(struct LCheckbox* w) {
	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &w->text, &textFont, true);

	w->width   = Display_ScaleX(CB_SIZE + CB_OFFSET) + Drawer2D_TextWidth(&args);
	w->height  = Display_ScaleY(CB_SIZE);
	w->OnClick = LCheckbox_OnClick;
}

void LBackend_CheckboxUpdate(struct LCheckbox* w) {
	LBackend_MarkDirty(w);
}

/* Based off checkbox from original ClassiCube Launcher */
static const cc_uint8 checkbox_indices[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x06, 0x07, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x06, 0x09, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x06, 0x0B, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0D, 0x0E, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0F, 0x06, 0x10, 0x00, 0x11, 0x06, 0x12, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x13, 0x14, 0x15, 0x00, 0x16, 0x17, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x18, 0x06, 0x19, 0x06, 0x1A, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x1B, 0x06, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x1D, 0x06, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const BitmapCol checkbox_palette[] = {
	BitmapCol_Make(0,0,0,0),        BitmapColor_RGB(144, 144, 144),
	BitmapColor_RGB( 61,  61,  61), BitmapColor_RGB( 94,  94,  94),
	BitmapColor_RGB(197, 196, 197), BitmapColor_RGB( 57,  57,  57),
	BitmapColor_RGB( 33,  33,  33), BitmapColor_RGB(177, 177, 177),
	BitmapColor_RGB(189, 189, 189), BitmapColor_RGB( 67,  67,  67),
	BitmapColor_RGB(108, 108, 108), BitmapColor_RGB(171, 171, 171),
	BitmapColor_RGB(220, 220, 220), BitmapColor_RGB( 43,  43,  43),
	BitmapColor_RGB( 63,  63,  63), BitmapColor_RGB(100, 100, 100),
	BitmapColor_RGB(192, 192, 192), BitmapColor_RGB(132, 132, 132),
	BitmapColor_RGB(175, 175, 175), BitmapColor_RGB(217, 217, 217),
	BitmapColor_RGB( 42,  42,  42), BitmapColor_RGB( 86,  86,  86),
	BitmapColor_RGB( 56,  56,  56), BitmapColor_RGB( 76,  76,  76),
	BitmapColor_RGB(139, 139, 139), BitmapColor_RGB(130, 130, 130),
	BitmapColor_RGB(181, 181, 181), BitmapColor_RGB( 62,  62,  62),
	BitmapColor_RGB( 75,  75,  75), BitmapColor_RGB(184, 184, 184),
};

static void DrawIndexed(int size, int x, int y, struct Context2D* ctx) {
	struct Bitmap* bmp = (struct Bitmap*)ctx;
	BitmapCol* row, color;
	int i, xx, yy;

	for (i = 0, yy = 0; yy < size; yy++) {
		if ((y + yy) < 0) { i += size; continue; }
		if ((y + yy) >= bmp->height) break;
		row = Bitmap_GetRow(bmp, y + yy);

		for (xx = 0; xx < size; xx++) {
			color = checkbox_palette[checkbox_indices[i++]];
			if (color == 0) continue; /* transparent pixel */

			if ((x + xx) < 0 || (x + xx) >= bmp->width) continue;
			row[x + xx] = color;
		}
	}
}

void LBackend_CheckboxDraw(struct LCheckbox* w) {
	BitmapCol boxTop    = BitmapColor_RGB(255, 255, 255);
	BitmapCol boxBottom = BitmapColor_RGB(240, 240, 240);
	struct DrawTextArgs args;
	int x, y, width, height;

	width  = Display_ScaleX(CB_SIZE);
	height = Display_ScaleY(CB_SIZE);

	Gradient_Vertical(&framebuffer, boxTop, boxBottom,
						w->x, w->y,              width, height / 2);
	Gradient_Vertical(&framebuffer, boxBottom, boxTop,
						w->x, w->y + height / 2, width, height / 2);

	if (w->value) {
		const int size = 12;
		x = w->x + width  / 2 - size / 2;
		y = w->y + height / 2 - size / 2;
		DrawIndexed(size, x, y, &framebuffer);
	}
	DrawBoxBounds(BITMAPCOLOR_BLACK, w->x, w->y, width, height);

	DrawTextArgs_Make(&args, &w->text, &textFont, true);
	x = w->x + Display_ScaleX(CB_SIZE + CB_OFFSET);
	y = w->y + (height - Drawer2D_TextHeight(&args)) / 2;
	Context2D_DrawText(&framebuffer, &args, x, y);
}


/*########################################################################################################################*
*------------------------------------------------------InputWidget--------------------------------------------------------*
*#########################################################################################################################*/
static TimeMS caretStart;
static Rect2D caretRect, lastCaretRect;
#define Rect2D_Equals(a, b) a.X == b.X && a.Y == b.Y && a.Width == b.Width && a.Height == b.Height

void LBackend_InputInit(struct LInput* w, int width) {
	w->width    = Display_ScaleX(width);
	w->height   = Display_ScaleY(LINPUT_HEIGHT);
	w->minWidth = w->width;
}

void LBackend_InputUpdate(struct LInput* w) {
	cc_string text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	int textWidth;

	String_InitArray(text, textBuffer);
	LInput_UNSAFE_GetText(w, &text);
	LBackend_MarkDirty(w);

	DrawTextArgs_Make(&args, &text, &textFont, false);
	textWidth      = Drawer2D_TextWidth(&args);
	w->width       = max(w->minWidth, textWidth + inputExpand);
	w->_textHeight = Drawer2D_TextHeight(&args);
}

static Rect2D LInput_MeasureCaret(struct LInput* w, cc_string* text) {
	struct DrawTextArgs args;
	Rect2D r;
	DrawTextArgs_Make(&args, text, &textFont, true);

	r.X = w->x + xInputOffset;
	r.Y = w->y + w->height - caretOffset; r.Height = caretHeight;

	if (w->caretPos == -1) {
		r.X += Drawer2D_TextWidth(&args);
		r.Width = caretWidth;
	} else {
		args.text = String_UNSAFE_Substring(text, 0, w->caretPos);
		r.X += Drawer2D_TextWidth(&args);

		args.text = String_UNSAFE_Substring(text, w->caretPos, 1);
		r.Width   = Drawer2D_TextWidth(&args);
	}
	return r;
}

static void LInput_MoveCaretToCursor(struct LInput* w, int idx) {
	cc_string text; char textBuffer[STRING_SIZE];
	int x = Pointers[idx].x, y = Pointers[idx].y;
	struct DrawTextArgs args;
	int i, charX, charWidth;

	/* Input widget may have been selected by pressing tab */
	/* In which case cursor is completely outside, so ignore */
	if (!Gui_Contains(w->x, w->y, w->width, w->height, x, y)) return;

	String_InitArray(text, textBuffer);
	LInput_UNSAFE_GetText(w, &text);
	x -= w->x; y -= w->y;

	DrawTextArgs_Make(&args, &text, &textFont, true);
	if (x >= Drawer2D_TextWidth(&args)) {
		w->caretPos = -1; return; 
	}

	for (i = 0; i < text.length; i++) {
		args.text = String_UNSAFE_Substring(&text, 0, i);
		charX     = Drawer2D_TextWidth(&args);

		args.text = String_UNSAFE_Substring(&text, i, 1);
		charWidth = Drawer2D_TextWidth(&args);
		if (x >= charX && x < charX + charWidth) {
			w->caretPos = i; return;
		}
	}
}

void LBackend_InputTick(struct LInput* w) {
	int elapsed;
	cc_bool caretShow;
	Rect2D r;

	if (!caretStart) return;
	elapsed = (int)(DateTime_CurrentUTC_MS() - caretStart);

	caretShow = (elapsed % 1000) < 500;
	if (caretShow == w->caretShow) return;
	w->caretShow = caretShow;

	LBackend_InputDraw(w);
	r = caretRect;
	
	if (Rect2D_Equals(r, lastCaretRect)) {
		/* Fast path, caret is blinking in same spot */
		MarkAreaDirty(r.X, r.Y, r.Width, r.Height);
	} else {
		/* Slow path (new widget, caret moved, etc) */
		MarkAreaDirty(w->x, w->y, w->width, w->height);
	}
	lastCaretRect = r;
}

void LBackend_InputSelect(struct LInput* w, int idx, cc_bool wasSelected) {
	struct OpenKeyboardArgs args;
	caretStart   = DateTime_CurrentUTC_MS();
	w->caretShow = true;
	LInput_MoveCaretToCursor(w, idx);
	LBackend_MarkDirty(w);

	if (wasSelected) return;
	OpenKeyboardArgs_Init(&args, &w->text, w->inputType);
	Window_OpenKeyboard(&args);
}

void LBackend_InputUnselect(struct LInput* w) {
	caretStart   = 0;
	w->caretShow = false;
	LBackend_MarkDirty(w);
	Window_CloseKeyboard();
}


static void LInput_DrawOuterBorder(struct LInput* w) {
	struct LScreen* s     = Launcher_Active;
	struct Context2D* ctx = &framebuffer;
	BitmapCol color       = Launcher_Theme.ButtonBorderColor;

	if (w->selected) {
		DrawBoxBounds(color, w->x, w->y, w->width, w->height);
	} else {
		s->ResetArea(ctx, w->x,                      w->y, 
						  w->width,                  yBorder);
		s->ResetArea(ctx, w->x,                      w->y + w->height - yBorder,
						  w->width,                  yBorder);
		s->ResetArea(ctx, w->x,                      w->y,
						  xBorder,                   w->height);
		s->ResetArea(ctx, w->x + w->width - xBorder, w->y,
						  xBorder,                   w->height);
	}
}

static void LInput_DrawInnerBorder(struct LInput* w) {
	/* e.g. for modern theme: 162,131,186 --> 165,142,168 */
	BitmapCol color = BitmapColor_Offset(Launcher_Theme.ButtonHighlightColor, 3,11,-18);

	Context2D_Clear(&framebuffer, color,
		w->x + xBorder,             w->y + yBorder,
		w->width - xBorder2,        yBorder);
	Context2D_Clear(&framebuffer, color,
		w->x + xBorder,             w->y + w->height - yBorder2,
		w->width - xBorder2,        yBorder);
	Context2D_Clear(&framebuffer, color,
		w->x + xBorder,             w->y + yBorder,
		xBorder,                    w->height - yBorder2);
	Context2D_Clear(&framebuffer, color,
		w->x + w->width - xBorder2, w->y + yBorder,
		xBorder,                    w->height - yBorder2);
}

static void LInput_BlendBoxTop(struct LInput* w) {
	BitmapCol color = BitmapColor_RGB(0, 0, 0);

	Gradient_Blend(&framebuffer, color, 75,
		w->x + xBorder,      w->y + yBorder, 
		w->width - xBorder2, yBorder);
	Gradient_Blend(&framebuffer, color, 50,
		w->x + xBorder,      w->y + yBorder2,
		w->width - xBorder2, yBorder);
	Gradient_Blend(&framebuffer, color, 25,
		w->x + xBorder,      w->y + yBorder3, 
		w->width - xBorder2, yBorder);
}

static void LInput_DrawText(struct LInput* w, struct DrawTextArgs* args) {
	int y, hintHeight;

	if (w->text.length || !w->hintText) {
		y = w->y + (w->height - w->_textHeight) / 2;
		Context2D_DrawText(&framebuffer, args, 
							w->x + xInputOffset, y + yInputOffset);
	} else {
		args->text = String_FromReadonly(w->hintText);
		args->font = &hintFont;

		hintHeight = Drawer2D_TextHeight(args);
		y = w->y + (w->height - hintHeight) / 2;

		Drawer2D.Colors['f'] = BitmapColor_RGB(125, 125, 125);
		Context2D_DrawText(&framebuffer, args, 
							w->x + xInputOffset, y);
		Drawer2D.Colors['f'] = BITMAPCOLOR_WHITE;
	}
}

void LBackend_InputDraw(struct LInput* w) {
	cc_string text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;

	String_InitArray(text, textBuffer);
	LInput_UNSAFE_GetText(w, &text);
	DrawTextArgs_Make(&args, &text, &textFont, false);

	LInput_DrawOuterBorder(w);
	LInput_DrawInnerBorder(w);
	Context2D_Clear(&framebuffer, BITMAPCOLOR_WHITE,
		w->x + xBorder2,     w->y + yBorder2,
		w->width - xBorder4, w->height - yBorder4);
	LInput_BlendBoxTop(w);

	Drawer2D.Colors['f'] = Drawer2D.Colors['0'];
	LInput_DrawText(w, &args);
	Drawer2D.Colors['f'] = Drawer2D.Colors['F'];

	caretRect = LInput_MeasureCaret(w, &text);
	if (!w->caretShow) return;
	Context2D_Clear(&framebuffer, BITMAPCOLOR_BLACK,
					caretRect.X, caretRect.Y, caretRect.Width, caretRect.Height);
}


/*########################################################################################################################*
*------------------------------------------------------LabelWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_LabelInit(struct LLabel* w) { }
#define LLabel_GetFont(w) (w->small ? &hintFont : &textFont)

void LBackend_LabelUpdate(struct LLabel* w) {
	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &w->text, LLabel_GetFont(w), true);
	LBackend_MarkDirty(w);

	w->width  = Drawer2D_TextWidth(&args);
	w->height = Drawer2D_TextHeight(&args);
}

void LBackend_LabelDraw(struct LLabel* w) {
	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &w->text, LLabel_GetFont(w), true);
	Context2D_DrawText(&framebuffer, &args, w->x, w->y);
}


/*########################################################################################################################*
*-------------------------------------------------------LineWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_LineInit(struct LLine* w, int width) {
	w->width  = Display_ScaleX(width);
	w->height = Display_ScaleY(LLINE_HEIGHT);
}

void LBackend_LineDraw(struct LLine* w) {
	BitmapCol color = LLine_GetColor();
	Gradient_Blend(&framebuffer, color, 128, w->x, w->y, w->width, w->height);
}


/*########################################################################################################################*
*------------------------------------------------------SliderWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_SliderInit(struct LSlider* w, int width, int height) {
	w->width  = Display_ScaleX(width); 
	w->height = Display_ScaleY(height);
}

void LBackend_SliderUpdate(struct LSlider* w) {
	LBackend_MarkDirty(w);
}

static void LSlider_DrawBoxBounds(struct LSlider* w) {
	BitmapCol boundsTop    = BitmapColor_RGB(119, 100, 132);
	BitmapCol boundsBottom = BitmapColor_RGB(150, 130, 165);

	/* TODO: Check these are actually right */
	Context2D_Clear(&framebuffer, boundsTop,
				  w->x,     w->y,
				  w->width, yBorder);
	Context2D_Clear(&framebuffer, boundsBottom,
				  w->x,	    w->y + w->height - yBorder,
				  w->width, yBorder);

	Gradient_Vertical(&framebuffer, boundsTop, boundsBottom,
					 w->x,                      w->y,
					 xBorder,                   w->height);
	Gradient_Vertical(&framebuffer, boundsTop, boundsBottom,
					 w->x + w->width - xBorder, w->y,
					 xBorder,				    w->height);
}

static void LSlider_DrawBox(struct LSlider* w) {
	BitmapCol progTop    = BitmapColor_RGB(220, 204, 233);
	BitmapCol progBottom = BitmapColor_RGB(207, 181, 216);
	int halfHeight = (w->height - yBorder2) / 2;

	Gradient_Vertical(&framebuffer, progTop, progBottom,
					  w->x + xBorder,	   w->y + yBorder, 
					  w->width - xBorder2, halfHeight);
	Gradient_Vertical(&framebuffer, progBottom, progTop,
					  w->x + xBorder,	   w->y + yBorder + halfHeight, 
		              w->width - xBorder2, halfHeight);
}

#define LSLIDER_MAXVALUE 100
void LBackend_SliderDraw(struct LSlider* w) {
	int curWidth;
	LSlider_DrawBoxBounds(w);
	LSlider_DrawBox(w);

	curWidth = (int)((w->width - xBorder2) * w->value / LSLIDER_MAXVALUE);
	Context2D_Clear(&framebuffer, w->color,
				   w->x + xBorder, w->y + yBorder, 
				   curWidth,       w->height - yBorder2);
}


/*########################################################################################################################*
*-------------------------------------------------------TableWidget-------------------------------------------------------*
*#########################################################################################################################*/
static void InitRowFont(void) {
	if (rowFont.handle) return;
	Font_Make(&rowFont, 11, FONT_FLAGS_NONE);
}

void LBackend_TableInit(struct LTable* w) { }
void LBackend_TableUpdate(struct LTable* w) { }

void LBackend_TableReposition(struct LTable* w) {
	int rowsHeight;
	InitRowFont();
	w->hdrHeight = Font_CalcHeight(&textFont, true) + hdrYPadding;
	w->rowHeight = Font_CalcHeight(&rowFont,  true) + rowYPadding;

	w->rowsBegY = w->y + w->hdrHeight + gridlineHeight;
	w->rowsEndY = w->y + w->height;
	rowsHeight  = w->height - (w->rowsBegY - w->y);

	w->visibleRows = rowsHeight / w->rowHeight;
	LTable_ClampTopRow(w);
}

void LBackend_TableFlagAdded(struct LTable* w) {
	/* TODO: Only redraw flags */
	LBackend_MarkDirty(w);
}

/* Draws background behind column headers */
static void LTable_DrawHeaderBackground(struct LTable* w) {
	BitmapCol gridColor = BitmapColor_RGB(20, 20, 10);

	if (!Launcher_Theme.ClassicBackground) {
		Context2D_Clear(&framebuffer, gridColor,
						w->x, w->y, w->width, w->hdrHeight);
	} else {
		Launcher_DrawBackground(&framebuffer,
						w->x, w->y, w->width, w->hdrHeight);
	}
}

static BitmapCol LBackend_TableRowColor(struct LTable* w, int row) {
	struct ServerInfo* entry = row < w->rowsCount ? LTable_Get(row) : NULL;
	cc_bool selected         = entry && String_Equals(&entry->hash, w->selectedHash);

	return LTable_RowColor(entry, row, selected);
}

/* Draws background behind each row in the table */
static void LTable_DrawRowsBackground(struct LTable* w) {
	int y, height, row;
	BitmapCol color;

	y = w->rowsBegY;
	for (row = w->topRow; ; row++, y += w->rowHeight) {
		color = LBackend_TableRowColor(w, row);

		/* last row may get chopped off */
		height = min(y + w->rowHeight, w->rowsEndY) - y;
		/* hit the end of the table */
		if (height < 0) break;

		if (color) {
			Context2D_Clear(&framebuffer, color,
							w->x, y, w->width, height);
		} else {
			Launcher_DrawBackground(&framebuffer, 
							w->x, y, w->width, height);
		}
	}
}

/* Draws a gridline below column headers and gridlines after each column */
static void LTable_DrawGridlines(struct LTable* w) {
	int i, x;
	if (Launcher_Theme.ClassicBackground) return;

	x = w->x;
	Context2D_Clear(&framebuffer, Launcher_Theme.BackgroundColor,
				   x, w->y + w->hdrHeight, w->width, gridlineHeight);

	for (i = 0; i < w->numColumns; i++) {
		x += w->columns[i].width;
		if (!w->columns[i].hasGridline) continue;
			
		Context2D_Clear(&framebuffer, Launcher_Theme.BackgroundColor,
					   x, w->y, gridlineWidth, w->height);
		x += gridlineWidth;
	}
}

/* Draws the entire background of the table */
static void LTable_DrawBackground(struct LTable* w) {
	LTable_DrawHeaderBackground(w);
	LTable_DrawRowsBackground(w);
	LTable_DrawGridlines(w);
}

/* Draws title of each column at top of the table */
static void LTable_DrawHeaders(struct LTable* w) {
	struct DrawTextArgs args;
	int i, x, y;

	DrawTextArgs_MakeEmpty(&args, &textFont, true);
	x = w->x; y = w->y;

	for (i = 0; i < w->numColumns; i++) {
		args.text = String_FromReadonly(w->columns[i].name);
		Drawer2D_DrawClippedText(&framebuffer, &args, 
								x + cellXOffset, y + hdrYOffset, 
								w->columns[i].width - cellXPadding);

		x += w->columns[i].width;
		if (w->columns[i].hasGridline) x += gridlineWidth;
	}
}

/* Draws contents of the currently visible rows in the table */
static void LTable_DrawRows(struct LTable* w) {
	cc_string str; char strBuffer[STRING_SIZE];
	struct ServerInfo* entry;
	struct DrawTextArgs args;
	struct LTableCell cell;
	int i, x, y, row, end;

	InitRowFont();
	String_InitArray(str, strBuffer);
	DrawTextArgs_Make(&args, &str, &rowFont, true);
	cell.table = w;
	y   = w->rowsBegY;
	end = w->topRow + w->visibleRows;

	for (row = w->topRow; row < end; row++, y += w->rowHeight) {
		x = w->x;

		if (row >= w->rowsCount)            break;
		if (y + w->rowHeight > w->rowsEndY) break;
		entry = LTable_Get(row);

		for (i = 0; i < w->numColumns; i++) {
			args.text  = str; cell.x = x; cell.y = y;
			cell.width = w->columns[i].width;
			w->columns[i].DrawRow(entry, &args, &cell, &framebuffer);

			if (args.text.length) {
				Drawer2D_DrawClippedText(&framebuffer, &args, 
										x + cellXOffset, y + rowYOffset, 
										cell.width - cellXPadding);
			}

			x += w->columns[i].width;
			if (w->columns[i].hasGridline) x += gridlineWidth;
		}
	}
}

/* Draws scrollbar on the right edge of the table */
static void LTable_DrawScrollbar(struct LTable* w) {
	BitmapCol classicBack   = BitmapColor_RGB( 80,  80,  80);
	BitmapCol classicScroll = BitmapColor_RGB(160, 160, 160);
	BitmapCol backCol   = Launcher_Theme.ClassicBackground ? classicBack   : Launcher_Theme.ButtonBorderColor;
	BitmapCol scrollCol = Launcher_Theme.ClassicBackground ? classicScroll : Launcher_Theme.ButtonForeActiveColor;

	int x, y, height;
	x = w->x + w->width - scrollbarWidth;
	LTable_GetScrollbarCoords(w, &y, &height);

	Context2D_Clear(&framebuffer, backCol,
					x, w->y,     scrollbarWidth, w->height);		
	Context2D_Clear(&framebuffer, scrollCol, 
					x, w->y + y, scrollbarWidth, height);
}

void LBackend_TableDraw(struct LTable* w) {
	LTable_DrawBackground(w);
	LTable_DrawHeaders(w);
	LTable_DrawRows(w);
	LTable_DrawScrollbar(w);
	MarkAllDirty();
}


static void LTable_RowsClick(struct LTable* w, int idx) {
	int mouseY = Pointers[idx].y - w->rowsBegY;
	int row    = w->topRow + mouseY / w->rowHeight;
	LTable_RowClick(w, row);
}

/* Handles clicking on column headers (either resizes a column or sort rows) */
static void LTable_HeadersClick(struct LTable* w, int idx) {
	int x, i, mouseX = Pointers[idx].x;

	for (i = 0, x = w->x; i < w->numColumns; i++) {
		/* clicked on gridline, begin dragging */
		if (mouseX >= (x - dragPad) && mouseX < (x + dragPad) && w->columns[i].draggable) {
			w->draggingColumn = i - 1;
			w->dragXStart = (mouseX - w->x) - w->columns[i - 1].width;
			return;
		}

		x += w->columns[i].width;
		if (w->columns[i].hasGridline) x += gridlineWidth;
	}

	for (i = 0, x = w->x; i < w->numColumns; i++) {
		if (mouseX >= x && mouseX < (x + w->columns[i].width) && w->columns[i].sortable) {
			w->sortingCol = i;
			w->columns[i].invertSort = !w->columns[i].invertSort;
			LTable_Sort(w);
			return;
		}

		x += w->columns[i].width;
		if (w->columns[i].hasGridline) x += gridlineWidth;
	}
}

/* Handles clicking on the scrollbar on right edge of table */
static void LTable_ScrollbarClick(struct LTable* w, int idx) {
	int y, height, mouseY = Pointers[idx].y - w->y;
	LTable_GetScrollbarCoords(w, &y, &height);

	if (mouseY < y) {
		w->topRow -= w->visibleRows;
	} else if (mouseY >= y + height) {
		w->topRow += w->visibleRows;
	} else {
		w->dragYOffset = mouseY - y;
	}

	w->draggingScrollbar = true;
	LTable_ClampTopRow(w);
}

void LBackend_TableMouseDown(struct LTable* w, int idx) {
	if (Pointers[idx].x >= WindowInfo.Width - scrollbarWidth) {
		LTable_ScrollbarClick(w, idx);
		w->_lastRow = -1;
	} else if (Pointers[idx].y < w->rowsBegY) {
		LTable_HeadersClick(w, idx);
		w->_lastRow = -1;
	} else {
		LTable_RowsClick(w, idx);
	}
	LBackend_MarkDirty(w);
}

void LBackend_TableMouseMove(struct LTable* w, int idx) {
	int x = Pointers[idx].x - w->x, y = Pointers[idx].y - w->y;
	int i, col, width, maxW;

	if (w->draggingScrollbar) {
		float scale = w->height / (float)w->rowsCount;
		int row     = (int)((y - w->dragYOffset) / scale);
		/* avoid expensive redraw when possible */
		if (w->topRow == row) return;

		w->topRow = row;
		LTable_ClampTopRow(w);
		LBackend_MarkDirty(w);
	} else if (w->draggingColumn >= 0) {
		col   = w->draggingColumn;
		width = x - w->dragXStart;

		/* Ensure this column doesn't expand past right side of table */
		maxW = w->width;
		for (i = 0; i < col; i++) maxW -= w->columns[i].width;

		Math_Clamp(width, cellMinWidth, maxW - cellMinWidth);
		if (width == w->columns[col].width) return;
		w->columns[col].width = width;
		LBackend_MarkDirty(w);
	}
}

/* Stops an in-progress dragging of resizing column. */
void LBackend_TableMouseUp(struct LTable* w, int idx) {
	w->draggingColumn    = -1;
	w->draggingScrollbar = false;
	w->dragYOffset       = 0;
}
#endif
