#ifndef CC_GUI_H
#define CC_GUI_H
#include "Input.h"
#include "Texture.h"
#include "VertexStructs.h"
/* Describes and manages 2D GUI elements on screen.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define ANCHOR_MIN 0    /* Left or top */
#define ANCHOR_CENTRE 1 /* Middle */
#define ANCHOR_MAX 2    /* Bottom or right */

struct IGameComponent;
struct GuiElem;
struct GuiElementVTABLE {
	void (*Init)(struct GuiElem* elem);
	void (*Render)(struct GuiElem* elem, Real64 delta);
	void (*Free)(struct GuiElem* elem);
	/* Recreates all sub-elements and/or textures. (e.g. for when bitmap font changes) */
	void (*Recreate)(struct GuiElem* elem);
	bool (*HandlesKeyDown)(struct GuiElem* elem, Key key);
	bool (*HandlesKeyUp)(struct GuiElem* elem, Key key);
	bool (*HandlesKeyPress)(struct GuiElem* elem, UInt8 keyChar);
	bool (*HandlesMouseDown)(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn);
	bool (*HandlesMouseUp)(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn);
	bool (*HandlesMouseMove)(struct GuiElem* elem, Int32 x, Int32 y);
	bool (*HandlesMouseScroll)(struct GuiElem* elem, Real32 delta);
};

struct GuiElem { struct GuiElementVTABLE* VTABLE; };
void GuiElement_Reset(struct GuiElem* elem);


#define Screen_Layout struct GuiElementVTABLE* VTABLE; \
bool HandlesAllInput; /* Whether this screen handles all input. Prevents user interacting with the world */ \
bool BlocksWorld;     /* Whether this screen completely and opaquely covers the game world behind it */ \
bool HidesHUD;        /* Whether this screen hides the normal in-game HUD */ \
bool RenderHUDOver;   /* Whether the normal in-game HUD should be drawn over the top of this screen */ \
void (*OnResize)(struct GuiElem* elem);

/* Represents a container of widgets and other 2D elements. May cover entire window. */
struct Screen { Screen_Layout };
void Screen_Reset(struct Screen* screen);


typedef void (*Widget_LeftClick)(struct GuiElem* screenElem, struct GuiElem* widget);
#define Widget_Layout struct GuiElementVTABLE* VTABLE; \
Int32 X, Y, Width, Height;  /* Top left corner, and dimensions, of this widget */ \
bool Active;                /* Whether this widget is currently being moused over*/ \
bool Disabled;              /* Whether widget is prevented from being interacted with */ \
UInt8 HorAnchor, VerAnchor; /* Specifies the reference point for when this widget is resized */ \
Int32 XOffset, YOffset;     /* Offset from the reference point */ \
void (*Reposition)(struct GuiElem* elem); \
Widget_LeftClick MenuClick;

/* Represents an individual 2D gui component. */
struct Widget { Widget_Layout };
void Widget_DoReposition(struct GuiElem* elem);
void Widget_Init(struct Widget* widget);
bool Widget_Contains(struct Widget* widget, Int32 x, Int32 y);


GfxResourceID Gui_GuiTex, Gui_GuiClassicTex, Gui_IconsTex;
struct Screen* Gui_HUD;
struct Screen* Gui_Active;
#define GUI_MAX_OVERLAYS 6
struct Screen* Gui_Overlays[GUI_MAX_OVERLAYS];
Int32 Gui_OverlaysCount;

Int32 Gui_CalcPos(UInt8 anchor, Int32 offset, Int32 size, Int32 axisLen);
bool Gui_Contains(Int32 recX, Int32 recY, Int32 width, Int32 height, Int32 x, Int32 y);
void Gui_MakeComponent(struct IGameComponent* comp);
/* Gets the screen that the user is currently interacting with.
This means if an overlay is active, it will be over the top of other screens. */
struct Screen* Gui_GetActiveScreen(void);
/* Gets the non-overlay screen that the user is currently interacting with.
This means if an overlay is active, it will return the screen under it. */
struct Screen* Gui_GetUnderlyingScreen(void);

void Gui_ReplaceActive(struct Screen* screen);
void Gui_FreeActive(void);
/* This doesn't free old active screen - you probably want Gui_ReplaceActive */
void Gui_SetActive(struct Screen* screen);
void Gui_RefreshHud(void);
void Gui_ShowOverlay(struct Screen* overlay, bool atFront);
Int32 Gui_IndexOverlay(struct Screen* overlay);
void Gui_FreeOverlay(struct Screen* overlay);
void Gui_RenderGui(Real64 delta);
void Gui_OnResize(void);


#define TEXTATLAS_MAX_WIDTHS 16
struct TextAtlas {
	struct Texture Tex;
	Int32 Offset, CurX, FontSize;
	Real32 uScale;
	Int32 Widths[TEXTATLAS_MAX_WIDTHS];
};
void TextAtlas_Make(struct TextAtlas* atlas, STRING_PURE String* chars, struct FontDesc* font, STRING_PURE String* prefix);
void TextAtlas_Free(struct TextAtlas* atlas);
void TextAtlas_Add(struct TextAtlas* atlas, Int32 charI, VertexP3fT2fC4b** vertices);
void TextAtlas_AddInt(struct TextAtlas* atlas, Int32 value, VertexP3fT2fC4b** vertices);


#define Elem_Init(elem)           (elem)->VTABLE->Init((struct GuiElem*)(elem))
#define Elem_Render(elem, delta)  (elem)->VTABLE->Render((struct GuiElem*)(elem), delta)
#define Elem_Free(elem)           (elem)->VTABLE->Free((struct GuiElem*)(elem))
#define Elem_Recreate(elem)       (elem)->VTABLE->Recreate((struct GuiElem*)(elem))
#define Elem_HandlesKeyPress(elem, key) (elem)->VTABLE->HandlesKeyPress((struct GuiElem*)(elem), key)
#define Elem_HandlesKeyDown(elem, key)  (elem)->VTABLE->HandlesKeyDown((struct GuiElem*)(elem), key)
#define Elem_HandlesKeyUp(elem, key)    (elem)->VTABLE->HandlesKeyUp((struct GuiElem*)(elem), key)
#define Elem_HandlesMouseDown(elem, x, y, btn) (elem)->VTABLE->HandlesMouseDown((struct GuiElem*)(elem), x, y, btn)
#define Elem_HandlesMouseUp(elem, x, y, btn)   (elem)->VTABLE->HandlesMouseUp((struct GuiElem*)(elem), x, y, btn)
#define Elem_HandlesMouseMove(elem, x, y)      (elem)->VTABLE->HandlesMouseMove((struct GuiElem*)(elem), x, y)
#define Elem_HandlesMouseScroll(elem, delta)   (elem)->VTABLE->HandlesMouseScroll((struct GuiElem*)(elem), delta)

#define Widget_Reposition(widget) (widget)->Reposition((struct GuiElem*)(widget));
#define Elem_TryFree(elem)        if ((elem)->VTABLE) { Elem_Free(elem); }
#endif
