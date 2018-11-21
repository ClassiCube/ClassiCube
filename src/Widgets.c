#include "Widgets.h"
#include "Graphics.h"
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
#include "Bitmap.h"
#include "Block.h"

#define Widget_UV(u1,v1, u2,v2) Tex_UV(u1/256.0f,v1/256.0f, u2/256.0f,v2/256.0f)
static void Widget_NullFunc(void* widget) { }
static Size2D Size2D_Empty;

static bool Widget_Mouse(void* elem, int x, int y, MouseButton btn) { return false; }
static bool Widget_Key(void* elem, Key key) { return false; }
static bool Widget_KeyPress(void* elem, char keyChar) { return false; }
static bool Widget_MouseMove(void* elem, int x, int y) { return false; }
static bool Widget_MouseScroll(void* elem, float delta) { return false; }

/*########################################################################################################################*
*-------------------------------------------------------TextWidget--------------------------------------------------------*
*#########################################################################################################################*/
static void TextWidget_Render(void* widget, double delta) {
	struct TextWidget* w = widget;
	if (w->Texture.ID) { Texture_RenderShaded(&w->Texture, w->Col); }
}

static void TextWidget_Free(void* widget) {
	struct TextWidget* w = widget;
	Gfx_DeleteTexture(&w->Texture.ID);
}

static void TextWidget_Reposition(void* widget) {
	struct TextWidget* w = widget;
	int oldX = w->X, oldY = w->Y;

	Widget_CalcPosition(w);
	w->Texture.X += w->X - oldX;
	w->Texture.Y += w->Y - oldY;
}

static struct WidgetVTABLE TextWidget_VTABLE = {
	Widget_NullFunc, TextWidget_Render, TextWidget_Free,  Gui_DefaultRecreate,
	Widget_Key,	     Widget_Key,        Widget_KeyPress,
	Widget_Mouse,    Widget_Mouse,      Widget_MouseMove, Widget_MouseScroll,
	TextWidget_Reposition,
};
void TextWidget_Make(struct TextWidget* w) {
	PackedCol col = PACKEDCOL_WHITE;
	Widget_Reset(w);
	w->VTABLE = &TextWidget_VTABLE;
	w->Col    = col;
}

void TextWidget_Create(struct TextWidget* w, const String* text, const FontDesc* font) {
	TextWidget_Make(w);
	TextWidget_Set(w,  text, font);
}

void TextWidget_Set(struct TextWidget* w, const String* text, const FontDesc* font) {
	struct DrawTextArgs args;
	Gfx_DeleteTexture(&w->Texture.ID);

	if (Drawer2D_IsEmptyText(text)) {
		w->Texture.Width  = 0; 
		w->Texture.Height = Drawer2D_FontHeight(font, true);
	} else {	
		DrawTextArgs_Make(&args, text, font, true);
		Drawer2D_MakeTextTexture(&w->Texture, &args, 0, 0);
	}

	if (w->ReducePadding) {
		Drawer2D_ReducePadding_Tex(&w->Texture, font->Size, 4);
	}

	w->Width = w->Texture.Width; w->Height = w->Texture.Height;
	Widget_Reposition(w);
	w->Texture.X = w->X; w->Texture.Y = w->Y;
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
#define BUTTON_uWIDTH (200.0f / 256.0f)
#define BUTTON_MIN_WIDTH 40

static struct Texture Button_ShadowTex   = { GFX_NULL, Tex_Rect(0,0, 0,0), Widget_UV(0,66, 200,86)  };
static struct Texture Button_SelectedTex = { GFX_NULL, Tex_Rect(0,0, 0,0), Widget_UV(0,86, 200,106) };
static struct Texture Button_DisabledTex = { GFX_NULL, Tex_Rect(0,0, 0,0), Widget_UV(0,46, 200,66)  };

static void ButtonWidget_Free(void* widget) {
	struct ButtonWidget* w = widget;
	Gfx_DeleteTexture(&w->Texture.ID);
}

static void ButtonWidget_Reposition(void* widget) {
	struct ButtonWidget* w = widget;
	int oldX = w->X, oldY = w->Y;
	Widget_CalcPosition(w);
	
	w->Texture.X += w->X - oldX;
	w->Texture.Y += w->Y - oldY;
}

static void ButtonWidget_Render(void* widget, double delta) {
	PackedCol normCol     = PACKEDCOL_CONST(224, 224, 224, 255);
	PackedCol activeCol   = PACKEDCOL_CONST(255, 255, 160, 255);
	PackedCol disabledCol = PACKEDCOL_CONST(160, 160, 160, 255);
	PackedCol col, white  = PACKEDCOL_WHITE;
	struct ButtonWidget* w = widget;
	struct Texture back;	
	float scale;
		
	back = w->Active ? Button_SelectedTex : Button_ShadowTex;
	if (w->Disabled) back = Button_DisabledTex;

	back.ID = Game_UseClassicGui ? Gui_GuiClassicTex : Gui_GuiTex;
	back.X = w->X; back.Width  = w->Width;
	back.Y = w->Y; back.Height = w->Height;

	if (w->Width == 400) {
		/* Button can be drawn normally */
		Texture_Render(&back);
	} else {
		/* Split button down the middle */
		scale = (w->Width / 400.0f) * 0.5f;
		Gfx_BindTexture(back.ID); /* avoid bind twice */

		back.Width = (w->Width / 2);
		back.uv.U1 = 0.0f; back.uv.U2 = BUTTON_uWIDTH * scale;
		GfxCommon_Draw2DTexture(&back, white);

		back.X += (w->Width / 2);
		back.uv.U1 = BUTTON_uWIDTH * (1.0f - scale); back.uv.U2 = BUTTON_uWIDTH;
		GfxCommon_Draw2DTexture(&back, white);
	}

	if (!w->Texture.ID) return;
	col = w->Disabled ? disabledCol : (w->Active ? activeCol : normCol);
	Texture_RenderShaded(&w->Texture, col);
}

static struct WidgetVTABLE ButtonWidget_VTABLE = {
	Widget_NullFunc, ButtonWidget_Render, ButtonWidget_Free, Gui_DefaultRecreate,
	Widget_Key,	     Widget_Key,          Widget_KeyPress,
	Widget_Mouse,    Widget_Mouse,        Widget_MouseMove,  Widget_MouseScroll,
	ButtonWidget_Reposition,
};
void ButtonWidget_Create(struct ButtonWidget* w, int minWidth, const String* text, const FontDesc* font, Widget_LeftClick onClick) {
	Widget_Reset(w);
	w->VTABLE    = &ButtonWidget_VTABLE;
	w->OptName   = NULL;
	w->MinWidth  = minWidth;
	w->MenuClick = onClick;
	ButtonWidget_Set(w, text, font);
}

void ButtonWidget_Set(struct ButtonWidget* w, const String* text, const FontDesc* font) {
	struct DrawTextArgs args;
	Gfx_DeleteTexture(&w->Texture.ID);

	if (Drawer2D_IsEmptyText(text)) {
		w->Texture.Width  = 0;
		w->Texture.Height = Drawer2D_FontHeight(font, true);
	} else {
		DrawTextArgs_Make(&args, text, font, true);
		Drawer2D_MakeTextTexture(&w->Texture, &args, 0, 0);
	}

	w->Width  = max(w->Texture.Width,  w->MinWidth);
	w->Height = max(w->Texture.Height, BUTTON_MIN_WIDTH);

	Widget_Reposition(w);
	w->Texture.X = w->X + (w->Width  / 2 - w->Texture.Width  / 2);
	w->Texture.Y = w->Y + (w->Height / 2 - w->Texture.Height / 2);
}


/*########################################################################################################################*
*-----------------------------------------------------ScrollbarWidget-----------------------------------------------------*
*#########################################################################################################################*/
#define TABLE_MAX_ROWS_DISPLAYED 8
#define SCROLL_WIDTH 22
#define SCROLL_BORDER 2
#define SCROLL_NUBS_WIDTH 3
static PackedCol Scroll_BackCol  = PACKEDCOL_CONST( 10,  10,  10, 220);
static PackedCol Scroll_BarCol   = PACKEDCOL_CONST(100, 100, 100, 220);
static PackedCol Scroll_HoverCol = PACKEDCOL_CONST(122, 122, 122, 220);

static void ScrollbarWidget_ClampScrollY(struct ScrollbarWidget* w) {
	int maxRows = w->TotalRows - TABLE_MAX_ROWS_DISPLAYED;
	if (w->ScrollY >= maxRows) w->ScrollY = maxRows;
	if (w->ScrollY < 0) w->ScrollY = 0;
}

static float ScrollbarWidget_GetScale(struct ScrollbarWidget* w) {
	float rows = (float)w->TotalRows;
	return (w->Height - SCROLL_BORDER * 2) / rows;
}

static void ScrollbarWidget_GetScrollbarCoords(struct ScrollbarWidget* w, int* y, int* height) {
	float scale = ScrollbarWidget_GetScale(w);
	*y = Math_Ceil(w->ScrollY * scale) + SCROLL_BORDER;
	*height = Math_Ceil(TABLE_MAX_ROWS_DISPLAYED * scale);
	*height = min(*y + *height, w->Height - SCROLL_BORDER) - *y;
}

static void ScrollbarWidget_Render(void* widget, double delta) {
	struct ScrollbarWidget* w = widget;
	int x, y, width, height;
	PackedCol barCol;
	bool hovered;

	x = w->X; width = w->Width;
	GfxCommon_Draw2DFlat(x, w->Y, width, w->Height, Scroll_BackCol);

	ScrollbarWidget_GetScrollbarCoords(w, &y, &height);
	x += SCROLL_BORDER; y += w->Y;
	width -= SCROLL_BORDER * 2; 

	hovered = Gui_Contains(x, y, width, height, Mouse_X, Mouse_Y);
	barCol  = hovered ? Scroll_HoverCol : Scroll_BarCol;
	GfxCommon_Draw2DFlat(x, y, width, height, barCol);

	if (height < 20) return;
	x += SCROLL_NUBS_WIDTH; y += (height / 2);
	width -= SCROLL_NUBS_WIDTH * 2;

	GfxCommon_Draw2DFlat(x, y - 1 - 4, width, SCROLL_BORDER, Scroll_BackCol);
	GfxCommon_Draw2DFlat(x, y - 1,     width, SCROLL_BORDER, Scroll_BackCol);
	GfxCommon_Draw2DFlat(x, y - 1 + 4, width, SCROLL_BORDER, Scroll_BackCol);
}

static bool ScrollbarWidget_MouseDown(void* widget, int x, int y, MouseButton btn) {
	struct ScrollbarWidget* w = widget;
	int posY, height;

	if (w->DraggingMouse) return true;
	if (btn != MouseButton_Left) return false;
	if (x < w->X || x >= w->X + w->Width) return false;

	y -= w->Y;
	ScrollbarWidget_GetScrollbarCoords(w, &posY, &height);

	if (y < posY) {
		w->ScrollY -= TABLE_MAX_ROWS_DISPLAYED;
	} else if (y >= posY + height) {
		w->ScrollY += TABLE_MAX_ROWS_DISPLAYED;
	} else {
		w->DraggingMouse = true;
		w->MouseOffset = y - posY;
	}
	ScrollbarWidget_ClampScrollY(w);
	return true;
}

static bool ScrollbarWidget_MouseUp(void* widget, int x, int y, MouseButton btn) {
	struct ScrollbarWidget* w = widget;
	w->DraggingMouse = false;
	w->MouseOffset   = 0;
	return true;
}

static bool ScrollbarWidget_MouseScroll(void* widget, float delta) {
	struct ScrollbarWidget* w = widget;
	int steps = Utils_AccumulateWheelDelta(&w->ScrollingAcc, delta);

	w->ScrollY -= steps;
	ScrollbarWidget_ClampScrollY(w);
	return true;
}

static bool ScrollbarWidget_MouseMove(void* widget, int x, int y) {
	struct ScrollbarWidget* w = widget;
	float scale;

	if (w->DraggingMouse) {
		y -= w->Y;
		scale = ScrollbarWidget_GetScale(w);
		w->ScrollY = (int)((y - w->MouseOffset) / scale);
		ScrollbarWidget_ClampScrollY(w);
		return true;
	}
	return false;
}

static struct WidgetVTABLE ScrollbarWidget_VTABLE = {
	Widget_NullFunc,           ScrollbarWidget_Render,  Widget_NullFunc,           Gui_DefaultRecreate,
	Widget_Key,	               Widget_Key,              Widget_KeyPress,
	ScrollbarWidget_MouseDown, ScrollbarWidget_MouseUp, ScrollbarWidget_MouseMove, ScrollbarWidget_MouseScroll,
	Widget_CalcPosition,
};
void ScrollbarWidget_Create(struct ScrollbarWidget* w) {
	Widget_Reset(w);
	w->VTABLE = &ScrollbarWidget_VTABLE;
	w->Width  = SCROLL_WIDTH;
	w->TotalRows     = 0;
	w->ScrollY       = 0;
	w->ScrollingAcc  = 0.0f;
	w->DraggingMouse = false;
	w->MouseOffset   = 0;
}


/*########################################################################################################################*
*------------------------------------------------------HotbarWidget-------------------------------------------------------*
*#########################################################################################################################*/
static void HotbarWidget_RenderHotbarOutline(struct HotbarWidget* w) {
	PackedCol white = PACKEDCOL_WHITE;
	GfxResourceID tex;
	float width;
	int i, x;
	
	tex = Game_UseClassicGui ? Gui_GuiClassicTex : Gui_GuiTex;
	w->BackTex.ID = tex;
	Texture_Render(&w->BackTex);

	i     = Inventory_SelectedIndex;
	width = w->ElemSize + w->BorderSize;
	x     = (int)(w->X + w->BarXOffset + width * i + w->ElemSize / 2);

	w->SelTex.ID = tex;
	w->SelTex.X  = (int)(x - w->SelBlockSize / 2);
	GfxCommon_Draw2DTexture(&w->SelTex, white);
}

static void HotbarWidget_RenderHotbarBlocks(struct HotbarWidget* w) {
	/* TODO: Should hotbar use its own VB? */
	VertexP3fT2fC4b vertices[INVENTORY_BLOCKS_PER_HOTBAR * ISOMETRICDRAWER_MAXVERTICES];
	float width, scale;
	int i, x, y;

	IsometricDrawer_BeginBatch(vertices, ModelCache_Vb);
	width =  w->ElemSize + w->BorderSize;
	scale = (w->ElemSize * 13.5f/16.0f) / 2.0f;

	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		x = (int)(w->X + w->BarXOffset + width * i + w->ElemSize / 2);
		y = (int)(w->Y + (w->Height - w->BarHeight / 2));
		IsometricDrawer_DrawBatch(Inventory_Get(i), scale, x, y);
	}
	IsometricDrawer_EndBatch();
}

static void HotbarWidget_RepositonBackgroundTexture(struct HotbarWidget* w) {
	w->BackTex.ID = GFX_NULL;
	Tex_SetRect(w->BackTex, w->X,w->Y, w->Width,w->Height);
	Tex_SetUV(w->BackTex,   0,0, 182/256.0f,22/256.0f);
}

static void HotbarWidget_RepositionSelectionTexture(struct HotbarWidget* w) {
	float scale = Game_GetHotbarScale();
	int hSize = (int)w->SelBlockSize;
	int vSize = (int)(22.0f * scale);
	int y = w->Y + (w->Height - (int)(23.0f * scale));

	w->SelTex.ID = GFX_NULL;
	Tex_SetRect(w->SelTex, 0,y, hSize,vSize);
	Tex_SetUV(w->SelTex,   0,22/256.0f, 24/256.0f,44/256.0f);
}

static int HotbarWidget_ScrolledIndex(struct HotbarWidget* w, float delta, int index, int dir) {
	int steps = Utils_AccumulateWheelDelta(&w->ScrollAcc, delta);
	index += (dir * steps) % INVENTORY_BLOCKS_PER_HOTBAR;

	if (index < 0) index += INVENTORY_BLOCKS_PER_HOTBAR;
	if (index >= INVENTORY_BLOCKS_PER_HOTBAR) {
		index -= INVENTORY_BLOCKS_PER_HOTBAR;
	}
	return index;
}

static void HotbarWidget_Reposition(void* widget) {
	float scale = Game_GetHotbarScale();
	struct HotbarWidget* w = widget;

	w->BarHeight = (float)Math_Floor(22.0f * scale);
	w->Width     = (int)(182 * scale);
	w->Height    = (int)w->BarHeight;

	w->SelBlockSize = (float)Math_Ceil(24.0f * scale);
	w->ElemSize     = 16.0f * scale;
	w->BarXOffset   = 3.1f * scale;
	w->BorderSize   = 4.0f * scale;

	Widget_CalcPosition(w);
	HotbarWidget_RepositonBackgroundTexture(w);
	HotbarWidget_RepositionSelectionTexture(w);
}

static void HotbarWidget_Init(void* widget) {
	struct HotbarWidget* w = widget;
	Widget_Reposition(w);
}

static void HotbarWidget_Render(void* widget, double delta) {
	struct HotbarWidget* w = widget;
	HotbarWidget_RenderHotbarOutline(w);
	HotbarWidget_RenderHotbarBlocks(w);
}

static bool HotbarWidget_KeyDown(void* widget, Key key) {
	struct HotbarWidget* w = widget;
	int index;
	if (key < Key_1 || key > Key_9) return false;

	index = key - Key_1;
	if (KeyBind_IsPressed(KeyBind_HotbarSwitching)) {
		/* Pick from first to ninth row */
		Inventory_SetOffset(index * INVENTORY_BLOCKS_PER_HOTBAR);
		w->AltHandled = true;
	} else {
		Inventory_SetSelectedIndex(index);
	}
	return true;
}

static bool HotbarWidget_KeyUp(void* widget, Key key) {
	struct HotbarWidget* w = widget;
	int index;

	/* Need to handle these cases:
	     a) user presses alt then number
	     b) user presses alt
	   We only do case b) if case a) did not happen */
	if (key != KeyBind_Get(KeyBind_HotbarSwitching)) return false;
	if (w->AltHandled) { w->AltHandled = false; return true; } /* handled already */

	/* Don't switch hotbar when alt+tab */
	if (!Window_Focused) return true;

	/* Alternate between first and second row */
	index = Inventory_Offset == 0 ? 1 : 0;
	Inventory_SetOffset(index * INVENTORY_BLOCKS_PER_HOTBAR);
	return true;
}

static bool HotbarWidget_MouseDown(void* widget, int x, int y, MouseButton btn) {
	struct HotbarWidget* w = widget;
	struct Screen* screen;
	int width, height;
	int i, cellX, cellY;

	if (btn != MouseButton_Left || !Widget_Contains(w, x, y)) return false;
	screen = Gui_GetActiveScreen();
	if (screen != InventoryScreen_UNSAFE_RawPointer) return false;

	width  = (int)(w->ElemSize + w->BorderSize);
	height = Math_Ceil(w->BarHeight);

	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		cellX = (int)(w->X + width * i);
		cellY = (int)(w->Y + (w->Height - height));

		if (Gui_Contains(cellX, cellY, width, height, x, y)) {
			Inventory_SetSelectedIndex(i);
			return true;
		}
	}
	return false;
}

static bool HotbarWidget_MouseScroll(void* widget, float delta) {
	struct HotbarWidget* w = widget;
	int index;

	if (KeyBind_IsPressed(KeyBind_HotbarSwitching)) {
		index = Inventory_Offset / INVENTORY_BLOCKS_PER_HOTBAR;
		index = HotbarWidget_ScrolledIndex(w, delta, index, 1);
		Inventory_SetOffset(index * INVENTORY_BLOCKS_PER_HOTBAR);
		w->AltHandled = true;
	} else {
		index = HotbarWidget_ScrolledIndex(w, delta, Inventory_SelectedIndex, -1);
		Inventory_SetSelectedIndex(index);
	}
	return true;
}

static struct WidgetVTABLE HotbarWidget_VTABLE = {
	HotbarWidget_Init,      HotbarWidget_Render, Widget_NullFunc,  Gui_DefaultRecreate,
	HotbarWidget_KeyDown,	HotbarWidget_KeyUp,  Widget_KeyPress,
	HotbarWidget_MouseDown, Widget_Mouse,        Widget_MouseMove, HotbarWidget_MouseScroll,
	HotbarWidget_Reposition,
};
void HotbarWidget_Create(struct HotbarWidget* w) {
	Widget_Reset(w);
	w->VTABLE    = &HotbarWidget_VTABLE;
	w->HorAnchor = ANCHOR_CENTRE;
	w->VerAnchor = ANCHOR_MAX;
}


/*########################################################################################################################*
*-------------------------------------------------------TableWidget-------------------------------------------------------*
*#########################################################################################################################*/
static int Table_X(struct TableWidget* w) { return w->X - 5 - 10; }
static int Table_Y(struct TableWidget* w) { return w->Y - 5 - 30; }
static int Table_Width(struct TableWidget* w) {
	return w->ElementsPerRow * w->BlockSize + 10 + 20; 
}
static int Table_Height(struct TableWidget* w) {
	return min(w->RowsCount, TABLE_MAX_ROWS_DISPLAYED) * w->BlockSize + 10 + 40;
}

#define TABLE_MAX_VERTICES (8 * 10 * ISOMETRICDRAWER_MAXVERTICES)

static bool TableWidget_GetCoords(struct TableWidget* w, int i, int* cellX, int* cellY) {
	int x, y;
	x = i % w->ElementsPerRow;
	y = i / w->ElementsPerRow - w->Scroll.ScrollY;

	*cellX = w->X + w->BlockSize * x;
	*cellY = w->Y + w->BlockSize * y + 3;
	return y >= 0 && y < TABLE_MAX_ROWS_DISPLAYED;
}

static void TableWidget_UpdateScrollbarPos(struct TableWidget* w) {
	struct ScrollbarWidget* scroll = &w->Scroll;
	scroll->X = Table_X(w) + Table_Width(w);
	scroll->Y = Table_Y(w);
	scroll->Height    = Table_Height(w);
	scroll->TotalRows = w->RowsCount;
}

static void TableWidget_MoveCursorToSelected(struct TableWidget* w) {
	Point2D topLeft;
	int x, y, idx;
	if (w->SelectedIndex == -1) return;

	idx = w->SelectedIndex;
	TableWidget_GetCoords(w, idx, &x, &y);
	x += w->BlockSize / 2; y += w->BlockSize / 2;

	topLeft = Window_PointToScreen(0, 0);
	x += topLeft.X; y += topLeft.Y;
	Window_SetScreenCursorPos(x, y);
}

static void TableWidget_MakeBlockDesc(String* desc, BlockID block) {
	String name;
	int block_ = block;
	if (Game_PureClassic) { String_AppendConst(desc, "Select block"); return; }

	name = Block_UNSAFE_GetName(block);
	String_AppendString(desc, &name);
	if (Game_ClassicMode) return;

	String_Format1(desc, " (ID %i&f", &block_);
	if (!Block_CanPlace[block])  { String_AppendConst(desc,  ", place &cNo&f"); }
	if (!Block_CanDelete[block]) { String_AppendConst(desc, ", delete &cNo&f"); }
	String_Append(desc, ')');
}

static void TableWidget_UpdateDescTexPos(struct TableWidget* w) {
	w->DescTex.X = w->X + w->Width / 2 - w->DescTex.Width / 2;
	w->DescTex.Y = w->Y - w->DescTex.Height - 5;
}

static void TableWidget_UpdatePos(struct TableWidget* w) {
	int rowsDisplayed = min(TABLE_MAX_ROWS_DISPLAYED, w->RowsCount);
	w->Width  = w->BlockSize * w->ElementsPerRow;
	w->Height = w->BlockSize * rowsDisplayed;
	w->X = Game_Width  / 2 - w->Width  / 2;
	w->Y = Game_Height / 2 - w->Height / 2;
	TableWidget_UpdateDescTexPos(w);
}

static void TableWidget_RecreateDescTex(struct TableWidget* w) {
	if (w->SelectedIndex == w->LastCreatedIndex) return;
	if (w->ElementsCount == 0) return;
	w->LastCreatedIndex = w->SelectedIndex;

	Gfx_DeleteTexture(&w->DescTex.ID);
	if (w->SelectedIndex == -1) return;
	TableWidget_MakeDescTex(w, w->Elements[w->SelectedIndex]);
}

void TableWidget_MakeDescTex(struct TableWidget* w, BlockID block) {
	String desc; char descBuffer[STRING_SIZE * 2];
	struct DrawTextArgs args;

	Gfx_DeleteTexture(&w->DescTex.ID);
	if (block == BLOCK_AIR) return;
	String_InitArray(desc, descBuffer);
	TableWidget_MakeBlockDesc(&desc, block);
	
	DrawTextArgs_Make(&args, &desc, &w->Font, true);
	Drawer2D_MakeTextTexture(&w->DescTex, &args, 0, 0);
	TableWidget_UpdateDescTexPos(w);
}

static bool TableWidget_RowEmpty(struct TableWidget* w, int start) {
	int i, end = min(start + w->ElementsPerRow, Array_Elems(Inventory_Map));

	for (i = start; i < end; i++) {
		if (Inventory_Map[i] != BLOCK_AIR) return false;
	}
	return true;
}

static void TableWidget_RecreateElements(struct TableWidget* w) {
	int i, max = Game_UseCPEBlocks ? BLOCK_COUNT : BLOCK_ORIGINAL_COUNT;
	BlockID block;
	w->ElementsCount = 0;

	for (i = 0; i < Array_Elems(Inventory_Map); ) {
		if ((i % w->ElementsPerRow) == 0 && TableWidget_RowEmpty(w, i)) {
			i += w->ElementsPerRow; continue;
		}

		block = Inventory_Map[i];
		if (block < max) { w->Elements[w->ElementsCount++] = block; }
		i++;
	}

	w->RowsCount = Math_CeilDiv(w->ElementsCount, w->ElementsPerRow);
	TableWidget_UpdateScrollbarPos(w);
	TableWidget_UpdatePos(w);
}

static void TableWidget_Init(void* widget) {
	struct TableWidget* w = widget;
	w->LastX = Mouse_X; w->LastY = Mouse_Y;

	ScrollbarWidget_Create(&w->Scroll);
	TableWidget_RecreateElements(w);
	Widget_Reposition(w);
}

static void TableWidget_Render(void* widget, double delta) {
	struct TableWidget* w = widget;
	VertexP3fT2fC4b vertices[TABLE_MAX_VERTICES];
	int i, x, y;

	/* These were sourced by taking a screenshot of vanilla
	Then using paint to extract the colour components
	Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topBackCol    = PACKEDCOL_CONST( 34,  34,  34, 168);
	PackedCol bottomBackCol = PACKEDCOL_CONST( 57,  57, 104, 202);
	PackedCol topSelCol     = PACKEDCOL_CONST(255, 255, 255, 142);
	PackedCol bottomSelCol  = PACKEDCOL_CONST(255, 255, 255, 192);

	GfxCommon_Draw2DGradient(Table_X(w), Table_Y(w),
		Table_Width(w), Table_Height(w), topBackCol, bottomBackCol);

	if (w->RowsCount > TABLE_MAX_ROWS_DISPLAYED) {
		Elem_Render(&w->Scroll, delta);
	}

	int blockSize = w->BlockSize;
	if (w->SelectedIndex != -1 && Game_ClassicMode) {
		TableWidget_GetCoords(w, w->SelectedIndex, &x, &y);

		float off = blockSize * 0.1f;
		int size = (int)(blockSize + off * 2);
		GfxCommon_Draw2DGradient((int)(x - off), (int)(y - off),
			size, size, topSelCol, bottomSelCol);
	}
	Gfx_SetTexturing(true);
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);

	IsometricDrawer_BeginBatch(vertices, w->VB);
	for (i = 0; i < w->ElementsCount; i++) {
		if (!TableWidget_GetCoords(w, i, &x, &y)) continue;

		/* We want to always draw the selected block on top of others */
		if (i == w->SelectedIndex) continue;
		IsometricDrawer_DrawBatch(w->Elements[i], blockSize * 0.7f / 2.0f,
			x + blockSize / 2, y + blockSize / 2);
	}

	i = w->SelectedIndex;
	if (i != -1) {
		TableWidget_GetCoords(w, i, &x, &y);

		IsometricDrawer_DrawBatch(w->Elements[i],
			(blockSize + w->SelBlockExpand) * 0.7f / 2.0f,
			x + blockSize / 2, y + blockSize / 2);
	}
	IsometricDrawer_EndBatch();

	if (w->DescTex.ID) { Texture_Render(&w->DescTex); }
	Gfx_SetTexturing(false);
}

static void TableWidget_Free(void* widget) {
	struct TableWidget* w = widget;
	Gfx_DeleteVb(&w->VB);
	Gfx_DeleteTexture(&w->DescTex.ID);
	w->LastCreatedIndex = -1000;
}

static void TableWidget_Recreate(void* widget) {
	struct TableWidget* w = widget;
	Elem_TryFree(w);
	w->VB = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, TABLE_MAX_VERTICES);
	TableWidget_RecreateDescTex(w);
}

static void TableWidget_Reposition(void* widget) {
	struct TableWidget* w = widget;
	float scale = Game_GetInventoryScale();
	w->BlockSize = (int)(50 * Math_SqrtF(scale));
	w->SelBlockExpand = 25.0f * Math_SqrtF(scale);

	TableWidget_UpdatePos(w);
	TableWidget_UpdateScrollbarPos(w);
}

static void TableWidget_ScrollRelative(struct TableWidget* w, int delta) {
	int start = w->SelectedIndex, index = start;
	index += delta;
	if (index < 0) index -= delta;
	if (index >= w->ElementsCount) index -= delta;
	w->SelectedIndex = index;

	/* adjust scrollbar by number of rows moved up/down */
	w->Scroll.ScrollY += (index / w->ElementsPerRow) - (start / w->ElementsPerRow);
	ScrollbarWidget_ClampScrollY(&w->Scroll);

	TableWidget_RecreateDescTex(w);
	TableWidget_MoveCursorToSelected(w);
}

static bool TableWidget_MouseDown(void* widget, int x, int y, MouseButton btn) {
	struct TableWidget* w = widget;
	w->PendingClose = false;
	if (btn != MouseButton_Left) return false;

	if (Elem_HandlesMouseDown(&w->Scroll, x, y, btn)) {
		return true;
	} else if (w->SelectedIndex != -1 && w->Elements[w->SelectedIndex] != BLOCK_AIR) {
		Inventory_SetSelectedBlock(w->Elements[w->SelectedIndex]);
		w->PendingClose = true;
		return true;
	} else if (Gui_Contains(Table_X(w), Table_Y(w), Table_Width(w), Table_Height(w), x, y)) {
		return true;
	}
	return false;
}

static bool TableWidget_MouseUp(void* widget, int x, int y, MouseButton btn) {
	struct TableWidget* w = widget;
	return Elem_HandlesMouseUp(&w->Scroll, x, y, btn);
}

static bool TableWidget_MouseScroll(void* widget, float delta) {
	struct TableWidget* w = widget;
	int startScrollY, index;

	bool bounds = Gui_Contains(Table_X(w), Table_Y(w),
		Table_Width(w) + w->Scroll.Width, Table_Height(w), Mouse_X, Mouse_Y);
	if (!bounds) return false;

	startScrollY = w->Scroll.ScrollY;
	Elem_HandlesMouseScroll(&w->Scroll, delta);
	if (w->SelectedIndex == -1) return true;

	index = w->SelectedIndex;
	index += (w->Scroll.ScrollY - startScrollY) * w->ElementsPerRow;
	if (index >= w->ElementsCount) index = -1;

	w->SelectedIndex = index;
	TableWidget_RecreateDescTex(w);
	return true;
}

static bool TableWidget_MouseMove(void* widget, int x, int y) {
	struct TableWidget* w = widget;
	int cellSize, maxHeight;
	int i, cellX, cellY;

	if (Elem_HandlesMouseMove(&w->Scroll, x, y)) return true;
	if (w->LastX == x && w->LastY == y) return true;
	w->LastX = x; w->LastY = y;

	w->SelectedIndex = -1;
	cellSize  = w->BlockSize;
	maxHeight = cellSize * TABLE_MAX_ROWS_DISPLAYED;

	if (Gui_Contains(w->X, w->Y + 3, w->Width, maxHeight - 3 * 2, x, y)) {
		for (i = 0; i < w->ElementsCount; i++) {
			TableWidget_GetCoords(w, i, &cellX, &cellY);

			if (Gui_Contains(cellX, cellY, cellSize, cellSize, x, y)) {
				w->SelectedIndex = i;
				break;
			}
		}
	}
	TableWidget_RecreateDescTex(w);
	return true;
}

static bool TableWidget_KeyDown(void* widget, Key key) {
	struct TableWidget* w = widget;
	if (w->SelectedIndex == -1) return false;

	if (key == Key_Left || key == Key_Keypad4) {
		TableWidget_ScrollRelative(w, -1);
	} else if (key == Key_Right || key == Key_Keypad6) {
		TableWidget_ScrollRelative(w, 1);
	} else if (key == Key_Up || key == Key_Keypad8) {
		TableWidget_ScrollRelative(w, -w->ElementsPerRow);
	} else if (key == Key_Down || key == Key_Keypad2) {
		TableWidget_ScrollRelative(w, w->ElementsPerRow);
	} else {
		return false;
	}
	return true;
}

static struct WidgetVTABLE TableWidget_VTABLE = {
	TableWidget_Init,      TableWidget_Render,  TableWidget_Free,      TableWidget_Recreate,
	TableWidget_KeyDown,   Widget_Key,          Widget_KeyPress,
	TableWidget_MouseDown, TableWidget_MouseUp, TableWidget_MouseMove, TableWidget_MouseScroll,
	TableWidget_Reposition,
};
void TableWidget_Create(struct TableWidget* w) {	
	Widget_Reset(w);
	w->VTABLE = &TableWidget_VTABLE;
	w->LastCreatedIndex = -1000;
}

void TableWidget_SetBlockTo(struct TableWidget* w, BlockID block) {
	int i;
	w->SelectedIndex = -1;
	
	for (i = 0; i < w->ElementsCount; i++) {
		if (w->Elements[i] == block) w->SelectedIndex = i;
	}
	/* When holding air, inventory should open at middle */
	if (block == BLOCK_AIR) w->SelectedIndex = -1;

	w->Scroll.ScrollY = w->SelectedIndex / w->ElementsPerRow;
	w->Scroll.ScrollY -= (TABLE_MAX_ROWS_DISPLAYED - 1);
	ScrollbarWidget_ClampScrollY(&w->Scroll);
	TableWidget_MoveCursorToSelected(w);
	TableWidget_RecreateDescTex(w);
}

void TableWidget_OnInventoryChanged(struct TableWidget* w) {
	TableWidget_RecreateElements(w);
	if (w->SelectedIndex >= w->ElementsCount) {
		w->SelectedIndex = w->ElementsCount - 1;
	}
	w->LastX = -1; w->LastY = -1;

	w->Scroll.ScrollY = w->SelectedIndex / w->ElementsPerRow;
	ScrollbarWidget_ClampScrollY(&w->Scroll);
	TableWidget_RecreateDescTex(w);
}


/*########################################################################################################################*
*-------------------------------------------------------InputWidget-------------------------------------------------------*
*#########################################################################################################################*/
static bool InputWidget_ControlDown(void) {
#ifdef CC_BUILD_OSX
	return Key_IsWinPressed();
#else
	return Key_IsControlPressed();
#endif
}

static void InputWidget_FormatLine(struct InputWidget* w, int i, String* line) {
	String src = w->Lines[i];
	if (!w->ConvertPercents) { String_AppendString(line, &src); return; }

	for (i = 0; i < src.length; i++) {
		char c = src.buffer[i];
		if (c == '%' && Drawer2D_ValidColCodeAt(&src, i + 1)) { c = '&'; }
		String_Append(line, c);
	}
}

static void InputWidget_CalculateLineSizes(struct InputWidget* w) {
	String line; char lineBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	Size2D size;
	int y;

	for (y = 0; y < INPUTWIDGET_MAX_LINES; y++) {
		w->LineSizes[y] = Size2D_Empty;
	}
	w->LineSizes[0].Width = w->PrefixWidth;
	DrawTextArgs_MakeEmpty(&args, &w->Font, true);

	String_InitArray(line, lineBuffer);
	for (y = 0; y < w->GetMaxLines(); y++) {
		line.length = 0;
		InputWidget_FormatLine(w, y, &line);

		args.Text = line;
		size = Drawer2D_MeasureText(&args);
		w->LineSizes[y].Width += size.Width;
		w->LineSizes[y].Height = size.Height;
	}

	if (w->LineSizes[0].Height == 0) {
		w->LineSizes[0].Height = w->PrefixHeight;
	}
}

static char InputWidget_GetLastCol(struct InputWidget* w, int x, int y) {
	String line; char lineBuffer[STRING_SIZE];
	char col;
	String_InitArray(line, lineBuffer);

	for (; y >= 0; y--) {
		line.length = 0;
		InputWidget_FormatLine(w, y, &line);

		col = Drawer2D_LastCol(&line, x);
		if (col) return col;
		if (y > 0) { x = w->Lines[y - 1].length; }
	}
	return '\0';
}

static void InputWidget_UpdateCaret(struct InputWidget* w) {
	BitmapCol col;
	String line; char lineBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	int maxChars, lineWidth;
	char colCode;
	
	maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (w->CaretPos >= maxChars) w->CaretPos = -1;
	WordWrap_GetCoords(w->CaretPos, w->Lines, w->GetMaxLines(), &w->CaretX, &w->CaretY);

	DrawTextArgs_MakeEmpty(&args, &w->Font, false);
	w->CaretAccumulator = 0;
	w->CaretTex.Width   = w->CaretWidth;

	/* Caret is at last character on line */
	if (w->CaretX == INPUTWIDGET_LEN) {
		lineWidth = w->LineSizes[w->CaretY].Width;	
	} else {
		String_InitArray(line, lineBuffer);
		InputWidget_FormatLine(w, w->CaretY, &line);

		args.Text = String_UNSAFE_Substring(&line, 0, w->CaretX);
		lineWidth = Drawer2D_MeasureText(&args).Width;
		if (w->CaretY == 0) lineWidth += w->PrefixWidth;

		if (w->CaretX < line.length) {
			args.Text = String_UNSAFE_Substring(&line, w->CaretX, 1);
			args.UseShadow = true;
			w->CaretTex.Width = Drawer2D_MeasureText(&args).Width;
		}
	}

	w->CaretTex.X = w->X + w->Padding + lineWidth;
	w->CaretTex.Y = w->InputTex.Y + w->CaretY * w->LineSizes[0].Height + 2;
	colCode = InputWidget_GetLastCol(w, w->CaretX, w->CaretY);

	if (colCode) {
		col = Drawer2D_GetCol(colCode);
		w->CaretCol.B = col.B; w->CaretCol.G = col.G; 
		w->CaretCol.R = col.R; w->CaretCol.A = col.A;
	} else {
		PackedCol white = PACKEDCOL_WHITE;
		w->CaretCol = PackedCol_Scale(white, 0.8f);
	}
}

static void InputWidget_RenderCaret(struct InputWidget* w, double delta) {
	float second;
	if (!w->ShowCaret) return;
	w->CaretAccumulator += delta;

	second = Math_Mod1((float)w->CaretAccumulator);
	if (second < 0.5f) Texture_RenderShaded(&w->CaretTex, w->CaretCol);
}

static void InputWidget_OnPressedEnter(void* widget) {
	struct InputWidget* w = widget;
	InputWidget_Clear(w);
	w->Height = w->PrefixHeight;
}

void InputWidget_Clear(struct InputWidget* w) {
	int i;
	w->Text.length = 0;
	
	for (i = 0; i < Array_Elems(w->Lines); i++) {
		w->Lines[i] = String_Empty;
	}

	w->CaretPos = -1;
	Gfx_DeleteTexture(&w->InputTex.ID);
}

static bool InputWidget_AllowedChar(void* widget, char c) {
	return Utils_IsValidInputChar(c, ServerConnection_SupportsFullCP437);
}

static void InputWidget_AppendChar(struct InputWidget* w, char c) {
	if (w->CaretPos == -1) {
		String_InsertAt(&w->Text, w->Text.length, c);
	} else {
		String_InsertAt(&w->Text, w->CaretPos, c);
		w->CaretPos++;
		if (w->CaretPos >= w->Text.length) { w->CaretPos = -1; }
	}
}

static bool InputWidget_TryAppendChar(struct InputWidget* w, char c) {
	int maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (w->Text.length >= maxChars) return false;
	if (!w->AllowedChar(w, c)) return false;

	InputWidget_AppendChar(w, c);
	return true;
}

void InputWidget_AppendString(struct InputWidget* w, const String* text) {
	int i, appended = 0;
	for (i = 0; i < text->length; i++) {
		if (InputWidget_TryAppendChar(w, text->buffer[i])) appended++;
	}

	if (!appended) return;
	Elem_Recreate(w);
}

void InputWidget_Append(struct InputWidget* w, char c) {
	if (!InputWidget_TryAppendChar(w, c)) return;
	Elem_Recreate(w);
}

static void InputWidget_DeleteChar(struct InputWidget* w) {
	if (!w->Text.length) return;

	if (w->CaretPos == -1) {
		String_DeleteAt(&w->Text, w->Text.length - 1);
	} else if (w->CaretPos > 0) {
		w->CaretPos--;
		String_DeleteAt(&w->Text, w->CaretPos);
	}
}

static bool InputWidget_CheckCol(struct InputWidget* w, int index) {
	char code, col;
	if (index < 0) return false;

	code = w->Text.buffer[index];
	col  = w->Text.buffer[index + 1];
	return (code == '%' || code == '&') && Drawer2D_ValidColCode(col);
}

static void InputWidget_BackspaceKey(struct InputWidget* w) {
	int i, len;

	if (InputWidget_ControlDown()) {
		if (w->CaretPos == -1) { w->CaretPos = w->Text.length - 1; }
		len = WordWrap_GetBackLength(&w->Text, w->CaretPos);
		if (!len) return;

		w->CaretPos -= len;
		if (w->CaretPos < 0) { w->CaretPos = 0; }

		for (i = 0; i <= len; i++) {
			String_DeleteAt(&w->Text, w->CaretPos);
		}

		if (w->CaretPos >= w->Text.length) { w->CaretPos = -1; }
		if (w->CaretPos == -1 && w->Text.length > 0) {
			String_InsertAt(&w->Text, w->Text.length, ' ');
		} else if (w->CaretPos >= 0 && w->Text.buffer[w->CaretPos] != ' ') {
			String_InsertAt(&w->Text, w->CaretPos, ' ');
		}
		Elem_Recreate(w);
	} else if (w->Text.length > 0 && w->CaretPos != 0) {
		int index = w->CaretPos == -1 ? w->Text.length - 1 : w->CaretPos;
		if (InputWidget_CheckCol(w, index - 1)) {
			InputWidget_DeleteChar(w); /* backspace XYZ%e to XYZ */
		} else if (InputWidget_CheckCol(w, index - 2)) {
			InputWidget_DeleteChar(w); 
			InputWidget_DeleteChar(w); /* backspace XYZ%eH to XYZ */
		}

		InputWidget_DeleteChar(w);
		Elem_Recreate(w);
	}
}

static void InputWidget_DeleteKey(struct InputWidget* w) {
	if (w->Text.length > 0 && w->CaretPos != -1) {
		String_DeleteAt(&w->Text, w->CaretPos);
		if (w->CaretPos >= w->Text.length) { w->CaretPos = -1; }
		Elem_Recreate(w);
	}
}

static void InputWidget_LeftKey(struct InputWidget* w) {
	if (InputWidget_ControlDown()) {
		if (w->CaretPos == -1) { w->CaretPos = w->Text.length - 1; }
		w->CaretPos -= WordWrap_GetBackLength(&w->Text, w->CaretPos);
		InputWidget_UpdateCaret(w);
		return;
	}

	if (w->Text.length > 0) {
		if (w->CaretPos == -1) { w->CaretPos = w->Text.length; }
		w->CaretPos--;
		if (w->CaretPos < 0) { w->CaretPos = 0; }
		InputWidget_UpdateCaret(w);
	}
}

static void InputWidget_RightKey(struct InputWidget* w) {
	if (InputWidget_ControlDown()) {
		w->CaretPos += WordWrap_GetForwardLength(&w->Text, w->CaretPos);
		if (w->CaretPos >= w->Text.length) { w->CaretPos = -1; }
		InputWidget_UpdateCaret(w);
		return;
	}

	if (w->Text.length > 0 && w->CaretPos != -1) {
		w->CaretPos++;
		if (w->CaretPos >= w->Text.length) { w->CaretPos = -1; }
		InputWidget_UpdateCaret(w);
	}
}

static void InputWidget_HomeKey(struct InputWidget* w) {
	if (!w->Text.length) return;
	w->CaretPos = 0;
	InputWidget_UpdateCaret(w);
}

static void InputWidget_EndKey(struct InputWidget* w) {
	w->CaretPos = -1;
	InputWidget_UpdateCaret(w);
}

static bool InputWidget_OtherKey(struct InputWidget* w, Key key) {
	String text; char textBuffer[INPUTWIDGET_MAX_LINES * STRING_SIZE];
	int maxChars;
	
	maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (!InputWidget_ControlDown()) return false;

	if (key == Key_V && w->Text.length < maxChars) {
		String_InitArray(text, textBuffer);
		Window_GetClipboardText(&text);

		String_TrimStart(&text);
		String_TrimEnd(&text);

		if (!text.length) return true;
		InputWidget_AppendString(w, &text);
		return true;
	} else if (key == Key_C) {
		if (!w->Text.length) return true;
		Window_SetClipboardText(&w->Text);
		return true;
	}
	return false;
}

static void InputWidget_Init(void* widget) {
	struct InputWidget* w = widget;
	int lines = w->GetMaxLines();

	if (lines > 1) {
		WordWrap_Do(&w->Text, w->Lines, lines, INPUTWIDGET_LEN);
	} else {
		w->Lines[0] = w->Text;
	}

	InputWidget_CalculateLineSizes(w);
	w->RemakeTexture(w);
	InputWidget_UpdateCaret(w);
}

static void InputWidget_Free(void* widget) {
	struct InputWidget* w = widget;
	Gfx_DeleteTexture(&w->InputTex.ID);
	Gfx_DeleteTexture(&w->CaretTex.ID);
}

static void InputWidget_Recreate(void* widget) {
	struct InputWidget* w = widget;
	Gfx_DeleteTexture(&w->InputTex.ID);
	InputWidget_Init(w);
}

static void InputWidget_Reposition(void* widget) {
	struct InputWidget* w = widget;
	int oldX = w->X, oldY = w->Y;
	Widget_CalcPosition(w);
	
	w->CaretTex.X += w->X - oldX; w->CaretTex.Y += w->Y - oldY;
	w->InputTex.X += w->X - oldX; w->InputTex.Y += w->Y - oldY;
}

static bool InputWidget_KeyDown(void* widget, Key key) {
	struct InputWidget* w = widget;
	if (key == Key_Left) {
		InputWidget_LeftKey(w);
	} else if (key == Key_Right) {
		InputWidget_RightKey(w);
	} else if (key == Key_BackSpace) {
		InputWidget_BackspaceKey(w);
	} else if (key == Key_Delete) {
		InputWidget_DeleteKey(w);
	} else if (key == Key_Home) {
		InputWidget_HomeKey(w);
	} else if (key == Key_End) {
		InputWidget_EndKey(w);
	} else if (!InputWidget_OtherKey(w, key)) {
		return false;
	}
	return true;
}

static bool InputWidget_KeyUp(void* widget, Key key) { return true; }

static bool InputWidget_KeyPress(void* widget, char keyChar) {
	struct InputWidget* w = widget;
	InputWidget_Append(w, keyChar);
	return true;
}

static bool InputWidget_MouseDown(void* widget, int x, int y, MouseButton button) {
	String line; char lineBuffer[STRING_SIZE];
	struct InputWidget* w = widget;
	struct DrawTextArgs args;
	int cx, cy, offset = 0;
	int charX, charWidth, charHeight;

	if (button != MouseButton_Left) return true;
	x -= w->InputTex.X; y -= w->InputTex.Y;

	DrawTextArgs_MakeEmpty(&args, &w->Font, true);
	charHeight = w->CaretTex.Height;
	String_InitArray(line, lineBuffer);

	for (cy = 0; cy < w->GetMaxLines(); cy++) {
		line.length = 0;
		InputWidget_FormatLine(w, cy, &line);
		if (!line.length) continue;

		for (cx = 0; cx < line.length; cx++) {
			args.Text = String_UNSAFE_Substring(&line, 0, cx);
			charX     = Drawer2D_MeasureText(&args).Width;
			if (cy == 0) charX += w->PrefixWidth;

			args.Text = String_UNSAFE_Substring(&line, cx, 1);
			charWidth = Drawer2D_MeasureText(&args).Width;

			if (Gui_Contains(charX, cy * charHeight, charWidth, charHeight, x, y)) {
				w->CaretPos = offset + cx;
				InputWidget_UpdateCaret(w);
				return true;
			}
		}
		offset += line.length;
	}

	w->CaretPos = -1;
	InputWidget_UpdateCaret(w);
	return true;
}

CC_NOINLINE static void InputWidget_Create(struct InputWidget* w, const FontDesc* font, STRING_REF const String* prefix) {
	static String caret = String_FromConst("_");
	struct DrawTextArgs args;
	Size2D size;
	Widget_Reset(w);

	w->Font           = *font;
	w->Prefix         = *prefix;
	w->CaretPos       = -1;
	w->OnPressedEnter = InputWidget_OnPressedEnter;
	w->AllowedChar    = InputWidget_AllowedChar;
	
	DrawTextArgs_Make(&args, &caret, font, true);
	Drawer2D_MakeTextTexture(&w->CaretTex, &args, 0, 0);
	w->CaretTex.Width = (uint16_t)((w->CaretTex.Width * 3) / 4);
	w->CaretWidth     = w->CaretTex.Width;

	if (!prefix->length) return;
	DrawTextArgs_Make(&args, prefix, font, true);
	size = Drawer2D_MeasureText(&args);
	w->PrefixWidth  = size.Width;  w->Width  = size.Width;
	w->PrefixHeight = size.Height; w->Height = size.Height;
}


/*########################################################################################################################*
*---------------------------------------------------MenuInputValidator----------------------------------------------------*
*#########################################################################################################################*/
static void Hex_Range(struct MenuInputValidator* v, String* range) {
	String_AppendConst(range, "&7(#000000 - #FFFFFF)");
}

static bool Hex_ValidChar(struct MenuInputValidator* v, char c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static bool Hex_ValidString(struct MenuInputValidator* v, const String* s) {
	return s->length <= 6;
}

static bool Hex_ValidValue(struct MenuInputValidator* v, const String* s) {
	PackedCol col;
	return PackedCol_TryParseHex(s, &col);
}

static struct MenuInputValidatorVTABLE HexInputValidator_VTABLE = {
	Hex_Range, Hex_ValidChar, Hex_ValidString, Hex_ValidValue,
};
struct MenuInputValidator MenuInputValidator_Hex(void) {
	struct MenuInputValidator v;
	v.VTABLE = &HexInputValidator_VTABLE;
	return v;
}

static void Int_Range(struct MenuInputValidator* v, String* range) {
	String_Format2(range, "&7(%i - %i)", &v->Meta._Int.Min, &v->Meta._Int.Max);
}

static bool Int_ValidChar(struct MenuInputValidator* v, char c) {
	return (c >= '0' && c <= '9') || c == '-';
}

static bool Int_ValidString(struct MenuInputValidator* v, const String* s) {
	int value;
	if (s->length == 1 && s->buffer[0] == '-') return true; /* input is just a minus sign */
	return Convert_TryParseInt(s, &value);
}

static bool Int_ValidValue(struct MenuInputValidator* v, const String* s) {
	int value, min = v->Meta._Int.Min, max = v->Meta._Int.Max;
	return Convert_TryParseInt(s, &value) && min <= value && value <= max;
}

static struct MenuInputValidatorVTABLE IntInputValidator_VTABLE = {
	Int_Range, Int_ValidChar, Int_ValidString, Int_ValidValue,
};
struct MenuInputValidator MenuInputValidator_Int(int min, int max) {
	struct MenuInputValidator v;
	v.VTABLE = &IntInputValidator_VTABLE;
	v.Meta._Int.Min = min;
	v.Meta._Int.Max = max;
	return v;
}

static void Seed_Range(struct MenuInputValidator* v, String* range) {
	String_AppendConst(range, "&7(an integer)");
}

static struct MenuInputValidatorVTABLE SeedInputValidator_VTABLE = {
	Seed_Range, Int_ValidChar, Int_ValidString, Int_ValidValue,
};
struct MenuInputValidator MenuInputValidator_Seed(void) {
	struct MenuInputValidator v = MenuInputValidator_Int(Int32_MinValue, Int32_MaxValue);
	v.VTABLE = &SeedInputValidator_VTABLE;
	return v;
}

static void Float_Range(struct MenuInputValidator* v, String* range) {
	String_Format2(range, "&7(%f2 - %f2)", &v->Meta._Float.Min, &v->Meta._Float.Max);
}

static bool Float_ValidChar(struct MenuInputValidator* v, char c) {
	return (c >= '0' && c <= '9') || c == '-' || c == '.' || c == ',';
}

static bool Float_ValidString(struct MenuInputValidator* v, const String* s) {
	float value;
	if (s->length == 1 && Float_ValidChar(v, s->buffer[0])) return true;
	return Convert_TryParseFloat(s, &value);
}

static bool Float_ValidValue(struct MenuInputValidator* v, const String* s) {
	float value, min = v->Meta._Float.Min, max = v->Meta._Float.Max;
	return Convert_TryParseFloat(s, &value) && min <= value && value <= max;
}

static struct MenuInputValidatorVTABLE FloatInputValidator_VTABLE = {
	Float_Range, Float_ValidChar, Float_ValidString, Float_ValidValue,
};
struct MenuInputValidator MenuInputValidator_Float(float min, float max) {
	struct MenuInputValidator v;
	v.VTABLE = &FloatInputValidator_VTABLE;
	v.Meta._Float.Min = min;
	v.Meta._Float.Max = max;
	return v;
}

static void Path_Range(struct MenuInputValidator* v, String* range) {
	String_AppendConst(range, "&7(Enter name)");
}

static bool Path_ValidChar(struct MenuInputValidator* v, char c) {
	return !(c == '/' || c == '\\' || c == '?' || c == '*' || c == ':'
		|| c == '<' || c == '>' || c == '|' || c == '"' || c == '.');
}
static bool Path_ValidString(struct MenuInputValidator* v, const String* s) { return true; }

static struct MenuInputValidatorVTABLE PathInputValidator_VTABLE = {
	Path_Range, Path_ValidChar, Path_ValidString, Path_ValidString,
};
struct MenuInputValidator MenuInputValidator_Path(void) {
	struct MenuInputValidator v;
	v.VTABLE = &PathInputValidator_VTABLE;
	return v;
}

struct MenuInputValidator MenuInputValidator_Enum(const char** names, int namesCount) {
	struct MenuInputValidator v;
	v.VTABLE           = NULL;
	v.Meta._Enum.Names = names;
	v.Meta._Enum.Count = namesCount;
	return v;
}

static void String_Range(struct MenuInputValidator* v, String* range) {
	String_AppendConst(range, "&7(Enter text)");
}

static bool String_ValidChar(struct MenuInputValidator* v, char c) {
	return c != '&' && Utils_IsValidInputChar(c, true);
}

static bool String_ValidString(struct MenuInputValidator* v, const String* s) {
	return s->length <= STRING_SIZE;
}

static struct MenuInputValidatorVTABLE StringInputValidator_VTABLE = {
	String_Range, String_ValidChar, String_ValidString, String_ValidString,
};
struct MenuInputValidator MenuInputValidator_String(void) {
	struct MenuInputValidator v;
	v.VTABLE = &StringInputValidator_VTABLE;
	return v;
}


/*########################################################################################################################*
*-----------------------------------------------------MenuInputWidget-----------------------------------------------------*
*#########################################################################################################################*/
static void MenuInputWidget_Render(void* widget, double delta) {
	struct InputWidget* w = widget;
	PackedCol backCol = PACKEDCOL_CONST(30, 30, 30, 200);

	Gfx_SetTexturing(false);
	GfxCommon_Draw2DFlat(w->X, w->Y, w->Width, w->Height, backCol);
	Gfx_SetTexturing(true);

	Texture_Render(&w->InputTex);
	InputWidget_RenderCaret(w, delta);
}

static void MenuInputWidget_RemakeTexture(void* widget) {
	String range; char rangeBuffer[STRING_SIZE];
	struct MenuInputWidget* w = widget;
	struct MenuInputValidator* v;
	struct DrawTextArgs args;
	struct Texture* tex;
	Size2D size, adjSize;
	Bitmap bmp;

	DrawTextArgs_Make(&args, &w->Base.Lines[0], &w->Base.Font, false);
	size = Drawer2D_MeasureText(&args);
	w->Base.CaretAccumulator = 0.0;

	String_InitArray(range, rangeBuffer);
	v = &w->Validator;
	v->VTABLE->GetRange(v, &range);

	/* Ensure we don't have 0 text height */
	if (size.Height == 0) {
		args.Text   = range;
		size.Height = Drawer2D_MeasureText(&args).Height;
		args.Text   = w->Base.Lines[0];
	}

	w->Base.Width  = max(size.Width,  w->MinWidth);
	w->Base.Height = max(size.Height, w->MinHeight);
	adjSize = size; adjSize.Width = w->Base.Width;

	Bitmap_AllocateClearedPow2(&bmp, adjSize.Width, adjSize.Height);
	{
		Drawer2D_DrawText(&bmp, &args, w->Base.Padding, 0);

		args.Text = range;
		Size2D hintSize = Drawer2D_MeasureText(&args);
		int hintX = adjSize.Width - hintSize.Width;
		if (size.Width + 3 < hintX) {
			Drawer2D_DrawText(&bmp, &args, hintX, 0);
		}
	}

	tex = &w->Base.InputTex;
	Drawer2D_Make2DTexture(tex, &bmp, adjSize, 0, 0);
	Mem_Free(bmp.Scan0);

	Widget_Reposition(&w->Base);
	tex->X = w->Base.X; tex->Y = w->Base.Y;
	if (size.Height < w->MinHeight) {
		tex->Y += w->MinHeight / 2 - size.Height / 2;
	}
}

static bool MenuInputWidget_AllowedChar(void* widget, char c) {
	struct InputWidget* w = widget;
	struct MenuInputValidator* v;
	int maxChars;
	bool valid;

	if (c == '&') return false;
	v = &((struct MenuInputWidget*)w)->Validator;

	if (!v->VTABLE->IsValidChar(v, c)) return false;
	maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (w->Text.length == maxChars) return false;

	/* See if the new string is in valid format */
	InputWidget_AppendChar(w, c);
	valid = v->VTABLE->IsValidString(v, &w->Text);
	InputWidget_DeleteChar(w);
	return valid;
}

static int MenuInputWidget_GetMaxLines(void) { return 1; }
static struct WidgetVTABLE MenuInputWidget_VTABLE = {
	InputWidget_Init,      MenuInputWidget_Render, InputWidget_Free,     InputWidget_Recreate,
	InputWidget_KeyDown,   InputWidget_KeyUp,      InputWidget_KeyPress,
	InputWidget_MouseDown, Widget_Mouse,           Widget_MouseMove,     Widget_MouseScroll,
	InputWidget_Reposition,
};
void MenuInputWidget_Create(struct MenuInputWidget* w, int width, int height, const String* text, const FontDesc* font, struct MenuInputValidator* validator) {
	InputWidget_Create(&w->Base, font, &String_Empty);
	w->Base.VTABLE = &MenuInputWidget_VTABLE;

	w->MinWidth  = width;
	w->MinHeight = height;
	w->Validator = *validator;

	w->Base.ConvertPercents = false;
	w->Base.Padding = 3;
	String_InitArray(w->Base.Text, w->__TextBuffer);

	w->Base.GetMaxLines   = MenuInputWidget_GetMaxLines;
	w->Base.RemakeTexture = MenuInputWidget_RemakeTexture;
	w->Base.AllowedChar   = MenuInputWidget_AllowedChar;

	Elem_Init(&w->Base);
	InputWidget_AppendString(&w->Base, text);
}


/*########################################################################################################################*
*-----------------------------------------------------ChatInputWidget-----------------------------------------------------*
*#########################################################################################################################*/
static void ChatInputWidget_RemakeTexture(void* widget) {
	String line; char lineBuffer[STRING_SIZE + 2];
	struct InputWidget* w = widget;
	struct DrawTextArgs args;
	Size2D size = { 0, 0 };
	Bitmap bmp; 
	char lastCol;
	int i, x, y = 0;

	for (i = 0; i < w->GetMaxLines(); i++) {
		size.Height += w->LineSizes[i].Height;
		size.Width   = max(size.Width, w->LineSizes[i].Width);
	}
	Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);

	DrawTextArgs_MakeEmpty(&args, &w->Font, true);
	if (w->Prefix.length) {
		args.Text = w->Prefix;
		Drawer2D_DrawText(&bmp, &args, 0, 0);
	}

	String_InitArray(line, lineBuffer);
	for (i = 0; i < Array_Elems(w->Lines); i++) {
		if (!w->Lines[i].length) break;
		line.length = 0;

		/* Colour code continues in next line */
		lastCol = InputWidget_GetLastCol(w, 0, i);
		if (!Drawer2D_IsWhiteCol(lastCol)) {
			String_Append(&line, '&'); String_Append(&line, lastCol);
		}
		/* Convert % to & for colour codes */
		InputWidget_FormatLine(w, i, &line);
		args.Text = line;

		x = i == 0 ? w->PrefixWidth : 0;
		Drawer2D_DrawText(&bmp, &args, x, y);
		y += w->LineSizes[i].Height;
	}

	Drawer2D_Make2DTexture(&w->InputTex, &bmp, size, 0, 0);
	Mem_Free(bmp.Scan0);
	w->CaretAccumulator = 0;

	w->Width  = size.Width;
	w->Height = y == 0 ? w->PrefixHeight : y;
	Widget_Reposition(w);
	w->InputTex.X = w->X + w->Padding;
	w->InputTex.Y = w->Y;
}

static void ChatInputWidget_Render(void* widget, double delta) {
	PackedCol backCol = PACKEDCOL_CONST(0, 0, 0, 127);
	struct InputWidget* w = widget;
	int x = w->X, y = w->Y;
	bool caretAtEnd;
	int i, width;

	Gfx_SetTexturing(false);
	for (i = 0; i < INPUTWIDGET_MAX_LINES; i++) {
		if (i > 0 && !w->LineSizes[i].Height) break;

		caretAtEnd = (w->CaretY == i) && (w->CaretX == INPUTWIDGET_LEN || w->CaretPos == -1);
		width      = w->LineSizes[i].Width + (caretAtEnd ? w->CaretTex.Width : 0);
		/* Cover whole window width to match original classic behaviour */
		if (Game_PureClassic) { width = max(width, Game_Width - x * 4); }
	
		GfxCommon_Draw2DFlat(x, y, width + w->Padding * 2, w->PrefixHeight, backCol);
		y += w->LineSizes[i].Height;
	}

	Gfx_SetTexturing(true);
	Texture_Render(&w->InputTex);
	InputWidget_RenderCaret(w, delta);
}

static void ChatInputWidget_OnPressedEnter(void* widget) {
	struct ChatInputWidget* w = widget;
	/* Don't want trailing spaces in output message */
	String text = w->Base.Text;
	String_TrimEnd(&text);
	if (text.length) { Chat_Send(&text, true); }

	w->OrigStr.length = 0;
	w->TypingLogPos = Chat_InputLog.Count; /* Index of newest entry + 1. */

	Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_2);
	Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_3);
	InputWidget_OnPressedEnter(widget);
}

static void ChatInputWidget_UpKey(struct InputWidget* w) {
	struct ChatInputWidget* W = (struct ChatInputWidget*)w;
	String prevInput;
	int pos;

	if (InputWidget_ControlDown()) {
		pos = w->CaretPos == -1 ? w->Text.length : w->CaretPos;
		if (pos < INPUTWIDGET_LEN) return;

		w->CaretPos = pos - INPUTWIDGET_LEN;
		InputWidget_UpdateCaret(w);
		return;
	}

	if (W->TypingLogPos == Chat_InputLog.Count) {
		String_Copy(&W->OrigStr, &w->Text);
	}

	if (!Chat_InputLog.Count) return;
	W->TypingLogPos--;
	w->Text.length = 0;

	if (W->TypingLogPos < 0) W->TypingLogPos = 0;
	prevInput = StringsBuffer_UNSAFE_Get(&Chat_InputLog, W->TypingLogPos);
	String_AppendString(&w->Text, &prevInput);

	w->CaretPos = -1;
	Elem_Recreate(w);
}

static void ChatInputWidget_DownKey(struct InputWidget* w) {
	struct ChatInputWidget* W = (struct ChatInputWidget*)w;
	String prevInput;
	int lines;

	if (InputWidget_ControlDown()) {
		lines = w->GetMaxLines();
		if (w->CaretPos == -1 || w->CaretPos >= (lines - 1) * INPUTWIDGET_LEN) return;

		w->CaretPos += INPUTWIDGET_LEN;
		InputWidget_UpdateCaret(w);
		return;
	}

	if (!Chat_InputLog.Count) return;
	W->TypingLogPos++;
	w->Text.length = 0;

	if (W->TypingLogPos >= Chat_InputLog.Count) {
		W->TypingLogPos = Chat_InputLog.Count;
		String_AppendString(&w->Text, &W->OrigStr);
	} else {
		prevInput = StringsBuffer_UNSAFE_Get(&Chat_InputLog, W->TypingLogPos);
		String_AppendString(&w->Text, &prevInput);
	}

	w->CaretPos = -1;
	Elem_Recreate(w);
}

static bool ChatInputWidget_IsNameChar(char c) {
	return c == '_' || c == '.' || (c >= '0' && c <= '9')
		|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void ChatInputWidget_TabKey(struct InputWidget* w) {
	String str; char strBuffer[STRING_SIZE];
	EntityID matches[TABLIST_MAX_NAMES];
	String part, name;
	int beg, end;
	int i, j, numMatches;
	char* buffer;

	end = w->CaretPos == -1 ? w->Text.length - 1 : w->CaretPos;
	beg = end;
	buffer = w->Text.buffer;

	/* e.g. if player typed "hi Nam", backtrack to "N" */
	while (beg >= 0 && ChatInputWidget_IsNameChar(buffer[beg])) beg--;
	beg++;
	if (end < 0 || beg > end) return;

	part = String_UNSAFE_Substring(&w->Text, beg, (end + 1) - beg);
	Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_3);
	numMatches = 0;

	for (i = 0; i < TABLIST_MAX_NAMES; i++) {
		EntityID id = (EntityID)i;
		if (!TabList_Valid(id)) continue;

		name = TabList_UNSAFE_GetPlayer(i);
		if (!String_CaselessContains(&name, &part)) continue;
		matches[numMatches++] = id;
	}

	if (numMatches == 1) {
		if (w->CaretPos == -1) end++;
		int len = end - beg;
		for (j = 0; j < len; j++) {
			String_DeleteAt(&w->Text, beg);
		}

		if (w->CaretPos != -1) w->CaretPos -= len;
		name = TabList_UNSAFE_GetPlayer(matches[0]);
		InputWidget_AppendString(w, &name);
	} else if (numMatches > 1) {
		String_InitArray(str, strBuffer);
		String_Format1(&str, "&e%i matching names: ", &numMatches);

		for (i = 0; i < numMatches; i++) {
			name = TabList_UNSAFE_GetPlayer(matches[i]);
			if ((str.length + name.length + 1) > STRING_SIZE) break;

			String_AppendString(&str, &name);
			String_Append(&str, ' ');
		}
		Chat_AddOf(&str, MSG_TYPE_CLIENTSTATUS_3);
	}
}

static bool ChatInputWidget_KeyDown(void* widget, Key key) {
	struct InputWidget* w = widget;
	if (key == Key_Tab)  { ChatInputWidget_TabKey(w);  return true; }
	if (key == Key_Up)   { ChatInputWidget_UpKey(w);   return true; }
	if (key == Key_Down) { ChatInputWidget_DownKey(w); return true; }
	return InputWidget_KeyDown(w, key);
}

static int ChatInputWidget_GetMaxLines(void) {
	return !Game_ClassicMode && ServerConnection_SupportsPartialMessages ? 3 : 1;
}

static struct WidgetVTABLE ChatInputWidget_VTABLE = {
	InputWidget_Init,        ChatInputWidget_Render, InputWidget_Free,     InputWidget_Recreate,
	ChatInputWidget_KeyDown, InputWidget_KeyUp,      InputWidget_KeyPress,
	InputWidget_MouseDown,   Widget_Mouse,           Widget_MouseMove,     Widget_MouseScroll,
	InputWidget_Reposition,
};
void ChatInputWidget_Create(struct ChatInputWidget* w, const FontDesc* font) {
	static String prefix = String_FromConst("> ");

	InputWidget_Create(&w->Base, font, &prefix);
	w->TypingLogPos = Chat_InputLog.Count; /* Index of newest entry + 1. */
	w->Base.VTABLE  = &ChatInputWidget_VTABLE;

	w->Base.ConvertPercents = !Game_ClassicMode;
	w->Base.ShowCaret       = true;
	w->Base.Padding         = 5;
	w->Base.GetMaxLines    = ChatInputWidget_GetMaxLines;
	w->Base.RemakeTexture  = ChatInputWidget_RemakeTexture;
	w->Base.OnPressedEnter = ChatInputWidget_OnPressedEnter;

	String_InitArray(w->Base.Text, w->__TextBuffer);
	String_InitArray(w->OrigStr,   w->__OrigBuffer);
}	


/*########################################################################################################################*
*----------------------------------------------------PlayerListWidget-----------------------------------------------------*
*#########################################################################################################################*/
#define GROUP_NAME_ID UInt16_MaxValue
#define LIST_COLUMN_PADDING 5
#define LIST_BOUNDS_SIZE 10
#define LIST_NAMES_PER_COLUMN 16

static void PlayerListWidget_DrawName(struct Texture* tex, struct PlayerListWidget* w, const String* name) {
	String tmp; char tmpBuffer[STRING_SIZE];
	struct DrawTextArgs args;

	if (Game_PureClassic) {
		String_InitArray(tmp, tmpBuffer);
		String_AppendColorless(&tmp, name);
	} else {
		tmp = *name;
	}

	DrawTextArgs_Make(&args, &tmp, &w->Font, !w->Classic);
	Drawer2D_MakeTextTexture(tex, &args, 0, 0);
	Drawer2D_ReducePadding_Tex(tex, w->Font.Size, 3);
}

static int PlayerListWidget_HighlightedName(struct PlayerListWidget* w, int x, int y) {
	struct Texture tex;
	int i;
	if (!w->Active) return -1;
	
	for (i = 0; i < w->NamesCount; i++) {
		if (!w->Textures[i].ID || w->IDs[i] == GROUP_NAME_ID) continue;

		tex = w->Textures[i];
		if (Gui_Contains(tex.X, tex.Y, tex.Width, tex.Height, x, y)) return i;
	}
	return -1;
}

void PlayerListWidget_GetNameUnder(struct PlayerListWidget* w, int x, int y, String* name) {
	String player;
	int i = PlayerListWidget_HighlightedName(w, x, y);
	if (i == -1) return;

	player = TabList_UNSAFE_GetPlayer(w->IDs[i]);
	String_AppendString(name, &player);
}

static void PlayerListWidget_UpdateTableDimensions(struct PlayerListWidget* w) {
	int width = w->XMax - w->XMin, height = w->YHeight;
	w->X = (w->XMin                     ) - LIST_BOUNDS_SIZE;
	w->Y = (Game_Height / 2 - height / 2) - LIST_BOUNDS_SIZE;
	w->Width  = width  + LIST_BOUNDS_SIZE * 2;
	w->Height = height + LIST_BOUNDS_SIZE * 2;
}

static int PlayerListWidget_GetColumnWidth(struct PlayerListWidget* w, int column) {
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(w->NamesCount, i + LIST_NAMES_PER_COLUMN);
	int maxWidth = 0;

	for (; i < end; i++) {
		maxWidth = max(maxWidth, w->Textures[i].Width);
	}
	return maxWidth + LIST_COLUMN_PADDING + w->ElementOffset;
}

static int PlayerListWidget_GetColumnHeight(struct PlayerListWidget* w, int column) {
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(w->NamesCount, i + LIST_NAMES_PER_COLUMN);
	int height = 0;

	for (; i < end; i++) {
		height += w->Textures[i].Height + 1;
	}
	return height;
}

static void PlayerListWidget_SetColumnPos(struct PlayerListWidget* w, int column, int x, int y) {
	struct Texture tex;
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(w->NamesCount, i + LIST_NAMES_PER_COLUMN);

	for (; i < end; i++) {
		tex = w->Textures[i];
		tex.X = x; tex.Y = y - 10;

		y += tex.Height + 1;
		/* offset player names a bit, compared to group name */
		if (!w->Classic && w->IDs[i] != GROUP_NAME_ID) {
			tex.X += w->ElementOffset;
		}
		w->Textures[i] = tex;
	}
}

static void PlayerListWidget_RepositionColumns(struct PlayerListWidget* w) {
	int x, y, width = 0, height;
	int col, columns;

	w->YHeight = 0;
	columns    = Math_CeilDiv(w->NamesCount, LIST_NAMES_PER_COLUMN);

	for (col = 0; col < columns; col++) {
		width += PlayerListWidget_GetColumnWidth(w, col);
		height = PlayerListWidget_GetColumnHeight(w, col);
		w->YHeight = max(height, w->YHeight);
	}

	if (width < 480) width = 480;
	w->XMin = Game_Width / 2 - width / 2;
	w->XMax = Game_Width / 2 + width / 2;

	x = w->XMin;
	y = Game_Height / 2 - w->YHeight / 2;

	for (col = 0; col < columns; col++) {
		PlayerListWidget_SetColumnPos(w, col, x, y);
		x += PlayerListWidget_GetColumnWidth(w, col);
	}
}

static void PlayerListWidget_Reposition(void* widget) {
	struct PlayerListWidget* w = widget;
	int i, y, oldX, oldY;

	y = Game_Height / 4 - w->Height / 2;
	w->YOffset = -max(0, y);

	oldX = w->X; oldY = w->Y;
	Widget_CalcPosition(w);	

	for (i = 0; i < w->NamesCount; i++) {
		w->Textures[i].X += w->X - oldX;
		w->Textures[i].Y += w->Y - oldY;
	}
}

static void PlayerListWidget_AddName(struct PlayerListWidget* w, EntityID id, int index) {
	String name;
	/* insert at end of list */
	if (index == -1) { index = w->NamesCount; w->NamesCount++; }

	name = TabList_UNSAFE_GetList(id);
	w->IDs[index] = id;
	PlayerListWidget_DrawName(&w->Textures[index], w, &name);
}

static void PlayerListWidget_DeleteAt(struct PlayerListWidget* w, int i) {
	struct Texture tex = w->Textures[i];
	Gfx_DeleteTexture(&tex.ID);
	struct Texture empty = { 0 };

	for (; i < w->NamesCount - 1; i++) {
		w->IDs[i]      = w->IDs[i + 1];
		w->Textures[i] = w->Textures[i + 1];
	}

	w->IDs[w->NamesCount]      = 0;
	w->Textures[w->NamesCount] = empty;
	w->NamesCount--;
}

static void PlayerListWidget_AddGroup(struct PlayerListWidget* w, int id, int* index) {
	String group;
	int i;
	group = TabList_UNSAFE_GetGroup(id);

	for (i = Array_Elems(w->IDs) - 1; i > (*index); i--) {
		w->IDs[i]      = w->IDs[i - 1];
		w->Textures[i] = w->Textures[i - 1];
	}
	
	w->IDs[*index] = GROUP_NAME_ID;
	PlayerListWidget_DrawName(&w->Textures[*index], w, &group);

	(*index)++;
	w->NamesCount++;
}

static int PlayerListWidget_GetGroupCount(struct PlayerListWidget* w, int id, int i) {
	String group, curGroup;
	int count;
	group = TabList_UNSAFE_GetGroup(id);

	for (count = 0; i < w->NamesCount; i++, count++) {
		curGroup = TabList_UNSAFE_GetGroup(w->IDs[i]);
		if (!String_CaselessEquals(&group, &curGroup)) break;
	}
	return count;
}

static int PlayerListWidget_PlayerCompare(int x, int y) {
	String xName; char xNameBuffer[STRING_SIZE];
	String yName; char yNameBuffer[STRING_SIZE];
	uint8_t xRank, yRank;
	String xNameRaw, yNameRaw;

	xRank = TabList_GroupRanks[x];
	yRank = TabList_GroupRanks[y];
	if (xRank != yRank) return (xRank < yRank ? -1 : 1);
	
	String_InitArray(xName, xNameBuffer);
	xNameRaw = TabList_UNSAFE_GetList(x);
	String_AppendColorless(&xName, &xNameRaw);

	String_InitArray(yName, yNameBuffer);
	yNameRaw = TabList_UNSAFE_GetList(y);
	String_AppendColorless(&yName, &yNameRaw);

	return String_Compare(&xName, &yName);
}

static int PlayerListWidget_GroupCompare(int x, int y) {
	String xGroup, yGroup;
	/* TODO: should we use colourless comparison? ClassicalSharp sorts groups with colours */
	xGroup = TabList_UNSAFE_GetGroup(x);
	yGroup = TabList_UNSAFE_GetGroup(y);
	return String_Compare(&xGroup, &yGroup);
}

struct PlayerListWidget* List_SortObj;
int (*List_SortCompare)(int x, int y);
static void PlayerListWidget_QuickSort(int left, int right) {
	struct Texture* values = List_SortObj->Textures; struct Texture value;
	uint16_t* keys = List_SortObj->IDs; uint16_t key;

	while (left < right) {
		int i = left, j = right;
		int pivot = keys[(i + j) / 2];

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

static void PlayerListWidget_SortEntries(struct PlayerListWidget* w) {
	int i, id, count;
	if (!w->NamesCount) return;

	List_SortObj = w;
	if (w->Classic) {
		List_SortCompare = PlayerListWidget_PlayerCompare;
		PlayerListWidget_QuickSort(0, w->NamesCount - 1);
		return;
	}

	/* Sort the list by group */
	/* Loop backwards, since DeleteAt() reduces NamesCount */
	for (i = w->NamesCount - 1; i >= 0; i--) {
		if (w->IDs[i] != GROUP_NAME_ID) continue;
		PlayerListWidget_DeleteAt(w, i);
	}
	List_SortCompare = PlayerListWidget_GroupCompare;
	PlayerListWidget_QuickSort(0, w->NamesCount - 1);

	/* Sort the entries in each group */
	List_SortCompare = PlayerListWidget_PlayerCompare;
	for (i = 0; i < w->NamesCount; ) {
		id = w->IDs[i];
		PlayerListWidget_AddGroup(w, id, &i);

		count = PlayerListWidget_GetGroupCount(w, id, i);
		PlayerListWidget_QuickSort(i, i + (count - 1));
		i += count;
	}
}

static void PlayerListWidget_SortAndReposition(struct PlayerListWidget* w) {
	PlayerListWidget_SortEntries(w);
	PlayerListWidget_RepositionColumns(w);
	PlayerListWidget_UpdateTableDimensions(w);
	PlayerListWidget_Reposition(w);
}

static void PlayerListWidget_TabEntryAdded(void* widget, int id) {
	struct PlayerListWidget* w = widget;
	PlayerListWidget_AddName(w, id, -1);
	PlayerListWidget_SortAndReposition(w);
}

static void PlayerListWidget_TabEntryChanged(void* widget, int id) {
	struct PlayerListWidget* w = widget;
	struct Texture tex;
	int i;

	for (i = 0; i < w->NamesCount; i++) {
		if (w->IDs[i] != id) continue;
		tex = w->Textures[i];

		Gfx_DeleteTexture(&tex.ID);
		PlayerListWidget_AddName(w, id, i);
		PlayerListWidget_SortAndReposition(w);
		return;
	}
}

static void PlayerListWidget_TabEntryRemoved(void* widget, int id) {
	struct PlayerListWidget* w = widget;
	int i;
	for (i = 0; i < w->NamesCount; i++) {
		if (w->IDs[i] != id) continue;

		PlayerListWidget_DeleteAt(w, i);
		PlayerListWidget_SortAndReposition(w);
		return;
	}
}

static void PlayerListWidget_Init(void* widget) {
	static String title = String_FromConst("Connected players:");
	struct PlayerListWidget* w = widget;
	int id;

	for (id = 0; id < TABLIST_MAX_NAMES; id++) {
		if (!TabList_Valid((EntityID)id)) continue;
		PlayerListWidget_AddName(w, (EntityID)id, -1);
	}

	PlayerListWidget_SortAndReposition(w); 
	TextWidget_Create(&w->Title, &title, &w->Font);
	Widget_SetLocation(&w->Title, ANCHOR_CENTRE, ANCHOR_MIN, 0, 0);

	Event_RegisterInt(&TabListEvents_Added,   w, PlayerListWidget_TabEntryAdded);
	Event_RegisterInt(&TabListEvents_Changed, w, PlayerListWidget_TabEntryChanged);
	Event_RegisterInt(&TabListEvents_Removed, w, PlayerListWidget_TabEntryRemoved);
}

static void PlayerListWidget_Render(void* widget, double delta) {
	struct PlayerListWidget* w = widget;
	struct TextWidget* title = &w->Title;
	struct Texture tex;
	int offset, height;
	int i, selectedI;
	PackedCol topCol    = PACKEDCOL_CONST( 0,  0,  0, 180);
	PackedCol bottomCol = PACKEDCOL_CONST(50, 50, 50, 205);

	Gfx_SetTexturing(false);
	offset = title->Height + 10;
	height = max(300, w->Height + title->Height);
	GfxCommon_Draw2DGradient(w->X, w->Y - offset, w->Width, height, topCol, bottomCol);

	Gfx_SetTexturing(true);
	title->YOffset = w->Y - offset + 5;
	Widget_Reposition(title);
	Elem_Render(title, delta);

	selectedI = PlayerListWidget_HighlightedName(w, Mouse_X, Mouse_Y);
	for (i = 0; i < w->NamesCount; i++) {
		if (!w->Textures[i].ID) continue;

		tex = w->Textures[i];
		if (i == selectedI) tex.X += 4;
		Texture_Render(&tex);
	}
}

static void PlayerListWidget_Free(void* widget) {
	struct PlayerListWidget* w = widget;
	int i;
	for (i = 0; i < w->NamesCount; i++) {
		Gfx_DeleteTexture(&w->Textures[i].ID);
	}

	Elem_TryFree(&w->Title);
	Event_UnregisterInt(&TabListEvents_Added,   w, PlayerListWidget_TabEntryAdded);
	Event_UnregisterInt(&TabListEvents_Changed, w, PlayerListWidget_TabEntryChanged);
	Event_UnregisterInt(&TabListEvents_Removed, w, PlayerListWidget_TabEntryRemoved);
}

static struct WidgetVTABLE PlayerListWidget_VTABLE = {
	PlayerListWidget_Init, PlayerListWidget_Render, PlayerListWidget_Free, Gui_DefaultRecreate,
	Widget_Key,	           Widget_Key,              Widget_KeyPress,
	Widget_Mouse,          Widget_Mouse,            Widget_MouseMove,      Widget_MouseScroll,
	PlayerListWidget_Reposition,
};
void PlayerListWidget_Create(struct PlayerListWidget* w, const FontDesc* font, bool classic) {
	Widget_Reset(w);
	w->VTABLE     = &PlayerListWidget_VTABLE;
	w->HorAnchor  = ANCHOR_CENTRE;
	w->VerAnchor  = ANCHOR_CENTRE;

	w->NamesCount = 0;
	w->Font       = *font;
	w->Classic    = classic;
	w->ElementOffset = classic ? 0 : 10;
}


/*########################################################################################################################*
*-----------------------------------------------------TextGroupWidget-----------------------------------------------------*
*#########################################################################################################################*/
#define TextGroupWidget_LineBuffer(w, i) ((w)->Buffer + (i) * TEXTGROUPWIDGET_LEN)
String TextGroupWidget_UNSAFE_Get(struct TextGroupWidget* w, int i) {
	int len = w->LineLengths[i];
	return String_Init(TextGroupWidget_LineBuffer(w, i), len, len);
}

void TextGroupWidget_GetText(struct TextGroupWidget* w, int index, String* text) {
	String line = TextGroupWidget_UNSAFE_Get(w, index);
	String_Copy(text, &line);
}

void TextGroupWidget_PushUpAndReplaceLast(struct TextGroupWidget* w, const String* text) {
	int max_index;
	int i, y = w->Y;

	Gfx_DeleteTexture(&w->Textures[0].ID);
	max_index = w->LinesCount - 1;

	/* Move contents of X line to X - 1 line */
	for (i = 0; i < max_index; i++) {
		char* dst = TextGroupWidget_LineBuffer(w, i);
		char* src = TextGroupWidget_LineBuffer(w, i + 1);
		uint8_t lineLen = w->LineLengths[i + 1];

		Mem_Copy(dst, src, lineLen);
		w->Textures[i]    = w->Textures[i + 1];
		w->LineLengths[i] = lineLen;

		w->Textures[i].Y = y;
		y += w->Textures[i].Height;
	}

	w->Textures[max_index].ID = GFX_NULL; /* Delete() is called by TextGroupWidget_SetText otherwise */
	TextGroupWidget_SetText(w, max_index, text);
}

static int TextGroupWidget_CalcY(struct TextGroupWidget* w, int index, int newHeight) {	
	struct Texture* textures = w->Textures;
	int deltaY = newHeight - textures[index].Height;
	int i, y = 0;

	if (w->VerAnchor == ANCHOR_MIN) {
		y = w->Y;
		for (i = 0; i < index; i++) {
			y += textures[i].Height;
		}
		for (i = index + 1; i < w->LinesCount; i++) {
			textures[i].Y += deltaY;
		}
	} else {
		y = Game_Height - w->YOffset;
		for (i = index + 1; i < w->LinesCount; i++) {
			y -= textures[i].Height;
		}

		y -= newHeight;
		for (i = 0; i < index; i++) {
			textures[i].Y -= deltaY;
		}
	}
	return y;
}

void TextGroupWidget_SetUsePlaceHolder(struct TextGroupWidget* w, int index, bool placeHolder) {
	int height;
	w->PlaceholderHeight[index] = placeHolder;
	if (w->Textures[index].ID) return;

	height = placeHolder ? w->DefaultHeight : 0;
	w->Textures[index].Y      = TextGroupWidget_CalcY(w, index, height);
	w->Textures[index].Height = height;
}

int TextGroupWidget_UsedHeight(struct TextGroupWidget* w) {
	struct Texture* textures = w->Textures;
	int i, height = 0;

	for (i = 0; i < w->LinesCount; i++) {
		if (textures[i].ID) break;
	}
	for (; i < w->LinesCount; i++) {
		height += textures[i].Height;
	}
	return height;
}

static void TextGroupWidget_Reposition(void* widget) {
	struct TextGroupWidget* w = widget;
	struct Texture* textures = w->Textures;
	int i, oldY = w->Y;

	Widget_CalcPosition(w);
	if (!w->LinesCount) return;

	for (i = 0; i < w->LinesCount; i++) {
		textures[i].X = Gui_CalcPos(w->HorAnchor, w->XOffset, textures[i].Width, Game_Width);
		textures[i].Y += w->Y - oldY;
	}
}

static void TextGroupWidget_UpdateDimensions(struct TextGroupWidget* w) {	
	struct Texture* textures = w->Textures;
	int i, width = 0, height = 0;

	for (i = 0; i < w->LinesCount; i++) {
		width = max(width, textures[i].Width);
		height += textures[i].Height;
	}

	w->Width  = width;
	w->Height = height;
	Widget_Reposition(w);
}

struct Portion { int16_t Beg, Len, LineBeg, LineLen; };
#define TEXTGROUPWIDGET_HTTP_LEN 7 /* length of http:// */
#define TEXTGROUPWIDGET_URL 0x8000
#define TEXTGROUPWIDGET_PACKED_LEN 0x7FFF

static int TextGroupWidget_NextUrl(char* chars, int charsLen, int i) {
	int start, left;

	for (; i < charsLen; i++) {
		if (!(chars[i] == 'h' || chars[i] == '&')) continue;
		left = charsLen - i;
		if (left < TEXTGROUPWIDGET_HTTP_LEN) return charsLen;

		/* colour codes at start of URL */
		start = i;
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

static int TextGroupWidget_UrlEnd(char* chars, int charsLen, int32_t* begs, int begsLen, int i) {
	int start = i, j;
	int next, left;
	bool isBeg;

	for (; i < charsLen && chars[i] != ' '; i++) {
		/* Is this character the start of a line */
		isBeg = false;
		for (j = 0; j < begsLen; j++) {
			if (i == begs[j]) { isBeg = true; break; }
		}

		/* Definitely not a multilined URL */
		if (!isBeg || i == start) continue;
		if (chars[i] != '>') break;

		/* Does this line start with "> ", making it a multiline */
		next = i + 1; left = charsLen - next;
		while (left >= 2 && chars[next] == '&') { left -= 2; next += 2; }
		if (left == 0 || chars[next] != ' ') break;

		i = next;
	}
	return i;
}

static void TextGroupWidget_Output(struct Portion bit, int lineBeg, int lineEnd, struct Portion** portions) {
	struct Portion* cur;
	int overBy, underBy;
	if (bit.Beg >= lineEnd || !bit.Len) return;

	bit.LineBeg = bit.Beg;
	bit.LineLen = bit.Len & TEXTGROUPWIDGET_PACKED_LEN;

	/* Adjust this portion to be within this line */
	if (bit.Beg >= lineBeg) {
	} else if (bit.Beg + bit.LineLen > lineBeg) {
		/* Adjust start of portion to be within this line */
		underBy = lineBeg - bit.Beg;
		bit.LineBeg += underBy; bit.LineLen -= underBy;
	} else { return; }

	/* Limit length of portion to be within this line */
	overBy = (bit.LineBeg + bit.LineLen) - lineEnd;
	if (overBy > 0) bit.LineLen -= overBy;

	bit.LineBeg -= lineBeg;
	if (!bit.LineLen) return;

	cur = *portions; *cur++ = bit; *portions = cur;
}

static int TextGroupWidget_Reduce(struct TextGroupWidget* w, char* chars, int target, struct Portion* portions) {
	struct Portion* start = portions;	
	int32_t begs[TEXTGROUPWIDGET_MAX_LINES];
	int32_t ends[TEXTGROUPWIDGET_MAX_LINES];
	struct Portion bit;
	int len, nextStart;
	int i, total = 0, end;

	for (i = 0; i < w->LinesCount; i++) {
		len = w->LineLengths[i];
		begs[i] = -1; ends[i] = -1;
		if (!len) continue;

		begs[i] = total;
		Mem_Copy(&chars[total], TextGroupWidget_LineBuffer(w, i), len);
		total += len; ends[i] = total;
	}

	end = 0;
	for (;;) {
		nextStart = TextGroupWidget_NextUrl(chars, total, end);

		/* add normal portion between urls */
		bit.Beg = end;
		bit.Len = nextStart - end;
		TextGroupWidget_Output(bit, begs[target], ends[target], &portions);

		if (nextStart == total) break;
		end = TextGroupWidget_UrlEnd(chars, total, begs, w->LinesCount, nextStart);

		/* add this url portion */
		bit.Beg = nextStart;
		bit.Len = (end - nextStart) | TEXTGROUPWIDGET_URL;
		TextGroupWidget_Output(bit, begs[target], ends[target], &portions);
	}
	return (int)(portions - start);
}

static void TextGroupWidget_FormatUrl(String* text, const String* url) {
	char* dst;
	int i;
	String_AppendColorless(text, url);

	/* Delete "> " multiline chars from URLs */
	dst = text->buffer;
	for (i = text->length - 2; i >= 0; i--) {
		if (dst[i] != '>' || dst[i + 1] != ' ') continue;

		String_DeleteAt(text, i + 1);
		String_DeleteAt(text, i);
	}
}

static bool TextGroupWidget_GetUrl(struct TextGroupWidget* w, String* text, int index, int mouseX) {
	char chars[TEXTGROUPWIDGET_MAX_LINES * TEXTGROUPWIDGET_LEN];
	struct Portion portions[2 * (TEXTGROUPWIDGET_LEN / TEXTGROUPWIDGET_HTTP_LEN)];
	struct Portion bit;
	struct DrawTextArgs args = { 0 };
	String line, url;
	int portionsCount;
	int i, x, width;

	mouseX -= w->Textures[index].X;
	args.UseShadow = true;
	line = TextGroupWidget_UNSAFE_Get(w, index);

	if (Game_ClassicMode) return false;
	portionsCount = TextGroupWidget_Reduce(w, chars, index, portions);

	for (i = 0, x = 0; i < portionsCount; i++) {
		bit = portions[i];
		args.Text = String_UNSAFE_Substring(&line, bit.LineBeg, bit.LineLen);
		args.Font = (bit.Len & TEXTGROUPWIDGET_URL) ? w->UnderlineFont : w->Font;

		width = Drawer2D_MeasureText(&args).Width;
		if ((bit.Len & TEXTGROUPWIDGET_URL) && mouseX >= x && mouseX < x + width) {
			bit.Len &= TEXTGROUPWIDGET_PACKED_LEN;
			url = String_Init(&chars[bit.Beg], bit.Len, bit.Len);

			TextGroupWidget_FormatUrl(text, &url);
			return true;
		}
		x += width;
	}
	return false;
}

void TextGroupWidget_GetSelected(struct TextGroupWidget* w, String* text, int x, int y) {
	struct Texture tex;
	String line;
	int i;

	for (i = 0; i < w->LinesCount; i++) {
		if (!w->Textures[i].ID) continue;
		tex = w->Textures[i];
		if (!Gui_Contains(tex.X, tex.Y, tex.Width, tex.Height, x, y)) continue;

		if (!TextGroupWidget_GetUrl(w, text, i, x)) {
			line = TextGroupWidget_UNSAFE_Get(w, i);
			String_AppendString(text, &line);
		}
		return;
	}
}

static bool TextGroupWidget_MightHaveUrls(struct TextGroupWidget* w) {
	String line;
	int i;
	if (Game_ClassicMode) return false;

	for (i = 0; i < w->LinesCount; i++) {
		if (!w->LineLengths[i]) continue;

		line = TextGroupWidget_UNSAFE_Get(w, i);
		if (String_IndexOf(&line, '/', 0) >= 0) return true;
	}
	return false;
}

static void TextGroupWidget_DrawAdvanced(struct TextGroupWidget* w, struct Texture* tex, struct DrawTextArgs* args, int index, const String* text) {
	char chars[TEXTGROUPWIDGET_MAX_LINES * TEXTGROUPWIDGET_LEN];
	struct Portion portions[2 * (TEXTGROUPWIDGET_LEN / TEXTGROUPWIDGET_HTTP_LEN)];
	struct Portion bit;
	Size2D size = { 0, 0 };
	Size2D partSizes[Array_Elems(portions)];
	Bitmap bmp;
	int portionsCount;
	int i, x;

	portionsCount = TextGroupWidget_Reduce(w, chars, index, portions);
	for (i = 0; i < portionsCount; i++) {
		bit = portions[i];

		args->Text = String_UNSAFE_Substring(text, bit.LineBeg, bit.LineLen);
		args->Font = (bit.Len & TEXTGROUPWIDGET_URL) ? w->UnderlineFont : w->Font;

		partSizes[i] = Drawer2D_MeasureText(args);
		size.Height = max(partSizes[i].Height, size.Height);
		size.Width += partSizes[i].Width;
	}
	
	Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
	{
		x = 0;
		for (i = 0; i < portionsCount; i++) {
			bit = portions[i];

			args->Text = String_UNSAFE_Substring(text, bit.LineBeg, bit.LineLen);
			args->Font = (bit.Len & TEXTGROUPWIDGET_URL) ? w->UnderlineFont : w->Font;

			Drawer2D_DrawText(&bmp, args, x, 0);
			x += partSizes[i].Width;
		}
		Drawer2D_Make2DTexture(tex, &bmp, size, 0, 0);
	}
	Mem_Free(bmp.Scan0);
}

void TextGroupWidget_SetText(struct TextGroupWidget* w, int index, const String* text_orig) {
	String text = *text_orig;
	struct DrawTextArgs args;
	struct Texture tex = { 0 };
	Gfx_DeleteTexture(&w->Textures[index].ID);

	text.length = min(text.length, TEXTGROUPWIDGET_LEN);
	Mem_Copy(TextGroupWidget_LineBuffer(w, index), text.buffer, text.length);
	w->LineLengths[index] = (uint8_t)text.length;
	
	if (!Drawer2D_IsEmptyText(&text)) {
		DrawTextArgs_Make(&args, &text, &w->Font, true);

		if (!TextGroupWidget_MightHaveUrls(w)) {
			Drawer2D_MakeTextTexture(&tex, &args, 0, 0);
		} else {
			TextGroupWidget_DrawAdvanced(w, &tex, &args, index, &text);
		}
		Drawer2D_ReducePadding_Tex(&tex, w->Font.Size, 3);
	} else {
		tex.Height = w->PlaceholderHeight[index] ? w->DefaultHeight : 0;
	}

	tex.X = Gui_CalcPos(w->HorAnchor, w->XOffset, tex.Width, Game_Width);
	tex.Y = TextGroupWidget_CalcY(w, index, tex.Height);
	w->Textures[index] = tex;
	TextGroupWidget_UpdateDimensions(w);
}


static void TextGroupWidget_Init(void* widget) {
	struct TextGroupWidget* w = widget;
	int i, height;
	
	height = Drawer2D_FontHeight(&w->Font, true);
	Drawer2D_ReducePadding_Height(&height, w->Font.Size, 3);
	w->DefaultHeight = height;

	for (i = 0; i < w->LinesCount; i++) {
		w->Textures[i].Height = height;
		w->PlaceholderHeight[i] = true;
	}
	TextGroupWidget_UpdateDimensions(w);
}

static void TextGroupWidget_Render(void* widget, double delta) {
	struct TextGroupWidget* w = widget;
	struct Texture* textures  = w->Textures;
	int i;

	for (i = 0; i < w->LinesCount; i++) {
		if (!textures[i].ID) continue;
		Texture_Render(&textures[i]);
	}
}

static void TextGroupWidget_Free(void* widget) {
	struct TextGroupWidget* w = widget;
	int i;

	for (i = 0; i < w->LinesCount; i++) {
		w->LineLengths[i] = 0;
		Gfx_DeleteTexture(&w->Textures[i].ID);
	}
}

static struct WidgetVTABLE TextGroupWidget_VTABLE = {
	TextGroupWidget_Init, TextGroupWidget_Render, TextGroupWidget_Free, Gui_DefaultRecreate,
	Widget_Key,	          Widget_Key,             Widget_KeyPress,
	Widget_Mouse,         Widget_Mouse,           Widget_MouseMove,     Widget_MouseScroll,
	TextGroupWidget_Reposition,
};
void TextGroupWidget_Create(struct TextGroupWidget* w, int lines, const FontDesc* font, const FontDesc* ulFont, STRING_REF struct Texture* textures, STRING_REF char* buffer) {
	Widget_Reset(w);
	w->VTABLE = &TextGroupWidget_VTABLE;

	w->LinesCount = lines;
	w->Font       = *font;
	w->UnderlineFont = *ulFont;
	w->Textures   = textures;
	w->Buffer     = buffer;
}


/*########################################################################################################################*
*---------------------------------------------------SpecialInputWidget----------------------------------------------------*
*#########################################################################################################################*/
static void SpecialInputWidget_UpdateColString(struct SpecialInputWidget* w) {
	int i;
	String_InitArray(w->ColString, w->__ColBuffer);

	for (i = 0; i < DRAWER2D_MAX_COLS; i++) {
		if (i >= 'A' && i <= 'F') continue;
		if (Drawer2D_Cols[i].A == 0) continue;

		String_Append(&w->ColString, '&'); String_Append(&w->ColString, (char)i);
		String_Append(&w->ColString, '%'); String_Append(&w->ColString, (char)i);
	}
}

static bool SpecialInputWidget_IntersectsTitle(struct SpecialInputWidget* w, int x, int y) {
	Size2D size;
	int i, titleX = 0;

	for (i = 0; i < Array_Elems(w->Tabs); i++) {
		size = w->Tabs[i].TitleSize;
		if (Gui_Contains(titleX, 0, size.Width, size.Height, x, y)) {
			w->SelectedIndex = i;
			return true;
		}
		titleX += size.Width;
	}
	return false;
}

static void SpecialInputWidget_IntersectsBody(struct SpecialInputWidget* w, int x, int y) {
	struct SpecialInputTab e = w->Tabs[w->SelectedIndex];
	String str;
	int i;

	y -= w->Tabs[0].TitleSize.Height;
	x /= w->ElementSize.Width; y /= w->ElementSize.Height;
	
	i = (x + y * e.ItemsPerRow) * e.CharsPerItem;
	if (i >= e.Contents.length) return;

	/* TODO: need to insert characters that don't affect w->CaretPos index, adjust w->CaretPos colour */
	str = String_Init(&e.Contents.buffer[i], e.CharsPerItem, 0);
	InputWidget_AppendString(w->AppendObj, &str);
}

static void SpecialInputTab_Init(struct SpecialInputTab* tab, STRING_REF String* title, int itemsPerRow, int charsPerItem, STRING_REF String* contents) {
	tab->Title     = *title;
	tab->TitleSize = Size2D_Empty;
	tab->Contents  = *contents;
	tab->ItemsPerRow  = itemsPerRow;
	tab->CharsPerItem = charsPerItem;
}

static void SpecialInputWidget_InitTabs(struct SpecialInputWidget* w) {
	static String title_cols = String_FromConst("Colours");
	static String title_math = String_FromConst("Math");
	static String tab_math   = String_FromConst("\x9F\xAB\xAC\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xFB\xFC\xFD");
	static String title_line = String_FromConst("Line/Box");
	static String tab_line   = String_FromConst("\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xFE");
	static String title_letters = String_FromConst("Letters");
	static String tab_letters   = String_FromConst("\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\xA0\xA1\xA2\xA3\xA4\xA5");
	static String title_other = String_FromConst("Other");
	static String tab_other   = String_FromConst("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x7F\x9B\x9C\x9D\x9E\xA6\xA7\xA8\xA9\xAA\xAD\xAE\xAF\xF9\xFA");

	SpecialInputWidget_UpdateColString(w);
	SpecialInputTab_Init(&w->Tabs[0], &title_cols,    10, 4, &w->ColString);
	SpecialInputTab_Init(&w->Tabs[1], &title_math,    16, 1, &tab_math);
	SpecialInputTab_Init(&w->Tabs[2], &title_line,    17, 1, &tab_line);
	SpecialInputTab_Init(&w->Tabs[3], &title_letters, 17, 1, &tab_letters);
	SpecialInputTab_Init(&w->Tabs[4], &title_other,   16, 1, &tab_other);
}

#define SPECIAL_TITLE_SPACING 10
#define SPECIAL_CONTENT_SPACING 5
static Size2D SpecialInputWidget_MeasureTitles(struct SpecialInputWidget* w) {
	struct DrawTextArgs args; 
	Size2D size = { 0, 0 };
	int i;

	DrawTextArgs_MakeEmpty(&args, &w->Font, false);
	for (i = 0; i < Array_Elems(w->Tabs); i++) {
		args.Text = w->Tabs[i].Title;

		w->Tabs[i].TitleSize = Drawer2D_MeasureText(&args);
		w->Tabs[i].TitleSize.Width += SPECIAL_TITLE_SPACING;
		size.Width += w->Tabs[i].TitleSize.Width;
	}

	size.Height = w->Tabs[0].TitleSize.Height;
	return size;
}

static void SpecialInputWidget_DrawTitles(struct SpecialInputWidget* w, Bitmap* bmp) {
	BitmapCol col_selected = BITMAPCOL_CONST(30, 30, 30, 200);
	BitmapCol col_inactive = BITMAPCOL_CONST( 0,  0,  0, 127);
	BitmapCol col;

	struct DrawTextArgs args;
	Size2D size;
	int i, x = 0;

	DrawTextArgs_MakeEmpty(&args, &w->Font, false);
	for (i = 0; i < Array_Elems(w->Tabs); i++) {
		args.Text = w->Tabs[i].Title;
		col  = i == w->SelectedIndex ? col_selected : col_inactive;
		size = w->Tabs[i].TitleSize;

		Drawer2D_Clear(bmp, col, x, 0, size.Width, size.Height);
		Drawer2D_DrawText(bmp, &args, x + SPECIAL_TITLE_SPACING / 2, 0);
		x += size.Width;
	}
}

static Size2D SpecialInputWidget_MeasureContent(struct SpecialInputWidget* w, struct SpecialInputTab* tab) {
	struct DrawTextArgs args;
	Size2D size;
	int i, rows;
	
	int maxWidth = 0;
	DrawTextArgs_MakeEmpty(&args, &w->Font, false);
	args.Text.length = tab->CharsPerItem;

	for (i = 0; i < tab->Contents.length; i += tab->CharsPerItem) {
		args.Text.buffer = &tab->Contents.buffer[i];
		size     = Drawer2D_MeasureText(&args);
		maxWidth = max(maxWidth, size.Width);
	}

	w->ElementSize.Width  = maxWidth    + SPECIAL_CONTENT_SPACING;
	w->ElementSize.Height = size.Height + SPECIAL_CONTENT_SPACING;
	rows = Math_CeilDiv(tab->Contents.length / tab->CharsPerItem, tab->ItemsPerRow);

	size.Width  = w->ElementSize.Width  * tab->ItemsPerRow;
	size.Height = w->ElementSize.Height * rows;
	return size;
}

static void SpecialInputWidget_DrawContent(struct SpecialInputWidget* w, struct SpecialInputTab* tab, Bitmap* bmp, int yOffset) {
	struct DrawTextArgs args;
	int i, x, y, item;	

	int wrap = tab->ItemsPerRow;
	DrawTextArgs_MakeEmpty(&args, &w->Font, false);
	args.Text.length = tab->CharsPerItem;

	for (i = 0; i < tab->Contents.length; i += tab->CharsPerItem) {
		args.Text.buffer = &tab->Contents.buffer[i];
		item = i / tab->CharsPerItem;

		x = (item % wrap) * w->ElementSize.Width;
		y = (item / wrap) * w->ElementSize.Height + yOffset;
		Drawer2D_DrawText(bmp, &args, x, y);
	}
}

static void SpecialInputWidget_Make(struct SpecialInputWidget* w, struct SpecialInputTab* tab) {
	BitmapCol col = PACKEDCOL_CONST(30, 30, 30, 200);
	Size2D size, titles, content;
	Bitmap bmp;

	titles  = SpecialInputWidget_MeasureTitles(w);
	content = SpecialInputWidget_MeasureContent(w, tab);	

	size.Width  = max(titles.Width, content.Width);
	size.Height = titles.Height + content.Height;
	Gfx_DeleteTexture(&w->Tex.ID);

	Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
	{
		SpecialInputWidget_DrawTitles(w, &bmp);
		Drawer2D_Clear(&bmp, col, 0, titles.Height, size.Width, content.Height);
		SpecialInputWidget_DrawContent(w, tab, &bmp, titles.Height);
	}
	Drawer2D_Make2DTexture(&w->Tex, &bmp, size, w->X, w->Y);
	Mem_Free(bmp.Scan0);
}

static void SpecialInputWidget_Redraw(struct SpecialInputWidget* w) {
	SpecialInputWidget_Make(w, &w->Tabs[w->SelectedIndex]);
	w->Width  = w->Tex.Width;
	w->Height = w->Tex.Height;
	w->PendingRedraw = false;
}

static void SpecialInputWidget_Init(void* widget) {
	struct SpecialInputWidget* w = widget;
	w->X = 5; w->Y = 5;

	SpecialInputWidget_InitTabs(w);
	SpecialInputWidget_Redraw(w);
	SpecialInputWidget_SetActive(w, w->Active);
}

static void SpecialInputWidget_Render(void* widget, double delta) {
	struct SpecialInputWidget* w = widget;
	Texture_Render(&w->Tex);
}

static void SpecialInputWidget_Free(void* widget) {
	struct SpecialInputWidget* w = widget;
	Gfx_DeleteTexture(&w->Tex.ID);
}

static bool SpecialInputWidget_MouseDown(void* widget, int x, int y, MouseButton btn) {
	struct SpecialInputWidget* w = widget;
	x -= w->X; y -= w->Y;

	if (SpecialInputWidget_IntersectsTitle(w, x, y)) {
		SpecialInputWidget_Redraw(w);
	} else {
		SpecialInputWidget_IntersectsBody(w, x, y);
	}
	return true;
}

void SpecialInputWidget_UpdateCols(struct SpecialInputWidget* w) {
	SpecialInputWidget_UpdateColString(w);
	w->Tabs[0].Contents = w->ColString;
	if (w->SelectedIndex != 0) return;

	/* defer updating colours tab until visible */
	if (!w->Active) { w->PendingRedraw = true; return; }
	SpecialInputWidget_Redraw(w);
}

void SpecialInputWidget_SetActive(struct SpecialInputWidget* w, bool active) {
	w->Active = active;
	w->Height = active ? w->Tex.Height : 0;
	if (active && w->PendingRedraw) SpecialInputWidget_Redraw(w);
}

static struct WidgetVTABLE SpecialInputWidget_VTABLE = {
	SpecialInputWidget_Init,      SpecialInputWidget_Render, SpecialInputWidget_Free, Gui_DefaultRecreate,
	Widget_Key,                   Widget_Key,                Widget_KeyPress,
	SpecialInputWidget_MouseDown, Widget_Mouse,              Widget_MouseMove,        Widget_MouseScroll,
	Widget_CalcPosition,
};
void SpecialInputWidget_Create(struct SpecialInputWidget* w, const FontDesc* font, struct InputWidget* appendObj) {
	Widget_Reset(w);
	w->VTABLE    = &SpecialInputWidget_VTABLE;
	w->VerAnchor = ANCHOR_MAX;
	w->Font      = *font;
	w->AppendObj = appendObj;
}
