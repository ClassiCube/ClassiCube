#include "LWidgets.h"
#include "Gui.h"
#include "Game.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "ExtMath.h"
#include "Window.h"
#include "Funcs.h"
#include "LWeb.h"
#include "Platform.h"

#define BORDER 1
#define BORDER2 (2 * BORDER)
#define BORDER3 (3 * BORDER)
#define BORDER4 (4 * BORDER)

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
						w->X + BORDER,      w->Y + BORDER,
						w->Width - BORDER2, w->Height - BORDER2);
	} else {
		col = w->Hovered ? Launcher_ButtonForeActiveCol : Launcher_ButtonForeCol;
		Gradient_Vertical(&Launcher_Framebuffer, LButton_Expand(col, 8), LButton_Expand(col, -8),
						  w->X + BORDER,      w->Y + BORDER,
						  w->Width - BORDER2, w->Height - BORDER2);
	}
}

static void LButton_DrawBorder(struct LButton* w) {
	BitmapCol black   = BITMAPCOL_CONST(0, 0, 0, 255);
	BitmapCol backCol = Launcher_ClassicBackground ? black : Launcher_ButtonBorderCol;

	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X + BORDER,            w->Y,
					w->Width - BORDER2,       BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X + BORDER,            w->Y + w->Height - BORDER,
					w->Width - BORDER2,       BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X,                     w->Y + BORDER,
					BORDER,                   w->Height - BORDER2);
	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X + w->Width - BORDER, w->Y + BORDER,
					BORDER,                   w->Height - BORDER2);
}

static void LButton_DrawHighlight(struct LButton* w) {
	BitmapCol activeCol   = BITMAPCOL_CONST(189, 198, 255, 255);
	BitmapCol inactiveCol = BITMAPCOL_CONST(168, 168, 168, 255);
	BitmapCol highlightCol;

	if (Launcher_ClassicBackground) {
		highlightCol = w->Hovered ? activeCol : inactiveCol;
		Drawer2D_Clear(&Launcher_Framebuffer, highlightCol,
						w->X + BORDER2,     w->Y + BORDER,
						w->Width - BORDER4, BORDER);
		Drawer2D_Clear(&Launcher_Framebuffer, highlightCol, 
						w->X + BORDER,       w->Y + BORDER2,
						BORDER,              w->Height - BORDER4);
	} else if (!w->Hovered) {
		Drawer2D_Clear(&Launcher_Framebuffer, Launcher_ButtonHighlightCol, 
						w->X + BORDER2,      w->Y + BORDER,
						w->Width - BORDER4,  BORDER);
	}
}

static void LButton_Draw(void* widget) {
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
	Launcher_MarkDirty(w->X, w->Y, w->Width, w->Height);
}

static void LButton_Hover(void* w, int x, int y, bool wasOver) { 
	/* only need to redraw when changing from unhovered to hovered */	
	if (!wasOver) LButton_Draw(w); 
}

static struct LWidgetVTABLE lbutton_VTABLE = {
	LButton_Draw, NULL,
	NULL, NULL,                  /* Key    */
	LButton_Hover, LButton_Draw, /* Hover  */
	NULL, NULL                   /* Select */
};
void LButton_Init(struct LButton* w, const FontDesc* font, int width, int height) {
	w->VTABLE = &lbutton_VTABLE;
	w->TabSelectable = true;
	w->Font   = *font;
	w->Width  = width; w->Height = height;
	String_InitArray(w->Text, w->_TextBuffer);
}

void LButton_SetText(struct LButton* w, const String* text) {
	struct DrawTextArgs args;
	String_Copy(&w->Text, text);
	DrawTextArgs_Make(&args, text, &w->Font, true);
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

	if (w->Selected) {
		Drawer2D_Clear(&Launcher_Framebuffer, col, 
			w->X,                     w->Y, 
			w->Width,                 BORDER);
		Drawer2D_Clear(&Launcher_Framebuffer, col, 
			w->X,                     w->Y + w->Height - BORDER, 
			w->Width,                 BORDER);
		Drawer2D_Clear(&Launcher_Framebuffer, col, 
			w->X,                     w->Y, 
			BORDER,                   w->Height);
		Drawer2D_Clear(&Launcher_Framebuffer, col, 
			w->X + w->Width - BORDER, w->Y, 
			BORDER,                   w->Height);
	} else {
		Launcher_ResetArea(w->X,                     w->Y, 
						   w->Width,                 BORDER);
		Launcher_ResetArea(w->X,                     w->Y + w->Height - BORDER,
						   w->Width,                 BORDER);
		Launcher_ResetArea(w->X,                     w->Y, 
						   BORDER,                   w->Height);
		Launcher_ResetArea(w->X + w->Width - BORDER, w->Y, 
						   BORDER,                   w->Height);
	}
}

static void LInput_DrawInnerBorder(struct LInput* w) {
	BitmapCol col = BITMAPCOL_CONST(165, 142, 168, 255);

	Drawer2D_Clear(&Launcher_Framebuffer, col,
		w->X + BORDER,             w->Y + BORDER, 
		w->Width - BORDER2,        BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, col,
		w->X + BORDER,             w->Y + w->Height - BORDER2,
		w->Width - BORDER2,        BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, col,
		w->X + BORDER,             w->Y + BORDER, 
		BORDER,                    w->Height - BORDER2);
	Drawer2D_Clear(&Launcher_Framebuffer, col,
		w->X + w->Width - BORDER2, w->Y + BORDER,
		BORDER,                    w->Height - BORDER2);
}

static void LInput_BlendBoxTop(struct LInput* w) {
	BitmapCol col = BITMAPCOL_CONST(0, 0, 0, 255);

	Gradient_Blend(&Launcher_Framebuffer, col, 75,
		w->X + BORDER,      w->Y + BORDER, 
		w->Width - BORDER2, BORDER);
	Gradient_Blend(&Launcher_Framebuffer, col, 50,
		w->X + BORDER,      w->Y + BORDER2,
		w->Width - BORDER2, BORDER);
	Gradient_Blend(&Launcher_Framebuffer, col, 25,
		w->X + BORDER,      w->Y + BORDER3, 
		w->Width - BORDER2, BORDER);
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

static void LInput_Draw(void* widget) {
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
	w->Width       = max(w->MinWidth, size.Width + 20);
	w->_TextHeight = size.Height;

	LInput_DrawOuterBorder(w);
	LInput_DrawInnerBorder(w);
	Drawer2D_Clear(&Launcher_Framebuffer, white,
		w->X + BORDER2,     w->Y + BORDER2, 
		w->Width - BORDER4, w->Height - BORDER4);
	LInput_BlendBoxTop(w);

	Drawer2D_Cols['f'] = Drawer2D_Cols['0'];
	LInput_DrawText(w, &args);
	Drawer2D_Cols['f'] = Drawer2D_Cols['F'];
	Launcher_MarkDirty(w->X, w->Y, w->Width, w->Height);
}

static Rect2D LInput_MeasureCaret(struct LInput* w) {
	String text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	Rect2D r;

	String_InitArray(text, textBuffer);
	LInput_GetText(w, &text);
	DrawTextArgs_Make(&args, &text, &w->Font, true);

	r.X = w->X + 5;
	r.Y = w->Y + w->Height - 5; r.Height = 2;

	if (w->CaretPos == -1) {
		r.X += Drawer2D_TextWidth(&args);
		r.Width = 10;
	} else {
		args.Text = String_UNSAFE_Substring(&text, 0, w->CaretPos);
		r.X += Drawer2D_TextWidth(&args);

		args.Text = String_UNSAFE_Substring(&text, w->CaretPos, 1);
		r.Width   = Drawer2D_TextWidth(&args);
	}
	return r;
}

static TimeMS caretStart;
static bool lastCaretShow;
static Rect2D lastCaretRec;
#define Rect2D_Equals(a, b) a.X == b.X && a.Y == b.Y && a.Width == b.Width && a.Height == b.Height

static void LInput_TickCaret(void* widget) {
	struct LInput* w = widget;
	BitmapCol col = BITMAPCOL_CONST(0, 0, 0, 255);
	int elapsed;
	bool caretShow;
	Rect2D r;

	elapsed = (int)(DateTime_CurrentUTC_MS() - caretStart);
	if (!caretStart) return;

	caretShow = (elapsed % 1000) < 500;
	if (caretShow == lastCaretShow) return;
	lastCaretShow = caretShow;

	LInput_Draw(w);
	r = LInput_MeasureCaret(w);

	if (caretShow) {
		Drawer2D_Clear(&Launcher_Framebuffer, col, 
					   r.X, r.Y, r.Width, r.Height);
	}
	
	if (Rect2D_Equals(r, lastCaretRec)) {
		/* Fast path, caret is blinking in same spot */
		Launcher_MarkDirty(r.X, r.Y, r.Width, r.Height);
	} else {
		Launcher_MarkDirty(w->X, w->Y, w->Width, w->Height);
	}
	lastCaretRec = r;
}

static void LInput_AdvanceCaretPos(struct LInput* w, bool forwards) {
	if (forwards && w->CaretPos == -1) return;
	if (!forwards && w->CaretPos == 0) return;
	if (w->CaretPos == -1 && !forwards) /* caret after text */
		w->CaretPos = w->Text.length;

	w->CaretPos += (forwards ? 1 : -1);
	if (w->CaretPos < 0 || w->CaretPos >= w->Text.length) w->CaretPos = -1;
}

static void LInput_MoveCaretToCursor(struct LInput* w) {
	String text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	int i, charX, charWidth;
	int x = Mouse_X, y = Mouse_Y;

	/* Input widget may have been selected by pressing tab */
	/* In which case cursor is completely outside, so ignore */
	if (!Gui_Contains(w->X, w->Y, w->Width, w->Height, x, y)) return;
	lastCaretShow = false;

	String_InitArray(text, textBuffer);
	LInput_GetText(w, &text);
	x -= w->X; y -= w->Y;

	DrawTextArgs_Make(&args, &text, &w->Font, true);
	if (x >= Drawer2D_TextWidth(&args)) {
		w->CaretPos = -1; return; 
	}

	for (i = 0; i < text.length; i++) {
		args.Text = String_UNSAFE_Substring(&text, 0, i);
		charX     = Drawer2D_TextWidth(&args);

		args.Text = String_UNSAFE_Substring(&text, i, 1);
		charWidth = Drawer2D_TextWidth(&args);
		if (x >= charX && x < charX + charWidth) {
			w->CaretPos = i; return;
		}
	}
}

static void LInput_Select(void* widget, bool wasSelected) {
	caretStart = DateTime_CurrentUTC_MS();
	LInput_MoveCaretToCursor((struct LInput*)widget);
	/* TODO: Only draw outer border */
	if (wasSelected) return;
	LInput_Draw(widget);
}

static void LInput_Unselect(void* widget) {
	caretStart = 0;
	/* TODO: Only draw outer border */
	LInput_Draw(widget);
}

static void LInput_KeyDown(void* widget, Key key, bool was) {
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
	LInput_Draw, LInput_TickCaret,
	LInput_KeyDown, LInput_KeyChar, /* Key    */
	NULL, NULL,                     /* Hover  */
	/* TODO: Don't redraw whole thing, just the outer border */
	LInput_Select, LInput_Unselect  /* Select */
};
void LInput_Init(struct LInput* w, const FontDesc* font, int width, int height, const char* hintText, const FontDesc* hintFont) {
	w->VTABLE = &linput_VTABLE;
	w->TabSelectable = true;
	String_InitArray(w->Text, w->_TextBuffer);
	w->Font  = *font;

	w->MinWidth = width;
	w->Width    = width; w->Height = height;
	LWidget_CalcPosition(w);

	w->HintFont = *hintFont;
	w->HintText = hintText;
	w->CaretPos = -1;
}

void LInput_SetText(struct LInput* w, const String* text_) {
	String text; char textBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	Size2D size;

	String_Copy(&w->Text, text_);
	String_InitArray(text, textBuffer);
	LInput_GetText(w, &text);
	DrawTextArgs_Make(&args, &text, &w->Font, true);

	size = Drawer2D_MeasureText(&args);
	w->Width       = max(w->MinWidth, size.Width + 20);
	w->_TextHeight = size.Height;
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
static void LLabel_Draw(void* widget) {
	struct DrawTextArgs args;
	struct LLabel* w = widget;
	if (w->Hidden) return;

	DrawTextArgs_Make(&args, &w->Text, &w->Font, true);
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, w->X, w->Y);
	Launcher_MarkDirty(w->X, w->Y, w->Width, w->Height);
}

static struct LWidgetVTABLE llabel_VTABLE = {
	LLabel_Draw, NULL,
	NULL, NULL, /* Key    */
	NULL, NULL, /* Hover  */
	NULL, NULL  /* Select */
};
void LLabel_Init(struct LLabel* w, const FontDesc* font) {
	w->VTABLE = &llabel_VTABLE;
	String_InitArray(w->Text, w->_TextBuffer);
	w->Font  = *font;
}

void LLabel_SetText(struct LLabel* w, const String* text) {
	struct DrawTextArgs args;
	Size2D size;
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
				  w->X,     w->Y,
				  w->Width, BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, boundsBottom,
				  w->X,	    w->Y + w->Height - BORDER,
				  w->Width, BORDER);

	Gradient_Vertical(&Launcher_Framebuffer, boundsTop, boundsBottom,
					 w->X,					   w->Y,
					 BORDER,                   w->Height);
	Gradient_Vertical(&Launcher_Framebuffer, boundsTop, boundsBottom,
					 w->X + w->Width - BORDER, w->Y,
					 BORDER,				   w->Height);
}

static void LSlider_DrawBox(struct LSlider* w) {
	BitmapCol progTop    = BITMAPCOL_CONST(220, 204, 233, 255);
	BitmapCol progBottom = BITMAPCOL_CONST(207, 181, 216, 255);
	int halfHeight = (w->Height - BORDER2) / 2;

	Gradient_Vertical(&Launcher_Framebuffer, progTop, progBottom,
					  w->X + BORDER,	  w->Y + BORDER, 
					  w->Width - BORDER2, halfHeight);
	Gradient_Vertical(&Launcher_Framebuffer, progBottom, progTop,
					  w->X + BORDER,	  w->Y + BORDER + halfHeight, 
		              w->Width - BORDER2, halfHeight);
}

static void LSlider_Draw(void* widget) {
	struct LSlider* w = widget;
	int curWidth;
	if (w->Hidden) return;

	LSlider_DrawBoxBounds(w);
	LSlider_DrawBox(w);

	curWidth = (int)((w->Width - BORDER2) * w->Value / w->MaxValue);
	Drawer2D_Clear(&Launcher_Framebuffer, w->ProgressCol,
				   w->X + BORDER, w->Y + BORDER, 
				   curWidth,      w->Height - BORDER2);
	Launcher_MarkDirty(w->X, w->Y, w->Width, w->Height);
}

static struct LWidgetVTABLE lslider_VTABLE = {
	LSlider_Draw, NULL,
	NULL, NULL, /* Key    */
	NULL, NULL, /* Hover  */
	NULL, NULL  /* Select */
};
void LSlider_Init(struct LSlider* w, int width, int height) {
	w->VTABLE = &lslider_VTABLE;
	w->Width  = width; w->Height = height;
	w->MaxValue = 100;
}


/*########################################################################################################################*
*------------------------------------------------------TableWidget--------------------------------------------------------*
*#########################################################################################################################*/
static void FlagColumn_Draw(struct ServerInfo* row, struct DrawTextArgs* args, int x, int y) {
	BitmapCol col = BITMAPCOL_CONST(128, 0, 0, 255);
	Drawer2D_Clear(&Launcher_Framebuffer, col, x, y, 16, 11);
}
static void NameColumn_Draw(struct ServerInfo* row, struct DrawTextArgs* args, int x, int y) {
	args->Text = row->Name;
}
static int NameColumn_Sort(const struct ServerInfo* a, const struct ServerInfo* b) {
	return String_Compare(&b->Name, &a->Name);
}

static void PlayersColumn_Draw(struct ServerInfo* row, struct DrawTextArgs* args, int x, int y) {
	String_Format2(&args->Text, "%i/%i", &row->Players, &row->MaxPlayers);
}
static int PlayersColumn_Sort(const struct ServerInfo* a, const struct ServerInfo* b) {
	return b->Players - a->Players;
}

static void UptimeColumn_Draw(struct ServerInfo* row, struct DrawTextArgs* args, int x, int y) {
	int uptime = row->Uptime;
	char unit  = 's';

	if (uptime >= SECS_PER_DAY * 7) {
		uptime /= SECS_PER_DAY;  unit = 'd';
	} else if (uptime >= SECS_PER_HOUR) {
		uptime /= SECS_PER_HOUR; unit = 'h';
	} else if (uptime >= SECS_PER_MIN) {
		uptime /= SECS_PER_MIN;  unit = 'm';
	}
	String_Format2(&args->Text, "%i%r", &uptime, &unit);
}
static int UptimeColumn_Sort(const struct ServerInfo* a, const struct ServerInfo* b) {
	return b->Uptime - a->Uptime;
}

static void SoftwareColumn_Draw(struct ServerInfo* row, struct DrawTextArgs* args, int x, int y) {
	args->Text = row->Software;
}
static int SoftwareColumn_Sort(const struct ServerInfo* a, const struct ServerInfo* b) {
	return String_Compare(&b->Software, &a->Software);
}

static struct LTableColumn tableColumns[5] = {
	{ "",          15, FlagColumn_Draw,     NULL,                false, false },
	{ "Name",     320, NameColumn_Draw,     NameColumn_Sort,     true,  true  },
	{ "Players",   65, PlayersColumn_Draw,  PlayersColumn_Sort,  true,  true  },
	{ "Uptime",    65, UptimeColumn_Draw,   UptimeColumn_Sort,   true,  true  },
	{ "Software", 140, SoftwareColumn_Draw, SoftwareColumn_Sort, false, true  }
};


static int sortingCol = -1;
/* Works out top and height of the scrollbar */
static void LTable_GetScrollbarCoords(struct LTable* w, int* y, int* height) {
	float scale;
	if (!w->RowsCount) { *y = 0; *height = 0; return; }

	scale   = w->Height / (float)w->RowsCount;
	*y      = Math_Ceil(w->TopRow * scale);
	*height = Math_Ceil(w->VisibleRows * scale);
	*height = min(*y + *height, w->Height) - *y;
}

/* Ensures top/first visible row index lies within table */
static void LTable_ClampTopRow(struct LTable* w) { 
	if (w->TopRow > w->RowsCount - w->VisibleRows) {
		w->TopRow = w->RowsCount - w->VisibleRows;
	}
	if (w->TopRow < 0) w->TopRow = 0;
}

/* Returns index of selected row in currently visible rows */
static int LTable_GetSelectedIndex(struct LTable* w) {
	struct ServerInfo* entry;
	int row;

	for (row = 0; row < w->RowsCount; row++) {
		entry = LTable_Get(row);
		if (String_CaselessEquals(w->SelectedHash, &entry->Hash)) return row;
	}
	return -1;
}

/* Sets selected row to given row, scrolling table if needed. */
static void LTable_SetSelectedTo(struct LTable* w, int index) {
	if (!w->RowsCount) return;
	if (index >= w->RowsCount) index = w->RowsCount - 1;
	if (index < 0) index = 0;

	String_Copy(w->SelectedHash, &LTable_Get(index)->Hash);
	LTable_ShowSelected(w);
	w->OnSelectedChanged();
}

#define GRIDLINE_SIZE   2
#define SCROLLBAR_WIDTH 10
#define HDR_YPADDING 3
#define ROW_YPADDING 1
#define CELL_XPADDING 5

/* Draws background behind column headers */
static void LTable_DrawHeaderBackground(struct LTable* w) {
	BitmapCol gridCol = BITMAPCOL_CONST(20, 20, 10, 255);
	if (Launcher_ClassicBackground) return;

	Drawer2D_Clear(&Launcher_Framebuffer, gridCol,
					w->X, w->Y, w->Width, w->HdrHeight);
}

/* Works out the background colour of the given row */
static BitmapCol LTable_RowCol(struct LTable* w, struct ServerInfo* row) {
	BitmapCol emptyCol = BITMAPCOL_CONST(0, 0, 0, 0);
	BitmapCol gridCol  = BITMAPCOL_CONST(20, 20, 10, 255);
	BitmapCol featSelCol  = BITMAPCOL_CONST( 50,  53,  0, 255);
	BitmapCol featuredCol = BITMAPCOL_CONST(101, 107,  0, 255);
	BitmapCol selectedCol = BITMAPCOL_CONST( 40,  40, 40, 255);
	bool selected;

	if (row) {
		selected = String_Equals(&row->Hash, w->SelectedHash);
		if (row->Featured) {
			return selected ? featSelCol : featuredCol;
		} else if (selected) {
			return selectedCol;
		}
	}
	return Launcher_ClassicBackground ? emptyCol : gridCol;
}

/* Draws background behind each row in the table */
static void LTable_DrawRowsBackground(struct LTable* w) {
	struct ServerInfo* entry;
	BitmapCol col;
	int y, height, row;

	y = w->RowsBegY;
	for (row = w->TopRow; ; row++, y += w->RowHeight) {
		entry = row < w->RowsCount ? LTable_Get(row) : NULL;
		col   = LTable_RowCol(w, entry);
		if (!col.A) continue;

		/* last row may get chopped off */
		height = min(y + w->RowHeight, w->RowsEndY) - y;
		/* hit the end of the table */
		if (height < 0) break;

		Drawer2D_Clear(&Launcher_Framebuffer, col,
					   w->X, y, w->Width, height);
	}
}

/* Draws a gridline below column headers and gridlines after each column */
static void LTable_DrawGridlines(struct LTable* w) {
	int i, x;
	if (Launcher_ClassicBackground) return;

	x = w->X;
	Drawer2D_Clear(&Launcher_Framebuffer, Launcher_BackgroundCol,
				   x, w->Y + w->HdrHeight, w->Width, GRIDLINE_SIZE);

	for (i = 0; i < w->NumColumns; i++) {
		x += w->Columns[i].Width;
		if (!w->Columns[i].ColumnGridline) continue;
			
		Drawer2D_Clear(&Launcher_Framebuffer, Launcher_BackgroundCol,
					   x, w->Y, GRIDLINE_SIZE, w->Height);
		x += GRIDLINE_SIZE;
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

	DrawTextArgs_MakeEmpty(&args, &w->HdrFont, true);
	x = w->X; y = w->Y;

	for (i = 0; i < w->NumColumns; i++) {
		args.Text = String_FromReadonly(w->Columns[i].Name);
		Drawer2D_DrawClippedText(&Launcher_Framebuffer, &args, 
								x + CELL_XPADDING, y + HDR_YPADDING, w->Columns[i].Width);

		x += w->Columns[i].Width;
		if (w->Columns[i].ColumnGridline) x += GRIDLINE_SIZE;
	}
}

/* Draws contents of the currently visible rows in the table */
static void LTable_DrawRows(struct LTable* w) {
	BitmapCol gridCol = BITMAPCOL_CONST(20, 20, 10, 255);
	String str; char strBuffer[STRING_SIZE];
	struct ServerInfo* entry;
	struct DrawTextArgs args;
	int i, x, y, row, end;

	String_InitArray(str, strBuffer);
	DrawTextArgs_Make(&args, &str, &w->RowFont, true);
	y   = w->RowsBegY;
	end = w->TopRow + w->VisibleRows;

	for (row = w->TopRow; row < end; row++, y += w->RowHeight) {
		x = w->X;

		if (row >= w->RowsCount)            break;
		if (y + w->RowHeight > w->RowsEndY) break;
		entry = LTable_Get(row);

		for (i = 0; i < w->NumColumns; i++) {
			args.Text = str;
			w->Columns[i].DrawRow(entry, &args, x, y);

			if (args.Text.length) {
				Drawer2D_DrawClippedText(&Launcher_Framebuffer, &args, 
										x + CELL_XPADDING, y + ROW_YPADDING, w->Columns[i].Width);
			}

			x += w->Columns[i].Width;
			if (w->Columns[i].ColumnGridline) x += GRIDLINE_SIZE;
		}
	}
}

/* Draws scrollbar on the right edge of the table */
static void LTable_DrawScrollbar(struct LTable* w) {
	BitmapCol classicBack   = BITMAPCOL_CONST( 80,  80,  80, 255);
	BitmapCol classicScroll = BITMAPCOL_CONST(160, 160, 160, 255);
	BitmapCol backCol   = Launcher_ClassicBackground ? classicBack   : Launcher_ButtonBorderCol;
	BitmapCol scrollCol = Launcher_ClassicBackground ? classicScroll : Launcher_ButtonForeActiveCol;

	int x, y, height;
	x = w->X + w->Width - SCROLLBAR_WIDTH;
	LTable_GetScrollbarCoords(w, &y, &height);

	Drawer2D_Clear(&Launcher_Framebuffer, backCol,
					x, w->Y,     SCROLLBAR_WIDTH, w->Height);		
	Drawer2D_Clear(&Launcher_Framebuffer, scrollCol, 
					x, w->Y + y, SCROLLBAR_WIDTH, height);
}

bool LTable_HandlesKey(Key key) {
	return key == KEY_UP || key == KEY_DOWN || key == KEY_PAGEUP || key == KEY_PAGEDOWN;
}

static void LTable_KeyDown(void* widget, Key key, bool was) {
	struct LTable* w = widget;
	int index = LTable_GetSelectedIndex(w);

	if (key == KEY_UP) {
		index--;
	} else if (key == KEY_DOWN) {
		index++;
	} else if (key == KEY_PAGEUP) {
		index -= w->VisibleRows;
	} else if (key == KEY_PAGEDOWN) {
		index += w->VisibleRows;
	} else { return; }

	w->_lastRow = -1;
	LTable_SetSelectedTo(w, index);
}

static void LTable_MouseMove(void* widget, int deltaX, int deltaY, bool wasOver) {
	struct LTable* w = widget;
	int x = Mouse_X - w->X, y = Mouse_Y - w->Y, col;

	if (w->DraggingScrollbar) {
		float scale = w->Height / (float)w->RowsCount;
		int row     = (int)((y - w->MouseOffset) / scale);
		/* avoid expensive redraw when possible */
		if (w->TopRow == row) return;

		w->TopRow = row;
		LTable_ClampTopRow(w);
		LWidget_Redraw(w);
	} else if (w->DraggingColumn >= 0) {
		if (!deltaX || x >= w->X + w->Width - 20) return;
		col = w->DraggingColumn;

		w->Columns[col].Width += deltaX;
		Math_Clamp(w->Columns[col].Width, 20, w->Width - 20);
		LWidget_Redraw(w);
	}
}

static void LTable_RowsClick(struct LTable* w) {
	int mouseY = Mouse_Y - w->RowsBegY;
	int row    = w->TopRow + mouseY / w->RowHeight;
	TimeMS now;

	LTable_SetSelectedTo(w, row);
	now = DateTime_CurrentUTC_MS();

	/* double click on row to join */
	if (w->_lastClick + 1000 >= now && row == w->_lastRow) {
		Launcher_ConnectToServer(&LTable_Get(row)->Hash);
	}

	w->_lastRow   = LTable_GetSelectedIndex(w);
	w->_lastClick = now;
}

/* Handles clicking on column headers (either resizes a column or sort rows) */
static void LTable_HeadersClick(struct LTable* w) {
	int x, i, mouseX = Mouse_X;

	for (i = 0, x = w->X; i < w->NumColumns; i++) {
		/* clicked on gridline, begin dragging */
		if (mouseX >= (x - 8) && mouseX < (x + 8) && w->Columns[i].Interactable) {
			w->DraggingColumn = i - 1;
			return;
		}

		x += w->Columns[i].Width;
		if (w->Columns[i].ColumnGridline) x += GRIDLINE_SIZE;
	}

	for (i = 0, x = w->X; i < w->NumColumns; i++) {
		if (mouseX >= x && mouseX < (x + w->Columns[i].Width) && w->Columns[i].Interactable) {
			sortingCol = i;
			w->Columns[i].InvertSort = !w->Columns[i].InvertSort;
			LTable_Sort(w);
			return;
		}

		x += w->Columns[i].Width;
		if (w->Columns[i].ColumnGridline) x += GRIDLINE_SIZE;
	}
}

/* Handles clicking on the scrollbar on right edge of table */
static void LTable_ScrollbarClick(struct LTable* w) {
	int y, height, mouseY = Mouse_Y - w->Y;
	LTable_GetScrollbarCoords(w, &y, &height);

	if (mouseY < y) {
		w->TopRow -= w->VisibleRows;
	} else if (mouseY >= y + height) {
		w->TopRow += w->VisibleRows;
	} else {
		w->MouseOffset = mouseY - y;
	}

	w->DraggingScrollbar = true;
	LTable_ClampTopRow(w);
}

static void LTable_MouseDown(void* widget, bool wasSelected) {
	struct LTable* w = widget;

	if (Mouse_X >= Game_Width - SCROLLBAR_WIDTH) {
		LTable_ScrollbarClick(w);
		w->_lastRow = -1;
	} else if (Mouse_Y < w->RowsBegY) {
		LTable_HeadersClick(w);
		w->_lastRow = -1;
	} else {
		LTable_RowsClick(w);
	}
	LWidget_Draw(w);
}

static void LTable_MouseWheel(void* widget, float delta) {
	struct LTable* w = widget;
	w->TopRow -= Utils_AccumulateWheelDelta(&w->_wheelAcc, delta);
	LTable_ClampTopRow(w);
	LWidget_Draw(w);
	w->_lastRow = -1;
}

/* Stops an in-progress dragging of resizing column. */
static void LTable_StopDragging(struct LTable* table) {
	table->DraggingColumn    = -1;
	table->DraggingScrollbar = false;
	table->MouseOffset       = 0;
}

void LTable_Reposition(struct LTable* w) {
	int rowsHeight;
	w->HdrHeight = Drawer2D_FontHeight(&w->HdrFont, true) + HDR_YPADDING * 2;
	w->RowHeight = Drawer2D_FontHeight(&w->RowFont, true) + ROW_YPADDING * 2;

	w->RowsBegY = w->Y + w->HdrHeight + GRIDLINE_SIZE;
	w->RowsEndY = w->Y + w->Height;
	rowsHeight  = w->Height - (w->RowsBegY - w->Y);

	w->VisibleRows = rowsHeight / w->RowHeight;
	LTable_ClampTopRow(w);
}

static void LTable_Draw(void* widget) {
	struct LTable* w = widget;
	LTable_DrawBackground(w);
	LTable_DrawHeaders(w);
	LTable_DrawRows(w);
	LTable_DrawScrollbar(w);
	Launcher_MarkAllDirty();
}

static struct LWidgetVTABLE ltable_VTABLE = {
	LTable_Draw, NULL,
	LTable_KeyDown,   NULL, /* Key    */
	LTable_MouseMove, NULL, /* Hover  */
	LTable_MouseDown, LTable_StopDragging,  /* Select */
	LTable_MouseWheel,      /* Wheel */
};
void LTable_Init(struct LTable* w, const FontDesc* hdrFont, const FontDesc* rowFont) {
	w->VTABLE     = &ltable_VTABLE;
	w->Columns    = tableColumns;
	w->NumColumns = Array_Elems(tableColumns);

	w->HdrFont = *hdrFont;
	w->RowFont = *rowFont;
}

void LTable_Reset(struct LTable* w) {
	LTable_StopDragging(w);
	LTable_Reposition(w);

	w->TopRow    = 0;
	sortingCol   = -1;
	w->_wheelAcc = 0.0f;

	w->SelectedHash->length = 0;
	w->Filter->length       = 0;
	LTable_Sort(w);
}

void LTable_ApplyFilter(struct LTable* w) {
	int i, j, count;

	count = FetchServersTask.NumServers;
	for (i = 0, j = 0; i < count; i++) {
		if (String_CaselessContains(&Servers_Get(i)->Name, w->Filter)) {
			FetchServersTask.Servers[j++]._order = FetchServersTask.Orders[i];
		}
	}

	w->RowsCount = j;
	for (; j < count; j++) {
		FetchServersTask.Servers[j]._order = -100000;
	}

	w->_lastRow = -1;
	LTable_ClampTopRow(w);
}

static int LTable_SortOrder(const struct ServerInfo* a, const struct ServerInfo* b) {
	int order;
	if (sortingCol >= 0) {
		order = tableColumns[sortingCol].SortOrder(a, b);
		return tableColumns[sortingCol].InvertSort ? -order : order;
	}

	/* Default sort order. (most active server, then by highest uptime) */
	if (a->Players != b->Players) return a->Players - b->Players;
	return a->Uptime - b->Uptime;
}

static void LTable_QuickSort(int left, int right) {
	uint16_t* keys = FetchServersTask.Orders; uint16_t key;

	while (left < right) {
		int i = left, j = right;
		struct ServerInfo* mid = Servers_Get((i + j) >> 1);

		/* partition the list */
		while (i <= j) {
			while (LTable_SortOrder(mid, Servers_Get(i)) < 0) i++;
			while (LTable_SortOrder(mid, Servers_Get(j)) > 0) j--;
			QuickSort_Swap_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(LTable_QuickSort)
	}
}

void LTable_Sort(struct LTable* w) {
	if (!FetchServersTask.NumServers) return;
	FetchServersTask_ResetOrder();
	LTable_QuickSort(0, FetchServersTask.NumServers - 1);

	LTable_ApplyFilter(w);
	LTable_ShowSelected(w);
}

void LTable_ShowSelected(struct LTable* w) {
	int i = LTable_GetSelectedIndex(w);
	if (i == -1) return;

	if (i >= w->TopRow + w->VisibleRows) {
		w->TopRow = i - (w->VisibleRows - 1);
	}
	if (i < w->TopRow) w->TopRow = i;
	LTable_ClampTopRow(w);
}
