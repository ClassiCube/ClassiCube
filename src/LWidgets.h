#ifndef CC_LWIDGETS_H
#define CC_LWIDGETS_H
#include "Bitmap.h"
#include "Constants.h"
/* Describes and manages individual 2D GUI elements in the launcher.
   Copyright 2014-2020 ClassiCube | Licensed under BSD-3
*/
struct LScreen;
struct FontDesc;

struct LWidgetVTABLE {
	/* Called to draw contents of this widget */
	void (*Draw)(void* widget);
	/* Called repeatedly to update this widget when selected. */
	void (*Tick)(void* widget);
	/* Called when key is pressed and this widget is selected. */
	void (*KeyDown)(void* widget, int key, cc_bool wasDown);
	/* Called when key is pressed and this widget is selected. */
	void (*KeyPress)(void* widget, char c);
	/* Called when mouse hovers/moves over this widget. */
	void (*MouseMove)(void* widget, int deltaX, int deltaY, cc_bool wasOver);
	/* Called when mouse moves away from this widget. */
	void (*MouseLeft)(void* widget);
	/* Called when mouse clicks on this widget. */
	/* NOTE: This function is just for general widget behaviour. */
	/* OnClick callback is for per-widget instance behaviour. */
	void (*OnSelect)(void* widget, cc_bool wasSelected);
	/* Called when mouse clicks on another widget. */
	void (*OnUnselect)(void* widget);
	/* Called when mouse wheel is scrolled and this widget is selected. */
	void (*MouseWheel)(void* widget, float delta);
	/* Called when on-screen keyboard text changed. */
	void (*TextChanged)(void* elem, const cc_string* str);
};

#define LWidget_Layout \
	const struct LWidgetVTABLE* VTABLE;  /* General widget functions */ \
	int x, y, width, height;       /* Top left corner and dimensions of this widget */ \
	cc_bool hovered;               /* Whether this widget is currently being moused over */ \
	cc_bool selected;              /* Whether this widget is last widget to be clicked on */ \
	cc_bool hidden;                /* Whether this widget is hidden from view */ \
	cc_bool tabSelectable;         /* Whether this widget gets selected when pressing tab */ \
	cc_uint8 horAnchor, verAnchor; /* Specifies the reference point for when this widget is resized */ \
	int xOffset, yOffset;          /* Offset from the reference point */ \
	void (*OnClick)(void* widget, int x, int y); /* Called when widget is clicked */ \
	Rect2D last;                  /* Widget's last drawn area */

/* Represents an individual 2D gui component in the launcher. */
struct LWidget { LWidget_Layout };
void LWidget_SetLocation(void* widget, cc_uint8 horAnchor, cc_uint8 verAnchor, int xOffset, int yOffset);
void LWidget_CalcPosition(void* widget);
void LWidget_Draw(void* widget);
void LWidget_Redraw(void* widget);
void LWidget_CalcOffsets(void);

struct LButton {
	LWidget_Layout
	cc_string text;
	int _textWidth, _textHeight;
};
CC_NOINLINE void LButton_Init(struct LScreen* s, struct LButton* w, int width, int height, const char* text);
CC_NOINLINE void LButton_SetConst(struct LButton* w, const char* text);

struct LInput;
struct LInput {
	LWidget_Layout
	int minWidth;
	/* Text displayed when the user has not entered anything in the text field. */
	const char* hintText;
	/* The type of this input (see KEYBOARD_TYPE_ enum in Window.h) */
	/* If type is KEYBOARD_TYPE_PASSWORD, all characters are drawn as *. */
	cc_uint8 type;
	/* Filter applied to text received from the clipboard. Can be NULL. */
	void (*ClipboardFilter)(cc_string* str);
	/* Callback invoked when the text is changed. Can be NULL. */
	void (*TextChanged)(struct LInput* w);
	/* Callback invoked whenever user attempts to append a character to the text. */
	cc_bool (*TextFilter)(char c);
	/* Specifies the position that characters are inserted/deleted from. */
	/* NOTE: -1 to insert/delete characters at end of the text. */
	int caretPos;
	cc_string text;
	int _textHeight;
	char _textBuffer[STRING_SIZE];
};
CC_NOINLINE void LInput_Init(struct LScreen* s, struct LInput* w, int width, const char* hintText);
CC_NOINLINE void LInput_SetText(struct LInput* w, const cc_string* text);

/* Appends a character to the currently entered text. */
CC_NOINLINE void LInput_Append(struct LInput* w, char c);
/* Appends a string to the currently entered text. */
CC_NOINLINE void LInput_AppendString(struct LInput* w, const cc_string* str);
/* Removes the character preceding the caret in the currently entered text. */
CC_NOINLINE void LInput_Backspace(struct LInput* w);
/* Removes the character at the caret in the currently entered text. */
CC_NOINLINE void LInput_Delete(struct LInput* w);
/* Resets the currently entered text to an empty string. */
CC_NOINLINE void LInput_Clear(struct LInput* w);

/* Represents non-interactable text. */
struct LLabel {
	LWidget_Layout
	struct FontDesc* font;
	cc_string text;
	char _textBuffer[STRING_SIZE];
};
CC_NOINLINE void LLabel_Init(struct LScreen* s, struct LLabel* w, const char* text);
CC_NOINLINE void LLabel_SetText(struct LLabel* w, const cc_string* text);
CC_NOINLINE void LLabel_SetConst(struct LLabel* w, const char* text);

/* Represents a coloured translucent line separator. */
struct LLine {
	LWidget_Layout
};
CC_NOINLINE void LLine_Init(struct LScreen* s, struct LLine* w, int width);

/* Represents a slider bar that may or may not be modifiable by the user. */
struct LSlider {
	LWidget_Layout
	int value, maxValue;
	BitmapCol col;
};
CC_NOINLINE void LSlider_Init(struct LScreen* s, struct LSlider* w, int width, int height, BitmapCol col);

struct ServerInfo;
struct DrawTextArgs;

struct LTableColumn {
	/* Name of this column. */
	const char* name;
	/* Width of this column in pixels. */
	int width;
	/* Draws the value of this column for the given row. */
	/* If args.Text is changed to something, that text gets drawn afterwards. */
	/* Most of the time that's all you need to do. */
	void (*DrawRow)(struct ServerInfo* row, struct DrawTextArgs* args, int x, int y);
	/* Returns sort order of two rows, based on value of this column in both rows. */
	int (*SortOrder)(const struct ServerInfo* a, const struct ServerInfo* b);
	/* Whether a vertical gridline (and padding) appears after this. */
	cc_bool hasGridline;
	/* Whether user can interact with this column. */
	/* Non-interactable columns can't be sorted/resized. */
	cc_bool interactable;
	/* Whether to invert the order of row sorting. */
	cc_bool invertSort;
};

/* Represents a table of server entries. */
struct LTable {
	LWidget_Layout
	/* Columns of the table. */
	struct LTableColumn* columns;
	/* Number of columns in the table. */
	int numColumns;
	/* Fonts for text in rows. */
	struct FontDesc* rowFont;
	/* Y start and end of rows and height of each row. */
	int rowsBegY, rowsEndY, rowHeight;
	/* Y height of headers. */
	int hdrHeight;
	/* Maximum number of rows visible. */
	int visibleRows;
	/* Total number of rows in the table (after filter is applied). */
	int rowsCount;
	/* Index of top row currently visible. */
	int topRow;

	/* Hash of the currently selected server. */
	cc_string* selectedHash;
	/* Filter for which server names to show. */
	cc_string* filter;
	/* Callback when selected has has changed. */
	void (*OnSelectedChanged)(void);

	/* Index of table column currently being dragged. */
	int draggingColumn;
	cc_bool draggingScrollbar; /* Is scrollbar is currently being dragged */
	int mouseOffset;    /* Offset of mouse for scrollbar dragging */

	float _wheelAcc; /* mouse wheel accumulator */
	int _lastRow;    /* last clicked row (for doubleclick join) */
	TimeMS _lastClick; /* time of last mouse click on a row */
};
/* Gets the current ith row */
#define LTable_Get(row) (&FetchServersTask.servers[FetchServersTask.servers[row]._order])

/* Initialises a table. */
/* NOTE: Must also call LTable_Reset to make a table actually useful. */
void LTable_Init(struct LTable* table, struct FontDesc* rowFont);
/* Resets state of a table (reset sorter, filter, etc) */
void LTable_Reset(struct LTable* table);
/* Adjusts Y position of rows and number of visible rows. */
void LTable_Reposition(struct LTable* table);
/* Whether this table would handle the given key being pressed. */
/* e.g. used so pressing up/down works even when another widget is selected */
cc_bool LTable_HandlesKey(int key);
/* Filters rows to only show those containing 'w->Filter' in the name. */
void LTable_ApplyFilter(struct LTable* table);
/* Sorts the rows in the table by current Sorter function of table */
void LTable_Sort(struct LTable* table);
/* If selected row is not visible, adjusts top row so it does show. */
void LTable_ShowSelected(struct LTable* table);
#endif
