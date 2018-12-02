#include "LWidgets.h"
#include "Gui.h"
#include "Game.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "ExtMath.h"

void LWidget_SetLocation(void* widget, uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset) {
	struct Widget* w = widget;
	w->HorAnchor = horAnchor; w->VerAnchor = verAnchor;
	w->XOffset   = xOffset;   w->YOffset = yOffset;
	LWidget_CalcPosition(widget);
}

void LWidget_CalcPosition(void* widget) {
	struct LWidget* w = widget;
	w->X = Gui_CalcPos(w->HorAnchor, w->XOffset, w->Width,  Game_Width);
	w->Y = Gui_CalcPos(w->VerAnchor, w->YOffset, w->Height, Game_Height);
}

void LWidget_Reset(void* widget) {
	struct LWidget* w = widget;
	w->Active   = false;
	w->Hidden   = false;
	w->X = 0; w->Y = 0;
	w->Width = 0; w->Height = 0;
	w->HorAnchor = ANCHOR_MIN;
	w->VerAnchor = ANCHOR_MIN;
	w->XOffset = 0; w->YOffset = 0;

	w->TabSelectable = false;
	w->OnClick = NULL;
	w->Redraw  = NULL;
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
#define BTN_BORDER 1
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
		col = w->Active ? activeCol : inactiveCol;
		Gradient_Noise(&Launcher_Framebuffer, col, 8,
						w->X + BTN_BORDER,         w->Y + BTN_BORDER,
						w->Width - 2 * BTN_BORDER, w->Height - 2 * BTN_BORDER);
	} else {
		col = w->Active ? Launcher_ButtonForeActiveCol : Launcher_ButtonForeCol;
		Gradient_Vertical(&Launcher_Framebuffer, LButton_Expand(col, 8), LButton_Expand(col, -8),
						  w->X + BTN_BORDER,         w->Y + BTN_BORDER,
						  w->Width - 2 * BTN_BORDER, w->Height - 2 * BTN_BORDER);
	}
}

static void LButton_DrawBorder(struct LButton* w) {
	BitmapCol black   = BITMAPCOL_CONST(0, 0, 0, 255);
	BitmapCol backCol = Launcher_ClassicBackground ? black : Launcher_ButtonBorderCol;

	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X + BTN_BORDER,            w->Y,
					w->Width - 2 * BTN_BORDER,    BTN_BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X + BTN_BORDER,            w->Y + w->Height - BTN_BORDER,
					w->Width - 2 * BTN_BORDER,    BTN_BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X,                         w->Y + BTN_BORDER,
					BTN_BORDER,                   w->Height - 2 * BTN_BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					w->X + w->Width - BTN_BORDER, w->Y + BTN_BORDER,
					BTN_BORDER,                   w->Height - 2 * BTN_BORDER);
}

static void LButton_DrawHighlight(struct LButton* w) {
	BitmapCol activeCol   = BITMAPCOL_CONST(189, 198, 255, 255);
	BitmapCol inactiveCol = BITMAPCOL_CONST(168, 168, 168, 255);
	BitmapCol highlightCol;

	if (Launcher_ClassicBackground) {
		highlightCol = w->Active ? activeCol : inactiveCol;
		Drawer2D_Clear(&Launcher_Framebuffer, highlightCol,
						w->X + BTN_BORDER * 2,     w->Y + BTN_BORDER,
						w->Width - BTN_BORDER * 4, BTN_BORDER);
		Drawer2D_Clear(&Launcher_Framebuffer, highlightCol, 
						w->X + BTN_BORDER,         w->Y + BTN_BORDER * 2,
						BTN_BORDER,                w->Height - BTN_BORDER * 4);
	} else if (!w->Active) {
		Drawer2D_Clear(&Launcher_Framebuffer, Launcher_ButtonHighlightCol, 
						w->X + BTN_BORDER * 2,     w->Y + BTN_BORDER,
						w->Width - BTN_BORDER * 4, BTN_BORDER);
	}
}

static void LButton_Redraw(void* widget) {
	struct DrawTextArgs args;
	struct LButton* w = widget;
	int xOffset, yOffset;
	if (w->Hidden) return;

	xOffset = w->Width  - w->__TextSize.Width;
	yOffset = w->Height - w->__TextSize.Height;
	DrawTextArgs_Make(&args, &w->Text, &w->Font, true);

	LButton_DrawBackground(w);
	LButton_DrawBorder(w);
	LButton_DrawHighlight(w);

	if (!w->Active) Drawer2D_Cols['f'] = Drawer2D_Cols['7'];
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, 
					  w->X + xOffset / 2, w->Y + yOffset / 2);
	if (!w->Active) Drawer2D_Cols['f'] = Drawer2D_Cols['F'];
}

void LButton_Init(struct LButton* w, int width, int height) {
	Widget_Reset(w);
	w->TabSelectable = true;
	w->Width  = width; w->Height = height;
	w->Redraw = LButton_Redraw;
	String_InitArray(w->Text, w->__TextBuffer);
}

void LButton_SetText(struct LButton* w, const String* text, const FontDesc* font) {
	struct DrawTextArgs args;
	w->Font = *font;
	String_Copy(&w->Text, text);

	DrawTextArgs_Make(&args, text, font, true);
	w->__TextSize = Drawer2D_MeasureText(&args);
}


/*########################################################################################################################*
*------------------------------------------------------InputWidget--------------------------------------------------------*
*#########################################################################################################################*/
CC_NOINLINE void LInput_Init(struct LInput* w, int width, int height, const char* hintText, const FontDesc* hintFont);
CC_NOINLINE void LInput_SetText(struct LInput* w, const String* text, const FontDesc* font);


/*########################################################################################################################*
*------------------------------------------------------LabelWidget--------------------------------------------------------*
*#########################################################################################################################*/
static void LLabel_Redraw(void* widget) {
	struct DrawTextArgs args;
	struct LLabel* w = widget;
	if (w->Hidden) return;

	DrawTextArgs_Make(&args, &w->Text, &w->Font, true);
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, w->X, w->Y);
}

void LLabel_Init(struct LLabel* w) {
	Widget_Reset(w);
	w->Redraw = LLabel_Redraw;
	String_InitArray(w->Text, w->__TextBuffer);
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
#define SDR_BORDER 1
static void LSlider_DrawBoxBounds(struct LSlider* w) {
	BitmapCol boundsTop    =  BITMAPCOL_CONST(119, 100, 132, 255);
	BitmapCol boundsBottom =  BITMAPCOL_CONST(150, 130, 165, 255);

	/* TODO: Check these are actually right */
	Drawer2D_Clear(&Launcher_Framebuffer, boundsTop,
				  w->X - SDR_BORDER,         w->Y - SDR_BORDER,
				  w->Width + 2 * SDR_BORDER, SDR_BORDER);
	Drawer2D_Clear(&Launcher_Framebuffer, boundsBottom,
				  w->X - SDR_BORDER,         w->Y + w->Height,
				  w->Width + 2 * SDR_BORDER, SDR_BORDER);

	Gradient_Vertical(&Launcher_Framebuffer, boundsTop, boundsBottom,
					 w->X - SDR_BORDER,      w->Y - SDR_BORDER,
				  SDR_BORDER,                w->Height + SDR_BORDER);
	Gradient_Vertical(&Launcher_Framebuffer, boundsTop, boundsBottom,
					 w->X + w->Width,        w->Y - SDR_BORDER,
					 SDR_BORDER,             w->Height + SDR_BORDER);
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
}

void LSlider_Init(struct LSlider* w, int width, int height) {
	Widget_Reset(w);
	w->Width  = width; w->Height = height;
	w->Redraw = LSlider_Redraw;
	w->MaxValue = 100;
}
