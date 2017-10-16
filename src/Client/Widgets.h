#ifndef CS_WIDGETS_H
#define CS_WIDGETS_H
#include "Gui.h"
#include "Texture.h"
#include "PackedCol.h"
#include "String.h"
#include "BlockID.h"
#include "Constants.h"

void Widget_SetLocation(Widget* widget, Anchor horAnchor, Anchor verAnchor, Int32 xOffset, Int32 yOffset);

typedef struct TextWidget_ {
	Widget Base;
	Texture Texture;
	Int32 DefaultHeight;
	void* Font;

	bool ReducePadding;
	PackedCol Col;
} TextWidget;

void TextWidget_Create(TextWidget* widget, STRING_TRANSIENT String* text, void* font);
void TextWidget_SetText(TextWidget* widget, STRING_TRANSIENT String* text);


typedef void (*ButtonWidget_SetValue)(STRING_TRANSIENT String* raw);
typedef void (*ButtonWidget_GetValue)(STRING_TRANSIENT String* raw);
typedef struct ButtonWidget_ {
	Widget Base;
	Texture Texture;
	Int32 DefaultHeight;
	void* Font;

	String OptName;
	ButtonWidget_GetValue GetValue;
	ButtonWidget_SetValue SetValue;
	Int32 MinWidth, MinHeight, Metadata;
} ButtonWidget;

void ButtonWidget_Create(ButtonWidget* widget, STRING_TRANSIENT String* text, Int32 minWidth, void* font, Gui_MouseHandler onClick);
void ButtonWidget_SetText(ButtonWidget* widget, STRING_TRANSIENT String* text);


typedef struct ScrollbarWidget_ {
	Widget Base;
	Int32 TotalRows, ScrollY;
	Real32 ScrollingAcc;
	bool DraggingMouse;
	Int32 MouseOffset;
} ScrollbarWidget;

void ScrollbarWidget_Create(ScrollbarWidget* widget);
void ScrollbarWidget_ClampScrollY(ScrollbarWidget* widget);


typedef struct HotbarWidget_ {
	Widget Base;
	Texture SelTex, BackTex;
	Real32 BarHeight, SelBlockSize, ElemSize;
	Real32 BarXOffset, BorderSize;
	bool AltHandled;
} HotbarWidget;

void HotbarWidget_Create(HotbarWidget* widget);


typedef struct TableWidget_ {
	Widget Base;
	Int32 ElementsCount, ElementsPerRow, RowsCount;
	Int32 LastCreatedIndex;
	void* Font;
	bool PendingClose;
	Int32 SelectedIndex, BlockSize;
	Real32 SelBlockExpand;
	GfxResourceID VB;

	BlockID Elements[BLOCK_COUNT];
	ScrollbarWidget Scroll;
	Texture DescTex;
} TableWidget;

void TableWidget_Create(TableWidget* widget);
void TableWidget_SetBlockTo(TableWidget* widget, BlockID block);
void TableWidget_OnInventoryChanged(TableWidget* widget);


typedef void (*SpecialInputAppendFunc)(UInt8 c);
typedef struct SpecialInputTab_ {
	String Title;
	Size2D TitleSize;
	String Contents;
	Int32 ItemsPerRow;
	Int32 CharsPerItem;
} SpecialInputTab;
void SpecialInputTab_Init(String title, Int32 itemsPerRow, Int32 charsPerItem, String contents);

typedef struct SpecialInputWidget_ {
	Widget Base;
	Texture texture;
	void* Font;
	SpecialInputAppendFunc AppendFunc;
	Size2D ElementSize;
	SpecialInputTab Tabs[5];
} SpecialInputWidget;

void SpecialInputWidget_Create(SpecialInputWidget* widget, void* font, SpecialInputAppendFunc appendFunc);
void SpecialInputWidget_UpdateColours(SpecialInputWidget* widget);
void SpecialInputWidget_SetActive(SpecialInputWidget* widget, bool active);
void SpecialInputWidget_Redraw(SpecialInputWidget* widget);


#define INPUTWIDGET_MAX_LINES 4
struct InputWidget_;
/* Remakes the raw texture containing all the chat lines. Also updates the dimensions of the widget. */
typedef struct InputWidget_ {
	Widget Base;
	void* Font;	
	Int32 MaxLines;
	Int32 MaxCharsPerLine;
	Int32 Padding;
	Gui_Void RemakeTexture;  /* Remakes the raw texture containing all the chat lines. Also updates dimensions. */
	Gui_Void OnPressedEnter; /* Invoked when the user presses enter. */

	String Text;
	String Lines[INPUTWIDGET_MAX_LINES];     /* raw text of each line */
	Size2D LineSizes[INPUTWIDGET_MAX_LINES]; /* size of each line in pixels */
	Texture InputTex;
	String Prefix;
	Int32 PrefixWidth, PrefixHeight;
	Texture PrefixTex;

	Int32 CaretX, CaretY;          /* Coordinates of caret in lines */
	Int32 CaretWidth, CaretHeight;	
	Int32 CaretPos;                /* Position of caret, -1 for at end of string. */
	bool ShowCaret;
	PackedCol CaretCol;
	Texture CaretTex;
	Real64 CaretAccumulator;
} InputWidget;

void InputWidget_Create(InputWidget* widget, void* font);
/* Calculates the sizes of each line in the text buffer. */
void InputWidget_CalculateLineSizes(InputWidget* widget);
/* Calculates the location and size of the caret character. */
void InputWidget_UpdateCaret(InputWidget* widget);
/* Clears all the characters from the text buffer. Deletes the native texture. */
void InputWidget_Clear(InputWidget* widget);
/* Appends a sequence of characters to current text buffer. May recreate the native texture. */
void InputWidget_Append(InputWidget* widget, STRING_TRANSIENT String* text);
/* Appends a single character to current text buffer. May recreate the native texture. */
void InputWidget_Append(InputWidget* widget, UInt8 c);


/* "part1" "> part2" type urls */
#define LINK_FLAG_CONTINUE 2
/* used for internally combining "part1" and "part2" */
#define LINK_FLAG_APPEND 4
/* used to signify that part2 is a separate url from part1 */
#define LINK_FLAG_NEWLINK 8
/* min size of link is 'http:// ' */
#define LINK_MAX_PER_LINE (STRING_SIZE / 8)
typedef struct LinkData_ {
	Rectangle2D Bounds[LINK_MAX_PER_LINE];
	String Parts[LINK_MAX_PER_LINE];
	String Urls[LINK_MAX_PER_LINE];
	UInt8 LinkFlags, LinksCount;
} LinkData;

#define TEXTGROUPWIDGET_MAX_LINES 30
typedef struct TextGroupWidget_ {
	Widget Base;
	Int32 LinesCount, DefaultHeight;
	void* Font;
	void* UnderlineFont;	
	bool PlaceholderHeight[TEXTGROUPWIDGET_MAX_LINES];
	String Lines[TEXTGROUPWIDGET_MAX_LINES];
	LinkData LinkDatas[TEXTGROUPWIDGET_MAX_LINES];
	Texture Textures[TEXTGROUPWIDGET_MAX_LINES];
} TextGroupWidget;

void TextGroupWidget_Create(TextGroupWidget* widget, Int32 linesCount, void* font, void* underlineFont);
void TextGroupWidget_SetUsePlaceHolder(TextGroupWidget* widget, Int32 index, bool placeHolder);
void TextGroupWidget_PushUpAndReplaceLast(TextGroupWidget* widget, STRING_TRANSIENT String* text);
Int32 TextGroupWidget_GetUsedHeight();
void TextGroupWidget_GetSelected(TextGroupWidget* widget, String* dstText, Int32 mouseX, Int32 mouseY);
void TextGroupWidget_SetText(TextGroupWidget* widget, int index, String* text);
#endif