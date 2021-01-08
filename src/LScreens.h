#ifndef CC_LSCREENS_H
#define CC_LSCREENS_H
#include "Core.h"
/* Implements screens/menus for the launcher.
	Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/
struct LWidget;
struct LScreen;

typedef void (*LScreen_Func)(struct LScreen* s);
typedef void (*LWidget_Func)(struct LScreen* s, struct LWidget* w);

#define LScreen_Layout \
	LScreen_Func Init;   /* Initialises widgets and other data. Only called once. */ \
	LScreen_Func Show;   /* Called every time this screen is set as the active one. */ \
	LScreen_Func Free;   /* Cleans up all native resources. */ \
	LScreen_Func Layout; /* Positions the widgets on the screen. */ \
	LScreen_Func Draw;   /* Draws all widgets and any other features such as lines/rectangles. */ \
	LScreen_Func Tick;   /* Repeatedly called multiple times every second. */ \
	void (*KeyDown)(struct LScreen* s,     int key, cc_bool wasDown); \
	void (*KeyPress)(struct LScreen* s,    char c);  \
	void (*MouseDown)(struct LScreen* s,   int idx); \
	void (*MouseUp)(struct LScreen* s,     int idx); \
	void (*MouseMove)(struct LScreen* s,   int idx); \
	void (*MouseWheel)(struct LScreen* s,  float delta); \
	void (*TextChanged)(struct LScreen* s, const cc_string* str); \
	LWidget_Func HoverWidget;    /* Called when mouse is moved over a given widget. */ \
	LWidget_Func UnhoverWidget;  /* Called when the mouse is moved away from a previously hovered widget. */ \
	struct LWidget* onEnterWidget;  /* Default widget to auto-click when Enter is pressed. Can be NULL. */ \
	struct LWidget* hoveredWidget;  /* Widget the mouse is currently hovering over. */ \
	struct LWidget* selectedWidget; /* Widget mouse last clicked on. */ \
	int numWidgets;           /* Number of widgets actually used. */ \
	struct LWidget** widgets; /* Array of pointers to all widgets in the screen. */ \
	cc_bool hidesTitlebar;    /* Whether titlebar in window is hidden. */

struct LScreen { LScreen_Layout };
	
void ChooseModeScreen_SetActive(cc_bool firstTime);
void ColoursScreen_SetActive(void);
void DirectConnectScreen_SetActive(void);
void MFAScreen_SetActive(void);
void MainScreen_SetActive(void);
void CheckResourcesScreen_SetActive(void);
void FetchResourcesScreen_SetActive(void);
void ServersScreen_SetActive(void);
void SettingsScreen_SetActive(void);
void UpdatesScreen_SetActive(void);
#endif
