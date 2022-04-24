#include "Launcher.h"
#ifndef CC_BUILD_WEB
#include "String.h"
#include "LScreens.h"
#include "LWidgets.h"
#include "LWeb.h"
#include "Resources.h"
#include "Drawer2D.h"
#include "Game.h"
#include "Deflate.h"
#include "Stream.h"
#include "Utils.h"
#include "Input.h"
#include "Window.h"
#include "Event.h"
#include "Http.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Logger.h"
#include "Options.h"
#include "LBackend.h"
#include "PackedCol.h"

struct LScreen* Launcher_Active;
struct FontDesc Launcher_LogoFont;

cc_bool Launcher_ShouldExit, Launcher_ShouldUpdate;
static char hashBuffer[STRING_SIZE], userBuffer[STRING_SIZE];
cc_string Launcher_AutoHash = String_FromArray(hashBuffer);
cc_string Launcher_Username = String_FromArray(userBuffer);
cc_bool Launcher_ShowEmptyServers;

static cc_bool useBitmappedFont, hasBitmappedFont;
static struct Bitmap dirtBmp, stoneBmp;
#define TILESIZE 48

static void CloseActiveScreen(void) {
	Window_CloseKeyboard();
	if (!Launcher_Active) return;
	
	Launcher_Active->Free(Launcher_Active);
	LBackend_CloseScreen(Launcher_Active);
	Launcher_Active = NULL;
}

void Launcher_SetScreen(struct LScreen* screen) {
	int i;
	CloseActiveScreen();
	Launcher_Active = screen;
	if (!screen->numWidgets) screen->Init(screen);

	screen->Show(screen);
	screen->Layout(screen);
	/* for hovering over active button etc */
	for (i = 0; i < Pointers_Count; i++) {
		screen->MouseMove(screen, i);
	}

	LBackend_SetScreen(screen);
	LBackend_Redraw();
}

void Launcher_DisplayHttpError(cc_result res, int status, const char* action, cc_string* dst) {
	if (res) {
		/* Non HTTP error - this is not good */
		Logger_Warn(res, action, Http_DescribeError);
		String_Format2(dst, "&cError %i when %c", &res, action);
	} else if (status != 200) {
		String_Format2(dst, "&c%i error when %c", &status, action);
	} else {
		String_Format1(dst, "&cEmpty response when %c", action);
	}
}


/*########################################################################################################################*
*--------------------------------------------------------Starter/Updater--------------------------------------------------*
*#########################################################################################################################*/
static cc_uint64 lastJoin;
cc_bool Launcher_StartGame(const cc_string* user, const cc_string* mppass, const cc_string* ip, const cc_string* port, const cc_string* server) {
	cc_string args[4]; int numArgs;
	cc_uint64 now;
	cc_result res;
	
	now = Stopwatch_Measure();
	if (Stopwatch_ElapsedMS(lastJoin, now) < 1000) return false;
	lastJoin = now;

	/* Save resume info */
	if (server->length) {
		Options_Set(ROPT_SERVER, server);
		Options_Set(ROPT_USER,   user);
		Options_Set(ROPT_IP,     ip);
		Options_Set(ROPT_PORT,   port);
		Options_SetSecure(ROPT_MPPASS, mppass);
	}
	/* Save options BEFORE starting new game process */
	/* Otherwise can get 'file already in use' errors on startup */
	Options_SaveIfChanged();

	args[0] = *user;
	numArgs = 1;
	if (mppass->length) {
		args[1] = *mppass;
		args[2] = *ip;
		args[3] = *port;
		numArgs = 4;
	}

	res = Process_StartGame2(args, numArgs);
	if (res) { Logger_SysWarn(res, "starting game"); return false; }

#ifdef CC_BUILD_MOBILE
	Launcher_ShouldExit = true;
#else
	Launcher_ShouldExit = Options_GetBool(LOPT_AUTO_CLOSE, false);
#endif
	return true;
}

CC_NOINLINE static void StartFromInfo(struct ServerInfo* info) {
	cc_string port; char portBuffer[STRING_INT_CHARS];
	String_InitArray(port, portBuffer);

	String_AppendInt(&port, info->port);
	Launcher_StartGame(&Launcher_Username, &info->mppass, &info->ip, &port, &info->name);
}

cc_bool Launcher_ConnectToServer(const cc_string* hash) {
	struct ServerInfo* info;
	cc_string logMsg;
	int i;
	if (!hash->length) return false;

	for (i = 0; i < FetchServersTask.numServers; i++) {
		info = &FetchServersTask.servers[i];
		if (!String_Equals(hash, &info->hash)) continue;

		StartFromInfo(info);
		return true;
	}

	/* Fallback to private server handling */
	/* TODO: Rewrite to be async */
	FetchServerTask_Run(hash);

	while (!FetchServerTask.Base.completed) { 
		LWebTask_Tick(&FetchServerTask.Base);
		Thread_Sleep(10); 
	}

	if (FetchServerTask.server.hash.length) {
		StartFromInfo(&FetchServerTask.server);
		return true;
	} else if (FetchServerTask.Base.success) {
		Window_ShowDialog("Failed to connect", "No server has that hash");
	} else {
		logMsg = String_Init(NULL, 0, 0);
		LWebTask_DisplayError(&FetchServerTask.Base, "fetching server info", &logMsg);
	}
	return false;
}


/*########################################################################################################################*
*---------------------------------------------------------Event handler---------------------------------------------------*
*#########################################################################################################################*/
static void OnResize(void* obj) {
	LBackend_FreeFramebuffer();
	LBackend_InitFramebuffer();

	if (Launcher_Active) Launcher_Active->Layout(Launcher_Active);
	LBackend_Redraw();
}

static cc_bool IsShutdown(int key) {
	if (key == KEY_F4 && Key_IsAltPressed()) return true;

	/* On macOS, Cmd+Q should also end the process */
#ifdef CC_BUILD_DARWIN
	return key == 'Q' && Key_IsWinPressed();
#else
	return false;
#endif
}

static void OnInputDown(void* obj, int key, cc_bool was) {
	if (IsShutdown(key)) Launcher_ShouldExit = true;
	Launcher_Active->KeyDown(Launcher_Active, key, was);
}

static void OnKeyPress(void* obj, int c) {
	Launcher_Active->KeyPress(Launcher_Active, c);
}

static void OnTextChanged(void* obj, const cc_string* str) {
	Launcher_Active->TextChanged(Launcher_Active, str);
}

static void OnMouseWheel(void* obj, float delta) {
	Launcher_Active->MouseWheel(Launcher_Active, delta);
}


/*########################################################################################################################*
*-----------------------------------------------------------Main body-----------------------------------------------------*
*#########################################################################################################################*/
static void Launcher_Init(void) {
	Event_Register_(&WindowEvents.Resized,      NULL, OnResize);
	Event_Register_(&WindowEvents.StateChanged, NULL, OnResize);

	Event_Register_(&InputEvents.Down,          NULL, OnInputDown);
	Event_Register_(&InputEvents.Press,         NULL, OnKeyPress);
	Event_Register_(&InputEvents.Wheel,         NULL, OnMouseWheel);
	Event_Register_(&InputEvents.TextChanged,   NULL, OnTextChanged);

	Utils_EnsureDirectory("texpacks");
	Utils_EnsureDirectory("audio");
}

static void Launcher_Free(void) {
	Event_UnregisterAll();
	LBackend_Free();
	Flags_Free();
	Font_Free(&Launcher_LogoFont);
	hasBitmappedFont = false;

	CloseActiveScreen();
	LBackend_FreeFramebuffer();
}

void Launcher_Run(void) {
	static const cc_string title = String_FromConst(GAME_APP_TITLE);
	Window_Create2D(640, 400);
#ifdef CC_BUILD_MOBILE
	Window_LockLandscapeOrientation(Options_GetBool(OPT_LANDSCAPE_MODE, false));
#endif
	Window_SetTitle(&title);
	Window_Show();
	LWidget_CalcOffsets();

#ifdef CC_BUILD_WIN
	/* clean leftover exe from updating */
	if (Options_GetBool("update-dirty", false) && Updater_Clean()) {
		Options_Set("update-dirty", NULL);
	}
#endif

	Drawer2D_Component.Init();
	Drawer2D.BitmappedText    = false;
	Drawer2D.BlackTextShadows = true;

	LBackend_Init();
	LBackend_InitFramebuffer();
	Launcher_ShowEmptyServers = Options_GetBool(LOPT_SHOW_EMPTY, true);
	Options_Get(LOPT_USERNAME, &Launcher_Username, "");

	LWebTasks_Init();
	Session_Load();
	Launcher_LoadTheme();
	Launcher_Init();
	Launcher_TryLoadTexturePack();

	Http_Component.Init();
	Resources_CheckExistence();
	CheckUpdateTask_Run();

	if (Resources_Count) {
		CheckResourcesScreen_SetActive();
	} else {
		MainScreen_SetActive();
	}

	for (;;) {
		Window_ProcessEvents();
		if (!WindowInfo.Exists || Launcher_ShouldExit) break;

		Launcher_Active->Tick(Launcher_Active);
		LBackend_Tick();
		Thread_Sleep(10);
	}

	Options_SaveIfChanged();
	Launcher_Free();

#ifdef CC_BUILD_MOBILE
	/* infinite loop on mobile */
	Launcher_ShouldExit = false;
	/* Reset components */
	Platform_LogConst("undoing components");
	Drawer2D_Component.Free();
	Http_Component.Free();
#else
	if (Launcher_ShouldUpdate) {
		const char* action;
		cc_result res = Updater_Start(&action);
		if (res) Logger_SysWarn(res, action);
	}

	if (WindowInfo.Exists) Window_Close();
#endif
}


/*########################################################################################################################*
*---------------------------------------------------------Colours/Skin----------------------------------------------------*
*#########################################################################################################################*/
struct LauncherTheme Launcher_Theme;
const struct LauncherTheme Launcher_ModernTheme = {
	false,
	BitmapCol_Make(153, 127, 172, 255), /* background */
	BitmapCol_Make( 97,  81, 110, 255), /* button border */
	BitmapCol_Make(189, 168, 206, 255), /* active button */
	BitmapCol_Make(141, 114, 165, 255), /* button foreground */
	BitmapCol_Make(162, 131, 186, 255), /* button highlight */
};
const struct LauncherTheme Launcher_ClassicTheme = {
	true,
	BitmapCol_Make( 41,  41,  41, 255), /* background */
	BitmapCol_Make(  0,   0,   0, 255), /* button border */
	BitmapCol_Make(126, 136, 191, 255), /* active button */
	BitmapCol_Make(111, 111, 111, 255), /* button foreground */
	BitmapCol_Make(168, 168, 168, 255), /* button highlight */
};
const struct LauncherTheme Launcher_NordicTheme = {
	false,
	BitmapCol_Make( 46,  52,  64, 255), /* background */
	BitmapCol_Make( 59,  66,  82, 255), /* button border */
	BitmapCol_Make( 66,  74,  90, 255), /* active button */
	BitmapCol_Make( 59,  66,  82, 255), /* button foreground */
	BitmapCol_Make( 76,  86, 106, 255), /* button highlight */
};

CC_NOINLINE static void Launcher_GetCol(const char* key, BitmapCol* col) {
	cc_uint8 rgb[3];
	cc_string value;
	if (!Options_UNSAFE_Get(key, &value))    return;
	if (!PackedCol_TryParseHex(&value, rgb)) return;

	*col = BitmapCol_Make(rgb[0], rgb[1], rgb[2], 255);
}

void Launcher_LoadTheme(void) {
	if (Options_GetBool(OPT_CLASSIC_MODE, false)) {
		Launcher_Theme = Launcher_ClassicTheme;
		return;
	}
	Launcher_Theme = Launcher_ModernTheme;
	Launcher_Theme.ClassicBackground = Options_GetBool("nostalgia-classicbg", false);

	Launcher_GetCol("launcher-back-col",                   &Launcher_Theme.BackgroundColor);
	Launcher_GetCol("launcher-btn-border-col",             &Launcher_Theme.ButtonBorderColor);
	Launcher_GetCol("launcher-btn-fore-active-col",        &Launcher_Theme.ButtonForeActiveColor);
	Launcher_GetCol("launcher-btn-fore-inactive-col",      &Launcher_Theme.ButtonForeColor);
	Launcher_GetCol("launcher-btn-highlight-inactive-col", &Launcher_Theme.ButtonHighlightColor);
}

CC_NOINLINE static void Launcher_SetCol(const char* key, BitmapCol col) {
	cc_string value; char valueBuffer[8];
	/* Component order might be different to BitmapCol */
	PackedCol tmp = PackedCol_Make(BitmapCol_R(col), BitmapCol_G(col), BitmapCol_B(col), 0);
	
	String_InitArray(value, valueBuffer);
	PackedCol_ToHex(&value, tmp);
	Options_Set(key, &value);
}

void Launcher_SaveTheme(void) {
	Launcher_SetCol("launcher-back-col",                   Launcher_Theme.BackgroundColor);
	Launcher_SetCol("launcher-btn-border-col",             Launcher_Theme.ButtonBorderColor);
	Launcher_SetCol("launcher-btn-fore-active-col",        Launcher_Theme.ButtonForeActiveColor);
	Launcher_SetCol("launcher-btn-fore-inactive-col",      Launcher_Theme.ButtonForeColor);
	Launcher_SetCol("launcher-btn-highlight-inactive-col", Launcher_Theme.ButtonHighlightColor);
	Options_SetBool("nostalgia-classicbg",                 Launcher_Theme.ClassicBackground);
}


/*########################################################################################################################*
*---------------------------------------------------------Texture pack----------------------------------------------------*
*#########################################################################################################################*/
static cc_bool Launcher_SelectZipEntry(const cc_string* path) {
	return
		String_CaselessEqualsConst(path, "default.png") ||
		String_CaselessEqualsConst(path, "terrain.png");
}

static void ExtractTerrainTiles(struct Bitmap* bmp) {
	int tileSize = bmp->width / 16;
	Bitmap_Allocate(&dirtBmp,  TILESIZE, TILESIZE);
	Bitmap_Allocate(&stoneBmp, TILESIZE, TILESIZE);

	/* Precompute the scaled background */
	Bitmap_Scale(&dirtBmp,  bmp, 2 * tileSize, 0, tileSize, tileSize);
	Bitmap_Scale(&stoneBmp, bmp, 1 * tileSize, 0, tileSize, tileSize);

	Gradient_Tint(&dirtBmp, 128, 64, 0, 0, TILESIZE, TILESIZE);
	Gradient_Tint(&stoneBmp, 96, 96, 0, 0, TILESIZE, TILESIZE);
}

static cc_result Launcher_ProcessZipEntry(const cc_string* path, struct Stream* data, struct ZipState* s) {
	struct Bitmap bmp;
	cc_result res;

	if (String_CaselessEqualsConst(path, "default.png")) {
		if (hasBitmappedFont) return 0;
		hasBitmappedFont = false;
		res = Png_Decode(&bmp, data);

		if (res) {
			Logger_SysWarn(res, "decoding default.png"); return res;
		} else if (Drawer2D_SetFontBitmap(&bmp)) {
			useBitmappedFont = !Options_GetBool(OPT_USE_CHAT_FONT, false);
			hasBitmappedFont = true;
		} else {
			Mem_Free(bmp.scan0);
		}
	} else if (String_CaselessEqualsConst(path, "terrain.png")) {
		if (dirtBmp.scan0 != NULL) return 0;
		res = Png_Decode(&bmp, data);

		if (res) {
			Logger_SysWarn(res, "decoding terrain.png"); return res;
		} else {
			ExtractTerrainTiles(&bmp);
		}
	}
	return 0;
}

static void ExtractTexturePack(const cc_string* path) {
	struct ZipState state;
	struct Stream stream;
	cc_result res;

	res = Stream_OpenFile(&stream, path);
	if (res == ReturnCode_FileNotFound) return;
	if (res) { Logger_SysWarn(res, "opening texture pack"); return; }

	Zip_Init(&state, &stream);
	state.SelectEntry  = Launcher_SelectZipEntry;
	state.ProcessEntry = Launcher_ProcessZipEntry;
	res = Zip_Extract(&state);

	if (res) { Logger_SysWarn(res, "extracting texture pack"); }
	/* No point logging error for closing readonly file */
	(void)stream.Close(&stream);
}

void Launcher_TryLoadTexturePack(void) {
	static const cc_string defZip = String_FromConst("texpacks/default.zip");
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_string texPack;

	if (Options_UNSAFE_Get(OPT_DEFAULT_TEX_PACK, &texPack)) {
		String_InitArray(path, pathBuffer);
		String_Format1(&path, "texpacks/%s", &texPack);
		ExtractTexturePack(&path);
	}

	/* user selected texture pack is missing some required .png files */
	if (!hasBitmappedFont || dirtBmp.scan0 == NULL) ExtractTexturePack(&defZip);
	Launcher_UpdateLogoFont();
}


/*########################################################################################################################*
*----------------------------------------------------------Background-----------------------------------------------------*
*#########################################################################################################################*/
/* Fills the given area using pixels from the source bitmap, by repeatedly tiling the bitmap */
CC_NOINLINE static void ClearTile(int x, int y, int width, int height, 
								struct Bitmap* dst, struct Bitmap* src) {
	BitmapCol* dstRow;
	BitmapCol* srcRow;
	int xx, yy;
	if (!Drawer2D_Clamp(dst, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		srcRow = Bitmap_GetRow(src, (y + yy) % TILESIZE);
		dstRow = Bitmap_GetRow(dst, y + yy) + x;

		for (xx = 0; xx < width; xx++) {
			dstRow[xx] = srcRow[(x + xx) % TILESIZE];
		}
	}
}

void Launcher_DrawBackground(struct Bitmap* bmp, int x, int y, int width, int height) {
	if (Launcher_Theme.ClassicBackground && dirtBmp.scan0) {
		ClearTile(x, y, width, height, bmp, &stoneBmp);
	} else {
		Gradient_Noise(bmp, Launcher_Theme.BackgroundColor, 6, x, y, width, height);
	}
}

void Launcher_DrawBackgroundAll(struct Bitmap* bmp) {
	if (Launcher_Theme.ClassicBackground && dirtBmp.scan0) {
		ClearTile(0,        0, bmp->width,               TILESIZE, bmp, &dirtBmp);
		ClearTile(0, TILESIZE, bmp->width, bmp->height - TILESIZE, bmp, &stoneBmp);
	} else {
		Launcher_DrawBackground(bmp, 0, 0, bmp->width, bmp->height);
	}
}

void Launcher_UpdateLogoFont(void) {
	Font_Free(&Launcher_LogoFont);
	Drawer2D.BitmappedText = (useBitmappedFont || Launcher_Theme.ClassicBackground) && hasBitmappedFont;
	Drawer2D_MakeFont(&Launcher_LogoFont, 32, FONT_FLAGS_NONE);
	Drawer2D.BitmappedText = false;
}
#endif
