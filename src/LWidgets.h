#ifndef CC_LWIDGETS_H
#define CC_LWIDGETS_H
#include "Bitmap.h"
#include "Constants.h"
#include "Input.h"
/* Describes and manages individual 2D GUI elements in the launcher.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

struct LWidgetVTABLE {
	/* Called to draw contents of this widget */
	void (*Draw)(void* widget);
	/* Called repeatedly to update this widget when selected. */
	void (*Tick)(void* widget);
	/* Called when key is pressed and this widget is selected. */
	void (*KeyDown)(void* widget, Key key, bool wasDown);
	/* Called when key is pressed and this widget is selected. */
	void (*KeyPress)(void* widget, char c);
	/* Called when mouse hovers/moves over this widget. */
	void (*MouseMove)(void* widget, int deltaX, int deltaY, bool wasOver);
	/* Called when mouse moves away from this widget. */
	void (*MouseLeft)(void* widget);
	/* Called when mouse clicks on this widget. */
	/* NOTE: This function is just for general widget behaviour. */
	/* OnClick callback is for per-widget instance behaviour. */
	void (*OnSelect)(void* widget, bool wasSelected);
	/* Called when mouse clicks on another widget. */
	void (*OnUnselect)(void* widget);
	/* Called when mouse wheel is scrolled and this widget is selected. */
	void (*MouseWheel)(void* widget, float delta);
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
	Rect2D Last;                  /* Widget's last drawn area */

/* Represents an individual 2D gui component in the launcher. */
struct LWidget { LWidget_Layout };
void LWidget_SetLocation(void* widget, uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset);
void LWidget_CalcPosition(void* widget);
void LWidget_Draw(void* widget);
void LWidget_Redraw(void* widget);

struct LButton {
	LWidget_Layout
	String Text;
	Size2D _TextSize;
};
CC_NOINLINE void LButton_Init(struct LButton* w, int width, int height);
CC_NOINLINE void LButton_SetConst(struct LButton* w, const char* text);

struct LInput;
struct LInput {
	LWidget_Layout
	int MinWidth;
	/* Text displayed when the user has not entered anything in the text field. */
	const char* HintText;
	/* Whether all characters should be rendered as *. */
	bool Password;
	/* Filter applied to text received from the clipboard. Can be NULL. */
	void (*ClipboardFilter)(String* str);
	/* Callback invoked when the text is changed. Can be NULL. */
	void (*TextChanged)(struct LInput* w);
	/* Specifies the position that characters are inserted/deleted from. */
	/* NOTE: -1 to insert/delete characters at end of the text. */
	int CaretPos;
	String Text;
	int _TextHeight;
	char _TextBuffer[STRING_SIZE];
};
CC_NOINLINE void LInput_Init(struct LInput* w, int width, int height, const char* hintText);
CC_NOINLINE void LInput_SetText(struct LInput* w, const String* text);

/* Appends a character to the currently entered text. */
CC_NOINLINE bool LInput_Append(struct LInput* w, char c);
/* Appends a string to the currently entered text. */
CC_NOINLINE bool LInput_AppendString(struct LInput* w, const String* str);
/* Removes the character preceding the caret in the currently entered text. */
CC_NOINLINE bool LInput_Backspace(struct LInput* w);
/* Removes the character at the caret in the currently entered text. */
CC_NOINLINE bool LInput_Delete(struct LInput* w);
/* Resets the currently entered text to an empty string. */
CC_NOINLINE bool LInput_Clear(struct LInput* w);

/* Represents non-interactable text. */
struct LLabel {
	LWidget_Layout
	FontDesc* Font;
	String Text;
	Size2D _TextSize;
	char _TextBuffer[STRING_SIZE];
};
CC_NOINLINE void LLabel_Init(struct LLabel* w);
CC_NOINLINE void LLabel_SetText(struct LLabel* w, const String* text);
CC_NOINLINE void LLabel_SetConst(struct LLabel* w, const char* text);

/* Represents a coloured rectangle. Usually used as a line separator. */
struct LBox {
	LWidget_Layout
	BitmapCol Col;
};
CC_NOINLINE void LBox_Init(struct LBox* w, int width, int height);

/* Represents a slider bar that may or may not be modifiable by the user. */
struct LSlider {
	LWidget_Layout
	int Value, MaxValue;
	BitmapCol ProgressCol;
};
CC_NOINLINE void LSlider_Init(struct LSlider* w, int width, int height);

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
	bool columnGridline;
	/* Whether user can interact with this column. */
	/* Non-interactable columns can't be sorted/resized. */
	bool interactable;
	/* Whether to invert the order of row sorting. */
	bool invertSort;
};

/* Represents a table of server entries. */
struct LTable {
	LWidget_Layout
	/* Columns of the table. */
	struct LTableColumn* Columns;
	/* Number of columns in the table. */
	int NumColumns;
	/* Fonts for text in rows. */
	FontDesc* RowFont;
	/* Y start and end of rows and height of each row. */
	int RowsBegY, RowsEndY, RowHeight;
	/* Y height of headers. */
	int HdrHeight;
	/* Maximum number of rows visible. */
	int VisibleRows;
	/* Total number of rows in the table (after filter is applied). */
	int RowsCount;
	/* Index of top row currently visible. */
	int TopRow;

	/* Hash of the currently selected server. */
	String* SelectedHash;
	/* Filter for which server names to show. */
	String* Filter;
	/* Callback when selected has has changed. */
	void (*OnSelectedChanged)(void);

	/* Index of table column currently being dragged. */
	int DraggingColumn;
	int GridlineWidth, GridlineHeight;

	bool DraggingScrollbar;	/* Is scrollbar is currently being dragged */
	int MouseOffset;        /* Offset of mouse for scrollbar dragging */
	int ScrollbarWidth;     /* How wide scrollbar is in pixels */

	float _wheelAcc; /* mouse wheel accumulator */
	int _lastRow; /* last clicked row (for doubleclick join) */
	TimeMS _lastClick;   /* time of last mouse click on a row */
};
/* Gets the current ith row */
#define LTable_Get(row) (&FetchServersTask.Servers[FetchServersTask.Servers[row]._order])

/* Initialises a table. */
/* NOTE: Must also call LTable_Reset to make a table actually useful. */
void LTable_Init(struct LTable* table, FontDesc* rowFont);
/* Resets state of a table (reset sorter, filter, etc) */
void LTable_Reset(struct LTable* table);
/* Adjusts Y position of rows and number of visible rows. */
void LTable_Reposition(struct LTable* table);
/* Whether this table would handle the given key being pressed. */
/* e.g. used so pressing up/down works even when another widget is selected */
bool LTable_HandlesKey(Key key);
/* Filters rows to only show those containing 'w->Filter' in the name. */
void LTable_ApplyFilter(struct LTable* table);
/* Sorts the rows in the table by current Sorter function of table */
void LTable_Sort(struct LTable* table);
/* If selected row is not visible, adjusts top row so it does show. */
void LTable_ShowSelected(struct LTable* table);
#endif
