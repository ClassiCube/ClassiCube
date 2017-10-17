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
	PackedCol col = PACKEDCOL_WHITE;
	widget->Col = col;
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

#define BUTTON_uWIDTH (200.0f / 256.0f)
Texture Button_ShadowTex   = { 0, 0, 0, 0, 0,  0.0f, 66.0f / 256.0f, 200.0f / 256.0f,  86.0f / 256.0f };
Texture Button_SelectedTex = { 0, 0, 0, 0, 0,  0.0f, 86.0f / 256.0f, 200.0f / 256.0f, 106.0f / 256.0f };
Texture Button_DisabledTex = { 0, 0, 0, 0, 0,  0.0f, 46.0f / 256.0f, 200.0f / 256.0f,  66.0f / 256.0f };
PackedCol Button_NormCol     = PACKEDCOL_CONST(224, 224, 244, 255);
PackedCol Button_ActiveCol   = PACKEDCOL_CONST(255, 255, 160, 255);
PackedCol Button_DisabledCol = PACKEDCOL_CONST(160, 160, 160, 255);

void ButtonWidget_Init(GuiElement* elem) {
	ButtonWidget* widget = (ButtonWidget*)elem;
	widget->DefaultHeight = Drawer2D_FontHeight(widget->Font, true);
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

void ButtonWidget_Create(ButtonWidget* widget, STRING_TRANSIENT String* text, Int32 minWidth, void* font, Gui_MouseHandler onClick) {
	Widget_Init(&widget->Base);
	widget->Base.Base.Init   = ButtonWidget_Init;
	widget->Base.Base.Render = ButtonWidget_Render;
	widget->Base.Base.Free   = ButtonWidget_Free;
	widget->Base.Reposition  = ButtonWidget_Reposition;

	widget->Font = font;
	GuiElement* elem = &widget->Base.Base;
	elem->Init(elem);
	widget->MinWidth = minWidth; widget->MinHeight = 40;
	ButtonWidget_SetText(widget, text);
	widget->Base.Base.HandlesMouseDown = onClick;
}

void ButtonWidget_SetText(ButtonWidget* widget, STRING_TRANSIENT String* text) {
	Gfx_DeleteTexture(&widget->Texture.ID);
	Widget* elem = &widget->Base;
	if (Drawer2D_EmptyText(text)) {
		widget->Texture = Texture_MakeInvalid();
		elem->Width = 0; elem->Height = widget->DefaultHeight;
	} else {
		DrawTextArgs args;
		DrawTextArgs_Make(&args, text, widget->Font, true);
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
	Int32 width = widget->ElemSize + widget->BorderSize;
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

	Int32 width = widget->ElemSize + widget->BorderSize;
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

void HotbarWidget_Reposition(Widget* elem) {
	HotbarWidget* widget = (HotbarWidget*)elem;
	Real32 scale = Game_GetHotbarScale();

	widget->BarHeight = (Int32)(22 * scale);
	elem->Width = (Int32)(182 * scale);
	elem->Height = widget->BarHeight;

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

	/* Alternate between first and second row */
	Int32 index = Inventory_Offset == 0 ? 1 : 0;
	Inventory_SetOffset(index * INVENTORY_BLOCKS_PER_HOTBAR);
	return true;
}

bool HotbarWidget_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	Widget* w = (Widget*)elem;
	if (btn != MouseButton_Left || !Gui_Contains(w->X, w->Y, w->Width, w->Height, x, y)) return false;
	InventoryScreen screen = game.Gui.ActiveScreen as InventoryScreen;
	if (screen == null) return false;

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

void HotbarWidget_Create(HotbarWidget* widget) {
	Widget_Init(&widget->Base);
	widget->Base.HorAnchor = ANCHOR_CENTRE;
	widget->Base.VerAnchor = ANCHOR_BOTTOM_OR_RIGHT;
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
   Then using paInt32 to extract the colour components
   Then using wolfram alpha to solve the glblendfunc equation */
PackedCol Table_TopCol       = PACKEDCOL_CONST( 34,  34,  34, 168);
PackedCol Table_BottomCol    = PACKEDCOL_CONST( 57,  57, 104, 202);
PackedCol Table_TopSelCol    = PACKEDCOL_CONST(255, 255, 255, 142);
PackedCol Table_BottomSelCol = PACKEDCOL_CONST(255, 255, 255, 192);
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
	DrawTextArgs_Make(&args, &desc, widget->Font, true);
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
		Table_Width(widget), Table_Height(widget), Table_TopCol, Table_BottomCol);
	if (widget->RowsCount > TABLE_MAX_ROWS_DISPLAYED) {
		GuiElement* scroll = &widget->Scroll.Base.Base;
		scroll->Render(scroll, delta);
	}

	Int32 blockSize = widget->BlockSize;
	if (widget->SelectedIndex != -1 && Game_ClassicMode) {
		Int32 x, y;
		TableWidget_GetCoords(widget, widget->SelectedIndex, &x, &y);
		Real32 off = blockSize * 0.1f;
		GfxCommon_Draw2DGradient(x - off, y - off, blockSize + off * 2,
			blockSize + off * 2, Table_TopSelCol, Table_BottomSelCol);
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
