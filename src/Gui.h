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
struct FontDesc;
struct Widget;
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

struct ScreenVTABLE {
	void (*Init)(void* elem);
	void (*Render)(void* elem, double delta);
	void (*Free)(void* elem);
	bool (*HandlesKeyDown)(void* elem, Key key);
	bool (*HandlesKeyUp)(void* elem, Key key);
	bool (*HandlesKeyPress)(void* elem, char keyChar);
	bool (*HandlesMouseDown)(void* elem, int x, int y, MouseButton btn);
	bool (*HandlesMouseUp)(void* elem, int x, int y, MouseButton btn);
	bool (*HandlesMouseMove)(void* elem, int x, int y);
	bool (*HandlesMouseScroll)(void* elem, float delta);
	void (*OnResize)(void* elem);
	Event_Void_Callback ContextLost;
	Event_Void_Callback ContextRecreated;
};
#define Screen_Layout const struct ScreenVTABLE* VTABLE; \
	bool grabsInput;  /* Whether this screen grabs input. Causes the cursor to become visible. */ \
	bool blocksWorld; /* Whether this screen completely and opaquely covers the game world behind it. */ \
	bool closable;    /* Whether this screen is automatically closed when pressing Escape */ \
	struct Widget** widgets; int numWidgets;

/* Represents a container of widgets and other 2D elements. May cover entire window. */
struct Screen { Screen_Layout };

typedef void (*Widget_LeftClick)(void* screen, void* widget);
struct WidgetVTABLE {
	void (*Init)(void* elem);
	void (*Render)(void* elem, double delta);
	void (*Free)(void* elem);
	bool (*HandlesKeyDown)(void* elem, Key key);
	bool (*HandlesKeyUp)(void* elem, Key key);
	bool (*HandlesMouseDown)(void* elem, int x, int y, MouseButton btn);
	bool (*HandlesMouseUp)(void* elem, int x, int y, MouseButton btn);
	bool (*HandlesMouseMove)(void* elem, int x, int y);
	bool (*HandlesMouseScroll)(void* elem, float delta);
	void (*Reposition)(void* elem);
};
#define Widget_Layout const struct WidgetVTABLE* VTABLE; \
	int x, y, width, height;      /* Top left corner, and dimensions, of this widget */ \
	bool active;                  /* Whether this widget is currently being moused over */ \
	bool disabled;                /* Whether widget is prevented from being interacted with */ \
	cc_uint8 horAnchor, verAnchor; /* The reference point for when this widget is resized */ \
	int xOffset, yOffset;         /* Offset from the reference point */ \
	Widget_LeftClick MenuClick;

/* Represents an individual 2D gui component. */
struct Widget { Widget_Layout };
void Widget_SetLocation(void* widget, cc_uint8 horAnchor, cc_uint8 verAnchor, int xOffset, int yOffset);
void Widget_CalcPosition(void* widget);
/* Resets Widget struct fields to 0/NULL (except VTABLE) */
void Widget_Reset(void* widget);
/* Whether the given point is located within the bounds of the widget. */
bool Widget_Contains(void* widget, int x, int y);


extern GfxResourceID Gui_GuiTex, Gui_GuiClassicTex, Gui_IconsTex;

/* Higher priority handles input first and draws on top */
enum GuiPriority {
	GUI_PRIORITY_DISCONNECT = 50,
	GUI_PRIORITY_OLDLOADING = 45,
	GUI_PRIORITY_URLWARNING = 40,
	GUI_PRIORITY_TEXPACK    = 35,
	GUI_PRIORITY_TEXIDS     = 30,
	GUI_PRIORITY_MENU       = 25,
	GUI_PRIORITY_INVENTORY  = 20,
	GUI_PRIORITY_STATUS     = 15,
	GUI_PRIORITY_HUD        = 10,
	GUI_PRIORITY_LOADING    =  5,
};

extern struct Screen* Gui_Status;
extern struct Screen* Gui_HUD;
#define GUI_MAX_SCREENS 10
extern struct Screen* Gui_Screens[GUI_MAX_SCREENS];
extern int Gui_ScreensCount;

/* Calculates position of an element on a particular axis */
/* For example, to calculate X position of a text widget on screen */
int  Gui_CalcPos(cc_uint8 anchor, int offset, int size, int axisLen);
/* Returns whether the given rectangle contains the given point. */
bool Gui_Contains(int recX, int recY, int width, int height, int x, int y);
/* Shows HUD and Status screens. */
void Gui_ShowDefault(void);

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

void Gui_RefreshAll(void);
void Gui_RefreshHud(void);
void Gui_Refresh(struct Screen* s);

void Gui_RenderGui(double delta);
void Gui_OnResize(void);

#define TEXTATLAS_MAX_WIDTHS 16
struct TextAtlas {
	struct Texture tex;
	int offset, curX;
	float uScale;
	short widths[TEXTATLAS_MAX_WIDTHS];
	short offsets[TEXTATLAS_MAX_WIDTHS];
};
void TextAtlas_Make(struct TextAtlas* atlas, const String* chars, const struct FontDesc* font, const String* prefix);
void TextAtlas_Free(struct TextAtlas* atlas);
void TextAtlas_Add(struct TextAtlas* atlas, int charI, VertexP3fT2fC4b** vertices);
void TextAtlas_AddInt(struct TextAtlas* atlas, int value, VertexP3fT2fC4b** vertices);


#define Elem_Init(elem)          (elem)->VTABLE->Init(elem)
#define Elem_Render(elem, delta) (elem)->VTABLE->Render(elem, delta)
#define Elem_Free(elem)          (elem)->VTABLE->Free(elem)
#define Elem_HandlesKeyPress(elem, key) (elem)->VTABLE->HandlesKeyPress(elem, key)
#define Elem_HandlesKeyDown(elem, key)  (elem)->VTABLE->HandlesKeyDown(elem, key)
#define Elem_HandlesKeyUp(elem, key)    (elem)->VTABLE->HandlesKeyUp(elem, key)
#define Elem_HandlesMouseDown(elem, x, y, btn) (elem)->VTABLE->HandlesMouseDown(elem, x, y, btn)
#define Elem_HandlesMouseUp(elem, x, y, btn)   (elem)->VTABLE->HandlesMouseUp(elem, x, y, btn)
#define Elem_HandlesMouseMove(elem, x, y)      (elem)->VTABLE->HandlesMouseMove(elem, x, y)
#define Elem_HandlesMouseScroll(elem, delta)   (elem)->VTABLE->HandlesMouseScroll(elem, delta)

#define Widget_Reposition(widget) (widget)->VTABLE->Reposition(widget);
#define Elem_TryFree(elem)        if ((elem)->VTABLE) { Elem_Free(elem); }
#endif
