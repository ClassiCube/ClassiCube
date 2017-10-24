#ifndef CC_GUI_H
#define CC_GUI_H
#include "Typedefs.h"
#include "Input.h"
#include "GameStructs.h"
#include "GraphicsEnums.h"
/* Describes and manages 2D GUI elements on screen.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef UInt8 Anchor;
#define ANCHOR_LEFT_OR_TOP 0
#define ANCHOR_CENTRE 1
#define ANCHOR_BOTTOM_OR_RIGHT 2

struct GuiElement_;
typedef struct GuiElement_ {
	/* Initalises state of this GUI element */
	void (*Init)(struct GuiElement_* elem);
	/* Draws this gui element on screen */
	void (*Render)(struct GuiElement_* elem, Real64 delta);
	/* Frees the state of this GUI element */
	void (*Free)(struct GuiElement_* elem);
	/* Recreates all sub-elements and/or textures. (e.g. for when bitmap font changes) */
	void (*Recreate)(struct GuiElement_* elem);
	/* Returns whether this GUI element handles a key being pressed. */
	bool (*HandlesKeyDown)(struct GuiElement_* elem, Key key);
	/* Returns whether this GUI element handles a key being released. */
	bool (*HandlesKeyUp)(struct GuiElement_* elem, Key key);
	/* Returns whether this GUI element handles a character being input */
	bool (*HandlesKeyPress)(struct GuiElement_* elem, UInt8 keyChar);
	/* Returns whether this GUI element handles a mouse button being pressed. */
	bool (*HandlesMouseDown)(struct GuiElement_* elem, Int32 x, Int32 y, MouseButton btn);
	/* Returns whether this GUI element handles a mouse button being released. */
	bool (*HandlesMouseUp)(struct GuiElement_* elem, Int32 x, Int32 y, MouseButton btn);
	/* Returns whether this GUI element handles the mouse being moved. */
	bool (*HandlesMouseMove)(struct GuiElement_* elem, Int32 x, Int32 y);
	/* Returns whether this GUI element handles the mouse being scrolled. */
	bool (*HandlesMouseScroll)(struct GuiElement_* elem, Real32 delta);
} GuiElement;
void GuiElement_Init(GuiElement* elem);

struct Screen_;
/* Represents a container of widgets and other 2D elements. May cover entire window. */
typedef struct Screen_ {
	GuiElement Base;
	/* Whether this screen handles all mouse and keyboard input.
	This prevents the user from interacting with the world. */
	bool HandlesAllInput;
	/* Whether this screen completely and opaquely covers the game world behind it. */
	bool BlocksWorld;
	/* Whether this screen hides the normal in-game HUD. */
	bool HidesHUD;
	/* Whether the normal in-game HUD should be drawn over the top of this screen. */
	bool RenderHUDOver;
	/* Called when the game window is resized. */
	void (*OnResize)(struct Screen_* screen);
	void (*OnContextLost)(struct Screen_* screen);
	void (*OnContextRecreated)(struct Screen_* screen);
} Screen;
void Screen_Init(Screen* screen);

struct Widget_;
/* Represents an individual 2D gui component. */
typedef struct Widget_ {
	GuiElement Base;
	/* Top left corner, and dimensions, of this widget */
	Int32 X, Y, Width, Height;
	/* Whether this widget is currently being moused over. */
	bool Active;
	/* Whether widget is prevented from being interacted with. */
	bool Disabled;
	/* Specifies the reference point for when this widget is resized */
	Anchor HorAnchor, VerAnchor;
	/* Offset from the reference point */
	Int32 XOffset, YOffset;
	void (*Reposition)(struct Widget_* widget);
} Widget;
void Widget_DoReposition(Widget* w);
void Widget_Init(Widget* widget);

GfxResourceID Gui_GuiTex, Gui_GuiClassicTex, Gui_IconsTex;
Screen* Gui_HUD;
Screen* Gui_Active;
#define GUI_MAX_OVERLAYS 40
Screen* Gui_Overlays[GUI_MAX_OVERLAYS];
UInt32 Gui_OverlayCount;

Int32 Gui_CalcPos(Anchor anchor, Int32 offset, Int32 size, Int32 axisLen);
bool Gui_Contains(Int32 recX, Int32 recY, Int32 width, Int32 height, Int32 x, Int32 y);
/* Creates game component implementation. */
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
void Gui_ShowOverlay(Screen* overlay);
void Gui_RenderGui(Real64 delta);
void Gui_OnResize(void);
#endif