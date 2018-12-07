#ifndef CC_LSCREENS_H
#define CC_LSCREENS_H
#include "AsyncDownloader.h"
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
	LScreen_Func DrawAll; /* Redraws all widgets. */ \
	LScreen_Func Tick; /* Repeatedly called multiple times every second. */ \
	LScreen_Func OnDisplay; /* Called when framebuffer is about to be displayed. */ \
	void (*KeyDown)(struct LScreen* s,   Key key); \
	void (*MouseDown)(struct LScreen* s, MouseButton btn); \
	void (*MouseUp)(struct LScreen* s,   MouseButton btn); \
	void (*MouseMove)(struct LScreen* s, int deltaX, int deltaY); \
	LWidget_Func HoverWidget;    /* Called when mouse is moved over a given widget. */ \
	LWidget_Func UnhoverWidget;  /* Called when the mouse is moved away from a previously hovered widget. */ \
	LWidget_Func SelectWidget;   /* Called when mouse clicks on a given widget. */ \
	LWidget_Func UnselectWidget; /* Called when the mouse clicks on a different widget. */ \
	struct LWidget* OnEnterWidget;  /* Default widget to auto-click when Enter is pressed. Can be NULL. */ \
	struct LWidget* HoveredWidget;  /* Widget the mouse is currently hovering over. */ \
	struct LWidget* SelectedWidget; /* Widget mouse last clicked on. */ \
	int NumWidgets;           /* Number of widgets actually used. */ \
	struct LWidget** Widgets; /* Array of pointers to all widgets in the screen. */

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
