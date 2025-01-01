#ifndef CC_LWIDGETS_H
#define CC_LWIDGETS_H
#include "Bitmap.h"
#include "Constants.h"
CC_BEGIN_HEADER

/* Describes and manages individual 2D GUI elements in the launcher.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct FontDesc;
struct Context2D;
struct InputDevice;

enum LWIDGET_TYPE {
	LWIDGET_BUTTON, LWIDGET_CHECKBOX, LWIDGET_INPUT,
	LWIDGET_LABEL,  LWIDGET_LINE, LWIDGET_SLIDER, LWIDGET_TABLE
};

#define LLAYOUT_EXTRA  0x0100
#define LLAYOUT_WIDTH  0x0200
#define LLAYOUT_HEIGHT 0x0300
struct LLayout { short type, offset; };

struct LWidgetVTABLE {
	/* Called to draw contents of this widget */
	void (*Draw)(void* widget);
	/* Called repeatedly to update this widget when selected. */
	void (*Tick)(void* widget);
	/* Called when key is pressed and this widget is selected. */
	/* Returns whether the key press was intercepted */
	cc_bool (*KeyDown)(void* widget, int key, cc_bool wasDown, struct InputDevice* device);
	/* Called when key is pressed and this widget is selected. */
	void (*KeyPress)(void* widget, char c);
	/* Called when mouse hovers/moves over this widget. */
	void (*MouseMove)(void* widget, int idx, cc_bool wasOver);
	/* Called when mouse moves away from this widget. */
	void (*MouseLeft)(void* widget);
	/* Called when mouse clicks on this widget. */
	/* NOTE: This function is just for general widget behaviour. */
	/* OnClick callback is for per-widget instance behaviour. */
	void (*OnSelect)(void* widget, int idx, cc_bool wasSelected);
	/* Called when mouse clicks on another widget. */
	void (*OnUnselect)(void* widget, int idx);
	/* Called when mouse wheel is scrolled and this widget is selected. */
	void (*MouseWheel)(void* widget, float delta);
	/* Called when on-screen keyboard text changed. */
	void (*TextChanged)(void* elem, const cc_string* str);
};


#define LWidget_Layout \
	const struct LWidgetVTABLE* VTABLE; /* General widget functions */ \
	int x, y, width, height; /* Top left corner and dimensions of this widget */ \
	cc_bool hovered;         /* Whether this widget is currently being moused over */ \
	cc_bool selected;        /* Whether this widget is last widget to be clicked on */ \
	cc_bool autoSelectable;  /* Whether this widget can get auto selected (e.g. pressing tab) */ \
	cc_bool dirty;           /* Whether this widget needs to be redrawn */ \
	cc_bool opaque;          /* Whether this widget completely obscures background behind it */ \
	cc_uint8 type;           /* Type of this widget */ \
	cc_bool skipsEnter;      /* Whether clicking this widget DOESN'T trigger OnEnterWidget */ \
	LWidgetFunc OnClick;     /* Called when widget is clicked */ \
	LWidgetFunc OnHover;     /* Called when widget is hovered over */ \
	LWidgetFunc OnUnhover;   /* Called when widget is no longer hovered over */ \
	Rect2D last;             /* Widget's last drawn area */ \
	void* meta;              /* Backend specific data */ \
	const struct LLayout* layouts;

typedef void (*LWidgetFunc)(void* widget);
/* Represents an individual 2D gui component in the launcher. */
struct LWidget { LWidget_Layout };
void LWidget_CalcOffsets(void);

struct LButton {
	LWidget_Layout
	cc_string text;
	int _textWidth, _textHeight;
};
CC_NOINLINE void LButton_Add(void* screen, struct LButton* w, int width, int height, const char* text, 
							LWidgetFunc onClick, const struct LLayout* layouts);
CC_NOINLINE void LButton_SetConst(struct LButton* w, const char* text);
CC_NOINLINE void LButton_DrawBackground(struct Context2D* ctx, int x, int y, int width, int height, cc_bool active);

struct LCheckbox;
typedef void (*LCheckboxChanged)(struct LCheckbox* cb);
struct LCheckbox {
	LWidget_Layout
	cc_bool value;
	cc_string text;
	LCheckboxChanged ValueChanged;
};
CC_NOINLINE void LCheckbox_Add(void* screen, struct LCheckbox* w, const char* text, 
								LCheckboxChanged onChanged, const struct LLayout* layouts);
CC_NOINLINE void LCheckbox_Set(struct LCheckbox* w, cc_bool value);

struct LInput;
struct LInput {
	LWidget_Layout
	int minWidth;
	/* Text displayed when the user has not entered anything in the text field. */
	const char* hintText;
	/* The type of this input (see KEYBOARD_TYPE_ enum in Window.h) */
	/* If type is KEYBOARD_TYPE_PASSWORD, all characters are drawn as *. */
	cc_uint8 inputType;
	/* Whether caret is currently visible */
	cc_bool caretShow;
	/* Filter applied to text received from the clipboard. Can be NULL. */
	void (*ClipboardFilter)(cc_string* str);
	/* Callback invoked when the text is changed. Can be NULL. */
	void (*TextChanged)(struct LInput* w);
	/* Specifies the position that characters are inserted/deleted from. */
	/* NOTE: -1 to insert/delete characters at end of the text. */
	int caretPos;
	cc_string text;
	int _textHeight;
	char _textBuffer[STRING_SIZE * 2];
};
CC_NOINLINE void LInput_Add(void* screen, struct LInput* w, int width, const char* hintText, 
							const struct LLayout* layouts);
CC_NOINLINE void LInput_UNSAFE_GetText(struct LInput* w, cc_string* text);
CC_NOINLINE void LInput_SetText(struct LInput* w, const cc_string* text);
CC_NOINLINE void LInput_ClearText(struct LInput* w);

/* Appends a string to the currently entered text */
CC_NOINLINE void LInput_AppendString(struct LInput* w, const cc_string* str);
/* Sets the currently entered text to the given string */
CC_NOINLINE void LInput_SetString(struct LInput* w, const cc_string* str);
#define LINPUT_HEIGHT 30

/* Represents non-interactable text. */
struct LLabel {
	LWidget_Layout
	cc_bool small; /* whether to use 12pt instead of 14pt font size */
	cc_string text;
	char _textBuffer[STRING_SIZE];
};
CC_NOINLINE void LLabel_Add(void* screen, struct LLabel* w, const char* text, 
							const struct LLayout* layouts);
CC_NOINLINE void LLabel_SetText(struct LLabel* w, const cc_string* text);
CC_NOINLINE void LLabel_SetConst(struct LLabel* w, const char* text);

/* Represents a coloured translucent line separator. */
struct LLine {
	LWidget_Layout
	int _width;
};
CC_NOINLINE void LLine_Add(void* screen, struct LLine* w, int width, 
							const struct LLayout* layouts);
CC_NOINLINE BitmapCol LLine_GetColor(void);
#define LLINE_HEIGHT 2

/* Represents a slider bar that may or may not be modifiable by the user. */
struct LSlider {
	LWidget_Layout
	int value, _width, _height;
	BitmapCol color;
};
CC_NOINLINE void LSlider_Add(void* screen, struct LSlider* w, int width, int height, BitmapCol color, 
							const struct LLayout* layouts);
CC_NOINLINE void LSlider_SetProgress(struct LSlider* w, int progress);

struct ServerInfo;
struct DrawTextArgs;
struct LTableCell;

struct LTableColumn {
	/* Name of this column. */
	const char* name;
	/* Width of this column in pixels. */
	int width;
	/* Draws the value of this column for the given row. */
	/* If args.Text is changed to something, that text gets drawn afterwards. */
	/* Most of the time that's all you need to do. */
	void (*DrawRow)(struct ServerInfo* row, struct DrawTextArgs* args, struct LTableCell* cell, struct Context2D* ctx);
	/* Returns sort order of two rows, based on value of this column in both rows. */
	int (*SortOrder)(const struct ServerInfo* a, const struct ServerInfo* b);
	/* Whether a vertical gridline (and padding) appears after this. */
	cc_bool hasGridline;
	/* Whether user can resize this column. */
	cc_bool draggable;
	/* Whether user can sort this column. */
	cc_bool sortable;
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
	int dragXStart; /* X coordinate column drag started at */
	cc_bool draggingScrollbar; /* Is scrollbar is currently being dragged */
	int dragYOffset;    /* Offset of mouse for scrollbar dragging */

	float _wheelAcc; /* mouse wheel accumulator */
	int _lastRow;    /* last clicked row (for doubleclick join) */
	cc_uint64 _lastClick; /* timestamp of last mouse click on a row */
	int sortingCol;
};

struct LTableCell { struct LTable* table; int x, y, width; };
/* Gets the current ith row */
#define LTable_Get(row) (&FetchServersTask.servers[FetchServersTask.servers[row]._order])

/* Initialises a table. */
/* NOTE: Must also call LTable_Reset to make a table actually useful. */
void LTable_Add(void* screen, struct LTable* table, 
				const struct LLayout* layouts);
/* Resets state of a table (reset sorter, filter, etc) */
void LTable_Reset(struct LTable* table);
/* Whether this table would handle the given key being pressed. */
/* e.g. used so pressing up/down works even when another widget is selected */
cc_bool LTable_HandlesKey(int key, struct InputDevice* device);
/* Filters rows to only show those containing 'w->Filter' in the name. */
void LTable_ApplyFilter(struct LTable* table);
/* Sorts the rows in the table by current Sorter function of table */
void LTable_Sort(struct LTable* table);
/* If selected row is not visible, adjusts top row so it does show. */
void LTable_ShowSelected(struct LTable* table);

void LTable_FormatUptime(cc_string* dst, int uptime);
/* Works out top and height of the scrollbar */
void LTable_GetScrollbarCoords(struct LTable* w, int* y, int* height);
/* Ensures top/first visible row index lies within table */
void LTable_ClampTopRow(struct LTable* w);
/* Returns index of selected row in currently visible rows */
int LTable_GetSelectedIndex(struct LTable* w);
/* Sets selected row to given row, scrolling table if needed */
void LTable_SetSelectedTo(struct LTable* w, int index);
void LTable_RowClick(struct LTable* w, int row);
/* Works out the background color of the given row */
BitmapCol LTable_RowColor(int row, cc_bool selected, cc_bool featured);

CC_END_HEADER
#endif
