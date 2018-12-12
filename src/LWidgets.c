#include "LWidgets.h"
#include "Gui.h"
#include "Game.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "ExtMath.h"
#include "Window.h"
#include "Funcs.h"
#include "LWeb.h"

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
	Launcher_Dirty = true;
}

static struct LWidgetVTABLE lbutton_VTABLE = {
	LButton_Draw, 
	NULL, NULL,                 /* Key    */
	LButton_Draw, LButton_Draw, /* Hover  */
	NULL, NULL                  /* Select */
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
	LInput_Draw, 
	LInput_KeyDown, LInput_KeyChar, /* Key    */
	NULL, NULL,                     /* Hover  */
	/* TODO: Don't redraw whole thing, just the outer border */
	LInput_Draw, LInput_Draw        /* Select */
};
void LInput_Init(struct LInput* w, const FontDesc* font, int width, int height, const char* hintText, const FontDesc* hintFont) {
	w->VTABLE = &linput_VTABLE;
	w->TabSelectable = true;
	String_InitArray(w->Text, w->_TextBuffer);
	w->Font  = *font;

	w->BaseWidth = width;
	w->Width = width; w->Height = height;
	LWidget_CalcPosition(w);

	w->HintFont = *hintFont;
	w->HintText = hintText;
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
static void LLabel_Draw(void* widget) {
	struct DrawTextArgs args;
	struct LLabel* w = widget;
	if (w->Hidden) return;

	DrawTextArgs_Make(&args, &w->Text, &w->Font, true);
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, w->X, w->Y);
	Launcher_Dirty = true;
}

static struct LWidgetVTABLE llabel_VTABLE = {
	LLabel_Draw, 
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

static void LSlider_Draw(void* widget) {
	struct LSlider* w = widget;
	LSlider_DrawBoxBounds(w);
	LSlider_DrawBox(w);

	Drawer2D_Clear(&Launcher_Framebuffer, w->ProgressCol,
				   w->X, w->Y, (int)(w->Width * w->Value / w->MaxValue), w->Height);
	Launcher_Dirty = true;
}

static struct LWidgetVTABLE lslider_VTABLE = {
	LSlider_Draw, 
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
static void NameColumn_Get(struct ServerInfo* row, String* str) {
	String_Copy(str, &row->Name);
}
static int NameColumn_Sort(struct ServerInfo* a, struct ServerInfo* b) {
	return String_Compare(&a->Name, &b->Name);
}

static void PlayersColumn_Get(struct ServerInfo* row, String* str) {
	String_Format2(str, "%i/%i", &row->Players, &row->MaxPlayers);
}
static int PlayersColumn_Sort(struct ServerInfo* a, struct ServerInfo* b) {
	return b->Players - a->Players;
}

static void UptimeColumn_Get(struct ServerInfo* row, String* str) {
	int uptime = row->Uptime;
	char unit  = 's';

	if (uptime >= SECS_PER_DAY * 7) {
		uptime /= SECS_PER_DAY;  unit = 'd';
	} else if (uptime >= SECS_PER_HOUR) {
		uptime /= SECS_PER_HOUR; unit = 'h';
	} else if (uptime >= SECS_PER_MIN) {
		uptime /= SECS_PER_MIN;  unit = 'm';
	}
	String_Format2(str, "%i%r", &uptime, &unit);
}
static int UptimeColumn_Sort(struct ServerInfo* a, struct ServerInfo* b) {
	return b->Uptime - a->Uptime;
}

static void SoftwareColumn_Get(struct ServerInfo* row, String* str) {
	String_Copy(str, &row->Software);
}
static int SoftwareColumn_Sort(struct ServerInfo* a, struct ServerInfo* b) {
	return String_Compare(&a->Software, &b->Software);
}

static struct LTableColumn tableColumns[4] = {
	{ "Name",     320, 320, NameColumn_Get,     NameColumn_Sort     },
	{ "Players",   65,  65, PlayersColumn_Get,  PlayersColumn_Sort  },
	{ "Uptime",    65,  65, UptimeColumn_Get,   UptimeColumn_Sort   },
	{ "Software", 140, 140, SoftwareColumn_Get, SoftwareColumn_Sort }
};

/*
PackedCol backGridCol = new PackedCol(20, 20, 10);
PackedCol foreGridCol = new PackedCol(40, 40, 40);

int entryHeight, headerHeight;
int headerStartY, headerEndY;
int numEntries, maxIndex;
Font font, titleFont;

void SetDrawData(IDrawer2D drawer, Font font, Font titleFont) {
	this.font = font;
	this.titleFont = titleFont;

	headerHeight = drawer.FontHeight(titleFont, true);
	entryHeight = drawer.FontHeight(font, true);
}

void RecalculateDrawData() {
		for (int i = 0; i < table.ColumnWidths.Length; i++) {
			table.ColumnWidths[i] = table.DesiredColumnWidths[i];
			Utils.Clamp(ref table.ColumnWidths[i], 20, game.Width - 20);
		}
		table.Width = game.Width - table.X;
		ResetEntries();

		int y = table.Y + 3;
		y += headerHeight + 2;
		maxIndex = table.Count;
		y += 5;

		for (int i = table.CurrentIndex; i < table.Count; i++) {
			if (y + entryHeight > table.Y + table.Height) {
				maxIndex = i; return;
			}

			table.entries[table.order[i]].Y = y;
			table.entries[table.order[i]].Height = entryHeight;
			y += entryHeight + 2;
		}
	}

	const int flagPadding = 15;
	void RedrawData(IDrawer2D drawer) {
		DrawGrid(drawer);
		int x = table.X + flagPadding + 5;
		x += DrawColumn(drawer, "Name",     0, x, filterName) + 5;
		x += DrawColumn(drawer, "Players",  1, x, filterPlayers) + 5;
		x += DrawColumn(drawer, "Uptime",   2, x, filterUptime) + 5;
		x += DrawColumn(drawer, "Software", 3, x, filterSoftware) + 5;

		DrawScrollbar(drawer);
		DrawFlags();
	}

	void Redraw(IDrawer2D drawer) {
		RecalculateDrawData();
		RedrawData(drawer);
	}

	delegate string ColumnFilter(TableEntry entry);
	// cache to avoid allocations every redraw
	static string FilterName(TableEntry e) { return e.Name; }     static ColumnFilter filterName = FilterName;
	static string FilterPlayers(TableEntry e) { return e.Players; }  static ColumnFilter filterPlayers = FilterPlayers;
	static string FilterUptime(TableEntry e) { return e.Uptime; }   static ColumnFilter filterUptime = FilterUptime;
	static string FilterSoftware(TableEntry e) { return e.Software; } static ColumnFilter filterSoftware = FilterSoftware;

	static FastBitmap GetFlag(string flag) {
		List<string> flags = FetchFlagsTask.Flags;
		List<FastBitmap> bitmaps = FetchFlagsTask.Bitmaps;

		for (int i = 0; i < flags.Count; i++) {
			if (flags[i] != flag) continue;
			return i < bitmaps.Count ? bitmaps[i] : null;
		}
		return null;
	}

	public void DrawFlags() {
		using (FastBitmap dst = game.LockBits()) {
			for (int i = table.CurrentIndex; i < maxIndex; i++) {
				TableEntry entry = table.Get(i);
				FastBitmap flag = GetFlag(entry.Flag);
				if (flag == null) continue;

				int x = table.X, y = entry.Y;
				Rectangle rect = new Rectangle(x + 2, y + 3, 16, 11);
				BitmapDrawer.Draw(flag, dst, rect);
			}
		}
	}

	int DrawColumn(IDrawer2D drawer, string header, int columnI, int x, ColumnFilter filter) {
		int y = table.Y + 3;
		int maxWidth = table.ColumnWidths[columnI];
		bool separator = columnI > 0;

		DrawTextArgs args = new DrawTextArgs(header, titleFont, true);
		TableEntry headerEntry = default(TableEntry);
		DrawColumnEntry(drawer, ref args, maxWidth, x, ref y, ref headerEntry);
		maxIndex = table.Count;

		y += 5;
		for (int i = table.CurrentIndex; i < table.Count; i++) {
			TableEntry entry = table.Get(i);
			args = new DrawTextArgs(filter(entry), font, true);

			if ((i == table.SelectedIndex || entry.Featured) && !separator) {
				int startY = y - 3;
				int height = Math.Min(startY + (entryHeight + 4), table.Y + table.Height) - startY;
				drawer.Clear(GetGridCol(entry.Featured, i == table.SelectedIndex), table.X, startY, table.Width, height);
			}
			if (!DrawColumnEntry(drawer, ref args, maxWidth, x, ref y, ref entry)) {
				maxIndex = i; break;
			}
		}
		if (separator && !game.ClassicBackground) {
			drawer.Clear(LauncherSkin.BackgroundCol, x - 7, table.Y, 2, table.Height);
		}
		return maxWidth + 5;
	}

	PackedCol GetGridCol(bool featured, bool selected) {
		if (featured) {
			if (selected) return new PackedCol(50, 53, 0);
			return new PackedCol(101, 107, 0);
		}
		return foreGridCol;
	}

	bool DrawColumnEntry(IDrawer2D drawer, ref DrawTextArgs args,
		int maxWidth, int x, ref int y, ref TableEntry entry) {
		Size size = drawer.MeasureText(ref args);
		bool empty = args.Text == "";
		if (empty)
			size.Height = entryHeight;
		if (y + size.Height > table.Y + table.Height) {
			y = table.Y + table.Height + 2; return false;
		}

		entry.Y = y; entry.Height = size.Height;
		if (!empty) {
			size.Width = Math.Min(maxWidth, size.Width);
			args.SkipPartsCheck = false;
			Drawer2DExt.DrawClippedText(ref args, drawer, x, y, maxWidth);
		}
		y += size.Height + 2;
		return true;
	}

	void ResetEntries() {
		for (int i = 0; i < table.Count; i++) {
			table.entries[i].Height = 0;
			table.entries[i].Y = -10;
		}
	}

	void DrawGrid(IDrawer2D drawer) {
		if (!game.ClassicBackground)
			drawer.Clear(LauncherSkin.BackgroundCol,
				table.X, table.Y + headerHeight + 5, table.Width, 2);
		headerStartY = table.Y;

		headerEndY = table.Y + headerHeight + 5;
		int startY = headerEndY + 3;
		numEntries = (table.Y + table.Height - startY) / (entryHeight + 3);
	}

	void DrawScrollbar(IDrawer2D drawer) {
		PackedCol col = game.ClassicBackground ? new PackedCol(80, 80, 80) : LauncherSkin.ButtonBorderCol;
		drawer.Clear(col, game.Width - 10, table.Y, 10, table.Height);
		col = game.ClassicBackground ? new PackedCol(160, 160, 160) : LauncherSkin.ButtonForeActiveCol;
		int yOffset, height;
		table.GetScrollbarCoords(out yOffset, out height);
		drawer.Clear(col, game.Width - 10, table.Y + yOffset, 10, height);
	}
}
}

internal struct TableEntry {
	public string Hash, Name, Players, Uptime, Software, RawUptime, Flag;
	public int Y, Height;
	public bool Featured;
}

public delegate void TableNeedsRedrawHandler();

	TableView view;
	public TableWidget(LauncherWindow window) : base(window) {
		OnClick = HandleOnClick;
		view = new TableView();
		view.Init(window, this);
	}

	public TableNeedsRedrawHandler NeedRedraw;
	public Action<string> SelectedChanged;
	public int SelectedIndex = -1;
	public string SelectedHash = "";
	public int CurrentIndex, Count;

	internal TableEntry[] entries;
	internal int[] order;
	internal List<ServerListEntry> servers;
	internal TableEntry Get(int i) { return entries[order[i]]; }

	public void SetEntries(List<ServerListEntry> servers) {
		entries = new TableEntry[servers.Count];
		order = new int[servers.Count];
		this.servers = servers;

		for (int i = 0; i < servers.Count; i++) {
			ServerListEntry e = servers[i];
			TableEntry tableEntry = default(TableEntry);
			tableEntry.Hash = e.Hash;
			tableEntry.Name = e.Name;
			tableEntry.Players = e.Players + "/" + e.MaxPlayers;
			tableEntry.Software = e.Software;
			tableEntry.Uptime = MakeUptime(e.Uptime);
			tableEntry.RawUptime = e.Uptime;
			tableEntry.Featured = e.Featured;
			tableEntry.Flag = e.Flag;

			entries[i] = tableEntry;
			order[i] = i;
		}
		Count = entries.Length;
	}

	string curFilter;
	public void FilterEntries(string filter) {
		curFilter = filter;
		Count = 0;
		for (int i = 0; i < entries.Length; i++) {
			TableEntry entry = entries[i];
			if (entry.Name.IndexOf(filter, StringComparison.OrdinalIgnoreCase) >= 0) {
				order[Count++] = i;
			}
		}
		for (int i = Count; i < entries.Length; i++) {
			order[i] = -100000;
		}
	}

	internal void GetScrollbarCoords(out int y, out int height) {
		if (Count == 0) { y = 0; height = 0; return; }

		float scale = Height / (float)Count;
		y = (int)Math.Ceiling(CurrentIndex * scale);
		height = (int)Math.Ceiling((view.maxIndex - CurrentIndex) * scale);
		height = Math.Min(y + height, Height) - y;
	}

	public void SetSelected(int index) {
		if (index >= view.maxIndex) CurrentIndex = index + 1 - view.numEntries;
		if (index < CurrentIndex) CurrentIndex = index;
		if (index >= Count) index = Count - 1;
		if (index < 0) index = 0;

		SelectedHash = "";
		SelectedIndex = index;
		lastIndex = index;
		ClampIndex();

		if (Count > 0) {
			TableEntry entry = Get(index);
			SelectedChanged(entry.Hash);
			SelectedHash = entry.Hash;
		}
	}

	public void SetSelected(string hash) {
		SelectedIndex = -1;
		for (int i = 0; i < Count; i++) {
			if (Get(i).Hash != hash) continue;
			SetSelected(i);
			return;
		}
	}

	public void ClampIndex() {
		if (CurrentIndex > Count - view.numEntries)
			CurrentIndex = Count - view.numEntries;
		if (CurrentIndex < 0)
			CurrentIndex = 0;
	}

	string MakeUptime(string rawSeconds) {
		TimeSpan t = TimeSpan.FromSeconds(Double.Parse(rawSeconds));
		if (t.TotalHours >= 24 * 7)
			return (int)t.TotalDays + "d";
		if (t.TotalHours >= 1)
			return (int)t.TotalHours + "h";
		if (t.TotalMinutes >= 1)
			return (int)t.TotalMinutes + "m";
		return (int)t.TotalSeconds + "s";
	}


	public int[] ColumnWidths = new int[] { 320, 65, 65, 140 };
	public int[] DesiredColumnWidths = new int[] { 320, 65, 65, 140 };

	public void SetDrawData(IDrawer2D drawer, Font font, Font titleFont,
		Anchor horAnchor, Anchor verAnchor, int x, int y) {
		SetLocation(horAnchor, verAnchor, x, y);
		view.SetDrawData(drawer, font, titleFont);
	}

	public void RecalculateDrawData() { view.RecalculateDrawData(); }
	public void RedrawData(IDrawer2D drawer) { view.RedrawData(drawer); }
	public void RedrawFlags() { view.DrawFlags(); }

	public override void Redraw(IDrawer2D drawer) {
		RecalculateDrawData();
		RedrawData(drawer);
	}


	DefaultComparer defComp = new DefaultComparer();
	NameComparer nameComp = new NameComparer();
	PlayersComparer playerComp = new PlayersComparer();
	UptimeComparer uptimeComp = new UptimeComparer();
	SoftwareComparer softwareComp = new SoftwareComparer();
	internal int DraggingColumn = -1;
	internal bool DraggingScrollbar = false;
	internal int mouseOffset;

	public void SortDefault() {
		SortEntries(defComp, true);
	}

	void SelectHeader(int mouseX, int mouseY) {
		int x = X + 15;
		for (int i = 0; i < ColumnWidths.Length; i++) {
			x += ColumnWidths[i] + 10;
			if (mouseX >= x - 8 && mouseX < x + 8) {
				DraggingColumn = i;
				lastIndex = -10; return;
			}
		}
		TrySortColumns(mouseX);
	}

	void TrySortColumns(int mouseX) {
		int x = X + TableView.flagPadding;
		if (mouseX >= x && mouseX < x + ColumnWidths[0]) {
			SortEntries(nameComp, false); return;
		}

		x += ColumnWidths[0] + 10;
		if (mouseX >= x && mouseX < x + ColumnWidths[1]) {
			SortEntries(playerComp, false); return;
		}

		x += ColumnWidths[1] + 10;
		if (mouseX >= x && mouseX < x + ColumnWidths[2]) {
			SortEntries(uptimeComp, false); return;
		}

		x += ColumnWidths[2] + 10;
		if (mouseX >= x) {
			SortEntries(softwareComp, false); return;
		}
	}

	void SortEntries(TableEntryComparer comparer, bool noRedraw) {
		Array.Sort(entries, 0, entries.Length, comparer);
		lastIndex = -10;
		if (curFilter != null && curFilter.Length > 0) {
			FilterEntries(curFilter);
		}

		if (noRedraw) return;
		comparer.Invert = !comparer.Invert;
		SetSelected(SelectedHash);
		NeedRedraw();
	}

	void GetSelectedServer(int mouseX, int mouseY) {
		for (int i = 0; i < Count; i++) {
			TableEntry entry = Get(i);
			if (mouseY < entry.Y || mouseY >= entry.Y + entry.Height + 2) continue;

			if (lastIndex == i && (DateTime.UtcNow - lastPress).TotalSeconds < 1) {
				Window.ConnectToServer(servers, entry.Hash);
				lastPress = DateTime.MinValue;
				return;
			}

			SetSelected(i);
			NeedRedraw();
			break;
		}
	}

	void HandleOnClick(int mouseX, int mouseY) {
		if (mouseX >= Window.Width - 10) {
			ScrollbarClick(mouseY);
			DraggingScrollbar = true;
			lastIndex = -10; return;
		}

		if (mouseY >= view.headerStartY && mouseY < view.headerEndY) {
			SelectHeader(mouseX, mouseY);
		} else {
			GetSelectedServer(mouseX, mouseY);
		}
		lastPress = DateTime.UtcNow;
	}

	int lastIndex = -10;
	DateTime lastPress;
	public void MouseMove(int x, int y, int deltaX, int deltaY) {
		if (DraggingScrollbar) {
			y -= Y;
			float scale = Height / (float)Count;
			CurrentIndex = (int)((y - mouseOffset) / scale);
			ClampIndex();
			NeedRedraw();
		} else if (DraggingColumn >= 0) {
			if (x >= Window.Width - 20) return;
			int col = DraggingColumn;
			ColumnWidths[col] += deltaX;
			Utils.Clamp(ref ColumnWidths[col], 20, Window.Width - 20);
			DesiredColumnWidths[col] = ColumnWidths[col];
			NeedRedraw();
		}

	}

	void ScrollbarClick(int mouseY) {
		mouseY -= Y;
		int y, height;
		GetScrollbarCoords(out y, out height);
		int delta = (view.maxIndex - CurrentIndex);

		if (mouseY < y) {
			CurrentIndex -= delta;
		} else if (mouseY >= y + height) {
			CurrentIndex += delta;
		} else {
			DraggingScrollbar = true;
			mouseOffset = mouseY - y;
		}
		ClampIndex();
		NeedRedraw();
	}
}
}
*/