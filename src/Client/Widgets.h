#ifndef CS_WIDGETS_H
#define CS_WIDGETS_H
#include "Gui.h"
#include "Texture.h"
#include "PackedCol.h"
#include "String.h"

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
	String Text;
	Int32 DefaultHeight;
	void* Font;

	String OptName;
	ButtonWidget_GetValue GetValue;
	ButtonWidget_SetValue SetValue;
	Int32 MinWidth, MinHeight;
} ButtonWidget;

void ButtonWidget_Create(ButtonWidget* widget, STRING_TRANSIENT String* text, Int32 minWidth, void* font, Gui_MouseHandler onClick);
void ButtonWidget_SetText(ButtonWidget* widget, STRING_TRANSIENT String* text);


#endif