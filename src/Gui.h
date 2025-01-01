#ifndef CC_GUI_H
#define CC_GUI_H
#include "Core.h"
#include "PackedCol.h"

CC_BEGIN_HEADER

/* Describes and manages 2D GUI elements on screen.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

enum GuiAnchor {
	ANCHOR_MIN,        /* = offset */
	ANCHOR_CENTRE,     /* = (axis/2) - (size/2) - offset; */
	ANCHOR_MAX,        /* = axis - size - offset */
	ANCHOR_CENTRE_MIN, /* = (axis/2) + offset */
	ANCHOR_CENTRE_MAX  /* = (axis/2) - size - offset */
};

struct IGameComponent;
struct VertexTextured;
struct FontDesc;
struct Widget;
struct InputDevice;
extern struct IGameComponent Gui_Component;

CC_VAR extern struct _GuiData {
	/* The list of screens currently shown. */
	struct Screen** Screens;
	/* The number of screens currently shown. */
	int ScreensCount;
	/* Whether vanilla Minecraft Classic gui texture is used. */
	cc_bool ClassicTexture;
	/* Whether tab list is laid out like vanilla Minecraft Classic. */
	cc_bool ClassicTabList;
	/* Whether menus are laid out like vanilla Minecraft Classic. */
	cc_bool ClassicMenu;
	/* Whether classic-style chat screen is used */
	cc_bool ClassicChat;
	/* Maximum number of visible chatlines on screen. Can be 0. */
	int     Chatlines;
	/* Whether clicking on a chatline inserts it into chat input. */
	cc_bool ClickableChat;
	/* Whether pressing tab in chat input attempts to autocomplete player names. */
	cc_bool TabAutocomplete;
	/* Whether FPS counter (and other info) is shown in top left. */
	cc_bool ShowFPS;
	/* Whether classic-style inventory is used */
	cc_bool ClassicInventory;
	float RawHotbarScale, RawChatScale, RawInventoryScale, RawCrosshairScale;
	GfxResourceID GuiTex, GuiClassicTex, IconsTex, TouchTex;
	int DefaultLines;
	int _unused;
	float RawTouchScale;
	/* The highest priority screen that has grabbed input. */
	struct Screen* InputGrab;
	/* Whether chat automatically scales based on window size. */
	cc_bool AutoScaleChat;
	/* Whether the touch UI is currently being displayed. */
	cc_bool TouchUI;
	/* Whether the first person crosshair should be hidden. */
	cc_bool HideCrosshair;
	/* Whether the player hand/block model should be hidden. */
	cc_bool HideHand;
	/* Whether the hotbar should be hidden. */
	cc_bool HideHotbar;
	/* The height of the cinematic bars, where 0 = no bars visible and 1 = bars completely cover the screen. */
	float BarSize;
	/* The color of the cinematic bars, if enabled. */
	PackedCol CinematicBarColor;
} Gui;

#ifdef CC_BUILD_TOUCH
#define Gui_TouchUI Gui.TouchUI
#else
#define Gui_TouchUI false
#endif

float Gui_Scale(float value);
float Gui_GetHotbarScale(void);
float Gui_GetInventoryScale(void);
float Gui_GetChatScale(void);
float Gui_GetCrosshairScale(void);

CC_NOINLINE void Gui_MakeTitleFont(struct FontDesc* font);
CC_NOINLINE void Gui_MakeBodyFont(struct FontDesc* font);

/* Functions for a Screen instance. */
struct ScreenVTABLE {
	/* Initialises persistent state. */
	void (*Init)(void* elem);
	/* Updates this screen, called every frame just before Render(). */
	void (*Update)(void* elem, float delta);
	/* Frees/releases persistent state. */
	void (*Free)(void* elem);
	/* Draws this screen and its widgets on screen. */
	void (*Render)(void* elem, float delta);
	/* Builds the vertex mesh for all the widgets in the screen. */
	void (*BuildMesh)(void* elem);
	/* Returns non-zero if an input press is handled. */
	int  (*HandlesInputDown)(void* elem, int key, struct InputDevice* device);
	/* Called when an input key or button is released */
	void (*OnInputUp)(void* elem, int key, struct InputDevice* device);
	/* Returns non-zero if a key character press is handled. */
	int  (*HandlesKeyPress)(void* elem, char keyChar);
	/* Returns non-zero if on-screen keyboard text changed is handled. */
	int  (*HandlesTextChanged)(void* elem, const cc_string* str);
	/* Returns non-zero if a pointer press is handled. */
	int  (*HandlesPointerDown)(void* elem, int id, int x, int y);
	/* Called when a pointer is released. */
	void (*OnPointerUp)(void* elem,   int id, int x, int y);
	/* Returns non-zero if a pointer movement is handled. */
	int  (*HandlesPointerMove)(void* elem, int id, int x, int y);
	/* Returns non-zero if a mouse wheel scroll is handled. */
	int  (*HandlesMouseScroll)(void* elem, float delta);
	/* Positions widgets on screen. Typically called on window resize. */
	void (*Layout)(void* elem);
	/* Destroys graphics resources. (textures, vertex buffers, etc) */
	void (*ContextLost)(void* elem);
	/* Allocates graphics resources. (textures, vertex buffers, etc) */
	void (*ContextRecreated)(void* elem);
	/* Returns non-zero if a pad axis update is handled. */
	int (*HandlesPadAxis)(void* elem, int axis, float x, float y);
};
#define Screen_Body const struct ScreenVTABLE* VTABLE; \
	cc_bool grabsInput;  /* Whether this screen grabs input. Causes the cursor to become visible. */ \
	cc_bool blocksWorld; /* Whether this screen completely and opaquely covers the game world behind it. */ \
	cc_bool closable;    /* Whether this screen is automatically closed when pressing Escape */ \
	cc_bool dirty;       /* Whether this screens needs to have its mesh rebuilt. */ \
	int maxVertices; GfxResourceID vb; /* Vertex buffer storing the contents of the screen */ \
	struct Widget** widgets; int numWidgets; /* The widgets/individual elements in the screen */ \
	int selectedI, maxWidgets;

/* Represents a container of widgets and other 2D elements. May cover entire window. */
struct Screen { Screen_Body };
/* Calls Widget_Render2 on each widget in the screen. */
void Screen_Render2Widgets(void* screen, float delta);
void Screen_UpdateVb(void* screen);
struct VertexTextured* Screen_LockVb(void* screen);
int Screen_DoPointerDown(void* screen, int id, int x, int y);
int Screen_CalcDefaultMaxVertices(void* screen);

/* Default mesh building implementation for a screen */
/*  (Locks vb, calls Widget_BuildMesh on each widget, then unlocks vb) */
void Screen_BuildMesh(void* screen);
/* Default layout implementation for a screen */
/*  (Calls Widget_Layout on each widget) */
void Screen_Layout(void* screen);
/* Default context lost implementation for a screen */
/*  (Deletes vb, then calls Elem_Free on each widget) */
void Screen_ContextLost(void* screen);
/* Default input down implementation for a screen */
/*  (returns true if key is NOT a function key) */
int  Screen_InputDown(void* screen, int key, struct InputDevice* device);
/* Default input up implementation for a screen */
/*  (does nothing) */
void Screen_InputUp(void*   screen, int key, struct InputDevice* device);
/* Default pointer release implementation for a screen */
/*  (does nothing) */
void Screen_PointerUp(void* s, int id, int x, int y);


typedef void (*Widget_LeftClick)(void* screen, void* widget);

struct WidgetVTABLE {
	/* Draws this widget on-screen. */
	void (*Render)(void* elem, float delta);
	/* Destroys allocated graphics resources. */
	void (*Free)(void* elem);
	/* Positions this widget on-screen. */
	void (*Reposition)(void* elem);
	/* Returns non-zero if an input press is handled. */
	int (*HandlesKeyDown)(void* elem, int key, struct InputDevice* device);
	/* Called when an input key or button is released. */
	void (*OnInputUp)(void* elem, int key, struct InputDevice* device);
	/* Returns non-zero if a mouse wheel scroll is handled. */
	int (*HandlesMouseScroll)(void* elem, float delta);
	/* Returns non-zero if a pointer press is handled. */
	int (*HandlesPointerDown)(void* elem, int id, int x, int y);
	/* Called when a pointer is released. */
	void (*OnPointerUp)(void* elem, int id, int x, int y);
	/* Returns non-zero if a pointer movement is handled. */
	int (*HandlesPointerMove)(void* elem, int id, int x, int y);
	/* Builds the mesh of vertices for this widget. */
	void (*BuildMesh)(void* elem, struct VertexTextured** vertices);
	/* Draws this widget on-screen. */
	int  (*Render2)(void* elem, int offset);
	/* Returns the maximum number of vertices this widget may use */
	int  (*GetMaxVertices)(void* elem);
	/* Returns non-zero if a pad axis update is handled. */
	int (*HandlesPadAxis)(void* elem, int axis, float x, float y);
};

#define Widget_Body const struct WidgetVTABLE* VTABLE; \
	int x, y, width, height;       /* Top left corner, and dimensions, of this widget */ \
	cc_bool active;                /* Whether this widget is currently being moused over */ \
	cc_uint8 flags;                /* Flags controlling the widget's interactability */ \
	cc_uint8 horAnchor, verAnchor; /* The reference point for when this widget is resized */ \
	int xOffset, yOffset;          /* Offset from the reference point */ \
	Widget_LeftClick MenuClick; \
	cc_pointer meta;

/* Whether a widget is prevented from being interacted with */
#define WIDGET_FLAG_DISABLED   0x01
/* Whether a widget can be selected via up/down */
#define WIDGET_FLAG_SELECTABLE 0x02
/* Whether for dual screen builds, this widget still appears on */
/*  the main game screen instead of the dedicated UI screen */
#define WIDGET_FLAG_MAINSCREEN 0x04
#ifdef CC_BUILD_DUALSCREEN
	#define Window_UI Window_Alt
#else
	#define Window_UI Window_Main
#endif

/* Represents an individual 2D gui component. */
struct Widget { Widget_Body };
void Widget_SetLocation(void* widget, cc_uint8 horAnchor, cc_uint8 verAnchor, int xOffset, int yOffset);
/* Calculates where this widget should be on-screen based on its attributes. */
/* These attributes are width/height, horAnchor/verAnchor, xOffset/yOffset */
void Widget_CalcPosition(void* widget);
/* Resets Widget struct fields to 0/NULL (except VTABLE) */
void Widget_Reset(void* widget);
/* Returns non-zero if the given point is located within the bounds of the widget. */
int Widget_Contains(void* widget, int x, int y);
/* Sets whether the widget is prevented from being interacted with */
void Widget_SetDisabled(void* widget, int disabled);


/* Higher priority handles input first and draws on top */
/* NOTE: Values are 5 apart to allow plugins to insert custom screens */
enum GuiPriority {
	GUI_PRIORITY_DISCONNECT = 60,
	GUI_PRIORITY_OLDLOADING = 55,
	GUI_PRIORITY_MENUINPUT  = 57,
	GUI_PRIORITY_MENU       = 50,
	GUI_PRIORITY_TOUCHMORE  = 45,
	GUI_PRIORITY_URLWARNING = 40,
	GUI_PRIORITY_TEXPACK    = 35,
	GUI_PRIORITY_TEXIDS     = 30,
	GUI_PRIORITY_TOUCH      = 25,
	GUI_PRIORITY_INVENTORY  = 20,
	GUI_PRIORITY_TABLIST    = 17,
	GUI_PRIORITY_CHAT       = 15,
	GUI_PRIORITY_HUD        = 10,
	GUI_PRIORITY_LOADING    =  5
};

#define GUI_MAX_SCREENS 10
extern struct Screen* Gui_Screens[GUI_MAX_SCREENS];

/* Calculates position of an element on a particular axis */
/* For example, to calculate X position of a text widget on screen */
int Gui_CalcPos(cc_uint8 anchor, int offset, int size, int axisLen);
/* Returns non-zero if the given rectangle contains the given point. */
int Gui_Contains(int recX, int recY, int width, int height, int x, int y);
/* Returns non-zero if one or more pointers lie within the given rectangle. */
int Gui_ContainsPointers(int x, int y, int width, int height);
/* Shows HUD and Status screens. */
void Gui_ShowDefault(void);
#ifdef CC_BUILD_TOUCH
/* Sets whether touch UI should be displayed or not */
void Gui_SetTouchUI(cc_bool enabled);
#endif

/* Removes the screen from the screens list. */
CC_API void Gui_Remove(struct Screen* screen);
/* Inserts a screen into the screen lists with the given priority. */
/* NOTE: If there is an existing screen with the same priority, it is removed. */
CC_API void Gui_Add(struct Screen* screen, int priority);

/* Returns highest priority screen that has grabbed input. */
CC_API struct Screen* Gui_GetInputGrab(void);
/* Returns highest priority screen that blocks world rendering. */
struct Screen* Gui_GetBlocksWorld(void);
/* Returns highest priority screen that is closable. */
struct Screen* Gui_GetClosable(void);
/* Returns screen with the given priority */
CC_API struct Screen* Gui_GetScreen(int priority);
void Gui_UpdateInputGrab(void);
void Gui_ShowPauseMenu(void);

void Gui_LayoutAll(void);
void Gui_RefreshAll(void);
void Gui_Refresh(struct Screen* s);
void Gui_RenderGui(float delta);

#define TEXTATLAS_MAX_WIDTHS 16
struct TextAtlas {
	struct Texture tex;
	int offset, curX;
	float uScale;
	short widths[TEXTATLAS_MAX_WIDTHS];
	short offsets[TEXTATLAS_MAX_WIDTHS];
};
void TextAtlas_Make(struct TextAtlas* atlas, const cc_string* chars, struct FontDesc* font, const cc_string* prefix);
void TextAtlas_Free(struct TextAtlas* atlas);
void TextAtlas_Add(struct TextAtlas* atlas, int charI, struct VertexTextured** vertices);
void TextAtlas_AddInt(struct TextAtlas* atlas, int value, struct VertexTextured** vertices);

#define Elem_Render(elem, delta) (elem)->VTABLE->Render(elem, delta)
#define Elem_Free(elem)          (elem)->VTABLE->Free(elem)
#define Elem_HandlesKeyPress(elem, key) (elem)->VTABLE->HandlesKeyPress(elem, key)

#define Elem_HandlesKeyDown(elem, key, device) (elem)->VTABLE->HandlesKeyDown(elem, key, device)
#define Elem_OnInputUp(elem,      key, device) (elem)->VTABLE->OnInputUp(elem, key, device)

#define Elem_HandlesMouseScroll(elem, delta)    (elem)->VTABLE->HandlesMouseScroll(elem, delta)
#define Elem_HandlesPointerDown(elem, id, x, y) (elem)->VTABLE->HandlesPointerDown(elem, id, x, y)
#define Elem_OnPointerUp(elem,        id, x, y) (elem)->VTABLE->OnPointerUp(elem,        id, x, y)
#define Elem_HandlesPointerMove(elem, id, x, y) (elem)->VTABLE->HandlesPointerMove(elem, id, x, y)

#define Elem_HandlesPadAxis(elem, axis, x, y) (elem)->VTABLE->HandlesPadAxis(elem, axis, x, y)

#define Widget_BuildMesh(widget, vertices) (widget)->VTABLE->BuildMesh(widget, vertices)
#define Widget_Render2(widget, offset)     (widget)->VTABLE->Render2(widget, offset)
#define Widget_Layout(widget) (widget)->VTABLE->Reposition(widget)

CC_END_HEADER
#endif
