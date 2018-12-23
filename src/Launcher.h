#ifndef CC_LAUNCHER_H
#define CC_LAUNCHER_H
#include "Bitmap.h"
#include "String.h"
/* Implements the launcher part of the game.
	Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct LScreen;
#ifdef CC_BUILD_WIN
#define GAME_EXE_NAME "ClassiCube.exe"
#else
#define GAME_EXE_NAME "./ClassiCube"
#endif

/* Currently active screen/menu. */
extern struct LScreen* Launcher_Screen;
/* The area/region of the window that needs to be redrawn and presented to the screen. */
/* If width is 0, means no area needs to be redrawn. */
extern Rect2D Launcher_Dirty;
/* Contains the pixels that are drawn to the window. */
extern Bitmap Launcher_Framebuffer;
/* Whether to use stone tile background like minecraft.net. */
extern bool Launcher_ClassicBackground;
/* Default font for buttons and labels. */
extern FontDesc Launcher_TitleFont, Launcher_TextFont;
/* Default font for input widget hints. */
extern FontDesc Launcher_HintFont;

/* Whether at the next tick, the launcher window should proceed to stop displaying frames and subsequently exit. */
extern bool Launcher_ShouldExit;
/* Whether update script should be asynchronously run on exit. */
extern bool Launcher_ShouldUpdate;

/* Base colour of pixels before any widgets are drawn. */
extern BitmapCol Launcher_BackgroundCol;
/* Colour of pixels on the 4 line borders around buttons. */
extern BitmapCol Launcher_ButtonBorderCol;
/* Colour of button when user has mouse over it. */
extern BitmapCol Launcher_ButtonForeActiveCol;
/* Colour of button when user does not have mouse over it. */
extern BitmapCol Launcher_ButtonForeCol;
/* Colour of line at top of buttons to give them a less flat look.*/
extern BitmapCol Launcher_ButtonHighlightCol;

/* Resets colours to default. */
void Launcher_ResetSkin(void);
/* Loads colours from options. */
void Launcher_LoadSkin(void);
/* Saves the colours to options. */
/* NOTE: Does not save options file itself. */
void Launcher_SaveSkin(void);

/* Attempts to load font and terrain from texture pack. */
void Launcher_TryLoadTexturePack(void);
/* Redraws all pixels with default background. */
/* NOTE: Also draws titlebar at top, if current screen permits it. */
void Launcher_ResetPixels(void);
/* Redraws the specified region with the background pixels. */
/* Also marks that area as neeing to be redrawn. */
void Launcher_ResetArea(int x, int y, int width, int height);
/* Resets pixels to default, then draws widgets of current screen over it. */
/* Marks the entire window as needing to be redrawn. */
void Launcher_Redraw(void);
/* Marks the given area/region as needing to be redrawn. */
CC_NOINLINE void Launcher_MarkDirty(int x, int y, int width, int height);
/* Marks the entire window as needing to be redrawn. */
CC_NOINLINE void Launcher_MarkAllDirty(void);

/* Sets currently active screen/menu, freeing old one. */
void Launcher_SetScreen(struct LScreen* screen);
/* Attempts to start the game by connecting to the given server. */
bool Launcher_ConnectToServer(const String* hash);
/* Launcher main loop. */
void Launcher_Run(void);

/* Shows a message box for an error. */
void Launcher_ShowError(ReturnCode res, const char* place);
/* Starts the game from the given arguments. */
bool Launcher_StartGame(const String* user, const String* mppass, const String* ip, const String* port, const String* server);
#endif
