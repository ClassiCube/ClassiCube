#include "Widgets.h"
#include "GraphicsAPI.h"
#include "Drawer2D.h"
#include "GraphicsCommon.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Inventory.h"
#include "IsometricDrawer.h"
#include "Utils.h"
#include "ModelCache.h"
#include "Screens.h"
#include "Platform.h"
#include "WordWrap.h"
#include "ServerConnection.h"

void Widget_SetLocation(Widget* widget, Anchor horAnchor, Anchor verAnchor, Int32 xOffset, Int32 yOffset) {
	widget->HorAnchor = horAnchor; widget->VerAnchor = verAnchor;
	widget->XOffset = xOffset; widget->YOffset = yOffset;
	widget->Reposition(widget);
}

void TextWidget_SetHeight(TextWidget* widget, Int32 height) {
	if (widget->ReducePadding) {
		Drawer2D_ReducePadding_Height(&height, widget->Font.Size, 4);
	}
	widget->DefaultHeight = height;
	widget->Base.Height = height;
}

void TextWidget_Init(GuiElement* elem) {
	TextWidget* widget = (TextWidget*)elem;
	Int32 height = Drawer2D_FontHeight(&widget->Font, true);
	TextWidget_SetHeight(widget, height);
}

void TextWidget_Render(GuiElement* elem, Real64 delta) {
	TextWidget* widget = (TextWidget*)elem;	
	if (Texture_IsValid(&widget->Texture)) {
		Texture_RenderShaded(&widget->Texture, widget->Col);
	}
}

void TextWidget_Free(GuiElement* elem) {
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

void TextWidget_Create(TextWidget* widget, STRING_TRANSIENT String* text, FontDesc* font) {
	Widget_Init(&widget->Base);
	PackedCol col = PACKEDCOL_WHITE;
	widget->Col = col;
	widget->Font = *font;
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
		DrawTextArgs_Make(&args, text, &widget->Font, true);
		widget->Texture = Drawer2D_MakeTextTexture(&args, 0, 0);
		if (widget->ReducePadding) {
			Drawer2D_ReducePadding_Tex(&widget->Texture, widget->Font.Size, 4);
		}

		elem->Width = widget->Texture.Width; elem->Height = widget->Texture.Height;
		elem->Reposition(elem);
		widget->Texture.X = elem->X; widget->Texture.Y = elem->Y;
	}
}

#define BUTTON_uWIDTH (200.0f / 256.0f)
Texture Button_ShadowTex   = { 0, 0, 0, 0, 0,  0.0f, 66.0f / 256.0f, 200.0f / 256.0f,  86.0f / 256.0f };
Texture Button_SelectedTex = { 0, 0, 0, 0, 0,  0.0f, 86.0f / 256.0f, 200.0f / 256.0f, 106.0f / 256.0f };
Texture Button_DisabledTex = { 0, 0, 0, 0, 0,  0.0f, 46.0f / 256.0f, 200.0f / 256.0f,  66.0f / 256.0f };
PackedCol Button_NormCol     = PACKEDCOL_CONST(224, 224, 244, 255);
PackedCol Button_ActiveCol   = PACKEDCOL_CONST(255, 255, 160, 255);
PackedCol Button_DisabledCol = PACKEDCOL_CONST(160, 160, 160, 255);

void ButtonWidget_Init(GuiElement* elem) {
	ButtonWidget* widget = (ButtonWidget*)elem;
	widget->DefaultHeight = Drawer2D_FontHeight(&widget->Font, true);
	widget->Base.Height = widget->DefaultHeight;
}

void ButtonWidget_Free(GuiElement* elem) {
	ButtonWidget* widget = (ButtonWidget*)elem;
	Gfx_DeleteTexture(&widget->Texture.ID);
}

void ButtonWidget_Reposition(Widget* elem) {
	Int32 oldX = elem->X, oldY = elem->Y;
	Widget_DoReposition(elem);

	ButtonWidget* widget = (ButtonWidget*)elem;
	widget->Texture.X += elem->X - oldX;
	widget->Texture.Y += elem->Y - oldY;
}

void ButtonWidget_Render(GuiElement* elem, Real64 delta) {
	ButtonWidget* widget = (ButtonWidget*)elem;
	Widget* elemW = &widget->Base;
	if (!Texture_IsValid(&widget->Texture)) return;
	Texture back = elemW->Active ? Button_SelectedTex : Button_ShadowTex;
	if (elemW->Disabled) back = Button_DisabledTex;

	back.ID = Game_UseClassicGui ? Gui_GuiClassicTex : Gui_GuiTex;
	back.X = (Int16)elemW->X; back.Width  = (UInt16)elemW->Width;
	back.Y = (Int16)elemW->Y; back.Height = (UInt16)elemW->Height;

	if (elemW->Width == 400) {
		/* Button can be drawn normally */
		back.U1 = 0.0f; back.U2 = BUTTON_uWIDTH;
		Texture_Render(&back);
	} else {
		/* Split button down the middle */
		Real32 scale = (elemW->Width / 400.0f) * 0.5f;
		Gfx_BindTexture(back.ID); /* avoid bind twice */
		PackedCol white = PACKEDCOL_WHITE;

		back.Width = (UInt16)(elemW->Width / 2);
		back.U1 = 0.0f; back.U2 = BUTTON_uWIDTH * scale;
		GfxCommon_Draw2DTexture(&back, white);

		back.X += (Int16)(elemW->Width / 2);
		back.U1 = BUTTON_uWIDTH * (1.0f - scale); back.U2 = BUTTON_uWIDTH;
		GfxCommon_Draw2DTexture(&back, white);
	}

	PackedCol col = elemW->Disabled ? Button_DisabledCol : (elemW->Active ? Button_ActiveCol : Button_NormCol);
	Texture_RenderShaded(&widget->Texture, col);
}

void ButtonWidget_Create(ButtonWidget* widget, STRING_PURE String* text, Int32 minWidth, FontDesc* font, Gui_MouseHandler onClick) {
	Widget_Init(&widget->Base);
	widget->Base.Base.Init   = ButtonWidget_Init;
	widget->Base.Base.Render = ButtonWidget_Render;
	widget->Base.Base.Free   = ButtonWidget_Free;
	widget->Base.Reposition  = ButtonWidget_Reposition;

	widget->Font = *font;
	GuiElement* elem = &widget->Base.Base;
	elem->Init(elem);
	widget->MinWidth = minWidth; widget->MinHeight = 40;
	ButtonWidget_SetText(widget, text);
	widget->Base.Base.HandlesMouseDown = onClick;
}

void ButtonWidget_SetText(ButtonWidget* widget, STRING_PURE String* text) {
	Gfx_DeleteTexture(&widget->Texture.ID);
	Widget* elem = &widget->Base;
	if (Drawer2D_IsEmptyText(text)) {
		widget->Texture = Texture_MakeInvalid();
		elem->Width = 0; elem->Height = widget->DefaultHeight;
	} else {
		DrawTextArgs args;
		DrawTextArgs_Make(&args, text, &widget->Font, true);
		widget->Texture = Drawer2D_MakeTextTexture(&args, 0, 0);
		elem->Width  = max(widget->Texture.Width,  widget->MinWidth);
		elem->Height = max(widget->Texture.Height, widget->MinHeight);

		widget->Base.Reposition(&widget->Base);
		widget->Texture.X = elem->X + (elem->Width / 2  - widget->Texture.Width  / 2);
		widget->Texture.Y = elem->Y + (elem->Height / 2 - widget->Texture.Height / 2);
	}
}


#define TABLE_MAX_ROWS_DISPLAYED 8
#define SCROLL_WIDTH 22
#define SCROLL_BORDER 2
#define SCROLL_NUBS_WIDTH 3
PackedCol Scroll_BackCol  = PACKEDCOL_CONST( 10,  10,  10, 220);
PackedCol Scroll_BarCol   = PACKEDCOL_CONST(100, 100, 100, 220);
PackedCol Scroll_HoverCol = PACKEDCOL_CONST(122, 122, 122, 220);

void ScrollbarWidget_Init(GuiElement* elem) { }
void ScrollbarWidget_Free(GuiElement* elem) { }

Real32 ScrollbarWidget_GetScale(ScrollbarWidget* widget) {
	Real32 rows = (Real32)widget->TotalRows;
	return (widget->Base.Height - SCROLL_BORDER * 2) / rows;
}

void ScrollbarWidget_GetScrollbarCoords(ScrollbarWidget* widget, Int32* y, Int32* height) {
	Real32 scale = ScrollbarWidget_GetScale(widget);
	*y = Math_Ceil(widget->ScrollY * scale) + SCROLL_BORDER;
	*height = Math_Ceil(TABLE_MAX_ROWS_DISPLAYED * scale);
	*height = min(*y + *height, widget->Base.Height - SCROLL_BORDER) - *y;
}

void ScrollbarWidget_Render(GuiElement* elem, Real64 delta) {
	ScrollbarWidget* widget = (ScrollbarWidget*)elem;
	Widget* elemW = &widget->Base;
	Int32 x = elemW->X, width = elemW->Width;
	GfxCommon_Draw2DFlat(x, elemW->Y, width, elemW->Height, Scroll_BackCol);

	Int32 y, height;
	ScrollbarWidget_GetScrollbarCoords(widget, &y, &height);
	x += SCROLL_BORDER; y += elemW->Y;
	width -= SCROLL_BORDER * 2; 

	bool hovered = Mouse_Y >= y && Mouse_Y < (y + height) && Mouse_X >= x && Mouse_X < (x + width);
	PackedCol barCol = hovered ? Scroll_HoverCol : Scroll_BarCol;
	GfxCommon_Draw2DFlat(x, y, width, height, barCol);

	if (height < 20) return;
	x += SCROLL_NUBS_WIDTH; y += (height / 2);
	width -= SCROLL_NUBS_WIDTH * 2;

	GfxCommon_Draw2DFlat(x, y - 1 - 4, width, SCROLL_BORDER, Scroll_BackCol);
	GfxCommon_Draw2DFlat(x, y - 1,     width, SCROLL_BORDER, Scroll_BackCol);
	GfxCommon_Draw2DFlat(x, y - 1 + 4, width, SCROLL_BORDER, Scroll_BackCol);
}

bool ScrollbarWidget_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	ScrollbarWidget* widget = (ScrollbarWidget*)elem;
	if (widget->DraggingMouse) return true;
	if (btn != MouseButton_Left) return false;
	if (x < widget->Base.X || x >= widget->Base.X + widget->Base.Width) return false;

	y -= widget->Base.Y;
	Int32 curY, height;
	ScrollbarWidget_GetScrollbarCoords(widget, &curY, &height);

	if (y < curY) {
		widget->ScrollY -= TABLE_MAX_ROWS_DISPLAYED;
	} else if (y >= curY + height) {
		widget->ScrollY += TABLE_MAX_ROWS_DISPLAYED;
	} else {
		widget->DraggingMouse = true;
		widget->MouseOffset = y - curY;
	}
	ScrollbarWidget_ClampScrollY(widget);
	return true;
}

bool ScrollbarWidget_HandlesMouseUp(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	ScrollbarWidget* widget = (ScrollbarWidget*)elem;
	widget->DraggingMouse = false;
	widget->MouseOffset = 0;
	return true;
}

bool ScrollbarWidget_HandlesMouseScroll(GuiElement* elem, Real32 delta) {
	ScrollbarWidget* widget = (ScrollbarWidget*)elem;
	Int32 steps = Utils_AccumulateWheelDelta(&widget->ScrollingAcc, delta);
	widget->ScrollY -= steps;
	ScrollbarWidget_ClampScrollY(widget);
	return true;
}

bool ScrollbarWidget_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	ScrollbarWidget* widget = (ScrollbarWidget*)elem;
	if (widget->DraggingMouse) {
		y -= widget->Base.Y;
		Real32 scale = ScrollbarWidget_GetScale(widget);
		widget->ScrollY = (Int32)((y - widget->MouseOffset) / scale);
		ScrollbarWidget_ClampScrollY(widget);
		return true;
	}
	return false;
}

void ScrollbarWidget_Create(ScrollbarWidget* widget) {
	Widget_Init(&widget->Base);	
	widget->Base.Base.Init   = ScrollbarWidget_Init;
	widget->Base.Base.Render = ScrollbarWidget_Render;
	widget->Base.Base.Free   = ScrollbarWidget_Free;

	widget->Base.Base.HandlesMouseDown   = ScrollbarWidget_HandlesMouseDown;
	widget->Base.Base.HandlesMouseUp     = ScrollbarWidget_HandlesMouseUp;
	widget->Base.Base.HandlesMouseScroll = ScrollbarWidget_HandlesMouseScroll;
	widget->Base.Base.HandlesMouseMove   = ScrollbarWidget_HandlesMouseMove;	

	widget->Base.Width = SCROLL_WIDTH;
	widget->TotalRows = 0;
	widget->ScrollY = 0;
	widget->ScrollingAcc = 0.0f;
	widget->DraggingMouse = false;
	widget->MouseOffset = 0;
}

void ScrollbarWidget_ClampScrollY(ScrollbarWidget* widget) {
	Int32 maxRows = widget->TotalRows - TABLE_MAX_ROWS_DISPLAYED;
	if (widget->ScrollY >= maxRows) widget->ScrollY = maxRows;
	if (widget->ScrollY < 0) widget->ScrollY = 0;
}


void HotbarWidget_RenderHotbarOutline(HotbarWidget* widget) {
	Widget* w = &widget->Base;
	GfxResourceID texId = Game_UseClassicGui ? Gui_GuiClassicTex : Gui_GuiTex;
	widget->BackTex.ID = texId;
	Texture_Render(&widget->BackTex);

	Int32 i = Inventory_SelectedIndex;
	Real32 width = widget->ElemSize + widget->BorderSize;
	Int32 x = (Int32)(w->X + widget->BarXOffset + width * i + widget->ElemSize / 2);

	widget->SelTex.ID = texId;
	widget->SelTex.X = (Int32)(x - widget->SelBlockSize / 2);
	PackedCol white = PACKEDCOL_WHITE;
	GfxCommon_Draw2DTexture(&widget->SelTex, white);
}

void HotbarWidget_RenderHotbarBlocks(HotbarWidget* widget) {
	/* TODO: Should hotbar use its own VB? */
	VertexP3fT2fC4b vertices[INVENTORY_BLOCKS_PER_HOTBAR * ISOMETRICDRAWER_MAXVERTICES];
	IsometricDrawer_BeginBatch(vertices, ModelCache_Vb);
	Widget* w = &widget->Base;

	Real32 width = widget->ElemSize + widget->BorderSize;
	UInt32 i;
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		BlockID block = Inventory_Get(i);
		Int32 x = (Int32)(w->X + widget->BarXOffset + width * i + widget->ElemSize / 2);
		Int32 y = (Int32)(w->Y + (w->Height - widget->BarHeight / 2));

		Real32 scale = (widget->ElemSize * 13.5f / 16.0f) / 2.0f;
		IsometricDrawer_DrawBatch(block, scale, x, y);
	}
	IsometricDrawer_EndBatch();
}

void HotbarWidget_RepositonBackgroundTexture(HotbarWidget* widget) {
	Widget* w = &widget->Base;
	TextureRec rec = TextureRec_FromPoints(0.0f, 0.0f, 182.0f / 256.0f, 22.0f / 256.0f);
	widget->BackTex = Texture_FromRec(0, w->X, w->Y, w->Width, w->Height, rec);
}

void HotbarWidget_RepositionSelectionTexture(HotbarWidget* widget) {
	Widget* w = &widget->Base;
	Int32 hSize = (Int32)widget->SelBlockSize;

	Real32 scale = Game_GetHotbarScale();
	Int32 vSize = (Int32)(22.0f * scale);
	Int32 y = w->Y + (w->Height - (Int32)(23.0f * scale));

	TextureRec rec = TextureRec_FromPoints(0.0f, 22.0f / 256.0f, 24.0f / 256.0f, 22.0f / 256.0f);
	widget->SelTex = Texture_FromRec(0, 0, y, hSize, vSize, rec);
}

Int32 HotbarWidget_ScrolledIndex(HotbarWidget* widget, Real32 delta, Int32 index, Int32 dir) {
	Int32 steps = Utils_AccumulateWheelDelta(&widget->ScrollAcc, delta);
	index += (dir * steps) % INVENTORY_BLOCKS_PER_HOTBAR;

	if (index < 0) index += INVENTORY_BLOCKS_PER_HOTBAR;
	if (index >= INVENTORY_BLOCKS_PER_HOTBAR) {
		index -= INVENTORY_BLOCKS_PER_HOTBAR;
	}
	return index;
}

void HotbarWidget_Reposition(Widget* elem) {
	HotbarWidget* widget = (HotbarWidget*)elem;
	Real32 scale = Game_GetHotbarScale();

	widget->BarHeight = (Real32)Math_Floor(22.0f * scale);
	elem->Width  = (Int32)(182 * scale);
	elem->Height = (Int32)widget->BarHeight;

	widget->SelBlockSize = (Real32)Math_Ceil(24.0f * scale);
	widget->ElemSize     = 16.0f * scale;
	widget->BarXOffset   = 3.1f * scale;
	widget->BorderSize   = 4.0f * scale;

	elem->Reposition(elem);
	HotbarWidget_RepositonBackgroundTexture(widget);
	HotbarWidget_RepositionSelectionTexture(widget);
}

void HotbarWidget_Init(GuiElement* elem) { 
	Widget* widget = (Widget*)elem;
	widget->Reposition(widget);
}

void HotbarWidget_Render(GuiElement* elem, Real64 delta) {
	HotbarWidget* widget = (HotbarWidget*)elem;
	HotbarWidget_RenderHotbarOutline(widget);
	HotbarWidget_RenderHotbarBlocks(widget);
}
void HotbarWidget_Free(GuiElement* elem) { }

bool HotbarWidget_HandlesKeyDown(GuiElement* elem, Key key) {
	if (key >= Key_1 && key <= Key_9) {
		Int32 index = key - Key_1;
		if (KeyBind_GetPressed(KeyBind_HotbarSwitching)) {
			/* Pick from first to ninth row */
			Inventory_SetOffset(index * INVENTORY_BLOCKS_PER_HOTBAR);
			HotbarWidget* widget = (HotbarWidget*)elem;
			widget->AltHandled = true;
		} else {
			Inventory_SetSelectedIndex(index);
		}
		return true;
	}
	return false;
}

bool HotbarWidget_HandlesKeyUp(GuiElement* elem, Key key) {
	/* We need to handle these cases:
	   a) user presses alt then number
	   b) user presses alt
	   thus we only do case b) if case a) did not happen */
	HotbarWidget* widget = (HotbarWidget*)elem;
	if (key != KeyBind_Get(KeyBind_HotbarSwitching)) return false;
	if (widget->AltHandled) { widget->AltHandled = false; return true; } /* handled already */

	/* Don't switch hotbar when alt+tab */
	if (!Window_GetFocused()) return true;

	/* Alternate between first and second row */
	Int32 index = Inventory_Offset == 0 ? 1 : 0;
	Inventory_SetOffset(index * INVENTORY_BLOCKS_PER_HOTBAR);
	return true;
}

bool HotbarWidget_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	Widget* w = (Widget*)elem;
	if (btn != MouseButton_Left || !Gui_Contains(w->X, w->Y, w->Width, w->Height, x, y)) return false;
	Screen* screen = Gui_GetActiveScreen();
	if (screen != InventoryScreen_Unsafe_RawPointer) return false;

	HotbarWidget* widget = (HotbarWidget*)elem;
	Int32 width  = (Int32)(widget->ElemSize * widget->BorderSize);
	Int32 height = Math_Ceil(widget->BarHeight);
	UInt32 i;

	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		Int32 winX = (Int32)(w->X + width * i);
		Int32 winY = (Int32)(w->Y + (w->Height - height));

		if (Gui_Contains(winX, winY, width, height, x, y)) {
			Inventory_SetSelectedIndex(i);
			return true;
		}
	}
	return false;
}

bool HotbarWidget_HandlesMouseScroll(GuiElement* elem, Real32 delta) {
	HotbarWidget* widget = (HotbarWidget*)elem;
	if (Key_IsAltPressed()) {
		Int32 index = Inventory_Offset / INVENTORY_BLOCKS_PER_HOTBAR;
		index = HotbarWidget_ScrolledIndex(widget, delta, index, 1);
		Inventory_SetOffset(index * INVENTORY_BLOCKS_PER_HOTBAR);
		widget->AltHandled = true;
	} else {
		Int32 index = HotbarWidget_ScrolledIndex(widget, delta, Inventory_SelectedIndex, -1);
		Inventory_SetSelectedIndex(index);
	}
	return true;
}

void HotbarWidget_Create(HotbarWidget* widget) {
	Widget_Init(&widget->Base);
	widget->Base.HorAnchor = ANCHOR_CENTRE;
	widget->Base.VerAnchor = ANCHOR_BOTTOM_OR_RIGHT;

	widget->Base.Base.Init   = HotbarWidget_Init;
	widget->Base.Base.Render = HotbarWidget_Render;
	widget->Base.Base.Free   = HotbarWidget_Free;
	widget->Base.Reposition  = HotbarWidget_Reposition;

	widget->Base.Base.HandlesKeyDown     = HotbarWidget_HandlesKeyDown;
	widget->Base.Base.HandlesKeyUp       = HotbarWidget_HandlesKeyUp;
	widget->Base.Base.HandlesMouseDown   = HotbarWidget_HandlesMouseDown;
	widget->Base.Base.HandlesMouseScroll = HotbarWidget_HandlesMouseScroll;
}


Int32 Table_X(TableWidget* widget) { return widget->Base.X - 5 - 10; }
Int32 Table_Y(TableWidget* widget) { return widget->Base.Y - 5 - 30; }
Int32 Table_Width(TableWidget* widget) { 
	return widget->ElementsPerRow * widget->BlockSize + 10 + 20; 
}
Int32 Table_Height(TableWidget* widget) {
	return min(widget->RowsCount, TABLE_MAX_ROWS_DISPLAYED) * widget->BlockSize + 10 + 40;
}

/* These were sourced by taking a screenshot of vanilla
   Then using paint to extract the colour components
   Then using wolfram alpha to solve the glblendfunc equation */
PackedCol Table_TopBackCol    = PACKEDCOL_CONST( 34,  34,  34, 168);
PackedCol Table_BottomBackCol = PACKEDCOL_CONST( 57,  57, 104, 202);
PackedCol Table_TopSelCol     = PACKEDCOL_CONST(255, 255, 255, 142);
PackedCol Table_BottomSelCol  = PACKEDCOL_CONST(255, 255, 255, 192);
#define TABLE_MAX_VERTICES (8 * 10 * ISOMETRICDRAWER_MAXVERTICES)

bool TableWidget_GetCoords(TableWidget* widget, Int32 i, Int32* winX, Int32* winY) {
	Int32 x = i % widget->ElementsPerRow, y = i / widget->ElementsPerRow;
	*winX = widget->Base.X + widget->BlockSize * x;
	*winY = widget->Base.Y + widget->BlockSize * y + 3;

	*winY -= widget->Scroll.ScrollY * widget->BlockSize;
	y -= widget->Scroll.ScrollY;
	return y >= 0 && y < TABLE_MAX_ROWS_DISPLAYED;
}

void TableWidget_UpdateScrollbarPos(TableWidget* widget) {
	ScrollbarWidget* scroll = &widget->Scroll;
	scroll->Base.X = Table_X(widget) + Table_Width(widget);
	scroll->Base.Y = Table_Y(widget);
	scroll->Base.Height = Table_Height(widget);
	scroll->TotalRows = widget->RowsCount;
}

void TableWidget_MoveCursorToSelected(TableWidget* widget) {
	if (widget->SelectedIndex == -1) return;

	Int32 x, y, i = widget->SelectedIndex;
	TableWidget_GetCoords(widget, i, &x, &y);
	x += widget->BlockSize / 2; y += widget->BlockSize / 2;

	Point2D topLeft = Window_PointToScreen(Point2D_Empty);
	x += topLeft.X; y += topLeft.Y;
	Window_SetDesktopCursorPos(Point2D_Make(x, y));
}

void TableWidget_MakeBlockDesc(STRING_TRANSIENT String* desc, BlockID block) {
	if (Game_PureClassic) { String_AppendConst(desc, "Select block"); return; }
	String_AppendString(desc, &Block_Name[block]);
	if (Game_ClassicMode) return;

	String_AppendConst(desc, " (ID ");
	String_AppendInt32(desc, block);
	String_AppendConst(desc, "&f, place ");
	String_AppendConst(desc, Block_CanPlace[block] ? "&aYes" : "&cNo");
	String_AppendConst(desc, "&f, delete ");
	String_AppendConst(desc, Block_CanDelete[block] ? "&aYes" : "&cNo");
	String_AppendConst(desc, "&f)");
}

void TableWidget_UpdateDescTexPos(TableWidget* widget) {
	widget->DescTex.X = widget->Base.X + widget->Base.Width / 2 - widget->DescTex.Width / 2;
	widget->DescTex.Y = widget->Base.Y - widget->DescTex.Height - 5;
}

void TableWidget_UpdatePos(TableWidget* widget) {
	Int32 rowsDisplayed = min(TABLE_MAX_ROWS_DISPLAYED, widget->ElementsCount);
	widget->Base.Width = widget->BlockSize * widget->ElementsPerRow;
	widget->Base.Height = widget->BlockSize * rowsDisplayed;
	widget->Base.X = Game_Width  / 2 - widget->Base.Width  / 2;
	widget->Base.Y = Game_Height / 2 - widget->Base.Height / 2;
	TableWidget_UpdateDescTexPos(widget);
}

#define TABLE_NAME_LEN 128
void TableWidget_RecreateDescTex(TableWidget* widget) {
	if (widget->SelectedIndex == widget->LastCreatedIndex) return;
	if (widget->ElementsCount == 0) return;
	widget->LastCreatedIndex = widget->SelectedIndex;

	Gfx_DeleteTexture(&widget->DescTex.ID);
	if (widget->SelectedIndex == -1) return;

	UInt8 descBuffer[String_BufferSize(TABLE_NAME_LEN)];
	String desc = String_FromRawBuffer(descBuffer, TABLE_NAME_LEN);
	BlockID block = widget->Elements[widget->SelectedIndex];
	TableWidget_MakeBlockDesc(&desc, block);

	DrawTextArgs args;
	DrawTextArgs_Make(&args, &desc, &widget->Font, true);
	widget->DescTex = Drawer2D_MakeTextTexture(&args, 0, 0);
	TableWidget_UpdateDescTexPos(widget);
}

bool TableWidget_Show(BlockID block) {
	if (block == BlockID_Invalid) return false;

	if (block < BLOCK_CPE_COUNT) {
		Int32 count = Game_UseCPEBlocks ? BLOCK_CPE_COUNT : BLOCK_ORIGINAL_COUNT;
		return block < count;
	}
	return true;
}

void TableWidget_RecreateElements(TableWidget* widget) {
	widget->ElementsCount = 0;
	Int32 count = Game_UseCPE ? BLOCK_COUNT : BLOCK_ORIGINAL_COUNT, i;
	for (i = 0; i < count; i++) {
		BlockID block = Inventory_Map[i];
		if (TableWidget_Show(block)) {
			widget->ElementsCount++;
		}
	}

	widget->RowsCount = Math_CeilDiv(widget->ElementsCount, widget->ElementsPerRow);
	TableWidget_UpdateScrollbarPos(widget);
	TableWidget_UpdatePos(widget);

	Int32 index = 0;
	for (i = 0; i < count; i++) {
		BlockID block = Inventory_Map[i];
		if (TableWidget_Show(block)) {
			widget->Elements[index++] = block;
		}
	}
}

void TableWidget_Init(GuiElement* elem) {
	TableWidget* widget = (TableWidget*)elem;
	ScrollbarWidget_Create(&widget->Scroll);
	TableWidget_RecreateElements(widget);
	widget->Base.Reposition(&widget->Base);
	TableWidget_SetBlockTo(widget, Inventory_SelectedBlock);
	elem->Recreate(elem);
}

void TableWidget_Render(GuiElement* elem, Real64 delta) {
	TableWidget* widget = (TableWidget*)elem;
	GfxCommon_Draw2DGradient(Table_X(widget), Table_Y(widget),
		Table_Width(widget), Table_Height(widget), Table_TopBackCol, Table_BottomBackCol);
	if (widget->RowsCount > TABLE_MAX_ROWS_DISPLAYED) {
		GuiElement* scroll = &widget->Scroll.Base.Base;
		scroll->Render(scroll, delta);
	}

	Int32 blockSize = widget->BlockSize;
	if (widget->SelectedIndex != -1 && Game_ClassicMode) {
		Int32 x, y;
		TableWidget_GetCoords(widget, widget->SelectedIndex, &x, &y);
		Real32 off = blockSize * 0.1f;
		Int32 size = (Int32)(blockSize + off * 2);
		GfxCommon_Draw2DGradient((Int32)(x - off), (Int32)(y - off), 
			size, size, Table_TopSelCol, Table_BottomSelCol);
	}
	Gfx_SetTexturing(true);
	Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);

	VertexP3fT2fC4b vertices[TABLE_MAX_VERTICES];
	IsometricDrawer_BeginBatch(vertices, widget->VB);
	Int32 i;
	for (i = 0; i < widget->ElementsCount; i++) {
		Int32 x, y;
		if (!TableWidget_GetCoords(widget, i, &x, &y)) continue;

		/* We want to always draw the selected block on top of others */
		if (i == widget->SelectedIndex) continue;
		IsometricDrawer_DrawBatch(widget->Elements[i], blockSize * 0.7f / 2.0f,
			x + blockSize / 2, y + blockSize / 2);
	}

	i = widget->SelectedIndex;
	if (i != -1) {
		Int32 x, y;
		TableWidget_GetCoords(widget, i, &x, &y);
		IsometricDrawer_DrawBatch(widget->Elements[i],
			(blockSize + widget->SelBlockExpand) * 0.7f / 2.0f,
			x + blockSize / 2, y + blockSize / 2);
	}
	IsometricDrawer_EndBatch();

	if (Texture_IsValid(&widget->DescTex)) {
		Texture_Render(&widget->DescTex);
	}
	Gfx_SetTexturing(false);
}

void TableWidget_Free(GuiElement* elem) {
	TableWidget* widget = (TableWidget*)elem;
	Gfx_DeleteVb(&widget->VB);
	Gfx_DeleteTexture(&widget->DescTex.ID);
	widget->LastCreatedIndex = -1000;
}

void TableWidget_Recreate(GuiElement* elem) {
	TableWidget* widget = (TableWidget*)elem;
	elem->Free(elem);
	widget->VB = Gfx_CreateDynamicVb(VertexFormat_P3fT2fC4b, TABLE_MAX_VERTICES);
	TableWidget_RecreateDescTex(widget);
}

void TableWidget_Reposition(Widget* elem) {
	TableWidget* widget = (TableWidget*)elem;
	Real32 scale = Game_GetInventoryScale();
	widget->BlockSize = (Int32)(50 * Math_Sqrt(scale));
	widget->SelBlockExpand = 25.0f * Math_Sqrt(scale);
	TableWidget_UpdatePos(widget);
	TableWidget_UpdateScrollbarPos(widget);
}

void TableWidget_ScrollRelative(TableWidget* widget, Int32 delta) {
	Int32 startIndex = widget->SelectedIndex, index = widget->SelectedIndex;
	index += delta;
	if (index < 0) index -= delta;
	if (index >= widget->ElementsCount) index -= delta;
	widget->SelectedIndex = index;

	Int32 scrollDelta = (index / widget->ElementsPerRow) - (startIndex / widget->ElementsPerRow);
	widget->Scroll.ScrollY += scrollDelta;
	ScrollbarWidget_ClampScrollY(&widget->Scroll);
	TableWidget_RecreateDescTex(widget);
	TableWidget_MoveCursorToSelected(widget);
}

bool TableWidget_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	TableWidget* widget = (TableWidget*)elem;
	widget->PendingClose = false;
	if (btn != MouseButton_Left) return false;

	GuiElement* scroll = &widget->Scroll.Base.Base;
	if (scroll->HandlesMouseDown(scroll, x, y, btn)) {
		return true;
	} else if (widget->SelectedIndex != -1) {
		Inventory_SetSelectedBlock(widget->Elements[widget->SelectedIndex]);
		widget->PendingClose = true;
		return true;
	} else if (Gui_Contains(Table_X(widget), Table_Y(widget), Table_Width(widget), Table_Height(widget), x, y)) {
		return true;
	}
	return false;
}

bool TableWidget_HandlesMouseUp(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	TableWidget* widget = (TableWidget*)elem;
	GuiElement* scroll = &widget->Scroll.Base.Base;
	return scroll->HandlesMouseUp(scroll, x, y, btn);
}

bool TableWidget_HandlesMouseScroll(GuiElement* elem, Real32 delta) {
	TableWidget* widget = (TableWidget*)elem;
	Int32 scrollWidth = widget->Scroll.Base.Width;
	bool bounds = Gui_Contains(Table_X(widget) - scrollWidth, Table_Y(widget),
		Table_Width(widget) + scrollWidth, Table_Height(widget), Mouse_X, Mouse_Y);
	if (!bounds) return false;

	Int32 startScrollY = widget->Scroll.ScrollY;
	GuiElement* scroll = &widget->Scroll.Base.Base;
	scroll->HandlesMouseScroll(scroll, delta);
	if (widget->SelectedIndex == -1) return true;

	Int32 index = widget->SelectedIndex;
	index += (widget->Scroll.ScrollY - startScrollY) * widget->ElementsPerRow;
	if (index >= widget->ElementsCount) index = -1;

	widget->SelectedIndex = index;
	TableWidget_RecreateDescTex(widget);
	return true;
}

bool TableWidget_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	TableWidget* widget = (TableWidget*)elem;
	GuiElement* scroll = &widget->Scroll.Base.Base;
	if (scroll->HandlesMouseMove(scroll, x, y)) return true;

	widget->SelectedIndex = -1;
	Widget* elemW = &widget->Base;
	Int32 blockSize = widget->BlockSize;
	Int32 maxHeight = blockSize * TABLE_MAX_ROWS_DISPLAYED;

	if (Gui_Contains(elemW->X, elemW->Y + 3, elemW->Width, maxHeight - 3 * 2, x, y)) {
		Int32 i;
		for (i = 0; i < widget->ElementsCount; i++) {
			Int32 winX, winY;
			TableWidget_GetCoords(widget, i, &winX, &winY);

			if (Gui_Contains(winX, winY, blockSize, blockSize, x, y)) {
				widget->SelectedIndex = i;
				break;
			}
		}
	}
	TableWidget_RecreateDescTex(widget);
	return true;
}

bool TableWidget_HandlesKeyDown(GuiElement* elem, Key key) {
	TableWidget* widget = (TableWidget*)elem;
	if (widget->SelectedIndex == -1) return false;

	if (key == Key_Left || key == Key_Keypad4) {
		TableWidget_ScrollRelative(widget, -1);
	} else if (key == Key_Right || key == Key_Keypad6) {
		TableWidget_ScrollRelative(widget, 1);
	} else if (key == Key_Up || key == Key_Keypad8) {
		TableWidget_ScrollRelative(widget, -widget->ElementsPerRow);
	} else if (key == Key_Down || key == Key_Keypad2) {
		TableWidget_ScrollRelative(widget, widget->ElementsPerRow);
	} else {
		return false;
	}
	return true;
}

void TableWidget_Create(TableWidget* widget) {
	Widget_Init(&widget->Base);
	widget->LastCreatedIndex = -1000;

	widget->Base.Base.Init     = TableWidget_Init;
	widget->Base.Base.Render   = TableWidget_Render;
	widget->Base.Base.Free     = TableWidget_Free;
	widget->Base.Base.Recreate = TableWidget_Recreate;
	widget->Base.Reposition    = TableWidget_Reposition;
	
	widget->Base.Base.HandlesMouseDown   = TableWidget_HandlesMouseDown;
	widget->Base.Base.HandlesMouseUp     = TableWidget_HandlesMouseUp;
	widget->Base.Base.HandlesMouseScroll = TableWidget_HandlesMouseScroll;
	widget->Base.Base.HandlesMouseMove   = TableWidget_HandlesMouseMove;
	widget->Base.Base.HandlesKeyDown     = TableWidget_HandlesKeyDown;
}

void TableWidget_SetBlockTo(TableWidget* widget, BlockID block) {
	widget->SelectedIndex = -1;
	Int32 i;
	for (i = 0; i < widget->ElementsCount; i++) {
		if (widget->Elements[i] == block) widget->SelectedIndex = i;
	}

	widget->Scroll.ScrollY = widget->SelectedIndex / widget->ElementsPerRow;
	widget->Scroll.ScrollY -= (TABLE_MAX_ROWS_DISPLAYED - 1);
	ScrollbarWidget_ClampScrollY(&widget->Scroll);
	TableWidget_MoveCursorToSelected(widget);
	TableWidget_RecreateDescTex(widget);
}

void TableWidget_OnInventoryChanged(TableWidget* widget) {
	TableWidget_RecreateElements(widget);
	if (widget->SelectedIndex >= widget->ElementsCount) {
		widget->SelectedIndex = widget->ElementsCount - 1;
	}

	widget->Scroll.ScrollY = widget->SelectedIndex / widget->ElementsPerRow;
	ScrollbarWidget_ClampScrollY(&widget->Scroll);
	TableWidget_RecreateDescTex(widget);
}


void SpecialInputWidget_UpdateColString(SpecialInputWidget* widget) {
	Int32 count = 0, i;
	for (i = ' '; i <= '~'; i++) {
		if (i >= 'A' && i <= 'F') continue;
		if (Drawer2D_Cols[i].A > 0) count++;
	}

	widget->ColString = String_FromRawBuffer(widget->ColBuffer, DRAWER2D_MAX_COLS * 4);
	String* buffer = &widget->ColString;
	Int32 index = 0;
	for (i = ' '; i <= '~'; i++) {
		if (i >= 'A' && i <= 'F') continue;
		if (Drawer2D_Cols[i].A == 0) continue;

		String_Append(buffer, '&'); String_Append(buffer, (UInt8)i);
		String_Append(buffer, '%'); String_Append(buffer, (UInt8)i);
	}
}

bool SpecialInputWidget_IntersectsHeader(SpecialInputWidget* widget, Int32 x, Int32 y) {
	Int32 titleX = 0, i;
	for (i = 0; i < Array_NumElements(widget->Tabs); i++) {
		Size2D size = widget->Tabs[i].TitleSize;
		if (Gui_Contains(titleX, 0, size.Width, size.Height, x, y)) {
			widget->SelectedIndex = i;
			return true;
		}
		titleX += size.Width;
	}
	return false;
}

void SpecialInputWidget_IntersectsBody(SpecialInputWidget* widget, Int32 x, Int32 y) {
	y -= widget->Tabs[0].TitleSize.Height;
	x /= widget->ElementSize.Width; y /= widget->ElementSize.Height;
	SpecialInputTab e = widget->Tabs[widget->SelectedIndex];
	Int32 index = y * e.ItemsPerRow + x;
	if (index * e.CharsPerItem >= e.Contents.length) return;

	SpecialInputAppendFunc append = widget->AppendFunc;
	if (widget->SelectedIndex == 0) {
		/* TODO: need to insert characters that don't affect widget->CaretPos index, adjust widget->CaretPos colour */
		append(widget->AppendObj, e.Contents.buffer[index * e.CharsPerItem]);
		append(widget->AppendObj, e.Contents.buffer[index * e.CharsPerItem + 1]);
	} else {
		append(widget->AppendObj, e.Contents.buffer[index]);
	}
}

void SpecialInputTab_Init(SpecialInputTab* tab, STRING_REF String* title,
	Int32 itemsPerRow, Int32 charsPerItem, STRING_REF String* contents) {
	tab->Title = *title;
	tab->TitleSize = Size2D_Empty;
	tab->Contents = *contents;
	tab->ItemsPerRow = itemsPerRow;
	tab->CharsPerItem = charsPerItem;
}

void SpecialInputWidget_InitTabs(SpecialInputWidget* widget) {
	String title_cols = String_FromConst("Colours");
	SpecialInputWidget_UpdateColString(widget);
	SpecialInputTab_Init(&widget->Tabs[0], &title_cols, 10, 4, &widget->ColString);

	String title_math = String_FromConst("Math");
	String tab_math = String_FromConst("\x9F\xAB\xAC\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xFB\xFC\xFD");
	SpecialInputTab_Init(&widget->Tabs[1], &title_math, 16, 1, &tab_math);

	String title_line = String_FromConst("Line/Box");
	String tab_line = String_FromConst("\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xFE");
	SpecialInputTab_Init(&widget->Tabs[2], &title_line, 17, 1, &tab_line);

	String title_letters = String_FromConst("Letters");
	String tab_letters = String_FromConst("\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\xA0\xA1\xA2\xA3\xA4\xA5");
	SpecialInputTab_Init(&widget->Tabs[3], &title_letters, 17, 1, &tab_letters);

	String title_other = String_FromConst("Other");
	String tab_other = String_FromConst("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x7F\x9B\x9C\x9D\x9E\xA6\xA7\xA8\xA9\xAA\xAD\xAE\xAF\xF9\xFA");
	SpecialInputTab_Init(&widget->Tabs[4], &title_other, 16, 1, &tab_other);
}

#define SPECIAL_TITLE_SPACING 10
#define SPECIAL_CONTENT_SPACING 5
Int32 SpecialInputWidget_MeasureTitles(SpecialInputWidget* widget) {
	Int32 totalWidth = 0;
	DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, false);

	Int32 i;
	for (i = 0; i < Array_NumElements(widget->Tabs); i++) {
		args.Text = widget->Tabs[i].Title;
		widget->Tabs[i].TitleSize = Drawer2D_MeasureText(&args);
		widget->Tabs[i].TitleSize.Width += SPECIAL_TITLE_SPACING;
		totalWidth += widget->Tabs[i].TitleSize.Width;
	}
	return totalWidth;
}

void SpecialInputWidget_DrawTitles(SpecialInputWidget* widget, Bitmap* bmp) {
	Int32 x = 0;
	DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, false);

	Int32 i;
	PackedCol col_selected = PACKEDCOL_CONST(30, 30, 30, 200);
	PackedCol col_inactive = PACKEDCOL_CONST( 0,  0,  0, 127);
	for (i = 0; i < Array_NumElements(widget->Tabs); i++) {
		args.Text = widget->Tabs[i].Title;
		PackedCol col = i == widget->SelectedIndex ? col_selected : col_inactive;
		Size2D size = widget->Tabs[i].TitleSize;

		Drawer2D_Clear(bmp, col, x, 0, size.Width, size.Height);
		Drawer2D_DrawText(&args, x + SPECIAL_TITLE_SPACING / 2, 0);
		x += size.Width;
	}
}

Size2D SpecialInputWidget_CalculateContentSize(SpecialInputTab* e, Size2D* sizes, Size2D* elemSize) {
	*elemSize = Size2D_Empty;
	Int32 i;
	for (i = 0; i < e->Contents.length; i += e->CharsPerItem) {
		elemSize->Width = max(elemSize->Width, sizes[i / e->CharsPerItem].Width);
	}

	elemSize->Width += SPECIAL_CONTENT_SPACING;
	elemSize->Height = sizes[0].Height + SPECIAL_CONTENT_SPACING;
	Int32 rows = Math_CeilDiv(e->Contents.length / e->CharsPerItem, e->ItemsPerRow);
	return Size2D_Make(elemSize->Width * e->ItemsPerRow, elemSize->Height * rows);
}

void SpecialInputWidget_MeasureContentSizes(SpecialInputWidget* widget, SpecialInputTab* e, Size2D* sizes) {
	UInt8 buffer[String_BufferSize(STRING_SIZE)];
	String s = String_FromRawBuffer(buffer, e->CharsPerItem);
	DrawTextArgs args; DrawTextArgs_Make(&args, &s, &widget->Font, false);

	Int32 i, j;
	for (i = 0; i < e->Contents.length; i += e->CharsPerItem) {
		for (j = 0; j < e->CharsPerItem; j++) {
			s.buffer[j] = e->Contents.buffer[i + j];
		}
		sizes[i / e->CharsPerItem] = Drawer2D_MeasureText(&args);
	}
}

void SpecialInputWidget_DrawContent(SpecialInputWidget* widget, SpecialInputTab* e, Int32 yOffset) {
	UInt8 buffer[String_BufferSize(STRING_SIZE)];
	String s = String_FromRawBuffer(buffer, e->CharsPerItem);
	DrawTextArgs args; DrawTextArgs_Make(&args, &s, &widget->Font, false);

	Int32 i, j, wrap = e->ItemsPerRow;
	for (i = 0; i < e->Contents.length; i += e->CharsPerItem) {
		for (j = 0; j < e->CharsPerItem; j++) {
			s.buffer[j] = e->Contents.buffer[i + j];
		}

		Int32 item = i / e->CharsPerItem;
		Int32 x = (item % wrap) * widget->ElementSize.Width;
		Int32 y = (item / wrap) * widget->ElementSize.Height + yOffset;
		Drawer2D_DrawText(&args, x, y);
	}
}

void SpecialInputWidget_Make(SpecialInputWidget* widget, SpecialInputTab* e) {
	Size2D sizes[DRAWER2D_MAX_COLS];
	SpecialInputWidget_MeasureContentSizes(widget, e, sizes);
	Size2D bodySize = SpecialInputWidget_CalculateContentSize(e, sizes, &widget->ElementSize);
	Int32 titleWidth = SpecialInputWidget_MeasureTitles(widget);
	Int32 titleHeight = widget->Tabs[0].TitleSize.Height;
	Size2D size = Size2D_Make(max(bodySize.Width, titleWidth), bodySize.Height + titleHeight);
	Gfx_DeleteTexture(&widget->Tex.ID);

	Bitmap bmp;
	Bitmap_AllocatePow2(&bmp, size.Width, size.Height);
	Drawer2D_Begin(&bmp);

	SpecialInputWidget_DrawTitles(widget, &bmp);
	PackedCol col = PACKEDCOL_CONST(30, 30, 30, 200);
	Drawer2D_Clear(&bmp, col, 0, titleHeight, size.Width, bodySize.Height);
	SpecialInputWidget_DrawContent(widget, e, titleHeight);
	widget->Tex = Drawer2D_Make2DTexture(&bmp, size, widget->Base.X, widget->Base.Y);

	Drawer2D_End();
}

void SpecialInputWidget_Redraw(SpecialInputWidget* widget) {
	SpecialInputWidget_Make(widget, &widget->Tabs[widget->SelectedIndex]);
	widget->Base.Width = widget->Tex.Width;
	widget->Base.Height = widget->Tex.Height;
}

void SpecialInputWidget_Init(GuiElement* elem) {
	SpecialInputWidget* widget = (SpecialInputWidget*)elem;
	widget->Base.X = 5; widget->Base.Y = 5;
	SpecialInputWidget_InitTabs(widget);
	SpecialInputWidget_Redraw(widget);
	SpecialInputWidget_SetActive(widget, widget->Base.Active);
}

void SpecialInputWidget_Render(GuiElement* elem, Real64 delta) {
	SpecialInputWidget* widget = (SpecialInputWidget*)elem;
	Texture_Render(&widget->Tex);
}

void SpecialInputWidget_Free(GuiElement* elem) {
	SpecialInputWidget* widget = (SpecialInputWidget*)elem;
	Gfx_DeleteTexture(&widget->Tex.ID);
}

bool SpecialInputWidget_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	SpecialInputWidget* widget = (SpecialInputWidget*)elem;
	x -= widget->Base.X; y -= widget->Base.Y;

	if (SpecialInputWidget_IntersectsHeader(widget, x, y)) {
		SpecialInputWidget_Redraw(widget);
	} else {
		SpecialInputWidget_IntersectsBody(widget, x, y);
	}
	return true;
}

void SpecialInputWidget_UpdateCols(SpecialInputWidget* widget) {
	SpecialInputWidget_UpdateColString(widget);
	widget->Tabs[0].Contents = widget->ColString;
	SpecialInputWidget_Redraw(widget);
	SpecialInputWidget_SetActive(widget, widget->Base.Active);
}

void SpecialInputWidget_SetActive(SpecialInputWidget* widget, bool active) {
	widget->Base.Active = active;
	widget->Base.Height = active ? widget->Tex.Height : 0;
}

void SpecialInputWidget_Create(SpecialInputWidget* widget, FontDesc* font, SpecialInputAppendFunc appendFunc, void* appendObj) {
	Widget_Init(&widget->Base);
	widget->Base.VerAnchor = ANCHOR_BOTTOM_OR_RIGHT;
	widget->Font = *font;
	widget->AppendFunc = appendFunc;
	widget->AppendObj = appendObj;

	widget->Base.Base.Init   = SpecialInputWidget_Init;
	widget->Base.Base.Render = SpecialInputWidget_Render;
	widget->Base.Base.Free   = SpecialInputWidget_Free;
	widget->Base.Base.HandlesMouseDown   = SpecialInputWidget_HandlesMouseDown;
}


void InputWidget_CalculateLineSizes(InputWidget* widget) {
	Int32 y;
	for (y = 0; y < INPUTWIDGET_MAX_LINES; y++) {
		widget->LineSizes[y] = Size2D_Empty;
	}
	widget->LineSizes[0].Width = widget->PrefixWidth;

	DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, true);
	for (y = 0; y < widget->GetMaxLines(); y++) {
		args.Text = widget->Lines[y];
		Size2D textSize = Drawer2D_MeasureText(&args);
		widget->LineSizes[y].Width += textSize.Width;
		widget->LineSizes[y].Height = textSize.Height;
	}

	if (widget->LineSizes[0].Height == 0) {
		widget->LineSizes[0].Height = widget->PrefixHeight;
	}
}

UInt8 InputWidget_GetLastCol(InputWidget* widget, Int32 indexX, Int32 indexY) {
	Int32 x = indexX, y;
	for (y = indexY; y >= 0; y--) {
		UInt8 code = Drawer2D_LastCol(&widget->Lines[y], x);
		if (code != NULL) return code;
		if (y > 0) { x = widget->Lines[y - 1].length; }
	}
	return NULL;
}

void InputWidget_UpdateCaret(InputWidget* widget) {
	Int32 maxChars = widget->GetMaxLines() * widget->MaxCharsPerLine;
	if (widget->CaretPos >= maxChars) widget->CaretPos = -1;
	WordWrap_GetCoords(widget->CaretPos, widget->Lines, widget->GetMaxLines(), &widget->CaretX, &widget->CaretY);
	DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, false);
	widget->CaretAccumulator = 0;

	/* Caret is at last character on line */
	Widget* elem = &widget->Base;
	if (widget->CaretX == widget->MaxCharsPerLine) {
		widget->CaretTex.X = elem->X + widget->Padding + widget->LineSizes[widget->CaretY].Width;
		PackedCol yellow = PACKEDCOL_YELLOW; widget->CaretCol = yellow;
		widget->CaretTex.Width = widget->CaretWidth;
	} else {
		String* line = &widget->Lines[widget->CaretY];
		args.Text = String_UNSAFE_Substring(line, 0, widget->CaretX);
		Size2D trimmedSize = Drawer2D_MeasureText(&args);
		if (widget->CaretY == 0) { trimmedSize.Width += widget->PrefixWidth; }

		widget->CaretTex.X = elem->X + widget->Padding + trimmedSize.Width;
		PackedCol white = PACKEDCOL_WHITE;
		widget->CaretCol = PackedCol_Scale(white, 0.8f);

		if (widget->CaretX < line->length) {
			args.Text = String_UNSAFE_Substring(line, widget->CaretX, 1);
			args.UseShadow = true;
			widget->CaretTex.Width = (UInt16)Drawer2D_MeasureText(&args).Width;
		} else {
			widget->CaretTex.Width = widget->CaretWidth;
		}
	}
	widget->CaretTex.Y = widget->LineSizes[0].Height * widget->CaretY + widget->InputTex.Y + 2;

	/* Update the colour of the widget->CaretPos */
	UInt8 code = InputWidget_GetLastCol(widget, widget->CaretX, widget->CaretY);
	if (code != NULL) widget->CaretCol = Drawer2D_Cols[code];
}

void InputWidget_RenderCaret(InputWidget* widget, Real64 delta) {
	if (!widget->ShowCaret) return;

	widget->CaretAccumulator += delta;
	Real64 second = widget->CaretAccumulator - (Real64)Math_Floor((Real32)widget->CaretAccumulator);
	if (second < 0.5) {
		GfxCommon_Draw2DTexture(&widget->CaretTex, widget->CaretCol);
	}
}

void InputWidget_RemakeTexture(InputWidget* widget) {
	Int32 totalHeight = 0, maxWidth = 0, i;
	for (i = 0; i < widget->GetMaxLines(); i++) {
		totalHeight += widget->LineSizes[i].Height;
		maxWidth = max(maxWidth, widget->LineSizes[i].Width);
	}
	Size2D size = Size2D_Make(maxWidth, totalHeight);
	widget->CaretAccumulator = 0;

	Int32 realHeight = 0;
	Bitmap bmp; Bitmap_AllocatePow2(&bmp, size.Width, size.Height);
	Drawer2D_Begin(&bmp);

	DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, true);
	if (widget->Prefix.length > 0) {
		args.Text = widget->Prefix;
		Drawer2D_DrawText(&args, 0, 0);
	}

	UInt8 tmpBuffer[String_BufferSize(STRING_SIZE + 2)];
	for (i = 0; i < Array_NumElements(widget->Lines); i++) {
		if (widget->Lines[i].length == 0) break;
		args.Text = widget->Lines[i];
		UInt8 lastCol = InputWidget_GetLastCol(widget, 0, i);

		/* Colour code goes to next line */
		if (!Drawer2D_IsWhiteCol(lastCol)) {
			String tmp = String_FromRawBuffer(tmpBuffer, STRING_SIZE + 2);
			String_Append(&tmp, '&'); String_Append(&tmp, lastCol);
			String_AppendString(&tmp, &args.Text);
			args.Text = tmp;
		}

		Int32 offset = i == 0 ? widget->PrefixWidth : 0;
		Drawer2D_DrawText(&args, offset, realHeight);
		realHeight += widget->LineSizes[i].Height;
	}
	widget->InputTex = Drawer2D_Make2DTexture(&bmp, size, 0, 0);

	Drawer2D_End();
	Platform_MemFree(bmp.Scan0);

	Widget* elem = &widget->Base;
	elem->Width = size.Width;
	elem->Height = realHeight == 0 ? widget->PrefixHeight : realHeight;
	elem->Reposition(elem);
	widget->InputTex.X = elem->X + widget->Padding;
	widget->InputTex.Y = elem->Y;
}

void InputWidget_EnterInput(InputWidget* widget) {
	InputWidget_Clear(widget);
	widget->Base.Height = widget->PrefixHeight;
}

void InputWidget_Clear(InputWidget* widget) {
	String_Clear(&widget->Text);
	Int32 i;
	for (i = 0; i < Array_NumElements(widget->Lines); i++) {
		String_Clear(&widget->Lines[i]);
	}

	widget->CaretPos = -1;
	Gfx_DeleteTexture(&widget->InputTex.ID);
}

bool InputWidget_AllowedChar(InputWidget* widget, UInt8 c) {
	return Utils_IsValidInputChar(c, ServerConnection_SupportsFullCP437);
}

void InputWidget_AppendChar(InputWidget* widget, UInt8 c) {
	if (widget->CaretPos == -1) {
		String_InsertAt(&widget->Text, widget->Text.length, c);
	} else {
		String_InsertAt(&widget->Text, widget->CaretPos, c);
		widget->CaretPos++;
		if (widget->CaretPos >= widget->Text.length) { widget->CaretPos = -1; }
	}
}

bool InputWidget_TryAppendChar(InputWidget* widget, UInt8 c) {
	Int32 maxChars = widget->GetMaxLines() * widget->MaxCharsPerLine;
	if (widget->Text.length >= maxChars) return false;
	if (!InputWidget_AllowedChar(widget, c)) return false;

	InputWidget_AppendChar(widget, c);
	return true;
}

void InputWidget_AppendString(InputWidget* widget, STRING_PURE String* text) {
	Int32 appended = 0, i;
	for (i = 0; i < text->length; i++) {
		if (InputWidget_TryAppendChar(widget, text->buffer[i])) appended++;
	}

	if (appended == 0) return;
	GuiElement* elem = &widget->Base.Base;
	elem->Recreate(elem);
}

void InputWidget_Append(InputWidget* widget, UInt8 c) {
	if (!InputWidget_TryAppendChar(widget, c)) return;
	GuiElement* elem = &widget->Base.Base;
	elem->Recreate(elem);
}

void InputWidget_DeleteChar(InputWidget* widget) {
	if (widget->Text.length == 0) return;

	if (widget->CaretPos == -1) {
		String_DeleteAt(&widget->Text, widget->Text.length - 1);
	} else if (widget->CaretPos > 0) {
		widget->CaretPos--;
		String_DeleteAt(&widget->Text, widget->CaretPos);
	}
}

bool InputWidget_CheckCol(InputWidget* widget, Int32 index) {
	if (index < 0) return false;
	UInt8 code = widget->Text.buffer[index];
	UInt8 col = widget->Text.buffer[index + 1];
	return (code == '%' || code == '&') && Drawer2D_ValidColCode(col);
}

void InputWidget_BackspaceKey(InputWidget* widget, bool controlDown) {
	if (controlDown) {
		if (widget->CaretPos == -1) { widget->CaretPos = widget->Text.length - 1; }
		Int32 len = WordWrap_GetBackLength(&widget->Text, widget->CaretPos);
		if (len == 0) return;

		widget->CaretPos -= len;
		if (widget->CaretPos < 0) { widget->CaretPos = 0; }
		Int32 i;
		for (i = 0; i <= len; i++) {
			String_DeleteAt(&widget->Text, widget->CaretPos);
		}

		if (widget->CaretPos >= widget->Text.length) { widget->CaretPos = -1; }
		if (widget->CaretPos == -1 && widget->Text.length > 0) {
			String_InsertAt(&widget->Text, widget->Text.length, ' ');
		} else if (widget->CaretPos >= 0 && widget->Text.buffer[widget->CaretPos] != ' ') {
			String_InsertAt(&widget->Text, widget->CaretPos, ' ');
		}
		GuiElement* elem = &widget->Base.Base;
		elem->Recreate(elem);
	} else if (widget->Text.length > 0 && widget->CaretPos != 0) {
		Int32 index = widget->CaretPos == -1 ? widget->Text.length - 1 : widget->CaretPos;
		if (InputWidget_CheckCol(widget, index - 1)) {
			InputWidget_DeleteChar(widget); /* backspace XYZ%e to XYZ */
		} else if (InputWidget_CheckCol(widget, index - 2)) {
			InputWidget_DeleteChar(widget); 
			InputWidget_DeleteChar(widget); /* backspace XYZ%eH to XYZ */
		}

		InputWidget_DeleteChar(widget);
		GuiElement* elem = &widget->Base.Base;
		elem->Recreate(elem);
	}
}

void InputWidget_DeleteKey(InputWidget* widget) {
	if (widget->Text.length > 0 && widget->CaretPos != -1) {
		String_DeleteAt(&widget->Text, widget->CaretPos);
		if (widget->CaretPos >= widget->Text.length) { widget->CaretPos = -1; }
		GuiElement* elem = &widget->Base.Base;
		elem->Recreate(elem);
	}
}

void InputWidget_LeftKey(InputWidget* widget, bool controlDown) {
	if (controlDown) {
		if (widget->CaretPos == -1) { widget->CaretPos = widget->Text.length - 1; }
		widget->CaretPos -= WordWrap_GetBackLength(&widget->Text, widget->CaretPos);
		InputWidget_UpdateCaret(widget);
		return;
	}

	if (widget->Text.length > 0) {
		if (widget->CaretPos == -1) { widget->CaretPos = widget->Text.length; }
		widget->CaretPos--;
		if (widget->CaretPos < 0) { widget->CaretPos = 0; }
		InputWidget_UpdateCaret(widget);
	}
}

void InputWidget_RightKey(InputWidget* widget, bool controlDown) {
	if (controlDown) {
		widget->CaretPos += WordWrap_GetForwardLength(&widget->Text, widget->CaretPos);
		if (widget->CaretPos >= widget->Text.length) { widget->CaretPos = -1; }
		InputWidget_UpdateCaret(widget);
		return;
	}

	if (widget->Text.length > 0 && widget->CaretPos != -1) {
		widget->CaretPos++;
		if (widget->CaretPos >= widget->Text.length) { widget->CaretPos = -1; }
		InputWidget_UpdateCaret(widget);
	}
}

void InputWidget_HomeKey(InputWidget* widget) {
	if (widget->Text.length == 0) return;
	widget->CaretPos = 0;
	InputWidget_UpdateCaret(widget);
}

void InputWidget_EndKey(InputWidget* widget) {
	widget->CaretPos = -1;
	InputWidget_UpdateCaret(widget);
}

bool InputWidget_OtherKey(InputWidget* widget, Key key) {
	Int32 maxChars = widget->GetMaxLines() * widget->MaxCharsPerLine;
	if (key == Key_V && widget->Text.length < maxChars) {
		UInt8 textBuffer[String_BufferSize(INPUTWIDGET_MAX_LINES * STRING_SIZE)];
		String text = String_FromRawBuffer(textBuffer, INPUTWIDGET_MAX_LINES * STRING_SIZE);
		Window_GetClipboardText(&text);

		if (text.length == 0) return true;
		InputWidget_AppendString(widget, &text);
		return true;
	} else if (key == Key_C) {
		if (widget->Text.length == 0) return true;
		Window_SetClipboardText(&widget->Text);
		return true;
	}
	return false;
}

void InputWidget_Init(GuiElement* elem) {
	InputWidget* widget = (InputWidget*)elem;
	Int32 lines = widget->GetMaxLines();
	if (lines > 1) {
		WordWrap_Do(&widget->Text, widget->Lines, lines, widget->MaxCharsPerLine);
	} else {
		String_Clear(&widget->Lines[0]);
		String_AppendString(&widget->Lines[0], &widget->Text);
	}

	InputWidget_CalculateLineSizes(widget);
	InputWidget_RemakeTexture(widget);
	InputWidget_UpdateCaret(widget);
}

void InputWidget_Free(GuiElement* elem) {
	InputWidget* widget = (InputWidget*)elem;
	Gfx_DeleteTexture(&widget->InputTex.ID);
	Gfx_DeleteTexture(&widget->CaretTex.ID);
	Gfx_DeleteTexture(&widget->PrefixTex.ID);
}

void InputWidget_Recreate(GuiElement* elem) {
	InputWidget* widget = (InputWidget*)elem;
	Gfx_DeleteTexture(&widget->InputTex.ID);
	InputWidget_Init(elem);
}

void InputWidget_Reposition(Widget* elem) {
	Int32 oldX = elem->X, oldY = elem->Y;
	Widget_DoReposition(elem);

	InputWidget* widget = (InputWidget*)elem;
	widget->CaretTex.X += elem->X - oldX; widget->CaretTex.Y += elem->Y - oldY;
	widget->InputTex.X += elem->X - oldX; widget->InputTex.Y += elem->Y - oldY;
}

bool InputWidget_HandlesKeyDown(GuiElement* elem, Key key) {
#if CC_BUILD_OSX
	bool clipboardDown = Key_IsWinPressed();
#else
	bool clipboardDown = Key_IsControlPressed();
#endif
	InputWidget* widget = (InputWidget*)elem;

	if (key == Key_Left) {
		InputWidget_LeftKey(widget, clipboardDown);
	} else if (key == Key_Right) {
		InputWidget_RightKey(widget, clipboardDown);
	} else if (key == Key_BackSpace) {
		InputWidget_BackspaceKey(widget, clipboardDown);
	} else if (key == Key_Delete) {
		InputWidget_DeleteKey(widget);
	} else if (key == Key_Home) {
		InputWidget_HomeKey(widget);
	} else if (key == Key_End) {
		InputWidget_EndKey(widget);
	} else if (clipboardDown && !InputWidget_OtherKey(widget, key)) {
		return false;
	}
	return true;
}

bool InputWidget_HandlesKeyUp(GuiElement* elem, Key key) { return true; }

bool InputWidget_HandlesKeyPress(GuiElement* elem, UInt8 key) {
	InputWidget* widget = (InputWidget*)elem;
	InputWidget_AppendChar(widget, key);
	return true;
}

bool InputWidget_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton button) {
	InputWidget* widget = (InputWidget*)elem;
	if (button == MouseButton_Left) {
		x -= widget->InputTex.X; y -= widget->InputTex.Y;
		DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, true);
		Int32 offset = 0, charHeight = widget->CaretHeight;

		Int32 charX, i;
		for (i = 0; i < widget->GetMaxLines(); i++) {
			String* line = &widget->Lines[i];
			Int32 xOffset = i == 0 ? widget->PrefixWidth : 0;
			if (line->length == 0) continue;

			for (charX = 0; charX < line->length; charX++) {
				args.Text = String_UNSAFE_Substring(line, 0, charX);
				Int32 charOffset = Drawer2D_MeasureText(&args).Width + xOffset;

				args.Text = String_UNSAFE_Substring(line, charX, 1);
				Int32 charWidth = Drawer2D_MeasureText(&args).Width;

				if (Gui_Contains(charOffset, i * charHeight, charWidth, charHeight, x, y)) {
					widget->CaretPos = offset + charX;
					InputWidget_UpdateCaret(widget);
					return true;
				}
			}
			offset += line->length;
		}
		widget->CaretPos = -1;
		InputWidget_UpdateCaret(widget);
	}
	return true;
}

void InputWidget_Create(InputWidget* widget, FontDesc* font, STRING_REF String* prefix) {
	Widget_Init(&widget->Base);
	widget->Font = *font;
	widget->Prefix = *prefix;
	widget->CaretPos = -1;
	widget->MaxCharsPerLine = STRING_SIZE;

	widget->Base.Base.Init     = InputWidget_Init;
	widget->Base.Base.Free     = InputWidget_Free;
	widget->Base.Base.Recreate = InputWidget_Recreate;
	widget->Base.Reposition    = InputWidget_Reposition;

	widget->Base.Base.HandlesKeyDown   = InputWidget_HandlesKeyDown;
	widget->Base.Base.HandlesKeyUp     = InputWidget_HandlesKeyUp;
	widget->Base.Base.HandlesKeyPress  = InputWidget_HandlesKeyPress;
	widget->Base.Base.HandlesMouseDown = InputWidget_HandlesMouseDown;

	String caret = String_FromConst("_");
	DrawTextArgs args; DrawTextArgs_Make(&args, &caret, font, true);
	widget->CaretTex = Drawer2D_MakeTextTexture(&args, 0, 0);
	widget->CaretTex.Width = (UInt16)((widget->CaretTex.Width * 3) / 4);
	widget->CaretWidth  = (UInt16)widget->CaretTex.Width;
	widget->CaretHeight = (UInt16)widget->CaretTex.Height;

	if (prefix->length == 0) return;
	DrawTextArgs_Make(&args, prefix, font, true);
	Size2D size = Drawer2D_MeasureText(&args);
	widget->PrefixWidth  = (UInt16)size.Width;  widget->Base.Width  = size.Width;
	widget->PrefixHeight = (UInt16)size.Height; widget->Base.Height = size.Height;
}