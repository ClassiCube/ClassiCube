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
#include "Event.h"
#include "Chat.h"

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
	if (widget->Texture.ID != NULL) {
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

void TextWidget_Make(TextWidget* widget, FontDesc* font) {
	Widget_Init(&widget->Base);
	PackedCol col = PACKEDCOL_WHITE;
	widget->Col = col;
	widget->Font = *font;
	widget->Base.Reposition  = TextWidget_Reposition;
	widget->Base.Base.Init   = TextWidget_Init;
	widget->Base.Base.Render = TextWidget_Render;
	widget->Base.Base.Free   = TextWidget_Free;
}

void TextWidget_Create(TextWidget* widget, STRING_PURE String* text, FontDesc* font) {
	TextWidget_Make(widget, font);
	GuiElement* elem = &widget->Base.Base;
	elem->Init(elem);
	TextWidget_SetText(widget, text);
}

void TextWidget_SetText(TextWidget* widget, STRING_PURE String* text) {
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
	if (widget->Texture.ID == NULL) return;
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
		if (KeyBind_IsPressed(KeyBind_HotbarSwitching)) {
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
	if (screen != InventoryScreen_UNSAFE_RawPointer) return false;

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

void TableWidget_RecreateDescTex(TableWidget* widget) {
	if (widget->SelectedIndex == widget->LastCreatedIndex) return;
	if (widget->ElementsCount == 0) return;
	widget->LastCreatedIndex = widget->SelectedIndex;

	Gfx_DeleteTexture(&widget->DescTex.ID);
	if (widget->SelectedIndex == -1) return;

	UInt8 descBuffer[String_BufferSize(STRING_SIZE * 2)];
	String desc = String_InitAndClearArray(descBuffer);
	BlockID block = widget->Elements[widget->SelectedIndex];
	TableWidget_MakeBlockDesc(&desc, block);

	DrawTextArgs args;
	DrawTextArgs_Make(&args, &desc, &widget->Font, true);
	widget->DescTex = Drawer2D_MakeTextTexture(&args, 0, 0);
	TableWidget_UpdateDescTexPos(widget);
}

bool TableWidget_Show(BlockID block) {
	if (block == BLOCK_AIR) return false;

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
	/* These were sourced by taking a screenshot of vanilla
	Then using paint to extract the colour components
	Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topBackCol    = PACKEDCOL_CONST( 34,  34,  34, 168);
	PackedCol bottomBackCol = PACKEDCOL_CONST( 57,  57, 104, 202);
	PackedCol topSelCol     = PACKEDCOL_CONST(255, 255, 255, 142);
	PackedCol bottomSelCol  = PACKEDCOL_CONST(255, 255, 255, 192);

	TableWidget* widget = (TableWidget*)elem;
	GfxCommon_Draw2DGradient(Table_X(widget), Table_Y(widget),
		Table_Width(widget), Table_Height(widget), topBackCol, bottomBackCol);

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
			size, size, topSelCol, bottomSelCol);
	}
	Gfx_SetTexturing(true);
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);

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

	if (widget->DescTex.ID != NULL) {
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
	widget->VB = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, TABLE_MAX_VERTICES);
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
	UInt32 count = 0, i;
	for (i = 0; i < DRAWER2D_MAX_COLS; i++) {
		if (i >= 'A' && i <= 'F') continue;
		if (Drawer2D_Cols[i].A > 0) count++;
	}

	widget->ColString = String_InitAndClearArray(widget->ColBuffer);
	String* buffer = &widget->ColString;
	for (i = 0; i < DRAWER2D_MAX_COLS; i++) {
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
	String s = String_InitAndClear(buffer, e->CharsPerItem);
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
	String s = String_InitAndClear(buffer, e->CharsPerItem);
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


bool InputWidget_ControlDown(void) {
#if CC_BUILD_OSX
	return Key_IsWinPressed();
#else
	return Key_IsControlPressed();
#endif
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
	Int32 maxChars = widget->GetMaxLines() * INPUTWIDGET_LEN;
	if (widget->CaretPos >= maxChars) widget->CaretPos = -1;
	WordWrap_GetCoords(widget->CaretPos, widget->Lines, widget->GetMaxLines(), &widget->CaretX, &widget->CaretY);
	DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, false);
	widget->CaretAccumulator = 0;

	/* Caret is at last character on line */
	Widget* elem = &widget->Base;
	if (widget->CaretX == INPUTWIDGET_LEN) {
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

void InputWidget_RemakeTexture(GuiElement* elem) {
	InputWidget* widget = (InputWidget*)elem;
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
			String tmp = String_InitAndClearArray(tmpBuffer);
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

	Widget* elemW = &widget->Base;
	elemW->Width = size.Width;
	elemW->Height = realHeight == 0 ? widget->PrefixHeight : realHeight;
	elemW->Reposition(elemW);
	widget->InputTex.X = elemW->X + widget->Padding;
	widget->InputTex.Y = elemW->Y;
}

void InputWidget_OnPressedEnter(GuiElement* elem) {
	InputWidget* widget = (InputWidget*)elem;
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

bool InputWidget_AllowedChar(GuiElement* elem, UInt8 c) {
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
	Int32 maxChars = widget->GetMaxLines() * INPUTWIDGET_LEN;
	if (widget->Text.length >= maxChars) return false;
	if (!widget->AllowedChar(&widget->Base.Base, c)) return false;

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

void InputWidget_BackspaceKey(InputWidget* widget) {
	if (InputWidget_ControlDown()) {
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

void InputWidget_LeftKey(InputWidget* widget) {
	if (InputWidget_ControlDown()) {
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

void InputWidget_RightKey(InputWidget* widget) {
	if (InputWidget_ControlDown()) {
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
	Int32 maxChars = widget->GetMaxLines() * INPUTWIDGET_LEN;
	if (key == Key_V && widget->Text.length < maxChars) {
		UInt8 textBuffer[String_BufferSize(INPUTWIDGET_MAX_LINES * STRING_SIZE)];
		String text = String_InitAndClearArray(textBuffer);
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
		/* TODO: Actually make this work */
		WordWrap_Do(&widget->Text, widget->Lines, lines, INPUTWIDGET_LEN);
	} else {
		String_Clear(&widget->Lines[0]);
		String_AppendString(&widget->Lines[0], &widget->Text);
	}

	InputWidget_CalculateLineSizes(widget);
	InputWidget_RemakeTexture(elem);
	InputWidget_UpdateCaret(widget);
}

void InputWidget_Free(GuiElement* elem) {
	InputWidget* widget = (InputWidget*)elem;
	Gfx_DeleteTexture(&widget->InputTex.ID);
	Gfx_DeleteTexture(&widget->CaretTex.ID);
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
	InputWidget* widget = (InputWidget*)elem;

	if (key == Key_Left) {
		InputWidget_LeftKey(widget);
	} else if (key == Key_Right) {
		InputWidget_RightKey(widget);
	} else if (key == Key_BackSpace) {
		InputWidget_BackspaceKey(widget);
	} else if (key == Key_Delete) {
		InputWidget_DeleteKey(widget);
	} else if (key == Key_Home) {
		InputWidget_HomeKey(widget);
	} else if (key == Key_End) {
		InputWidget_EndKey(widget);
	} else if (InputWidget_ControlDown() && !InputWidget_OtherKey(widget, key)) {
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
		Int32 offset = 0, charHeight = widget->CaretTex.Height;

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
	widget->Font            = *font;
	widget->Prefix          = *prefix;
	widget->CaretPos        = -1;
	widget->RemakeTexture   = InputWidget_RemakeTexture;
	widget->OnPressedEnter  = InputWidget_OnPressedEnter;
	widget->AllowedChar     = InputWidget_AllowedChar;	

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
	widget->CaretWidth     = (UInt16)widget->CaretTex.Width;

	if (prefix->length == 0) return;
	DrawTextArgs_Make(&args, prefix, font, true);
	Size2D size = Drawer2D_MeasureText(&args);
	widget->PrefixWidth  = (UInt16)size.Width;  widget->Base.Width  = size.Width;
	widget->PrefixHeight = (UInt16)size.Height; widget->Base.Height = size.Height;
}


bool MenuInputValidator_AlwaysValidChar(MenuInputValidator* validator, UInt8 c) { return true; }
bool MenuInputValidator_AlwaysValidString(MenuInputValidator* validator, STRING_PURE String* s) { return true; }

void HexColourValidator_GetRange(MenuInputValidator* validator, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(#000000 - #FFFFFF)");
}

bool HexColourValidator_IsValidChar(MenuInputValidator* validator, UInt8 c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

bool HexColourValidator_IsValidString(MenuInputValidator* validator, STRING_PURE String* s) {
	return s->length <= 6;
}

bool HexColourValidator_IsValidValue(MenuInputValidator* validator, STRING_PURE String* s) {
	PackedCol col;
	return PackedCol_TryParseHex(s, &col);
}

MenuInputValidator MenuInputValidator_Hex(void) {
	MenuInputValidator validator;
	validator.GetRange      = HexColourValidator_GetRange;
	validator.IsValidChar   = HexColourValidator_IsValidChar;
	validator.IsValidString = HexColourValidator_IsValidString;
	validator.IsValidValue  = HexColourValidator_IsValidValue;
	return validator;
}

void IntegerValidator_GetRange(MenuInputValidator* validator, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(");
	String_AppendInt32(range, validator->Meta_Int[0]);
	String_AppendConst(range, " - ");
	String_AppendInt32(range, validator->Meta_Int[1]);
	String_AppendConst(range, ")");
}

bool IntegerValidator_IsValidChar(MenuInputValidator* validator, UInt8 c) {
	return (c >= '0' && c <= '9') || c == '-';
}

bool IntegerValidator_IsValidString(MenuInputValidator* validator, STRING_PURE String* s) {
	Int32 value;
	if (s->length == 1 && s->buffer[0] == '-') return true; /* input is just a minus sign */
	return Convert_TryParseInt32(s, &value);
}

bool IntegerValidator_IsValidValue(MenuInputValidator* validator, STRING_PURE String* s) {
	Int32 value;
	if (!Convert_TryParseInt32(s, &value)) return false;

	Int32 min = validator->Meta_Int[0], max = validator->Meta_Int[1];
	return min <= value && value <= max;
}

MenuInputValidator MenuInputValidator_Integer(Int32 min, Int32 max) {
	MenuInputValidator validator;
	validator.GetRange      = IntegerValidator_GetRange;
	validator.IsValidChar   = IntegerValidator_IsValidChar;
	validator.IsValidString = IntegerValidator_IsValidString;
	validator.IsValidValue  = IntegerValidator_IsValidValue;

	validator.Meta_Int[0] = min;
	validator.Meta_Int[1] = max;
	return validator;
}

void SeedValidator_GetRange(MenuInputValidator* validator, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(an integer)");
}

MenuInputValidator MenuInputValidator_Seed(void) {
	MenuInputValidator validator = MenuInputValidator_Integer(Int32_MinValue, Int32_MaxValue);
	validator.GetRange = SeedValidator_GetRange;
	return validator;
}

void RealValidator_GetRange(MenuInputValidator* validator, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(");
	String_AppendReal32(range, validator->Meta_Real[0]);
	String_AppendConst(range, " - ");
	String_AppendReal32(range, validator->Meta_Real[1]);
	String_AppendConst(range, ")");
}

bool RealValidator_IsValidChar(MenuInputValidator* validator, UInt8 c) {
	return (c >= '0' && c <= '9') || c == '-' || c == '.' || c == ',';
}

bool RealValidator_IsValidString(MenuInputValidator* validator, STRING_PURE String* s) {
	Real32 value;
	if (s->length == 1 && RealValidator_IsValidChar(validator, s->buffer[0])) return true;
	return Convert_TryParseReal32(s, &value);
}

bool RealValidator_IsValidValue(MenuInputValidator* validator, STRING_PURE String* s) {
	Real32 value;
	if (!Convert_TryParseReal32(s, &value)) return false;
	Real32 min = validator->Meta_Real[0], max = validator->Meta_Real[1];
	return min <= value && value <= max;
}

MenuInputValidator MenuInputValidator_Real(Real32 min, Real32 max) {
	MenuInputValidator validator;
	validator.GetRange      = RealValidator_GetRange;
	validator.IsValidChar   = RealValidator_IsValidChar;
	validator.IsValidString = RealValidator_IsValidString;
	validator.IsValidValue  = RealValidator_IsValidValue;
	validator.Meta_Real[0] = min;
	validator.Meta_Real[1] = max;
	return validator;
}

void PathValidator_GetRange(MenuInputValidator* validator, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(Enter name)");
}

bool PathValidator_IsValidChar(MenuInputValidator* validator, UInt8 c) {
	return !(c == '/' || c == '\\' || c == '?' || c == '*' || c == ':'
		|| c == '<' || c == '>' || c == '|' || c == '"' || c == '.');
}

MenuInputValidator MenuInputValidator_Path(void) {
	MenuInputValidator validator;
	validator.GetRange      = PathValidator_GetRange;
	validator.IsValidChar   = PathValidator_IsValidChar;
	validator.IsValidString = MenuInputValidator_AlwaysValidString;
	validator.IsValidValue  = MenuInputValidator_AlwaysValidString;
	return validator;
}

void BooleanInputValidator_GetRange(MenuInputValidator* validator, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(yes or no)");
}

MenuInputValidator MenuInputValidator_Boolean(void) {
	MenuInputValidator validator;
	validator.GetRange      = BooleanInputValidator_GetRange;
	validator.IsValidChar   = MenuInputValidator_AlwaysValidChar;
	validator.IsValidString = MenuInputValidator_AlwaysValidString;
	validator.IsValidValue  = MenuInputValidator_AlwaysValidString;
	return validator;
}

MenuInputValidator MenuInputValidator_Enum(const UInt8** names, UInt32 namesCount) {
	MenuInputValidator validator = MenuInputValidator_Boolean();
	validator.Meta_Ptr[0] = names;
	validator.Meta_Ptr[1] = (void*)namesCount; /* TODO: Need to handle void* size < 32 bits?? */
	return validator;
}

void StringValidator_GetRange(MenuInputValidator* validator, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(Enter text)");
}

bool StringValidator_IsValidChar(MenuInputValidator* validator, UInt8 c) {
	return c != '&' && Utils_IsValidInputChar(c, true);
}

bool StringValidator_IsValidString(MenuInputValidator* validator, STRING_PURE String* s) {
	return s->length <= STRING_SIZE;
}

MenuInputValidator MenuInputValidator_String(void) {
	MenuInputValidator validator;
	validator.GetRange       = StringValidator_GetRange;
	validator.IsValidChar    = StringValidator_IsValidChar;
	validator.IsValidString  = StringValidator_IsValidString;
	validator.IsValidValue   = StringValidator_IsValidString;
	return validator;
}


void MenuInputWidget_Render(GuiElement* elem, Real64 delta) {
	Widget* elemW = (Widget*)elem;
	PackedCol backCol = PACKEDCOL_CONST(30, 30, 30, 200);
	Gfx_SetTexturing(false);
	GfxCommon_Draw2DFlat(elemW->X, elemW->Y, elemW->Width, elemW->Height, backCol);
	Gfx_SetTexturing(true);

	InputWidget* widget = (InputWidget*)elem;
	Texture_Render(&widget->InputTex);
	InputWidget_RenderCaret(widget, delta);
}

void MenuInputWidget_RemakeTexture(GuiElement* elem) {
	Widget* elemW = (Widget*)elem;
	MenuInputWidget* widget = (MenuInputWidget*)elem;

	DrawTextArgs args;
	DrawTextArgs_Make(&args, &widget->Base.Lines[0], &widget->Base.Font, false);
	Size2D size = Drawer2D_MeasureText(&args);
	widget->Base.CaretAccumulator = 0.0;

	UInt8 rangeBuffer[String_BufferSize(STRING_SIZE)];
	String range = String_InitAndClearArray(rangeBuffer);
	MenuInputValidator* validator = &widget->Validator;
	validator->GetRange(validator, &range);

	/* Ensure we don't have 0 text height */
	if (size.Height == 0) {
		args.Text = range;
		size.Height = Drawer2D_MeasureText(&args).Height;
		args.Text = widget->Base.Lines[0];
	}

	elemW->Width  = max(size.Width,  widget->MinWidth);
	elemW->Height = max(size.Height, widget->MinHeight);
	Size2D adjSize = size; adjSize.Width = elemW->Width;

	Bitmap bmp; Bitmap_AllocatePow2(&bmp, adjSize.Width, adjSize.Height);
	Drawer2D_Begin(&bmp);
	Drawer2D_DrawText(&args, widget->Base.Padding, 0);

	args.Text = range;
	Size2D hintSize = Drawer2D_MeasureText(&args);
	Int32 hintX = adjSize.Width - hintSize.Width;
	if (size.Width + 3 < hintX) {
		Drawer2D_DrawText(&args, hintX, 0);
	}

	Drawer2D_End();
	Texture* tex = &widget->Base.InputTex;
	*tex = Drawer2D_Make2DTexture(&bmp, adjSize, 0, 0);

	elemW->Reposition(elemW);
	tex->X = elemW->X; tex->Y = elemW->Y;
	if (size.Height < widget->MinHeight) {
		tex->Y += widget->MinHeight / 2 - size.Height / 2;
	}
}

bool MenuInputWidget_AllowedChar(GuiElement* elem, UInt8 c) {
	if (c == '&' || !Utils_IsValidInputChar(c, true)) return false;
	MenuInputWidget* widget = (MenuInputWidget*)elem;
	InputWidget* elemW = (InputWidget*)elem;
	MenuInputValidator* validator = &widget->Validator;

	if (!validator->IsValidChar(validator, c)) return false;
	Int32 maxChars = elemW->GetMaxLines() * INPUTWIDGET_LEN;
	if (elemW->Text.length == maxChars) return false;

	/* See if the new string is in valid format */
	InputWidget_AppendChar(elemW, c);
	bool valid = validator->IsValidString(validator, &elemW->Text);
	InputWidget_DeleteChar(elemW);
	return valid;
}

Int32 MenuInputWidget_GetMaxLines(void) { return 1; }
void MenuInputWidget_Create(MenuInputWidget* widget, Int32 width, Int32 height, STRING_PURE String* text, FontDesc* font, MenuInputValidator* validator) {
	InputWidget_Create(&widget->Base, font, NULL);
	widget->MinWidth  = width;
	widget->MinHeight = height;
	widget->Validator = *validator;

	widget->Base.Padding = 3;	
	widget->Base.Text    = String_InitAndClearArray(widget->TextBuffer);
	widget->Base.GetMaxLines   = MenuInputWidget_GetMaxLines;
	widget->Base.RemakeTexture = MenuInputWidget_RemakeTexture;
	widget->Base.AllowedChar   = MenuInputWidget_AllowedChar;	

	GuiElement* elem = &widget->Base.Base.Base;
	elem->Render = MenuInputWidget_Render;
	elem->Init(elem);
	InputWidget_AppendString(&widget->Base, text);
}


void ChatInputWidget_Render(GuiElement* elem, Real64 delta) {
	ChatInputWidget* widget = (ChatInputWidget*)elem;
	InputWidget* input = (InputWidget*)elem;
	Gfx_SetTexturing(false);
	Int32 x = input->Base.X, y = input->Base.Y;

	UInt32 i;
	for (i = 0; i < INPUTWIDGET_MAX_LINES; i++) {
		if (i > 0 && input->LineSizes[i].Height == 0) break;
		bool caretAtEnd = (input->CaretY == i) && (input->CaretX == INPUTWIDGET_LEN || input->CaretPos == -1);
		Int32 drawWidth = input->LineSizes[i].Width + (caretAtEnd ? input->CaretTex.Width : 0);
		/* Cover whole window width to match original classic behaviour */
		if (Game_PureClassic) {
			drawWidth = max(drawWidth, Game_Width - x * 4);
		}

		PackedCol backCol = PACKEDCOL_CONST(0, 0, 0, 127);
		GfxCommon_Draw2DFlat(x, y, drawWidth + input->Padding * 2, input->PrefixHeight, backCol);
		y += input->LineSizes[i].Height;
	}

	Gfx_SetTexturing(true);
	Texture_Render(&input->InputTex);
	InputWidget_RenderCaret(input, delta);
}

void ChatInputWidget_OnPressedEnter(GuiElement* elem) {
	ChatInputWidget* widget = (ChatInputWidget*)elem;

	/* Don't want trailing spaces in output message */
	String text = widget->Base.Text;
	while (text.length > 0 && text.buffer[text.length - 1] == ' ') { text.length--; }
	if (text.length > 0) { Chat_Send(&text); }

	String orig = String_FromRawArray(widget->OrigBuffer);
	String_Clear(&orig);
	widget->TypingLogPos = Chat_InputLog.Count; /* Index of newest entry + 1. */

	String empty = String_MakeNull();
	Chat_AddOf(&empty, MESSAGE_TYPE_CLIENTSTATUS_2);
	Chat_AddOf(&empty, MESSAGE_TYPE_CLIENTSTATUS_3);
	InputWidget_OnPressedEnter(elem);
}

void ChatInputWidget_UpKey(GuiElement* elem) {
	ChatInputWidget* widget = (ChatInputWidget*)elem;
	InputWidget* input = (InputWidget*)elem;

	if (InputWidget_ControlDown()) {
		Int32 pos = input->CaretPos == -1 ? input->Text.length : input->CaretPos;
		if (pos < INPUTWIDGET_LEN) return;

		input->CaretPos = pos - INPUTWIDGET_LEN;
		InputWidget_UpdateCaret(input);
		return;
	}

	if (widget->TypingLogPos == Chat_InputLog.Count) {
		String orig = String_FromRawArray(widget->OrigBuffer);
		String_Clear(&orig);
		String_AppendString(&orig, &input->Text);
	}

	if (Chat_InputLog.Count == 0) return;
	widget->TypingLogPos--;
	String_Clear(&input->Text);

	if (widget->TypingLogPos < 0) widget->TypingLogPos = 0;
	String prevInput = StringsBuffer_UNSAFE_Get(&Chat_InputLog, widget->TypingLogPos);
	String_AppendString(&input->Text, &prevInput);

	input->CaretPos = -1;
	elem->Recreate(elem);
}

void ChatInputWidget_DownKey(GuiElement* elem) {
	ChatInputWidget* widget = (ChatInputWidget*)elem;
	InputWidget* input = (InputWidget*)elem;

	if (InputWidget_ControlDown()) {
		Int32 lines = input->GetMaxLines();
		if (input->CaretPos == -1 || input->CaretPos >= (lines - 1) * INPUTWIDGET_LEN) return;

		input->CaretPos += INPUTWIDGET_LEN;
		InputWidget_UpdateCaret(input);
		return;
	}

	if (Chat_InputLog.Count == 0) return;
	widget->TypingLogPos++;
	String_Clear(&input->Text);

	if (widget->TypingLogPos >= Chat_InputLog.Count) {
		widget->TypingLogPos = Chat_InputLog.Count;
		String orig = String_FromRawArray(widget->OrigBuffer);
		if (orig.length > 0) { String_AppendString(&input->Text, &orig); }
	} else {
		String prevInput = StringsBuffer_UNSAFE_Get(&Chat_InputLog, widget->TypingLogPos);
		String_AppendString(&input->Text, &prevInput);
	}

	input->CaretPos = -1;
	elem->Recreate(elem);
}

bool ChatInputWidget_IsNameChar(char c) {
	return c == '_' || c == '.' || (c >= '0' && c <= '9')
		|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void ChatInputWidget_TabKey(GuiElement* elem) {
	ChatInputWidget* widget = (ChatInputWidget*)elem;
	InputWidget* input = (InputWidget*)elem;

	Int32 end = input->CaretPos == -1 ? input->Text.length - 1 : input->CaretPos;
	Int32 start = end;
	UInt8* buffer = input->Text.buffer;

	while (start >= 0 && ChatInputWidget_IsNameChar(buffer[start])) { start--; }
	start++;
	if (end < 0 || start > end) return;

	String part = String_UNSAFE_Substring(&input->Text, start, (end + 1) - start);
	String empty = String_MakeNull();
	Chat_AddOf(&empty, MESSAGE_TYPE_CLIENTSTATUS_3);

	EntityID matches[TABLIST_MAX_NAMES];
	UInt32 i, matchesCount = 0;

	for (i = 0; i < TABLIST_MAX_NAMES; i++) {
		EntityID id = (EntityID)i;
		if (!TabList_Valid(id)) continue;

		String name = TabList_UNSAFE_GetPlayer(i);
		if (!String_CaselessStarts(&name, &part)) continue;
		matches[matchesCount++] = id;
	}

	if (matchesCount == 1) {
		if (input->CaretPos == -1) end++;
		Int32 len = end - start, j;
		for (j = 0; j < len; j++) {
			String_DeleteAt(&input->Text, start);
		}

		if (input->CaretPos != -1) input->CaretPos -= len;
		String match = TabList_UNSAFE_GetPlayer(matches[0]);
		InputWidget_AppendString(input, &match);
	} else if (matchesCount > 1) {
		UInt8 strBuffer[String_BufferSize(STRING_SIZE)];
		String str = String_InitAndClearArray(strBuffer);
		String_AppendConst(&str, "&e");
		String_AppendInt32(&str, matchesCount);
		String_AppendConst(&str, " matching names: ");

		for (i = 0; i < matchesCount; i++) {
			String match = TabList_UNSAFE_GetPlayer(matches[i]);
			if ((str.length + match.length + 1) > STRING_SIZE) break;

			String_AppendString(&str, &match);
			String_Append(&str, ' ');
		}
		Chat_AddOf(&str, MESSAGE_TYPE_CLIENTSTATUS_3);
	}
}

bool ChatInputWidget_HandlesKeyDown(GuiElement* elem, Key key) {
	if (key == Key_Tab)  { ChatInputWidget_TabKey(elem);  return true; }
	if (key == Key_Up)   { ChatInputWidget_UpKey(elem);   return true; }
	if (key == Key_Down) { ChatInputWidget_DownKey(elem); return true; }
	return InputWidget_HandlesKeyDown(elem, key);
}

Int32 ChatInputWidget_GetMaxLines(void) {
	return !Game_ClassicMode && ServerConnection_SupportsPartialMessages ? 3 : 1;
}

void ChatInputWidget_Create(ChatInputWidget* widget, FontDesc* font) {
	String prefix = String_FromConst("> ");
	InputWidget_Create(&widget->Base, font, &prefix);
	widget->TypingLogPos = Chat_InputLog.Count; /* Index of newest entry + 1. */

	widget->Base.ShowCaret      = true;
	widget->Base.Padding        = 5;
	widget->Base.GetMaxLines    = ChatInputWidget_GetMaxLines;
	widget->Base.OnPressedEnter = ChatInputWidget_OnPressedEnter;

	widget->Base.Base.Base.Render         = ChatInputWidget_Render;
	widget->Base.Base.Base.HandlesKeyDown = ChatInputWidget_HandlesKeyDown;
}



#define GROUP_NAME_ID UInt16_MaxValue
#define LIST_COLUMN_PADDING 5
#define LIST_BOUNDS_SIZE 10
#define LIST_NAMES_PER_COLUMN 16

Texture PlayerListWidget_DrawName(PlayerListWidget* widget, STRING_PURE String* name) {
	UInt8 tmpBuffer[String_BufferSize(STRING_SIZE)];
	String tmp;
	if (Game_PureClassic) {
		tmp = String_InitAndClearArray(tmpBuffer);
		String_AppendColorless(&tmp, name);
	} else {
		tmp = *name;
	}

	DrawTextArgs args; DrawTextArgs_Make(&args, &tmp, &widget->Font, !widget->Classic);
	Texture tex = Drawer2D_MakeTextTexture(&args, 0, 0);
	Drawer2D_ReducePadding_Tex(&tex, widget->Font.Size, 3);
	return tex;
}


Int32 PlayerListWidget_HighlightedName(PlayerListWidget* widget, Int32 mouseX, Int32 mouseY) {
	if (!widget->Base.Active) return -1;
	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		if (widget->Textures[i].ID == NULL || widget->IDs[i] == GROUP_NAME_ID) continue;

		Texture t = widget->Textures[i];
		if (Gui_Contains(t.X, t.Y, t.Width, t.Height, mouseX, mouseY)) return i;
	}
	return -1;
}

void PlayerListWidget_GetNameUnder(PlayerListWidget* widget, Int32 mouseX, Int32 mouseY, STRING_TRANSIENT String* name) {
	String_Clear(name);
	Int32 i = PlayerListWidget_HighlightedName(widget, mouseX, mouseY);
	if (i == -1) return;

	String player = TabList_UNSAFE_GetPlayer(widget->IDs[i]);
	String_AppendString(name, &player);
}

void PlayerListWidget_UpdateTableDimensions(PlayerListWidget* widget) {
	Int32 width = widget->XMax - widget->XMin, height = widget->YHeight;
	widget->Base.X = (widget->XMin                ) - LIST_BOUNDS_SIZE;
	widget->Base.Y = (Game_Height / 2 - height / 2) - LIST_BOUNDS_SIZE;
	widget->Base.Width  = width  + LIST_BOUNDS_SIZE * 2;
	widget->Base.Height = height + LIST_BOUNDS_SIZE * 2;
}

Int32 PlayerListWidget_GetColumnWidth(PlayerListWidget* widget, Int32 column) {
	Int32 i = column * LIST_NAMES_PER_COLUMN;
	Int32 maxWidth = 0;
	Int32 maxIndex = min(widget->NamesCount, i + LIST_NAMES_PER_COLUMN);

	for (; i < maxIndex; i++) {
		maxWidth = max(maxWidth, widget->Textures[i].Width);
	}
	return maxWidth + LIST_COLUMN_PADDING + widget->ElementOffset;
}

Int32 PlayerListWidget_GetColumnHeight(PlayerListWidget* widget, Int32 column) {
	Int32 i = column * LIST_NAMES_PER_COLUMN;
	Int32 total = 0;
	Int32 maxIndex = min(widget->NamesCount, i + LIST_NAMES_PER_COLUMN);

	for (; i < maxIndex; i++) {
		total += widget->Textures[i].Height + 1;
	}
	return total;
}

void PlayerListWidget_SetColumnPos(PlayerListWidget* widget, Int32 column, Int32 x, Int32 y) {
	Int32 i = column * LIST_NAMES_PER_COLUMN;
	Int32 maxIndex = min(widget->NamesCount, i + LIST_NAMES_PER_COLUMN);

	for (; i < maxIndex; i++) {
		Texture tex = widget->Textures[i];
		tex.X = (Int16)x; tex.Y = (Int16)(y - 10);

		y += tex.Height + 1;
		/* offset player names a bit, compared to group name */
		if (!widget->Classic && widget->IDs[i] != GROUP_NAME_ID) {
			tex.X += (Int16)widget->ElementOffset;
		}
		widget->Textures[i] = tex;
	}
}

void PlayerListWidget_RepositionColumns(PlayerListWidget* widget) {
	Int32 width = 0, centreX = Game_Width / 2;
	widget->YHeight = 0;

	Int32 col, columns = Math_CeilDiv(widget->NamesCount, LIST_NAMES_PER_COLUMN);
	for (col = 0; col < columns; col++) {
		width += PlayerListWidget_GetColumnWidth(widget, col);
		Int32 colHeight = PlayerListWidget_GetColumnHeight(widget, col);
		widget->YHeight = max(colHeight, widget->YHeight);
	}

	if (width < 480) width = 480;
	widget->XMin = centreX - width / 2;
	widget->XMax = centreX + width / 2;

	Int32 x = widget->XMin, y = Game_Height / 2 - widget->YHeight / 2;
	for (col = 0; col < columns; col++) {
		PlayerListWidget_SetColumnPos(widget, col, x, y);
		x += PlayerListWidget_GetColumnWidth(widget, col);
	}
}

void PlayerListWidget_Reposition(Widget* elem) {
	Int32 yPosition = Game_Height / 4 - elem->Height / 2;
	elem->YOffset = -max(0, yPosition);

	Int32 oldX = elem->X, oldY = elem->Y;
	Widget_DoReposition(elem);
	PlayerListWidget* widget = (PlayerListWidget*)elem;

	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		widget->Textures[i].X += elem->X - oldX;
		widget->Textures[i].Y += elem->Y - oldY;
	}
}

void PlayerListWidget_AddName(PlayerListWidget* widget, EntityID id, Int32 index) {
	/* insert at end of list */
	if (index == -1) { index = widget->NamesCount; widget->NamesCount++; }

	String name = TabList_UNSAFE_GetList(id);
	widget->IDs[index]      = id;
	widget->Textures[index] = PlayerListWidget_DrawName(widget, &name);
}

void PlayerListWidget_DeleteAt(PlayerListWidget* widget, Int32 i) {
	Texture tex = widget->Textures[i];
	Gfx_DeleteTexture(&tex.ID);

	for (; i < widget->NamesCount - 1; i++) {
		widget->IDs[i]      = widget->IDs[i + 1];
		widget->Textures[i] = widget->Textures[i + 1];
	}

	widget->IDs[widget->NamesCount] = 0;
	widget->Textures[widget->NamesCount] = Texture_MakeInvalid();
	widget->NamesCount--;
}

void PlayerListWidget_DeleteGroup(PlayerListWidget* widget, Int32* i) {
	PlayerListWidget_DeleteAt(widget, *i);
	(*i)--;
}

void PlayerListWidget_AddGroup(PlayerListWidget* widget, UInt16 id, Int32* index) {
	Int32 i;
	for (i = Array_NumElements(widget->IDs) - 1; i > (*index); i--) {
		widget->IDs[i] = widget->IDs[i - 1];
		widget->Textures[i] = widget->Textures[i - 1];
	}

	String group = TabList_UNSAFE_GetGroup(id);
	widget->IDs[*index] = GROUP_NAME_ID;
	widget->Textures[*index] = PlayerListWidget_DrawName(widget, &group);

	(*index)++;
	widget->NamesCount++;
}

Int32 PlayerListWidget_GetGroupCount(PlayerListWidget* widget, UInt16 id, Int32 idx) {
	String group = TabList_UNSAFE_GetGroup(id);
	Int32 count = 0;

	while (idx < widget->NamesCount) {
		String curGroup = TabList_UNSAFE_GetGroup(widget->IDs[idx]);
		if (!String_CaselessEquals(&group, &curGroup)) return count;
		idx++; count++;
	}
	return count;
}

Int32 PlayerListWidget_PlayerCompare(UInt16 x, UInt16 y) {
	UInt8 xRank = TabList_GroupRanks[x];
	UInt8 yRank = TabList_GroupRanks[y];
	if (xRank != yRank) return (xRank < yRank ? -1 : 1);

	UInt8 xNameBuffer[String_BufferSize(STRING_SIZE)];
	String xName    = String_InitAndClearArray(xNameBuffer);
	String xNameRaw = TabList_UNSAFE_GetList(x);
	String_AppendColorless(&xName, &xNameRaw);

	UInt8 yNameBuffer[String_BufferSize(STRING_SIZE)];
	String yName    = String_InitAndClearArray(yNameBuffer);
	String yNameRaw = TabList_UNSAFE_GetList(y);
	String_AppendColorless(&yName, &yNameRaw);

	return String_Compare(&xName, &yName);
}

Int32 PlayerListWidget_GroupCompare(UInt16 x, UInt16 y) {
	UInt8 xGroupBuffer[String_BufferSize(STRING_SIZE)];
	String xGroup    = String_InitAndClearArray(xGroupBuffer);
	String xGroupRaw = TabList_UNSAFE_GetGroup(x);
	String_AppendColorless(&xGroup, &xGroupRaw);

	UInt8 yGroupBuffer[String_BufferSize(STRING_SIZE)];
	String yGroup    = String_InitAndClearArray(yGroupBuffer);
	String yGroupRaw = TabList_UNSAFE_GetGroup(y);
	String_AppendColorless(&yGroup, &yGroupRaw);

	return String_Compare(&xGroup, &yGroup);
}

PlayerListWidget* List_SortObj;
Int32 (*List_SortCompare)(UInt16 x, UInt16 y);
void PlayerListWidget_QuickSort(Int32 left, Int32 right) {
	Texture* values = List_SortObj->Textures; Texture value;
	UInt16* keys = List_SortObj->IDs;         UInt16 key;
	while (left < right) {
		Int32 i = left, j = right;
		UInt16 pivot = keys[(i + j) / 2];

		/* partition the list */
		while (i <= j) {
			while (List_SortCompare(pivot, keys[i]) > 0) i++;
			while (List_SortCompare(pivot, keys[j]) < 0) j--;
			QuickSort_Swap_KV_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(PlayerListWidget_QuickSort)
	}
}

void PlayerListWidget_SortEntries(PlayerListWidget* widget) {
	if (widget->NamesCount == 0) return;
	List_SortObj = widget;
	if (widget->Classic) {
		List_SortCompare = PlayerListWidget_PlayerCompare;
		PlayerListWidget_QuickSort(0, widget->NamesCount - 1);
		return;
	}

	/* Sort the list into groups */
	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		if (widget->IDs[i] != GROUP_NAME_ID) continue;
		PlayerListWidget_DeleteGroup(widget, &i);
	}
	List_SortCompare = PlayerListWidget_GroupCompare;
	PlayerListWidget_QuickSort(0, widget->NamesCount - 1);

	/* Sort the entries in each group */
	i = 0;
	List_SortCompare = PlayerListWidget_PlayerCompare;
	while (i < widget->NamesCount) {
		UInt16 id = widget->IDs[i];
		PlayerListWidget_AddGroup(widget, id, &i);
		Int32 count = PlayerListWidget_GetGroupCount(widget, id, i);
		PlayerListWidget_QuickSort(i, i + (count - 1));
		i += count;
	}
}

void PlayerListWidget_SortAndReposition(PlayerListWidget* widget) {
	PlayerListWidget_SortEntries(widget);
	PlayerListWidget_RepositionColumns(widget);
	PlayerListWidget_UpdateTableDimensions(widget);
	PlayerListWidget_Reposition(&widget->Base);
}

void PlayerListWidget_TabEntryAdded(void* obj, EntityID id) {
	PlayerListWidget* widget = (PlayerListWidget*)obj;
	PlayerListWidget_AddName(widget, id, -1);
	PlayerListWidget_SortAndReposition(widget);
}

void PlayerListWidget_TabEntryChanged(void* obj, EntityID id) {
	PlayerListWidget* widget = (PlayerListWidget*)obj;
	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		if (widget->IDs[i] != id) continue;

		Texture tex = widget->Textures[i];
		Gfx_DeleteTexture(&tex.ID);
		PlayerListWidget_AddName(widget, id, i);
		PlayerListWidget_SortAndReposition(widget);
		return;
	}
}

void PlayerListWidget_TabEntryRemoved(void* obj, EntityID id) {
	PlayerListWidget* widget = (PlayerListWidget*)obj;
	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		if (widget->IDs[i] != id) continue;
		PlayerListWidget_DeleteAt(widget, i);
		PlayerListWidget_SortAndReposition(widget);
		return;
	}
}

void PlayerListWidget_Init(GuiElement* elem) {
	PlayerListWidget* widget = (PlayerListWidget*)elem;
	Int32 id;
	for (id = 0; id < TABLIST_MAX_NAMES; id++) {
		if (!TabList_Valid((EntityID)id)) continue;
		PlayerListWidget_AddName(widget, (EntityID)id, -1);
	}
	PlayerListWidget_SortAndReposition(widget);

	String msg = String_FromConst("Connected players:");
	TextWidget_Create(&widget->Overview, &msg, &widget->Font);
	Widget_SetLocation(&widget->Overview.Base, ANCHOR_CENTRE, ANCHOR_LEFT_OR_TOP, 0, 0);

	Event_RegisterEntityID(&TabListEvents_Added,   widget, PlayerListWidget_TabEntryAdded);
	Event_RegisterEntityID(&TabListEvents_Changed, widget, PlayerListWidget_TabEntryChanged);
	Event_RegisterEntityID(&TabListEvents_Removed, widget, PlayerListWidget_TabEntryRemoved);
}

void PlayerListWidget_Render(GuiElement* elem, Real64 delta) {
	PlayerListWidget* widget = (PlayerListWidget*)elem;
	Widget* elemW = &widget->Base;
	TextWidget* overview = &widget->Overview;
	PackedCol topCol = PACKEDCOL_CONST(0, 0, 0, 180);
	PackedCol bottomCol = PACKEDCOL_CONST(50, 50, 50, 205);

	Gfx_SetTexturing(false);
	Int32 offset = overview->Base.Height + 10;
	Int32 height = max(300, elemW->Height + overview->Base.Height);
	GfxCommon_Draw2DGradient(elemW->X, elemW->Y - offset, elemW->Width, elemW->Height, topCol, bottomCol);

	Gfx_SetTexturing(true);
	overview->Base.YOffset = elemW->Y - offset + 5;
	overview->Base.Reposition(&overview->Base);
	overview->Base.Base.Render(&overview->Base.Base, delta);

	Int32 i, highlightedI = PlayerListWidget_HighlightedName(widget, Mouse_X, Mouse_Y);
	for (i = 0; i < widget->NamesCount; i++) {
		if (widget->Textures[i].ID == NULL) continue;

		Texture tex = widget->Textures[i];
		if (i == highlightedI) tex.X += 4;
		Texture_Render(&tex);
	}
}

void PlayerListWidget_Free(GuiElement* elem) {
	PlayerListWidget* widget = (PlayerListWidget*)elem;
	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		Gfx_DeleteTexture(&widget->Textures[i].ID);
	}

	elem = &widget->Overview.Base.Base;
	elem->Free(elem);

	Event_UnregisterEntityID(&TabListEvents_Added,   widget, PlayerListWidget_TabEntryAdded);
	Event_UnregisterEntityID(&TabListEvents_Changed, widget, PlayerListWidget_TabEntryChanged);
	Event_UnregisterEntityID(&TabListEvents_Removed, widget, PlayerListWidget_TabEntryRemoved);
}

void PlayerListWidget_Create(PlayerListWidget* widget, FontDesc* font, bool classic) {
	Widget_Init(&widget->Base);
	widget->Base.Base.Init  = PlayerListWidget_Init;
	widget->Base.Base.Free  = PlayerListWidget_Free;
	widget->Base.Reposition = PlayerListWidget_Reposition;
	widget->Base.HorAnchor  = ANCHOR_CENTRE;
	widget->Base.VerAnchor  = ANCHOR_CENTRE;

	widget->NamesCount = 0;
	widget->Font = *font;
	widget->Classic = classic;
	widget->ElementOffset = classic ? 0 : 10;
}


void TextGroupWidget_PushUpAndReplaceLast(TextGroupWidget* widget, STRING_PURE String* text) {
	Int32 y = widget->Base.Y;
	Gfx_DeleteTexture(&widget->Textures[0].ID);
	UInt32 i;
#define tgw_max_idx (Array_NumElements(widget->Textures) - 1)

	/* Move contents of X line to X - 1 line */
	for (i = 0; i < tgw_max_idx; i++) {
		UInt8* dst = widget->Buffer + i       * TEXTGROUPWIDGET_LEN;
		UInt8* src = widget->Buffer + (i + 1) * TEXTGROUPWIDGET_LEN;
		UInt8 lineLen = widget->LineLengths[i + 1];

		if (lineLen > 0) Platform_MemCpy(dst, src, lineLen);
		widget->Textures[i]    = widget->Textures[i + 1];
		widget->LineLengths[i] = lineLen;

		widget->Textures[i].Y = y;
		y += widget->Textures[i].Height;
	}

	widget->Textures[tgw_max_idx].ID = NULL; /* Delete() is called by SetText otherwise */
	TextGroupWidget_SetText(widget, tgw_max_idx, text);
}

Int32 TextGroupWidget_CalcY(TextGroupWidget* widget, Int32 index, Int32 newHeight) {
	Int32 y = 0, i;
	Texture* textures = widget->Textures;
	Int32 deltaY = newHeight - textures[index].Height;

	if (widget->Base.VerAnchor == ANCHOR_LEFT_OR_TOP) {
		y = widget->Base.Y;
		for (i = 0; i < index; i++) {
			y += textures[i].Height;
		}
		for (i = index + 1; i < TEXTGROUPWIDGET_MAX_LINES; i++) {
			textures[i].Y += deltaY;
		}
	} else {
		y = Game_Height - widget->Base.YOffset;
		for (i = index + 1; i < TEXTGROUPWIDGET_MAX_LINES; i++) {
			y -= textures[i].Height;
		}

		y -= newHeight;
		for (i = 0; i < index; i++) {
			textures[i].Y -= deltaY;
		}
	}
	return y;
}

void TextGroupWidget_SetUsePlaceHolder(TextGroupWidget* widget, Int32 index, bool placeHolder) {
	widget->PlaceholderHeight[index] = placeHolder;
	if (widget->Textures[index].ID != NULL) return;

	Int32 newHeight = placeHolder ? widget->DefaultHeight : 0;
	widget->Textures[index].Y = TextGroupWidget_CalcY(widget, index, newHeight);
	widget->Textures[index].Height = (UInt16)newHeight;
}

Int32 TextGroupWidget_GetUsedHeight(TextGroupWidget* widget) {
	Int32 height = 0, i;
	Texture* textures = widget->Textures;

	for (i = 0; i < TEXTGROUPWIDGET_MAX_LINES; i++) {
		if (textures[i].ID != NULL) break;
	}
	for (; i < TEXTGROUPWIDGET_MAX_LINES; i++) {
		height += textures[i].Height;
	}
	return height;
}

void TextGroupWidget_Reposition(Widget* elem) {
	TextGroupWidget* widget = (TextGroupWidget*)elem;
	UInt32 i;
	Texture* textures = widget->Textures;

	Int32 oldY = elem->Y;
	Widget_DoReposition(elem);
	if (widget->LinesCount == 0) return;

	for (i = 0; i < TEXTGROUPWIDGET_MAX_LINES; i++) {
		textures[i].X = Gui_CalcPos(elem->HorAnchor, elem->XOffset, textures[i].Width, Game_Width);
		textures[i].Y += elem->Y - oldY;
	}
}

void TextGroupWidget_UpdateDimensions(TextGroupWidget* widget) {
	Int32 i, width = 0, height = 0;
	Texture* textures = widget->Textures;

	for (i = 0; i < TEXTGROUPWIDGET_MAX_LINES; i++) {
		width = max(width, textures[i].Width);
		height += textures[i].Height;
	}

	widget->Base.Width  = width;
	widget->Base.Height = height;
	TextGroupWidget_Reposition(&widget->Base);
}

String TextGroupWidget_UNSAFE_Get(TextGroupWidget* widget, Int32 i) {
	UInt8* buffer = widget->Buffer + i * TEXTGROUPWIDGET_LEN;
	UInt16 length = widget->LineLengths[i];
	return String_Init(widget->Buffer, length, length);
}

void TextGroupWidget_GetSelected(TextGroupWidget* widget, STRING_TRANSIENT String* text, Int32 x, Int32 y) {
	Int32 i;
	for (i = 0; i < TEXTGROUPWIDGET_MAX_LINES; i++) {
		if (widget->Textures[i].ID == NULL) continue;
		Texture tex = widget->Textures[i];
		/* TODO: Add support for URLS */
		if (!Gui_Contains(tex.X, tex.Y, tex.Width, tex.Height, x, y)) continue;

		String line = TextGroupWidget_UNSAFE_Get(widget, i);
		String_AppendString(text, &line);
		return;
	}
}

void TextGroupWidget_SetText(TextGroupWidget* widget, Int32 index, STRING_PURE String* text) {
	if (text->length > TEXTGROUPWIDGET_LEN) ErrorHandler_Fail("TextGroupWidget - too big text");
	Gfx_DeleteTexture(&widget->Textures[index].ID);
	Platform_MemCpy(widget->Buffer + index * TEXTGROUPWIDGET_LEN, text->buffer, text->length);
	widget->LineLengths[index] = (UInt8)text->length;

	Texture tex;
	if (!Drawer2D_IsEmptyText(text)) {
		/* TODO: Add support for URLs */
		DrawTextArgs args; DrawTextArgs_Make(&args, text, &widget->Font, true);
		tex = Drawer2D_MakeTextTexture(&args, 0, 0);
		Drawer2D_ReducePadding_Tex(&tex, widget->Font.Size, 3);
	} else {
		tex = Texture_MakeInvalid();
		tex.Height = (UInt16)(widget->PlaceholderHeight[index] ? widget->DefaultHeight : 0);
	}

	Widget* elem = &widget->Base;
	tex.X = Gui_CalcPos(elem->HorAnchor, elem->XOffset, tex.Width, Game_Width);
	tex.Y = TextGroupWidget_CalcY(widget, index, tex.Height);
	widget->Textures[index] = tex;
	TextGroupWidget_UpdateDimensions(widget);
}


void TextGroupWidget_Init(GuiElement* elem) {
	TextGroupWidget* widget = (TextGroupWidget*)elem;
	Int32 height = Drawer2D_FontHeight(&widget->Font, true);
	Drawer2D_ReducePadding_Height(&height, widget->Font.Size, 3);
	widget->DefaultHeight = height;

	UInt32 i;
	for (i = 0; i < Array_NumElements(widget->Textures); i++) {
		widget->Textures[i].Height = (UInt16)height;
		widget->PlaceholderHeight[i] = true;
	}
	TextGroupWidget_UpdateDimensions(widget);
}

void TextGroupWidget_Render(GuiElement* elem, Real64 delta) {
	TextGroupWidget* widget = (TextGroupWidget*)elem;
	UInt32 i;
	Texture* textures = widget->Textures;

	for (i = 0; i < TEXTGROUPWIDGET_MAX_LINES; i++) {
		if (textures[i].ID == NULL) continue;
		Texture_Render(&textures[i]);
	}
}

void TextGroupWidget_Free(GuiElement* elem) {
	TextGroupWidget* widget = (TextGroupWidget*)elem;
	UInt32 i;

	for (i = 0; i < TEXTGROUPWIDGET_MAX_LINES; i++) {
		widget->LineLengths[i] = 0;
		Gfx_DeleteTexture(&widget->Textures[i].ID);
	}
}

void TextGroupWidget_Create(TextGroupWidget* widget, Int32 linesCount, FontDesc* font, FontDesc* underlineFont) {
	Widget_Init(&widget->Base);
	widget->Base.Base.Init   = TextGroupWidget_Init;
	widget->Base.Base.Render = TextGroupWidget_Render;
	widget->Base.Base.Free   = TextGroupWidget_Free;
	widget->Base.Reposition  = TextGroupWidget_Reposition;

	widget->LinesCount = linesCount;
	widget->Font = *font;
	widget->UnderlineFont = *underlineFont;
}