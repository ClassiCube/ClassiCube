#include "LBackend.h"
#if defined CC_BUILD_WEB
/* Web backend doesn't use the launcher */
#elif defined CC_BUILD_WIN_TEST
/* Testing windows UI backend */
#include "LBackend_Win.c"
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
#include "PackedCol.h"

/*########################################################################################################################*
*---------------------------------------------------------LWidget---------------------------------------------------------*
*#########################################################################################################################*/
static int xBorder, xBorder2, xBorder3, xBorder4;
static int yBorder, yBorder2, yBorder3, yBorder4;
static int xInputOffset, yInputOffset;

void LBackend_CalcOffsets(void) {
	xBorder = Display_ScaleX(1); xBorder2 = xBorder * 2; xBorder3 = xBorder * 3; xBorder4 = xBorder * 4;
	yBorder = Display_ScaleY(1); yBorder2 = yBorder * 2; yBorder3 = yBorder * 3; yBorder4 = yBorder * 4;

	xInputOffset = Display_ScaleX(5);
	yInputOffset = Display_ScaleY(2);
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

static BitmapCol LButton_Expand(BitmapCol a, int amount) {
	int r, g, b;
	r = BitmapCol_R(a) + amount; Math_Clamp(r, 0, 255);
	g = BitmapCol_G(a) + amount; Math_Clamp(g, 0, 255);
	b = BitmapCol_B(a) + amount; Math_Clamp(b, 0, 255);
	return BitmapCol_Make(r, g, b, 255);
}

static void LButton_DrawBackground(struct LButton* w) {
	BitmapCol color = w->hovered ? Launcher_Theme.ButtonForeActiveColor 
								 : Launcher_Theme.ButtonForeColor;

	if (Launcher_Theme.ClassicBackground) {
		Gradient_Noise(&Launcher_Framebuffer, color, 8,
						w->x + xBorder,      w->y + yBorder,
						w->width - xBorder2, w->height - yBorder2);
	} else {
		
		Gradient_Vertical(&Launcher_Framebuffer, LButton_Expand(color, 8), LButton_Expand(color, -8),
						  w->x + xBorder,      w->y + yBorder,
						  w->width - xBorder2, w->height - yBorder2);
	}
}

static void LButton_DrawBorder(struct LButton* w) {
	BitmapCol backColor = Launcher_Theme.ButtonBorderColor;
	Drawer2D_Clear(&Launcher_Framebuffer, backColor, 
					w->x + xBorder,            w->y,
					w->width - xBorder2,       yBorder);
	Drawer2D_Clear(&Launcher_Framebuffer, backColor, 
					w->x + xBorder,            w->y + w->height - yBorder,
					w->width - xBorder2,       yBorder);
	Drawer2D_Clear(&Launcher_Framebuffer, backColor, 
					w->x,                      w->y + yBorder,
					xBorder,                   w->height - yBorder2);
	Drawer2D_Clear(&Launcher_Framebuffer, backColor, 
					w->x + w->width - xBorder, w->y + yBorder,
					xBorder,                   w->height - yBorder2);
}

static void LButton_DrawHighlight(struct LButton* w) {
	BitmapCol activeColor = BitmapCol_Make(189, 198, 255, 255);
	BitmapCol color = Launcher_Theme.ButtonHighlightColor;

	if (Launcher_Theme.ClassicBackground) {
		if (w->hovered) color = activeColor;

		Drawer2D_Clear(&Launcher_Framebuffer, color,
						w->x + xBorder2,     w->y + yBorder,
						w->width - xBorder4, yBorder);
		Drawer2D_Clear(&Launcher_Framebuffer, color,
						w->x + xBorder,       w->y + yBorder2,
						xBorder,              w->height - yBorder4);
	} else if (!w->hovered) {
		Drawer2D_Clear(&Launcher_Framebuffer, color,
						w->x + xBorder2,      w->y + yBorder,
						w->width - xBorder4,  yBorder);
	}
}

void LBackend_DrawButton(struct LButton* w) {
	struct DrawTextArgs args;
	int xOffset, yOffset;

	xOffset = w->width  - w->_textWidth;
	yOffset = w->height - w->_textHeight;
	DrawTextArgs_Make(&args, &w->text, &Launcher_TitleFont, true);

	LButton_DrawBackground(w);
	LButton_DrawBorder(w);
	LButton_DrawHighlight(w);

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
static const PackedCol checkbox_palette[] = {
	PackedCol_Make(  0,   0,   0,   0), PackedCol_Make(144, 144, 144, 255),
	PackedCol_Make( 61,  61,  61, 255), PackedCol_Make( 94,  94,  94, 255),
	PackedCol_Make(197, 196, 197, 255), PackedCol_Make( 57,  57,  57, 255),
	PackedCol_Make( 33,  33,  33, 255), PackedCol_Make(177, 177, 177, 255),
	PackedCol_Make(189, 189, 189, 255), PackedCol_Make( 67,  67,  67, 255),
	PackedCol_Make(108, 108, 108, 255), PackedCol_Make(171, 171, 171, 255),
	PackedCol_Make(220, 220, 220, 255), PackedCol_Make( 43,  43,  43, 255),
	PackedCol_Make( 63,  63,  63, 255), PackedCol_Make(100, 100, 100, 255),
	PackedCol_Make(192, 192, 192, 255), PackedCol_Make(132, 132, 132, 255),
	PackedCol_Make(175, 175, 175, 255), PackedCol_Make(217, 217, 217, 255),
	PackedCol_Make( 42,  42,  42, 255), PackedCol_Make( 86,  86,  86, 255),
	PackedCol_Make( 56,  56,  56, 255), PackedCol_Make( 76,  76,  76, 255),
	PackedCol_Make(139, 139, 139, 255), PackedCol_Make(130, 130, 130, 255),
	PackedCol_Make(181, 181, 181, 255), PackedCol_Make( 62,  62,  62, 255),
	PackedCol_Make( 75,  75,  75, 255), PackedCol_Make(184, 184, 184, 255),
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
	PackedCol boxTop    = PackedCol_Make(255, 255, 255, 255);
	PackedCol boxBottom = PackedCol_Make(240, 240, 240, 255);
	PackedCol black = PackedCol_Make(0, 0, 0, 255);
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
	DrawBoxBounds(black, w->x, w->y, width, height);

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
		Drawer2D_DrawText(&Launcher_Framebuffer, args, 
							w->x + xInputOffset, y);
	}
}

void LBackend_DrawInput(struct LInput* w, const cc_string* text) {
	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, text, &Launcher_TextFont, false);

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

#define CLASSIC_LINE_COLOR BitmapCol_Make(128,128,128, 255)
void LBackend_DrawLine(struct LLine* w) {
	BitmapCol color = Launcher_Theme.ClassicBackground ? CLASSIC_LINE_COLOR : Launcher_Theme.ButtonBorderColor;
	Gradient_Blend(&Launcher_Framebuffer, color, 128, w->x, w->y, w->width, w->height);
}


/*########################################################################################################################*
*------------------------------------------------------SliderWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_InitSlider(struct LSlider* w, int width, int height) {
	w->width  = Display_ScaleX(width); 
	w->height = Display_ScaleY(height);
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
