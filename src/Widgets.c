#include "Widgets.h"
#include "Graphics.h"
#include "Drawer2D.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Model.h"
#include "Screens.h"
#include "Platform.h"
#include "Server.h"
#include "Event.h"
#include "Chat.h"
#include "Game.h"
#include "Logger.h"
#include "Bitmap.h"
#include "Block.h"
#include "Input.h"

#define Widget_UV(u1,v1, u2,v2) Tex_UV(u1/256.0f,v1/256.0f, u2/256.0f,v2/256.0f)
static void Widget_NullFunc(void* widget) { }
static int  Widget_Pointer(void* elem, int id, int x, int y) { return false; }
static void Widget_InputUp(void* elem, int key)   { }
static int  Widget_InputDown(void* elem, int key) { return false; }
static void Widget_PointerUp(void* elem, int id, int x, int y) { }
static int  Widget_PointerMove(void* elem, int id, int x, int y) { return false; }
static int  Widget_MouseScroll(void* elem, float delta) { return false; }

/*########################################################################################################################*
*-------------------------------------------------------TextWidget--------------------------------------------------------*
*#########################################################################################################################*/
static void TextWidget_Render(void* widget, double delta) {
	struct TextWidget* w = (struct TextWidget*)widget;
	if (w->tex.ID) Texture_RenderShaded(&w->tex, w->color);
}

static void TextWidget_Free(void* widget) {
	struct TextWidget* w = (struct TextWidget*)widget;
	Gfx_DeleteTexture(&w->tex.ID);
}

static void TextWidget_Reposition(void* widget) {
	struct TextWidget* w = (struct TextWidget*)widget;
	Widget_CalcPosition(w);
	w->tex.x = w->x; w->tex.y = w->y;
}

static void TextWidget_BuildMesh(void* widget, struct VertexTextured** vertices) {
	struct TextWidget* w = (struct TextWidget*)widget;
	Gfx_Make2DQuad(&w->tex, w->color, vertices);
}

static int TextWidget_Render2(void* widget, int offset) {
	struct TextWidget* w = (struct TextWidget*)widget;
	if (w->tex.ID) {
		Gfx_BindTexture(w->tex.ID);
		Gfx_DrawVb_IndexedTris_Range(4, offset);
	}
	return offset + 4;
}

static int TextWidget_MaxVertices(void* widget) { return TEXTWIDGET_MAX; }

static const struct WidgetVTABLE TextWidget_VTABLE = {
	TextWidget_Render, TextWidget_Free,  TextWidget_Reposition,
	Widget_InputDown,  Widget_InputUp,   Widget_MouseScroll,
	Widget_Pointer,    Widget_PointerUp, Widget_PointerMove,
	TextWidget_BuildMesh, TextWidget_Render2, TextWidget_MaxVertices
};
void TextWidget_Init(struct TextWidget* w) {
	Widget_Reset(w);
	w->VTABLE = &TextWidget_VTABLE;
	w->color  = PACKEDCOL_WHITE;
}

void TextWidget_Set(struct TextWidget* w, const cc_string* text, struct FontDesc* font) {
	struct DrawTextArgs args;
	Gfx_DeleteTexture(&w->tex.ID);
	DrawTextArgs_Make(&args, text, font, true);
	Drawer2D_MakeTextTexture(&w->tex, &args);

	/* Give text widget default height when text is empty */
	if (!w->tex.Height) {
		w->tex.Height = Font_CalcHeight(font, true);
	}

	w->width = w->tex.Width; w->height = w->tex.Height;
	Widget_Layout(w);
}

void TextWidget_SetConst(struct TextWidget* w, const char* text, struct FontDesc* font) {
	cc_string str = String_FromReadonly(text);
	TextWidget_Set(w, &str, font);
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
#define BUTTON_uWIDTH (200.0f / 256.0f)

static struct Texture btnShadowTex   = { 0, Tex_Rect(0,0, 0,0), Widget_UV(0,66, 200,86)  };
static struct Texture btnSelectedTex = { 0, Tex_Rect(0,0, 0,0), Widget_UV(0,86, 200,106) };
static struct Texture btnDisabledTex = { 0, Tex_Rect(0,0, 0,0), Widget_UV(0,46, 200,66)  };

static void ButtonWidget_Free(void* widget) {
	struct ButtonWidget* w = (struct ButtonWidget*)widget;
	Gfx_DeleteTexture(&w->tex.ID);
}

static void ButtonWidget_Reposition(void* widget) {
	struct ButtonWidget* w = (struct ButtonWidget*)widget;
	w->width  = max(w->tex.Width,  w->minWidth);
	w->height = max(w->tex.Height, w->minHeight);

	Widget_CalcPosition(w);
	w->tex.x = w->x + (w->width  / 2 - w->tex.Width  / 2);
	w->tex.y = w->y + (w->height / 2 - w->tex.Height / 2);
}

static void ButtonWidget_Render(void* widget, double delta) {
	PackedCol normColor     = PackedCol_Make(224, 224, 224, 255);
	PackedCol activeColor   = PackedCol_Make(255, 255, 160, 255);
	PackedCol disabledColor = PackedCol_Make(160, 160, 160, 255);
	PackedCol color;

	struct ButtonWidget* w = (struct ButtonWidget*)widget;
	struct Texture back;	
	float scale;
		
	back = w->active ? btnSelectedTex : btnShadowTex;
	if (w->flags & WIDGET_FLAG_DISABLED) back = btnDisabledTex;

	back.ID = Gui.ClassicTexture ? Gui.GuiClassicTex : Gui.GuiTex;
	back.x = w->x; back.Width  = w->width;
	back.y = w->y; back.Height = w->height;

	/* TODO: Does this 400 need to take DPI into account */
	if (w->width >= 400) {
		/* Button can be drawn normally */
		Texture_Render(&back);
	} else {
		/* Split button down the middle */
		scale = (w->width / 400.0f) / (2 * DisplayInfo.ScaleX);
		Gfx_BindTexture(back.ID); /* avoid bind twice */

		back.Width = (w->width / 2);
		back.uv.U1 = 0.0f; back.uv.U2 = BUTTON_uWIDTH * scale;
		Gfx_Draw2DTexture(&back, w->color);

		back.x += (w->width / 2);
		back.uv.U1 = BUTTON_uWIDTH * (1.0f - scale); back.uv.U2 = BUTTON_uWIDTH;
		Gfx_Draw2DTexture(&back, w->color);
	}

	if (!w->tex.ID) return;
	color = (w->flags & WIDGET_FLAG_DISABLED) ? disabledColor 
											: (w->active ? activeColor : normColor);
	Texture_RenderShaded(&w->tex, color);
}

static void ButtonWidget_BuildMesh(void* widget, struct VertexTextured** vertices) {
	PackedCol normColor     = PackedCol_Make(224, 224, 224, 255);
	PackedCol activeColor   = PackedCol_Make(255, 255, 160, 255);
	PackedCol disabledColor = PackedCol_Make(160, 160, 160, 255);
	PackedCol color;

	struct ButtonWidget* w = (struct ButtonWidget*)widget;
	struct Texture back;	
	float scale;
		
	back = w->active ? btnSelectedTex : btnShadowTex;
	if (w->flags & WIDGET_FLAG_DISABLED) back = btnDisabledTex;

	back.x = w->x; back.Width  = w->width;
	back.y = w->y; back.Height = w->height;

	/* TODO: Does this 400 need to take DPI into account */
	if (w->width >= 400) {
		/* Button can be drawn normally */
		Gfx_Make2DQuad(&back, w->color, vertices);
		*vertices += 4; /* always use up 8 vertices for body */
	} else {
		/* Split button down the middle */
		scale = (w->width / 400.0f) / (2 * DisplayInfo.ScaleX);

		back.Width = (w->width / 2);
		back.uv.U1 = 0.0f; back.uv.U2 = BUTTON_uWIDTH * scale;
		Gfx_Make2DQuad(&back, w->color, vertices);

		back.x += (w->width / 2);
		back.uv.U1 = BUTTON_uWIDTH * (1.0f - scale); back.uv.U2 = BUTTON_uWIDTH;
		Gfx_Make2DQuad(&back, w->color, vertices);
	}

	color = (w->flags & WIDGET_FLAG_DISABLED) ? disabledColor 
											: (w->active ? activeColor : normColor);
	Gfx_Make2DQuad(&w->tex, color, vertices);
}

static int ButtonWidget_Render2(void* widget, int offset) {
	struct ButtonWidget* w = (struct ButtonWidget*)widget;	
	Gfx_BindTexture(Gui.ClassicTexture ? Gui.GuiClassicTex : Gui.GuiTex);
	/* TODO: Does this 400 need to take DPI into account */
	Gfx_DrawVb_IndexedTris_Range(w->width >= 400 ? 4 : 8, offset);

	if (w->tex.ID) {
		Gfx_BindTexture(w->tex.ID);
		Gfx_DrawVb_IndexedTris_Range(4, offset + 8);
	}
	return offset + 12;
}

static int ButtonWidget_MaxVertices(void* widget) { return BUTTONWIDGET_MAX; }

static const struct WidgetVTABLE ButtonWidget_VTABLE = {
	ButtonWidget_Render, ButtonWidget_Free, ButtonWidget_Reposition,
	Widget_InputDown,    Widget_InputUp,    Widget_MouseScroll,
	Widget_Pointer,      Widget_PointerUp,  Widget_PointerMove,
	ButtonWidget_BuildMesh, ButtonWidget_Render2, ButtonWidget_MaxVertices
};
void ButtonWidget_Make(struct ButtonWidget* w, int minWidth, Widget_LeftClick onClick, cc_uint8 horAnchor, cc_uint8 verAnchor, int xOffset, int yOffset) {
	ButtonWidget_Init(w, minWidth, onClick);
	Widget_SetLocation(w, horAnchor, verAnchor, xOffset, yOffset);
}

void ButtonWidget_Init(struct ButtonWidget* w, int minWidth, Widget_LeftClick onClick) {
	Widget_Reset(w);
	w->VTABLE    = &ButtonWidget_VTABLE;
	w->color     = PACKEDCOL_WHITE;
	w->optName   = NULL;
	w->flags     = WIDGET_FLAG_SELECTABLE;
	w->minWidth  = Display_ScaleX(minWidth);
	w->minHeight = Display_ScaleY(40);
	w->MenuClick = onClick;
}

void ButtonWidget_Set(struct ButtonWidget* w, const cc_string* text, struct FontDesc* font) {
	struct DrawTextArgs args;
	Gfx_DeleteTexture(&w->tex.ID);
	DrawTextArgs_Make(&args, text, font, true);
	Drawer2D_MakeTextTexture(&w->tex, &args);

	/* Give button default height when text is empty */
	if (!w->tex.Height) {
		w->tex.Height = Font_CalcHeight(font, true);
	}
	Widget_Layout(w);
}

void ButtonWidget_SetConst(struct ButtonWidget* w, const char* text, struct FontDesc* font) {
	cc_string str = String_FromReadonly(text);
	ButtonWidget_Set(w, &str, font);
}


/*########################################################################################################################*
*-----------------------------------------------------ScrollbarWidget-----------------------------------------------------*
*#########################################################################################################################*/
#define SCROLL_BACK_COL  PackedCol_Make( 10,  10,  10, 220)
#define SCROLL_BAR_COL   PackedCol_Make(100, 100, 100, 220)
#define SCROLL_HOVER_COL PackedCol_Make(122, 122, 122, 220)

static void ScrollbarWidget_ClampTopRow(struct ScrollbarWidget* w) {
	int maxTop = w->rowsTotal - w->rowsVisible;
	if (w->topRow >= maxTop) w->topRow = maxTop;
	if (w->topRow < 0) w->topRow = 0;
}

static float ScrollbarWidget_GetScale(struct ScrollbarWidget* w) {
	float rows = (float)w->rowsTotal;
	return (w->height - w->borderY * 2) / rows;
}

static void ScrollbarWidget_GetScrollbarCoords(struct ScrollbarWidget* w, int* y, int* height) {
	float scale = ScrollbarWidget_GetScale(w);
	*y = Math_Ceil(w->topRow * scale) + w->borderY;
	*height = Math_Ceil(w->rowsVisible * scale);
	*height = min(*y + *height, w->height - w->borderY) - *y;
}

static void ScrollbarWidget_Render(void* widget, double delta) {
	struct ScrollbarWidget* w = (struct ScrollbarWidget*)widget;
	int x, y, width, height;
	PackedCol barCol;
	cc_bool hovered;

	x = w->x; width = w->width;
	Gfx_Draw2DFlat(x, w->y, width, w->height, SCROLL_BACK_COL);

	ScrollbarWidget_GetScrollbarCoords(w, &y, &height);
	x += w->borderX; y += w->y;
	width -= w->borderX * 2; 

	hovered = Gui_ContainsPointers(x, y, width, height);
	barCol  = hovered ? SCROLL_HOVER_COL : SCROLL_BAR_COL;
	Gfx_Draw2DFlat(x, y, width, height, barCol);

	if (height < 20) return;
	x += w->nubsWidth; y += (height / 2);
	width -= w->nubsWidth * 2;

	Gfx_Draw2DFlat(x, y + w->offsets[0], width, w->borderY, SCROLL_BACK_COL);
	Gfx_Draw2DFlat(x, y + w->offsets[1], width, w->borderY, SCROLL_BACK_COL);
	Gfx_Draw2DFlat(x, y + w->offsets[2], width, w->borderY, SCROLL_BACK_COL);
}

static int ScrollbarWidget_PointerDown(void* widget, int id, int x, int y) {
	struct ScrollbarWidget* w = (struct ScrollbarWidget*)widget;
	int posY, height;

	if (w->draggingId == id) return TOUCH_TYPE_GUI;
	if (x < w->x || x >= w->x + w->width + w->padding) return false;
	/* only intercept pointer that's dragging scrollbar */
	if (w->draggingId) return false;

	y -= w->y;
	ScrollbarWidget_GetScrollbarCoords(w, &posY, &height);

	if (y < posY) {
		w->topRow -= w->rowsVisible;
	} else if (y >= posY + height) {
		w->topRow += w->rowsVisible;
	} else {
		w->draggingId = id;
		w->dragOffset = y - posY;
	}
	ScrollbarWidget_ClampTopRow(w);
	return TOUCH_TYPE_GUI;
}

static void ScrollbarWidget_PointerUp(void* widget, int id, int x, int y) {
	struct ScrollbarWidget* w = (struct ScrollbarWidget*)widget;
	if (w->draggingId != id) return;
	w->draggingId = 0;
	w->dragOffset = 0;
}

static int ScrollbarWidget_MouseScroll(void* widget, float delta) {
	struct ScrollbarWidget* w = (struct ScrollbarWidget*)widget;
	int steps = Utils_AccumulateWheelDelta(&w->scrollingAcc, delta);

	w->topRow -= steps;
	ScrollbarWidget_ClampTopRow(w);
	return true;
}

static int ScrollbarWidget_PointerMove(void* widget, int id, int x, int y) {
	struct ScrollbarWidget* w = (struct ScrollbarWidget*)widget;
	float scale;

	if (w->draggingId == id) {
		y -= w->y;
		scale = ScrollbarWidget_GetScale(w);
		w->topRow = (int)((y - w->dragOffset) / scale);
		ScrollbarWidget_ClampTopRow(w);
		return true;
	}
	return false;
}

static const struct WidgetVTABLE ScrollbarWidget_VTABLE = {
	ScrollbarWidget_Render,      Widget_NullFunc,           Widget_CalcPosition,
	Widget_InputDown,            Widget_InputUp,            ScrollbarWidget_MouseScroll,
	ScrollbarWidget_PointerDown, ScrollbarWidget_PointerUp, ScrollbarWidget_PointerMove
};
void ScrollbarWidget_Create(struct ScrollbarWidget* w, int width) {
	Widget_Reset(w);
	w->VTABLE    = &ScrollbarWidget_VTABLE;
	w->width     = Display_ScaleX(width);
	w->borderX   = Display_ScaleX(2);
	w->borderY   = Display_ScaleY(2);
	w->nubsWidth = Display_ScaleX(3);

	w->offsets[0] = Display_ScaleY(-1 - 4);
	w->offsets[1] = Display_ScaleY(-1);
	w->offsets[2] = Display_ScaleY(-1 + 4);

	w->rowsTotal    = 0;
	w->rowsVisible  = 0;
	w->topRow       = 0;
	w->scrollingAcc = 0.0f;
	w->draggingId   = 0;
	w->dragOffset   = 0;

	/* It's easy to accidentally touch a bit to the right of the */
	/* scrollbar with your finger, so just add some padding */
	if (!Input_TouchMode) return;
	w->padding = Display_ScaleX(15);
}


/*########################################################################################################################*
*------------------------------------------------------HotbarWidget-------------------------------------------------------*
*#########################################################################################################################*/
#define HotbarWidget_TileX(w, idx) (int)(w->x + w->slotXOffset + w->slotWidth * (idx))

static void HotbarWidget_BuildOutlineMesh(struct HotbarWidget* w, struct VertexTextured** vertices) {
	int x;
	Gfx_Make2DQuad(&w->backTex, PACKEDCOL_WHITE, vertices);

	x = HotbarWidget_TileX(w, Inventory.SelectedIndex);
	w->selTex.x = (int)(x - w->selWidth / 2);
	Gfx_Make2DQuad(&w->selTex, PACKEDCOL_WHITE, vertices);
}

static void HotbarWidget_BuildEntriesMesh(struct HotbarWidget* w, struct VertexTextured** vertices) {
	int i, x, y;
	float scale;

	IsometricDrawer_BeginBatch(*vertices, w->state);
	scale = w->elemSize / 2.0f;

	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		x = HotbarWidget_TileX(w, i);
		y = w->y + (w->height / 2);

#ifdef CC_BUILD_TOUCH
		if (i == HOTBAR_MAX_INDEX && Input_TouchMode) continue;
#endif
		IsometricDrawer_AddBatch(Inventory_Get(i), scale, x, y);
	}
	w->verticesCount = IsometricDrawer_EndBatch();
}

static void HotbarWidget_BuildMesh(void* widget, struct VertexTextured** vertices) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	struct VertexTextured* data = *vertices;

	HotbarWidget_BuildOutlineMesh(w, vertices);
	HotbarWidget_BuildEntriesMesh(w, vertices);
	*vertices = data + HOTBAR_MAX_VERTICES;
}


static void HotbarWidget_RenderOutline(struct HotbarWidget* w, int offset) {
	GfxResourceID tex;
	tex = Gui.ClassicTexture ? Gui.GuiClassicTex : Gui.GuiTex;

	Gfx_BindTexture(tex);
	Gfx_DrawVb_IndexedTris_Range(8, offset);
}

static void HotbarWidget_RenderEntries(struct HotbarWidget* w, int offset) {
	if (w->verticesCount == 0) return;
	IsometricDrawer_Render(w->verticesCount, offset, w->state);
}

static int HotbarWidget_Render2(void* widget, int offset) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	HotbarWidget_RenderOutline(w, offset    );
	HotbarWidget_RenderEntries(w, offset + 8);

#ifdef CC_BUILD_TOUCH
	if (!Input_TouchMode) return HOTBAR_MAX_VERTICES;
	w->ellipsisTex.x = HotbarWidget_TileX(w, HOTBAR_MAX_INDEX) - w->ellipsisTex.Width / 2;
	w->ellipsisTex.y = w->y + (w->height / 2) - w->ellipsisTex.Height / 2;
	Texture_Render(&w->ellipsisTex);
#endif
	return HOTBAR_MAX_VERTICES;
}

static int HotbarWidget_MaxVertices(void* w) { return HOTBAR_MAX_VERTICES; }

void HotbarWidget_Update(struct HotbarWidget* w, double delta) {
#ifdef CC_BUILD_TOUCH
	int i;
	if (!Input_TouchMode) return;

	for (i = 0; i < HOTBAR_MAX_INDEX; i++) {
		if(w->touchId[i] != -1) {
			w->touchTime[i] += delta;
			if(w->touchTime[i] > 1) {
				w->touchId[i] = -1;
				w->touchTime[i] = 0;
				Inventory_Set(i, 0);
			}
		}
	}
#endif
}

static int HotbarWidget_ScrolledIndex(struct HotbarWidget* w, float delta, int index, int dir) {
	int steps = Utils_AccumulateWheelDelta(&w->scrollAcc, delta);
	index += (dir * steps) % INVENTORY_BLOCKS_PER_HOTBAR;

	if (index < 0) index += INVENTORY_BLOCKS_PER_HOTBAR;
	if (index >= INVENTORY_BLOCKS_PER_HOTBAR) {
		index -= INVENTORY_BLOCKS_PER_HOTBAR;
	}
	return index;
}

static void HotbarWidget_Reposition(void* widget) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	float scaleX = w->scale * DisplayInfo.ScaleX;
	float scaleY = w->scale * DisplayInfo.ScaleY;
	int y;

	w->width  = (int)(182 * scaleX);
	w->height = Math_Floor(22.0f * scaleY);
	Widget_CalcPosition(w);

	w->selWidth    = (float)Math_Ceil(24.0f * scaleX);
	w->elemSize    = 13.5f * scaleX;
	w->slotXOffset = 11.1f * scaleX;
	w->slotWidth   = 20.0f * scaleX;

	Tex_SetRect(w->backTex, w->x,w->y, w->width,w->height);
	Tex_SetUV(w->backTex,   0,0, 182/256.0f,22/256.0f);

	y = w->y + (w->height - (int)(23.0f * scaleY));
	Tex_SetRect(w->selTex, 0,y, (int)w->selWidth,w->height);
	Tex_SetUV(w->selTex,   0,22/256.0f, 24/256.0f,44/256.0f);
}

static int HotbarWidget_MapKey(int key) {
	int i;
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++)
	{
		if (KeyBind_Claims(KEYBIND_HOTBAR_1 + i, key)) return i;
	}
	return -1;
}

static int HotbarWidget_CycleIndex(int dir) {
	Inventory.SelectedIndex += dir;
	if (Inventory.SelectedIndex < 0) 
		Inventory.SelectedIndex += INVENTORY_BLOCKS_PER_HOTBAR;
	if (Inventory.SelectedIndex >= INVENTORY_BLOCKS_PER_HOTBAR)
		Inventory.SelectedIndex -= INVENTORY_BLOCKS_PER_HOTBAR;

	return true;
}

static int HotbarWidget_KeyDown(void* widget, int key) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	int index = HotbarWidget_MapKey(key);

	if (index == -1) {
		if (KeyBind_Claims(KEYBIND_HOTBAR_LEFT, key))
			return HotbarWidget_CycleIndex(-1);
		if (KeyBind_Claims(KEYBIND_HOTBAR_RIGHT, key))
			return HotbarWidget_CycleIndex(+1);
		return false;
	}

	if (KeyBind_IsPressed(KEYBIND_HOTBAR_SWITCH)) {
		/* Pick from first to ninth row */
		Inventory_SetHotbarIndex(index);
		w->altHandled = true;
	} else {
		Inventory_SetSelectedIndex(index);
	}
	return true;
}

static void HotbarWidget_InputUp(void* widget, int key) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	/* Need to handle these cases:
	     a) user presses alt then number
	     b) user presses alt
	   We only do case b) if case a) did not happen */
	if (!KeyBind_Claims(KEYBIND_HOTBAR_SWITCH, key)) return;
	if (w->altHandled) { w->altHandled = false; return; } /* handled already */

	/* Don't switch hotbar when alt+tabbing to another window */
	if (WindowInfo.Focused) Inventory_SwitchHotbar();
}

static int HotbarWidget_PointerDown(void* widget, int id, int x, int y) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	int width, height;
	int i, cellX, cellY;

	if (!Widget_Contains(w, x, y)) return false;
	width  = (int)w->slotWidth;
	height = w->height;

	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		cellX = (int)(w->x + width * i);
		cellY = w->y;
		if (!Gui_Contains(cellX, cellY, width, height, x, y)) continue;

#ifdef CC_BUILD_TOUCH
		if(Input_TouchMode) {
			if (i == HOTBAR_MAX_INDEX) {
				InventoryScreen_Show(); return TOUCH_TYPE_GUI;
			} else {
				w->touchId[i] = id;
				w->touchTime[i] = 0;
			}
		}
#endif
		Inventory_SetSelectedIndex(i);
		return TOUCH_TYPE_GUI;
	}
	return false;
}

static void HotbarWidget_PointerUp(void* widget, int id, int x, int y) {
#ifdef CC_BUILD_TOUCH
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	int i;

	for (i = 0; i < HOTBAR_MAX_INDEX; i++) {
		if (w->touchId[i] == id) {
			w->touchId[i] = -1;
			w->touchTime[i] = 0;
		}
	}
#endif
}

static int HotbarWidget_PointerMove(void* widget, int id, int x, int y) {
#ifdef CC_BUILD_TOUCH
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	int i;

	for (i = 0; i < HOTBAR_MAX_INDEX; i++) {
		if (w->touchId[i] == id) {
			if (!Widget_Contains(w, x, y)) {
				w->touchId[i] = -1;
				w->touchTime[i] = 0;
				return true;
			}
		}
	}
#endif
	return false;
}

static int HotbarWidget_MouseScroll(void* widget, float delta) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	int index;

	if (KeyBind_IsPressed(KEYBIND_HOTBAR_SWITCH)) {
		index = Inventory.Offset / INVENTORY_BLOCKS_PER_HOTBAR;
		index = HotbarWidget_ScrolledIndex(w, delta, index, 1);
		Inventory_SetHotbarIndex(index);
		w->altHandled = true;
	} else {
		index = HotbarWidget_ScrolledIndex(w, delta, Inventory.SelectedIndex, -1);
		Inventory_SetSelectedIndex(index);
	}
	return true;
}

static void HotbarWidget_Free(void* widget) {
#ifdef CC_BUILD_TOUCH
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	if (!Input_TouchMode) return;

	Gfx_DeleteTexture(&w->ellipsisTex.ID);
#endif
}

static const struct WidgetVTABLE HotbarWidget_VTABLE = {
	NULL,                     HotbarWidget_Free,      HotbarWidget_Reposition,
	HotbarWidget_KeyDown,     HotbarWidget_InputUp,   HotbarWidget_MouseScroll,
	HotbarWidget_PointerDown, HotbarWidget_PointerUp, HotbarWidget_PointerMove,
	HotbarWidget_BuildMesh,   HotbarWidget_Render2,   HotbarWidget_MaxVertices
};
void HotbarWidget_Create(struct HotbarWidget* w) {
	Widget_Reset(w);
	w->VTABLE    = &HotbarWidget_VTABLE;
	w->horAnchor = ANCHOR_CENTRE;
	w->verAnchor = ANCHOR_MAX;
	w->scale     = 1;
	w->verticesCount = 0;

#ifdef CC_BUILD_TOUCH
	int i;
	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR - 1; i++) {
		w->touchId[i] = -1;
	}
#endif
}

void HotbarWidget_SetFont(struct HotbarWidget* w, struct FontDesc* font) {
#ifdef CC_BUILD_TOUCH
	static const cc_string dots = String_FromConst("...");
	struct DrawTextArgs args;
	if (!Input_TouchMode) return;

	DrawTextArgs_Make(&args, &dots, font, true);
	Drawer2D_MakeTextTexture(&w->ellipsisTex, &args);
#endif
}


/*########################################################################################################################*
*-------------------------------------------------------TableWidget-------------------------------------------------------*
*#########################################################################################################################*/
static int Table_X(struct TableWidget* w)      { return w->x - w->paddingL; }
static int Table_Y(struct TableWidget* w)      { return w->y - w->paddingT; }
static int Table_Width(struct TableWidget* w)  { return w->width  + w->paddingL + w->paddingR; }
static int Table_Height(struct TableWidget* w) { return w->height + w->paddingT + w->paddingB; }

static cc_bool TableWidget_GetCoords(struct TableWidget* w, int i, int* cellX, int* cellY) {
	int x, y;
	x = i % w->blocksPerRow;
	y = i / w->blocksPerRow - w->scroll.topRow;

	*cellX = w->x + w->cellSizeX * x;
	*cellY = w->y + w->cellSizeY * y + 3;
	return y >= 0 && y < w->rowsVisible;
}

static void TableWidget_MoveCursorToSelected(struct TableWidget* w) {
	int x, y, idx;
	if (w->selectedIndex == -1) return;

	idx = w->selectedIndex;
	TableWidget_GetCoords(w, idx, &x, &y);

	x += w->cellSizeX / 2; y += w->cellSizeY / 2;
	Cursor_SetPosition(x, y);
}

static void TableWidget_RecreateTitle(struct TableWidget* w) {
	BlockID block;
	if (w->selectedIndex == w->lastCreatedIndex) return;
	if (w->blocksCount == 0) return;
	w->lastCreatedIndex = w->selectedIndex;

	block = w->selectedIndex == -1 ? BLOCK_AIR : w->blocks[w->selectedIndex];
	w->UpdateTitle(block);
}

void TableWidget_RecreateBlocks(struct TableWidget* w) {
	int max = Game_UseCPEBlocks ? BLOCK_MAX_DEFINED : BLOCK_MAX_ORIGINAL;
	int i, begCount, rowEnd;
	cc_bool emptyRow;
	BlockID block;
	w->blocksCount = 0;

	for (i = 0; i < Array_Elems(Inventory.Map);) {
		emptyRow = true;
		begCount = w->blocksCount;
		rowEnd   = min(i + w->blocksPerRow, Array_Elems(Inventory.Map));

		for (; i < rowEnd; i++) {
			block = Inventory.Map[i];
			if (block > max) continue;
			
			w->blocks[w->blocksCount++] = block;
			if (block != BLOCK_AIR) emptyRow = false;
		}

		if (emptyRow) w->blocksCount = begCount;
	}

	w->rowsTotal = Math_CeilDiv(w->blocksCount, w->blocksPerRow);
	Widget_Layout(w);
}

static void TableWidget_BuildMesh(void* widget, struct VertexTextured** vertices) {
	struct TableWidget* w = (struct TableWidget*)widget;
	struct VertexTextured* data = *vertices;
	int cellSizeX, cellSizeY;
	int i, x, y;

	cellSizeX = w->cellSizeX;
	cellSizeY = w->cellSizeY;

	IsometricDrawer_BeginBatch(data, w->state);
	for (i = 0; i < w->blocksCount; i++) {
		if (!TableWidget_GetCoords(w, i, &x, &y)) continue;

		/* We want to always draw the selected block on top of others */
		/* TODO: Need two size arguments, in case X/Y dpi differs */
		if (i == w->selectedIndex) continue;
		IsometricDrawer_AddBatch(w->blocks[i],
			w->normBlockSize, x + cellSizeX / 2, y + cellSizeY / 2);
	}

	i = w->selectedIndex;
	if (i != -1) {
		TableWidget_GetCoords(w, i, &x, &y);

		IsometricDrawer_AddBatch(w->blocks[i],
			w->selBlockSize, x + cellSizeX / 2, y + cellSizeY / 2);
	}

	w->verticesCount = IsometricDrawer_EndBatch();
	*vertices        = data + TABLE_MAX_VERTICES;
}

static int TableWidget_Render2(void* widget, int offset) {
	struct TableWidget* w = (struct TableWidget*)widget;
	int cellSizeX, cellSizeY, size;
	float off;
	int x, y;

	/* These were sourced by taking a screenshot of vanilla */
	/* Then using paint to extract the color components */
	/* Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topBackColor    = PackedCol_Make( 34,  34,  34, 168);
	PackedCol bottomBackColor = PackedCol_Make( 57,  57, 104, 202);
	PackedCol topSelColor     = PackedCol_Make(255, 255, 255, 142);
	PackedCol bottomSelColor  = PackedCol_Make(255, 255, 255, 192);

	Gfx_Draw2DGradient(Table_X(w), Table_Y(w),
		Table_Width(w), Table_Height(w), topBackColor, bottomBackColor);

	if (w->rowsVisible < w->rowsTotal) {
		Elem_Render(&w->scroll, 0);
	}

	cellSizeX = w->cellSizeX;
	cellSizeY = w->cellSizeY;
	if (w->selectedIndex != -1 && Gui.ClassicInventory && w->blocks[w->selectedIndex] != BLOCK_AIR) {
		TableWidget_GetCoords(w, w->selectedIndex, &x, &y);

		/* TODO: Need two size arguments, in case X/Y dpi differs */
		off  = cellSizeX * 0.1f;
		size = (int)(cellSizeX + off * 2);
		Gfx_Draw2DGradient((int)(x - off), (int)(y - off),
			size, size, topSelColor, bottomSelColor);
	}

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindDynamicVb(w->vb);

	if (w->verticesCount) {
		IsometricDrawer_Render(w->verticesCount, offset, w->state);
	}
	return offset + TABLE_MAX_VERTICES;
}

static int TableWidget_MaxVertices(void* w) { return TABLE_MAX_VERTICES; }

static void TableWidget_Free(void* widget) { }

void TableWidget_Recreate(struct TableWidget* w) {
	w->lastCreatedIndex = -1000;
	TableWidget_RecreateTitle(w);
}

static void TableWidget_Reposition(void* widget) {
	struct TableWidget* w = (struct TableWidget*)widget;
	cc_bool classic = Gui.ClassicInventory;
	float scale = Math_SqrtF(w->scale);
	int cellSize, blockSize;

	cellSize     = classic ? 48 : 50;
	w->cellSizeX = Display_ScaleX(cellSize * scale);
	w->cellSizeY = Display_ScaleY(cellSize * scale);

	blockSize    = classic ? 40 : 50;
	blockSize    = Display_ScaleX(blockSize * scale);
	w->normBlockSize = (blockSize             ) * 0.7f / 2.0f;
	w->selBlockSize  = (blockSize + 25 * scale) * 0.7f / 2.0f;
	w->rowsVisible   = min(8, w->rowsTotal); /* 8 rows max */

	do {
		w->width  = w->cellSizeX * w->blocksPerRow;
		w->height = w->cellSizeY * w->rowsVisible;
		Widget_CalcPosition(w);

		/* Does the table fit on screen? */
		if (classic || Table_Y(w) >= 0) break;
		w->rowsVisible--;
	} while (w->rowsVisible > 1);

	w->scroll.x = Table_X(w) + Table_Width(w);
	w->scroll.y = Table_Y(w);
	w->scroll.height      = Table_Height(w);
	w->scroll.rowsTotal   = w->rowsTotal;
	w->scroll.rowsVisible = w->rowsVisible;
}

static void TableWidget_ScrollRelative(struct TableWidget* w, int delta) {
	int start = w->selectedIndex, index = start;
	index += delta;
	if (index < 0) index -= delta;
	if (index >= w->blocksCount) index -= delta;
	w->selectedIndex = index;

	/* adjust scrollbar by number of rows moved up/down */
	w->scroll.topRow += (index / w->blocksPerRow) - (start / w->blocksPerRow);
	ScrollbarWidget_ClampTopRow(&w->scroll);

	TableWidget_RecreateTitle(w);
	TableWidget_MoveCursorToSelected(w);
}

static int TableWidget_PointerDown(void* widget, int id, int x, int y) {
	struct TableWidget* w = (struct TableWidget*)widget;
	w->pendingClose = false;

	if (Elem_HandlesPointerDown(&w->scroll, id, x, y)) {
		return TOUCH_TYPE_GUI;
	} else if (w->selectedIndex != -1 && w->blocks[w->selectedIndex] != BLOCK_AIR) {
		Inventory_SetSelectedBlock(w->blocks[w->selectedIndex]);
		w->pendingClose = true;
		return TOUCH_TYPE_GUI;
	} else if (Gui_Contains(Table_X(w), Table_Y(w), Table_Width(w), Table_Height(w), x, y)) {
		return TOUCH_TYPE_GUI;
	}
	return false;
}

static void TableWidget_PointerUp(void* widget, int id, int x, int y) {
	struct TableWidget* w = (struct TableWidget*)widget;
	Elem_OnPointerUp(&w->scroll, id, x, y);
}

static int TableWidget_MouseScroll(void* widget, float delta) {
	struct TableWidget* w = (struct TableWidget*)widget;
	int origTopRow, index;

	cc_bool bounds = Gui_ContainsPointers(Table_X(w), Table_Y(w),
		Table_Width(w) + w->scroll.width, Table_Height(w));
	if (!bounds) return false;

	origTopRow = w->scroll.topRow;
	Elem_HandlesMouseScroll(&w->scroll, delta);
	if (w->selectedIndex == -1) return true;

	index = w->selectedIndex;
	index += (w->scroll.topRow - origTopRow) * w->blocksPerRow;
	if (index >= w->blocksCount) index = -1;

	w->selectedIndex = index;
	TableWidget_RecreateTitle(w);
	return true;
}

static int TableWidget_PointerMove(void* widget, int id, int x, int y) {
	struct TableWidget* w = (struct TableWidget*)widget;
	int cellSizeX, cellSizeY, maxHeight;
	int i, cellX, cellY;

	if (Elem_HandlesPointerMove(&w->scroll, id, x, y)) return true;
	if (w->lastX == x && w->lastY == y) return true;
	w->lastX = x; w->lastY = y;

	w->selectedIndex = -1;
	cellSizeX = w->cellSizeX;
	cellSizeY = w->cellSizeY;
	maxHeight = cellSizeY * w->rowsVisible;

	if (Gui_Contains(w->x, w->y + 3, w->width, maxHeight - 3 * 2, x, y)) {
		for (i = 0; i < w->blocksCount; i++) {
			TableWidget_GetCoords(w, i, &cellX, &cellY);

			if (Gui_Contains(cellX, cellY, cellSizeX, cellSizeY, x, y)) {
				w->selectedIndex = i;
				break;
			}
		}
	}
	TableWidget_RecreateTitle(w);
	return true;
}

static int TableWidget_KeyDown(void* widget, int key) {
	struct TableWidget* w = (struct TableWidget*)widget;
	if (w->selectedIndex == -1) return false;

	if (Input_IsLeftButton(key)         || key == CCKEY_KP4) {
		TableWidget_ScrollRelative(w, -1);
	} else if (Input_IsRightButton(key) || key == CCKEY_KP6) {
		TableWidget_ScrollRelative(w, 1);
	} else if (Input_IsUpButton(key)    || key == CCKEY_KP8) {
		TableWidget_ScrollRelative(w, -w->blocksPerRow);
	} else if (Input_IsDownButton(key)  || key == CCKEY_KP2) {
		TableWidget_ScrollRelative(w, w->blocksPerRow);
	} else {
		return false;
	}
	return true;
}

static const struct WidgetVTABLE TableWidget_VTABLE = {
	NULL,                    TableWidget_Free,      TableWidget_Reposition,
	TableWidget_KeyDown,     Widget_InputUp,        TableWidget_MouseScroll,
	TableWidget_PointerDown, TableWidget_PointerUp, TableWidget_PointerMove,
	TableWidget_BuildMesh,   TableWidget_Render2,   TableWidget_MaxVertices
};
void TableWidget_Create(struct TableWidget* w, int sbWidth) {
	cc_bool classic;
	Widget_Reset(w);
	w->VTABLE = &TableWidget_VTABLE;
	w->lastCreatedIndex = -1000;
	ScrollbarWidget_Create(&w->scroll, sbWidth);
	
	w->horAnchor = ANCHOR_CENTRE;
	w->verAnchor = ANCHOR_CENTRE;
	w->lastX = -20; w->lastY = -20;
	w->scale = 1;

	classic     = Gui.ClassicInventory;
	w->paddingL = Display_ScaleX(classic ? 20 : 15);
	w->paddingR = Display_ScaleX(classic ? 28 : 15);
	w->paddingT = Display_ScaleY(classic ? 46 : 35);
	w->paddingB = Display_ScaleY(classic ? 14 : 15);
}

void TableWidget_SetBlockTo(struct TableWidget* w, BlockID block) {
	int i;
	w->selectedIndex = -1;
	
	for (i = 0; i < w->blocksCount; i++) {
		if (w->blocks[i] == block) w->selectedIndex = i;
	}
	/* When holding air, inventory should open at middle */
	if (block == BLOCK_AIR) w->selectedIndex = -1;

	w->scroll.topRow = w->selectedIndex / w->blocksPerRow;
	w->scroll.topRow -= (w->rowsVisible - 1);
	ScrollbarWidget_ClampTopRow(&w->scroll);
	TableWidget_MoveCursorToSelected(w);
	TableWidget_RecreateTitle(w);
}

void TableWidget_OnInventoryChanged(struct TableWidget* w) {
	TableWidget_RecreateBlocks(w);
	if (w->selectedIndex >= w->blocksCount) {
		w->selectedIndex = w->blocksCount - 1;
	}
	w->lastX = -1; w->lastY = -1;

	w->scroll.topRow = w->selectedIndex / w->blocksPerRow;
	ScrollbarWidget_ClampTopRow(&w->scroll);
	TableWidget_RecreateTitle(w);
}


/*########################################################################################################################*
*-------------------------------------------------------InputWidget-------------------------------------------------------*
*#########################################################################################################################*/
static void InputWidget_Reset(struct InputWidget* w) {
	Widget_Reset(w);
	w->caretPos      = -1;
	w->caretOffset   = Display_ScaleY(2);
	w->OnTextChanged = NULL;
}

static void InputWidget_FormatLine(struct InputWidget* w, int i, cc_string* line) {
	cc_string src = w->lines[i];
	if (!w->convertPercents) { String_AppendString(line, &src); return; }

	for (i = 0; i < src.length; i++) {
		char c = src.buffer[i];
		if (c == '%' && Drawer2D_ValidColorCodeAt(&src, i + 1)) { c = '&'; }
		String_Append(line, c);
	}
}

static void InputWidget_CalculateLineSizes(struct InputWidget* w) {
	cc_string line; char lineBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	int y;

	for (y = 0; y < INPUTWIDGET_MAX_LINES; y++) {
		w->lineWidths[y] = 0;
	}
	w->lineWidths[0] = w->prefixWidth;
	DrawTextArgs_MakeEmpty(&args, w->font, true);

	String_InitArray(line, lineBuffer);
	for (y = 0; y < w->GetMaxLines(); y++) {
		line.length = 0;
		InputWidget_FormatLine(w, y, &line);

		args.text = line;
		w->lineWidths[y] += Drawer2D_TextWidth(&args);
	}
}

static char InputWidget_GetLastCol(struct InputWidget* w, int x, int y) {
	cc_string line; char lineBuffer[STRING_SIZE];
	char col;
	String_InitArray(line, lineBuffer);

	for (; y >= 0; y--) {
		line.length = 0;
		InputWidget_FormatLine(w, y, &line);

		col = Drawer2D_LastColor(&line, x);
		if (col) return col;
		if (y > 0) { x = w->lines[y - 1].length; }
	}
	return '\0';
}

static void InputWidget_UpdateCaret(struct InputWidget* w) {
	static const cc_string caret = String_FromConst("_");
	BitmapCol col;
	cc_string line; char lineBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	int maxChars, lineWidth;
	char colCode;

	if (!w->caretTex.ID) {
		DrawTextArgs_Make(&args, &caret, w->font, true);
		Drawer2D_MakeTextTexture(&w->caretTex, &args);
		w->caretWidth = (cc_uint16)((w->caretTex.Width * 3) / 4);
	}
	
	maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (w->caretPos >= maxChars) w->caretPos = -1;
	WordWrap_GetCoords(w->caretPos, w->lines, w->GetMaxLines(), &w->caretX, &w->caretY);

	DrawTextArgs_MakeEmpty(&args, w->font, false);
	w->caretAccumulator = 0;
	w->caretTex.Width   = w->caretWidth;

	/* Caret is at last character on line */
	if (w->caretX == INPUTWIDGET_LEN) {
		lineWidth = w->lineWidths[w->caretY];	
	} else {
		String_InitArray(line, lineBuffer);
		InputWidget_FormatLine(w, w->caretY, &line);

		args.text = String_UNSAFE_Substring(&line, 0, w->caretX);
		lineWidth = Drawer2D_TextWidth(&args);
		if (w->caretY == 0) lineWidth += w->prefixWidth;

		if (w->caretX < line.length) {
			args.text = String_UNSAFE_Substring(&line, w->caretX, 1);
			args.useShadow = true;
			w->caretTex.Width = Drawer2D_TextWidth(&args);
		}
	}

	w->caretTex.x = w->x + w->padding + lineWidth;
	w->caretTex.y = (w->inputTex.y + w->caretOffset) + w->caretY * w->lineHeight;
	colCode = InputWidget_GetLastCol(w, w->caretX, w->caretY);

	if (colCode) {
		col = Drawer2D_GetColor(colCode);
		/* Component order might be different to BitmapCol */
		w->caretCol = PackedCol_Make(BitmapCol_R(col), BitmapCol_G(col), 
									 BitmapCol_B(col), BitmapCol_A(col));
	} else {
		w->caretCol = PackedCol_Scale(PACKEDCOL_WHITE, 0.8f);
	}
}

static void InputWidget_RenderCaret(struct InputWidget* w, double delta) {
	float second;
	if (!w->showCaret) return;
	w->caretAccumulator += delta;

	second = Math_Mod1((float)w->caretAccumulator);
	if (second < 0.5f) Texture_RenderShaded(&w->caretTex, w->caretCol);
}

static void InputWidget_OnPressedEnter(void* widget) {
	struct InputWidget* w = (struct InputWidget*)widget;
	InputWidget_Clear(w);
	w->height = w->lineHeight;
	/* TODO get rid of this awful hack.. */
	Widget_Layout(w);
}

void InputWidget_Clear(struct InputWidget* w) {
	int i;
	w->text.length = 0;
	
	for (i = 0; i < Array_Elems(w->lines); i++) {
		w->lines[i] = String_Empty;
	}

	w->caretPos = -1;
	Gfx_DeleteTexture(&w->inputTex.ID);
	/* TODO: Maybe call w->OnTextChanged */
}

static cc_bool InputWidget_AllowedChar(void* widget, char c) {
	return Server.SupportsFullCP437 || (Convert_CP437ToUnicode(c) == c);
}

static void InputWidget_AppendChar(struct InputWidget* w, char c) {
	if (w->caretPos == -1) {
		String_InsertAt(&w->text, w->text.length, c);
	} else {
		String_InsertAt(&w->text, w->caretPos, c);
		w->caretPos++;
		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
	}
}

static cc_bool InputWidget_TryAppendChar(struct InputWidget* w, char c) {
	int maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (w->text.length >= maxChars) return false;
	if (!w->AllowedChar(w, c)) return false;

	InputWidget_AppendChar(w, c);
	return true;
}

static int InputWidget_DoAppendText(struct InputWidget* w, const cc_string* text) {
	int i, appended = 0;
	for (i = 0; i < text->length; i++) {
		if (InputWidget_TryAppendChar(w, text->buffer[i])) appended++;
	}
	return appended;
}

void InputWidget_AppendText(struct InputWidget* w, const cc_string* text) {
	int appended = InputWidget_DoAppendText(w, text);
	if (appended) InputWidget_UpdateText(w);
}

void InputWidget_Append(struct InputWidget* w, char c) {
	if (!InputWidget_TryAppendChar(w, c)) return;
	InputWidget_UpdateText(w);
}

static void InputWidget_DeleteChar(struct InputWidget* w) {
	if (!w->text.length) return;

	if (w->caretPos == -1) {
		String_DeleteAt(&w->text, w->text.length - 1);
	} else if (w->caretPos > 0) {
		w->caretPos--;
		String_DeleteAt(&w->text, w->caretPos);
	}
}

static void InputWidget_BackspaceKey(struct InputWidget* w) {
	int i, len;

	if (Input_IsActionPressed()) {
		if (w->caretPos == -1) { w->caretPos = w->text.length - 1; }
		len = WordWrap_GetBackLength(&w->text, w->caretPos);
		if (!len) return;

		w->caretPos -= len;
		if (w->caretPos < 0) { w->caretPos = 0; }

		for (i = 0; i <= len; i++) {
			String_DeleteAt(&w->text, w->caretPos);
		}

		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
		if (w->caretPos == -1 && w->text.length > 0) {
			String_InsertAt(&w->text, w->text.length, ' ');
		} else if (w->caretPos >= 0 && w->text.buffer[w->caretPos] != ' ') {
			String_InsertAt(&w->text, w->caretPos, ' ');
		}
		InputWidget_UpdateText(w);
	} else if (w->text.length > 0 && w->caretPos != 0) {
		InputWidget_DeleteChar(w);
		InputWidget_UpdateText(w);
	}
}

static void InputWidget_DeleteKey(struct InputWidget* w) {
	if (w->text.length > 0 && w->caretPos != -1) {
		String_DeleteAt(&w->text, w->caretPos);
		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
		InputWidget_UpdateText(w);
	}
}

static void InputWidget_LeftKey(struct InputWidget* w) {
	if (Input_IsActionPressed()) {
		if (w->caretPos == -1) { w->caretPos = w->text.length - 1; }
		w->caretPos -= WordWrap_GetBackLength(&w->text, w->caretPos);
		InputWidget_UpdateCaret(w);
		return;
	}

	if (w->text.length > 0) {
		if (w->caretPos == -1) { w->caretPos = w->text.length; }
		w->caretPos--;
		if (w->caretPos < 0) { w->caretPos = 0; }
		InputWidget_UpdateCaret(w);
	}
}

static void InputWidget_RightKey(struct InputWidget* w) {
	if (Input_IsActionPressed()) {
		w->caretPos += WordWrap_GetForwardLength(&w->text, w->caretPos);
		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
		InputWidget_UpdateCaret(w);
		return;
	}

	if (w->text.length > 0 && w->caretPos != -1) {
		w->caretPos++;
		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
		InputWidget_UpdateCaret(w);
	}
}

static void InputWidget_HomeKey(struct InputWidget* w) {
	if (!w->text.length) return;
	w->caretPos = 0;
	InputWidget_UpdateCaret(w);
}

static void InputWidget_EndKey(struct InputWidget* w) {
	w->caretPos = -1;
	InputWidget_UpdateCaret(w);
}

static void InputWidget_CopyFromClipboard(struct InputWidget* w) {
	cc_string text; char textBuffer[2048];
	String_InitArray(text, textBuffer);

	Clipboard_GetText(&text);
	InputWidget_AppendText(w, &text);
}

static cc_bool InputWidget_OtherKey(struct InputWidget* w, int key) {
	int maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (key == INPUT_CLIPBOARD_PASTE && w->text.length < maxChars) {
		InputWidget_CopyFromClipboard(w);
		return true;
	} else if (key == INPUT_CLIPBOARD_COPY) {
		if (!w->text.length) return true;
		Clipboard_SetText(&w->text);
		return true;
	}
	return false;
}

void InputWidget_UpdateText(struct InputWidget* w) {
	int lines = w->GetMaxLines();
	if (lines > 1) {
		WordWrap_Do(&w->text, w->lines, lines, INPUTWIDGET_LEN);
	} else {
		w->lines[0] = w->text;
	}

	Gfx_DeleteTexture(&w->inputTex.ID);
	InputWidget_CalculateLineSizes(w);
	w->RemakeTexture(w);
	InputWidget_UpdateCaret(w);
	Window_SetKeyboardText(&w->text);
	if (w->OnTextChanged) w->OnTextChanged(w);
}

void InputWidget_SetText(struct InputWidget* w, const cc_string* str) {
	InputWidget_Clear(w);
	InputWidget_DoAppendText(w, str);
	InputWidget_UpdateText(w);
}

static void InputWidget_Free(void* widget) {
	struct InputWidget* w = (struct InputWidget*)widget;
	Gfx_DeleteTexture(&w->inputTex.ID);
	Gfx_DeleteTexture(&w->caretTex.ID);
}

static void InputWidget_Reposition(void* widget) {
	struct InputWidget* w = (struct InputWidget*)widget;
	int oldX = w->x, oldY = w->y;
	Widget_CalcPosition(w);
	
	w->caretTex.x += w->x - oldX; w->caretTex.y += w->y - oldY;
	w->inputTex.x += w->x - oldX; w->inputTex.y += w->y - oldY;
}

static int InputWidget_KeyDown(void* widget, int key) {
	struct InputWidget* w = (struct InputWidget*)widget;
	if (Input_IsLeftButton(key)) {
		InputWidget_LeftKey(w);
	} else if (Input_IsRightButton(key)) {
		InputWidget_RightKey(w);
	} else if (key == CCKEY_BACKSPACE) {
		InputWidget_BackspaceKey(w);
	} else if (key == CCKEY_DELETE) {
		InputWidget_DeleteKey(w);
	} else if (key == CCKEY_HOME) {
		InputWidget_HomeKey(w);
	} else if (key == CCKEY_END) {
		InputWidget_EndKey(w);
	} else if (!InputWidget_OtherKey(w, key)) {
		return false;
	}
	return true;
}

static int InputWidget_PointerDown(void* widget, int id, int x, int y) {
	cc_string line; char lineBuffer[STRING_SIZE];
	struct InputWidget* w = (struct InputWidget*)widget;
	struct DrawTextArgs args;
	int cx, cy, offset = 0;
	int charX, charWidth, charHeight;

	x -= w->inputTex.x; y -= w->inputTex.y;
	DrawTextArgs_MakeEmpty(&args, w->font, true);
	charHeight = w->lineHeight;
	String_InitArray(line, lineBuffer);

	for (cy = 0; cy < w->GetMaxLines(); cy++) {
		line.length = 0;
		InputWidget_FormatLine(w, cy, &line);
		if (!line.length) continue;

		for (cx = 0; cx < line.length; cx++) {
			args.text = String_UNSAFE_Substring(&line, 0, cx);
			charX     = Drawer2D_TextWidth(&args);
			if (cy == 0) charX += w->prefixWidth;

			args.text = String_UNSAFE_Substring(&line, cx, 1);
			charWidth = Drawer2D_TextWidth(&args);

			if (Gui_Contains(charX, cy * charHeight, charWidth, charHeight, x, y)) {
				w->caretPos = offset + cx;
				InputWidget_UpdateCaret(w);
				return TOUCH_TYPE_GUI;
			}
		}
		offset += line.length;
	}

	w->caretPos = -1;
	InputWidget_UpdateCaret(w);
	return TOUCH_TYPE_GUI;
}


/*########################################################################################################################*
*-----------------------------------------------------MenuInputDesc-------------------------------------------------------*
*#########################################################################################################################*/
static void    MenuInput_NoDefault(struct MenuInputDesc* d, cc_string* value) { }
static cc_bool MenuInput_NoProcess(struct MenuInputDesc* d, cc_string* value, int btn) { return false; }

static void Hex_Range(struct MenuInputDesc* d, cc_string* range) {
	String_AppendConst(range, "&7(#000000 - #FFFFFF)");
}

static cc_bool Hex_ValidChar(struct MenuInputDesc* d, char c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static cc_bool Hex_ValidString(struct MenuInputDesc* d, const cc_string* s) {
	return s->length <= 6;
}

static cc_bool Hex_ValidValue(struct MenuInputDesc* d, const cc_string* s) {
	cc_uint8 rgb[3];
	return PackedCol_TryParseHex(s, rgb);
}

static void Hex_Default(struct MenuInputDesc* d, cc_string* value) {
	PackedCol_ToHex(value, d->meta.h.Default);
}

const struct MenuInputVTABLE HexInput_VTABLE = {
	Hex_Range, Hex_ValidChar, Hex_ValidString, Hex_ValidValue, 
	Hex_Default, MenuInput_NoProcess
};

static void Int_Range(struct MenuInputDesc* d, cc_string* range) {
	String_Format2(range, "&7(%i - %i)", &d->meta.i.Min, &d->meta.i.Max);
}

static cc_bool Int_ValidChar(struct MenuInputDesc* d, char c) {
	return (c >= '0' && c <= '9') || c == '-';
}

static cc_bool Int_ValidString(struct MenuInputDesc* d, const cc_string* s) {
	int value;
	if (s->length == 1 && s->buffer[0] == '-') return true; /* input is just a minus sign */
	return Convert_ParseInt(s, &value);
}

static cc_bool Int_ValidValue(struct MenuInputDesc* d, const cc_string* s) {
	int value, min = d->meta.i.Min, max = d->meta.i.Max;
	return Convert_ParseInt(s, &value) && min <= value && value <= max;
}

static void Int_Default(struct MenuInputDesc* d, cc_string* value) {
	String_AppendInt(value, d->meta.i.Default);
}

const struct MenuInputVTABLE IntInput_VTABLE = {
	Int_Range, Int_ValidChar, Int_ValidString, Int_ValidValue, 
	Int_Default, MenuInput_NoProcess
};

static void Seed_Range(struct MenuInputDesc* d, cc_string* range) {
	String_AppendConst(range, "&7(an integer)");
}

const struct MenuInputVTABLE SeedInput_VTABLE = {
	Seed_Range, Int_ValidChar, Int_ValidString, Int_ValidValue,
	MenuInput_NoDefault, MenuInput_NoProcess
};

static void Float_Range(struct MenuInputDesc* d, cc_string* range) {
	String_Format2(range, "&7(%f2 - %f2)", &d->meta.f.Min, &d->meta.f.Max);
}

static cc_bool Float_ValidChar(struct MenuInputDesc* d, char c) {
	return (c >= '0' && c <= '9') || c == '-' || c == '.' || c == ',';
}

static cc_bool Float_ValidString(struct MenuInputDesc* d, const cc_string* s) {
	float value;
	if (s->length == 1 && Float_ValidChar(d, s->buffer[0])) return true;
	return Convert_ParseFloat(s, &value);
}

static cc_bool Float_ValidValue(struct MenuInputDesc* d, const cc_string* s) {
	float value, min = d->meta.f.Min, max = d->meta.f.Max;
	return Convert_ParseFloat(s, &value) && min <= value && value <= max;
}

static void Float_Default(struct MenuInputDesc* d, cc_string* value) {
	String_AppendFloat(value, d->meta.f.Default, 3);
}

const struct MenuInputVTABLE FloatInput_VTABLE = {
	Float_Range, Float_ValidChar, Float_ValidString, Float_ValidValue, 
	Float_Default, MenuInput_NoProcess
};

static void Path_Range(struct MenuInputDesc* d, cc_string* range) {
	String_AppendConst(range, "&7(Enter name)");
}

static cc_bool Path_ValidChar(struct MenuInputDesc* d, char c) {
	return !(c == '/' || c == '\\' || c == '?' || c == '*' || c == ':'
		|| c == '<' || c == '>' || c == '|' || c == '"' || c == '.');
}
static cc_bool Path_ValidString(struct MenuInputDesc* d, const cc_string* s) { return true; }

const struct MenuInputVTABLE PathInput_VTABLE = {
	Path_Range, Path_ValidChar, Path_ValidString, Path_ValidString, 
	MenuInput_NoDefault, MenuInput_NoProcess
};

static void String_Range(struct MenuInputDesc* d, cc_string* range) {
	String_AppendConst(range, "&7(Enter text)");
}

static cc_bool String_ValidChar(struct MenuInputDesc* d, char c) {
	return c != '&';
}

static cc_bool String_ValidString(struct MenuInputDesc* d, const cc_string* s) {
	return s->length <= STRING_SIZE;
}

const struct MenuInputVTABLE StringInput_VTABLE = {
	String_Range, String_ValidChar, String_ValidString, String_ValidString, 
	MenuInput_NoDefault, MenuInput_NoProcess
};


/*########################################################################################################################*
*-----------------------------------------------------TextInputWidget-----------------------------------------------------*
*#########################################################################################################################*/
static void TextInputWidget_Render(void* widget, double delta) {
	struct InputWidget* w = (struct InputWidget*)widget;
	Texture_Render(&w->inputTex);
	InputWidget_RenderCaret(w, delta);
}

static void TextInputWidget_BuildMesh(void* widget, struct VertexTextured** vertices) {
	struct InputWidget* w = (struct InputWidget*)widget;
	Gfx_Make2DQuad(&w->inputTex, PACKEDCOL_WHITE, vertices);
	Gfx_Make2DQuad(&w->caretTex, w->caretCol,     vertices);
}

static int TextInputWidget_Render2(void* widget, int offset) {
	struct InputWidget* w = (struct InputWidget*)widget;
	Gfx_BindTexture(w->inputTex.ID);
	Gfx_DrawVb_IndexedTris_Range(4, offset);
	offset += 4;

	if (w->showCaret && Math_Mod1((float)w->caretAccumulator) < 0.5f) {
		Gfx_BindTexture(w->caretTex.ID);
		Gfx_DrawVb_IndexedTris_Range(4, offset);
	}
	return offset + 4;
}

static int TextInputWidget_MaxVertices(void* widget) { return MENUINPUTWIDGET_MAX; }

static void TextInputWidget_RemakeTexture(void* widget) {
	cc_string range; char rangeBuffer[STRING_SIZE];
	struct TextInputWidget* w = (struct TextInputWidget*)widget;
	PackedCol backColor = PackedCol_Make(30, 30, 30, 200);
	struct MenuInputDesc* desc;
	struct DrawTextArgs args;
	struct Texture* tex;
	int textWidth, lineHeight;
	int width, height, hintX, y;
	struct Context2D ctx;

	DrawTextArgs_Make(&args, &w->base.text, w->base.font, false);
	textWidth   = Drawer2D_TextWidth(&args);
	lineHeight  = w->base.lineHeight;
	w->base.caretAccumulator = 0.0;

	String_InitArray(range, rangeBuffer);
	desc = &w->desc;
	desc->VTABLE->GetRange(desc, &range);

	width  = max(textWidth,  w->minWidth);  w->base.width  = width;
	height = max(lineHeight, w->minHeight); w->base.height = height;

	Context2D_Alloc(&ctx, width, height);
	{
		/* Centre text vertically */
		y = 0;
		if (lineHeight < height) { y = height / 2 - lineHeight / 2; }
		w->base.caretOffset = 2 + y;

		Context2D_Clear(&ctx, backColor, 0, 0, width, height);
		Context2D_DrawText(&ctx, &args, w->base.padding, y);

		args.text = range;
		hintX     = width - Drawer2D_TextWidth(&args);
		/* Draw hint text right-aligned if it won't overlap input text */
		if (textWidth + 3 < hintX) {
			Context2D_DrawText(&ctx, &args, hintX, y);
		}
	}

	tex = &w->base.inputTex;
	Context2D_MakeTexture(tex, &ctx);
	Context2D_Free(&ctx);

	Widget_Layout(&w->base);
	tex->x = w->base.x; tex->y = w->base.y;
}

static cc_bool TextInputWidget_AllowedChar(void* widget, char c) {
	struct InputWidget* w = (struct InputWidget*)widget;
	struct MenuInputDesc* desc;
	int maxChars;
	cc_bool valid;

	if (c == '&') return false;
	desc = &((struct TextInputWidget*)w)->desc;

	if (!desc->VTABLE->IsValidChar(desc, c)) return false;
	maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (w->text.length == maxChars) return false;

	/* See if the new string is in valid format */
	InputWidget_AppendChar(w, c);
	valid = desc->VTABLE->IsValidString(desc, &w->text);
	InputWidget_DeleteChar(w);
	return valid;
}

static int TextInputWidget_PointerDown(void* widget, int id, int x, int y) {
	struct TextInputWidget* w = (struct TextInputWidget*)widget;
	struct OpenKeyboardArgs args;

	OpenKeyboardArgs_Init(&args, &w->base.text, w->onscreenType);
	args.placeholder = w->onscreenPlaceholder;
	Window_OpenKeyboard(&args);

	w->base.showCaret = true;
	return InputWidget_PointerDown(widget, id, x, y);
}

static int TextInputWidget_GetMaxLines(void) { return 1; }
static const struct WidgetVTABLE TextInputWidget_VTABLE = {
	TextInputWidget_Render,      InputWidget_Free, InputWidget_Reposition,
	InputWidget_KeyDown,         Widget_InputUp,   Widget_MouseScroll,
	TextInputWidget_PointerDown, Widget_PointerUp, Widget_PointerMove,
	TextInputWidget_BuildMesh,   TextInputWidget_Render2, TextInputWidget_MaxVertices
};
void TextInputWidget_Create(struct TextInputWidget* w, int width, const cc_string* text, struct MenuInputDesc* desc) {
	InputWidget_Reset(&w->base);
	w->base.VTABLE = &TextInputWidget_VTABLE;

	w->minWidth  = Display_ScaleX(width);
	w->minHeight = Display_ScaleY(30);
	w->desc      = *desc;

	w->base.convertPercents = false;
	w->base.padding         = 3;
	w->base.showCaret       = !Input_TouchMode;
	w->base.flags           = WIDGET_FLAG_SELECTABLE;

	w->base.GetMaxLines    = TextInputWidget_GetMaxLines;
	w->base.RemakeTexture  = TextInputWidget_RemakeTexture;
	w->base.OnPressedEnter = InputWidget_OnPressedEnter;
	w->base.AllowedChar    = TextInputWidget_AllowedChar;

	String_InitArray(w->base.text, w->_textBuffer);
	String_Copy(&w->base.text, text);
	w->onscreenPlaceholder = "";
	w->onscreenType        = KEYBOARD_TYPE_TEXT;
}

void TextInputWidget_SetFont(struct TextInputWidget* w, struct FontDesc* font) {
	w->base.font       = font;
	w->base.lineHeight = Font_CalcHeight(font, false);
	InputWidget_UpdateText(&w->base);
}


/*########################################################################################################################*
*-----------------------------------------------------ChatInputWidget-----------------------------------------------------*
*#########################################################################################################################*/
static const cc_string chatInputPrefix = String_FromConst("> ");

static void ChatInputWidget_MakeTexture(struct InputWidget* w, int width, int height) {
	cc_string line; char lineBuffer[STRING_SIZE + 2];
	struct DrawTextArgs args;
	struct Context2D ctx;
	char lastCol;
	int i, x, y;

	Context2D_Alloc(&ctx, width, height);

	DrawTextArgs_Make(&args, &chatInputPrefix, w->font, true);
	Context2D_DrawText(&ctx, &args, 0, 0);

	String_InitArray(line, lineBuffer);
	for (i = 0, y = 0; i < Array_Elems(w->lines); i++) {
		if (!w->lines[i].length) break;
		line.length = 0;

		/* Color code continues in next line */
		lastCol = InputWidget_GetLastCol(w, 0, i);
		if (!Drawer2D_IsWhiteColor(lastCol)) {
			String_Append(&line, '&'); String_Append(&line, lastCol);
		}
		/* Convert % to & for color codes */
		InputWidget_FormatLine(w, i, &line);
		args.text = line;

		x = i == 0 ? w->prefixWidth : 0;
		Context2D_DrawText(&ctx, &args, x, y);
		y += w->lineHeight;
	}

	Context2D_MakeTexture(&w->inputTex, &ctx);
	Context2D_Free(&ctx);
}

static void ChatInputWidget_RemakeTexture(void* widget) {
	struct InputWidget* w = (struct InputWidget*)widget;
	int width = 0, height = 0;
	int i;

	for (i = 0; i < w->GetMaxLines(); i++) {
		if (!w->lines[i].length) break;
		height += w->lineHeight;
		width   = max(width, w->lineWidths[i]);
	}

	if (!width)  width  = w->prefixWidth;
	if (!height) height = w->lineHeight;
	
	if (w->flags & WIDGET_FLAG_DISABLED) {
		Gfx_DeleteTexture(&w->inputTex.ID);
	} else {
		ChatInputWidget_MakeTexture(w, width, height);
	}
	w->caretAccumulator = 0;

	w->width  = width;
	w->height = height;
	Widget_Layout(w);
	w->inputTex.x = w->x + w->padding;
	w->inputTex.y = w->y;
}

static void ChatInputWidget_Render(void* widget, double delta) {
	struct InputWidget* w = (struct InputWidget*)widget;
	PackedCol backColor   = PackedCol_Make(0, 0, 0, 127);
	int x = w->x, y = w->y;
	cc_bool caretAtEnd;
	int i, width;
	if (w->flags & WIDGET_FLAG_DISABLED) return;

	for (i = 0; i < INPUTWIDGET_MAX_LINES; i++) {
		if (i > 0 && !w->lines[i].length) break;

		caretAtEnd = (w->caretY == i) && (w->caretX == INPUTWIDGET_LEN || w->caretPos == -1);
		width      = w->lineWidths[i] + (caretAtEnd ? w->caretTex.Width : 0);
		/* Cover whole window width to match Minecraft behaviour */
		width      = max(width, WindowInfo.Width - x * 4);
	
		Gfx_Draw2DFlat(x, y, width + w->padding * 2, w->lineHeight, backColor);
		y += w->lineHeight;
	}

	Texture_Render(&w->inputTex);
	InputWidget_RenderCaret(w, delta);
}

static void ChatInputWidget_OnPressedEnter(void* widget) {
	struct ChatInputWidget* w = (struct ChatInputWidget*)widget;
	/* Don't want trailing spaces in output message */
	cc_string text = w->base.text;
	String_UNSAFE_TrimEnd(&text);
	if (text.length) { Chat_Send(&text, true); }

	w->origStr.length = 0;
	w->typingLogPos = Chat_InputLog.count; /* Index of newest entry + 1. */

	Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_2);
	InputWidget_OnPressedEnter(widget);
}

static void ChatInputWidget_UpKey(struct InputWidget* w) {
	struct ChatInputWidget* W = (struct ChatInputWidget*)w;
	cc_string prevInput;
	int pos;

	if (Input_IsActionPressed()) {
		pos = w->caretPos == -1 ? w->text.length : w->caretPos;
		if (pos < INPUTWIDGET_LEN) return;

		w->caretPos = pos - INPUTWIDGET_LEN;
		InputWidget_UpdateCaret(w);
		return;
	}

	if (W->typingLogPos == Chat_InputLog.count) {
		String_Copy(&W->origStr, &w->text);
	}

	if (!Chat_InputLog.count) return;
	W->typingLogPos--;
	w->text.length = 0;

	if (W->typingLogPos < 0) W->typingLogPos = 0;
	prevInput = StringsBuffer_UNSAFE_Get(&Chat_InputLog, W->typingLogPos);
	String_AppendString(&w->text, &prevInput);

	w->caretPos = -1;
	InputWidget_UpdateText(w);
}

static void ChatInputWidget_DownKey(struct InputWidget* w) {
	struct ChatInputWidget* W = (struct ChatInputWidget*)w;
	cc_string prevInput;

	if (Input_IsActionPressed()) {
		if (w->caretPos == -1) return;

		w->caretPos += INPUTWIDGET_LEN;
		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
		InputWidget_UpdateCaret(w);
		return;
	}

	if (!Chat_InputLog.count) return;
	W->typingLogPos++;
	w->text.length = 0;

	if (W->typingLogPos >= Chat_InputLog.count) {
		W->typingLogPos = Chat_InputLog.count;
		String_AppendString(&w->text, &W->origStr);
	} else {
		prevInput = StringsBuffer_UNSAFE_Get(&Chat_InputLog, W->typingLogPos);
		String_AppendString(&w->text, &prevInput);
	}

	w->caretPos = -1;
	InputWidget_UpdateText(w);
}

static cc_bool ChatInputWidget_IsNameChar(char c) {
	return c == '_' || c == '.' || (c >= '0' && c <= '9')
		|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void ChatInputWidget_TabKey(struct InputWidget* w) {
	cc_string str; char strBuffer[STRING_SIZE];
	EntityID matches[TABLIST_MAX_NAMES];
	cc_string part, name;
	int beg, end, len;
	int i, j, numMatches;
	char* buffer;

	end = w->caretPos == -1 ? w->text.length - 1 : w->caretPos;
	beg = end;
	buffer = w->text.buffer;

	/* e.g. if player typed "hi Nam", backtrack to "N" */
	while (beg >= 0 && ChatInputWidget_IsNameChar(buffer[beg])) beg--;
	beg++;
	if (end < 0 || beg > end) return;

	part = String_UNSAFE_Substring(&w->text, beg, (end + 1) - beg);
	Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_2);
	numMatches = 0;

	for (i = 0; i < TABLIST_MAX_NAMES; i++) {
		if (!TabList.NameOffsets[i]) continue;

		name = TabList_UNSAFE_GetPlayer(i);
		if (!String_CaselessContains(&name, &part)) continue;
		matches[numMatches++] = (EntityID)i;
	}

	if (numMatches == 1) {
		if (w->caretPos == -1) end++;
		len = end - beg;

		/* Following on from above example, delete 'N','a','m' */
		/* Then insert the e.g. matching 'nAME1' player name */
		for (j = 0; j < len; j++) {
			String_DeleteAt(&w->text, beg);
		}

		if (w->caretPos != -1) w->caretPos -= len;
		name = TabList_UNSAFE_GetPlayer(matches[0]);
		InputWidget_AppendText(w, &name);
	} else if (numMatches > 1) {
		String_InitArray(str, strBuffer);
		String_Format1(&str, "&e%i matching names: ", &numMatches);

		for (i = 0; i < numMatches; i++) {
			name = TabList_UNSAFE_GetPlayer(matches[i]);
			if ((str.length + name.length + 1) > STRING_SIZE) break;

			String_AppendString(&str, &name);
			String_Append(&str, ' ');
		}
		Chat_AddOf(&str, MSG_TYPE_CLIENTSTATUS_2);
	}
}

static int ChatInputWidget_KeyDown(void* widget, int key) {
	struct InputWidget* w = (struct InputWidget*)widget;
	if (key == CCKEY_TAB) { 
		ChatInputWidget_TabKey(w);  return true; 
	} else if (Input_IsUpButton(key)) { 
		ChatInputWidget_UpKey(w);   return true;
	} else if (Input_IsDownButton(key)) { 
		ChatInputWidget_DownKey(w); return true; 
	}
	return InputWidget_KeyDown(w, key);
}

static int ChatInputWidget_GetMaxLines(void) {
	return !Game_ClassicMode && Server.SupportsPartialMessages ? INPUTWIDGET_MAX_LINES : 1;
}

static const struct WidgetVTABLE ChatInputWidget_VTABLE = {
	ChatInputWidget_Render,  InputWidget_Free, InputWidget_Reposition,
	ChatInputWidget_KeyDown, Widget_InputUp,   Widget_MouseScroll,
	InputWidget_PointerDown, Widget_PointerUp, Widget_PointerMove
};
void ChatInputWidget_Create(struct ChatInputWidget* w) {
	InputWidget_Reset(&w->base);
	w->typingLogPos = Chat_InputLog.count; /* Index of newest entry + 1. */
	w->base.VTABLE  = &ChatInputWidget_VTABLE;

	w->base.convertPercents = !Game_ClassicMode;
	w->base.showCaret       = true;
	w->base.padding         = 5;
	w->base.GetMaxLines     = ChatInputWidget_GetMaxLines;
	w->base.RemakeTexture   = ChatInputWidget_RemakeTexture;
	w->base.OnPressedEnter  = ChatInputWidget_OnPressedEnter;
	w->base.AllowedChar     = InputWidget_AllowedChar;

	String_InitArray(w->base.text, w->_textBuffer);
	String_InitArray(w->origStr,   w->_origBuffer);
}	

void ChatInputWidget_SetFont(struct ChatInputWidget* w, struct FontDesc* font) {
	struct DrawTextArgs args;	
	DrawTextArgs_Make(&args, &chatInputPrefix, font, true);

	w->base.font        = font;
	w->base.prefixWidth = Drawer2D_TextWidth(&args);
	w->base.lineHeight  = Drawer2D_TextHeight(&args);
	Gfx_DeleteTexture(&w->base.caretTex.ID);
}	


/*########################################################################################################################*
*-----------------------------------------------------TextGroupWidget-----------------------------------------------------*
*#########################################################################################################################*/
void TextGroupWidget_ShiftUp(struct TextGroupWidget* w) {
	int last, i;
	Gfx_DeleteTexture(&w->textures[0].ID);
	last = w->lines - 1;

	for (i = 0; i < last; i++) 
	{
		w->textures[i] = w->textures[i + 1];
	}
	w->textures[last].ID = 0; /* Gfx_DeleteTexture() called by TextGroupWidget_Redraw otherwise */
	TextGroupWidget_Redraw(w, last);
}

void TextGroupWidget_ShiftDown(struct TextGroupWidget* w) {
	int last, i;
	last = w->lines - 1;
	Gfx_DeleteTexture(&w->textures[last].ID);

	for (i = last; i > 0; i--) 
	{
		w->textures[i] = w->textures[i - 1];
	}
	w->textures[0].ID = 0; /* Gfx_DeleteTexture() called by TextGroupWidget_Redraw otherwise */
	TextGroupWidget_Redraw(w, 0);
}

int TextGroupWidget_UsedHeight(struct TextGroupWidget* w) {
	struct Texture* textures = w->textures;
	int i, height = 0;

	for (i = 0; i < w->lines; i++) 
	{
		if (textures[i].ID) break;
	}
	for (; i < w->lines; i++) 
	{
		height += textures[i].Height;
	}
	return height;
}

static void TextGroupWidget_Reposition(void* widget) {
	struct TextGroupWidget* w = (struct TextGroupWidget*)widget;
	struct Texture* textures  = w->textures;
	int i, y, width = 0, height = 0;
	
	/* Work out how big the text group is now */
	for (i = 0; i < w->lines; i++) 
	{
		width = max(width, textures[i].Width);
		height += textures[i].Height;
	}

	w->width = width; w->height = height;
	Widget_CalcPosition(w);

	for (i = 0, y = w->y; i < w->lines; i++) 
	{
		textures[i].x = Gui_CalcPos(w->horAnchor, w->xOffset, textures[i].Width, WindowInfo.Width);
		textures[i].y = y;
		y += textures[i].Height;
	}
}

struct Portion { short Beg, Len, LineBeg, LineLen; };
#define TEXTGROUPWIDGET_HTTP_LEN 7 /* length of http:// */
#define TEXTGROUPWIDGET_URL 0x8000
#define TEXTGROUPWIDGET_PACKED_LEN 0x7FFF

static int TextGroupWidget_NextUrl(char* chars, int charsLen, int i) {
	int start, left;

	for (; i < charsLen; i++) 
	{
		if (!(chars[i] == 'h' || chars[i] == '&')) continue;
		left = charsLen - i;
		if (left < TEXTGROUPWIDGET_HTTP_LEN) return charsLen;

		/* color codes at start of URL */
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

static int TextGroupWidget_UrlEnd(char* chars, int charsLen, int* begs, int begsLen, int i) {
	int start = i, j;
	int next, left;
	cc_bool isBeg;

	for (; i < charsLen && chars[i] != ' '; i++) 
	{
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
	int begs[GUI_MAX_CHATLINES];
	int ends[GUI_MAX_CHATLINES];
	struct Portion bit;
	cc_string line;
	int nextStart, i, total = 0, end;

	for (i = 0; i < w->lines; i++) 
	{
		line = TextGroupWidget_UNSAFE_Get(w, i);
		begs[i] = -1; ends[i] = -1;
		if (!line.length) continue;

		begs[i] = total;
		Mem_Copy(&chars[total], line.buffer, line.length);
		total += line.length; ends[i] = total;
	}

	end = 0;
	for (;;) 
	{
		nextStart = TextGroupWidget_NextUrl(chars, total, end);

		/* add normal portion between urls */
		bit.Beg = end;
		bit.Len = nextStart - end;
		TextGroupWidget_Output(bit, begs[target], ends[target], &portions);

		if (nextStart == total) break;
		end = TextGroupWidget_UrlEnd(chars, total, begs, w->lines, nextStart);

		/* add this url portion */
		bit.Beg = nextStart;
		bit.Len = (end - nextStart) | TEXTGROUPWIDGET_URL;
		TextGroupWidget_Output(bit, begs[target], ends[target], &portions);
	}
	return (int)(portions - start);
}

static void TextGroupWidget_FormatUrl(cc_string* text, const cc_string* url) {
	char* dst;
	int i;
	String_AppendColorless(text, url);

	/* Delete "> " multiline chars from URLs */
	dst = text->buffer;
	for (i = text->length - 2; i >= 0; i--) 
	{
		if (dst[i] != '>' || dst[i + 1] != ' ') continue;

		String_DeleteAt(text, i + 1);
		String_DeleteAt(text, i);
	}
}

static cc_bool TextGroupWidget_GetUrl(struct TextGroupWidget* w, cc_string* text, int index, int mouseX) {
	char chars[GUI_MAX_CHATLINES * TEXTGROUPWIDGET_LEN];
	struct Portion portions[2 * (TEXTGROUPWIDGET_LEN / TEXTGROUPWIDGET_HTTP_LEN)];
	struct Portion bit;
	struct DrawTextArgs args = { 0 };
	cc_string line, url;
	int portionsCount;
	int i, x, width;

	mouseX -= w->textures[index].x;
	args.useShadow = true;
	line = TextGroupWidget_UNSAFE_Get(w, index);

	if (Game_ClassicMode) return false;
	portionsCount = TextGroupWidget_Reduce(w, chars, index, portions);

	for (i = 0, x = 0; i < portionsCount; i++) 
	{
		bit = portions[i];
		args.text = String_UNSAFE_Substring(&line, bit.LineBeg, bit.LineLen);
		args.font = w->font;

		width = Drawer2D_TextWidth(&args);
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

int TextGroupWidget_GetSelected(struct TextGroupWidget* w, cc_string* text, int x, int y) {
	struct Texture tex;
	cc_string line;
	int i;

	for (i = 0; i < w->lines; i++) 
	{
		if (!w->textures[i].ID) continue;
		tex = w->textures[i];
		if (!Gui_Contains(tex.x, tex.y, tex.Width, tex.Height, x, y)) continue;

		if (!TextGroupWidget_GetUrl(w, text, i, x)) {
			line = TextGroupWidget_UNSAFE_Get(w, i);
			String_AppendString(text, &line);
		}
		return i;
	}
	return -1;
}

static cc_bool TextGroupWidget_MightHaveUrls(struct TextGroupWidget* w) {
	cc_string line;
	int i;

	for (i = 0; i < w->lines; i++) 
	{
		line = TextGroupWidget_UNSAFE_Get(w, i);
		if (String_IndexOf(&line, '/') >= 0) return true;
	}
	return false;
}

static void TextGroupWidget_DrawAdvanced(struct TextGroupWidget* w, struct Texture* tex, struct DrawTextArgs* args, int index, const cc_string* text) {
	char chars[GUI_MAX_CHATLINES * TEXTGROUPWIDGET_LEN];
	struct Portion portions[2 * (TEXTGROUPWIDGET_LEN / TEXTGROUPWIDGET_HTTP_LEN)];
	struct Portion bit;
	int width, height;
	int partWidths[Array_Elems(portions)];
	struct Context2D ctx;
	int portionsCount;
	int i, x, ul;

	width = 0;
	height = Drawer2D_TextHeight(args);
	portionsCount = TextGroupWidget_Reduce(w, chars, index, portions);
	
	for (i = 0; i < portionsCount; i++) {
		bit = portions[i];
		args->text = String_UNSAFE_Substring(text, bit.LineBeg, bit.LineLen);

		partWidths[i] = Drawer2D_TextWidth(args);
		width += partWidths[i];
	}
	
	Context2D_Alloc(&ctx, width, height);
	{
		x = 0;
		for (i = 0; i < portionsCount; i++) {
			bit = portions[i];
			ul  = (bit.Len & TEXTGROUPWIDGET_URL);
			args->text = String_UNSAFE_Substring(text, bit.LineBeg, bit.LineLen);

			if (ul) args->font->flags |= FONT_FLAGS_UNDERLINE;
			Context2D_DrawText(&ctx, args, x, 0);
			if (ul) args->font->flags &= ~FONT_FLAGS_UNDERLINE;

			x += partWidths[i];
		}
		Context2D_MakeTexture(tex, &ctx);
	}
	Context2D_Free(&ctx);
}

void TextGroupWidget_RedrawAll(struct TextGroupWidget* w) {
	int i;
	for (i = 0; i < w->lines; i++) { TextGroupWidget_Redraw(w, i); }
}

void TextGroupWidget_Redraw(struct TextGroupWidget* w, int index) {
	cc_string text;
	struct DrawTextArgs args;
	struct Texture tex = { 0 };
	Gfx_DeleteTexture(&w->textures[index].ID);

	text = TextGroupWidget_UNSAFE_Get(w, index);
	if (!Drawer2D_IsEmptyText(&text)) {
		DrawTextArgs_Make(&args, &text, w->font, true);

		if (w->underlineUrls && TextGroupWidget_MightHaveUrls(w)) {
			TextGroupWidget_DrawAdvanced(w, &tex, &args, index, &text);
		} else {
			Drawer2D_MakeTextTexture(&tex, &args);
		}
		Drawer2D_ReducePadding_Tex(&tex, w->font->size, 3);
	} else {
		tex.Height = w->collapsible[index] ? 0 : w->defaultHeight;
	}

	tex.x = Gui_CalcPos(w->horAnchor, w->xOffset, tex.Width, WindowInfo.Width);
	w->textures[index] = tex;
	Widget_Layout(w);
}

void TextGroupWidget_RedrawAllWithCol(struct TextGroupWidget* group, char col) {
	cc_string line;
	int i, j;

	for (i = 0; i < group->lines; i++) 
	{
		line = TextGroupWidget_UNSAFE_Get(group, i);
		if (!line.length) continue;

		for (j = 0; j < line.length - 1; j++) 
		{
			if (line.buffer[j] == '&' && line.buffer[j + 1] == col) {
				TextGroupWidget_Redraw(group, i);
				break;
			}
		}
	}
}


void TextGroupWidget_SetFont(struct TextGroupWidget* w, struct FontDesc* font) {
	int i, height;
	
	height = Font_CalcHeight(font, true);
	Drawer2D_ReducePadding_Height(&height, font->size, 3);
	w->defaultHeight = height;

	for (i = 0; i < w->lines; i++) 
	{
		w->textures[i].Height = w->collapsible[i] ? 0 : height;
	}
	w->font = font;
	Widget_Layout(w);
}

static void TextGroupWidget_Render(void* widget, double delta) {
	struct TextGroupWidget* w = (struct TextGroupWidget*)widget;
	struct Texture* textures  = w->textures;
	int i;

	for (i = 0; i < w->lines; i++) 
	{
		if (!textures[i].ID) continue;
		Texture_Render(&textures[i]);
	}
}

static void TextGroupWidget_Free(void* widget) {
	struct TextGroupWidget* w = (struct TextGroupWidget*)widget;
	int i;

	for (i = 0; i < w->lines; i++) 
	{
		Gfx_DeleteTexture(&w->textures[i].ID);
	}
}

static void TextGroupWidget_BuildMesh(void* widget, struct VertexTextured** vertices) {
	struct TextGroupWidget* w = (struct TextGroupWidget*)widget;
	int i;

	for (i = 0; i < w->lines; i++)
	{
		Gfx_Make2DQuad(&w->textures[i], PACKEDCOL_WHITE, vertices);
	}
}

static int TextGroupWidget_Render2(void* widget, int offset) {
	struct TextGroupWidget* w = (struct TextGroupWidget*)widget;
	struct Texture* textures  = w->textures;
	int i;

	for (i = 0; i < w->lines; i++, offset += 4)
	{
		if (!textures[i].ID) continue;

		Gfx_BindTexture(textures[i].ID);
		Gfx_DrawVb_IndexedTris_Range(4, offset);
	}
	return offset;
}

static int TextGroupWidget_MaxVertices(void* widget) { 
	struct TextGroupWidget* w = (struct TextGroupWidget*)widget;
	return w->lines * 4;
}

static const struct WidgetVTABLE TextGroupWidget_VTABLE = {
	TextGroupWidget_Render, TextGroupWidget_Free, TextGroupWidget_Reposition,
	Widget_InputDown,       Widget_InputUp,       Widget_MouseScroll,
	Widget_Pointer,         Widget_PointerUp,     Widget_PointerMove,
	TextGroupWidget_BuildMesh, TextGroupWidget_Render2, TextGroupWidget_MaxVertices
};
void TextGroupWidget_Create(struct TextGroupWidget* w, int lines, struct Texture* textures, TextGroupWidget_Get getLine) {
	Widget_Reset(w);
	w->VTABLE   = &TextGroupWidget_VTABLE;
	w->lines    = lines;
	w->textures = textures;
	w->GetLine  = getLine;
}


/*########################################################################################################################*
*---------------------------------------------------SpecialInputWidget----------------------------------------------------*
*#########################################################################################################################*/
static void SpecialInputWidget_UpdateColString(struct SpecialInputWidget* w) {
	int i;
	String_InitArray(w->colString, w->_colBuffer);

	for (i = 0; i < DRAWER2D_MAX_COLORS; i++) {
		if (i >= 'A' && i <= 'F')           continue;
		if (!BitmapCol_A(Drawer2D.Colors[i])) continue;

		String_Append(&w->colString, '&'); String_Append(&w->colString, (char)i);
		String_Append(&w->colString, '%'); String_Append(&w->colString, (char)i);
	}
}

static cc_bool SpecialInputWidget_IntersectsTitle(struct SpecialInputWidget* w, int x, int y) {
	int i, width, titleX = 0;

	for (i = 0; i < Array_Elems(w->tabs); i++) {
		width = w->tabs[i].titleWidth;
		if (Gui_Contains(titleX, 0, width, w->titleHeight, x, y)) {
			w->selectedIndex = i;
			return true;
		}
		titleX += width;
	}
	return false;
}

static void SpecialInputWidget_IntersectsBody(struct SpecialInputWidget* w, int x, int y) {
	struct SpecialInputTab e = w->tabs[w->selectedIndex];
	cc_string str;
	int i;

	y -= w->titleHeight;
	x /= w->elementWidth; y /= w->elementHeight;
	
	i = (x + y * e.itemsPerRow) * e.charsPerItem;
	if (i >= e.contents.length) return;

	/* TODO: need to insert characters that don't affect w->caretPos index, adjust w->caretPos color */
	str = String_Init(&e.contents.buffer[i], e.charsPerItem, 0);

	/* TODO: Not be so hacky */
	if (w->selectedIndex == 0) str.length = 2;
	InputWidget_AppendText(w->target, &str);
}

static void SpecialInputTab_Init(struct SpecialInputTab* tab, STRING_REF cc_string* title, int itemsPerRow, int charsPerItem, STRING_REF cc_string* contents) {
	tab->title      = *title;
	tab->titleWidth = 0;
	tab->contents   = *contents;
	tab->itemsPerRow  = itemsPerRow;
	tab->charsPerItem = charsPerItem;
}

static void SpecialInputWidget_InitTabs(struct SpecialInputWidget* w) {
	static cc_string title_cols = String_FromConst("Colours");
	static cc_string title_math = String_FromConst("Math");
	static cc_string tab_math   = String_FromConst("\x9F\xAB\xAC\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xFB\xFC\xFD");
	static cc_string title_line = String_FromConst("Line/Box");
	static cc_string tab_line   = String_FromConst("\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xFE");
	static cc_string title_letters = String_FromConst("Letters");
	static cc_string tab_letters   = String_FromConst("\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\xA0\xA1\xA2\xA3\xA4\xA5");
	static cc_string title_other = String_FromConst("Other");
	static cc_string tab_other   = String_FromConst("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x7F\x9B\x9C\x9D\x9E\xA6\xA7\xA8\xA9\xAA\xAD\xAE\xAF\xF9\xFA");

	SpecialInputWidget_UpdateColString(w);
	SpecialInputTab_Init(&w->tabs[0], &title_cols,    10, 4, &w->colString);
	SpecialInputTab_Init(&w->tabs[1], &title_math,    16, 1, &tab_math);
	SpecialInputTab_Init(&w->tabs[2], &title_line,    17, 1, &tab_line);
	SpecialInputTab_Init(&w->tabs[3], &title_letters, 17, 1, &tab_letters);
	SpecialInputTab_Init(&w->tabs[4], &title_other,   16, 1, &tab_other);
}

#define SPECIAL_TITLE_SPACING 10
#define SPECIAL_CONTENT_SPACING 5
static int SpecialInputWidget_MeasureTitles(struct SpecialInputWidget* w) {
	struct DrawTextArgs args; 
	int i, width = 0;

	DrawTextArgs_MakeEmpty(&args, w->font, false);
	for (i = 0; i < Array_Elems(w->tabs); i++) {
		args.text = w->tabs[i].title;

		w->tabs[i].titleWidth = Drawer2D_TextWidth(&args) + SPECIAL_TITLE_SPACING;
		width += w->tabs[i].titleWidth;
	}

	w->titleHeight = Drawer2D_TextHeight(&args);
	return width;
}

static void SpecialInputWidget_DrawTitles(struct SpecialInputWidget* w, struct Context2D* ctx) {
	BitmapCol color_selected = BitmapCol_Make(30, 30, 30, 200);
	BitmapCol color_inactive = BitmapCol_Make( 0,  0,  0, 127);
	BitmapCol color;
	struct DrawTextArgs args;
	int i, width, x = 0;

	DrawTextArgs_MakeEmpty(&args, w->font, false);
	for (i = 0; i < Array_Elems(w->tabs); i++) {
		args.text = w->tabs[i].title;
		color = i == w->selectedIndex ? color_selected : color_inactive;
		width = w->tabs[i].titleWidth;

		Context2D_Clear(ctx, color, x, 0, width, w->titleHeight);
		Context2D_DrawText(ctx, &args, x + SPECIAL_TITLE_SPACING / 2, 0);
		x += width;
	}
}

static int SpecialInputWidget_MeasureContent(struct SpecialInputWidget* w, struct SpecialInputTab* tab) {
	struct DrawTextArgs args;
	int textWidth, textHeight;
	int i, maxWidth = 0;

	DrawTextArgs_MakeEmpty(&args, w->font, false);
	args.text.length = tab->charsPerItem;
	textHeight       = Drawer2D_TextHeight(&args);

	for (i = 0; i < tab->contents.length; i += tab->charsPerItem) {
		args.text.buffer = &tab->contents.buffer[i];
		textWidth = Drawer2D_TextWidth(&args);
		maxWidth  = max(maxWidth, textWidth);
	}

	w->elementWidth  = maxWidth   + SPECIAL_CONTENT_SPACING;
	w->elementHeight = textHeight + SPECIAL_CONTENT_SPACING;
	return w->elementWidth  * tab->itemsPerRow;
}

static int SpecialInputWidget_ContentHeight(struct SpecialInputWidget* w, struct SpecialInputTab* tab) {
	int rows = Math_CeilDiv(tab->contents.length / tab->charsPerItem, tab->itemsPerRow);
	return w->elementHeight * rows;
}

static void SpecialInputWidget_DrawContent(struct SpecialInputWidget* w, struct SpecialInputTab* tab, struct Context2D* ctx, int yOffset) {
	struct DrawTextArgs args;
	int i, x, y, item;	

	int wrap = tab->itemsPerRow;
	DrawTextArgs_MakeEmpty(&args, w->font, false);
	args.text.length = tab->charsPerItem;

	for (i = 0; i < tab->contents.length; i += tab->charsPerItem) {
		args.text.buffer = &tab->contents.buffer[i];
		item = i / tab->charsPerItem;

		x = (item % wrap) * w->elementWidth;
		y = (item / wrap) * w->elementHeight + yOffset;
		Context2D_DrawText(ctx, &args, x, y);
	}
}

static void SpecialInputWidget_Make(struct SpecialInputWidget* w, struct SpecialInputTab* tab) {
	BitmapCol color = BitmapCol_Make(30, 30, 30, 200);
	int titlesWidth, titlesHeight;
	int contentWidth, contentHeight;
	struct Context2D ctx;
	int width, height;

	titlesWidth   = SpecialInputWidget_MeasureTitles(w);
	titlesHeight  = w->titleHeight;
	contentWidth  = SpecialInputWidget_MeasureContent(w, tab);
	contentHeight = SpecialInputWidget_ContentHeight(w, tab);

	width  = max(titlesWidth, contentWidth);
	height = titlesHeight + contentHeight;
	Gfx_DeleteTexture(&w->tex.ID);

	Context2D_Alloc(&ctx, width, height);
	{
		SpecialInputWidget_DrawTitles(w, &ctx);
		Context2D_Clear(&ctx, color, 0, titlesHeight, width, contentHeight);
		SpecialInputWidget_DrawContent(w, tab, &ctx, titlesHeight);
	}
	Context2D_MakeTexture(&w->tex, &ctx);
	Context2D_Free(&ctx);
}

void SpecialInputWidget_Redraw(struct SpecialInputWidget* w) {
	SpecialInputWidget_Make(w, &w->tabs[w->selectedIndex]);
	w->pendingRedraw = false;
	Widget_Layout(w);
}

static void SpecialInputWidget_Render(void* widget, double delta) {
	struct SpecialInputWidget* w = (struct SpecialInputWidget*)widget;
	Texture_Render(&w->tex);
}

static void SpecialInputWidget_Free(void* widget) {
	struct SpecialInputWidget* w = (struct SpecialInputWidget*)widget;
	Gfx_DeleteTexture(&w->tex.ID);
}

static void SpecialInputWidget_Reposition(void* widget) {
	struct SpecialInputWidget* w = (struct SpecialInputWidget*)widget;
	w->width  = w->tex.Width;
	w->height = w->active ? w->tex.Height : 0;
	Widget_CalcPosition(w);
	w->tex.x = w->x; w->tex.y = w->y;
}

static int SpecialInputWidget_PointerDown(void* widget, int id, int x, int y) {
	struct SpecialInputWidget* w = (struct SpecialInputWidget*)widget;
	x -= w->x; y -= w->y;

	if (SpecialInputWidget_IntersectsTitle(w, x, y)) {
		SpecialInputWidget_Redraw(w);
	} else {
		SpecialInputWidget_IntersectsBody(w, x, y);
	}
	return TOUCH_TYPE_GUI;
}

void SpecialInputWidget_UpdateCols(struct SpecialInputWidget* w) {
	SpecialInputWidget_UpdateColString(w);
	w->tabs[0].contents = w->colString;
	if (w->selectedIndex != 0) return;

	/* defer updating colours tab until visible */
	if (!w->active) { w->pendingRedraw = true; return; }
	SpecialInputWidget_Redraw(w);
}

void SpecialInputWidget_SetActive(struct SpecialInputWidget* w, cc_bool active) {
	w->active = active;
	if (active && w->pendingRedraw) SpecialInputWidget_Redraw(w);
	Widget_Layout(w);
}

static const struct WidgetVTABLE SpecialInputWidget_VTABLE = {
	SpecialInputWidget_Render,      SpecialInputWidget_Free, SpecialInputWidget_Reposition,
	Widget_InputDown,               Widget_InputUp,          Widget_MouseScroll,
	SpecialInputWidget_PointerDown, Widget_PointerUp,        Widget_PointerMove
};
void SpecialInputWidget_Create(struct SpecialInputWidget* w, struct FontDesc* font, struct InputWidget* target) {
	Widget_Reset(w);
	w->VTABLE    = &SpecialInputWidget_VTABLE;
	w->verAnchor = ANCHOR_MAX;
	w->font      = font;
	w->target    = target;
	SpecialInputWidget_InitTabs(w);
}


/*########################################################################################################################*
*----------------------------------------------------ThumbstickWidget-----------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_TOUCH
#define DIR_YMAX (1 << 0)
#define DIR_YMIN (1 << 1)
#define DIR_XMAX (1 << 2)
#define DIR_XMIN (1 << 3)

static void ThumbstickWidget_Rotate(void* widget, struct VertexTextured** vertices, int offset) {
	struct ThumbstickWidget* w = (struct ThumbstickWidget*)widget;
	struct VertexTextured* ptr;
	int i;

	ptr = *vertices - 4;
	for (i = 0; i < 4; i++) {
		int x = ptr[i].x - w->x;
		int y = ptr[i].y - w->y;
		ptr[i].x = -y + w->x + offset;
		ptr[i].y =  x + w->y;
	}
}

static void ThumbstickWidget_BuildGroup(void* widget, struct Texture* tex, struct VertexTextured** vertices) {
	struct ThumbstickWidget* w = (struct ThumbstickWidget*)widget;
	float tmp;
	tex->y = w->y + w->height / 2;
	Gfx_Make2DQuad(tex, PACKEDCOL_WHITE, vertices);

	tex->y = w->y;
	tmp    = tex->uv.V1; tex->uv.V1 = tex->uv.V2; tex->uv.V2 = tmp;
	Gfx_Make2DQuad(tex, PACKEDCOL_WHITE, vertices);

	Gfx_Make2DQuad(tex, PACKEDCOL_WHITE, vertices);
	ThumbstickWidget_Rotate(widget, vertices, w->width);

	tmp    = tex->uv.V1; tex->uv.V1 = tex->uv.V2; tex->uv.V2 = tmp;
	Gfx_Make2DQuad(tex, PACKEDCOL_WHITE, vertices);
	ThumbstickWidget_Rotate(widget, vertices, w->width / 2);
}

static void ThumbstickWidget_BuildMesh(void* widget, struct VertexTextured** vertices) {
	struct ThumbstickWidget* w = (struct ThumbstickWidget*)widget;
	struct Texture tex;

	tex.x     = w->x;
	tex.Width = w->width; tex.Height = w->height / 2;
	tex.uv.U1 = 0.0f;     tex.uv.U2  = 1.0f;

	tex.uv.V1 = 0.0f; tex.uv.V2 = 0.5f;
	ThumbstickWidget_BuildGroup(widget, &tex, vertices);
	tex.uv.V1 = 0.5f; tex.uv.V2 = 1.0f;
	ThumbstickWidget_BuildGroup(widget, &tex, vertices);
}

static int ThumbstickWidget_CalcDirs(struct ThumbstickWidget* w) {
	int i, dx, dy, dirs = 0;
	double angle;

	for (i = 0; i < INPUT_MAX_POINTERS; i++) {
		if (!(w->active & (1 << i))) continue;

		dx = Pointers[i].x - (w->x + w->width  / 2);
		dy = Pointers[i].y - (w->y + w->height / 2);
		angle = Math_Atan2(dx, dy) * MATH_RAD2DEG;

		/* 4 quadrants diagonally, but slightly expanded for overlap*/
		if (angle >=   30 && angle <= 150) dirs |= DIR_YMAX;
		if (angle >=  -60 && angle <=  60) dirs |= DIR_XMAX;
		if (angle >= -150 && angle <= -30) dirs |= DIR_YMIN;
		if (angle <  -120 || angle >  120) dirs |= DIR_XMIN;
	}
	return dirs;
}

static int ThumbstickWidget_Render2(void* widget, int offset) {
	struct ThumbstickWidget* w = (struct ThumbstickWidget*)widget;
	int i, base, flags = ThumbstickWidget_CalcDirs(w);

	if (Gui.TouchTex) {
		Gfx_BindTexture(Gui.TouchTex);
		for (i = 0; i < 4; i++) {
			base = (flags & (1 << i)) ? 0 : THUMBSTICKWIDGET_PER;
			Gfx_DrawVb_IndexedTris_Range(4, offset + base + (i * 4));
		}
	}
	return offset + THUMBSTICKWIDGET_MAX;
}

static void ThumbstickWidget_Reposition(void* widget) {
	struct ThumbstickWidget* w = (struct ThumbstickWidget*)widget;
	w->width  = Display_ScaleX(128 * w->scale);
	w->height = Display_ScaleY(128 * w->scale);
	Widget_CalcPosition(w);
}

static const struct WidgetVTABLE ThumbstickWidget_VTABLE = {
	NULL, Screen_NullFunc, ThumbstickWidget_Reposition,
	Widget_InputDown,  Widget_InputUp,   Widget_MouseScroll,
	Widget_Pointer,    Widget_PointerUp, Widget_PointerMove,
	ThumbstickWidget_BuildMesh, ThumbstickWidget_Render2
};
void ThumbstickWidget_Init(struct ThumbstickWidget* w) {
	Widget_Reset(w);
	w->VTABLE = &ThumbstickWidget_VTABLE;
	w->scale  = 1;
}

void ThumbstickWidget_GetMovement(struct ThumbstickWidget* w, float* xMoving, float* zMoving) {
	int dirs = ThumbstickWidget_CalcDirs(w);
	if (dirs & DIR_XMIN) *xMoving -= 1;
	if (dirs & DIR_XMAX) *xMoving += 1;
	if (dirs & DIR_YMIN) *zMoving -= 1;
	if (dirs & DIR_YMAX) *zMoving += 1;
}
#endif
