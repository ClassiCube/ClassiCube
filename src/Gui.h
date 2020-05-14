#ifndef CC_GUI_H
#define CC_GUI_H
#include "String.h"
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
struct VertexTextured;
struct FontDesc;
struct Widget;
extern struct IGameComponent Gui_Component;

/* Whether vanilla Minecraft Classic gui texture is used. */
extern cc_bool Gui_ClassicTexture;
/* Whether tab list is laid out like vanilla Minecraft Classic. */
extern cc_bool Gui_ClassicTabList;
/* Whether menus are laid out like vanilla Minecraft Classic. */
extern cc_bool Gui_ClassicMenu;
/* Whether classic-style chat screen is used */
extern cc_bool Gui_ClassicChat;
/* Maximum number of visible chatlines on screen. Can be 0. */
extern int     Gui_Chatlines;
/* Whether clicking on a chatline inserts it into chat input. */
extern cc_bool Gui_ClickableChat;
/* Whether pressing tab in chat input attempts to autocomplete player names. */
extern cc_bool Gui_TabAutocomplete;
/* Whether FPS counter (and other info) is shown in top left. */
extern cc_bool Gui_ShowFPS;

/* Functions for a Screen instance. */
struct ScreenVTABLE {
	/* Initialises persistent state. */
	void (*Init)(void* elem);
	/* Updates this screen, called every frame just before Render(). */
	void (*Update)(void* elem, double delta);
	/* Frees/releases persistent state. */
	void (*Free)(void* elem);
	/* Draws this screen and its widgets on screen. */
	void (*Render)(void* elem, double delta);
	/* Builds the vertex mesh for all the widgets in the screen. */
	void (*BuildMesh)(void* elem);
	/* Returns non-zero if an input press is handled. */
	int  (*HandlesInputDown)(void* elem, int key);
	/* Returns non-zero if an input release is handled. */
	int  (*HandlesInputUp)(void* elem, int key);
	/* Returns non-zero if a key character press is handled. */
	int  (*HandlesKeyPress)(void* elem, char keyChar);
	/* Returns non-zero if a key character press is handled. */
	/* Currently only raised by on-screen keyboard in web client. */
	int  (*HandlesTextChanged)(void* elem, const String* str);
	/* Returns non-zero if a pointer press is handled. */
	int  (*HandlesPointerDown)(void* elem, int id, int x, int y);
	/* Returns non-zero if a pointer release is handled. */
	int  (*HandlesPointerUp)(void* elem,   int id, int x, int y);
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
};
#define Screen_Body const struct ScreenVTABLE* VTABLE; \
	cc_bool grabsInput;  /* Whether this screen grabs input. Causes the cursor to become visible. */ \
	cc_bool blocksWorld; /* Whether this screen completely and opaquely covers the game world behind it. */ \
	cc_bool closable;    /* Whether this screen is automatically closed when pressing Escape */ \
	cc_bool dirty;       /* Whether this screens needs to have its mesh rebuilt. */ \
	int maxVertices; GfxResourceID vb; struct Widget** widgets; int numWidgets;

/* Represents a container of widgets and other 2D elements. May cover entire window. */
struct Screen { Screen_Body };

typedef void (*Widget_LeftClick)(void* screen, void* widget);
struct WidgetVTABLE {
	/* Draws this widget on-screen. */
	void (*Render)(void* elem, double delta);
	/* Destroys allocated graphics resources. */
	void (*Free)(void* elem);
	/* Positions this widget on-screen. */
	void (*Reposition)(void* elem);
	/* Returns non-zero if an input press is handled. */
	int (*HandlesKeyDown)(void* elem, int key);
	/* Returns non-zero if an input release is handled. */
	int (*HandlesKeyUp)(void* elem, int key);
	/* Returns non-zero if a mouse wheel scroll is handled. */
	int (*HandlesMouseScroll)(void* elem, float delta);
	/* Returns non-zero if a pointer press is handled. */
	int (*HandlesPointerDown)(void* elem, int id, int x, int y);
	/* Returns non-zero if a pointer release is handled. */
	int (*HandlesPointerUp)(void* elem,   int id, int x, int y);
	/* Returns non-zero if a pointer movement is handled. */
	int (*HandlesPointerMove)(void* elem, int id, int x, int y);
	/* Builds the mesh of vertices for this widget. */
	void (*BuildMesh)(void* elem, struct VertexTextured** vertices);
	/* Draws this widget on-screen. */
	int  (*Render2)(void* elem, int offset);
};
#define Widget_Body const struct WidgetVTABLE* VTABLE; \
	int x, y, width, height;       /* Top left corner, and dimensions, of this widget */ \
	cc_bool active;                /* Whether this widget is currently being moused over */ \
	cc_bool disabled;              /* Whether widget is prevented from being interacted with */ \
	cc_uint8 horAnchor, verAnchor; /* The reference point for when this widget is resized */ \
	int xOffset, yOffset;          /* Offset from the reference point */ \
	Widget_LeftClick MenuClick;

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


extern GfxResourceID Gui_GuiTex, Gui_GuiClassicTex, Gui_IconsTex;

/* Higher priority handles input first and draws on top */
enum GuiPriority {
	GUI_PRIORITY_DISCONNECT = 60,
	GUI_PRIORITY_OLDLOADING = 55,
	GUI_PRIORITY_MENU       = 50,
	GUI_PRIORITY_TOUCHMORE  = 45,
	GUI_PRIORITY_URLWARNING = 40,
	GUI_PRIORITY_TEXPACK    = 35,
	GUI_PRIORITY_TEXIDS     = 30,
	GUI_PRIORITY_TOUCH      = 25,
	GUI_PRIORITY_INVENTORY  = 20,
	GUI_PRIORITY_CHAT       = 15,
	GUI_PRIORITY_HUD        = 10,
	GUI_PRIORITY_LOADING    =  5
};

struct HUDScreen;
struct ChatScreen;
extern struct HUDScreen*  Gui_HUD;
extern struct ChatScreen* Gui_Chat;

#define GUI_MAX_SCREENS 10
extern struct Screen* Gui_Screens[GUI_MAX_SCREENS];
extern int Gui_ScreensCount;

/* Calculates position of an element on a particular axis */
/* For example, to calculate X position of a text widget on screen */
int Gui_CalcPos(cc_uint8 anchor, int offset, int size, int axisLen);
/* Returns non-zero if the given rectangle contains the given point. */
int Gui_Contains(int recX, int recY, int width, int height, int x, int y);
/* Returns non-zero if one or more pointers lie within the given rectangle. */
int Gui_ContainsPointers(int x, int y, int width, int height);
/* Shows HUD and Status screens. */
void Gui_ShowDefault(void);

/* Inserts a screen into the screen lists with the given priority. */
/* NOTE: You MUST ensure a screen isn't added twice. Or use Gui_Replace. */
CC_API void Gui_Add(struct Screen* screen, int priority);
/* Removes the screen from the screens list. */
CC_API void Gui_Remove(struct Screen* screen);
/* Shorthand for Gui_Remove then Gui_Add. */
CC_API void Gui_Replace(struct Screen* screen, int priority);

/* Returns highest priority screen that has grabbed input. */
struct Screen* Gui_GetInputGrab(void);
/* Returns highest priority screen that blocks world rendering. */
struct Screen* Gui_GetBlocksWorld(void);
/* Returns highest priority screen that is closable. */
struct Screen* Gui_GetClosable(void);

void Gui_RefreshAll(void);
void Gui_RefreshChat(void);
void Gui_Refresh(struct Screen* s);

void Gui_RenderGui(double delta);
void Gui_Layout(void);

#define TEXTATLAS_MAX_WIDTHS 16
struct TextAtlas {
	struct Texture tex;
	int offset, curX;
	float uScale;
	short widths[TEXTATLAS_MAX_WIDTHS];
	short offsets[TEXTATLAS_MAX_WIDTHS];
};
void TextAtlas_Make(struct TextAtlas* atlas, const String* chars, struct FontDesc* font, const String* prefix);
void TextAtlas_Free(struct TextAtlas* atlas);
void TextAtlas_Add(struct TextAtlas* atlas, int charI, struct VertexTextured** vertices);
void TextAtlas_AddInt(struct TextAtlas* atlas, int value, struct VertexTextured** vertices);

#define Elem_Render(elem, delta) (elem)->VTABLE->Render(elem, delta)
#define Elem_Free(elem)          (elem)->VTABLE->Free(elem)
#define Elem_HandlesKeyPress(elem, key) (elem)->VTABLE->HandlesKeyPress(elem, key)
#define Elem_HandlesKeyDown(elem, key)  (elem)->VTABLE->HandlesKeyDown(elem, key)
#define Elem_HandlesKeyUp(elem, key)    (elem)->VTABLE->HandlesKeyUp(elem, key)

#define Elem_HandlesMouseScroll(elem, delta)    (elem)->VTABLE->HandlesMouseScroll(elem, delta)
#define Elem_HandlesPointerDown(elem, id, x, y) (elem)->VTABLE->HandlesPointerDown(elem, id, x, y)
#define Elem_HandlesPointerUp(elem,   id, x, y) (elem)->VTABLE->HandlesPointerUp(elem,   id, x, y)
#define Elem_HandlesPointerMove(elem, id, x, y) (elem)->VTABLE->HandlesPointerMove(elem, id, x, y)

#define Widget_BuildMesh(widget, vertices) (widget)->VTABLE->BuildMesh(widget, vertices);
#define Widget_Render2(widget, offset)     (widget)->VTABLE->Render2(widget, offset);
#define Widget_Layout(widget) (widget)->VTABLE->Reposition(widget);
#define Elem_TryFree(elem)    if ((elem)->VTABLE) { Elem_Free(elem); }
#endif
