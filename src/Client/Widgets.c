#include "Widgets.h"
#include "GraphicsAPI.h"
#include "Drawer2D.h"

void Widget_SetLocation(Widget* widget, Anchor horAnchor, Anchor verAnchor, Int32 xOffset, Int32 yOffset) {
	widget->HorAnchor = horAnchor; widget->VerAnchor = verAnchor;
	widget->XOffset = xOffset; widget->YOffset = yOffset;
	widget->Reposition(widget);
}

void TextWidget_SetHeight(TextWidget* widget, Int32 height) {
	if (widget->ReducePadding) {
		Drawer2D_ReducePadding_Height(&height, font.Size, 4);
	}
	widget->DefaultHeight = height;
	widget->Base.Height = height;
}

void TextWidget_Init(GuiElement* elem) {
	TextWidget* widget = (TextWidget*)elem;
	Int32 height = Drawer2D_FontHeight(widget->Font, true);
	TextWidget_SetHeight(widget, height);
}

void TextWidget_Render(GuiElement* elem, Real64 delta) {
	TextWidget* widget = (TextWidget*)elem;	
	if (Texture_IsValid(&widget->Texture)) {
		Texture_RenderShaded(&widget->Texture, widget->Col);
	}
}

void TextWidget_Free(Widget* elem) {
	TextWidget* widget = (TextWidget*)elem;
	Gfx_DeleteTexture(&widget->Texture.ID);
}

void TextWidget_Reposition(Widget* elem) {
	TextWidget* widget = (TextWidget*)elem;
	Int32 oldX = elem->X, oldY = elem->Y;
	Widget_DoReposition(elem);
	widget->Texture.X += elem->X - oldX;
	widget->Texture.Y += elem->Y - oldY;
}

void TextWidget_Create(TextWidget* widget, STRING_TRANSIENT String* text, void* font) {
	Widget_Init(&widget->Base);
	widget->Font = font;
	widget->Base.Reposition  = TextWidget_Reposition;
	widget->Base.Base.Init   = TextWidget_Init;
	widget->Base.Base.Render = TextWidget_Render;
	widget->Base.Base.Free   = TextWidget_Free;
}

void TextWidget_SetText(TextWidget* widget, STRING_TRANSIENT String* text) {
	Gfx_DeleteTexture(&widget->Texture.ID);
	Widget* elem = (Widget*)widget;
	if (Drawer2D_IsEmptyText(text)) {
		widget->Texture = Texture_MakeInvalid();
		elem->Width = 0; elem->Height = widget->DefaultHeight;
	} else {
		DrawTextArgs args;
		DrawTextArgs_Make(&args, text, widget->Font, true);
		widget->Texture = Drawer2D_MakeTextTexture(&args, 0, 0);
		if (widget->ReducePadding) {
			Drawer2D_ReducePadding_Tex(&widget->Texture, font.Size, 4);
		}

		elem->Width = widget->Texture.Width; elem->Height = widget->Texture.Height;
		elem->Reposition(elem);
		widget->Texture.X = elem->X; widget->Texture.Y = elem->Y;
	}
}