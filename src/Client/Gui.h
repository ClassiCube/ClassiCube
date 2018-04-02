#ifndef CC_GUI_H
#define CC_GUI_H
#include "Input.h"
#include "Texture.h"
#include "GameStructs.h"
/* Describes and manages 2D GUI elements on screen.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct VertexP3fT2fC4b_ VertexP3fT2fC4b;

#define ANCHOR_MIN 0    /* Left or top */
#define ANCHOR_CENTRE 1 /* Middle */
#define ANCHOR_MAX 2    /* Bottom or right */

typedef struct GuiElement_ GuiElement;
typedef struct GuiElementVTABLE_ {
	void (*Init)(GuiElement* elem);
	void (*Render)(GuiElement* elem, Real64 delta);
	void (*Free)(GuiElement* elem);
	/* Recreates all sub-elements and/or textures. (e.g. for when bitmap font changes) */
	void (*Recreate)(GuiElement* elem);
	bool (*HandlesKeyDown)(GuiElement* elem, Key key);
	bool (*HandlesKeyUp)(GuiElement* elem, Key key);
	bool (*HandlesKeyPress)(GuiElement* elem, UInt8 keyChar);
	bool (*HandlesMouseDown)(GuiElement* elem, Int32 x, Int32 y, MouseButton btn);
	bool (*HandlesMouseUp)(GuiElement* elem, Int32 x, Int32 y, MouseButton btn);
	bool (*HandlesMouseMove)(GuiElement* elem, Int32 x, Int32 y);
	bool (*HandlesMouseScroll)(GuiElement* elem, Real32 delta);
} GuiElementVTABLE;

typedef struct GuiElement_ { GuiElementVTABLE* VTABLE; } GuiElement;
void GuiElement_Reset(GuiElement* elem);

/*
	HandlesAllInput; / Whether this screen handles all input. Prevents user interacting with the world
	BlocksWorld;     / Whether this screen completely and opaquely covers the game world behind it
	HidesHUD;        / Whether this screen hides the normal in-game HUD
	RenderHUDOver;   / Whether the normal in-game HUD should be drawn over the top of this screen */
#define Screen_Layout GuiElementVTABLE* VTABLE; bool HandlesAllInput, BlocksWorld; \
bool HidesHUD, RenderHUDOver; void (*OnResize)(GuiElement* elem);

/* Represents a container of widgets and other 2D elements. May cover entire window. */
typedef struct Screen_ { Screen_Layout } Screen;
void Screen_Reset(Screen* screen);


/*
	X, Y, Width, Height;  / Top left corner, and dimensions, of this widget 
	Active;               / Whether this widget is currently being moused over
	Disabled;             / Whether widget is prevented from being interacted with
	HorAnchor, VerAnchor; / Specifies the reference point for when this widget is resized
	XOffset, YOffset;     / Offset from the reference point */
#define Widget_Layout GuiElementVTABLE* VTABLE; Int32 X, Y, Width, Height; bool Active, Disabled; \
UInt8 HorAnchor, VerAnchor; Int32 XOffset, YOffset; void (*Reposition)(GuiElement* elem);

/* Represents an individual 2D gui component. */
typedef struct Widget_ { Widget_Layout } Widget;
void Widget_DoReposition(GuiElement* elem);
void Widget_Init(Widget* widget);
bool Widget_Contains(Widget* widget, Int32 x, Int32 y);

GfxResourceID Gui_GuiTex, Gui_GuiClassicTex, Gui_IconsTex;
Screen* Gui_HUD;
Screen* Gui_Active;
#define GUI_MAX_OVERLAYS 40
Screen* Gui_Overlays[GUI_MAX_OVERLAYS];
Int32 Gui_OverlaysCount;

Int32 Gui_CalcPos(UInt8 anchor, Int32 offset, Int32 size, Int32 axisLen);
bool Gui_Contains(Int32 recX, Int32 recY, Int32 width, Int32 height, Int32 x, Int32 y);
IGameComponent Gui_MakeGameComponent(void);
/* Gets the screen that the user is currently interacting with.
This means if an overlay is active, it will be over the top of other screens. */
Screen* Gui_GetActiveScreen(void);
/* Gets the non-overlay screen that the user is currently interacting with.
This means if an overlay is active, it will return the screen under it. */
Screen* Gui_GetUnderlyingScreen(void);
void Gui_SetScreen(Screen* screen, bool freeOld);
void Gui_SetNewScreen(Screen* screen);
void Gui_RefreshHud(void);
void Gui_ShowOverlay(Screen* overlay, bool atFront);
void Gui_RenderGui(Real64 delta);
void Gui_OnResize(void);


#define TEXTATLAS_MAX_WIDTHS 16
typedef struct TextAtlas_ {
	Texture Tex;
	Int32 Widths[TEXTATLAS_MAX_WIDTHS];
	Int32 Offset, CurX, TotalWidth, FontSize;
} TextAtlas;
void TextAtlas_Make(TextAtlas* atlas, STRING_PURE String* chars, FontDesc* font, STRING_PURE String* prefix);
void TextAtlas_Free(TextAtlas* atlas);
void TextAtlas_Add(TextAtlas* atlas, Int32 charI, VertexP3fT2fC4b** vertices);
void TextAtlas_AddInt(TextAtlas* atlas, Int32 value, VertexP3fT2fC4b** vertices);


#define Widget_Reposition(widget) (widget)->Reposition((GuiElement*)(widget));
#define Elem_Init(elem)           (elem)->VTABLE->Init((GuiElement*)(elem))
#define Elem_Render(elem, delta)  (elem)->VTABLE->Render((GuiElement*)(elem), delta)
#define Elem_Free(elem)           (elem)->VTABLE->Free((GuiElement*)(elem))
#define Elem_Recreate(elem)       (elem)->VTABLE->Recreate((GuiElement*)(elem))
#define Elem_HandlesKeyPress(elem, key) (elem)->VTABLE->HandlesKeyPress((GuiElement*)(elem), key)
#define Elem_HandlesKeyDown(elem, key)  (elem)->VTABLE->HandlesKeyDown((GuiElement*)(elem), key)
#define Elem_HandlesKeyUp(elem, key)    (elem)->VTABLE->HandlesKeyUp((GuiElement*)(elem), key)
#define Elem_HandlesMouseDown(elem, x, y, btn) (elem)->VTABLE->HandlesMouseDown((GuiElement*)(elem), x, y, btn)
#define Elem_HandlesMouseUp(elem, x, y, btn)   (elem)->VTABLE->HandlesMouseUp((GuiElement*)(elem), x, y, btn)
#define Elem_HandlesMouseMove(elem, x, y)      (elem)->VTABLE->HandlesMouseMove((GuiElement*)(elem), x, y)
#define Elem_HandlesMouseScroll(elem, delta)   (elem)->VTABLE->HandlesMouseScroll((GuiElement*)(elem), delta)
#endif