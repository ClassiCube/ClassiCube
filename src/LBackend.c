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

/*########################################################################################################################*
*---------------------------------------------------------LWidget---------------------------------------------------------*
*#########################################################################################################################*/
static int xBorder, xBorder2, xBorder3, xBorder4;
static int yBorder, yBorder2, yBorder3, yBorder4;
static int xInputOffset, yInputOffset, inputExpand;
static int caretOffset, caretWidth, caretHeight;

void LBackend_Init(void) {
	xBorder = Display_ScaleX(1); xBorder2 = xBorder * 2; xBorder3 = xBorder * 3; xBorder4 = xBorder * 4;
	yBorder = Display_ScaleY(1); yBorder2 = yBorder * 2; yBorder3 = yBorder * 3; yBorder4 = yBorder * 4;

	xInputOffset = Display_ScaleX(5);
	yInputOffset = Display_ScaleY(2);
	inputExpand  = Display_ScaleX(20);

	caretOffset  = Display_ScaleY(5);
	caretWidth   = Display_ScaleX(10);
	caretHeight  = Display_ScaleY(2);
}

static void DrawBoxBounds(BitmapCol col, int x, int y, int width, int height) {
	Drawer2D_Clear(&Launcher_Framebuffer, col, 
		x,                   y, 
		width,               yBorder);
	Drawer2D_Clear(&Launcher_Framebuffer, col, 
		x,                   y + height - yBorder,
		width,               yBorder);
	Drawer2D_Clear(&Launcher_Framebuffer, col, 
		x,                   y, 
		xBorder,             height);
	Drawer2D_Clear(&Launcher_Framebuffer, col, 
		x + width - xBorder, y, 
		xBorder,             height);
}

void LBackend_WidgetRepositioned(struct LWidget* w) { }
void LBackend_SetScreen(struct LScreen* s)   { }
void LBackend_CloseScreen(struct LScreen* s) { }


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_InitButton(struct LButton* w, int width, int height) {	
	w->width  = Display_ScaleX(width);
	w->height = Display_ScaleY(height);
}

void LBackend_UpdateButton(struct LButton* w) {
	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &w->text, &Launcher_TitleFont, true);

	w->_textWidth  = Drawer2D_TextWidth(&args);
	w->_textHeight = Drawer2D_TextHeight(&args);
}

void LBackend_DrawButton(struct LButton* w) {
	struct DrawTextArgs args;
	int xOffset, yOffset;

	LButton_DrawBackground(w, &Launcher_Framebuffer, w->x, w->y);
	xOffset = w->width  - w->_textWidth;
	yOffset = w->height - w->_textHeight;
	DrawTextArgs_Make(&args, &w->text, &Launcher_TitleFont, true);

	if (!w->hovered) Drawer2D.Colors['f'] = Drawer2D.Colors['7'];
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, 
					  w->x + xOffset / 2, w->y + yOffset / 2);

	if (!w->hovered) Drawer2D.Colors['f'] = Drawer2D.Colors['F'];
}


/*########################################################################################################################*
*-----------------------------------------------------CheckboxWidget------------------------------------------------------*
*#########################################################################################################################*/
#define CB_SIZE  24
#define CB_OFFSET 8

void LBackend_InitCheckbox(struct LCheckbox* w) {
	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &w->text, &Launcher_TextFont, true);

	w->width  = Display_ScaleX(CB_SIZE + CB_OFFSET) + Drawer2D_TextWidth(&args);
	w->height = Display_ScaleY(CB_SIZE);
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
	BitmapCol_Make(  0,   0,   0,   0), BitmapCol_Make(144, 144, 144, 255),
	BitmapCol_Make( 61,  61,  61, 255), BitmapCol_Make( 94,  94,  94, 255),
	BitmapCol_Make(197, 196, 197, 255), BitmapCol_Make( 57,  57,  57, 255),
	BitmapCol_Make( 33,  33,  33, 255), BitmapCol_Make(177, 177, 177, 255),
	BitmapCol_Make(189, 189, 189, 255), BitmapCol_Make( 67,  67,  67, 255),
	BitmapCol_Make(108, 108, 108, 255), BitmapCol_Make(171, 171, 171, 255),
	BitmapCol_Make(220, 220, 220, 255), BitmapCol_Make( 43,  43,  43, 255),
	BitmapCol_Make( 63,  63,  63, 255), BitmapCol_Make(100, 100, 100, 255),
	BitmapCol_Make(192, 192, 192, 255), BitmapCol_Make(132, 132, 132, 255),
	BitmapCol_Make(175, 175, 175, 255), BitmapCol_Make(217, 217, 217, 255),
	BitmapCol_Make( 42,  42,  42, 255), BitmapCol_Make( 86,  86,  86, 255),
	BitmapCol_Make( 56,  56,  56, 255), BitmapCol_Make( 76,  76,  76, 255),
	BitmapCol_Make(139, 139, 139, 255), BitmapCol_Make(130, 130, 130, 255),
	BitmapCol_Make(181, 181, 181, 255), BitmapCol_Make( 62,  62,  62, 255),
	BitmapCol_Make( 75,  75,  75, 255), BitmapCol_Make(184, 184, 184, 255),
};

static void DrawIndexed(int size, int x, int y, struct Bitmap* bmp) {
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

void LBackend_DrawCheckbox(struct LCheckbox* w) {
	BitmapCol boxTop    = BitmapCol_Make(255, 255, 255, 255);
	BitmapCol boxBottom = BitmapCol_Make(240, 240, 240, 255);
	struct DrawTextArgs args;
	int x, y, width, height;

	width  = Display_ScaleX(CB_SIZE);
	height = Display_ScaleY(CB_SIZE);

	Gradient_Vertical(&Launcher_Framebuffer, boxTop, boxBottom,
						w->x, w->y,              width, height / 2);
	Gradient_Vertical(&Launcher_Framebuffer, boxBottom, boxTop,
						w->x, w->y + height / 2, width, height / 2);

	if (w->value) {
		const int size = 12;
		x = w->x + width  / 2 - size / 2;
		y = w->y + height / 2 - size / 2;
		DrawIndexed(size, x, y, &Launcher_Framebuffer);
	}
	DrawBoxBounds(BITMAPCOL_BLACK, w->x, w->y, width, height);

	DrawTextArgs_Make(&args, &w->text, &Launcher_TextFont, true);
	x = w->x + Display_ScaleX(CB_SIZE + CB_OFFSET);
	y = w->y + (height - Drawer2D_TextHeight(&args)) / 2;
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, x, y);
}


/*########################################################################################################################*
*------------------------------------------------------InputWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_InitInput(struct LInput* w, int width) {
	w->width  = Display_ScaleX(width);
	w->height = Display_ScaleY(30);
}

static void LInput_DrawOuterBorder(struct LInput* w) {
	BitmapCol color = BitmapCol_Make(97, 81, 110, 255);

	if (w->selected) {
		DrawBoxBounds(color, w->x, w->y, w->width, w->height);
	} else {
		Launcher_ResetArea(w->x,                      w->y, 
						   w->width,                  yBorder);
		Launcher_ResetArea(w->x,                      w->y + w->height - yBorder,
						   w->width,                  yBorder);
		Launcher_ResetArea(w->x,                      w->y, 
						   xBorder,                   w->height);
		Launcher_ResetArea(w->x + w->width - xBorder, w->y,
						   xBorder,                   w->height);
	}
}

static void LInput_DrawInnerBorder(struct LInput* w) {
	BitmapCol color = BitmapCol_Make(165, 142, 168, 255);

	Drawer2D_Clear(&Launcher_Framebuffer, color,
		w->x + xBorder,             w->y + yBorder,
		w->width - xBorder2,        yBorder);
	Drawer2D_Clear(&Launcher_Framebuffer, color,
		w->x + xBorder,             w->y + w->height - yBorder2,
		w->width - xBorder2,        yBorder);
	Drawer2D_Clear(&Launcher_Framebuffer, color,
		w->x + xBorder,             w->y + yBorder,
		xBorder,                    w->height - yBorder2);
	Drawer2D_Clear(&Launcher_Framebuffer, color,
		w->x + w->width - xBorder2, w->y + yBorder,
		xBorder,                    w->height - yBorder2);
}

static void LInput_BlendBoxTop(struct LInput* w) {
	BitmapCol color = BitmapCol_Make(0, 0, 0, 255);

	Gradient_Blend(&Launcher_Framebuffer, color, 75,
		w->x + xBorder,      w->y + yBorder, 
		w->width - xBorder2, yBorder);
	Gradient_Blend(&Launcher_Framebuffer, color, 50,
		w->x + xBorder,      w->y + yBorder2,
		w->width - xBorder2, yBorder);
	Gradient_Blend(&Launcher_Framebuffer, color, 25,
		w->x + xBorder,      w->y + yBorder3, 
		w->width - xBorder2, yBorder);
}

static void LInput_DrawText(struct LInput* w, struct DrawTextArgs* args) {
	int y, hintHeight;

	if (w->text.length || !w->hintText) {
		y = w->y + (w->height - w->_textHeight) / 2;
		Drawer2D_DrawText(&Launcher_Framebuffer, args, 
							w->x + xInputOffset, y + yInputOffset);
	} else {
		args->text = String_FromReadonly(w->hintText);
		args->font = &Launcher_HintFont;

		hintHeight = Drawer2D_TextHeight(args);
		y = w->y + (w->height - hintHeight) / 2;

		Drawer2D.Colors['f'] = BitmapCol_Make(125, 125, 125, 255);
		Drawer2D_DrawText(&Launcher_Framebuffer, args, 
							w->x + xInputOffset, y);
		Drawer2D.Colors['f'] = BITMAPCOL_WHITE;
	}
}

static void LInput_UpdateDimensions(struct LInput* w, const cc_string* text) {
	struct DrawTextArgs args;
	int textWidth;
	DrawTextArgs_Make(&args, text, &Launcher_TextFont, false);

	textWidth      = Drawer2D_TextWidth(&args);
	w->width       = max(w->minWidth, textWidth + inputExpand);
	w->_textHeight = Drawer2D_TextHeight(&args);
}

void LBackend_UpdateInput(struct LInput* w) {
	cc_string text; char textBuffer[STRING_SIZE];
	String_InitArray(text, textBuffer);

	LInput_UNSAFE_GetText(w, &text);
	LInput_UpdateDimensions(w, &text);
}

void LBackend_DrawInput(struct LInput* w) {
	cc_string text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;

	String_InitArray(text, textBuffer);
	LInput_UNSAFE_GetText(w, &text);
	DrawTextArgs_Make(&args, &text, &Launcher_TextFont, false);

	/* TODO shouldn't be recalcing size in draw.... */
	LInput_UpdateDimensions(w, &text);
	LInput_DrawOuterBorder(w);
	LInput_DrawInnerBorder(w);
	Drawer2D_Clear(&Launcher_Framebuffer, BITMAPCOL_WHITE,
		w->x + xBorder2,     w->y + yBorder2,
		w->width - xBorder4, w->height - yBorder4);
	LInput_BlendBoxTop(w);

	Drawer2D.Colors['f'] = Drawer2D.Colors['0'];
	LInput_DrawText(w, &args);
	Drawer2D.Colors['f'] = Drawer2D.Colors['F'];
}


static TimeMS caretStart;
static cc_bool lastCaretShow;
static Rect2D lastCaretRec;
#define Rect2D_Equals(a, b) a.X == b.X && a.Y == b.Y && a.Width == b.Width && a.Height == b.Height

static Rect2D LInput_MeasureCaret(struct LInput* w) {
	cc_string text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	Rect2D r;

	String_InitArray(text, textBuffer);
	LInput_UNSAFE_GetText(w, &text);
	DrawTextArgs_Make(&args, &text, &Launcher_TextFont, true);

	r.X = w->x + xInputOffset;
	r.Y = w->y + w->height - caretOffset; r.Height = caretHeight;

	if (w->caretPos == -1) {
		r.X += Drawer2D_TextWidth(&args);
		r.Width = caretWidth;
	} else {
		args.text = String_UNSAFE_Substring(&text, 0, w->caretPos);
		r.X += Drawer2D_TextWidth(&args);

		args.text = String_UNSAFE_Substring(&text, w->caretPos, 1);
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
	lastCaretShow = false;

	String_InitArray(text, textBuffer);
	LInput_UNSAFE_GetText(w, &text);
	x -= w->x; y -= w->y;

	DrawTextArgs_Make(&args, &text, &Launcher_TextFont, true);
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

void LBackend_TickInput(struct LInput* w) {
	int elapsed;
	cc_bool caretShow;
	Rect2D r;

	if (!caretStart) return;
	elapsed = (int)(DateTime_CurrentUTC_MS() - caretStart);

	caretShow = (elapsed % 1000) < 500;
	if (caretShow == lastCaretShow) return;
	lastCaretShow = caretShow;

	LBackend_DrawInput(w);
	r = LInput_MeasureCaret(w);

	if (caretShow) {
		Drawer2D_Clear(&Launcher_Framebuffer, BITMAPCOL_BLACK,
					   r.X, r.Y, r.Width, r.Height);
	}
	
	if (Rect2D_Equals(r, lastCaretRec)) {
		/* Fast path, caret is blinking in same spot */
		Launcher_MarkDirty(r.X, r.Y, r.Width, r.Height);
	} else {
		Launcher_MarkDirty(w->x, w->y, w->width, w->height);
	}
	lastCaretRec = r;
}

void LBackend_SelectInput(struct LInput* w, int idx, cc_bool wasSelected) {
	struct OpenKeyboardArgs args;
	caretStart = DateTime_CurrentUTC_MS();
	LInput_MoveCaretToCursor(w, idx);
	/* TODO: Only draw outer border */
	if (wasSelected) return;

	LWidget_Draw(w);
	OpenKeyboardArgs_Init(&args, &w->text, w->type);
	Window_OpenKeyboard(&args);
}

void LBackend_UnselectInput(struct LInput* w) {
	caretStart = 0;
	/* TODO: Only draw outer border */
	LWidget_Draw(w);
	Window_CloseKeyboard();
}


/*########################################################################################################################*
*------------------------------------------------------LabelWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_InitLabel(struct LLabel* w) { }

void LBackend_UpdateLabel(struct LLabel* w) {
	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &w->text, w->font, true);

	w->width  = Drawer2D_TextWidth(&args);
	w->height = Drawer2D_TextHeight(&args);
}

void LBackend_DrawLabel(struct LLabel* w) {
	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &w->text, w->font, true);
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, w->x, w->y);
}


/*########################################################################################################################*
*-------------------------------------------------------LineWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_InitLine(struct LLine* w, int width) {
	w->width  = Display_ScaleX(width);
	w->height = Display_ScaleY(2);
}

void LBackend_DrawLine(struct LLine* w) {
	BitmapCol color = LLine_GetColor();
	Gradient_Blend(&Launcher_Framebuffer, color, 128, w->x, w->y, w->width, w->height);
}


/*########################################################################################################################*
*------------------------------------------------------SliderWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_InitSlider(struct LSlider* w, int width, int height) {
	w->width  = Display_ScaleX(width); 
	w->height = Display_ScaleY(height);
}

void LBackend_UpdateSlider(struct LSlider* w) {
	LWidget_Draw(w);
}

static void LSlider_DrawBoxBounds(struct LSlider* w) {
	BitmapCol boundsTop    = BitmapCol_Make(119, 100, 132, 255);
	BitmapCol boundsBottom = BitmapCol_Make(150, 130, 165, 255);

	/* TODO: Check these are actually right */
	Drawer2D_Clear(&Launcher_Framebuffer, boundsTop,
				  w->x,     w->y,
				  w->width, yBorder);
	Drawer2D_Clear(&Launcher_Framebuffer, boundsBottom,
				  w->x,	    w->y + w->height - yBorder,
				  w->width, yBorder);

	Gradient_Vertical(&Launcher_Framebuffer, boundsTop, boundsBottom,
					 w->x,                      w->y,
					 xBorder,                   w->height);
	Gradient_Vertical(&Launcher_Framebuffer, boundsTop, boundsBottom,
					 w->x + w->width - xBorder, w->y,
					 xBorder,				    w->height);
}

static void LSlider_DrawBox(struct LSlider* w) {
	BitmapCol progTop    = BitmapCol_Make(220, 204, 233, 255);
	BitmapCol progBottom = BitmapCol_Make(207, 181, 216, 255);
	int halfHeight = (w->height - yBorder2) / 2;

	Gradient_Vertical(&Launcher_Framebuffer, progTop, progBottom,
					  w->x + xBorder,	   w->y + yBorder, 
					  w->width - xBorder2, halfHeight);
	Gradient_Vertical(&Launcher_Framebuffer, progBottom, progTop,
					  w->x + xBorder,	   w->y + yBorder + halfHeight, 
		              w->width - xBorder2, halfHeight);
}

#define LSLIDER_MAXVALUE 100
void LBackend_DrawSlider(struct LSlider* w) {
	int curWidth;
	LSlider_DrawBoxBounds(w);
	LSlider_DrawBox(w);

	curWidth = (int)((w->width - xBorder2) * w->value / LSLIDER_MAXVALUE);
	Drawer2D_Clear(&Launcher_Framebuffer, w->color,
				   w->x + xBorder, w->y + yBorder, 
				   curWidth,       w->height - yBorder2);
}
#endif
