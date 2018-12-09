#include "LWidgets.h"
#include "Gui.h"
#include "Game.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "ExtMath.h"
#include "Window.h"
#include "Funcs.h"

#define BORDER 1

void LWidget_SetLocation(void* widget, uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset) {
	struct LWidget* w = widget;
	w->HorAnchor = horAnchor; w->VerAnchor = verAnchor;
	w->XOffset   = xOffset;   w->YOffset = yOffset;
	LWidget_CalcPosition(widget);
}

void LWidget_CalcPosition(void* widget) {
	struct LWidget* w = widget;
	w->X = Gui_CalcPos(w->HorAnchor, w->XOffset, w->Width,  Game_Width);
	w->Y = Gui_CalcPos(w->VerAnchor, w->YOffset, w->Height, Game_Height);
}

void LWidget_Draw(void* widget) {
	struct LWidget* w = widget;
	w->VTABLE->Draw(w);
	w->Last.X = w->X; w->Last.Width  = w->Width;
	w->Last.Y = w->Y; w->Last.Height = w->Height;
}

void LWidget_Redraw(void* widget) {
	struct LWidget* w = widget;
	Launcher_ResetArea(w->Last.X, w->Last.Y, w->Last.Width, w->Last.Height);
	LWidget_Draw(w);
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol LButton_Expand(BitmapCol a, int amount) {
	int r, g, b;
	r = a.R + amount; Math_Clamp(r, 0, 255); a.R = r;
	g = a.G + amount; Math_Clamp(g, 0, 255); a.G = g;
	b = a.B + amount; Math_Clamp(b, 0, 255); a.B = b;
	return a;
}

static void LButton_DrawBackground(struct LButton* w) {
	BitmapCol activeCol   = BITMAPCOL_CONST(126, 136, 191, 255);
	BitmapCol inactiveCol = BITMAPCOL_CONST(111, 111, 111, 255);
	BitmapCol col;

	if (Launcher_ClassicBackground) {
		col = w->Hovered ? activeCol : inactiveCol;
		Gradient_Noise(&Launcher_Framebuffer, col, 8,
						w->X + BORDER,         w->Y + BORDER,
						w->Width - 2 * BORDER, w->Height - 2 * BORDER);
	} else {
		col = w->Hovered ? Launcher_ButtonForeActiveCol : Launcher_ButtonForeCol;
		Gradient_Vertical(&Launcher_Framebuffer, LButton_Expand(col, 8), LButton_Expand(col, -8),
						  w->X + BORDER,         w->Y + BORDER,
						  w->Width - 2 * BORDER, w->Height - 2 * BORDER);
	}
}

static void LButton_DrawBorder(struct LButton* w) {
	BitmapCol black   = BITMAPCOL_CONST(0, 0, 0, 255);
	BitmapCol backCol = Launcher_ClassicBackground ? black : Launcher_ButtonBorderCol;

	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X + BORDER,            w->Y,
					w->Width - 2 * BORDER,    BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X + BORDER,            w->Y + w->Height - BORDER,
					w->Width - 2 * BORDER,    BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X,                     w->Y + BORDER,
					BORDER,                   w->Height - 2 * BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X + w->Width - BORDER, w->Y + BORDER,
					BORDER,                   w->Height - 2 * BORDER);
}

static void LButton_DrawHighlight(struct LButton* w) {
	BitmapCol activeCol   = BITMAPCOL_CONST(189, 198, 255, 255);
	BitmapCol inactiveCol = BITMAPCOL_CONST(168, 168, 168, 255);
	BitmapCol highlightCol;

	if (Launcher_ClassicBackground) {
		highlightCol = w->Hovered ? activeCol : inactiveCol;
		Drawer2D_Clear(&Launcher_Framebuffer, highlightCol,
						w->X + BORDER * 2,     w->Y + BORDER,
						w->Width - BORDER * 4, BORDER);
		Drawer2D_Clear(&Launcher_Framebuffer, highlightCol, 
						w->X + BORDER,         w->Y + BORDER * 2,
						BORDER,                w->Height - BORDER * 4);
	} else if (!w->Hovered) {
		Drawer2D_Clear(&Launcher_Framebuffer, Launcher_ButtonHighlightCol, 
						w->X + BORDER * 2,     w->Y + BORDER,
						w->Width - BORDER * 4, BORDER);
	}
}

static void LButton_Redraw(void* widget) {
	struct DrawTextArgs args;
	struct LButton* w = widget;
	int xOffset, yOffset;
	if (w->Hidden) return;

	xOffset = w->Width  - w->_TextSize.Width;
	yOffset = w->Height - w->_TextSize.Height;
	DrawTextArgs_Make(&args, &w->Text, &w->Font, true);

	LButton_DrawBackground(w);
	LButton_DrawBorder(w);
	LButton_DrawHighlight(w);

	if (!w->Hovered) Drawer2D_Cols['f'] = Drawer2D_Cols['7'];
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, 
					  w->X + xOffset / 2, w->Y + yOffset / 2);

	if (!w->Hovered) Drawer2D_Cols['f'] = Drawer2D_Cols['F'];
	Launcher_Dirty = true;
}

static struct LWidgetVTABLE lbutton_VTABLE = {
	LButton_Redraw, 
	NULL, NULL,                      /* Key    */
	LButton_Redraw, LButton_Redraw,  /* Hover  */
	NULL, NULL                       /* Select */
};
void LButton_Init(struct LButton* w, int width, int height) {
	w->VTABLE = &lbutton_VTABLE;
	w->TabSelectable = true;
	w->Width  = width; w->Height = height;
	String_InitArray(w->Text, w->_TextBuffer);
}

void LButton_SetText(struct LButton* w, const String* text, const FontDesc* font) {
	struct DrawTextArgs args;
	w->Font = *font;
	String_Copy(&w->Text, text);

	DrawTextArgs_Make(&args, text, font, true);
	w->_TextSize = Drawer2D_MeasureText(&args);
}


/*########################################################################################################################*
*------------------------------------------------------InputWidget--------------------------------------------------------*
*#########################################################################################################################*/
CC_NOINLINE static void LInput_GetText(struct LInput* w, String* text) {
	int i;
	if (!w->Password) { *text = w->Text; return; }

	for (i = 0; i < w->Text.length; i++) {
		String_Append(text, '*');
	}
}

static void LInput_DrawOuterBorder(struct LInput* w) {
	BitmapCol col = BITMAPCOL_CONST(97, 81, 110, 255);
	int width = w->_RealWidth;

	if (w->Selected) {
		Drawer2D_Clear(&Launcher_Framebuffer, col, 
			w->X,                  w->Y, 
			width,                 BORDER);
		Drawer2D_Clear(&Launcher_Framebuffer, col, 
			w->X,                  w->Y + w->Height - BORDER, 
			width,                 BORDER);
		Drawer2D_Clear(&Launcher_Framebuffer, col, 
			w->X,                  w->Y, 
			BORDER,                w->Height);
		Drawer2D_Clear(&Launcher_Framebuffer, col, 
			w->X + width - BORDER, w->Y, 
			BORDER,                w->Height);
	} else {
		Launcher_ResetArea(w->X,                  w->Y, 
						   width,                 BORDER);
		Launcher_ResetArea(w->X,                  w->Y + w->Height - BORDER,
						   width,                 BORDER);
		Launcher_ResetArea(w->X,                  w->Y, 
						   BORDER,                w->Height);
		Launcher_ResetArea(w->X + width - BORDER, w->Y, 
						   BORDER,                w->Height);
	}
}

static void LInput_DrawInnerBorder(struct LInput* w) {
	BitmapCol col = BITMAPCOL_CONST(165, 142, 168, 255);
	int width = w->_RealWidth;

	Drawer2D_Clear(&Launcher_Framebuffer, col,
		w->X + BORDER,             w->Y + BORDER, 
		width - BORDER * 2,        BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, col,
		w->X + BORDER,             w->Y + w->Height - BORDER * 2, 
		width - BORDER * 2,        BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, col,
		w->X + BORDER,             w->Y + BORDER, 
		BORDER,                    w->Height - BORDER * 2);
	Drawer2D_Clear(&Launcher_Framebuffer, col,
		w->X + width - BORDER * 2, w->Y + BORDER, 
		BORDER,                    w->Height - BORDER * 2);
}

static void LInput_BlendBoxTop(struct LInput* w) {
	BitmapCol col = BITMAPCOL_CONST(0, 0, 0, 255);
	int width = w->_RealWidth;

	Gradient_Blend(&Launcher_Framebuffer, col, 75,
		w->X + BORDER,      w->Y + BORDER * 1, 
		width - BORDER * 2, BORDER);
	Gradient_Blend(&Launcher_Framebuffer, col, 50,
		w->X + BORDER,      w->Y + BORDER * 2, 
		width - BORDER * 2, BORDER);
	Gradient_Blend(&Launcher_Framebuffer, col, 25,
		w->X + BORDER,      w->Y + BORDER * 3, 
		width - BORDER * 2, BORDER);
}

static void LInput_DrawText(struct LInput* w, struct DrawTextArgs* args) {
	int hintHeight, y;

	if (w->Text.length || !w->HintText) {
		y = w->Y + (w->Height - w->_TextHeight) / 2;
		Drawer2D_DrawText(&Launcher_Framebuffer, args, w->X + 5, y + 2);
	} else {
		args->Text = String_FromReadonly(w->HintText);
		args->Font = w->HintFont;

		hintHeight = Drawer2D_MeasureText(args).Height;
		y = w->Y + (w->Height - hintHeight) / 2;
		Drawer2D_DrawText(&Launcher_Framebuffer, args, w->X + 5, y);
	}
}

static void LInput_Redraw(void* widget) {
	String text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	Size2D size;

	BitmapCol white = BITMAPCOL_CONST(255, 255, 255, 255);
	struct LInput* w = widget;
	if (w->Hidden) return;

	String_InitArray(text, textBuffer);
	LInput_GetText(w, &text);
	DrawTextArgs_Make(&args, &text, &w->Font, false);

	size = Drawer2D_MeasureText(&args);
	w->_RealWidth  = max(w->BaseWidth, size.Width + 20);
	w->_TextHeight = size.Height;

	LInput_DrawOuterBorder(w);
	LInput_DrawInnerBorder(w);
	Drawer2D_Clear(&Launcher_Framebuffer, white,
		w->X + 2, w->Y + 2, w->_RealWidth - 4, w->Height - 4);
	LInput_BlendBoxTop(w);

	Drawer2D_Cols['f'] = Drawer2D_Cols['0'];
	LInput_DrawText(w, &args);
	Drawer2D_Cols['f'] = Drawer2D_Cols['F'];
	Launcher_Dirty = true;
}

static void LInput_KeyDown(void* widget, Key key) {
	struct LInput* w = widget;
	if (key == KEY_BACKSPACE && LInput_Backspace(w)) {
		LWidget_Redraw(w);
	} else if (key == KEY_DELETE && LInput_Delete(w)) {
		LWidget_Redraw(w);
	} else if (key == KEY_C && Key_IsControlPressed()) {
		if (w->Text.length) Window_SetClipboardText(&w->Text);
	} else if (key == KEY_V && Key_IsControlPressed()) {
		if (LInput_CopyFromClipboard(w)) LWidget_Redraw(w);
	} else if (key == KEY_ESCAPE) {
		if (LInput_Clear(w)) LWidget_Redraw(w);
	} else if (key == KEY_LEFT) {
		LInput_AdvanceCaretPos(w, false);
		LWidget_Redraw(w);
	} else if (key == KEY_RIGHT) {
		LInput_AdvanceCaretPos(w, true);
		LWidget_Redraw(w);
	}
}

static void LInput_KeyChar(void* widget, char c) {
	struct LInput* w = widget;
	if (LInput_Append(w, c)) LWidget_Redraw(w);
}

static struct LWidgetVTABLE linput_VTABLE = {
	LInput_Redraw, 
	LInput_KeyDown, LInput_KeyChar, /* Key    */
	NULL, NULL,                     /* Hover  */
	/* TODO: Don't redraw whole thing, just the outer border */
	LInput_Redraw, LInput_Redraw    /* Select */
};
void LInput_Init(struct LInput* w, int width, int height, const char* hintText, const FontDesc* hintFont) {
	w->VTABLE = &linput_VTABLE;
	w->TabSelectable = true;
	String_InitArray(w->Text, w->_TextBuffer);

	w->BaseWidth = width;
	w->Width = width; w->Height = height;
	LWidget_CalcPosition(w);

	w->HintFont = *hintFont;
	w->HintText = hintText;
}

void LInput_SetText(struct LInput* w, const String* text_, const FontDesc* font) {
	String text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	Size2D size;

	String_Copy(&w->Text, text_);
	w->Font = *font;

	String_InitArray(text, textBuffer);
	LInput_GetText(w, &text);
	DrawTextArgs_Make(&args, &text, &w->Font, true);

	size = Drawer2D_MeasureText(&args);
	w->_RealWidth  = max(w->BaseWidth, size.Width + 15);
	w->_TextHeight = size.Height;
}

Rect2D LInput_MeasureCaret(struct LInput* w) {
	String text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	Rect2D r;

	String_InitArray(text, textBuffer);
	LInput_GetText(w, &text);
	DrawTextArgs_Make(&args, &text, &w->Font, true);

	r.X = w->X + 5;
	r.Y = w->Y + w->Height - 5; r.Height = 2;

	if (w->CaretPos == -1) {
		r.X += Drawer2D_MeasureText(&args).Width;
		r.Width = 10;
	} else {
		args.Text = String_UNSAFE_Substring(&text, 0, w->CaretPos);
		r.X += Drawer2D_MeasureText(&args).Width;

		args.Text = String_UNSAFE_Substring(&text, w->CaretPos, 1);
		r.Width   = Drawer2D_MeasureText(&args).Width;
	}
	return r;
}

void LInput_AdvanceCaretPos(struct LInput* w, bool forwards) {
	if (forwards && w->CaretPos == -1) return;
	if (!forwards && w->CaretPos == 0) return;
	if (w->CaretPos == -1 && !forwards) /* caret after text */
		w->CaretPos = w->Text.length;

	w->CaretPos += (forwards ? 1 : -1);
	if (w->CaretPos < 0 || w->CaretPos >= w->Text.length) w->CaretPos = -1;
}

void LInput_SetCaretToCursor(struct LInput* w, int x, int y) {
	String text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	int i, charX, charWidth;

	String_InitArray(text, textBuffer);
	LInput_GetText(w, &text);
	x -= w->X; y -= w->Y;

	DrawTextArgs_Make(&args, &text, &w->Font, true);
	if (x >= Drawer2D_MeasureText(&args).Width) { 
		w->CaretPos = -1; return; 
	}

	for (i = 0; i < text.length; i++) {
		args.Text = String_UNSAFE_Substring(&text, 0, i);
		charX     = Drawer2D_MeasureText(&args).Width;

		args.Text = String_UNSAFE_Substring(&text, i, 1);
		charWidth = Drawer2D_MeasureText(&args).Width;
		if (x >= charX && x < charX + charWidth) {
			w->CaretPos = i; return;
		}
	}
}

bool LInput_Append(struct LInput* w, char c) {
	if (w->TextFilter && !w->TextFilter(c)) return false;

	if (c >= ' ' && c <= '~' && c != '&' && w->Text.length < w->Text.capacity) {
		if (w->CaretPos == -1) {
			String_Append(&w->Text, c);
		} else {
			String_InsertAt(&w->Text, w->CaretPos, c);
			w->CaretPos++;
		}
		if (w->TextChanged) w->TextChanged(w);
		return true;
	}
	return false;
}

bool LInput_Backspace(struct LInput* w) {
	if (w->Text.length == 0 || w->CaretPos == 0) return false;

	if (w->CaretPos == -1) {
		String_DeleteAt(&w->Text, w->Text.length - 1);
	} else {	
		String_DeleteAt(&w->Text, w->CaretPos - 1);
		w->CaretPos--;
		if (w->CaretPos == -1) w->CaretPos = 0;
	}

	if (w->TextChanged) w->TextChanged(w);
	if (w->CaretPos >= w->Text.length) w->CaretPos = -1;
	return true;
}

bool LInput_Delete(struct LInput* w) {
	if (w->Text.length == 0 || w->CaretPos == -1) return false;

	String_DeleteAt(&w->Text, w->CaretPos);
	if (w->CaretPos == -1) w->CaretPos = 0;

	if (w->TextChanged) w->TextChanged(w);
	if (w->CaretPos >= w->Text.length) w->CaretPos = -1;
	return true;
}

bool LInput_Clear(struct LInput* w) {
	if (w->Text.length == 0) return false;
	w->Text.length = 0;

	if (w->TextChanged) w->TextChanged(w);
	w->CaretPos = -1;
	return true;
}

bool LInput_CopyFromClipboard(struct LInput* w) {
	String text; char textBuffer[256];
	String_InitArray(text, textBuffer);

	Window_GetClipboardText(&text);
	String_UNSAFE_TrimStart(&text);
	String_UNSAFE_TrimEnd(&text);

	if (w->Text.length >= w->Text.capacity) return false;
	if (!text.length) return false;
	if (w->ClipboardFilter) w->ClipboardFilter(&text);

	String_AppendString(&w->Text, &text);
	if (w->TextChanged) w->TextChanged(w);
	return true;
}


/*########################################################################################################################*
*------------------------------------------------------LabelWidget--------------------------------------------------------*
*#########################################################################################################################*/
static void LLabel_Redraw(void* widget) {
	struct DrawTextArgs args;
	struct LLabel* w = widget;
	if (w->Hidden) return;

	DrawTextArgs_Make(&args, &w->Text, &w->Font, true);
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, w->X, w->Y);
	Launcher_Dirty = true;
}

static struct LWidgetVTABLE llabel_VTABLE = {
	LLabel_Redraw, 
	NULL, NULL, /* Key    */
	NULL, NULL, /* Hover  */
	NULL, NULL  /* Select */
};
void LLabel_Init(struct LLabel* w) {
	w->VTABLE = &llabel_VTABLE;
	String_InitArray(w->Text, w->_TextBuffer);
}

void LLabel_SetText(struct LLabel* w, const String* text, const FontDesc* font) {
	struct DrawTextArgs args;
	Size2D size;
	w->Font = *font;
	String_Copy(&w->Text, text);

	DrawTextArgs_Make(&args, &w->Text, &w->Font, true);
	size = Drawer2D_MeasureText(&args);
	w->Width = size.Width; w->Height = size.Height;
	LWidget_CalcPosition(w);
}


/*########################################################################################################################*
*------------------------------------------------------SliderWidget-------------------------------------------------------*
*#########################################################################################################################*/
static void LSlider_DrawBoxBounds(struct LSlider* w) {
	BitmapCol boundsTop    =  BITMAPCOL_CONST(119, 100, 132, 255);
	BitmapCol boundsBottom =  BITMAPCOL_CONST(150, 130, 165, 255);

	/* TODO: Check these are actually right */
	Drawer2D_Clear(&Launcher_Framebuffer, boundsTop,
				  w->X - BORDER,         w->Y - BORDER,
				  w->Width + 2 * BORDER, BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, boundsBottom,
				  w->X - BORDER,         w->Y + w->Height,
				  w->Width + 2 * BORDER, BORDER);

	Gradient_Vertical(&Launcher_Framebuffer, boundsTop, boundsBottom,
					 w->X - BORDER,      w->Y - BORDER,
				  BORDER,                w->Height + BORDER);
	Gradient_Vertical(&Launcher_Framebuffer, boundsTop, boundsBottom,
					 w->X + w->Width,    w->Y - BORDER,
					 BORDER,             w->Height + BORDER);
}

static void LSlider_DrawBox(struct LSlider* w) {
	BitmapCol progTop    = BITMAPCOL_CONST(220, 204, 233, 255);
	BitmapCol progBottom = BITMAPCOL_CONST(207, 181, 216, 255);

	Gradient_Vertical(&Launcher_Framebuffer, progTop, progBottom,
					  w->X, w->Y, w->Width, w->Height / 2);
	Gradient_Vertical(&Launcher_Framebuffer, progBottom, progTop,
					  w->X, w->Y + (w->Height / 2), w->Width, w->Height);
}

static void LSlider_Redraw(void* widget) {
	struct LSlider* w = widget;
	LSlider_DrawBoxBounds(w);
	LSlider_DrawBox(w);

	Drawer2D_Clear(&Launcher_Framebuffer, w->ProgressCol,
				   w->X, w->Y, (int)(w->Width * w->Value / w->MaxValue), w->Height);
	Launcher_Dirty = true;
}

static struct LWidgetVTABLE lslider_VTABLE = {
	LSlider_Redraw, 
	NULL, NULL, /* Key    */
	NULL, NULL, /* Hover  */
	NULL, NULL  /* Select */
};
void LSlider_Init(struct LSlider* w, int width, int height) {
	w->VTABLE = &lslider_VTABLE;
	w->Width  = width; w->Height = height;
	w->MaxValue = 100;
}
