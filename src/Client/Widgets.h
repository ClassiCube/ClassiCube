#ifndef CC_WIDGETS_H
#define CC_WIDGETS_H
#include "Gui.h"
#include "Texture.h"
#include "PackedCol.h"
#include "String.h"
#include "BlockID.h"
#include "Constants.h"
#include "Entity.h"
/* Contains all 2D widget implementations.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

void Widget_SetLocation(Widget* widget, Anchor horAnchor, Anchor verAnchor, Int32 xOffset, Int32 yOffset);

typedef struct TextWidget_ {
	Widget Base;
	Texture Texture;
	Int32 DefaultHeight;
	FontDesc Font;

	bool ReducePadding;
	PackedCol Col;
} TextWidget;

void TextWidget_Make(TextWidget* widget, FontDesc* font);
void TextWidget_Create(TextWidget* widget, STRING_PURE String* text, FontDesc* font);
void TextWidget_SetText(TextWidget* widget, STRING_PURE String* text);


typedef void (*ButtonWidget_SetValue)(STRING_TRANSIENT String* raw);
typedef void (*ButtonWidget_GetValue)(STRING_TRANSIENT String* raw);
typedef bool (*Gui_MouseHandler)(GuiElement* elem, Int32 x, Int32 y, MouseButton btn);
typedef struct ButtonWidget_ {
	Widget Base;
	Texture Texture;
	Int32 DefaultHeight;
	FontDesc Font;

	String OptName;
	ButtonWidget_GetValue GetValue;
	ButtonWidget_SetValue SetValue;
	Int32 MinWidth, MinHeight, Metadata;
} ButtonWidget;

void ButtonWidget_Create(ButtonWidget* widget, STRING_PURE String* text, Int32 minWidth, FontDesc* font, Gui_MouseHandler onClick);
void ButtonWidget_SetText(ButtonWidget* widget, STRING_PURE String* text);


typedef struct ScrollbarWidget_ {
	Widget Base;
	Int32 TotalRows, ScrollY;
	Real32 ScrollingAcc;
	Int32 MouseOffset;
	bool DraggingMouse;
} ScrollbarWidget;

void ScrollbarWidget_Create(ScrollbarWidget* widget);
void ScrollbarWidget_ClampScrollY(ScrollbarWidget* widget);


typedef struct HotbarWidget_ {
	Widget Base;
	Texture SelTex, BackTex;
	Real32 BarHeight, SelBlockSize, ElemSize;
	Real32 BarXOffset, BorderSize;
	Real32 ScrollAcc;
	bool AltHandled;
} HotbarWidget;

void HotbarWidget_Create(HotbarWidget* widget);


typedef struct TableWidget_ {
	Widget Base;
	Int32 ElementsCount, ElementsPerRow, RowsCount;
	Int32 LastCreatedIndex;
	FontDesc Font;
	Int32 SelectedIndex, BlockSize;
	Real32 SelBlockExpand;
	GfxResourceID VB;
	bool PendingClose;

	BlockID Elements[BLOCK_COUNT];
	ScrollbarWidget Scroll;
	Texture DescTex;
} TableWidget;

void TableWidget_Create(TableWidget* widget);
void TableWidget_SetBlockTo(TableWidget* widget, BlockID block);
void TableWidget_OnInventoryChanged(TableWidget* widget);


typedef void (*SpecialInputAppendFunc)(void* userData, UInt8 c);
typedef struct SpecialInputTab_ {
	String Title;
	Size2D TitleSize;
	String Contents;
	Int32 ItemsPerRow;
	Int32 CharsPerItem;
} SpecialInputTab;
void SpecialInputTab_Init(SpecialInputTab* tab, STRING_REF String* title, 
	Int32 itemsPerRow, Int32 charsPerItem, STRING_REF String* contents);

typedef struct SpecialInputWidget_ {
	Widget Base;
	Size2D ElementSize;
	Int32 SelectedIndex;
	SpecialInputAppendFunc AppendFunc;
	void* AppendObj;
	Texture Tex;
	FontDesc Font;
	SpecialInputTab Tabs[5];
	String ColString;
	UInt8 ColBuffer[String_BufferSize(DRAWER2D_MAX_COLS * 4)];
} SpecialInputWidget;

void SpecialInputWidget_Create(SpecialInputWidget* widget, FontDesc* font, SpecialInputAppendFunc appendFunc, void* appendObj);
void SpecialInputWidget_UpdateCols(SpecialInputWidget* widget);
void SpecialInputWidget_SetActive(SpecialInputWidget* widget, bool active);


#define INPUTWIDGET_MAX_LINES 3
struct InputWidget_;
typedef struct InputWidget_ {
	Widget Base;
	FontDesc Font;	
	Int32 (*GetMaxLines)(void);
	Int32 Padding, MaxCharsPerLine;
	void (*RemakeTexture)(GuiElement* elem);  /* Remakes the raw texture containing all the chat lines. Also updates dimensions. */
	void (*OnPressedEnter)(GuiElement* elem); /* Invoked when the user presses enter. */
	bool (*AllowedChar)(GuiElement* elem, UInt8 c);

	String Text;
	String Lines[INPUTWIDGET_MAX_LINES];     /* raw text of each line */
	Size2D LineSizes[INPUTWIDGET_MAX_LINES]; /* size of each line in pixels */
	Texture InputTex;
	String Prefix;
	UInt16 PrefixWidth, PrefixHeight;
	Texture PrefixTex;

	Int32 CaretX, CaretY;          /* Coordinates of caret in lines */
	UInt16 CaretWidth, CaretHeight;	
	Int32 CaretPos;                /* Position of caret, -1 for at end of string. */
	bool ShowCaret;
	PackedCol CaretCol;
	Texture CaretTex;
	Real64 CaretAccumulator;
} InputWidget;

void InputWidget_Create(InputWidget* widget, FontDesc* font, STRING_REF String* prefix);
/* Calculates the sizes of each line in the text buffer. */
void InputWidget_CalculateLineSizes(InputWidget* widget);
/* Calculates the location and size of the caret character. */
void InputWidget_UpdateCaret(InputWidget* widget);
/* Clears all the characters from the text buffer. Deletes the native texture. */
void InputWidget_Clear(InputWidget* widget);
/* Appends a sequence of characters to current text buffer. May recreate the native texture. */
void InputWidget_AppendString(InputWidget* widget, STRING_PURE String* text);
/* Appends a single character to current text buffer. May recreate the native texture. */
void InputWidget_Append(InputWidget* widget, UInt8 c);


typedef struct MenuInputValidator_ {
	void (*GetRange)(struct MenuInputValidator_* validator, STRING_TRANSIENT String* range);
	bool (*IsValidChar)(struct MenuInputValidator_* validator, UInt8 c);
	bool (*IsValidString)(struct MenuInputValidator_* validator, STRING_PURE String* s);
	bool (*IsValidValue)(struct MenuInputValidator_* validator, STRING_PURE String* s);

	union {
		void* Meta_Ptr[2];
		Int32 Meta_Int[2];
		Real32 Meta_Real[2];
	};
} MenuInputValidator;

MenuInputValidator MenuInputValidator_Hex(void);
MenuInputValidator MenuInputValidator_Integer(Int32 min, Int32 max);
MenuInputValidator MenuInputValidator_Seed(void);
MenuInputValidator MenuInputValidator_Real(Real32 min, Real32 max);
MenuInputValidator MenuInputValidator_Path(void);
MenuInputValidator MenuInputValidator_Boolean(void);
MenuInputValidator MenuInputValidator_Enum(const UInt8** names, UInt32 namesCount);
MenuInputValidator MenuInputValidator_String(void);

typedef struct MenuInputWidget_ {
	InputWidget Base;
	Int32 MinWidth, MinHeight;
	MenuInputValidator Validator;
	UInt8 TextBuffer[String_BufferSize(STRING_SIZE)];
} MenuInputWidget;

void MenuInputWidget_Create(MenuInputWidget* widget, Int32 width, Int32 height, STRING_PURE String* text, FontDesc* font, MenuInputValidator* validator);


typedef struct ChatInputWidget_ {
	InputWidget Base;
	Int32 TypingLogPos;
	UInt8 TextBuffer[String_BufferSize(INPUTWIDGET_MAX_LINES * STRING_SIZE)];
	UInt8 OrigBuffer[String_BufferSize(INPUTWIDGET_MAX_LINES * STRING_SIZE)];
} ChatInputWidget;

void ChatInputWidget_Create(ChatInputWidget* widget, FontDesc* font);


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
	FontDesc Font, UnderlineFont;	
	bool PlaceholderHeight[TEXTGROUPWIDGET_MAX_LINES];
	String Lines[TEXTGROUPWIDGET_MAX_LINES];
	LinkData LinkDatas[TEXTGROUPWIDGET_MAX_LINES];
	Texture Textures[TEXTGROUPWIDGET_MAX_LINES];
} TextGroupWidget;

void TextGroupWidget_Create(TextGroupWidget* widget, Int32 linesCount, FontDesc* font, FontDesc* underlineFont);
void TextGroupWidget_SetUsePlaceHolder(TextGroupWidget* widget, Int32 index, bool placeHolder);
void TextGroupWidget_PushUpAndReplaceLast(TextGroupWidget* widget, STRING_PURE String* text);
Int32 TextGroupWidget_GetUsedHeight(TextGroupWidget* widget);
void TextGroupWidget_GetSelected(TextGroupWidget* widget, STRING_TRANSIENT String* text, Int32 mouseX, Int32 mouseY);
void TextGroupWidget_SetText(TextGroupWidget* widget, Int32 index, STRING_PURE String* text);


typedef struct PlayerListWidget_ {
	Widget Base;
	FontDesc Font;
	UInt16 NamesCount, ElementOffset;
	Int32 XMin, XMax, YHeight;
	bool Classic;
	TextWidget Overview;
	UInt16 IDs[TABLIST_MAX_NAMES * 2];
	Texture Textures[TABLIST_MAX_NAMES * 2];
} PlayerListWidget;

void PlayerListWidget_Create(PlayerListWidget* widget, FontDesc* font, bool classic);
void PlayerListWidget_GetNameUnder(PlayerListWidget* widget, Int32 mouseX, Int32 mouseY, STRING_TRANSIENT String* name);
void PlayerListWidget_RecalcYOffset(PlayerListWidget* widget);
#endif