#ifndef CC_LWIDGETS_H
#define CC_LWIDGETS_H
#include "Bitmap.h"
#include "String.h"
#include "Constants.h"
/* Describes and manages individual 2D GUI elements in the launcher.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define LWidget_Layout \
	int X, Y, Width, Height;      /* Top left corner, and dimensions, of this widget */ \
	bool Active;                  /* Whether this widget is currently being moused over*/ \
	bool Hidden;                  /* Whether this widget is hidden from view */ \
	bool TabSelectable;           /* Whether this widget gets selected when pressing tab */ \
	uint8_t HorAnchor, VerAnchor; /* Specifies the reference point for when this widget is resized */ \
	int XOffset, YOffset;         /* Offset from the reference point */ \
	void (*OnClick)(void* widget, int x, int y); /* Called when widget is clicked */ \
	void (*Redraw)(void* widget); /* Called to redraw contents of this widget */

/* Represents an individual 2D gui component in the launcher. */
struct LWidget { LWidget_Layout };
void LWidget_SetLocation(void* widget, uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset);
void LWidget_CalcPosition(void* widget);
void LWidget_Reset(void* widget);

struct LButton {
	LWidget_Layout
	String Text;
	FontDesc Font;
	Size2D __TextSize;
	char __TextBuffer[STRING_SIZE];
};
CC_NOINLINE void LButton_Init(struct LButton* w, int width, int height);
CC_NOINLINE void LButton_SetText(struct LButton* w, const String* text, const FontDesc* font);

struct LInput {
	LWidget_Layout
	int BaseWidth, RealWidth;
	/* Text displayed when the user has not entered anything in the text field. */
	const char* HintText;
	/* Whether all characters should be rendered as *. */
	bool Password;
	FontDesc Font, HintFont;
	String Text;
	int __TextHeight;
	char __TextBuffer[STRING_SIZE];
};
CC_NOINLINE void LInput_Init(struct LInput* w, int width, int height, const char* hintText, const FontDesc* hintFont);
CC_NOINLINE void LInput_SetText(struct LInput* w, const String* text, const FontDesc* font);


struct LLabel {
	LWidget_Layout
	FontDesc Font;
	String Text;
	Size2D __TextSize;
	char __TextBuffer[STRING_SIZE];
};
CC_NOINLINE void LLabel_Init(struct LLabel* w);
CC_NOINLINE void LLabel_SetText(struct LLabel* w, const String* text, const FontDesc* font);

/* Represents a slider bar that may or may not be modifiable by the user. */
struct LSlider {
	LWidget_Layout
	int Value, MaxValue;
	BitmapCol ProgCol;
};
CC_NOINLINE void LSlider_Init(struct LSlider* w, int width, int height);
#endif
