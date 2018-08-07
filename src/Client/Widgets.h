#ifndef CC_WIDGETS_H
#define CC_WIDGETS_H
#include "Gui.h"
#include "BlockID.h"
#include "Constants.h"
#include "Entity.h"
#include "2DStructs.h"
/* Contains all 2D widget implementations.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

void Widget_SetLocation(struct Widget* widget, UInt8 horAnchor, UInt8 verAnchor, Int32 xOffset, Int32 yOffset);

struct TextWidget {
	Widget_Layout
	struct Texture Texture;
	Int32 DefaultHeight;
	struct FontDesc Font;

	bool ReducePadding;
	PackedCol Col;
};
void TextWidget_Make(struct TextWidget* widget, struct FontDesc* font);
void TextWidget_Create(struct TextWidget* widget, STRING_PURE String* text, struct FontDesc* font);
void TextWidget_SetText(struct TextWidget* widget, STRING_PURE String* text);


typedef void (*ButtonWidget_Get)(STRING_TRANSIENT String* raw);
typedef void (*ButtonWidget_Set)(STRING_PURE String* raw);
struct ButtonWidget {
	Widget_Layout
	struct Texture Texture;
	UInt16 DefaultHeight, MinWidth;
	struct FontDesc Font;

	const UChar* OptName;
	ButtonWidget_Get GetValue;
	ButtonWidget_Set SetValue;
};
void ButtonWidget_Create(struct ButtonWidget* widget, Int32 minWidth, STRING_PURE String* text, struct FontDesc* font, Widget_LeftClick onClick);
void ButtonWidget_SetText(struct ButtonWidget* widget, STRING_PURE String* text);


struct ScrollbarWidget {
	Widget_Layout
	Int32 TotalRows, ScrollY;
	Real32 ScrollingAcc;
	Int32 MouseOffset;
	bool DraggingMouse;
};
void ScrollbarWidget_Create(struct ScrollbarWidget* widget);


struct HotbarWidget {
	Widget_Layout
	struct Texture SelTex, BackTex;
	Real32 BarHeight, SelBlockSize, ElemSize;
	Real32 BarXOffset, BorderSize;
	Real32 ScrollAcc;
	bool AltHandled;
};
void HotbarWidget_Create(struct HotbarWidget* widget);


struct TableWidget {
	Widget_Layout
	Int32 ElementsCount, ElementsPerRow, RowsCount;
	Int32 LastCreatedIndex;
	struct FontDesc Font;
	Int32 SelectedIndex, BlockSize;
	Real32 SelBlockExpand;
	GfxResourceID VB;
	bool PendingClose;

	BlockID Elements[BLOCK_COUNT];
	struct ScrollbarWidget Scroll;
	struct Texture DescTex;
	Int32 LastX, LastY;
};

void TableWidget_Create(struct TableWidget* widget);
void TableWidget_SetBlockTo(struct TableWidget* widget, BlockID block);
void TableWidget_OnInventoryChanged(struct TableWidget* widget);
void TableWidget_MakeDescTex(struct TableWidget* widget, BlockID block);


#define INPUTWIDGET_MAX_LINES 3
#define INPUTWIDGET_LEN STRING_SIZE
struct InputWidget {
	Widget_Layout
	struct FontDesc Font;		
	Int32 (*GetMaxLines)(void);
	void (*RemakeTexture)(struct GuiElem* elem);  /* Remakes the raw texture containing all the chat lines. Also updates dimensions. */
	void (*OnPressedEnter)(struct GuiElem* elem); /* Invoked when the user presses enter. */
	bool (*AllowedChar)(struct GuiElem* elem, UChar c);

	String Text;
	String Lines[INPUTWIDGET_MAX_LINES];            /* raw text of each line */
	struct Size2D LineSizes[INPUTWIDGET_MAX_LINES]; /* size of each line in pixels */
	struct Texture InputTex;
	String Prefix;
	UInt16 PrefixWidth, PrefixHeight;
	bool ConvertPercents;

	UInt8 Padding;
	bool ShowCaret;
	UInt16 CaretWidth;
	Int32 CaretX, CaretY;          /* Coordinates of caret in lines */
	Int32 CaretPos;                /* Position of caret, -1 for at end of string. */
	PackedCol CaretCol;
	struct Texture CaretTex;
	Real64 CaretAccumulator;
};

void InputWidget_Create(struct InputWidget* widget, struct FontDesc* font, STRING_REF String* prefix);
/* Clears all the characters from the text buffer. Deletes the native texture. */
void InputWidget_Clear(struct InputWidget* widget);
/* Appends a sequence of characters to current text buffer. May recreate the native texture. */
void InputWidget_AppendString(struct InputWidget* widget, STRING_PURE String* text);
/* Appends a single character to current text buffer. May recreate the native texture. */
void InputWidget_Append(struct InputWidget* widget, UChar c);


struct MenuInputValidator;
struct MenuInputValidator {
	void (*GetRange)(struct MenuInputValidator* validator, STRING_TRANSIENT String* range);
	bool (*IsValidChar)(struct MenuInputValidator* validator, UChar c);
	bool (*IsValidString)(struct MenuInputValidator* validator, STRING_PURE String* s);
	bool (*IsValidValue)(struct MenuInputValidator* validator, STRING_PURE String* s);

	union {
		void* Meta_Ptr[2];
		Int32 Meta_Int[2];
		Real32 Meta_Real[2];
	};
};

struct MenuInputValidator MenuInputValidator_Hex(void);
struct MenuInputValidator MenuInputValidator_Integer(Int32 min, Int32 max);
struct MenuInputValidator MenuInputValidator_Seed(void);
struct MenuInputValidator MenuInputValidator_Real(Real32 min, Real32 max);
struct MenuInputValidator MenuInputValidator_Path(void);
struct MenuInputValidator MenuInputValidator_Enum(const UChar** names, UInt32 namesCount);
struct MenuInputValidator MenuInputValidator_String(void);

struct MenuInputWidget {
	struct InputWidget Base;
	Int32 MinWidth, MinHeight;
	struct MenuInputValidator Validator;
	UChar TextBuffer[String_BufferSize(INPUTWIDGET_LEN)];
};
void MenuInputWidget_Create(struct MenuInputWidget* widget, Int32 width, Int32 height, STRING_PURE String* text, struct FontDesc* font, struct MenuInputValidator* validator);


struct ChatInputWidget {
	struct InputWidget Base;
	Int32 TypingLogPos;
	UChar TextBuffer[String_BufferSize(INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN)];
	UChar OrigBuffer[String_BufferSize(INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN)];
};

void ChatInputWidget_Create(struct ChatInputWidget* widget, struct FontDesc* font);


#define TEXTGROUPWIDGET_MAX_LINES 30
#define TEXTGROUPWIDGET_LEN (STRING_SIZE + (STRING_SIZE / 2))
struct TextGroupWidget {
	Widget_Layout
	Int32 LinesCount, DefaultHeight;
	struct FontDesc Font, UnderlineFont;
	bool PlaceholderHeight[TEXTGROUPWIDGET_MAX_LINES];
	UInt8 LineLengths[TEXTGROUPWIDGET_MAX_LINES];
	struct Texture* Textures;
	UChar* Buffer;
};

void TextGroupWidget_Create(struct TextGroupWidget* widget, Int32 linesCount, struct FontDesc* font, struct FontDesc* underlineFont, STRING_REF struct Texture* textures, STRING_REF UChar* buffer);
void TextGroupWidget_SetUsePlaceHolder(struct TextGroupWidget* widget, Int32 index, bool placeHolder);
void TextGroupWidget_PushUpAndReplaceLast(struct TextGroupWidget* widget, STRING_PURE String* text);
Int32 TextGroupWidget_UsedHeight(struct TextGroupWidget* widget);
void TextGroupWidget_GetSelected(struct TextGroupWidget* widget, STRING_TRANSIENT String* text, Int32 mouseX, Int32 mouseY);
void TextGroupWidget_GetText(struct TextGroupWidget* widget, Int32 index, STRING_TRANSIENT String* text);
void TextGroupWidget_SetText(struct TextGroupWidget* widget, Int32 index, STRING_PURE String* text);


struct PlayerListWidget {
	Widget_Layout
	struct FontDesc Font;
	UInt16 NamesCount, ElementOffset;
	Int32 XMin, XMax, YHeight;
	bool Classic;
	struct TextWidget Overview;
	UInt16 IDs[TABLIST_MAX_NAMES * 2];
	struct Texture Textures[TABLIST_MAX_NAMES * 2];
};
void PlayerListWidget_Create(struct PlayerListWidget* widget, struct FontDesc* font, bool classic);
void PlayerListWidget_GetNameUnder(struct PlayerListWidget* widget, Int32 mouseX, Int32 mouseY, STRING_TRANSIENT String* name);


typedef void (*SpecialInputAppendFunc)(void* userData, UChar c);
struct SpecialInputTab {
	Int32 ItemsPerRow, CharsPerItem;
	struct Size2D TitleSize;
	String Title, Contents;	
};

struct SpecialInputWidget {
	Widget_Layout
	struct Size2D ElementSize;
	Int32 SelectedIndex;
	struct InputWidget* AppendObj;
	struct Texture Tex;
	struct FontDesc Font;
	struct SpecialInputTab Tabs[5];
	String ColString;
	UChar ColBuffer[String_BufferSize(DRAWER2D_MAX_COLS * 4)];
};

void SpecialInputWidget_Create(struct SpecialInputWidget* widget, struct FontDesc* font, struct InputWidget* appendObj);
void SpecialInputWidget_UpdateCols(struct SpecialInputWidget* widget);
void SpecialInputWidget_SetActive(struct SpecialInputWidget* widget, bool active);
#endif
