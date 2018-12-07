#ifndef CC_LWIDGETS_H
#define CC_LWIDGETS_H
#include "Bitmap.h"
#include "String.h"
#include "Constants.h"
/* Describes and manages individual 2D GUI elements in the launcher.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct LWidgetVTABLE {
	/* Called to redraw contents of this widget */
	void (*Redraw)(void* widget);
	/* Called when mouse hovers over this widget. */
	void (*OnHover)(void* widget);
	/* Called when mouse moves away from this widget. */
	void (*OnUnhover)(void* widget);
	/* Called when mouse clicks on this widget. */
	/* NOTE: This function is just for general widget behaviour. */
	/* OnClick callback is for per-widget instance behaviour. */
	void (*OnSelect)(void* widget);
	/* Called when mouse clicks on another widget. */
	void (*OnUnselect)(void* widget);
};

#define LWidget_Layout \
	struct LWidgetVTABLE* VTABLE; /* General widget functions */ \
	int X, Y, Width, Height;      /* Top left corner, and dimensions, of this widget */ \
	bool Hovered;                 /* Whether this widget is currently being moused over */ \
	bool Selected;                /* Whether this widget is last widget to be clicked on */ \
	bool Hidden;                  /* Whether this widget is hidden from view */ \
	bool TabSelectable;           /* Whether this widget gets selected when pressing tab */ \
	uint8_t HorAnchor, VerAnchor; /* Specifies the reference point for when this widget is resized */ \
	int XOffset, YOffset;         /* Offset from the reference point */ \
	void (*OnClick)(void* widget, int x, int y); /* Called when widget is clicked */ \

/* Represents an individual 2D gui component in the launcher. */
struct LWidget { LWidget_Layout };
void LWidget_SetLocation(void* widget, uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset);
void LWidget_CalcPosition(void* widget);
void LWidget_Reset(void* widget);

struct LButton {
	LWidget_Layout
	String Text;
	FontDesc Font;
	Size2D _TextSize;
	char _TextBuffer[STRING_SIZE];
};
CC_NOINLINE void LButton_Init(struct LButton* w, int width, int height);
CC_NOINLINE void LButton_SetText(struct LButton* w, const String* text, const FontDesc* font);

struct LInput;
struct LInput {
	LWidget_Layout
	int BaseWidth, _RealWidth;
	/* Text displayed when the user has not entered anything in the text field. */
	const char* HintText;
	/* Whether all characters should be rendered as *. */
	bool Password;
	/* Filter applied to text received from the clipboard. Can be NULL. */
	void (*ClipboardFilter)(String* text);
	/* Callback invoked when the text is changed. Can be NULL. */
	void (*TextChanged)(struct LInput* w);
	/* Filter that only lets certain characters be entered. Can be NULL. */
	bool (*TextFilter)(char c);
	/* Specifies the position that characters are inserted/deleted from. */
	/* NOTE: -1 to insert/delete characters at end of the text. */
	int CaretPos;
	FontDesc Font, HintFont;
	String Text;
	int _TextHeight;
	char _TextBuffer[STRING_SIZE];
};
CC_NOINLINE void LInput_Init(struct LInput* w, int width, int height, const char* hintText, const FontDesc* hintFont);
CC_NOINLINE void LInput_SetText(struct LInput* w, const String* text, const FontDesc* font);
CC_NOINLINE Rect2D LInput_MeasureCaret(struct LInput* w);
CC_NOINLINE void LInput_AdvanceCaretPos(struct LInput* w, bool forwards);
CC_NOINLINE void LInput_SetCaretToCursor(struct LInput* w, int x, int y);

/* Appends a character to the end of the currently entered text. */
CC_NOINLINE bool LInput_Append(struct LInput* w, char c);
/* Removes the character preceding the caret in the currently entered text. */
CC_NOINLINE bool LInput_Backspace(struct LInput* w);
/* Removes the character at the caret in the currently entered text. */
CC_NOINLINE bool LInput_Delete(struct LInput* w);
/* Resets the currently entered text to an empty string. */
CC_NOINLINE bool LInput_Clear(struct LInput* w);
/* Appends the contents of the system clipboard to the currently entered text. */
CC_NOINLINE bool LInput_CopyFromClipboard(struct LInput* w);

struct LLabel {
	LWidget_Layout
	FontDesc Font;
	String Text;
	Size2D _TextSize;
	char _TextBuffer[STRING_SIZE];
};
CC_NOINLINE void LLabel_Init(struct LLabel* w);
CC_NOINLINE void LLabel_SetText(struct LLabel* w, const String* text, const FontDesc* font);

/* Represents a slider bar that may or may not be modifiable by the user. */
struct LSlider {
	LWidget_Layout
	int Value, MaxValue;
	BitmapCol ProgressCol;
};
CC_NOINLINE void LSlider_Init(struct LSlider* w, int width, int height);
#endif
