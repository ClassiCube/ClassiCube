#ifndef CC_GUI_H
#define CC_GUI_H
#include "Input.h"
#include "Event.h"
#include "VertexStructs.h"
/* Describes and manages 2D GUI elements on screen.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

enum GuiAnchor {
	ANCHOR_MIN,    /* Left or top */
	ANCHOR_CENTRE, /* Middle */
	ANCHOR_MAX     /* Bottom or right */
};

struct IGameComponent;
struct GuiElem;
extern struct IGameComponent Gui_Component;

#define GuiElemVTABLE_Layout() \
	void (*Init)(void* elem); \
	void (*Render)(void* elem, double delta); \
	void (*Free)(void* elem); \
	void (*Recreate)(void* elem); \
	bool (*HandlesKeyDown)(void* elem, Key key); \
	bool (*HandlesKeyUp)(void* elem, Key key); \
	bool (*HandlesKeyPress)(void* elem, char keyChar); \
	bool (*HandlesMouseDown)(void* elem, int x, int y, MouseButton btn); \
	bool (*HandlesMouseUp)(void* elem, int x, int y, MouseButton btn); \
	bool (*HandlesMouseMove)(void* elem, int x, int y); \
	bool (*HandlesMouseScroll)(void* elem, float delta);

struct GuiElemVTABLE { GuiElemVTABLE_Layout() };
struct GuiElem { struct GuiElemVTABLE* VTABLE; };
void Gui_DefaultRecreate(void* elem);


struct ScreenVTABLE {
	GuiElemVTABLE_Layout()
	void (*OnResize)(void* elem);
	Event_Void_Callback ContextLost;
	Event_Void_Callback ContextRecreated;
};
#define Screen_Layout struct ScreenVTABLE* VTABLE; \
	bool HandlesAllInput; /* Whether this screen handles all input. Prevents user interacting with the world */ \
	bool BlocksWorld;     /* Whether this screen completely and opaquely covers the game world behind it */ \
	bool HidesHUD;        /* Whether this screen hides the normal in-game HUD */ \
	bool RenderHUDOver;   /* Whether the normal in-game HUD should be drawn over the top of this screen */

/* Represents a container of widgets and other 2D elements. May cover entire window. */
struct Screen { Screen_Layout };
void Screen_CommonInit(void* screen);
void Screen_CommonFree(void* screen);


typedef void (*Widget_LeftClick)(void* screen, void* widget);
struct Widget;
struct WidgetVTABLE {
	GuiElemVTABLE_Layout()
	void (*Reposition)(void* elem);
};
#define Widget_Layout struct WidgetVTABLE* VTABLE; \
	int X, Y, Width, Height;      /* Top left corner, and dimensions, of this widget */ \
	bool Active;                  /* Whether this widget is currently being moused over*/ \
	bool Disabled;                /* Whether widget is prevented from being interacted with */ \
	uint8_t HorAnchor, VerAnchor; /* Specifies the reference point for when this widget is resized */ \
	int XOffset, YOffset;         /* Offset from the reference point */ \
	Widget_LeftClick MenuClick;

/* Represents an individual 2D gui component. */
struct Widget { Widget_Layout };
void Widget_SetLocation(void* widget, uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset);
void Widget_CalcPosition(void* widget);
void Widget_Reset(void* widget);
/* Whether the given point is located within the bounds of the widget. */
bool Widget_Contains(void* widget, int x, int y);


GfxResourceID Gui_GuiTex, Gui_GuiClassicTex, Gui_IconsTex;
struct Screen* Gui_HUD;
struct Screen* Gui_Active;
#define GUI_MAX_OVERLAYS 6
struct Screen* Gui_Overlays[GUI_MAX_OVERLAYS];
int Gui_OverlaysCount;

int  Gui_CalcPos(uint8_t anchor, int offset, int size, int axisLen);
bool Gui_Contains(int recX, int recY, int width, int height, int x, int y);
void Gui_MakeComponent(struct IGameComponent* comp);
/* Gets the screen that the user is currently interacting with.
This means if an overlay is active, it will be over the top of other screens. */
struct Screen* Gui_GetActiveScreen(void);
/* Gets the non-overlay screen that the user is currently interacting with.
This means if an overlay is active, it will return the screen under it. */
struct Screen* Gui_GetUnderlyingScreen(void);

CC_NOINLINE void Gui_FreeActive(void);
/* NOTE: This doesn't free old active screen - must call Gui_FreeActive() first */
CC_NOINLINE void Gui_SetActive(struct Screen* screen);
/* NOTE: Same as Gui_FreeActive(); Gui_SetActive(NULL); */
CC_NOINLINE void Gui_CloseActive(void);

void Gui_RefreshHud(void);
void Gui_ShowOverlay(struct Screen* overlay, bool atFront);
int  Gui_IndexOverlay(const void* overlay);
void Gui_FreeOverlay(void* overlay);
void Gui_RenderGui(double delta);
void Gui_OnResize(void);
void Gui_CalcCursorVisible(void);

#define TEXTATLAS_MAX_WIDTHS 16
struct TextAtlas {
	struct Texture Tex;
	int Offset, CurX, FontSize;
	float uScale;
	int16_t Widths[TEXTATLAS_MAX_WIDTHS];
};
void TextAtlas_Make(struct TextAtlas* atlas, const String* chars, const FontDesc* font, const String* prefix);
void TextAtlas_Free(struct TextAtlas* atlas);
void TextAtlas_Add(struct TextAtlas* atlas, int charI, VertexP3fT2fC4b** vertices);
void TextAtlas_AddInt(struct TextAtlas* atlas, int value, VertexP3fT2fC4b** vertices);


#define Elem_Init(elem)           (elem)->VTABLE->Init(elem)
#define Elem_Render(elem, delta)  (elem)->VTABLE->Render(elem, delta)
#define Elem_Free(elem)           (elem)->VTABLE->Free(elem)
#define Elem_Recreate(elem)       (elem)->VTABLE->Recreate(elem)
#define Elem_HandlesKeyPress(elem, key) (elem)->VTABLE->HandlesKeyPress(elem, key)
#define Elem_HandlesKeyDown(elem, key)  (elem)->VTABLE->HandlesKeyDown(elem, key)
#define Elem_HandlesKeyUp(elem, key)    (elem)->VTABLE->HandlesKeyUp(elem, key)
#define Elem_HandlesMouseDown(elem, x, y, btn) (elem)->VTABLE->HandlesMouseDown(elem, x, y, btn)
#define Elem_HandlesMouseUp(elem, x, y, btn)   (elem)->VTABLE->HandlesMouseUp(elem, x, y, btn)
#define Elem_HandlesMouseMove(elem, x, y)      (elem)->VTABLE->HandlesMouseMove(elem, x, y)
#define Elem_HandlesMouseScroll(elem, delta)   (elem)->VTABLE->HandlesMouseScroll(elem, delta)

#define Screen_OnResize(screen)   (screen)->VTABLE->OnResize(screen);
#define Widget_Reposition(widget) (widget)->VTABLE->Reposition(widget);
#define Elem_TryFree(elem)        if ((elem)->VTABLE) { Elem_Free(elem); }
#endif
