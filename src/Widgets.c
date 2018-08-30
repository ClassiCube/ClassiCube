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
#include "ServerConnection.h"
#include "Event.h"
#include "Chat.h"
#include "Game.h"
#include "ErrorHandler.h"

#define WIDGET_UV(u1,v1, u2,v2) u1/256.0f,v1/256.0f, u2/256.0f,v2/256.0f
void Widget_SetLocation(struct Widget* widget, UInt8 horAnchor, UInt8 verAnchor, Int32 xOffset, Int32 yOffset) {
	widget->HorAnchor = horAnchor; widget->VerAnchor = verAnchor;
	widget->XOffset = xOffset; widget->YOffset = yOffset;
	widget->Reposition((struct GuiElem*)widget);
}


/*########################################################################################################################*
*-------------------------------------------------------TextWidget--------------------------------------------------------*
*#########################################################################################################################*/
static void TextWidget_SetHeight(struct TextWidget* widget, Int32 height) {
	if (widget->ReducePadding) {
		Drawer2D_ReducePadding_Height(&height, widget->Font.Size, 4);
	}
	widget->DefaultHeight = height;
	widget->Height = height;
}

static void TextWidget_Init(struct GuiElem* elem) {
	struct TextWidget* widget = (struct TextWidget*)elem;
	Int32 height = Drawer2D_FontHeight(&widget->Font, true);
	TextWidget_SetHeight(widget, height);
}

static void TextWidget_Render(struct GuiElem* elem, Real64 delta) {
	struct TextWidget* widget = (struct TextWidget*)elem;	
	if (widget->Texture.ID) {
		Texture_RenderShaded(&widget->Texture, widget->Col);
	}
}

static void TextWidget_Free(struct GuiElem* elem) {
	struct TextWidget* widget = (struct TextWidget*)elem;
	Gfx_DeleteTexture(&widget->Texture.ID);
}

static void TextWidget_Reposition(struct GuiElem* elem) {
	struct TextWidget* widget = (struct TextWidget*)elem;
	Int32 oldX = widget->X, oldY = widget->Y;
	Widget_DoReposition(elem);
	widget->Texture.X += widget->X - oldX;
	widget->Texture.Y += widget->Y - oldY;
}

struct GuiElemVTABLE TextWidget_VTABLE;
void TextWidget_Make(struct TextWidget* widget, struct FontDesc* font) {
	widget->VTABLE = &TextWidget_VTABLE;
	Widget_Init((struct Widget*)widget);
	PackedCol col = PACKEDCOL_WHITE;
	widget->Col = col;
	widget->Font = *font;
	widget->Reposition     = TextWidget_Reposition;
	widget->VTABLE->Init   = TextWidget_Init;
	widget->VTABLE->Render = TextWidget_Render;
	widget->VTABLE->Free   = TextWidget_Free;
}

void TextWidget_Create(struct TextWidget* widget, STRING_PURE String* text, struct FontDesc* font) {
	TextWidget_Make(widget, font);
	Elem_Init(widget);
	TextWidget_SetText(widget, text);
}

void TextWidget_SetText(struct TextWidget* widget, STRING_PURE String* text) {
	Gfx_DeleteTexture(&widget->Texture.ID);
	if (Drawer2D_IsEmptyText(text)) {
		widget->Width = 0; widget->Height = widget->DefaultHeight;
	} else {
		struct DrawTextArgs args;
		DrawTextArgs_Make(&args, text, &widget->Font, true);
		Drawer2D_MakeTextTexture(&widget->Texture, &args, 0, 0);
		if (widget->ReducePadding) {
			Drawer2D_ReducePadding_Tex(&widget->Texture, widget->Font.Size, 4);
		}

		widget->Width = widget->Texture.Width; widget->Height = widget->Texture.Height;
		Widget_Reposition(widget);
		widget->Texture.X = widget->X; widget->Texture.Y = widget->Y;
	}
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
#define BUTTON_uWIDTH (200.0f / 256.0f)
#define BUTTON_MIN_WIDTH 40

struct Texture Button_ShadowTex   = { NULL, TEX_RECT(0,0, 0,0), WIDGET_UV(0,66, 200,86)  };
struct Texture Button_SelectedTex = { NULL, TEX_RECT(0,0, 0,0), WIDGET_UV(0,86, 200,106) };
struct Texture Button_DisabledTex = { NULL, TEX_RECT(0,0, 0,0), WIDGET_UV(0,46, 200,66)  };

static void ButtonWidget_Init(struct GuiElem* elem) {
	struct ButtonWidget* widget = (struct ButtonWidget*)elem;
	widget->DefaultHeight = Drawer2D_FontHeight(&widget->Font, true);
	widget->Height = widget->DefaultHeight;
}

static void ButtonWidget_Free(struct GuiElem* elem) {
	struct ButtonWidget* widget = (struct ButtonWidget*)elem;
	Gfx_DeleteTexture(&widget->Texture.ID);
}

static void ButtonWidget_Reposition(struct GuiElem* elem) {
	struct ButtonWidget* widget = (struct ButtonWidget*)elem;
	Int32 oldX = widget->X, oldY = widget->Y;
	Widget_DoReposition(elem);
	
	widget->Texture.X += widget->X - oldX;
	widget->Texture.Y += widget->Y - oldY;
}

static void ButtonWidget_Render(struct GuiElem* elem, Real64 delta) {
	struct ButtonWidget* widget = (struct ButtonWidget*)elem;
	if (!widget->Texture.ID) return;
	struct Texture back = widget->Active ? Button_SelectedTex : Button_ShadowTex;
	if (widget->Disabled) back = Button_DisabledTex;

	back.ID = Game_UseClassicGui ? Gui_GuiClassicTex : Gui_GuiTex;
	back.X = widget->X; back.Width  = widget->Width;
	back.Y = widget->Y; back.Height = widget->Height;

	if (widget->Width == 400) {
		/* Button can be drawn normally */
		Texture_Render(&back);
	} else {
		/* Split button down the middle */
		Real32 scale = (widget->Width / 400.0f) * 0.5f;
		Gfx_BindTexture(back.ID); /* avoid bind twice */
		PackedCol white = PACKEDCOL_WHITE;

		back.Width = (widget->Width / 2);
		back.U1 = 0.0f; back.U2 = BUTTON_uWIDTH * scale;
		GfxCommon_Draw2DTexture(&back, white);

		back.X += (widget->Width / 2);
		back.U1 = BUTTON_uWIDTH * (1.0f - scale); back.U2 = BUTTON_uWIDTH;
		GfxCommon_Draw2DTexture(&back, white);
	}

	PackedCol normCol     = PACKEDCOL_CONST(224, 224, 244, 255);
	PackedCol activeCol   = PACKEDCOL_CONST(255, 255, 160, 255);
	PackedCol disabledCol = PACKEDCOL_CONST(160, 160, 160, 255);
	PackedCol col = widget->Disabled ? disabledCol : (widget->Active ? activeCol : normCol);
	Texture_RenderShaded(&widget->Texture, col);
}

struct GuiElemVTABLE ButtonWidget_VTABLE;
void ButtonWidget_Create(struct ButtonWidget* widget, Int32 minWidth, STRING_PURE String* text, struct FontDesc* font, Widget_LeftClick onClick) {
	widget->VTABLE = &ButtonWidget_VTABLE;
	Widget_Init((struct Widget*)widget);
	widget->VTABLE->Init   = ButtonWidget_Init;
	widget->VTABLE->Render = ButtonWidget_Render;
	widget->VTABLE->Free   = ButtonWidget_Free;
	widget->Reposition = ButtonWidget_Reposition;

	widget->OptName = NULL;
	widget->Font = *font;
	Elem_Init(widget);
	widget->MinWidth = minWidth;
	ButtonWidget_SetText(widget, text);
	widget->MenuClick = onClick;
}

void ButtonWidget_SetText(struct ButtonWidget* widget, STRING_PURE String* text) {
	Gfx_DeleteTexture(&widget->Texture.ID);
	if (Drawer2D_IsEmptyText(text)) {
		widget->Width = 0; widget->Height = widget->DefaultHeight;
	} else {
		struct DrawTextArgs args;
		DrawTextArgs_Make(&args, text, &widget->Font, true);
		Drawer2D_MakeTextTexture(&widget->Texture, &args, 0, 0);
		widget->Width  = max(widget->Texture.Width,  widget->MinWidth);
		widget->Height = max(widget->Texture.Height, BUTTON_MIN_WIDTH);

		Widget_Reposition(widget);
		widget->Texture.X = widget->X + (widget->Width  / 2 - widget->Texture.Width  / 2);
		widget->Texture.Y = widget->Y + (widget->Height / 2 - widget->Texture.Height / 2);
	}
}


/*########################################################################################################################*
*-----------------------------------------------------ScrollbarWidget-----------------------------------------------------*
*#########################################################################################################################*/
#define TABLE_MAX_ROWS_DISPLAYED 8
#define SCROLL_WIDTH 22
#define SCROLL_BORDER 2
#define SCROLL_NUBS_WIDTH 3
PackedCol Scroll_BackCol  = PACKEDCOL_CONST( 10,  10,  10, 220);
PackedCol Scroll_BarCol   = PACKEDCOL_CONST(100, 100, 100, 220);
PackedCol Scroll_HoverCol = PACKEDCOL_CONST(122, 122, 122, 220);

static void ScrollbarWidget_Init(struct GuiElem* elem) { }
static void ScrollbarWidget_Free(struct GuiElem* elem) { }

static void ScrollbarWidget_ClampScrollY(struct ScrollbarWidget* widget) {
	Int32 maxRows = widget->TotalRows - TABLE_MAX_ROWS_DISPLAYED;
	if (widget->ScrollY >= maxRows) widget->ScrollY = maxRows;
	if (widget->ScrollY < 0) widget->ScrollY = 0;
}

static Real32 ScrollbarWidget_GetScale(struct ScrollbarWidget* widget) {
	Real32 rows = (Real32)widget->TotalRows;
	return (widget->Height - SCROLL_BORDER * 2) / rows;
}

static void ScrollbarWidget_GetScrollbarCoords(struct ScrollbarWidget* widget, Int32* y, Int32* height) {
	Real32 scale = ScrollbarWidget_GetScale(widget);
	*y = Math_Ceil(widget->ScrollY * scale) + SCROLL_BORDER;
	*height = Math_Ceil(TABLE_MAX_ROWS_DISPLAYED * scale);
	*height = min(*y + *height, widget->Height - SCROLL_BORDER) - *y;
}

static void ScrollbarWidget_Render(struct GuiElem* elem, Real64 delta) {
	struct ScrollbarWidget* widget = (struct ScrollbarWidget*)elem;
	Int32 x = widget->X, width = widget->Width;
	GfxCommon_Draw2DFlat(x, widget->Y, width, widget->Height, Scroll_BackCol);

	Int32 y, height;
	ScrollbarWidget_GetScrollbarCoords(widget, &y, &height);
	x += SCROLL_BORDER; y += widget->Y;
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

static bool ScrollbarWidget_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct ScrollbarWidget* widget = (struct ScrollbarWidget*)elem;
	if (widget->DraggingMouse) return true;
	if (btn != MouseButton_Left) return false;
	if (x < widget->X || x >= widget->X + widget->Width) return false;

	y -= widget->Y;
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

static bool ScrollbarWidget_HandlesMouseUp(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct ScrollbarWidget* widget = (struct ScrollbarWidget*)elem;
	widget->DraggingMouse = false;
	widget->MouseOffset = 0;
	return true;
}

static bool ScrollbarWidget_HandlesMouseScroll(struct GuiElem* elem, Real32 delta) {
	struct ScrollbarWidget* widget = (struct ScrollbarWidget*)elem;
	Int32 steps = Utils_AccumulateWheelDelta(&widget->ScrollingAcc, delta);
	widget->ScrollY -= steps;
	ScrollbarWidget_ClampScrollY(widget);
	return true;
}

static bool ScrollbarWidget_HandlesMouseMove(struct GuiElem* elem, Int32 x, Int32 y) {
	struct ScrollbarWidget* widget = (struct ScrollbarWidget*)elem;
	if (widget->DraggingMouse) {
		y -= widget->Y;
		Real32 scale = ScrollbarWidget_GetScale(widget);
		widget->ScrollY = (Int32)((y - widget->MouseOffset) / scale);
		ScrollbarWidget_ClampScrollY(widget);
		return true;
	}
	return false;
}

struct GuiElemVTABLE ScrollbarWidget_VTABLE;
void ScrollbarWidget_Create(struct ScrollbarWidget* widget) {
	widget->VTABLE = &ScrollbarWidget_VTABLE;
	Widget_Init((struct Widget*)widget);
	widget->VTABLE->Init   = ScrollbarWidget_Init;
	widget->VTABLE->Render = ScrollbarWidget_Render;
	widget->VTABLE->Free   = ScrollbarWidget_Free;

	widget->VTABLE->HandlesMouseDown   = ScrollbarWidget_HandlesMouseDown;
	widget->VTABLE->HandlesMouseUp     = ScrollbarWidget_HandlesMouseUp;
	widget->VTABLE->HandlesMouseScroll = ScrollbarWidget_HandlesMouseScroll;
	widget->VTABLE->HandlesMouseMove   = ScrollbarWidget_HandlesMouseMove;	

	widget->Width = SCROLL_WIDTH;
	widget->TotalRows = 0;
	widget->ScrollY = 0;
	widget->ScrollingAcc = 0.0f;
	widget->DraggingMouse = false;
	widget->MouseOffset = 0;
}


/*########################################################################################################################*
*------------------------------------------------------HotbarWidget-------------------------------------------------------*
*#########################################################################################################################*/
static void HotbarWidget_RenderHotbarOutline(struct HotbarWidget* widget) {
	GfxResourceID texId = Game_UseClassicGui ? Gui_GuiClassicTex : Gui_GuiTex;
	widget->BackTex.ID = texId;
	Texture_Render(&widget->BackTex);

	Int32 i = Inventory_SelectedIndex;
	Real32 width = widget->ElemSize + widget->BorderSize;
	Int32 x = (Int32)(widget->X + widget->BarXOffset + width * i + widget->ElemSize / 2);

	widget->SelTex.ID = texId;
	widget->SelTex.X = (Int32)(x - widget->SelBlockSize / 2);
	PackedCol white = PACKEDCOL_WHITE;
	GfxCommon_Draw2DTexture(&widget->SelTex, white);
}

static void HotbarWidget_RenderHotbarBlocks(struct HotbarWidget* widget) {
	/* TODO: Should hotbar use its own VB? */
	VertexP3fT2fC4b vertices[INVENTORY_BLOCKS_PER_HOTBAR * ISOMETRICDRAWER_MAXVERTICES];
	IsometricDrawer_BeginBatch(vertices, ModelCache_Vb);

	Real32 width = widget->ElemSize + widget->BorderSize;
	UInt32 i;
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		BlockID block = Inventory_Get(i);
		Int32 x = (Int32)(widget->X + widget->BarXOffset + width * i + widget->ElemSize / 2);
		Int32 y = (Int32)(widget->Y + (widget->Height - widget->BarHeight / 2));

		Real32 scale = (widget->ElemSize * 13.5f / 16.0f) / 2.0f;
		IsometricDrawer_DrawBatch(block, scale, x, y);
	}
	IsometricDrawer_EndBatch();
}

static void HotbarWidget_RepositonBackgroundTexture(struct HotbarWidget* w) {
	struct Texture tex = { NULL, TEX_RECT(w->X,w->Y, w->Width,w->Height), WIDGET_UV(0,0, 182,22) };
	w->BackTex = tex;
}

static void HotbarWidget_RepositionSelectionTexture(struct HotbarWidget* w) {
	Real32 scale = Game_GetHotbarScale();
	Int32 hSize = (Int32)w->SelBlockSize;
	Int32 vSize = (Int32)(22.0f * scale);
	Int32 y = w->Y + (w->Height - (Int32)(23.0f * scale));

	struct Texture tex = { NULL, TEX_RECT(0,y, hSize,vSize), WIDGET_UV(0,22, 24,44) };
	w->SelTex = tex;
}

static Int32 HotbarWidget_ScrolledIndex(struct HotbarWidget* widget, Real32 delta, Int32 index, Int32 dir) {
	Int32 steps = Utils_AccumulateWheelDelta(&widget->ScrollAcc, delta);
	index += (dir * steps) % INVENTORY_BLOCKS_PER_HOTBAR;

	if (index < 0) index += INVENTORY_BLOCKS_PER_HOTBAR;
	if (index >= INVENTORY_BLOCKS_PER_HOTBAR) {
		index -= INVENTORY_BLOCKS_PER_HOTBAR;
	}
	return index;
}

static void HotbarWidget_Reposition(struct GuiElem* elem) {
	struct HotbarWidget* widget = (struct HotbarWidget*)elem;
	Real32 scale = Game_GetHotbarScale();

	widget->BarHeight = (Real32)Math_Floor(22.0f * scale);
	widget->Width  = (Int32)(182 * scale);
	widget->Height = (Int32)widget->BarHeight;

	widget->SelBlockSize = (Real32)Math_Ceil(24.0f * scale);
	widget->ElemSize     = 16.0f * scale;
	widget->BarXOffset   = 3.1f * scale;
	widget->BorderSize   = 4.0f * scale;

	Widget_DoReposition(elem);
	HotbarWidget_RepositonBackgroundTexture(widget);
	HotbarWidget_RepositionSelectionTexture(widget);
}

static void HotbarWidget_Init(struct GuiElem* elem) { 
	struct HotbarWidget* widget = (struct HotbarWidget*)elem;
	Widget_Reposition(widget);
}

static void HotbarWidget_Render(struct GuiElem* elem, Real64 delta) {
	struct HotbarWidget* widget = (struct HotbarWidget*)elem;
	HotbarWidget_RenderHotbarOutline(widget);
	HotbarWidget_RenderHotbarBlocks(widget);
}
static void HotbarWidget_Free(struct GuiElem* elem) { }

static bool HotbarWidget_HandlesKeyDown(struct GuiElem* elem, Key key) {
	if (key >= Key_1 && key <= Key_9) {
		Int32 index = key - Key_1;
		if (KeyBind_IsPressed(KeyBind_HotbarSwitching)) {
			/* Pick from first to ninth row */
			Inventory_SetOffset(index * INVENTORY_BLOCKS_PER_HOTBAR);
			struct HotbarWidget* widget = (struct HotbarWidget*)elem;
			widget->AltHandled = true;
		} else {
			Inventory_SetSelectedIndex(index);
		}
		return true;
	}
	return false;
}

static bool HotbarWidget_HandlesKeyUp(struct GuiElem* elem, Key key) {
	/* We need to handle these cases:
	   a) user presses alt then number
	   b) user presses alt
	   thus we only do case b) if case a) did not happen */
	struct HotbarWidget* widget = (struct HotbarWidget*)elem;
	if (key != KeyBind_Get(KeyBind_HotbarSwitching)) return false;
	if (widget->AltHandled) { widget->AltHandled = false; return true; } /* handled already */

	/* Don't switch hotbar when alt+tab */
	if (!Window_Focused) return true;

	/* Alternate between first and second row */
	Int32 index = Inventory_Offset == 0 ? 1 : 0;
	Inventory_SetOffset(index * INVENTORY_BLOCKS_PER_HOTBAR);
	return true;
}

static bool HotbarWidget_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct HotbarWidget* widget = (struct HotbarWidget*)elem;
	if (btn != MouseButton_Left || !Widget_Contains((struct Widget*)widget, x, y)) return false;
	struct Screen* screen = Gui_GetActiveScreen();
	if (screen != InventoryScreen_UNSAFE_RawPointer) return false;

	Int32 width  = (Int32)(widget->ElemSize * widget->BorderSize);
	Int32 height = Math_Ceil(widget->BarHeight);
	UInt32 i;

	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		Int32 winX = (Int32)(widget->X + width * i);
		Int32 winY = (Int32)(widget->Y + (widget->Height - height));

		if (Gui_Contains(winX, winY, width, height, x, y)) {
			Inventory_SetSelectedIndex(i);
			return true;
		}
	}
	return false;
}

static bool HotbarWidget_HandlesMouseScroll(struct GuiElem* elem, Real32 delta) {
	struct HotbarWidget* widget = (struct HotbarWidget*)elem;
	if (KeyBind_IsPressed(KeyBind_HotbarSwitching)) {
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

struct GuiElemVTABLE HotbarWidget_VTABLE;
void HotbarWidget_Create(struct HotbarWidget* widget) {
	widget->VTABLE = &HotbarWidget_VTABLE;
	Widget_Init((struct Widget*)widget);
	widget->HorAnchor = ANCHOR_CENTRE;
	widget->VerAnchor = ANCHOR_MAX;

	widget->VTABLE->Init   = HotbarWidget_Init;
	widget->VTABLE->Render = HotbarWidget_Render;
	widget->VTABLE->Free   = HotbarWidget_Free;
	widget->Reposition     = HotbarWidget_Reposition;

	widget->VTABLE->HandlesKeyDown     = HotbarWidget_HandlesKeyDown;
	widget->VTABLE->HandlesKeyUp       = HotbarWidget_HandlesKeyUp;
	widget->VTABLE->HandlesMouseDown   = HotbarWidget_HandlesMouseDown;
	widget->VTABLE->HandlesMouseScroll = HotbarWidget_HandlesMouseScroll;
}


/*########################################################################################################################*
*-------------------------------------------------------TableWidget-------------------------------------------------------*
*#########################################################################################################################*/
static Int32 Table_X(struct TableWidget* widget) { return widget->X - 5 - 10; }
static Int32 Table_Y(struct TableWidget* widget) { return widget->Y - 5 - 30; }
static Int32 Table_Width(struct TableWidget* widget) {
	return widget->ElementsPerRow * widget->BlockSize + 10 + 20; 
}
static Int32 Table_Height(struct TableWidget* widget) {
	return min(widget->RowsCount, TABLE_MAX_ROWS_DISPLAYED) * widget->BlockSize + 10 + 40;
}

#define TABLE_MAX_VERTICES (8 * 10 * ISOMETRICDRAWER_MAXVERTICES)

static bool TableWidget_GetCoords(struct TableWidget* widget, Int32 i, Int32* winX, Int32* winY) {
	Int32 x = i % widget->ElementsPerRow, y = i / widget->ElementsPerRow;
	*winX = widget->X + widget->BlockSize * x;
	*winY = widget->Y + widget->BlockSize * y + 3;

	*winY -= widget->Scroll.ScrollY * widget->BlockSize;
	y -= widget->Scroll.ScrollY;
	return y >= 0 && y < TABLE_MAX_ROWS_DISPLAYED;
}

static void TableWidget_UpdateScrollbarPos(struct TableWidget* widget) {
	struct ScrollbarWidget* scroll = &widget->Scroll;
	scroll->X = Table_X(widget) + Table_Width(widget);
	scroll->Y = Table_Y(widget);
	scroll->Height = Table_Height(widget);
	scroll->TotalRows = widget->RowsCount;
}

static void TableWidget_MoveCursorToSelected(struct TableWidget* widget) {
	if (widget->SelectedIndex == -1) return;

	Int32 x, y, i = widget->SelectedIndex;
	TableWidget_GetCoords(widget, i, &x, &y);
	x += widget->BlockSize / 2; y += widget->BlockSize / 2;

	struct Point2D topLeft = Window_PointToScreen(Point2D_Empty);
	x += topLeft.X; y += topLeft.Y;
	Window_SetDesktopCursorPos(Point2D_Make(x, y));
}

static void TableWidget_MakeBlockDesc(STRING_TRANSIENT String* desc, BlockID block) {
	if (Game_PureClassic) { String_AppendConst(desc, "Select block"); return; }
	String name = Block_UNSAFE_GetName(block);
	String_AppendString(desc, &name);
	if (Game_ClassicMode) return;

	String_Format1(desc, " (ID %b&f", &block);
	if (!Block_CanPlace[block])  { String_AppendConst(desc,  ", place &cNo&f"); }
	if (!Block_CanDelete[block]) { String_AppendConst(desc, ", delete &cNo&f"); }
	String_Append(desc, ')');
}

static void TableWidget_UpdateDescTexPos(struct TableWidget* widget) {
	widget->DescTex.X = widget->X + widget->Width / 2 - widget->DescTex.Width / 2;
	widget->DescTex.Y = widget->Y - widget->DescTex.Height - 5;
}

static void TableWidget_UpdatePos(struct TableWidget* widget) {
	Int32 rowsDisplayed = min(TABLE_MAX_ROWS_DISPLAYED, widget->RowsCount);
	widget->Width = widget->BlockSize * widget->ElementsPerRow;
	widget->Height = widget->BlockSize * rowsDisplayed;
	widget->X = Game_Width  / 2 - widget->Width  / 2;
	widget->Y = Game_Height / 2 - widget->Height / 2;
	TableWidget_UpdateDescTexPos(widget);
}

static void TableWidget_RecreateDescTex(struct TableWidget* widget) {
	if (widget->SelectedIndex == widget->LastCreatedIndex) return;
	if (widget->ElementsCount == 0) return;
	widget->LastCreatedIndex = widget->SelectedIndex;

	Gfx_DeleteTexture(&widget->DescTex.ID);
	if (widget->SelectedIndex == -1) return;
	BlockID block = widget->Elements[widget->SelectedIndex];
	TableWidget_MakeDescTex(widget, block);
}

void TableWidget_MakeDescTex(struct TableWidget* widget, BlockID block) {
	Gfx_DeleteTexture(&widget->DescTex.ID);
	if (block == BLOCK_AIR) return;

	char descBuffer[STRING_SIZE * 2];
	String desc = String_FromArray(descBuffer);
	TableWidget_MakeBlockDesc(&desc, block);

	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &desc, &widget->Font, true);
	Drawer2D_MakeTextTexture(&widget->DescTex, &args, 0, 0);
	TableWidget_UpdateDescTexPos(widget);
}

static bool TableWidget_RowEmpty(struct TableWidget* widget, Int32 i) {
	Int32 max = min(i + widget->ElementsPerRow, (Int32)Array_Elems(Inventory_Map));

	Int32 j;
	for (j = i; j < max; j++) {
		if (Inventory_Map[j] != BLOCK_AIR) return false;
	}
	return true;
}

static bool TableWidget_Show(BlockID block) {
	if (block < BLOCK_CPE_COUNT) {
		Int32 count = Game_UseCPEBlocks ? BLOCK_CPE_COUNT : BLOCK_ORIGINAL_COUNT;
		return block < count;
	}
	return true;
}

static void TableWidget_RecreateElements(struct TableWidget* widget) {
	widget->ElementsCount = 0;
	Int32 count = Game_UseCPE ? BLOCK_COUNT : BLOCK_ORIGINAL_COUNT, i;
	for (i = 0; i < count;) {
		if ((i % widget->ElementsPerRow) == 0 && TableWidget_RowEmpty(widget, i)) {
			i += widget->ElementsPerRow; continue;
		}

		BlockID block = Inventory_Map[i];
		if (TableWidget_Show(block)) { widget->ElementsCount++; }
		i++;
	}

	widget->RowsCount = Math_CeilDiv(widget->ElementsCount, widget->ElementsPerRow);
	TableWidget_UpdateScrollbarPos(widget);
	TableWidget_UpdatePos(widget);

	Int32 index = 0;
	for (i = 0; i < count;) {
		if ((i % widget->ElementsPerRow) == 0 && TableWidget_RowEmpty(widget, i)) {
			i += widget->ElementsPerRow; continue;
		}

		BlockID block = Inventory_Map[i];
		if (TableWidget_Show(block)) { widget->Elements[index++] = block; }
		i++;
	}
}

static void TableWidget_Init(struct GuiElem* elem) {
	struct TableWidget* widget = (struct TableWidget*)elem;
	widget->LastX = Mouse_X; widget->LastY = Mouse_Y;

	ScrollbarWidget_Create(&widget->Scroll);
	TableWidget_RecreateElements(widget);
	Widget_Reposition(widget);
}

static void TableWidget_Render(struct GuiElem* elem, Real64 delta) {	
	/* These were sourced by taking a screenshot of vanilla
	Then using paint to extract the colour components
	Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topBackCol    = PACKEDCOL_CONST( 34,  34,  34, 168);
	PackedCol bottomBackCol = PACKEDCOL_CONST( 57,  57, 104, 202);
	PackedCol topSelCol     = PACKEDCOL_CONST(255, 255, 255, 142);
	PackedCol bottomSelCol  = PACKEDCOL_CONST(255, 255, 255, 192);

	struct TableWidget* widget = (struct TableWidget*)elem;
	GfxCommon_Draw2DGradient(Table_X(widget), Table_Y(widget),
		Table_Width(widget), Table_Height(widget), topBackCol, bottomBackCol);

	if (widget->RowsCount > TABLE_MAX_ROWS_DISPLAYED) {
		Elem_Render(&widget->Scroll, delta);
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

	if (widget->DescTex.ID) {
		Texture_Render(&widget->DescTex);
	}
	Gfx_SetTexturing(false);
}

static void TableWidget_Free(struct GuiElem* elem) {
	struct TableWidget* widget = (struct TableWidget*)elem;
	Gfx_DeleteVb(&widget->VB);
	Gfx_DeleteTexture(&widget->DescTex.ID);
	widget->LastCreatedIndex = -1000;
}

static void TableWidget_Recreate(struct GuiElem* elem) {
	struct TableWidget* widget = (struct TableWidget*)elem;
	Elem_TryFree(widget);
	widget->VB = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, TABLE_MAX_VERTICES);
	TableWidget_RecreateDescTex(widget);
}

static void TableWidget_Reposition(struct GuiElem* elem) {
	struct TableWidget* widget = (struct TableWidget*)elem;
	Real32 scale = Game_GetInventoryScale();
	widget->BlockSize = (Int32)(50 * Math_SqrtF(scale));
	widget->SelBlockExpand = 25.0f * Math_SqrtF(scale);
	TableWidget_UpdatePos(widget);
	TableWidget_UpdateScrollbarPos(widget);
}

static void TableWidget_ScrollRelative(struct TableWidget* widget, Int32 delta) {
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

static bool TableWidget_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct TableWidget* widget = (struct TableWidget*)elem;
	widget->PendingClose = false;
	if (btn != MouseButton_Left) return false;

	if (Elem_HandlesMouseDown(&widget->Scroll, x, y, btn)) {
		return true;
	} else if (widget->SelectedIndex != -1 && widget->Elements[widget->SelectedIndex] != BLOCK_AIR) {
		Inventory_SetSelectedBlock(widget->Elements[widget->SelectedIndex]);
		widget->PendingClose = true;
		return true;
	} else if (Gui_Contains(Table_X(widget), Table_Y(widget), Table_Width(widget), Table_Height(widget), x, y)) {
		return true;
	}
	return false;
}

static bool TableWidget_HandlesMouseUp(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct TableWidget* widget = (struct TableWidget*)elem;
	return Elem_HandlesMouseUp(&widget->Scroll, x, y, btn);
}

static bool TableWidget_HandlesMouseScroll(struct GuiElem* elem, Real32 delta) {
	struct TableWidget* widget = (struct TableWidget*)elem;
	Int32 scrollWidth = widget->Scroll.Width;
	bool bounds = Gui_Contains(Table_X(widget) - scrollWidth, Table_Y(widget),
		Table_Width(widget) + scrollWidth, Table_Height(widget), Mouse_X, Mouse_Y);
	if (!bounds) return false;

	Int32 startScrollY = widget->Scroll.ScrollY;
	Elem_HandlesMouseScroll(&widget->Scroll, delta);
	if (widget->SelectedIndex == -1) return true;

	Int32 index = widget->SelectedIndex;
	index += (widget->Scroll.ScrollY - startScrollY) * widget->ElementsPerRow;
	if (index >= widget->ElementsCount) index = -1;

	widget->SelectedIndex = index;
	TableWidget_RecreateDescTex(widget);
	return true;
}

static bool TableWidget_HandlesMouseMove(struct GuiElem* elem, Int32 x, Int32 y) {
	struct TableWidget* widget = (struct TableWidget*)elem;
	if (Elem_HandlesMouseMove(&widget->Scroll, x, y)) return true;

	if (widget->LastX == x && widget->LastY == y) return true;
	widget->LastX = x; widget->LastY = y;

	widget->SelectedIndex = -1;
	Int32 blockSize = widget->BlockSize;
	Int32 maxHeight = blockSize * TABLE_MAX_ROWS_DISPLAYED;

	if (Gui_Contains(widget->X, widget->Y + 3, widget->Width, maxHeight - 3 * 2, x, y)) {
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

static bool TableWidget_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct TableWidget* widget = (struct TableWidget*)elem;
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

struct GuiElemVTABLE TableWidget_VTABLE;
void TableWidget_Create(struct TableWidget* widget) {
	widget->VTABLE = &TableWidget_VTABLE;
	Widget_Init((struct Widget*)widget);
	widget->LastCreatedIndex = -1000;

	widget->VTABLE->Init     = TableWidget_Init;
	widget->VTABLE->Render   = TableWidget_Render;
	widget->VTABLE->Free     = TableWidget_Free;
	widget->VTABLE->Recreate = TableWidget_Recreate;
	widget->Reposition       = TableWidget_Reposition;
	
	widget->VTABLE->HandlesMouseDown   = TableWidget_HandlesMouseDown;
	widget->VTABLE->HandlesMouseUp     = TableWidget_HandlesMouseUp;
	widget->VTABLE->HandlesMouseScroll = TableWidget_HandlesMouseScroll;
	widget->VTABLE->HandlesMouseMove   = TableWidget_HandlesMouseMove;
	widget->VTABLE->HandlesKeyDown     = TableWidget_HandlesKeyDown;
}

void TableWidget_SetBlockTo(struct TableWidget* widget, BlockID block) {
	widget->SelectedIndex = -1;
	Int32 i;
	for (i = 0; i < widget->ElementsCount; i++) {
		if (widget->Elements[i] == block) widget->SelectedIndex = i;
	}
	/* When holding air, inventory should open at middle */
	if (block == BLOCK_AIR) widget->SelectedIndex = -1;

	widget->Scroll.ScrollY = widget->SelectedIndex / widget->ElementsPerRow;
	widget->Scroll.ScrollY -= (TABLE_MAX_ROWS_DISPLAYED - 1);
	ScrollbarWidget_ClampScrollY(&widget->Scroll);
	TableWidget_MoveCursorToSelected(widget);
	TableWidget_RecreateDescTex(widget);
}

void TableWidget_OnInventoryChanged(struct TableWidget* widget) {
	TableWidget_RecreateElements(widget);
	if (widget->SelectedIndex >= widget->ElementsCount) {
		widget->SelectedIndex = widget->ElementsCount - 1;
	}
	widget->LastX = -1; widget->LastY = -1;

	widget->Scroll.ScrollY = widget->SelectedIndex / widget->ElementsPerRow;
	ScrollbarWidget_ClampScrollY(&widget->Scroll);
	TableWidget_RecreateDescTex(widget);
}


/*########################################################################################################################*
*-------------------------------------------------------InputWidget-------------------------------------------------------*
*#########################################################################################################################*/
static bool InputWidget_ControlDown(void) {
#if CC_BUILD_OSX
	return Key_IsWinPressed();
#else
	return Key_IsControlPressed();
#endif
}

static void InputWidget_FormatLine(struct InputWidget* widget, Int32 i, STRING_TRANSIENT String* line) {
	if (!widget->ConvertPercents) { String_AppendString(line, &widget->Lines[i]); return; }
	String src = widget->Lines[i];

	for (i = 0; i < src.length; i++) {
		char c = src.buffer[i];
		if (c == '%' && Drawer2D_ValidColCodeAt(&src, i + 1)) { c = '&'; }
		String_Append(line, c);
	}
}

static void InputWidget_CalculateLineSizes(struct InputWidget* widget) {
	Int32 y;
	for (y = 0; y < INPUTWIDGET_MAX_LINES; y++) {
		widget->LineSizes[y] = Size2D_Empty;
	}
	widget->LineSizes[0].Width = widget->PrefixWidth;

	struct DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, true);
	char lineBuffer[STRING_SIZE];
	String line = String_FromArray(lineBuffer);

	for (y = 0; y < widget->GetMaxLines(); y++) {
		line.length = 0;
		InputWidget_FormatLine(widget, y, &line);
		args.Text = line;

		struct Size2D textSize = Drawer2D_MeasureText(&args);
		widget->LineSizes[y].Width += textSize.Width;
		widget->LineSizes[y].Height = textSize.Height;
	}

	if (widget->LineSizes[0].Height == 0) {
		widget->LineSizes[0].Height = widget->PrefixHeight;
	}
}

static char InputWidget_GetLastCol(struct InputWidget* widget, Int32 indexX, Int32 indexY) {
	Int32 x = indexX, y;
	char lineBuffer[STRING_SIZE];
	String line = String_FromArray(lineBuffer);

	for (y = indexY; y >= 0; y--) {
		line.length = 0;
		InputWidget_FormatLine(widget, y, &line);

		char code = Drawer2D_LastCol(&line, x);
		if (code) return code;
		if (y > 0) { x = widget->Lines[y - 1].length; }
	}
	return '\0';
}

static void InputWidget_UpdateCaret(struct InputWidget* widget) {
	Int32 maxChars = widget->GetMaxLines() * INPUTWIDGET_LEN;
	if (widget->CaretPos >= maxChars) widget->CaretPos = -1;
	WordWrap_GetCoords(widget->CaretPos, widget->Lines, widget->GetMaxLines(), &widget->CaretX, &widget->CaretY);
	struct DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, false);
	widget->CaretAccumulator = 0;

	/* Caret is at last character on line */
	if (widget->CaretX == INPUTWIDGET_LEN) {
		widget->CaretTex.X = widget->X + widget->Padding + widget->LineSizes[widget->CaretY].Width;
		PackedCol yellow = PACKEDCOL_YELLOW; widget->CaretCol = yellow;
		widget->CaretTex.Width = widget->CaretWidth;
	} else {
		char lineBuffer[STRING_SIZE];
		String line = String_FromArray(lineBuffer);
		InputWidget_FormatLine(widget, widget->CaretY, &line);

		args.Text = String_UNSAFE_Substring(&line, 0, widget->CaretX);
		struct Size2D trimmedSize = Drawer2D_MeasureText(&args);
		if (widget->CaretY == 0) { trimmedSize.Width += widget->PrefixWidth; }

		widget->CaretTex.X = widget->X + widget->Padding + trimmedSize.Width;
		PackedCol white = PACKEDCOL_WHITE;
		widget->CaretCol = PackedCol_Scale(white, 0.8f);

		if (widget->CaretX < line.length) {
			args.Text = String_UNSAFE_Substring(&line, widget->CaretX, 1);
			args.UseShadow = true;
			widget->CaretTex.Width = Drawer2D_MeasureText(&args).Width;
		} else {
			widget->CaretTex.Width = widget->CaretWidth;
		}
	}
	widget->CaretTex.Y = widget->LineSizes[0].Height * widget->CaretY + widget->InputTex.Y + 2;

	/* Update the colour of the widget->CaretPos */
	char code = InputWidget_GetLastCol(widget, widget->CaretX, widget->CaretY);
	if (code) widget->CaretCol = Drawer2D_Cols[(UInt8)code];
}

static void InputWidget_RenderCaret(struct InputWidget* widget, Real64 delta) {
	if (!widget->ShowCaret) return;

	widget->CaretAccumulator += delta;
	Real32 second = Math_Mod1((Real32)widget->CaretAccumulator);
	if (second < 0.5f) {
		Texture_RenderShaded(&widget->CaretTex, widget->CaretCol);
	}
}

static void InputWidget_OnPressedEnter(struct GuiElem* elem) {
	struct InputWidget* widget = (struct InputWidget*)elem;
	InputWidget_Clear(widget);
	widget->Height = widget->PrefixHeight;
}

void InputWidget_Clear(struct InputWidget* widget) {
	widget->Text.length = 0;
	Int32 i;
	for (i = 0; i < Array_Elems(widget->Lines); i++) {
		widget->Lines[i] = String_MakeNull();
	}

	widget->CaretPos = -1;
	Gfx_DeleteTexture(&widget->InputTex.ID);
}

static bool InputWidget_AllowedChar(struct GuiElem* elem, char c) {
	return Utils_IsValidInputChar(c, ServerConnection_SupportsFullCP437);
}

static void InputWidget_AppendChar(struct InputWidget* widget, char c) {
	if (widget->CaretPos == -1) {
		String_InsertAt(&widget->Text, widget->Text.length, c);
	} else {
		String_InsertAt(&widget->Text, widget->CaretPos, c);
		widget->CaretPos++;
		if (widget->CaretPos >= widget->Text.length) { widget->CaretPos = -1; }
	}
}

static bool InputWidget_TryAppendChar(struct InputWidget* widget, char c) {
	Int32 maxChars = widget->GetMaxLines() * INPUTWIDGET_LEN;
	if (widget->Text.length >= maxChars) return false;
	if (!widget->AllowedChar((struct GuiElem*)widget, c)) return false;

	InputWidget_AppendChar(widget, c);
	return true;
}

void InputWidget_AppendString(struct InputWidget* widget, STRING_PURE String* text) {
	Int32 appended = 0, i;
	for (i = 0; i < text->length; i++) {
		if (InputWidget_TryAppendChar(widget, text->buffer[i])) appended++;
	}

	if (!appended) return;
	Elem_Recreate(widget);
}

void InputWidget_Append(struct InputWidget* widget, char c) {
	if (!InputWidget_TryAppendChar(widget, c)) return;
	Elem_Recreate(widget);
}

static void InputWidget_DeleteChar(struct InputWidget* widget) {
	if (!widget->Text.length) return;

	if (widget->CaretPos == -1) {
		String_DeleteAt(&widget->Text, widget->Text.length - 1);
	} else if (widget->CaretPos > 0) {
		widget->CaretPos--;
		String_DeleteAt(&widget->Text, widget->CaretPos);
	}
}

static bool InputWidget_CheckCol(struct InputWidget* widget, Int32 index) {
	if (index < 0) return false;
	char code = widget->Text.buffer[index];
	char col  = widget->Text.buffer[index + 1];
	return (code == '%' || code == '&') && Drawer2D_ValidColCode(col);
}

static void InputWidget_BackspaceKey(struct InputWidget* widget) {
	if (InputWidget_ControlDown()) {
		if (widget->CaretPos == -1) { widget->CaretPos = widget->Text.length - 1; }
		Int32 len = WordWrap_GetBackLength(&widget->Text, widget->CaretPos);
		if (!len) return;

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
		Elem_Recreate(widget);
	} else if (widget->Text.length > 0 && widget->CaretPos != 0) {
		Int32 index = widget->CaretPos == -1 ? widget->Text.length - 1 : widget->CaretPos;
		if (InputWidget_CheckCol(widget, index - 1)) {
			InputWidget_DeleteChar(widget); /* backspace XYZ%e to XYZ */
		} else if (InputWidget_CheckCol(widget, index - 2)) {
			InputWidget_DeleteChar(widget); 
			InputWidget_DeleteChar(widget); /* backspace XYZ%eH to XYZ */
		}

		InputWidget_DeleteChar(widget);
		Elem_Recreate(widget);
	}
}

static void InputWidget_DeleteKey(struct InputWidget* widget) {
	if (widget->Text.length > 0 && widget->CaretPos != -1) {
		String_DeleteAt(&widget->Text, widget->CaretPos);
		if (widget->CaretPos >= widget->Text.length) { widget->CaretPos = -1; }
		Elem_Recreate(widget);
	}
}

static void InputWidget_LeftKey(struct InputWidget* widget) {
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

static void InputWidget_RightKey(struct InputWidget* widget) {
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

static void InputWidget_HomeKey(struct InputWidget* widget) {
	if (!widget->Text.length) return;
	widget->CaretPos = 0;
	InputWidget_UpdateCaret(widget);
}

static void InputWidget_EndKey(struct InputWidget* widget) {
	widget->CaretPos = -1;
	InputWidget_UpdateCaret(widget);
}

static bool InputWidget_OtherKey(struct InputWidget* widget, Key key) {
	Int32 maxChars = widget->GetMaxLines() * INPUTWIDGET_LEN;
	if (!InputWidget_ControlDown()) return false;

	if (key == Key_V && widget->Text.length < maxChars) {
		char textBuffer[INPUTWIDGET_MAX_LINES * STRING_SIZE];
		String text = String_FromArray(textBuffer);
		Window_GetClipboardText(&text);

		String_UNSAFE_TrimStart(&text);
		String_UNSAFE_TrimEnd(&text);

		if (!text.length) return true;
		InputWidget_AppendString(widget, &text);
		return true;
	} else if (key == Key_C) {
		if (!widget->Text.length) return true;
		Window_SetClipboardText(&widget->Text);
		return true;
	}
	return false;
}

static void InputWidget_Init(struct GuiElem* elem) {
	struct InputWidget* widget = (struct InputWidget*)elem;
	Int32 lines = widget->GetMaxLines();
	if (lines > 1) {
		WordWrap_Do(&widget->Text, widget->Lines, lines, INPUTWIDGET_LEN);
	} else {
		widget->Lines[0] = widget->Text;
	}

	InputWidget_CalculateLineSizes(widget);
	widget->RemakeTexture(elem);
	InputWidget_UpdateCaret(widget);
}

static void InputWidget_Free(struct GuiElem* elem) {
	struct InputWidget* widget = (struct InputWidget*)elem;
	Gfx_DeleteTexture(&widget->InputTex.ID);
	Gfx_DeleteTexture(&widget->CaretTex.ID);
}

static void InputWidget_Recreate(struct GuiElem* elem) {
	struct InputWidget* widget = (struct InputWidget*)elem;
	Gfx_DeleteTexture(&widget->InputTex.ID);
	InputWidget_Init(elem);
}

static void InputWidget_Reposition(struct GuiElem* elem) {
	struct InputWidget* widget = (struct InputWidget*)elem;
	Int32 oldX = widget->X, oldY = widget->Y;
	Widget_DoReposition(elem);
	
	widget->CaretTex.X += widget->X - oldX; widget->CaretTex.Y += widget->Y - oldY;
	widget->InputTex.X += widget->X - oldX; widget->InputTex.Y += widget->Y - oldY;
}

static bool InputWidget_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct InputWidget* widget = (struct InputWidget*)elem;

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
	} else if (!InputWidget_OtherKey(widget, key)) {
		return false;
	}
	return true;
}

static bool InputWidget_HandlesKeyUp(struct GuiElem* elem, Key key) { return true; }

static bool InputWidget_HandlesKeyPress(struct GuiElem* elem, UInt8 key) {
	struct InputWidget* widget = (struct InputWidget*)elem;
	InputWidget_Append(widget, key);
	return true;
}

static bool InputWidget_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton button) {
	struct InputWidget* widget = (struct InputWidget*)elem;
	if (button != MouseButton_Left) return true;

	x -= widget->InputTex.X; y -= widget->InputTex.Y;
	struct DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, true);
	Int32 offset = 0, charHeight = widget->CaretTex.Height;

	char lineBuffer[STRING_SIZE];
	String line = String_FromArray(lineBuffer);
	Int32 charX, charY;

	for (charY = 0; charY < widget->GetMaxLines(); charY++) {
		line.length = 0;
		InputWidget_FormatLine(widget, charY, &line);
		if (!line.length) continue;

		for (charX = 0; charX < line.length; charX++) {
			args.Text = String_UNSAFE_Substring(&line, 0, charX);
			Int32 charOffset = Drawer2D_MeasureText(&args).Width;
			if (charY == 0) charOffset += widget->PrefixWidth;

			args.Text = String_UNSAFE_Substring(&line, charX, 1);
			Int32 charWidth = Drawer2D_MeasureText(&args).Width;

			if (Gui_Contains(charOffset, charY * charHeight, charWidth, charHeight, x, y)) {
				widget->CaretPos = offset + charX;
				InputWidget_UpdateCaret(widget);
				return true;
			}
		}
		offset += line.length;
	}

	widget->CaretPos = -1;
	InputWidget_UpdateCaret(widget);
	return true;
}

struct GuiElemVTABLE InputWidget_VTABLE;
void InputWidget_Create(struct InputWidget* widget, struct FontDesc* font, STRING_REF String* prefix) {
	widget->VTABLE = &InputWidget_VTABLE;
	Widget_Init((struct Widget*)widget);
	widget->Font            = *font;
	widget->Prefix          = *prefix;
	widget->CaretPos        = -1;
	widget->OnPressedEnter  = InputWidget_OnPressedEnter;
	widget->AllowedChar     = InputWidget_AllowedChar;	

	widget->VTABLE->Init     = InputWidget_Init;
	widget->VTABLE->Free     = InputWidget_Free;
	widget->VTABLE->Recreate = InputWidget_Recreate;
	widget->Reposition       = InputWidget_Reposition;

	widget->VTABLE->HandlesKeyDown   = InputWidget_HandlesKeyDown;
	widget->VTABLE->HandlesKeyUp     = InputWidget_HandlesKeyUp;
	widget->VTABLE->HandlesKeyPress  = InputWidget_HandlesKeyPress;
	widget->VTABLE->HandlesMouseDown = InputWidget_HandlesMouseDown;

	String caret = String_FromConst("_");
	struct DrawTextArgs args; DrawTextArgs_Make(&args, &caret, font, true);
	Drawer2D_MakeTextTexture(&widget->CaretTex, &args, 0, 0);
	widget->CaretTex.Width = (UInt16)((widget->CaretTex.Width * 3) / 4);
	widget->CaretWidth     = (UInt16)widget->CaretTex.Width;

	if (!prefix->length) return;
	DrawTextArgs_Make(&args, prefix, font, true);
	struct Size2D size = Drawer2D_MeasureText(&args);
	widget->PrefixWidth  = size.Width;  widget->Width  = size.Width;
	widget->PrefixHeight = size.Height; widget->Height = size.Height;
}


/*########################################################################################################################*
*---------------------------------------------------MenuInputValidator----------------------------------------------------*
*#########################################################################################################################*/
static bool MenuInputValidator_AlwaysValidString(struct MenuInputValidator* v, STRING_PURE String* s) { return true; }

static void HexColValidator_GetRange(struct MenuInputValidator* v, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(#000000 - #FFFFFF)");
}

static bool HexColValidator_IsValidChar(struct MenuInputValidator* v, char c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static bool HexColValidator_IsValidString(struct MenuInputValidator* v, STRING_PURE String* s) {
	return s->length <= 6;
}

static bool HexColValidator_IsValidValue(struct MenuInputValidator* v, STRING_PURE String* s) {
	PackedCol col;
	return PackedCol_TryParseHex(s, &col);
}

struct MenuInputValidator MenuInputValidator_Hex(void) {
	struct MenuInputValidator validator;
	validator.GetRange      = HexColValidator_GetRange;
	validator.IsValidChar   = HexColValidator_IsValidChar;
	validator.IsValidString = HexColValidator_IsValidString;
	validator.IsValidValue  = HexColValidator_IsValidValue;
	return validator;
}

static void IntegerValidator_GetRange(struct MenuInputValidator* v, STRING_TRANSIENT String* range) {
	String_Format2(range, "&7(%i - %i)", &v->Meta_Int[0], &v->Meta_Int[1]);
}

static bool IntegerValidator_IsValidChar(struct MenuInputValidator* v, char c) {
	return (c >= '0' && c <= '9') || c == '-';
}

static bool IntegerValidator_IsValidString(struct MenuInputValidator* v, STRING_PURE String* s) {
	Int32 value;
	if (s->length == 1 && s->buffer[0] == '-') return true; /* input is just a minus sign */
	return Convert_TryParseInt32(s, &value);
}

static bool IntegerValidator_IsValidValue(struct MenuInputValidator* v, STRING_PURE String* s) {
	Int32 value;
	if (!Convert_TryParseInt32(s, &value)) return false;

	Int32 min = v->Meta_Int[0], max = v->Meta_Int[1];
	return min <= value && value <= max;
}

struct MenuInputValidator MenuInputValidator_Integer(Int32 min, Int32 max) {
	struct MenuInputValidator validator;
	validator.GetRange      = IntegerValidator_GetRange;
	validator.IsValidChar   = IntegerValidator_IsValidChar;
	validator.IsValidString = IntegerValidator_IsValidString;
	validator.IsValidValue  = IntegerValidator_IsValidValue;

	validator.Meta_Int[0] = min;
	validator.Meta_Int[1] = max;
	return validator;
}

static void SeedValidator_GetRange(struct MenuInputValidator* v, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(an integer)");
}

struct MenuInputValidator MenuInputValidator_Seed(void) {
	struct MenuInputValidator validator = MenuInputValidator_Integer(Int32_MinValue, Int32_MaxValue);
	validator.GetRange = SeedValidator_GetRange;
	return validator;
}

static void RealValidator_GetRange(struct MenuInputValidator* v, STRING_TRANSIENT String* range) {
	String_Format2(range, "&7(%f2 - %f2)", &v->Meta_Real[0], &v->Meta_Real[1]);
}

static bool RealValidator_IsValidChar(struct MenuInputValidator* v, char c) {
	return (c >= '0' && c <= '9') || c == '-' || c == '.' || c == ',';
}

static bool RealValidator_IsValidString(struct MenuInputValidator* v, STRING_PURE String* s) {
	Real32 value;
	if (s->length == 1 && RealValidator_IsValidChar(v, s->buffer[0])) return true;
	return Convert_TryParseReal32(s, &value);
}

static bool RealValidator_IsValidValue(struct MenuInputValidator* v, STRING_PURE String* s) {
	Real32 value;
	if (!Convert_TryParseReal32(s, &value)) return false;
	Real32 min = v->Meta_Real[0], max = v->Meta_Real[1];
	return min <= value && value <= max;
}

struct MenuInputValidator MenuInputValidator_Real(Real32 min, Real32 max) {
	struct MenuInputValidator validator;
	validator.GetRange      = RealValidator_GetRange;
	validator.IsValidChar   = RealValidator_IsValidChar;
	validator.IsValidString = RealValidator_IsValidString;
	validator.IsValidValue  = RealValidator_IsValidValue;
	validator.Meta_Real[0] = min;
	validator.Meta_Real[1] = max;
	return validator;
}

static void PathValidator_GetRange(struct MenuInputValidator* validator, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(Enter name)");
}

static bool PathValidator_IsValidChar(struct MenuInputValidator* validator, char c) {
	return !(c == '/' || c == '\\' || c == '?' || c == '*' || c == ':'
		|| c == '<' || c == '>' || c == '|' || c == '"' || c == '.');
}

struct MenuInputValidator MenuInputValidator_Path(void) {
	struct MenuInputValidator validator;
	validator.GetRange      = PathValidator_GetRange;
	validator.IsValidChar   = PathValidator_IsValidChar;
	validator.IsValidString = MenuInputValidator_AlwaysValidString;
	validator.IsValidValue  = MenuInputValidator_AlwaysValidString;
	return validator;
}

struct MenuInputValidator MenuInputValidator_Enum(const char** names, UInt32 namesCount) {
	struct MenuInputValidator validator = { 0 };
	validator.Meta_Ptr[0] = names;
	validator.Meta_Ptr[1] = (void*)namesCount; /* TODO: Need to handle void* size < 32 bits?? */
	return validator;
}

static void StringValidator_GetRange(struct MenuInputValidator* validator, STRING_TRANSIENT String* range) {
	String_AppendConst(range, "&7(Enter text)");
}

static bool StringValidator_IsValidChar(struct MenuInputValidator* validator, char c) {
	return c != '&' && Utils_IsValidInputChar(c, true);
}

static bool StringValidator_IsValidString(struct MenuInputValidator* validator, STRING_PURE String* s) {
	return s->length <= STRING_SIZE;
}

struct MenuInputValidator MenuInputValidator_String(void) {
	struct MenuInputValidator validator;
	validator.GetRange      = StringValidator_GetRange;
	validator.IsValidChar   = StringValidator_IsValidChar;
	validator.IsValidString = StringValidator_IsValidString;
	validator.IsValidValue  = StringValidator_IsValidString;
	return validator;
}


/*########################################################################################################################*
*-----------------------------------------------------MenuInputWidget-----------------------------------------------------*
*#########################################################################################################################*/
static void MenuInputWidget_Render(struct GuiElem* elem, Real64 delta) {
	struct InputWidget* widget = (struct InputWidget*)elem;
	PackedCol backCol = PACKEDCOL_CONST(30, 30, 30, 200);

	Gfx_SetTexturing(false);
	GfxCommon_Draw2DFlat(widget->X, widget->Y, widget->Width, widget->Height, backCol);
	Gfx_SetTexturing(true);

	Texture_Render(&widget->InputTex);
	InputWidget_RenderCaret(widget, delta);
}

static void MenuInputWidget_RemakeTexture(struct GuiElem* elem) {
	struct MenuInputWidget* widget = (struct MenuInputWidget*)elem;

	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &widget->Base.Lines[0], &widget->Base.Font, false);
	struct Size2D size = Drawer2D_MeasureText(&args);
	widget->Base.CaretAccumulator = 0.0;

	char rangeBuffer[STRING_SIZE];
	String range = String_FromArray(rangeBuffer);
	struct MenuInputValidator* validator = &widget->Validator;
	validator->GetRange(validator, &range);

	/* Ensure we don't have 0 text height */
	if (size.Height == 0) {
		args.Text = range;
		size.Height = Drawer2D_MeasureText(&args).Height;
		args.Text = widget->Base.Lines[0];
	}

	widget->Base.Width  = max(size.Width,  widget->MinWidth);
	widget->Base.Height = max(size.Height, widget->MinHeight);
	struct Size2D adjSize = size; adjSize.Width = widget->Base.Width;

	struct Bitmap bmp; Bitmap_AllocateClearedPow2(&bmp, adjSize.Width, adjSize.Height);
	Drawer2D_Begin(&bmp);
	{
		Drawer2D_DrawText(&args, widget->Base.Padding, 0);

		args.Text = range;
		struct Size2D hintSize = Drawer2D_MeasureText(&args);
		Int32 hintX = adjSize.Width - hintSize.Width;
		if (size.Width + 3 < hintX) {
			Drawer2D_DrawText(&args, hintX, 0);
		}
	}
	Drawer2D_End();

	struct Texture* tex = &widget->Base.InputTex;
	Drawer2D_Make2DTexture(tex, &bmp, adjSize, 0, 0);
	Mem_Free(bmp.Scan0);

	Widget_Reposition(&widget->Base);
	tex->X = widget->Base.X; tex->Y = widget->Base.Y;
	if (size.Height < widget->MinHeight) {
		tex->Y += widget->MinHeight / 2 - size.Height / 2;
	}
}

static bool MenuInputWidget_AllowedChar(struct GuiElem* elem, char c) {
	if (c == '&' || !Utils_IsValidInputChar(c, true)) return false;
	struct MenuInputWidget* widget = (struct MenuInputWidget*)elem;
	struct InputWidget* elemW = (struct InputWidget*)elem;
	struct MenuInputValidator* validator = &widget->Validator;

	if (!validator->IsValidChar(validator, c)) return false;
	Int32 maxChars = elemW->GetMaxLines() * INPUTWIDGET_LEN;
	if (elemW->Text.length == maxChars) return false;

	/* See if the new string is in valid format */
	InputWidget_AppendChar(elemW, c);
	bool valid = validator->IsValidString(validator, &elemW->Text);
	InputWidget_DeleteChar(elemW);
	return valid;
}

static Int32 MenuInputWidget_GetMaxLines(void) { return 1; }
struct GuiElemVTABLE MenuInputWidget_VTABLE;
void MenuInputWidget_Create(struct MenuInputWidget* widget, Int32 width, Int32 height, STRING_PURE String* text, struct FontDesc* font, struct MenuInputValidator* validator) {
	String empty = String_MakeNull();
	InputWidget_Create(&widget->Base, font, &empty);
	widget->MinWidth  = width;
	widget->MinHeight = height;
	widget->Validator = *validator;

	widget->Base.ConvertPercents = false;
	widget->Base.Padding = 3;
	String inputStr   = String_FromArray(widget->__TextBuffer);
	widget->Base.Text = inputStr;

	widget->Base.GetMaxLines   = MenuInputWidget_GetMaxLines;
	widget->Base.RemakeTexture = MenuInputWidget_RemakeTexture;
	widget->Base.AllowedChar   = MenuInputWidget_AllowedChar;

	MenuInputWidget_VTABLE = *widget->Base.VTABLE;
	widget->Base.VTABLE = &MenuInputWidget_VTABLE;
	widget->Base.VTABLE->Render = MenuInputWidget_Render;
	Elem_Init(&widget->Base);
	InputWidget_AppendString(&widget->Base, text);
}


/*########################################################################################################################*
*-----------------------------------------------------ChatInputWidget-----------------------------------------------------*
*#########################################################################################################################*/
static void ChatInputWidget_RemakeTexture(struct GuiElem* elem) {
	struct InputWidget* widget = (struct InputWidget*)elem;
	Int32 totalHeight = 0, maxWidth = 0, i;
	for (i = 0; i < widget->GetMaxLines(); i++) {
		totalHeight += widget->LineSizes[i].Height;
		maxWidth = max(maxWidth, widget->LineSizes[i].Width);
	}
	struct Size2D size = { maxWidth, totalHeight };
	widget->CaretAccumulator = 0;

	Int32 realHeight = 0;
	struct Bitmap bmp; Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
	Drawer2D_Begin(&bmp);

	struct DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, true);
	if (widget->Prefix.length) {
		args.Text = widget->Prefix;
		Drawer2D_DrawText(&args, 0, 0);
	}

	char lineBuffer[STRING_SIZE + 2];
	String line = String_FromArray(lineBuffer);

	for (i = 0; i < Array_Elems(widget->Lines); i++) {
		if (!widget->Lines[i].length) break;
		line.length = 0;

		/* Colour code goes to next line */
		char lastCol = InputWidget_GetLastCol(widget, 0, i);
		if (!Drawer2D_IsWhiteCol(lastCol)) {			
			String_Append(&line, '&'); String_Append(&line, lastCol);
		}
		/* Convert % to & for colour codes */
		InputWidget_FormatLine(widget, i, &line);

		args.Text = line;
		Int32 offset = i == 0 ? widget->PrefixWidth : 0;
		Drawer2D_DrawText(&args, offset, realHeight);
		realHeight += widget->LineSizes[i].Height;
	}
	Drawer2D_End();

	Drawer2D_Make2DTexture(&widget->InputTex, &bmp, size, 0, 0);
	Mem_Free(bmp.Scan0);

	widget->Width = size.Width;
	widget->Height = realHeight == 0 ? widget->PrefixHeight : realHeight;
	Widget_Reposition(widget);
	widget->InputTex.X = widget->X + widget->Padding;
	widget->InputTex.Y = widget->Y;
}

static void ChatInputWidget_Render(struct GuiElem* elem, Real64 delta) {
	struct InputWidget* input = (struct InputWidget*)elem;
	Gfx_SetTexturing(false);
	Int32 x = input->X, y = input->Y;

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

static void ChatInputWidget_OnPressedEnter(struct GuiElem* elem) {
	struct ChatInputWidget* widget = (struct ChatInputWidget*)elem;

	/* Don't want trailing spaces in output message */
	String text = widget->Base.Text;
	while (text.length && text.buffer[text.length - 1] == ' ') { text.length--; }
	if (text.length) { Chat_Send(&text, true); }

	widget->OrigStr.length = 0;
	widget->TypingLogPos = Chat_InputLog.Count; /* Index of newest entry + 1. */

	String empty = String_MakeNull();
	Chat_AddOf(&empty, MSG_TYPE_CLIENTSTATUS_2);
	Chat_AddOf(&empty, MSG_TYPE_CLIENTSTATUS_3);
	InputWidget_OnPressedEnter(elem);
}

static void ChatInputWidget_UpKey(struct GuiElem* elem) {
	struct ChatInputWidget* widget = (struct ChatInputWidget*)elem;
	struct InputWidget* input = (struct InputWidget*)elem;

	if (InputWidget_ControlDown()) {
		Int32 pos = input->CaretPos == -1 ? input->Text.length : input->CaretPos;
		if (pos < INPUTWIDGET_LEN) return;

		input->CaretPos = pos - INPUTWIDGET_LEN;
		InputWidget_UpdateCaret(input);
		return;
	}

	if (widget->TypingLogPos == Chat_InputLog.Count) {
		String_Copy(&widget->OrigStr, &input->Text);
	}

	if (!Chat_InputLog.Count) return;
	widget->TypingLogPos--;
	input->Text.length = 0;

	if (widget->TypingLogPos < 0) widget->TypingLogPos = 0;
	String prevInput = StringsBuffer_UNSAFE_Get(&Chat_InputLog, widget->TypingLogPos);
	String_AppendString(&input->Text, &prevInput);

	input->CaretPos = -1;
	Elem_Recreate(&widget->Base);
}

static void ChatInputWidget_DownKey(struct GuiElem* elem) {
	struct ChatInputWidget* widget = (struct ChatInputWidget*)elem;
	struct InputWidget* input = (struct InputWidget*)elem;

	if (InputWidget_ControlDown()) {
		Int32 lines = input->GetMaxLines();
		if (input->CaretPos == -1 || input->CaretPos >= (lines - 1) * INPUTWIDGET_LEN) return;

		input->CaretPos += INPUTWIDGET_LEN;
		InputWidget_UpdateCaret(input);
		return;
	}

	if (!Chat_InputLog.Count) return;
	widget->TypingLogPos++;
	input->Text.length = 0;

	if (widget->TypingLogPos >= Chat_InputLog.Count) {
		widget->TypingLogPos = Chat_InputLog.Count;
		String* orig = &widget->OrigStr;
		if (orig->length) { String_AppendString(&input->Text, orig); }
	} else {
		String prevInput = StringsBuffer_UNSAFE_Get(&Chat_InputLog, widget->TypingLogPos);
		String_AppendString(&input->Text, &prevInput);
	}

	input->CaretPos = -1;
	Elem_Recreate(input);
}

static bool ChatInputWidget_IsNameChar(char c) {
	return c == '_' || c == '.' || (c >= '0' && c <= '9')
		|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void ChatInputWidget_TabKey(struct GuiElem* elem) {
	struct InputWidget* input = (struct InputWidget*)elem;

	Int32 end = input->CaretPos == -1 ? input->Text.length - 1 : input->CaretPos;
	Int32 start = end;
	char* buffer = input->Text.buffer;

	while (start >= 0 && ChatInputWidget_IsNameChar(buffer[start])) { start--; }
	start++;
	if (end < 0 || start > end) return;

	String part = String_UNSAFE_Substring(&input->Text, start, (end + 1) - start);
	String empty = String_MakeNull();
	Chat_AddOf(&empty, MSG_TYPE_CLIENTSTATUS_3);

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
		char strBuffer[STRING_SIZE];
		String str = String_FromArray(strBuffer);
		String_Format1(&str, "&e%i matching names: ", &matchesCount);

		for (i = 0; i < matchesCount; i++) {
			String match = TabList_UNSAFE_GetPlayer(matches[i]);
			if ((str.length + match.length + 1) > STRING_SIZE) break;

			String_AppendString(&str, &match);
			String_Append(&str, ' ');
		}
		Chat_AddOf(&str, MSG_TYPE_CLIENTSTATUS_3);
	}
}

static bool ChatInputWidget_HandlesKeyDown(struct GuiElem* elem, Key key) {
	if (key == Key_Tab)  { ChatInputWidget_TabKey(elem);  return true; }
	if (key == Key_Up)   { ChatInputWidget_UpKey(elem);   return true; }
	if (key == Key_Down) { ChatInputWidget_DownKey(elem); return true; }
	return InputWidget_HandlesKeyDown(elem, key);
}

static Int32 ChatInputWidget_GetMaxLines(void) {
	return !Game_ClassicMode && ServerConnection_SupportsPartialMessages ? 3 : 1;
}

struct GuiElemVTABLE ChatInputWidget_VTABLE;
void ChatInputWidget_Create(struct ChatInputWidget* widget, struct FontDesc* font) {
	String prefix = String_FromConst("> ");
	InputWidget_Create(&widget->Base, font, &prefix);
	widget->TypingLogPos = Chat_InputLog.Count; /* Index of newest entry + 1. */

	widget->Base.ConvertPercents = true;
	widget->Base.ShowCaret       = true;
	widget->Base.Padding         = 5;
	widget->Base.GetMaxLines    = ChatInputWidget_GetMaxLines;
	widget->Base.RemakeTexture  = ChatInputWidget_RemakeTexture;
	widget->Base.OnPressedEnter = ChatInputWidget_OnPressedEnter;

	String inputStr   = String_FromArray(widget->__TextBuffer);
	widget->Base.Text = inputStr;
	String origStr    = String_FromArray(widget->__OrigBuffer);
	widget->OrigStr   = origStr;

	ChatInputWidget_VTABLE = *widget->Base.VTABLE;
	widget->Base.VTABLE = &ChatInputWidget_VTABLE;
	widget->Base.VTABLE->Render         = ChatInputWidget_Render;
	widget->Base.VTABLE->HandlesKeyDown = ChatInputWidget_HandlesKeyDown;
}


/*########################################################################################################################*
*----------------------------------------------------PlayerListWidget-----------------------------------------------------*
*#########################################################################################################################*/
#define GROUP_NAME_ID UInt16_MaxValue
#define LIST_COLUMN_PADDING 5
#define LIST_BOUNDS_SIZE 10
#define LIST_NAMES_PER_COLUMN 16

static void PlayerListWidget_DrawName(struct Texture* tex, struct PlayerListWidget* widget, STRING_PURE String* name) {
	char tmpBuffer[STRING_SIZE];
	String tmp = String_FromArray(tmpBuffer);
	if (Game_PureClassic) {
		String_AppendColorless(&tmp, name);
	} else {
		tmp = *name;
	}

	struct DrawTextArgs args; DrawTextArgs_Make(&args, &tmp, &widget->Font, !widget->Classic);
	Drawer2D_MakeTextTexture(tex, &args, 0, 0);
	Drawer2D_ReducePadding_Tex(tex, widget->Font.Size, 3);
}

static Int32 PlayerListWidget_HighlightedName(struct PlayerListWidget* widget, Int32 mouseX, Int32 mouseY) {
	if (!widget->Active) return -1;
	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		if (!widget->Textures[i].ID || widget->IDs[i] == GROUP_NAME_ID) continue;

		struct Texture t = widget->Textures[i];
		if (Gui_Contains(t.X, t.Y, t.Width, t.Height, mouseX, mouseY)) return i;
	}
	return -1;
}

void PlayerListWidget_GetNameUnder(struct PlayerListWidget* widget, Int32 mouseX, Int32 mouseY, STRING_TRANSIENT String* name) {
	Int32 i = PlayerListWidget_HighlightedName(widget, mouseX, mouseY);
	if (i == -1) return;

	String player = TabList_UNSAFE_GetPlayer(widget->IDs[i]);
	String_AppendString(name, &player);
}

static void PlayerListWidget_UpdateTableDimensions(struct PlayerListWidget* widget) {
	Int32 width = widget->XMax - widget->XMin, height = widget->YHeight;
	widget->X = (widget->XMin                ) - LIST_BOUNDS_SIZE;
	widget->Y = (Game_Height / 2 - height / 2) - LIST_BOUNDS_SIZE;
	widget->Width  = width  + LIST_BOUNDS_SIZE * 2;
	widget->Height = height + LIST_BOUNDS_SIZE * 2;
}

static Int32 PlayerListWidget_GetColumnWidth(struct PlayerListWidget* widget, Int32 column) {
	Int32 i = column * LIST_NAMES_PER_COLUMN;
	Int32 maxWidth = 0;
	Int32 maxIndex = min(widget->NamesCount, i + LIST_NAMES_PER_COLUMN);

	for (; i < maxIndex; i++) {
		maxWidth = max(maxWidth, widget->Textures[i].Width);
	}
	return maxWidth + LIST_COLUMN_PADDING + widget->ElementOffset;
}

static Int32 PlayerListWidget_GetColumnHeight(struct PlayerListWidget* widget, Int32 column) {
	Int32 i = column * LIST_NAMES_PER_COLUMN;
	Int32 total = 0;
	Int32 maxIndex = min(widget->NamesCount, i + LIST_NAMES_PER_COLUMN);

	for (; i < maxIndex; i++) {
		total += widget->Textures[i].Height + 1;
	}
	return total;
}

static void PlayerListWidget_SetColumnPos(struct PlayerListWidget* widget, Int32 column, Int32 x, Int32 y) {
	Int32 i = column * LIST_NAMES_PER_COLUMN;
	Int32 maxIndex = min(widget->NamesCount, i + LIST_NAMES_PER_COLUMN);

	for (; i < maxIndex; i++) {
		struct Texture tex = widget->Textures[i];
		tex.X = x; tex.Y = y - 10;

		y += tex.Height + 1;
		/* offset player names a bit, compared to group name */
		if (!widget->Classic && widget->IDs[i] != GROUP_NAME_ID) {
			tex.X += widget->ElementOffset;
		}
		widget->Textures[i] = tex;
	}
}

static void PlayerListWidget_RepositionColumns(struct PlayerListWidget* widget) {
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

static void PlayerListWidget_Reposition(struct GuiElem* elem) {
	struct PlayerListWidget* widget = (struct PlayerListWidget*)elem;
	Int32 yPosition = Game_Height / 4 - widget->Height / 2;
	widget->YOffset = -max(0, yPosition);

	Int32 oldX = widget->X, oldY = widget->Y;
	Widget_DoReposition(elem);	

	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		widget->Textures[i].X += widget->X - oldX;
		widget->Textures[i].Y += widget->Y - oldY;
	}
}

static void PlayerListWidget_AddName(struct PlayerListWidget* widget, EntityID id, Int32 index) {
	/* insert at end of list */
	if (index == -1) { index = widget->NamesCount; widget->NamesCount++; }

	String name = TabList_UNSAFE_GetList(id);
	widget->IDs[index]      = id;
	PlayerListWidget_DrawName(&widget->Textures[index], widget, &name);
}

static void PlayerListWidget_DeleteAt(struct PlayerListWidget* widget, Int32 i) {
	struct Texture tex = widget->Textures[i];
	Gfx_DeleteTexture(&tex.ID);
	struct Texture empty = { 0 };

	for (; i < widget->NamesCount - 1; i++) {
		widget->IDs[i]      = widget->IDs[i + 1];
		widget->Textures[i] = widget->Textures[i + 1];
	}

	widget->IDs[widget->NamesCount]      = 0;
	widget->Textures[widget->NamesCount] = empty;
	widget->NamesCount--;
}

static void PlayerListWidget_DeleteGroup(struct PlayerListWidget* widget, Int32* i) {
	PlayerListWidget_DeleteAt(widget, *i);
	(*i)--;
}

static void PlayerListWidget_AddGroup(struct PlayerListWidget* widget, UInt16 id, Int32* index) {
	Int32 i;
	for (i = Array_Elems(widget->IDs) - 1; i > (*index); i--) {
		widget->IDs[i] = widget->IDs[i - 1];
		widget->Textures[i] = widget->Textures[i - 1];
	}

	String group = TabList_UNSAFE_GetGroup(id);
	widget->IDs[*index] = GROUP_NAME_ID;
	PlayerListWidget_DrawName(&widget->Textures[*index], widget, &group);

	(*index)++;
	widget->NamesCount++;
}

static Int32 PlayerListWidget_GetGroupCount(struct PlayerListWidget* widget, UInt16 id, Int32 idx) {
	String group = TabList_UNSAFE_GetGroup(id);
	Int32 count = 0;

	while (idx < widget->NamesCount) {
		String curGroup = TabList_UNSAFE_GetGroup(widget->IDs[idx]);
		if (!String_CaselessEquals(&group, &curGroup)) return count;
		idx++; count++;
	}
	return count;
}

static Int32 PlayerListWidget_PlayerCompare(UInt16 x, UInt16 y) {
	UInt8 xRank = TabList_GroupRanks[x];
	UInt8 yRank = TabList_GroupRanks[y];
	if (xRank != yRank) return (xRank < yRank ? -1 : 1);

	char xNameBuffer[STRING_SIZE];
	String xName    = String_FromArray(xNameBuffer);
	String xNameRaw = TabList_UNSAFE_GetList(x);
	String_AppendColorless(&xName, &xNameRaw);

	char yNameBuffer[STRING_SIZE];
	String yName    = String_FromArray(yNameBuffer);
	String yNameRaw = TabList_UNSAFE_GetList(y);
	String_AppendColorless(&yName, &yNameRaw);

	return String_Compare(&xName, &yName);
}

static Int32 PlayerListWidget_GroupCompare(UInt16 x, UInt16 y) {
	/* TODO: should we use colourless comparison? ClassicalSharp sorts groups with colours */
	String xGroup = TabList_UNSAFE_GetGroup(x);
	String yGroup = TabList_UNSAFE_GetGroup(y);
	return String_Compare(&xGroup, &yGroup);
}

struct PlayerListWidget* List_SortObj;
Int32 (*List_SortCompare)(UInt16 x, UInt16 y);
static void PlayerListWidget_QuickSort(Int32 left, Int32 right) {
	struct Texture* values = List_SortObj->Textures; struct Texture value;
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

static void PlayerListWidget_SortEntries(struct PlayerListWidget* widget) {
	if (!widget->NamesCount) return;
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

static void PlayerListWidget_SortAndReposition(struct PlayerListWidget* widget) {
	PlayerListWidget_SortEntries(widget);
	PlayerListWidget_RepositionColumns(widget);
	PlayerListWidget_UpdateTableDimensions(widget);
	PlayerListWidget_Reposition((struct GuiElem*)widget);
}

static void PlayerListWidget_TabEntryAdded(void* obj, Int32 id) {
	struct PlayerListWidget* widget = (struct PlayerListWidget*)obj;
	PlayerListWidget_AddName(widget, id, -1);
	PlayerListWidget_SortAndReposition(widget);
}

static void PlayerListWidget_TabEntryChanged(void* obj, Int32 id) {
	struct PlayerListWidget* widget = (struct PlayerListWidget*)obj;
	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		if (widget->IDs[i] != id) continue;

		struct Texture tex = widget->Textures[i];
		Gfx_DeleteTexture(&tex.ID);
		PlayerListWidget_AddName(widget, id, i);
		PlayerListWidget_SortAndReposition(widget);
		return;
	}
}

static void PlayerListWidget_TabEntryRemoved(void* obj, Int32 id) {
	struct PlayerListWidget* widget = (struct PlayerListWidget*)obj;
	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		if (widget->IDs[i] != id) continue;
		PlayerListWidget_DeleteAt(widget, i);
		PlayerListWidget_SortAndReposition(widget);
		return;
	}
}

static void PlayerListWidget_Init(struct GuiElem* elem) {
	struct PlayerListWidget* widget = (struct PlayerListWidget*)elem;
	Int32 id;
	for (id = 0; id < TABLIST_MAX_NAMES; id++) {
		if (!TabList_Valid((EntityID)id)) continue;
		PlayerListWidget_AddName(widget, (EntityID)id, -1);
	}
	PlayerListWidget_SortAndReposition(widget);

	String msg = String_FromConst("Connected players:");
	TextWidget_Create(&widget->Overview, &msg, &widget->Font);
	Widget_SetLocation((struct Widget*)(&widget->Overview), ANCHOR_CENTRE, ANCHOR_MIN, 0, 0);

	Event_RegisterInt(&TabListEvents_Added,   widget, PlayerListWidget_TabEntryAdded);
	Event_RegisterInt(&TabListEvents_Changed, widget, PlayerListWidget_TabEntryChanged);
	Event_RegisterInt(&TabListEvents_Removed, widget, PlayerListWidget_TabEntryRemoved);
}

static void PlayerListWidget_Render(struct GuiElem* elem, Real64 delta) {
	struct PlayerListWidget* widget = (struct PlayerListWidget*)elem;
	struct TextWidget* overview = &widget->Overview;
	PackedCol topCol = PACKEDCOL_CONST(0, 0, 0, 180);
	PackedCol bottomCol = PACKEDCOL_CONST(50, 50, 50, 205);

	Gfx_SetTexturing(false);
	Int32 offset = overview->Height + 10;
	Int32 height = max(300, widget->Height + overview->Height);
	GfxCommon_Draw2DGradient(widget->X, widget->Y - offset, widget->Width, height, topCol, bottomCol);

	Gfx_SetTexturing(true);
	overview->YOffset = widget->Y - offset + 5;
	Widget_Reposition(overview);
	Elem_Render(overview, delta);

	Int32 i, highlightedI = PlayerListWidget_HighlightedName(widget, Mouse_X, Mouse_Y);
	for (i = 0; i < widget->NamesCount; i++) {
		if (!widget->Textures[i].ID) continue;

		struct Texture tex = widget->Textures[i];
		if (i == highlightedI) tex.X += 4;
		Texture_Render(&tex);
	}
}

static void PlayerListWidget_Free(struct GuiElem* elem) {
	struct PlayerListWidget* widget = (struct PlayerListWidget*)elem;
	Int32 i;
	for (i = 0; i < widget->NamesCount; i++) {
		Gfx_DeleteTexture(&widget->Textures[i].ID);
	}
	Elem_TryFree(&widget->Overview);

	Event_UnregisterInt(&TabListEvents_Added,   widget, PlayerListWidget_TabEntryAdded);
	Event_UnregisterInt(&TabListEvents_Changed, widget, PlayerListWidget_TabEntryChanged);
	Event_UnregisterInt(&TabListEvents_Removed, widget, PlayerListWidget_TabEntryRemoved);
}

struct GuiElemVTABLE PlayerListWidgetVTABLE;
void PlayerListWidget_Create(struct PlayerListWidget* widget, struct FontDesc* font, bool classic) {
	widget->VTABLE = &PlayerListWidgetVTABLE;
	Widget_Init((struct Widget*)widget);
	widget->VTABLE->Init   = PlayerListWidget_Init;
	widget->VTABLE->Render = PlayerListWidget_Render;
	widget->VTABLE->Free   = PlayerListWidget_Free;
	widget->Reposition = PlayerListWidget_Reposition;
	widget->HorAnchor  = ANCHOR_CENTRE;
	widget->VerAnchor  = ANCHOR_CENTRE;

	widget->NamesCount = 0;
	widget->Font = *font;
	widget->Classic = classic;
	widget->ElementOffset = classic ? 0 : 10;
}


/*########################################################################################################################*
*-----------------------------------------------------TextGroupWidget-----------------------------------------------------*
*#########################################################################################################################*/
#define TextGroupWidget_LineBuffer(widget, i) ((widget)->Buffer + (i) * TEXTGROUPWIDGET_LEN)
String TextGroupWidget_UNSAFE_Get(struct TextGroupWidget* widget, Int32 i) {
	UInt16 length = widget->LineLengths[i];
	return String_Init(TextGroupWidget_LineBuffer(widget, i), length, length);
}

void TextGroupWidget_GetText(struct TextGroupWidget* widget, Int32 index, STRING_TRANSIENT String* text) {
	String line = TextGroupWidget_UNSAFE_Get(widget, index);
	String_Copy(text, &line);
}

void TextGroupWidget_PushUpAndReplaceLast(struct TextGroupWidget* widget, STRING_PURE String* text) {
	Int32 y = widget->Y;
	Gfx_DeleteTexture(&widget->Textures[0].ID);
	Int32 i, max_index = widget->LinesCount - 1;

	/* Move contents of X line to X - 1 line */
	for (i = 0; i < max_index; i++) {
		char* dst = TextGroupWidget_LineBuffer(widget, i);
		char* src = TextGroupWidget_LineBuffer(widget, i + 1);
		UInt8 lineLen = widget->LineLengths[i + 1];

		if (lineLen > 0) Mem_Copy(dst, src, lineLen);
		widget->Textures[i]    = widget->Textures[i + 1];
		widget->LineLengths[i] = lineLen;

		widget->Textures[i].Y = y;
		y += widget->Textures[i].Height;
	}

	widget->Textures[max_index].ID = NULL; /* Delete() is called by SetText otherwise */
	TextGroupWidget_SetText(widget, max_index, text);
}

static Int32 TextGroupWidget_CalcY(struct TextGroupWidget* widget, Int32 index, Int32 newHeight) {
	Int32 y = 0, i;
	struct Texture* textures = widget->Textures;
	Int32 deltaY = newHeight - textures[index].Height;

	if (widget->VerAnchor == ANCHOR_MIN) {
		y = widget->Y;
		for (i = 0; i < index; i++) {
			y += textures[i].Height;
		}
		for (i = index + 1; i < widget->LinesCount; i++) {
			textures[i].Y += deltaY;
		}
	} else {
		y = Game_Height - widget->YOffset;
		for (i = index + 1; i < widget->LinesCount; i++) {
			y -= textures[i].Height;
		}

		y -= newHeight;
		for (i = 0; i < index; i++) {
			textures[i].Y -= deltaY;
		}
	}
	return y;
}

void TextGroupWidget_SetUsePlaceHolder(struct TextGroupWidget* widget, Int32 index, bool placeHolder) {
	widget->PlaceholderHeight[index] = placeHolder;
	if (widget->Textures[index].ID) return;

	UInt16 newHeight = placeHolder ? widget->DefaultHeight : 0;
	widget->Textures[index].Y = TextGroupWidget_CalcY(widget, index, newHeight);
	widget->Textures[index].Height = newHeight;
}

Int32 TextGroupWidget_UsedHeight(struct TextGroupWidget* widget) {
	Int32 height = 0, i;
	struct Texture* textures = widget->Textures;

	for (i = 0; i < widget->LinesCount; i++) {
		if (textures[i].ID) break;
	}
	for (; i < widget->LinesCount; i++) {
		height += textures[i].Height;
	}
	return height;
}

static void TextGroupWidget_Reposition(struct GuiElem* elem) {
	struct TextGroupWidget* widget = (struct TextGroupWidget*)elem;
	Int32 i;
	struct Texture* textures = widget->Textures;

	Int32 oldY = widget->Y;
	Widget_DoReposition(elem);
	if (!widget->LinesCount) return;

	for (i = 0; i < widget->LinesCount; i++) {
		textures[i].X = Gui_CalcPos(widget->HorAnchor, widget->XOffset, textures[i].Width, Game_Width);
		textures[i].Y += widget->Y - oldY;
	}
}

static void TextGroupWidget_UpdateDimensions(struct TextGroupWidget* widget) {
	Int32 i, width = 0, height = 0;
	struct Texture* textures = widget->Textures;

	for (i = 0; i < widget->LinesCount; i++) {
		width = max(width, textures[i].Width);
		height += textures[i].Height;
	}

	widget->Width  = width;
	widget->Height = height;
	Widget_Reposition(widget);
}

struct Portion { Int16 Beg, Len, LineBeg, LineLen; };
#define TEXTGROUPWIDGET_HTTP_LEN 7 /* length of http:// */
#define TEXTGROUPWIDGET_URL 0x8000
#define TEXTGROUPWIDGET_PACKED_LEN 0x7FFF

Int32 TextGroupWidget_NextUrl(char* chars, Int32 charsLen, Int32 i) {
	for (; i < charsLen; i++) {
		if (!(chars[i] == 'h' || chars[i] == '&')) continue;
		Int32 left = charsLen - i;
		if (left < TEXTGROUPWIDGET_HTTP_LEN) return charsLen;

		/* colour codes at start of URL */
		Int32 start = i;
		while (left >= 2 && chars[i] == '&') { left -= 2; i += 2; }
		if (left < TEXTGROUPWIDGET_HTTP_LEN) continue;

		/* Starts with "http" */
		if (chars[i] != 'h' || chars[i + 1] != 't' || chars[i + 2] != 't' || chars[i + 3] != 'p') continue;
		left -= 4; i += 4;

		/* And then with "s://" or "://" */
		if (chars[i] == 's') { left--; i++; }
		if (left >= 3 && chars[i] == ':' && chars[i + 1] == '/' && chars[i + 2] == '/') return start;
	}
	return charsLen;
}

Int32 TextGroupWidget_UrlEnd(char* chars, Int32 charsLen, Int32* begs, Int32 begsLen, Int32 i) {
	Int32 start = i, j;
	for (; i < charsLen && chars[i] != ' '; i++) {
		/* Is this character the start of a line */
		bool isBeg = false;
		for (j = 0; j < begsLen; j++) {
			if (i == begs[j]) { isBeg = true; break; }
		}

		/* Definitely not a multilined URL */
		if (!isBeg || i == start) continue;
		if (chars[i] != '>') break;

		/* Does this line start with "> ", making it a multiline */
		Int32 next = i + 1, left = charsLen - next;
		while (left >= 2 && chars[next] == '&') { left -= 2; next += 2; }
		if (left == 0 || chars[next] != ' ') break;

		i = next;
	}
	return i;
}

void TextGroupWidget_Output(struct Portion bit, Int32 lineBeg, Int32 lineEnd, struct Portion** portions) {
	if (bit.Beg >= lineEnd || !bit.Len) return;
	bit.LineBeg = bit.Beg;
	bit.LineLen = bit.Len & TEXTGROUPWIDGET_PACKED_LEN;

	/* Adjust this portion to be within this line */
	if (bit.Beg >= lineBeg) {
	} else if (bit.Beg + bit.LineLen > lineBeg) {
		/* Adjust start of portion to be within this line */
		Int32 underBy = lineBeg - bit.Beg;
		bit.LineBeg += underBy; bit.LineLen -= underBy;
	} else { return; }

	/* Limit length of portion to be within this line */
	Int32 overBy = (bit.LineBeg + bit.LineLen) - lineEnd;
	if (overBy > 0) bit.LineLen -= overBy;

	bit.LineBeg -= lineBeg;
	if (!bit.LineLen) return;

	struct Portion* cur = *portions; *cur++ = bit; *portions = cur;
}

Int32 TextGroupWidget_Reduce(struct TextGroupWidget* widget, char* chars, Int32 target, struct Portion* portions) {
	struct Portion* start = portions;
	Int32 total = 0, i;
	Int32 begs[TEXTGROUPWIDGET_MAX_LINES];
	Int32 ends[TEXTGROUPWIDGET_MAX_LINES];

	for (i = 0; i < widget->LinesCount; i++) {
		Int32 lineLen = widget->LineLengths[i];
		begs[i] = -1; ends[i] = -1;
		if (!lineLen) continue;

		begs[i] = total;
		Mem_Copy(&chars[total], TextGroupWidget_LineBuffer(widget, i), lineLen);
		total += lineLen; ends[i] = total;
	}

	Int32 end = 0; struct Portion bit;
	for (;;) {
		Int32 nextStart = TextGroupWidget_NextUrl(chars, total, end);

		/* add normal portion between urls */
		bit.Beg = end;
		bit.Len = nextStart - end;
		TextGroupWidget_Output(bit, begs[target], ends[target], &portions);

		if (nextStart == total) break;
		end = TextGroupWidget_UrlEnd(chars, total, begs, widget->LinesCount, nextStart);

		/* add this url portion */
		bit.Beg = nextStart;
		bit.Len = (end - nextStart) | TEXTGROUPWIDGET_URL;
		TextGroupWidget_Output(bit, begs[target], ends[target], &portions);
	}
	return (Int32)(portions - start);
}

void TextGroupWidget_FormatUrl(STRING_TRANSIENT String* text, STRING_PURE String* url) {
	String_AppendColorless(text, url);
	Int32 i;
	char* dst = text->buffer;

	/* Delete "> " multiline chars from URLs */
	for (i = text->length - 2; i >= 0; i--) {
		if (dst[i] != '>' || dst[i + 1] != ' ') continue;
		String_DeleteAt(text, i + 1);
		String_DeleteAt(text, i);
	}
}

bool TextGroupWidget_GetUrl(struct TextGroupWidget* widget, STRING_TRANSIENT String* text, Int32 index, Int32 mouseX) {
	mouseX -= widget->Textures[index].X;
	struct DrawTextArgs args = { 0 }; args.UseShadow = true;
	String line = TextGroupWidget_UNSAFE_Get(widget, index);
	if (Game_ClassicMode) return false;

	char chars[TEXTGROUPWIDGET_MAX_LINES * TEXTGROUPWIDGET_LEN];
	struct Portion portions[2 * (TEXTGROUPWIDGET_LEN / TEXTGROUPWIDGET_HTTP_LEN)];
	Int32 i, x, portionsCount = TextGroupWidget_Reduce(widget, chars, index, portions);

	for (i = 0, x = 0; i < portionsCount; i++) {
		struct Portion bit = portions[i];
		args.Text = String_UNSAFE_Substring(&line, bit.LineBeg, bit.LineLen);
		args.Font = (bit.Len & TEXTGROUPWIDGET_URL) ? widget->UnderlineFont : widget->Font;

		Int32 width = Drawer2D_MeasureText(&args).Width;
		if ((bit.Len & TEXTGROUPWIDGET_URL) && mouseX >= x && mouseX < x + width) {
			bit.Len &= TEXTGROUPWIDGET_PACKED_LEN;
			String url = String_Init(&chars[bit.Beg], bit.Len, bit.Len);

			TextGroupWidget_FormatUrl(text, &url);
			return true;
		}
		x += width;
	}
	return false;
}

void TextGroupWidget_GetSelected(struct TextGroupWidget* widget, STRING_TRANSIENT String* text, Int32 x, Int32 y) {
	Int32 i;
	for (i = 0; i < widget->LinesCount; i++) {
		if (!widget->Textures[i].ID) continue;
		struct Texture tex = widget->Textures[i];
		if (!Gui_Contains(tex.X, tex.Y, tex.Width, tex.Height, x, y)) continue;

		if (!TextGroupWidget_GetUrl(widget, text, i, x)) {
			String line = TextGroupWidget_UNSAFE_Get(widget, i);
			String_AppendString(text, &line);
		}
		return;
	}
}

bool TextGroupWidget_MightHaveUrls(struct TextGroupWidget* widget) {
	if (Game_ClassicMode) return false;
	Int32 i;

	for (i = 0; i < widget->LinesCount; i++) {
		if (!widget->LineLengths[i]) continue;
		String line = TextGroupWidget_UNSAFE_Get(widget, i);
		if (String_IndexOf(&line, '/', 0) >= 0) return true;
	}
	return false;
}

void TextGroupWidget_DrawAdvanced(struct TextGroupWidget* widget, struct Texture* tex, 
	struct DrawTextArgs* args, Int32 index, STRING_PURE String* text) {
	char chars[TEXTGROUPWIDGET_MAX_LINES * TEXTGROUPWIDGET_LEN];
	struct Portion portions[2 * (TEXTGROUPWIDGET_LEN / TEXTGROUPWIDGET_HTTP_LEN)];
	Int32 i, x, portionsCount = TextGroupWidget_Reduce(widget, chars, index, portions);

	struct Size2D total = Size2D_Empty;
	struct Size2D partSizes[Array_Elems(portions)];

	for (i = 0; i < portionsCount; i++) {
		struct Portion bit = portions[i];
		args->Text = String_UNSAFE_Substring(text, bit.LineBeg, bit.LineLen);
		args->Font = (bit.Len & TEXTGROUPWIDGET_URL) ? widget->UnderlineFont : widget->Font;

		partSizes[i] = Drawer2D_MeasureText(args);
		total.Height = max(partSizes[i].Height, total.Height);
		total.Width += partSizes[i].Width;
	}

	struct Bitmap bmp;
	Bitmap_AllocateClearedPow2(&bmp, total.Width, total.Height);	
	Drawer2D_Begin(&bmp);
	{
		for (i = 0, x = 0; i < portionsCount; i++) {
			struct Portion bit = portions[i];
			args->Text = String_UNSAFE_Substring(text, bit.LineBeg, bit.LineLen);
			args->Font = (bit.Len & TEXTGROUPWIDGET_URL) ? widget->UnderlineFont : widget->Font;

			Drawer2D_DrawText(args, x, 0);
			x += partSizes[i].Width;
		}
		Drawer2D_Make2DTexture(tex, &bmp, total, 0, 0);
	}
	Drawer2D_End();
}

void TextGroupWidget_SetText(struct TextGroupWidget* widget, Int32 index, STRING_PURE String* text_orig) {
	String text = *text_orig;
	text.length = min(text.length, TEXTGROUPWIDGET_LEN);

	Gfx_DeleteTexture(&widget->Textures[index].ID);
	Mem_Copy(TextGroupWidget_LineBuffer(widget, index), text.buffer, text.length);
	widget->LineLengths[index] = (UInt8)text.length;

	struct Texture tex = { 0 };
	if (!Drawer2D_IsEmptyText(&text)) {
		struct DrawTextArgs args; DrawTextArgs_Make(&args, &text, &widget->Font, true);

		if (!TextGroupWidget_MightHaveUrls(widget)) {
			Drawer2D_MakeTextTexture(&tex, &args, 0, 0);
		} else {
			TextGroupWidget_DrawAdvanced(widget, &tex, &args, index, &text);
		}
		Drawer2D_ReducePadding_Tex(&tex, widget->Font.Size, 3);
	} else {
		tex.Height = widget->PlaceholderHeight[index] ? widget->DefaultHeight : 0;
	}

	tex.X = Gui_CalcPos(widget->HorAnchor, widget->XOffset, tex.Width, Game_Width);
	tex.Y = TextGroupWidget_CalcY(widget, index, tex.Height);
	widget->Textures[index] = tex;
	TextGroupWidget_UpdateDimensions(widget);
}


static void TextGroupWidget_Init(struct GuiElem* elem) {
	struct TextGroupWidget* widget = (struct TextGroupWidget*)elem;
	Int32 height = Drawer2D_FontHeight(&widget->Font, true);
	Drawer2D_ReducePadding_Height(&height, widget->Font.Size, 3);
	widget->DefaultHeight = height;

	Int32 i;
	for (i = 0; i < widget->LinesCount; i++) {
		widget->Textures[i].Height = height;
		widget->PlaceholderHeight[i] = true;
	}
	TextGroupWidget_UpdateDimensions(widget);
}

static void TextGroupWidget_Render(struct GuiElem* elem, Real64 delta) {
	struct TextGroupWidget* widget = (struct TextGroupWidget*)elem;
	Int32 i;
	struct Texture* textures = widget->Textures;

	for (i = 0; i < widget->LinesCount; i++) {
		if (!textures[i].ID) continue;
		Texture_Render(&textures[i]);
	}
}

static void TextGroupWidget_Free(struct GuiElem* elem) {
	struct TextGroupWidget* widget = (struct TextGroupWidget*)elem;
	Int32 i;

	for (i = 0; i < widget->LinesCount; i++) {
		widget->LineLengths[i] = 0;
		Gfx_DeleteTexture(&widget->Textures[i].ID);
	}
}

struct GuiElemVTABLE TextGroupWidget_VTABLE;
void TextGroupWidget_Create(struct TextGroupWidget* widget, Int32 linesCount, struct FontDesc* font, struct FontDesc* underlineFont, STRING_REF struct Texture* textures, STRING_REF char* buffer) {
	widget->VTABLE = &TextGroupWidget_VTABLE;
	Widget_Init((struct Widget*)widget);
	widget->VTABLE->Init   = TextGroupWidget_Init;
	widget->VTABLE->Render = TextGroupWidget_Render;
	widget->VTABLE->Free   = TextGroupWidget_Free;
	widget->Reposition     = TextGroupWidget_Reposition;

	widget->LinesCount = linesCount;
	widget->Font = *font;
	widget->UnderlineFont = *underlineFont;
	widget->Textures = textures;
	widget->Buffer = buffer;
}


/*########################################################################################################################*
*---------------------------------------------------SpecialInputWidget----------------------------------------------------*
*#########################################################################################################################*/
static void SpecialInputWidget_UpdateColString(struct SpecialInputWidget* widget) {
	UInt32 i;
	for (i = 0; i < DRAWER2D_MAX_COLS; i++) {
		if (i >= 'A' && i <= 'F') continue;
	}
	String buffer = String_FromArray(widget->__ColBuffer);

	for (i = 0; i < DRAWER2D_MAX_COLS; i++) {
		if (i >= 'A' && i <= 'F') continue;
		if (Drawer2D_Cols[i].A == 0) continue;

		String_Append(&buffer, '&'); String_Append(&buffer, (UInt8)i);
		String_Append(&buffer, '%'); String_Append(&buffer, (UInt8)i);
	}
	widget->ColString = buffer;
}

static bool SpecialInputWidget_IntersectsHeader(struct SpecialInputWidget* widget, Int32 x, Int32 y) {
	Int32 titleX = 0, i;
	for (i = 0; i < Array_Elems(widget->Tabs); i++) {
		struct Size2D size = widget->Tabs[i].TitleSize;
		if (Gui_Contains(titleX, 0, size.Width, size.Height, x, y)) {
			widget->SelectedIndex = i;
			return true;
		}
		titleX += size.Width;
	}
	return false;
}

static void SpecialInputWidget_IntersectsBody(struct SpecialInputWidget* widget, Int32 x, Int32 y) {
	y -= widget->Tabs[0].TitleSize.Height;
	x /= widget->ElementSize.Width; y /= widget->ElementSize.Height;
	struct SpecialInputTab e = widget->Tabs[widget->SelectedIndex];
	Int32 index = y * e.ItemsPerRow + x;
	if (index * e.CharsPerItem >= e.Contents.length) return;

	if (widget->SelectedIndex == 0) {
		/* TODO: need to insert characters that don't affect widget->CaretPos index, adjust widget->CaretPos colour */
		InputWidget_Append(widget->AppendObj, e.Contents.buffer[index * e.CharsPerItem]);
		InputWidget_Append(widget->AppendObj, e.Contents.buffer[index * e.CharsPerItem + 1]);
	} else {
		InputWidget_Append(widget->AppendObj, e.Contents.buffer[index]);
	}
}

static void SpecialInputTab_Init(struct SpecialInputTab* tab, STRING_REF String* title,
	Int32 itemsPerRow, Int32 charsPerItem, STRING_REF String* contents) {
	tab->Title = *title;
	tab->TitleSize = Size2D_Empty;
	tab->Contents = *contents;
	tab->ItemsPerRow = itemsPerRow;
	tab->CharsPerItem = charsPerItem;
}

static void SpecialInputWidget_InitTabs(struct SpecialInputWidget* widget) {
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
static Int32 SpecialInputWidget_MeasureTitles(struct SpecialInputWidget* widget) {
	Int32 totalWidth = 0;
	struct DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, false);

	Int32 i;
	for (i = 0; i < Array_Elems(widget->Tabs); i++) {
		args.Text = widget->Tabs[i].Title;
		widget->Tabs[i].TitleSize = Drawer2D_MeasureText(&args);
		widget->Tabs[i].TitleSize.Width += SPECIAL_TITLE_SPACING;
		totalWidth += widget->Tabs[i].TitleSize.Width;
	}
	return totalWidth;
}

static void SpecialInputWidget_DrawTitles(struct SpecialInputWidget* widget, struct Bitmap* bmp) {
	Int32 x = 0;
	struct DrawTextArgs args; DrawTextArgs_MakeEmpty(&args, &widget->Font, false);

	Int32 i;
	PackedCol col_selected = PACKEDCOL_CONST(30, 30, 30, 200);
	PackedCol col_inactive = PACKEDCOL_CONST( 0,  0,  0, 127);
	for (i = 0; i < Array_Elems(widget->Tabs); i++) {
		args.Text = widget->Tabs[i].Title;
		PackedCol col = i == widget->SelectedIndex ? col_selected : col_inactive;
		struct Size2D size = widget->Tabs[i].TitleSize;

		Drawer2D_Clear(bmp, col, x, 0, size.Width, size.Height);
		Drawer2D_DrawText(&args, x + SPECIAL_TITLE_SPACING / 2, 0);
		x += size.Width;
	}
}

static struct Size2D SpecialInputWidget_CalculateContentSize(struct SpecialInputTab* e, struct Size2D* sizes, struct Size2D* elemSize) {
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

static void SpecialInputWidget_MeasureContentSizes(struct SpecialInputWidget* widget, struct SpecialInputTab* e, struct Size2D* sizes) {
	char buffer[STRING_SIZE];
	String s = String_FromArray(buffer);
	s.length = e->CharsPerItem;
	struct DrawTextArgs args; DrawTextArgs_Make(&args, &s, &widget->Font, false);

	Int32 i, j;
	for (i = 0; i < e->Contents.length; i += e->CharsPerItem) {
		for (j = 0; j < e->CharsPerItem; j++) {
			s.buffer[j] = e->Contents.buffer[i + j];
		}
		sizes[i / e->CharsPerItem] = Drawer2D_MeasureText(&args);
	}
}

static void SpecialInputWidget_DrawContent(struct SpecialInputWidget* widget, struct SpecialInputTab* e, Int32 yOffset) {
	char buffer[STRING_SIZE];
	String s = String_FromArray(buffer);
	s.length = e->CharsPerItem;
	struct DrawTextArgs args; DrawTextArgs_Make(&args, &s, &widget->Font, false);

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

static void SpecialInputWidget_Make(struct SpecialInputWidget* widget, struct SpecialInputTab* e) {
	struct Size2D sizes[DRAWER2D_MAX_COLS];
	SpecialInputWidget_MeasureContentSizes(widget, e, sizes);
	struct Size2D bodySize = SpecialInputWidget_CalculateContentSize(e, sizes, &widget->ElementSize);
	Int32 titleWidth = SpecialInputWidget_MeasureTitles(widget);
	Int32 titleHeight = widget->Tabs[0].TitleSize.Height;
	struct Size2D size = Size2D_Make(max(bodySize.Width, titleWidth), bodySize.Height + titleHeight);
	Gfx_DeleteTexture(&widget->Tex.ID);

	struct Bitmap bmp; Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
	Drawer2D_Begin(&bmp);
	{
		SpecialInputWidget_DrawTitles(widget, &bmp);
		PackedCol col = PACKEDCOL_CONST(30, 30, 30, 200);
		Drawer2D_Clear(&bmp, col, 0, titleHeight, size.Width, bodySize.Height);
		SpecialInputWidget_DrawContent(widget, e, titleHeight);
	}
	Drawer2D_End();

	Drawer2D_Make2DTexture(&widget->Tex, &bmp, size, widget->X, widget->Y);
	Mem_Free(bmp.Scan0);
}

static void SpecialInputWidget_Redraw(struct SpecialInputWidget* widget) {
	SpecialInputWidget_Make(widget, &widget->Tabs[widget->SelectedIndex]);
	widget->Width = widget->Tex.Width;
	widget->Height = widget->Tex.Height;
}

static void SpecialInputWidget_Init(struct GuiElem* elem) {
	struct SpecialInputWidget* widget = (struct SpecialInputWidget*)elem;
	widget->X = 5; widget->Y = 5;
	SpecialInputWidget_InitTabs(widget);
	SpecialInputWidget_Redraw(widget);
	SpecialInputWidget_SetActive(widget, widget->Active);
}

static void SpecialInputWidget_Render(struct GuiElem* elem, Real64 delta) {
	struct SpecialInputWidget* widget = (struct SpecialInputWidget*)elem;
	Texture_Render(&widget->Tex);
}

static void SpecialInputWidget_Free(struct GuiElem* elem) {
	struct SpecialInputWidget* widget = (struct SpecialInputWidget*)elem;
	Gfx_DeleteTexture(&widget->Tex.ID);
}

static bool SpecialInputWidget_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	struct SpecialInputWidget* widget = (struct SpecialInputWidget*)elem;
	x -= widget->X; y -= widget->Y;

	if (SpecialInputWidget_IntersectsHeader(widget, x, y)) {
		SpecialInputWidget_Redraw(widget);
	} else {
		SpecialInputWidget_IntersectsBody(widget, x, y);
	}
	return true;
}

void SpecialInputWidget_UpdateCols(struct SpecialInputWidget* widget) {
	SpecialInputWidget_UpdateColString(widget);
	widget->Tabs[0].Contents = widget->ColString;
	if (!widget->Active || widget->SelectedIndex != 0) return;
	SpecialInputWidget_Redraw(widget);
}

void SpecialInputWidget_SetActive(struct SpecialInputWidget* widget, bool active) {
	widget->Active = active;
	widget->Height = active ? widget->Tex.Height : 0;
}

struct GuiElemVTABLE SpecialInputWidget_VTABLE;
void SpecialInputWidget_Create(struct SpecialInputWidget* widget, struct FontDesc* font, struct InputWidget* appendObj) {
	widget->VTABLE = &SpecialInputWidget_VTABLE;
	Widget_Init((struct Widget*)widget);
	widget->VerAnchor = ANCHOR_MAX;
	widget->Font = *font;
	widget->AppendObj = appendObj;

	widget->VTABLE->Init   = SpecialInputWidget_Init;
	widget->VTABLE->Render = SpecialInputWidget_Render;
	widget->VTABLE->Free   = SpecialInputWidget_Free;
	widget->VTABLE->HandlesMouseDown = SpecialInputWidget_HandlesMouseDown;
}
