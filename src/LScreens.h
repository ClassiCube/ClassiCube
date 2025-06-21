#ifndef CC_LSCREENS_H
#define CC_LSCREENS_H
#include "Core.h"
CC_BEGIN_HEADER

/* 
Implements all of the screens/menus in the launcher
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct LWidget;
struct LScreen;
struct Context2D;
struct InputDevice;

typedef void (*LScreen_Func)(struct LScreen* s);

#define LScreen_Layout \
	LScreen_Func Activated;   /* Called whenever this screen is set as the active screen */ \
	LScreen_Func LoadState;   /* Called after the first time this screen is set as the active screen */ \
	LScreen_Func Deactivated; /* Called when the active screen is switched to a different one */ \
	LScreen_Func Layout;      /* Positions the widgets on this screen */ \
	LScreen_Func Tick;        /* Repeatedly called multiple times every second */ \
	void (*DrawBackground)(struct LScreen* s, struct Context2D* ctx); \
	void (*KeyDown)(struct LScreen* s,     int key, cc_bool wasDown, struct InputDevice* device); \
	void (*MouseUp)(struct LScreen* s,     int idx); \
	void (*MouseWheel)(struct LScreen* s,  float delta); \
	void (*ResetArea)(struct Context2D* ctx, int x, int y, int width, int height); \
	struct LWidget* onEnterWidget;  /* Default widget to auto-click when Enter is pressed, can be NULL */ \
	struct LWidget* onEscapeWidget; /* Widget to auto-click when Escape is pressed, can be NULL */ \
	struct LWidget* hoveredWidget;  /* Widget the mouse is currently hovering over */ \
	struct LWidget* selectedWidget; /* Widget mouse last clicked on */ \
	int numWidgets;           /* Number of widgets actually used */ \
	short maxWidgets;         /* Maximum number of widgets that can be added to this screen */ \
	cc_bool everShown;        /* Whether this screen has ever been shown before */ \
	struct LWidget** widgets; /* Array of pointers to all the widgets in this screen */ \
	const char* title;        /* Titlebar text */

struct LScreen { LScreen_Layout };

void LScreen_SelectWidget(struct LScreen* s, int idx, struct LWidget* w, cc_bool was);
void LScreen_UnselectWidget(struct LScreen* s, int idx, struct LWidget* w);
void LScreen_AddWidget(void* screen, void* widget);
	
void ChooseModeScreen_SetActive(cc_bool firstTime);
void ColoursScreen_SetActive(void);
void DirectConnectScreen_SetActive(void);
void MFAScreen_SetActive(void);
void MainScreen_SetActive(void);
void CheckResourcesScreen_SetActive(void);
void FetchResourcesScreen_SetActive(void);
void ServersScreen_SetActive(void);
void SettingsScreen_SetActive(void);
void ThemesScreen_SetActive(void);
void UpdatesScreen_SetActive(void);

CC_END_HEADER
#endif
