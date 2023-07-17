#ifndef CC_LSCREENS_H
#define CC_LSCREENS_H
#include "Core.h"
/* 
Implements all of the screens/menus in the launcher
Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct LWidget;
struct LScreen;
struct Context2D;

typedef void (*LScreen_Func)(struct LScreen* s);

#define LScreen_Layout \
	LScreen_Func Init;   /* Initialises widgets and other data. Only called once. */ \
	LScreen_Func Show;   /* Called every time this screen is set as the active one. */ \
	LScreen_Func Free;   /* Cleans up all native resources. */ \
	LScreen_Func Layout; /* Positions the widgets on the screen. */ \
	LScreen_Func Tick;   /* Repeatedly called multiple times every second. */ \
	void (*DrawBackground)(struct LScreen* s, struct Context2D* ctx); \
	void (*KeyDown)(struct LScreen* s,     int key, cc_bool wasDown); \
	void (*MouseUp)(struct LScreen* s,     int idx); \
	void (*MouseWheel)(struct LScreen* s,  float delta); \
	void (*ResetArea)(struct Context2D* ctx, int x, int y, int width, int height); \
	struct LWidget* onEnterWidget;  /* Default widget to auto-click when Enter is pressed. Can be NULL. */ \
	struct LWidget* onEscapeWidget; /* Widget to auto-click when Escape is pressed. Can be NULL. */ \
	struct LWidget* hoveredWidget;  /* Widget the mouse is currently hovering over. */ \
	struct LWidget* selectedWidget; /* Widget mouse last clicked on. */ \
	int numWidgets;           /* Number of widgets actually used */ \
	struct LWidget** widgets; /* Array of pointers to all the widgets in the screen */ \
	const char* title;        /* Titlebar text */

struct LScreen { LScreen_Layout };

void LScreen_SelectWidget(struct LScreen* s, int idx, struct LWidget* w, cc_bool was);
void LScreen_UnselectWidget(struct LScreen* s, int idx, struct LWidget* w);
	
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
#endif
