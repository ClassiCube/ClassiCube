#ifndef CC_GUI_H
#define CC_GUI_H
#include "Input.h"
#include "Event.h"
#include "VertexStructs.h"
/* Describes and manages 2D GUI elements on screen.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

enum GuiAnchor {
	ANCHOR_MIN,        /* = offset */
	ANCHOR_CENTRE,     /* = (axis/2) - (size/2) - offset; */
	ANCHOR_MAX,        /* = axis - size - offset */
	ANCHOR_CENTRE_MIN, /* = (axis/2) + offset */
	ANCHOR_CENTRE_MAX  /* = (axis/2) - size - offset */
};

struct IGameComponent;
struct GuiElem;
extern struct IGameComponent Gui_Component;

/* Whether vanilla Minecraft Classic gui texture is used. */
extern bool Gui_ClassicTexture;
/* Whether tab list is laid out like vanilla Minecraft Classic. */
extern bool Gui_ClassicTabList;
/* Whether menus are laid out like vanilla Minecraft Classic. */
extern bool Gui_ClassicMenu;
/* Maximum number of visible chatlines on screen. Can be 0. */
extern int  Gui_Chatlines;
/* Whether clicking on a chatline inserts it into chat input. */
extern bool Gui_ClickableChat;
/* Whether pressing tab in chat input attempts to autocomplete player names. */
extern bool Gui_TabAutocomplete;
/* Whether FPS counter (and other info) is shown in top left. */
extern bool Gui_ShowFPS;

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
	bool grabsInput;  /* Whether this screen grabs input. Causes the cursor to become visible. */ \
	bool blocksWorld; /* Whether this screen completely and opaquely covers the game world behind it. */ \
	bool hidden;      /* Whether this screen is prevented from rendering. */ \
	bool closable;    /* Whether this screen is automatically closed when pressing Escape */

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
	int x, y, width, height;      /* Top left corner, and dimensions, of this widget */ \
	bool active;                  /* Whether this widget is currently being moused over */ \
	bool disabled;                /* Whether widget is prevented from being interacted with */ \
	uint8_t horAnchor, verAnchor; /* The reference point for when this widget is resized */ \
	int xOffset, yOffset;         /* Offset from the reference point */ \
	Widget_LeftClick MenuClick;

/* Represents an individual 2D gui component. */
struct Widget { Widget_Layout };
void Widget_SetLocation(void* widget, uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset);
void Widget_CalcPosition(void* widget);
/* Resets Widget struct fields to 0/NULL (except VTABLE) */
void Widget_Reset(void* widget);
/* Whether the given point is located within the bounds of the widget. */
bool Widget_Contains(void* widget, int x, int y);


extern GfxResourceID Gui_GuiTex, Gui_GuiClassicTex, Gui_IconsTex;

/* Higher priority handles input first and draws on top */
enum GuiPriority {
	GUI_PRIORITY_DISCONNECT = 45,
	GUI_PRIORITY_STATUS     = 40,
	GUI_PRIORITY_URLWARNING = 35,
	GUI_PRIORITY_TEXPACK    = 30,
	GUI_PRIORITY_TEXIDS     = 25,
	GUI_PRIORITY_MENU       = 20,
	GUI_PRIORITY_INVENTORY  = 15,
	GUI_PRIORITY_HUD        = 10,
	GUI_PRIORITY_LOADING    =  5,
};

extern struct Screen* Gui_Status;
extern struct Screen* Gui_HUD;
extern struct Screen* Gui_Active;
#define GUI_MAX_SCREENS 10
#define GUI_MAX_OVERLAYS 4
extern struct Screen* Gui_Overlays[GUI_MAX_OVERLAYS];
extern struct Screen* Gui_Screens[GUI_MAX_SCREENS];
extern int Gui_ScreensCount;
extern int Gui_OverlaysCount;

/* Calculates position of an element on a particular axis */
/* For example, to calculate X position of a text widget on screen */
int  Gui_CalcPos(uint8_t anchor, int offset, int size, int axisLen);
/* Returns whether the given rectangle contains the given point. */
bool Gui_Contains(int recX, int recY, int width, int height, int x, int y);
/* Gets the screen that the user is currently interacting with. */
/* This means if an overlay is active, it will be over the top of other screens. */
struct Screen* Gui_GetActiveScreen(void);
/* Gets the non-overlay screen that the user is currently interacting with. */
/* This means if an overlay is active, the screen under it is returned. */
struct Screen* Gui_GetUnderlyingScreen(void);

/* Frees the active screen if it is not NULL. */
/* NOTE: You should usually use Gui_CloseActive instead. */
CC_NOINLINE void Gui_FreeActive(void);
/* Sets the active screen/menu that the user interacts with. */
/* NOTE: This doesn't free old active screen - must call Gui_FreeActive() first */
CC_NOINLINE void Gui_SetActive(struct Screen* screen);
/* Shortcut for Gui_Close(Gui_Active) */
CC_NOINLINE void Gui_CloseActive(void);
/* Frees the given screen, and if == Gui_Active, calls Gui_SetActive(NULL) */
CC_NOINLINE void Gui_Close(void* screen);

/* Returns index of the given screen in the screens list, -1 if not */
int Gui_Index(struct Screen* screen);
/* Inserts a screen into the screen lists with the given priority. */
/* NOTE: You MUST ensure a screen isn't added twice. Or use Gui_Replace. */
void Gui_Add(struct Screen* screen, int priority);
/* Removes the screen from the screens list. */
void Gui_Remove(struct Screen* screen);
/* Shorthand for Gui_Remove then Gui_Add. */
void Gui_Replace(struct Screen* screen, int priority);

/* Returns highest priority screen that has grabbed input. */
struct Screen* Gui_GetInputGrab(void);
/* Returns highest priority screen that blocks world rendering. */
struct Screen* Gui_GetBlocksWorld(void);
/* Returns highest priority screen that is closable. */
struct Screen* Gui_GetClosable(void);

void Gui_Refresh(void);
void Gui_RefreshHud(void);
void Gui_ShowOverlay(struct Screen* screen);
/* Returns index of the given screen in the overlays list, -1 if not */
int  Gui_IndexOverlay(const void* screen);
/* Removes given screen from the overlays list */
void Gui_RemoveOverlay(const void* screen);
void Gui_RenderGui(double delta);
void Gui_OnResize(void);

#define TEXTATLAS_MAX_WIDTHS 16
struct TextAtlas {
	struct Texture tex;
	int offset, curX;
	float uScale;
	int16_t widths[TEXTATLAS_MAX_WIDTHS];
	int16_t offsets[TEXTATLAS_MAX_WIDTHS];
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
