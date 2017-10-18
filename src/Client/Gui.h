#ifndef CS_GUI_H
#define CS_GUI_H
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
typedef void (*Gui_Void)(struct GuiElement_* elem);
typedef void (*Gui_Render)(struct GuiElement_* elem, Real64 delta);
typedef bool (*Gui_KeyHandler)(struct GuiElement_* elem, Key key);
typedef bool (*Gui_KeyPress)(struct GuiElement_* elem, UInt8 keyChar);
typedef bool (*Gui_MouseHandler)(struct GuiElement_* elem, Int32 x, Int32 y, MouseButton btn);
typedef bool (*Gui_MouseMove)(struct GuiElement_* elem, Int32 x, Int32 y);
typedef bool (*Gui_MouseScroll)(struct GuiElement_* elem, Real32 delta);

typedef struct GuiElement_ {
	/* Initalises state of this GUI element */
	Gui_Void Init;
	/* Draws this gui element on screen */
	Gui_Render Render;
	/* Frees the state of this GUI element */
	Gui_Void Free;
	/* Recreates all sub-elements and/or textures. (e.g. for when bitmap font changes) */
	Gui_Void Recreate;
	/* Returns whether this GUI element handles a key being pressed. */
	Gui_KeyHandler HandlesKeyDown;
	/* Returns whether this GUI element handles a key being released. */
	Gui_KeyHandler HandlesKeyUp;
	/* Returns whether this GUI element handles a character being input */
	Gui_KeyPress HandlesKeyPress;
	/* Returns whether this GUI element handles a mouse button being pressed. */
	Gui_MouseHandler HandlesMouseDown;
	/* Returns whether this GUI element handles a mouse button being released. */
	Gui_MouseHandler HandlesMouseUp;
	/* Returns whether this GUI element handles the mouse being moved. */
	Gui_MouseMove HandlesMouseMove;
	/* Returns whether this GUI element handles the mouse being scrolled. */
	Gui_MouseScroll HandlesMouseScroll;
} GuiElement;
void GuiElement_Init(GuiElement* elem);

struct Screen_;
typedef void (*Screen_Void)(struct Screen_* screen);

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
	Screen_Void OnResize;
	Screen_Void OnContextLost;
	Screen_Void OnContextRecreated;
} Screen;
void Screen_Init(Screen* screen);

struct Widget_;
typedef void (*Widget_Reposition)(struct Widget_* widget);

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
	Widget_Reposition Reposition;
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
void Gui_SetScreen(Screen* screen);
void Gui_RefreshHud(void);
void Gui_ShowOverlay(Screen* overlay);
void Gui_RenderGui(Real64 delta);
void Gui_OnResize(void);
#endif