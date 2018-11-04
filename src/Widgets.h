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
NOINLINE_ void TextWidget_Create(struct TextWidget* w, const String* text, const FontDesc* font);
NOINLINE_ void TextWidget_Set(struct TextWidget* w, const String* text, const FontDesc* font);


typedef void (*Button_Get)(String* raw);
typedef void (*Button_Set)(const String* raw);
struct ButtonWidget {
	Widget_Layout
	struct Texture Texture;
	int MinWidth;

	const char* OptName;
	Button_Get GetValue;
	Button_Set SetValue;
};
NOINLINE_ void ButtonWidget_Create(struct ButtonWidget* w, int minWidth, const String* text, const FontDesc* font, Widget_LeftClick onClick);
NOINLINE_ void ButtonWidget_Set(struct ButtonWidget* w, const String* text, const FontDesc* font);


struct ScrollbarWidget {
	Widget_Layout
	int TotalRows, ScrollY;
	float ScrollingAcc;
	int MouseOffset;
	bool DraggingMouse;
};
NOINLINE_ void ScrollbarWidget_Create(struct ScrollbarWidget* w);


struct HotbarWidget {
	Widget_Layout
	struct Texture SelTex, BackTex;
	float BarHeight, SelBlockSize, ElemSize;
	float BarXOffset, BorderSize;
	float ScrollAcc;
	bool AltHandled;
};
NOINLINE_ void HotbarWidget_Create(struct HotbarWidget* w);


struct TableWidget {
	Widget_Layout
	int ElementsCount, ElementsPerRow, RowsCount;
	int LastCreatedIndex;
	FontDesc Font;
	int SelectedIndex, BlockSize;
	float SelBlockExpand;
	GfxResourceID VB;
	bool PendingClose;

	BlockID Elements[BLOCK_COUNT];
	struct ScrollbarWidget Scroll;
	struct Texture DescTex;
	int LastX, LastY;
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
	int (*GetMaxLines)(void);
	void (*RemakeTexture)(void* elem);  /* Remakes the raw texture containing all the chat lines. Also updates dimensions. */
	void (*OnPressedEnter)(void* elem); /* Invoked when the user presses enter. */
	bool (*AllowedChar)(void* elem, char c);

	String Text;
	String Lines[INPUTWIDGET_MAX_LINES];     /* raw text of each line */
	Size2D LineSizes[INPUTWIDGET_MAX_LINES]; /* size of each line in pixels */
	struct Texture InputTex;
	String Prefix;
	int PrefixWidth, PrefixHeight;
	bool ConvertPercents;

	uint8_t Padding;
	bool ShowCaret;
	int CaretWidth;
	int CaretX, CaretY; /* Coordinates of caret in lines */
	int CaretPos;       /* Position of caret, -1 for at end of string */
	PackedCol CaretCol;
	struct Texture CaretTex;
	double CaretAccumulator;
};

NOINLINE_ void InputWidget_Clear(struct InputWidget* w);
NOINLINE_ void InputWidget_AppendString(struct InputWidget* w, const String* text);
NOINLINE_ void InputWidget_Append(struct InputWidget* w, char c);


struct MenuInputValidator;
struct MenuInputValidatorVTABLE {
	void (*GetRange)(struct MenuInputValidator*      v, String* range);
	bool (*IsValidChar)(struct MenuInputValidator*   v, char c);
	bool (*IsValidString)(struct MenuInputValidator* v, const String* s);
	bool (*IsValidValue)(struct MenuInputValidator*  v, const String* s);
};

struct MenuInputValidator {
	struct MenuInputValidatorVTABLE* VTABLE;
	union {
		struct { const char** Names; int Count; } _Enum;
		struct { int Min, Max; } _Int;
		struct { float Min, Max; } _Float;
	} Meta;
};

struct MenuInputValidator MenuInputValidator_Hex(void);
struct MenuInputValidator MenuInputValidator_Int(int min, int max);
struct MenuInputValidator MenuInputValidator_Seed(void);
struct MenuInputValidator MenuInputValidator_Float(float min, float max);
struct MenuInputValidator MenuInputValidator_Path(void);
struct MenuInputValidator MenuInputValidator_Enum(const char** names, int namesCount);
struct MenuInputValidator MenuInputValidator_String(void);

struct MenuInputWidget {
	struct InputWidget Base;
	int MinWidth, MinHeight;
	struct MenuInputValidator Validator;
	char __TextBuffer[INPUTWIDGET_LEN];
};
NOINLINE_ void MenuInputWidget_Create(struct MenuInputWidget* w, int width, int height, const String* text, const FontDesc* font, struct MenuInputValidator* v);


struct ChatInputWidget {
	struct InputWidget Base;
	int TypingLogPos;
	char __TextBuffer[INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN];
	char __OrigBuffer[INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN];
	String OrigStr;
};

NOINLINE_ void ChatInputWidget_Create(struct ChatInputWidget* w, const FontDesc* font);


#define TEXTGROUPWIDGET_MAX_LINES 30
#define TEXTGROUPWIDGET_LEN (STRING_SIZE + (STRING_SIZE / 2))
struct TextGroupWidget {
	Widget_Layout
	int LinesCount, DefaultHeight;
	FontDesc Font, UnderlineFont;
	bool PlaceholderHeight[TEXTGROUPWIDGET_MAX_LINES];
	uint8_t LineLengths[TEXTGROUPWIDGET_MAX_LINES];
	struct Texture* Textures;
	char* Buffer;
};

NOINLINE_ void TextGroupWidget_Create(struct TextGroupWidget* w, int lines, const FontDesc* font, const FontDesc* ulFont, STRING_REF struct Texture* textures, STRING_REF char* buffer);
NOINLINE_ void TextGroupWidget_SetUsePlaceHolder(struct TextGroupWidget* w, int index, bool placeHolder);
NOINLINE_ void TextGroupWidget_PushUpAndReplaceLast(struct TextGroupWidget* w, const String* text);
NOINLINE_ int  TextGroupWidget_UsedHeight(struct TextGroupWidget* w);
NOINLINE_ void TextGroupWidget_GetSelected(struct TextGroupWidget* w, String* text, int mouseX, int mouseY);
NOINLINE_ void TextGroupWidget_GetText(struct TextGroupWidget* w, int index, String* text);
NOINLINE_ void TextGroupWidget_SetText(struct TextGroupWidget* w, int index, const String* text);


struct PlayerListWidget {
	Widget_Layout
	FontDesc Font;
	int NamesCount, ElementOffset;
	int XMin, XMax, YHeight;
	bool Classic;
	struct TextWidget Title;
	uint16_t IDs[TABLIST_MAX_NAMES * 2];
	struct Texture Textures[TABLIST_MAX_NAMES * 2];
};
NOINLINE_ void PlayerListWidget_Create(struct PlayerListWidget* w, const FontDesc* font, bool classic);
NOINLINE_ void PlayerListWidget_GetNameUnder(struct PlayerListWidget* w, int mouseX, int mouseY, String* name);


typedef void (*SpecialInputAppendFunc)(void* userData, char c);
struct SpecialInputTab {
	int ItemsPerRow, CharsPerItem;
	Size2D TitleSize;
	String Title, Contents;	
};

struct SpecialInputWidget {
	Widget_Layout
	Size2D ElementSize;
	int SelectedIndex;
	bool PendingRedraw;
	struct InputWidget* AppendObj;
	struct Texture Tex;
	FontDesc Font;
	struct SpecialInputTab Tabs[5];
	String ColString;
	char __ColBuffer[DRAWER2D_MAX_COLS * 4];
};

NOINLINE_ void SpecialInputWidget_Create(struct SpecialInputWidget* w, const FontDesc* font, struct InputWidget* appendObj);
NOINLINE_ void SpecialInputWidget_UpdateCols(struct SpecialInputWidget* w);
NOINLINE_ void SpecialInputWidget_SetActive(struct SpecialInputWidget* w, bool active);
#endif
