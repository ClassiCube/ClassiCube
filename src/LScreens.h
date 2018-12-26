#ifndef CC_LSCREENS_H
#define CC_LSCREENS_H
#include "Http.h"
#include "String.h"
#include "Input.h"
/* Implements screens/menus for the launcher.
	Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct LWidget;
struct LScreen;

typedef void (*LScreen_Func)(struct LScreen* s);
typedef void(*LWidget_Func)(struct LScreen* s, struct LWidget* w);

#define LScreen_Layout \
	LScreen_Func Init; /* Initialises widgets and other data. */ \
	LScreen_Func Free; /* Cleans up all native resources. */ \
	LScreen_Func Reposition; /* Repositions the widgets in the screen. */ \
	LScreen_Func Draw; /* Draws all widgets and any other features such as lines/rectangles. */ \
	LScreen_Func Tick; /* Repeatedly called multiple times every second. */ \
	LScreen_Func OnDisplay; /* Called when framebuffer is about to be displayed. */ \
	void (*KeyDown)(struct LScreen* s,    Key key, bool wasDown); \
	void (*KeyPress)(struct LScreen* s,   char c);  \
	void (*MouseDown)(struct LScreen* s,  MouseButton btn); \
	void (*MouseUp)(struct LScreen* s,    MouseButton btn); \
	void (*MouseMove)(struct LScreen* s,  int deltaX, int deltaY); \
	void (*MouseWheel)(struct LScreen* s, float delta); \
	LWidget_Func HoverWidget;    /* Called when mouse is moved over a given widget. */ \
	LWidget_Func UnhoverWidget;  /* Called when the mouse is moved away from a previously hovered widget. */ \
	struct LWidget* OnEnterWidget;  /* Default widget to auto-click when Enter is pressed. Can be NULL. */ \
	struct LWidget* HoveredWidget;  /* Widget the mouse is currently hovering over. */ \
	struct LWidget* SelectedWidget; /* Widget mouse last clicked on. */ \
	int NumWidgets;           /* Number of widgets actually used. */ \
	struct LWidget** Widgets; /* Array of pointers to all widgets in the screen. */ \
	bool HidesTitlebar;       /* Whether titlebar in window is hidden. */

struct LScreen { LScreen_Layout };
	
struct LScreen* ChooseModeScreen_MakeInstance(bool firstTime);
struct LScreen* ColoursScreen_MakeInstance(void);
struct LScreen* DirectConnectScreen_MakeInstance(void);
struct LScreen* MainScreen_MakeInstance(void);
struct LScreen* ResourcesScreen_MakeInstance(void);
struct LScreen* ServersScreen_MakeInstance(void);
struct LScreen* SettingsScreen_MakeInstance(void);
struct LScreen* UpdatesScreen_MakeInstance(void);
#endif
