#ifndef CC_LSCREENS_H
#define CC_LSCREENS_H
#include "AsyncDownloader.h"
#include "String.h"
/* Implements screens/menus for the launcher.
	Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct LWidget;
struct LScreen;

typedef void (*LScreen_Func)(struct LScreen* s);
typedef void (*LWidget_Func)(struct LScreen* s, struct LWidget* w);

#define LScreen_Layout \
	LScreen_Func Init; /* Initialises widgets and other data. */ \
	LScreen_Func Free; /* Cleans up all native resources. */ \
	LScreen_Func DrawAll; /* Redraws all widgets. */ \
	LScreen_Func Tick; /* Repeatedly called multiple times every second. */ \
	LScreen_Func OnDisplay; /* Called when framebuffer is about to be displayed. */ \
	LWidget_Func HoverWidget; /* Called when mouse is moved over a given widget. */ \
	LWidget_Func UnhoverWidget; /* Called when the mouse is moved away from a previously hovered widget. */ \
	struct LWidget* HoveredWidget; /* Widget the mouse is currently hovering over. */ \
	int NumWidgets; /* Number of widgets actually used. */

struct LScreen {
	LScreen_Layout
	/* All widgets in the screen. */
	/* NOTE: This is vriable sized, so must be the last member. */
	/* (e.g. derived screens might use: struct LWidget* Widgets[10];) */
	struct LWidget** Widgets;
};
	
struct LScreen* ChooseModeScreen_MakeInstance(void);
struct LScreen* ColoursScreen_MakeInstance(void);
struct LScreen* DirectConnectScreen_MakeInstance(void);
struct LScreen* MainScreen_MakeInstance(void);
struct LScreen* ResourcesScreen_MakeInstance(void);
struct LScreen* ServersScreen_MakeInstance(void);
struct LScreen* SettingsScreen_MakeInstance(void);
struct LScreen* UpdateScreen_MakeInstance(void);
#endif
