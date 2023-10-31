#ifndef CC_LAUNCHER_H
#define CC_LAUNCHER_H
#include "Bitmap.h"
/* Implements the launcher part of the game.
	Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct LScreen;
struct FontDesc;
struct Context2D;
struct HttpRequest;

/* The screen/menu currently being shown */
extern struct LScreen* Launcher_Active;

/* Whether at the next tick, the launcher window should proceed to stop displaying frames and subsequently exit */
extern cc_bool Launcher_ShouldExit;
/* Whether game should be updated on exit */
extern cc_bool Launcher_ShouldUpdate;
/* (optional) Hash of the server the game should automatically try to connect to after signing in */
extern cc_string Launcher_AutoHash;
/* The username of currently active user */
extern cc_string Launcher_Username;
/* Whether to show empty servers in the server list */
extern cc_bool Launcher_ShowEmptyServers;

struct LauncherTheme {
	/* Whether to use stone tile background like minecraft.net */
	cc_bool ClassicBackground;
	/* Base colour of pixels before any widgets are drawn */
	BitmapCol BackgroundColor;
	/* Colour of pixels on the 4 line borders around buttons */
	BitmapCol ButtonBorderColor;
	/* Colour of button when user has mouse over it */
	BitmapCol ButtonForeActiveColor;
	/* Colour of button when user does not have mouse over it */
	BitmapCol ButtonForeColor;
	/* Colour of line at top of buttons to give them a less flat look*/
	BitmapCol ButtonHighlightColor;
};
/* Currently active theme */
extern struct LauncherTheme Launcher_Theme;
/* Modern / enhanced theme */
extern const struct LauncherTheme Launcher_ModernTheme;
/* Minecraft Classic theme */
extern const struct LauncherTheme Launcher_ClassicTheme;
/* Custom Nordic style theme */
extern const struct LauncherTheme Launcher_NordicTheme;

/* Loads theme from options. */
void Launcher_LoadTheme(void);
/* Saves the theme to options. */
/* NOTE: Does not save options file itself. */
void Launcher_SaveTheme(void);

/* Whether logo should be drawn using bitmapped text */
cc_bool Launcher_BitmappedText(void);
/* Draws title styled text using the given font */
void Launcher_DrawTitle(struct FontDesc* font, const char* text, struct Context2D* ctx);
/* Allocates a font appropriate for drawing title text */
void Launcher_MakeTitleFont(struct FontDesc* font);

/* Attempts to load font and terrain from texture pack. */
void Launcher_TryLoadTexturePack(void);
/* Fills the given region of the given bitmap with the default background */
void Launcher_DrawBackground(struct Context2D* ctx, int x, int y, int width, int height);
/* Fills the entire contents of the given bitmap with the default background */
/* NOTE: Also draws titlebar at top, if current screen permits it */
void Launcher_DrawBackgroundAll(struct Context2D* ctx);

/* Sets currently active screen/menu, freeing old one. */
void Launcher_SetScreen(struct LScreen* screen);
/* Attempts to start the game by connecting to the given server. */
cc_bool Launcher_ConnectToServer(const cc_string* hash);
/* Launcher main loop. */
void Launcher_Run(void);
/* Starts the game from the given arguments. */
cc_bool Launcher_StartGame(const cc_string* user, const cc_string* mppass, const cc_string* ip, const cc_string* port, const cc_string* server);
/* Prints information about a http error to dst. (for status widget) */
/* If req->result is non-zero, also displays a dialog box on-screen. */
void Launcher_DisplayHttpError(struct HttpRequest* req, const char* action, cc_string* dst);
#endif
