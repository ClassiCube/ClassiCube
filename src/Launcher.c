#include "Launcher.h"
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

#ifndef CC_BUILD_WEB
struct LScreen* Launcher_Screen;
Rect2D Launcher_Dirty;
Bitmap Launcher_Framebuffer;
cc_bool Launcher_ClassicBackground;
struct FontDesc Launcher_TitleFont, Launcher_TextFont, Launcher_HintFont;

static cc_bool pendingRedraw;
static struct FontDesc logoFont;

cc_bool Launcher_ShouldExit, Launcher_ShouldUpdate;
static char hashBuffer[STRING_SIZE];
String Launcher_AutoHash = String_FromArray(hashBuffer);
static void Launcher_ApplyUpdate(void);

void Launcher_SetScreen(struct LScreen* screen) {
	if (Launcher_Screen) Launcher_Screen->Free(Launcher_Screen);
	Launcher_Screen = screen;
	if (!screen->numWidgets) screen->Init(screen);

	screen->Show(screen);
	screen->Layout(screen);
	/* for hovering over active button etc */
	screen->MouseMove(screen, 0, 0);

	Launcher_Redraw();
}

CC_NOINLINE static void StartFromInfo(struct ServerInfo* info) {
	String port; char portBuffer[STRING_INT_CHARS];
	String_InitArray(port, portBuffer);

	String_AppendInt(&port, info->port);
	Launcher_StartGame(&SignInTask.username, &info->mppass, &info->ip, &port, &info->name);
}

cc_bool Launcher_ConnectToServer(const String* hash) {
	struct ServerInfo* info;
	String logMsg;
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

static CC_NOINLINE void InitFramebuffer(void) {
	Launcher_Framebuffer.width = max(WindowInfo.Width, 1);
	Launcher_Framebuffer.height = max(WindowInfo.Height, 1);
	Window_AllocFramebuffer(&Launcher_Framebuffer);
}


/*########################################################################################################################*
*---------------------------------------------------------Event handler---------------------------------------------------*
*#########################################################################################################################*/
static void ReqeustRedraw(void* obj) {
	/* We may get multiple Redraw events in short timespan */
	/* So we just request a redraw at next launcher tick */
	pendingRedraw  = true;
	Launcher_MarkAllDirty();
}

static void OnResize(void* obj) {
	Window_FreeFramebuffer(&Launcher_Framebuffer);
	InitFramebuffer();

	if (Launcher_Screen) Launcher_Screen->Layout(Launcher_Screen);
	Launcher_Redraw();
}

static cc_bool IsShutdown(int key) {
	if (key == KEY_F4 && Key_IsAltPressed()) return true;

	/* On macOS, Cmd+Q should also terminate the process */
#ifdef CC_BUILD_OSX
	return key == 'Q' && Key_IsWinPressed();
#else
	return false;
#endif
}

static void HandleInputDown(void* obj, int key, cc_bool was) {
	if (IsShutdown(key)) Launcher_ShouldExit = true;
	Launcher_Screen->KeyDown(Launcher_Screen, key, was);
}

static void HandleKeyPress(void* obj, int c) {
	Launcher_Screen->KeyPress(Launcher_Screen, c);
}

static void HandleMouseWheel(void* obj, float delta) {
	Launcher_Screen->MouseWheel(Launcher_Screen, delta);
}

static void HandlePointerDown(void* obj, int idx) {
	Launcher_Screen->MouseDown(Launcher_Screen, 0);
}

static void HandlePointerUp(void* obj, int idx) {
	Launcher_Screen->MouseUp(Launcher_Screen, 0);
}

static void HandlePointerMove(void* obj, int idx, int deltaX, int deltaY) {
	if (!Launcher_Screen) return;
	Launcher_Screen->MouseMove(Launcher_Screen, deltaX, deltaY);
}


/*########################################################################################################################*
*-----------------------------------------------------------Main body-----------------------------------------------------*
*#########################################################################################################################*/
static void Launcher_Display(void) {
	if (pendingRedraw) {
		Launcher_Redraw();
		pendingRedraw = false;
	}

	Window_DrawFramebuffer(Launcher_Dirty);
	Launcher_Dirty.X = 0; Launcher_Dirty.Width   = 0;
	Launcher_Dirty.Y = 0; Launcher_Dirty.Height  = 0;
}

static void Launcher_Init(void) {
	Event_RegisterVoid(&WindowEvents.Resized,      NULL, OnResize);
	Event_RegisterVoid(&WindowEvents.StateChanged, NULL, OnResize);
	Event_RegisterVoid(&WindowEvents.Redraw,       NULL, ReqeustRedraw);

	Event_RegisterInput(&InputEvents.Down,   NULL, HandleInputDown);
	Event_RegisterInt(&InputEvents.Press,    NULL, HandleKeyPress);
	Event_RegisterFloat(&InputEvents.Wheel,  NULL, HandleMouseWheel);
	Event_RegisterInt(&PointerEvents.Down,   NULL, HandlePointerDown);
	Event_RegisterInt(&PointerEvents.Up,     NULL, HandlePointerUp);
	Event_RegisterMove(&PointerEvents.Moved, NULL, HandlePointerMove);

	Drawer2D_MakeFont(&logoFont,           32, FONT_STYLE_NORMAL);
	Drawer2D_MakeFont(&Launcher_TitleFont, 16, FONT_STYLE_BOLD);
	Drawer2D_MakeFont(&Launcher_TextFont,  14, FONT_STYLE_NORMAL);
	Drawer2D_MakeFont(&Launcher_HintFont,  12, FONT_STYLE_ITALIC);

	Drawer2D_Cols['g'] = BitmapCol_Make(125, 125, 125, 255);
	Utils_EnsureDirectory("texpacks");
	Utils_EnsureDirectory("audio");
}

static void Launcher_Free(void) {
	Event_UnregisterVoid(&WindowEvents.Resized,      NULL, OnResize);
	Event_UnregisterVoid(&WindowEvents.StateChanged, NULL, OnResize);
	Event_UnregisterVoid(&WindowEvents.Redraw,       NULL, ReqeustRedraw);
	
	Event_UnregisterInput(&InputEvents.Down,    NULL, HandleInputDown);
	Event_UnregisterInt(&InputEvents.Press,     NULL, HandleKeyPress);
	Event_UnregisterFloat(&InputEvents.Wheel,   NULL, HandleMouseWheel);
	Event_UnregisterInt(&PointerEvents.Down,    NULL, HandlePointerDown);
	Event_UnregisterInt(&PointerEvents.Up,      NULL, HandlePointerUp);
	Event_UnregisterMove(&PointerEvents.Moved,  NULL, HandlePointerMove);	

	Flags_Free();
	Font_Free(&logoFont);
	Font_Free(&Launcher_TitleFont);
	Font_Free(&Launcher_TextFont);
	Font_Free(&Launcher_HintFont);

	Launcher_Screen->Free(Launcher_Screen);
	Launcher_Screen = NULL;
	Window_FreeFramebuffer(&Launcher_Framebuffer);
}

#ifdef CC_BUILD_ANDROID
static cc_bool winCreated;
static void OnWindowCreated(void* obj) { winCreated = true; }
int Program_Run(int argc, char** argv);

static void SwitchToGame() {
	JNIEnv* env;
	JavaGetCurrentEnv(env);

	/* Reset components */
	Platform_LogConst("undoing components");
	Drawer2D_Component.Free();
	//Http_Component.Free();

	/* Force window to be destroyed and re-created */
	/* (see comments in setupForGame for why this has to be done) */
	JavaCallVoid(env, "setupForGame", "()V", NULL);
	Event_RegisterVoid(&WindowEvents.Created, NULL, OnWindowCreated);
	Platform_LogConst("Entering wait for window loop..");

	/* Loop until window gets created async */
	while (WindowInfo.Exists && !winCreated) {
		Window_ProcessEvents();
		Thread_Sleep(10);
	}

	Platform_LogConst("OK I'm starting the game..");
	Event_UnregisterVoid(&WindowEvents.Created, NULL, OnWindowCreated);
	if (winCreated) Program_Run(0, NULL);
}
#endif

void Launcher_Run(void) {
	static const String title = String_FromConst(GAME_APP_TITLE);
	Window_Create(640, 400);
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
	Drawer2D_BitmappedText    = false;
	Drawer2D_BlackTextShadows = true;
	InitFramebuffer();

	Launcher_LoadSkin();
	Launcher_Init();
	Launcher_TryLoadTexturePack();

	Http_Component.Init();
	Resources_CheckExistence();
	CheckUpdateTask_Run();

	if (Resources_Count) {
		Launcher_SetScreen(ResourcesScreen_MakeInstance());
	} else {
		Launcher_SetScreen(MainScreen_MakeInstance());
	}

	for (;;) {
		Window_ProcessEvents();
		if (!WindowInfo.Exists || Launcher_ShouldExit) break;

		Launcher_Screen->Tick(Launcher_Screen);
		if (Launcher_Dirty.Width) Launcher_Display();
		Thread_Sleep(10);
	}

	Options_SaveIfChanged();
	Launcher_Free();
	if (Launcher_ShouldUpdate) Launcher_ApplyUpdate();

#ifdef CC_BUILD_ANDROID
	if (Launcher_ShouldExit) SwitchToGame();
#endif
	if (WindowInfo.Exists) Window_Close();
}


/*########################################################################################################################*
*---------------------------------------------------------Colours/Skin----------------------------------------------------*
*#########################################################################################################################*/
#define DEFAULT_BACKGROUND_COL         BitmapCol_Make(153, 127, 172, 255)
#define DEFAULT_BUTTON_BORDER_COL      BitmapCol_Make( 97,  81, 110, 255)
#define DEFAULT_BUTTON_FORE_ACTIVE_COL BitmapCol_Make(189, 168, 206, 255)
#define DEFAULT_BUTTON_FORE_COL        BitmapCol_Make(141, 114, 165, 255)
#define DEFAULT_BUTTON_HIGHLIGHT_COL   BitmapCol_Make(162, 131, 186, 255)

BitmapCol Launcher_BackgroundCol       = DEFAULT_BACKGROUND_COL;
BitmapCol Launcher_ButtonBorderCol     = DEFAULT_BUTTON_BORDER_COL;
BitmapCol Launcher_ButtonForeActiveCol = DEFAULT_BUTTON_FORE_ACTIVE_COL;
BitmapCol Launcher_ButtonForeCol       = DEFAULT_BUTTON_FORE_COL;
BitmapCol Launcher_ButtonHighlightCol  = DEFAULT_BUTTON_HIGHLIGHT_COL;

void Launcher_ResetSkin(void) {
	Launcher_BackgroundCol       = DEFAULT_BACKGROUND_COL;
	Launcher_ButtonBorderCol     = DEFAULT_BUTTON_BORDER_COL;
	Launcher_ButtonForeActiveCol = DEFAULT_BUTTON_FORE_ACTIVE_COL;
	Launcher_ButtonForeCol       = DEFAULT_BUTTON_FORE_COL;
	Launcher_ButtonHighlightCol  = DEFAULT_BUTTON_HIGHLIGHT_COL;
}

CC_NOINLINE static void Launcher_GetCol(const char* key, BitmapCol* col) {
	cc_uint8 rgb[3];
	String value;
	if (!Options_UNSAFE_Get(key, &value))    return;
	if (!PackedCol_TryParseHex(&value, rgb)) return;

	*col = BitmapCol_Make(rgb[0], rgb[1], rgb[2], 255);
}

void Launcher_LoadSkin(void) {
	Launcher_GetCol("launcher-back-col",                   &Launcher_BackgroundCol);
	Launcher_GetCol("launcher-btn-border-col",             &Launcher_ButtonBorderCol);
	Launcher_GetCol("launcher-btn-fore-active-col",        &Launcher_ButtonForeActiveCol);
	Launcher_GetCol("launcher-btn-fore-inactive-col",      &Launcher_ButtonForeCol);
	Launcher_GetCol("launcher-btn-highlight-inactive-col", &Launcher_ButtonHighlightCol);
}

CC_NOINLINE static void Launcher_SetCol(const char* key, BitmapCol col) {
	String value; char valueBuffer[8];
	/* Component order might be different to BitmapCol */
	PackedCol tmp = PackedCol_Make(BitmapCol_R(col), BitmapCol_G(col), BitmapCol_B(col), 0);
	
	String_InitArray(value, valueBuffer);
	PackedCol_ToHex(&value, tmp);
	Options_Set(key, &value);
}

void Launcher_SaveSkin(void) {
	Launcher_SetCol("launcher-back-col",                   Launcher_BackgroundCol);
	Launcher_SetCol("launcher-btn-border-col",             Launcher_ButtonBorderCol);
	Launcher_SetCol("launcher-btn-fore-active-col",        Launcher_ButtonForeActiveCol);
	Launcher_SetCol("launcher-btn-fore-inactive-col",      Launcher_ButtonForeCol);
	Launcher_SetCol("launcher-btn-highlight-inactive-col", Launcher_ButtonHighlightCol);
}


/*########################################################################################################################*
*----------------------------------------------------------Background-----------------------------------------------------*
*#########################################################################################################################*/
static cc_bool useBitmappedFont;
static Bitmap dirtBmp, stoneBmp, fontBmp;
#define TILESIZE 48

static cc_bool Launcher_SelectZipEntry(const String* path) {
	return
		String_CaselessEqualsConst(path, "default.png") ||
		String_CaselessEqualsConst(path, "terrain.png");
}

static void Launcher_LoadTextures(Bitmap* bmp) {
	int tileSize = bmp->width / 16;
	Bitmap_Allocate(&dirtBmp,  TILESIZE, TILESIZE);
	Bitmap_Allocate(&stoneBmp, TILESIZE, TILESIZE);

	/* Precompute the scaled background */
	Bitmap_Scale(&dirtBmp,  bmp, 2 * tileSize, 0, tileSize, tileSize);
	Bitmap_Scale(&stoneBmp, bmp, 1 * tileSize, 0, tileSize, tileSize);

	Gradient_Tint(&dirtBmp, 128, 64, 0, 0, TILESIZE, TILESIZE);
	Gradient_Tint(&stoneBmp, 96, 96, 0, 0, TILESIZE, TILESIZE);
}

static cc_result Launcher_ProcessZipEntry(const String* path, struct Stream* data, struct ZipState* s) {
	Bitmap bmp;
	cc_result res;

	if (String_CaselessEqualsConst(path, "default.png")) {
		if (fontBmp.scan0) return 0;
		res = Png_Decode(&fontBmp, data);

		if (res) {
			Logger_Warn(res, "decoding default.png"); return res;
		} else if (Drawer2D_SetFontBitmap(&fontBmp)) {
			useBitmappedFont = !Options_GetBool(OPT_USE_CHAT_FONT, false);
		} else {
			Mem_Free(fontBmp.scan0);
			fontBmp.scan0 = NULL;
		}
	} else if (String_CaselessEqualsConst(path, "terrain.png")) {
		if (dirtBmp.scan0) return 0;
		res = Png_Decode(&bmp, data);

		if (res) {
			Logger_Warn(res, "decoding terrain.png"); return res;
		} else {
			Launcher_LoadTextures(&bmp);
		}
	}
	return 0;
}

static void Launcher_ExtractTexturePack(const String* path) {
	struct ZipState state;
	struct Stream stream;
	cc_result res;

	res = Stream_OpenFile(&stream, path);
	if (res) { Logger_Warn(res, "opening texture pack"); return; }

	Zip_Init(&state, &stream);
	state.SelectEntry  = Launcher_SelectZipEntry;
	state.ProcessEntry = Launcher_ProcessZipEntry;
	res = Zip_Extract(&state);

	if (res) { Logger_Warn(res, "extracting texture pack"); }
	stream.Close(&stream);
}

void Launcher_TryLoadTexturePack(void) {
	static const String defZipPath = String_FromConst("texpacks/default.zip");
	String path; char pathBuffer[FILENAME_SIZE];
	String texPack;

	if (Options_UNSAFE_Get("nostalgia-classicbg", &texPack)) {
		Launcher_ClassicBackground = Options_GetBool("nostalgia-classicbg", false);
	} else {
		Launcher_ClassicBackground = Options_GetBool(OPT_CLASSIC_MODE,      false);
	}

	Options_UNSAFE_Get(OPT_DEFAULT_TEX_PACK, &texPack);
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "texpacks/%s", &texPack);

	if (!texPack.length || !File_Exists(&path)) path = defZipPath;
	if (!File_Exists(&path)) return;

	Launcher_ExtractTexturePack(&path);
	/* user selected texture pack is missing some required .png files */
	if (!fontBmp.scan0 || !dirtBmp.scan0) {
		Launcher_ExtractTexturePack(&defZipPath);
	}
}

/* Fills the given area using pixels from the source bitmap, by repeatedly tiling the bitmap. */
CC_NOINLINE static void Launcher_ClearTile(int x, int y, int width, int height, Bitmap* src) {
	Bitmap* dst = &Launcher_Framebuffer;
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

void Launcher_ResetArea(int x, int y, int width, int height) {
	if (Launcher_ClassicBackground && dirtBmp.scan0) {
		Launcher_ClearTile(x, y, width, height, &stoneBmp);
	} else {
		Gradient_Noise(&Launcher_Framebuffer, Launcher_BackgroundCol, 6, x, y, width, height);
	}
	Launcher_MarkDirty(x, y, width, height);
}

void Launcher_ResetPixels(void) {
	static const String title_fore = String_FromConst("&eClassi&fCube");
	static const String title_back = String_FromConst("&0Classi&0Cube");
	struct DrawTextArgs args;
	int x;

	if (Launcher_Screen && Launcher_Screen->hidesTitlebar) {
		Launcher_ResetArea(0, 0, WindowInfo.Width, WindowInfo.Height);
		return;
	}

	if (Launcher_ClassicBackground && dirtBmp.scan0) {
		Launcher_ClearTile(0,        0, WindowInfo.Width,                     TILESIZE, &dirtBmp);
		Launcher_ClearTile(0, TILESIZE, WindowInfo.Width, WindowInfo.Height - TILESIZE, &stoneBmp);
	} else {
		Launcher_ResetArea(0, 0, WindowInfo.Width, WindowInfo.Height);
	}

	Drawer2D_BitmappedText = (useBitmappedFont || Launcher_ClassicBackground) && fontBmp.scan0;
	DrawTextArgs_Make(&args, &title_fore, &logoFont, false);
	x = WindowInfo.Width / 2 - Drawer2D_TextWidth(&args) / 2;

	args.text = title_back;
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, x + 4, 4);
	args.text = title_fore;
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, x, 0);

	Drawer2D_BitmappedText = false;
	Launcher_MarkAllDirty();
}

void Launcher_Redraw(void) {
	Launcher_ResetPixels();
	Launcher_Screen->Draw(Launcher_Screen);
	Launcher_MarkAllDirty();
}

void Launcher_MarkDirty(int x, int y, int width, int height) {
	int x1, y1, x2, y2;
	if (!Drawer2D_Clamp(&Launcher_Framebuffer, &x, &y, &width, &height)) return;

	/* union with existing dirty area */
	if (Launcher_Dirty.Width) {
		x1 = min(x, Launcher_Dirty.X);
		y1 = min(y, Launcher_Dirty.Y);

		x2 = max(x +  width, Launcher_Dirty.X + Launcher_Dirty.Width);
		y2 = max(y + height, Launcher_Dirty.Y + Launcher_Dirty.Height);

		x = x1; width  = x2 - x1;
		y = y1; height = y2 - y1;
	}

	Launcher_Dirty.X = x; Launcher_Dirty.Width  = width;
	Launcher_Dirty.Y = y; Launcher_Dirty.Height = height;
}

void Launcher_MarkAllDirty(void) {
	Launcher_Dirty.X = 0; Launcher_Dirty.Width  = Launcher_Framebuffer.width;
	Launcher_Dirty.Y = 0; Launcher_Dirty.Height = Launcher_Framebuffer.height;
}


/*########################################################################################################################*
*--------------------------------------------------------Starter/Updater--------------------------------------------------*
*#########################################################################################################################*/
static TimeMS lastJoin;
cc_bool Launcher_StartGame(const String* user, const String* mppass, const String* ip, const String* port, const String* server) {
	String args; char argsBuffer[512];
	TimeMS now;
	cc_result res;
	
	now = DateTime_CurrentUTC_MS();
	if (lastJoin + 1000 > now) return false;
	lastJoin = now;

	/* Save resume info */
	if (server->length) {
		Options_Set("launcher-server",   server);
		Options_Set("launcher-username", user);
		Options_Set("launcher-ip",       ip);
		Options_Set("launcher-port",     port);
		Options_SetSecure("launcher-mppass", mppass, user);
	}
	/* Save options BEFORE starting new game process */
	/* Otherwise can get 'file already in use' errors on startup */
	Options_SaveIfChanged();

	String_InitArray(args, argsBuffer);
	String_AppendString(&args, user);
	if (mppass->length) String_Format3(&args, " %s %s %s", mppass, ip, port);

	res = Process_StartGame(&args);
	if (res) { Logger_Warn(res, "starting game"); return false; }

#ifdef CC_BUILD_ANDROID
	Launcher_ShouldExit = true;
#else
	Launcher_ShouldExit = Options_GetBool(OPT_AUTO_CLOSE_LAUNCHER, false);
#endif
	return true;
}

static void Launcher_ApplyUpdate(void) {
	cc_result res = Updater_Start();
	if (res) Logger_Warn(res, "running updater");
}

void Launcher_DisplayHttpError(cc_result res, int status, const char* action, String* dst) {
	if (res) {
		/* Non HTTP error - this is not good */
		Logger_SysWarn(res, action, Http_DescribeError);
		String_Format2(dst, "&cError %i when %c", &res, action);
	} else if (status != 200) {
		String_Format2(dst, "&c%i error when %c", &status, action);
	} else {
		String_Format1(dst, "&cEmpty response when %c", action);
	}
}
#endif
