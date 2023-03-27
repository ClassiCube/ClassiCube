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
	CloseActiveScreen();
	Launcher_Active = screen;
	if (!screen->numWidgets) screen->Init(screen);

	screen->Show(screen);
	screen->Layout(screen);

	LBackend_SetScreen(screen);
	LBackend_Redraw();
}

void Launcher_DisplayHttpError(struct HttpRequest* req, const char* action, cc_string* dst) {
	cc_result res = req->result;
	int status    = req->statusCode;

	if (res) {
		/* Non HTTP error - this is not good */
		Http_LogError(action, req);
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
		Options_PauseSaving();
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

static void ConnectToServerError(struct HttpRequest* req) {
	cc_string logMsg = String_Init(NULL, 0, 0);
	Launcher_DisplayHttpError(req, "fetching server info", &logMsg);
}

cc_bool Launcher_ConnectToServer(const cc_string* hash) {
	struct ServerInfo* info;
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
		LWebTask_Tick(&FetchServerTask.Base, ConnectToServerError);
		Thread_Sleep(10); 
	}

	if (FetchServerTask.server.hash.length) {
		StartFromInfo(&FetchServerTask.server);
		return true;
	} else if (FetchServerTask.Base.success) {
		Window_ShowDialog("Failed to connect", "No server has that hash");
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
	Event_Register_(&InputEvents.Wheel,         NULL, OnMouseWheel);

	Utils_EnsureDirectory("texpacks");
	Utils_EnsureDirectory("audio");
}

static void Launcher_Free(void) {
	Event_UnregisterAll();
	LBackend_Free();
	Flags_Free();
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
	CheckUpdateTask_Run();
	Resources_CheckExistence();

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
	BitmapColor_RGB(153, 127, 172), /* background */
	BitmapColor_RGB( 97,  81, 110), /* button border */
	BitmapColor_RGB(189, 168, 206), /* active button */
	BitmapColor_RGB(141, 114, 165), /* button foreground */
	BitmapColor_RGB(162, 131, 186), /* button highlight */
};
const struct LauncherTheme Launcher_ClassicTheme = {
	true,
	BitmapColor_RGB( 41,  41,  41), /* background */
	BitmapColor_RGB(  0,   0,   0), /* button border */
	BitmapColor_RGB(126, 136, 191), /* active button */
	BitmapColor_RGB(111, 111, 111), /* button foreground */
	BitmapColor_RGB(168, 168, 168), /* button highlight */
};
const struct LauncherTheme Launcher_NordicTheme = {
	false,
	BitmapColor_RGB( 46,  52,  64), /* background */
	BitmapColor_RGB( 59,  66,  82), /* button border */
	BitmapColor_RGB( 66,  74,  90), /* active button */
	BitmapColor_RGB( 59,  66,  82), /* button foreground */
	BitmapColor_RGB( 76,  86, 106), /* button highlight */
};

CC_NOINLINE static void ParseColor(const char* key, BitmapCol* color) {
	cc_uint8 rgb[3];
	cc_string value;
	if (!Options_UNSAFE_Get(key, &value))    return;
	if (!PackedCol_TryParseHex(&value, rgb)) return;

	*color = BitmapColor_RGB(rgb[0], rgb[1], rgb[2]);
}

void Launcher_LoadTheme(void) {
	if (Options_GetBool(OPT_CLASSIC_MODE, false)) {
		Launcher_Theme = Launcher_ClassicTheme;
		return;
	}
	Launcher_Theme = Launcher_ModernTheme;
	Launcher_Theme.ClassicBackground = Options_GetBool("nostalgia-classicbg", false);

	ParseColor("launcher-back-col",                   &Launcher_Theme.BackgroundColor);
	ParseColor("launcher-btn-border-col",             &Launcher_Theme.ButtonBorderColor);
	ParseColor("launcher-btn-fore-active-col",        &Launcher_Theme.ButtonForeActiveColor);
	ParseColor("launcher-btn-fore-inactive-col",      &Launcher_Theme.ButtonForeColor);
	ParseColor("launcher-btn-highlight-inactive-col", &Launcher_Theme.ButtonHighlightColor);
}

CC_NOINLINE static void SaveColor(const char* key, BitmapCol color) {
	cc_string value; char valueBuffer[6];
	
	String_InitArray(value, valueBuffer);
	String_AppendHex(&value, BitmapCol_R(color));
	String_AppendHex(&value, BitmapCol_G(color));
	String_AppendHex(&value, BitmapCol_B(color));
	Options_Set(key, &value);
}

void Launcher_SaveTheme(void) {
	SaveColor("launcher-back-col",                   Launcher_Theme.BackgroundColor);
	SaveColor("launcher-btn-border-col",             Launcher_Theme.ButtonBorderColor);
	SaveColor("launcher-btn-fore-active-col",        Launcher_Theme.ButtonForeActiveColor);
	SaveColor("launcher-btn-fore-inactive-col",      Launcher_Theme.ButtonForeColor);
	SaveColor("launcher-btn-highlight-inactive-col", Launcher_Theme.ButtonHighlightColor);
	Options_SetBool("nostalgia-classicbg",                 Launcher_Theme.ClassicBackground);
}


/*########################################################################################################################*
*---------------------------------------------------------Texture pack----------------------------------------------------*
*#########################################################################################################################*/
/* Tints the given area, linearly interpolating from a to b */
/*  Note that this only tints RGB, A is not tinted */
static void TintBitmap(struct Bitmap* bmp, cc_uint8 tintA, cc_uint8 tintB, int width, int height) {
	BitmapCol* row;
	cc_uint8 tint;
	int xx, yy;

	for (yy = 0; yy < height; yy++) {
		row  = Bitmap_GetRow(bmp, yy);
		tint = (cc_uint8)Math_Lerp(tintA, tintB, (float)yy / height);

		for (xx = 0; xx < width; xx++) {
			/* TODO: Not shift when multiplying */
			row[xx] = BitmapColor_RGB(
				BitmapCol_R(row[xx]) * tint / 255,
				BitmapCol_G(row[xx]) * tint / 255,
				BitmapCol_B(row[xx]) * tint / 255);
		}
	}
}

static void ExtractTerrainTiles(struct Bitmap* bmp) {
	int tileSize = bmp->width / 16;
	Bitmap_Allocate(&dirtBmp,  TILESIZE, TILESIZE);
	Bitmap_Allocate(&stoneBmp, TILESIZE, TILESIZE);

	/* Precompute the scaled background */
	Bitmap_Scale(&dirtBmp,  bmp, 2 * tileSize, 0, tileSize, tileSize);
	Bitmap_Scale(&stoneBmp, bmp, 1 * tileSize, 0, tileSize, tileSize);

	TintBitmap(&dirtBmp, 128, 64, TILESIZE, TILESIZE);
	TintBitmap(&stoneBmp, 96, 96, TILESIZE, TILESIZE);
}

static cc_bool Launcher_SelectZipEntry(const cc_string* path) {
	return
		String_CaselessEqualsConst(path, "default.png") ||
		String_CaselessEqualsConst(path, "terrain.png");
}

static cc_result Launcher_ProcessZipEntry(const cc_string* path, struct Stream* data, struct ZipEntry* source) {
	struct Bitmap bmp;
	cc_result res;

	if (String_CaselessEqualsConst(path, "default.png")) {
		if (hasBitmappedFont) return 0;
		hasBitmappedFont = false;
		res = Png_Decode(&bmp, data);

		if (res) {
			Logger_SysWarn(res, "decoding default.png"); return res;
		} else if (Font_SetBitmapAtlas(&bmp)) {
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
	struct Stream stream;
	cc_result res;

	res = Stream_OpenFile(&stream, path);
	if (res == ReturnCode_FileNotFound) return;
	if (res) { Logger_SysWarn(res, "opening texture pack"); return; }

	res = Zip_Extract(&stream, 
			Launcher_SelectZipEntry, Launcher_ProcessZipEntry);

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
	LBackend_UpdateLogoFont();
}


/*########################################################################################################################*
*----------------------------------------------------------Background-----------------------------------------------------*
*#########################################################################################################################*/
/* Fills the given area using pixels from the source bitmap, by repeatedly tiling the bitmap */
CC_NOINLINE static void ClearTile(int x, int y, int width, int height, 
								struct Context2D* ctx, struct Bitmap* src) {
	struct Bitmap* dst = (struct Bitmap*)ctx;
	BitmapCol* dstRow;
	BitmapCol* srcRow;
	int xx, yy;
	if (!Drawer2D_Clamp(ctx, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		srcRow = Bitmap_GetRow(src, (y + yy) % TILESIZE);
		dstRow = Bitmap_GetRow(dst, y + yy) + x;

		for (xx = 0; xx < width; xx++) {
			dstRow[xx] = srcRow[(x + xx) % TILESIZE];
		}
	}
}

void Launcher_DrawBackground(struct Context2D* ctx, int x, int y, int width, int height) {
	if (Launcher_Theme.ClassicBackground && dirtBmp.scan0) {
		ClearTile(x, y, width, height, ctx, &stoneBmp);
	} else {
		Gradient_Noise(ctx, Launcher_Theme.BackgroundColor, 6, x, y, width, height);
	}
}

void Launcher_DrawBackgroundAll(struct Context2D* ctx) {
	if (Launcher_Theme.ClassicBackground && dirtBmp.scan0) {
		ClearTile(0,        0, ctx->width,               TILESIZE, ctx, &dirtBmp);
		ClearTile(0, TILESIZE, ctx->width, ctx->height - TILESIZE, ctx, &stoneBmp);
	} else {
		Launcher_DrawBackground(ctx, 0, 0, ctx->width, ctx->height);
	}
}

cc_bool Launcher_BitmappedText(void) {
	return (useBitmappedFont || Launcher_Theme.ClassicBackground) && hasBitmappedFont;
}

void Launcher_DrawLogo(struct FontDesc* font, const char* text, struct Context2D* ctx) {
	cc_string title = String_FromReadonly(text);
	struct DrawTextArgs args;
	int x;

	DrawTextArgs_Make(&args, &title, font, false);
	x = ctx->width / 2 - Drawer2D_TextWidth(&args) / 2;

	Drawer2D.Colors['f'] = BITMAPCOLOR_BLACK;
	Context2D_DrawText(ctx, &args, x + Display_ScaleX(4), Display_ScaleY(4));
	Drawer2D.Colors['f'] = BITMAPCOLOR_WHITE;
	Context2D_DrawText(ctx, &args, x,                     0);
}

void Launcher_MakeLogoFont(struct FontDesc* font) {
	Drawer2D.BitmappedText = Launcher_BitmappedText();
	Font_Make(font, 32, FONT_FLAGS_NONE);
	Drawer2D.BitmappedText = false;
}
#endif
