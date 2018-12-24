#ifndef CC_LWIDGETS_H
#define CC_LWIDGETS_H
#include "Bitmap.h"
#include "Constants.h"
#include "Input.h"
/* Describes and manages individual 2D GUI elements in the launcher.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct LWidgetVTABLE {
	/* Called to draw contents of this widget */
	void (*Draw)(void* widget);
	/* Called repeatedly to update this widget when selected. */
	void (*Tick)(void* widget);
	/* Called when key is pressed and this widget is selected. */
	void (*OnKeyDown)(void* widget, Key key);
	/* Called when key is pressed and this widget is selected. */
	void (*OnKeyPress)(void* widget, char c);
	/* Called when mouse hovers over this widget. */
	void (*OnHover)(void* widget);
	/* Called when mouse moves away from this widget. */
	void (*OnUnhover)(void* widget);
	/* Called when mouse clicks on this widget. */
	/* NOTE: This function is just for general widget behaviour. */
	/* OnClick callback is for per-widget instance behaviour. */
	void (*OnSelect)(void* widget, bool wasSelected);
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
	FontDesc Font;
	Size2D _TextSize;
	char _TextBuffer[STRING_SIZE];
};
CC_NOINLINE void LButton_Init(struct LButton* w, const FontDesc* font, int width, int height);
CC_NOINLINE void LButton_SetText(struct LButton* w, const String* text);

struct LInput;
struct LInput {
	LWidget_Layout
	int BaseWidth, _RealWidth;
	/* Text displayed when the user has not entered anything in the text field. */
	const char* HintText;
	/* Whether all characters should be rendered as *. */
	bool Password;
	/* Filter applied to text received from the clipboard. Can be NULL. */
	void (*ClipboardFilter)(String* str);
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
CC_NOINLINE void LInput_Init(struct LInput* w, const FontDesc* font, int width, int height, const char* hintText, const FontDesc* hintFont);
CC_NOINLINE void LInput_SetText(struct LInput* w, const String* text);

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
CC_NOINLINE void LLabel_Init(struct LLabel* w, const FontDesc* font);
CC_NOINLINE void LLabel_SetText(struct LLabel* w, const String* text);

/* Represents a slider bar that may or may not be modifiable by the user. */
struct LSlider {
	LWidget_Layout
	int Value, MaxValue;
	BitmapCol ProgressCol;
};
CC_NOINLINE void LSlider_Init(struct LSlider* w, int width, int height);

struct ServerInfo;
struct DrawTextArgs;
/* Returns sort order of two rows/server entries. */
typedef int (*LTableSorter)(struct ServerInfo* a, struct ServerInfo* b);

struct LTableColumn {
	/* Name of this column. */
	const char* Name;
	/* Width of this column in pixels. */
	int Width;
	/* Draws the value of this column for the given row. */
	/* If args.Text is changed to something, that text gets drawn afterwards. */
	/* Most of the time that's all you need to do. */
	void (*DrawRow)(struct ServerInfo* row, struct DrawTextArgs* args, int x, int y);
	/* Sorts two rows based on value of this column in both rows. */
	LTableSorter SortRows;
	/* Whether a vertical gridline (and padding) appears after this. */
	bool ColumnGridline;
	/* Whether user can interact with this column. */
	/* Non-interactable columns can't be sorted/resized. */
	bool Interactable;
	/* Whether to invert the order of row sorting. */
	bool InvertSort;
};

/* Represents a table of server entries. */
struct LTable {
	LWidget_Layout
	/* Columns of the table. */
	struct LTableColumn* Columns;
	/* Number of columns in the table. */
	int NumColumns;
	/* Fonts for text in header and rows. */
	FontDesc RowFont, HdrFont;
	/* Y start and end of rows and height of each row. */
	int RowsBegY, RowsEndY, RowHeight;
	/* Y height of headers. */
	int HdrHeight;
	/* Maximum number of rows visible. */
	int VisibleRows;
	/* Total number of rows in the table (after filter is applied). */
	int RowsCount;
	/* Comparison function used to sort rows. */
	LTableSorter Sorter;

	/* Hash of the currently selected server. */
	String* SelectedHash;
	/* Filter for which server names to show. */
	String* Filter;

	/* Index of column currently being dragged. */
	int DraggingColumn;
	/* Whether scrollbar is currently being dragged up or down. */
	bool DraggingScrollbar;
	int MouseOffset;
};
/* Initialises a table. */
/* NOTE: Must also call LTable_Reset to make a table actually useful. */
void LTable_Init(struct LTable* table, const FontDesc* hdrFont, const FontDesc* rowFont);
/* Resets state of a table (reset sorter, filter, etc) */
void LTable_Reset(struct LTable* table);
/* Adjusts Y position of rows and number of visible rows. */
void LTable_Reposition(struct LTable* table);
/* Filters rows to only show those containing 'w->Filter' in the name. */
void LTable_ApplyFilter(struct LTable* table);
/* Calculates the sorted order of rows in the table. */
/* NOTE: You must use ApplyFilter to actually update visible row order. */
void LTable_CalcSortOrder(struct LTable* table);
/* Attempts to select the row whose hash equals the given hash. Scrolls table if needed. */
void LTable_SetSelected(struct LTable* table, const String* hash);
#endif
