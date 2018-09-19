#ifndef CC_WIDGETS_H
#define CC_WIDGETS_H
#include "Gui.h"
#include "BlockID.h"
#include "Constants.h"
#include "Entity.h"
/* Contains all 2D widget implementations.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct TextWidget {
	Widget_Layout
	struct Texture Texture;

	bool ReducePadding;
	PackedCol Col;
};
NOINLINE_ void TextWidget_Make(struct TextWidget* w);
NOINLINE_ void TextWidget_Create(struct TextWidget* w, STRING_PURE String* text, FontDesc* font);
NOINLINE_ void TextWidget_Set(struct TextWidget* w, STRING_PURE String* text, FontDesc* font);


typedef void (*Button_Get)(STRING_TRANSIENT String* raw);
typedef void (*Button_Set)(STRING_PURE String* raw);
struct ButtonWidget {
	Widget_Layout
	struct Texture Texture;
	UInt16 MinWidth;

	const char* OptName;
	Button_Get GetValue;
	Button_Set SetValue;
};
NOINLINE_ void ButtonWidget_Create(struct ButtonWidget* w, Int32 minWidth, STRING_PURE String* text, FontDesc* font, Widget_LeftClick onClick);
NOINLINE_ void ButtonWidget_Set(struct ButtonWidget* w, STRING_PURE String* text, FontDesc* font);


struct ScrollbarWidget {
	Widget_Layout
	Int32 TotalRows, ScrollY;
	Real32 ScrollingAcc;
	Int32 MouseOffset;
	bool DraggingMouse;
};
NOINLINE_ void ScrollbarWidget_Create(struct ScrollbarWidget* w);


struct HotbarWidget {
	Widget_Layout
	struct Texture SelTex, BackTex;
	Real32 BarHeight, SelBlockSize, ElemSize;
	Real32 BarXOffset, BorderSize;
	Real32 ScrollAcc;
	bool AltHandled;
};
NOINLINE_ void HotbarWidget_Create(struct HotbarWidget* w);


struct TableWidget {
	Widget_Layout
	Int32 ElementsCount, ElementsPerRow, RowsCount;
	Int32 LastCreatedIndex;
	FontDesc Font;
	Int32 SelectedIndex, BlockSize;
	Real32 SelBlockExpand;
	GfxResourceID VB;
	bool PendingClose;

	BlockID Elements[BLOCK_COUNT];
	struct ScrollbarWidget Scroll;
	struct Texture DescTex;
	Int32 LastX, LastY;
};

NOINLINE_ void TableWidget_Create(struct TableWidget* w);
NOINLINE_ void TableWidget_SetBlockTo(struct TableWidget* w, BlockID block);
NOINLINE_ void TableWidget_OnInventoryChanged(struct TableWidget* w);
NOINLINE_ void TableWidget_MakeDescTex(struct TableWidget* w, BlockID block);


#define INPUTWIDGET_MAX_LINES 3
#define INPUTWIDGET_LEN STRING_SIZE
struct InputWidget {
	Widget_Layout
	FontDesc Font;		
	Int32 (*GetMaxLines)(void);
	void (*RemakeTexture)(void* elem);  /* Remakes the raw texture containing all the chat lines. Also updates dimensions. */
	void (*OnPressedEnter)(void* elem); /* Invoked when the user presses enter. */
	bool (*AllowedChar)(void* elem, char c);

	String Text;
	String Lines[INPUTWIDGET_MAX_LINES];            /* raw text of each line */
	Size2D LineSizes[INPUTWIDGET_MAX_LINES]; /* size of each line in pixels */
	struct Texture InputTex;
	String Prefix;
	UInt16 PrefixWidth, PrefixHeight;
	bool ConvertPercents;

	UInt8 Padding;
	bool ShowCaret;
	UInt16 CaretWidth;
	Int32 CaretX, CaretY; /* Coordinates of caret in lines */
	Int32 CaretPos;       /* Position of caret, -1 for at end of string */
	PackedCol CaretCol;
	struct Texture CaretTex;
	Real64 CaretAccumulator;
};

NOINLINE_ void InputWidget_Clear(struct InputWidget* w);
NOINLINE_ void InputWidget_AppendString(struct InputWidget* w, STRING_PURE String* text);
NOINLINE_ void InputWidget_Append(struct InputWidget* w, char c);


struct MenuInputValidator;
struct MenuInputValidatorVTABLE {
	void (*GetRange)(struct MenuInputValidator*      v, STRING_TRANSIENT String* range);
	bool (*IsValidChar)(struct MenuInputValidator*   v, char c);
	bool (*IsValidString)(struct MenuInputValidator* v, STRING_PURE String* s);
	bool (*IsValidValue)(struct MenuInputValidator*  v, STRING_PURE String* s);
};

struct MenuInputValidator {
	struct MenuInputValidatorVTABLE* VTABLE;
	union {
		struct { const char** Names; UInt32 Count; } Meta_Enum;
		struct { Int32 Min,  Max; } Meta_Int;
		struct { Real32 Min, Max; } Meta_Real;
	};
};

struct MenuInputValidator MenuInputValidator_Hex(void);
struct MenuInputValidator MenuInputValidator_Integer(Int32 min, Int32 max);
struct MenuInputValidator MenuInputValidator_Seed(void);
struct MenuInputValidator MenuInputValidator_Real(Real32 min, Real32 max);
struct MenuInputValidator MenuInputValidator_Path(void);
struct MenuInputValidator MenuInputValidator_Enum(const char** names, UInt32 namesCount);
struct MenuInputValidator MenuInputValidator_String(void);

struct MenuInputWidget {
	struct InputWidget Base;
	Int32 MinWidth, MinHeight;
	struct MenuInputValidator Validator;
	char __TextBuffer[INPUTWIDGET_LEN];
};
NOINLINE_ void MenuInputWidget_Create(struct MenuInputWidget* w, Int32 width, Int32 height, STRING_PURE String* text, FontDesc* font, struct MenuInputValidator* v);


struct ChatInputWidget {
	struct InputWidget Base;
	Int32 TypingLogPos;
	char __TextBuffer[INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN];
	char __OrigBuffer[INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN];
	String OrigStr;
};

NOINLINE_ void ChatInputWidget_Create(struct ChatInputWidget* w, FontDesc* font);


#define TEXTGROUPWIDGET_MAX_LINES 30
#define TEXTGROUPWIDGET_LEN (STRING_SIZE + (STRING_SIZE / 2))
struct TextGroupWidget {
	Widget_Layout
	Int32 LinesCount, DefaultHeight;
	FontDesc Font, UnderlineFont;
	bool PlaceholderHeight[TEXTGROUPWIDGET_MAX_LINES];
	UInt8 LineLengths[TEXTGROUPWIDGET_MAX_LINES];
	struct Texture* Textures;
	char* Buffer;
};

NOINLINE_ void TextGroupWidget_Create(struct TextGroupWidget* w, Int32 linesCount, FontDesc* font, FontDesc* underlineFont, STRING_REF struct Texture* textures, STRING_REF char* buffer);
NOINLINE_ void TextGroupWidget_SetUsePlaceHolder(struct TextGroupWidget* w, Int32 index, bool placeHolder);
NOINLINE_ void TextGroupWidget_PushUpAndReplaceLast(struct TextGroupWidget* w, STRING_PURE String* text);
NOINLINE_ Int32 TextGroupWidget_UsedHeight(struct TextGroupWidget* w);
NOINLINE_ void TextGroupWidget_GetSelected(struct TextGroupWidget* w, STRING_TRANSIENT String* text, Int32 mouseX, Int32 mouseY);
NOINLINE_ void TextGroupWidget_GetText(struct TextGroupWidget* w, Int32 index, STRING_TRANSIENT String* text);
NOINLINE_ void TextGroupWidget_SetText(struct TextGroupWidget* w, Int32 index, STRING_PURE String* text);


struct PlayerListWidget {
	Widget_Layout
	FontDesc Font;
	UInt16 NamesCount, ElementOffset;
	Int32 XMin, XMax, YHeight;
	bool Classic;
	struct TextWidget Overview;
	UInt16 IDs[TABLIST_MAX_NAMES * 2];
	struct Texture Textures[TABLIST_MAX_NAMES * 2];
};
NOINLINE_ void PlayerListWidget_Create(struct PlayerListWidget* w, FontDesc* font, bool classic);
NOINLINE_ void PlayerListWidget_GetNameUnder(struct PlayerListWidget* w, Int32 mouseX, Int32 mouseY, STRING_TRANSIENT String* name);


typedef void (*SpecialInputAppendFunc)(void* userData, char c);
struct SpecialInputTab {
	Int32 ItemsPerRow, CharsPerItem;
	Size2D TitleSize;
	String Title, Contents;	
};

struct SpecialInputWidget {
	Widget_Layout
	Size2D ElementSize;
	Int32 SelectedIndex;
	bool PendingRedraw;
	struct InputWidget* AppendObj;
	struct Texture Tex;
	FontDesc Font;
	struct SpecialInputTab Tabs[5];
	String ColString;
	char __ColBuffer[DRAWER2D_MAX_COLS * 4];
};

NOINLINE_ void SpecialInputWidget_Create(struct SpecialInputWidget* w, FontDesc* font, struct InputWidget* appendObj);
NOINLINE_ void SpecialInputWidget_UpdateCols(struct SpecialInputWidget* w);
NOINLINE_ void SpecialInputWidget_SetActive(struct SpecialInputWidget* w, bool active);
#endif
