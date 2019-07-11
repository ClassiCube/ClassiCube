#include "Launcher.h"
#include "LScreens.h"
#include "LWeb.h"
#include "Resources.h"
#include "Drawer2D.h"
#include "Game.h"
#include "Deflate.h"
#include "Stream.h"
#include "Utils.h"
#include "Input.h"
#include "Window.h"
#include "GameStructs.h"
#include "Event.h"
#include "Http.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Logger.h"

#ifndef CC_BUILD_WEB
struct LScreen* Launcher_Screen;
Rect2D Launcher_Dirty;
Bitmap Launcher_Framebuffer;
bool Launcher_ClassicBackground;
FontDesc Launcher_TitleFont, Launcher_TextFont, Launcher_HintFont;

static bool pendingRedraw;
static FontDesc logoFont;

bool Launcher_ShouldExit, Launcher_ShouldUpdate;
static void Launcher_ApplyUpdate(void);

void Launcher_SetScreen(struct LScreen* screen) {
	if (Launcher_Screen) Launcher_Screen->Free(Launcher_Screen);
	Launcher_Screen = screen;

	screen->Init(screen);
	screen->Reposition(screen);
	/* for hovering over active button etc */
	screen->MouseMove(screen, 0, 0);

	Launcher_Redraw();
}

CC_NOINLINE static void Launcher_StartFromInfo(struct ServerInfo* info) {
	String port; char portBuffer[STRING_INT_CHARS];
	String_InitArray(port, portBuffer);

	String_AppendInt(&port, info->port);
	Launcher_StartGame(&SignInTask.Username, &info->mppass, &info->ip, &port, &info->name);
}

bool Launcher_ConnectToServer(const String* hash) {
	struct ServerInfo* info;
	String logMsg;
	int i;
	if (!hash->length) return false;

	for (i = 0; i < FetchServersTask.NumServers; i++) {
		info = &FetchServersTask.Servers[i];
		if (!String_Equals(hash, &info->hash)) continue;

		Launcher_StartFromInfo(info);
		return true;
	}

	/* Fallback to private server handling */
	/* TODO: Rewrite to be async */
	FetchServerTask_Run(hash);

	while (!FetchServerTask.Base.Completed) { 
		LWebTask_Tick(&FetchServerTask.Base);
		Thread_Sleep(10); 
	}

	if (FetchServerTask.Server.hash.length) {
		Launcher_StartFromInfo(&FetchServerTask.Server);
		return true;
	} else if (FetchServerTask.Base.Success) {
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
static void Launcher_MaybeRedraw(void* obj) {
	/* Only redraw when launcher has been initialised */
	if (Launcher_Screen && Window_Exists) Launcher_Redraw();
}

static void Launcher_ReqeustRedraw(void* obj) {
	/* We may get multiple Redraw events in short timespan */
	/* So we just request a redraw at next launcher tick */
	pendingRedraw  = true;
	Launcher_MarkAllDirty();
}

static void Launcher_OnResize(void* obj) {
	Game_UpdateDimensions();
	Launcher_Framebuffer.Width  = Game.Width;
	Launcher_Framebuffer.Height = Game.Height;

	Window_InitRaw(&Launcher_Framebuffer);
	if (Launcher_Screen) Launcher_Screen->Reposition(Launcher_Screen);
	Launcher_Redraw();
}

static bool Launcher_IsShutdown(int key) {
	if (key == KEY_F4 && Key_IsAltPressed()) return true;

	/* On OSX, Cmd+Q should also terminate the process */
#ifdef CC_BUILD_OSX
	return key == 'Q' && Key_IsWinPressed();
#else
	return false;
#endif
}

static void Launcher_KeyDown(void* obj, int key, bool was) {
	if (Launcher_IsShutdown(key)) Launcher_ShouldExit = true;
	Launcher_Screen->KeyDown(Launcher_Screen, key, was);
}

static void Launcher_KeyPress(void* obj, int c) {
	Launcher_Screen->KeyPress(Launcher_Screen, c);
}

static void Launcher_MouseDown(void* obj, int btn) {
	Launcher_Screen->MouseDown(Launcher_Screen, btn);
}

static void Launcher_MouseUp(void* obj, int btn) {
	Launcher_Screen->MouseUp(Launcher_Screen, btn);
}

static void Launcher_MouseMove(void* obj, int deltaX, int deltaY) {
	if (!Launcher_Screen) return;
	Launcher_Screen->MouseMove(Launcher_Screen, deltaX, deltaY);
}

static void Launcher_MouseWheel(void* obj, float delta) {
	Launcher_Screen->MouseWheel(Launcher_Screen, delta);
}


/*########################################################################################################################*
*-----------------------------------------------------------Main body-----------------------------------------------------*
*#########################################################################################################################*/
static void Launcher_Display(void) {
	if (pendingRedraw) {
		Launcher_Redraw();
		pendingRedraw = false;
	}

	Launcher_Screen->OnDisplay(Launcher_Screen);
	Window_DrawRaw(Launcher_Dirty);

	Launcher_Dirty.X = 0; Launcher_Dirty.Width   = 0;
	Launcher_Dirty.Y = 0; Launcher_Dirty.Height  = 0;
}

static void Launcher_Init(void) {
	BitmapCol col = BITMAPCOL_CONST(125, 125, 125, 255);

	Event_RegisterVoid(&WindowEvents.Resized,      NULL, Launcher_OnResize);
	Event_RegisterVoid(&WindowEvents.StateChanged, NULL, Launcher_OnResize);
	Event_RegisterVoid(&WindowEvents.FocusChanged, NULL, Launcher_MaybeRedraw);
	Event_RegisterVoid(&WindowEvents.Redraw,       NULL, Launcher_ReqeustRedraw);

	Event_RegisterInput(&KeyEvents.Down,        NULL, Launcher_KeyDown);
	Event_RegisterInt(&KeyEvents.Press,         NULL, Launcher_KeyPress);
	Event_RegisterInt(&MouseEvents.Down,        NULL, Launcher_MouseDown);
	Event_RegisterInt(&MouseEvents.Up,          NULL, Launcher_MouseUp);
	Event_RegisterMouseMove(&MouseEvents.Moved, NULL, Launcher_MouseMove);
	Event_RegisterFloat(&MouseEvents.Wheel,     NULL, Launcher_MouseWheel);

	Drawer2D_MakeFont(&logoFont,           32, FONT_STYLE_NORMAL);
	Drawer2D_MakeFont(&Launcher_TitleFont, 16, FONT_STYLE_BOLD);
	Drawer2D_MakeFont(&Launcher_TextFont,  14, FONT_STYLE_NORMAL);
	Drawer2D_MakeFont(&Launcher_HintFont,  12, FONT_STYLE_ITALIC);

	Drawer2D_Cols['g'] = col;
	Utils_EnsureDirectory("texpacks");
	Utils_EnsureDirectory("audio");
}

static void Launcher_Free(void) {
	Event_UnregisterVoid(&WindowEvents.Resized,      NULL, Launcher_OnResize);
	Event_UnregisterVoid(&WindowEvents.StateChanged, NULL, Launcher_OnResize);
	Event_UnregisterVoid(&WindowEvents.FocusChanged, NULL, Launcher_MaybeRedraw);
	Event_UnregisterVoid(&WindowEvents.Redraw,       NULL, Launcher_ReqeustRedraw);
	
	Event_UnregisterInput(&KeyEvents.Down,        NULL, Launcher_KeyDown);
	Event_UnregisterInt(&KeyEvents.Press,         NULL, Launcher_KeyPress);
	Event_UnregisterInt(&MouseEvents.Down,        NULL, Launcher_MouseDown);
	Event_UnregisterInt(&MouseEvents.Up,          NULL, Launcher_MouseUp);
	Event_UnregisterMouseMove(&MouseEvents.Moved, NULL, Launcher_MouseMove);
	Event_UnregisterFloat(&MouseEvents.Wheel,     NULL, Launcher_MouseWheel);

	Flags_Free();
	Font_Free(&logoFont);
	Font_Free(&Launcher_TitleFont);
	Font_Free(&Launcher_TextFont);
	Font_Free(&Launcher_HintFont);

	Launcher_Screen->Free(Launcher_Screen);
	Launcher_Screen = NULL;
}

void Launcher_Run(void) {
	static const String title = String_FromConst(GAME_APP_TITLE);
	Window_CreateSimple(640, 400);
	Window_SetTitle(&title);
	Window_SetVisible(true);

	Drawer2D_Component.Init();
	Game_UpdateDimensions();
	Drawer2D_BitmappedText    = false;
	Drawer2D_BlackTextShadows = true;

	Launcher_LoadSkin();
	Launcher_Init();
	Launcher_TryLoadTexturePack();

	Launcher_Framebuffer.Width  = Game.Width;
	Launcher_Framebuffer.Height = Game.Height;
	Window_InitRaw(&Launcher_Framebuffer);

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
		if (!Window_Exists || Launcher_ShouldExit) break;

		Launcher_Screen->Tick(Launcher_Screen);
		if (Launcher_Dirty.Width) Launcher_Display();
		Thread_Sleep(10);
	}

	if (Options_ChangedCount()) {
		Options_Load();
		Options_Save();
	}

	Launcher_Free();
	if (Launcher_ShouldUpdate) Launcher_ApplyUpdate();
	if (Window_Exists) Window_Close();
}


/*########################################################################################################################*
*---------------------------------------------------------Colours/Skin----------------------------------------------------*
*#########################################################################################################################*/
BitmapCol Launcher_BackgroundCol       = BITMAPCOL_CONST(153, 127, 172, 255);
BitmapCol Launcher_ButtonBorderCol     = BITMAPCOL_CONST( 97,  81, 110, 255);
BitmapCol Launcher_ButtonForeActiveCol = BITMAPCOL_CONST(189, 168, 206, 255);
BitmapCol Launcher_ButtonForeCol       = BITMAPCOL_CONST(141, 114, 165, 255);
BitmapCol Launcher_ButtonHighlightCol  = BITMAPCOL_CONST(162, 131, 186, 255);

void Launcher_ResetSkin(void) {
	/* Have to duplicate it here, sigh */
	BitmapCol defaultBackgroundCol       = BITMAPCOL_CONST(153, 127, 172, 255);
	BitmapCol defaultButtonBorderCol     = BITMAPCOL_CONST( 97,  81, 110, 255);
	BitmapCol defaultButtonForeActiveCol = BITMAPCOL_CONST(189, 168, 206, 255);
	BitmapCol defaultButtonForeCol       = BITMAPCOL_CONST(141, 114, 165, 255);
	BitmapCol defaultButtonHighlightCol  = BITMAPCOL_CONST(162, 131, 186, 255);

	Launcher_BackgroundCol       = defaultBackgroundCol;
	Launcher_ButtonBorderCol     = defaultButtonBorderCol;
	Launcher_ButtonForeActiveCol = defaultButtonForeActiveCol;
	Launcher_ButtonForeCol       = defaultButtonForeCol;
	Launcher_ButtonHighlightCol  = defaultButtonHighlightCol;
}

CC_NOINLINE static void Launcher_GetCol(const char* key, BitmapCol* col) {
	PackedCol tmp;
	String value;
	if (!Options_UNSAFE_Get(key, &value))     return;
	if (!PackedCol_TryParseHex(&value, &tmp)) return;

	col->R = tmp.R; col->G = tmp.G; col->B = tmp.B;
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
	PackedCol tmp;
	/* Component order might be different to BitmapCol */
	tmp.R = col.R; tmp.G = col.G; tmp.B = col.B; tmp.A = 0;
	
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
static bool useBitmappedFont;
static Bitmap terrainBmp, fontBmp;
#define TILESIZE 48

static bool Launcher_SelectZipEntry(const String* path) {
	return
		String_CaselessEqualsConst(path, "default.png") ||
		String_CaselessEqualsConst(path, "terrain.png");
}

static void Launcher_LoadTextures(Bitmap* bmp) {
	int tileSize = bmp->Width / 16;
	Bitmap_Allocate(&terrainBmp, TILESIZE * 2, TILESIZE);

	/* Precompute the scaled background */
	Drawer2D_BmpScaled(&terrainBmp, TILESIZE, 0, TILESIZE, TILESIZE,
						bmp, 2 * tileSize, 0, tileSize, tileSize,
						TILESIZE, TILESIZE);
	Drawer2D_BmpScaled(&terrainBmp, 0, 0, TILESIZE, TILESIZE,
						bmp, 1 * tileSize, 0, tileSize, tileSize,
						TILESIZE, TILESIZE);

	Gradient_Tint(&terrainBmp, 128, 64,
				  TILESIZE, 0, TILESIZE, TILESIZE);
	Gradient_Tint(&terrainBmp, 96, 96,
				  0,        0, TILESIZE, TILESIZE);
}

static ReturnCode Launcher_ProcessZipEntry(const String* path, struct Stream* data, struct ZipState* s) {
	Bitmap bmp;
	ReturnCode res;

	if (String_CaselessEqualsConst(path, "default.png")) {
		if (fontBmp.Scan0) return 0;
		res = Png_Decode(&fontBmp, data);

		if (res) {
			Logger_Warn(res, "decoding default.png"); return res;
		} else {
			Drawer2D_SetFontBitmap(&fontBmp);
			useBitmappedFont = !Options_GetBool(OPT_USE_CHAT_FONT, false);
		}
	} else if (String_CaselessEqualsConst(path, "terrain.png")) {
		if (terrainBmp.Scan0) return 0;
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
	ReturnCode res;

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
	if (!fontBmp.Scan0 || !terrainBmp.Scan0) {
		Launcher_ExtractTexturePack(&defZipPath);
	}
}

static void Launcher_ClearTile(int x, int y, int width, int height, int srcX) {
	Drawer2D_BmpTiled(&Launcher_Framebuffer, x, y, width, height,
		&terrainBmp, srcX, 0, TILESIZE, TILESIZE);
}

void Launcher_ResetArea(int x, int y, int width, int height) {
	if (Launcher_ClassicBackground && terrainBmp.Scan0) {
		Launcher_ClearTile(x, y, width, height, 0);
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

	if (Launcher_Screen && Launcher_Screen->HidesTitlebar) {
		Launcher_ResetArea(0, 0, Window_Width, Window_Height);
		return;
	}

	if (Launcher_ClassicBackground && terrainBmp.Scan0) {
		Launcher_ClearTile(0,        0, Window_Width,                 TILESIZE, TILESIZE);
		Launcher_ClearTile(0, TILESIZE, Window_Width, Window_Height - TILESIZE, 0);
	} else {
		Launcher_ResetArea(0, 0, Window_Width, Window_Height);
	}

	Drawer2D_BitmappedText = (useBitmappedFont || Launcher_ClassicBackground) && fontBmp.Scan0;
	DrawTextArgs_Make(&args, &title_fore, &logoFont, false);
	x = Window_Width / 2 - Drawer2D_TextWidth(&args) / 2;

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

		x2 = max(x + width, Launcher_Dirty.X + Launcher_Dirty.Width);
		y2 = max(y + width, Launcher_Dirty.Y + Launcher_Dirty.Height);

		x = x1; width  = x2 - x1;
		y = y1; height = y2 - y1;
	}

	Launcher_Dirty.X = x; Launcher_Dirty.Width  = width;
	Launcher_Dirty.Y = y; Launcher_Dirty.Height = height;
}

void Launcher_MarkAllDirty(void) {
	Launcher_Dirty.X = 0; Launcher_Dirty.Width  = Launcher_Framebuffer.Width;
	Launcher_Dirty.Y = 0; Launcher_Dirty.Height = Launcher_Framebuffer.Height;
}


/*########################################################################################################################*
*--------------------------------------------------------Starter/Updater--------------------------------------------------*
*#########################################################################################################################*/
static TimeMS lastJoin;
bool Launcher_StartGame(const String* user, const String* mppass, const String* ip, const String* port, const String* server) {
	String path; char pathBuffer[FILENAME_SIZE];
	String args; char argsBuffer[512];
	TimeMS now;
	ReturnCode res;
	
	now = DateTime_CurrentUTC_MS();
	if (lastJoin + 1000 > now) return false;
	lastJoin = now;

	/* Make sure if the client has changed some settings in the meantime, we keep the changes */
	Options_Load();

	/* Save resume info */
	if (server->length) {
		Options_Set("launcher-server",   server);
		Options_Set("launcher-username", user);
		Options_Set("launcher-ip",       ip);
		Options_Set("launcher-port",     port);
		Options_SetSecure("launcher-mppass", mppass, user);
		Options_Save();
	}

	String_InitArray(path, pathBuffer);
	res = Process_GetExePath(&path);
	if (res) { Logger_Warn(res, "getting .exe path"); return false; }

	String_InitArray(args, argsBuffer);
	String_AppendString(&args, user);
	if (mppass->length) String_Format3(&args, " %s %s %s", mppass, ip, port);

	res = Process_Start(&path, &args);
	if (res) { Logger_Warn(res, "starting game"); return false; }

	Launcher_ShouldExit = Options_GetBool(OPT_AUTO_CLOSE_LAUNCHER, false);
	return true;
}

#ifdef CC_BUILD_WIN
#define UPDATE_SCRIPT \
"@echo off\r\n" \
"echo Waiting for launcher to exit..\r\n" \
"echo 5..\r\n" \
"ping 127.0.0.1 -n 2 > nul\r\n" \
"echo 4..\r\n" \
"ping 127.0.0.1 -n 2 > nul\r\n" \
"echo 3..\r\n" \
"ping 127.0.0.1 -n 2 > nul\r\n" \
"echo 2..\r\n" \
"ping 127.0.0.1 -n 2 > nul\r\n" \
"echo 1..\r\n" \
"ping 127.0.0.1 -n 2 > nul\r\n" \
"echo Copying updated version\r\n" \
"move ClassiCube.update \"%s\"\r\n" \
"echo Starting launcher again\r\n" \
"start \"ClassiCube\" \"%s\"\r\n" \
"exit\r\n"
#else
#define UPDATE_SCRIPT \
"#!/bin/bash\n" \
"echo Waiting for launcher to exit..\n" \
"echo 5..\n" \
"sleep 1\n" \
"echo 4..\n" \
"sleep 1\n" \
"echo 3..\n" \
"sleep 1\n" \
"echo 2..\n" \
"sleep 1\n" \
"echo 1..\n" \
"sleep 1\n" \
"cd $(cd -P -- \"$(dirname -- \"$0\")\" && pwd -P)\n" \
"echo Copying updated version\n" \
"mv ./ClassiCube.update \"./%s\"\n" \
"echo Starting launcher again\n" \
"\"./%s\"\n"
#endif
/* The weird 'cd' line changes current directory to the directory update.sh is in */
/* Needed because bash's current directory isn't always client's directory (e.g. on OSX) */

static void Launcher_ApplyUpdate(void) {
	static const String scriptPath = String_FromConst(UPDATE_FILENAME);
	char strBuffer[1024], exeBuffer[FILENAME_SIZE];
	String str, exe;
	ReturnCode res;

	String_InitArray(exe, exeBuffer);
	res = Process_GetExePath(&exe);
	if (res) { Logger_Warn(res, "getting executable path"); return; }

	Utils_UNSAFE_GetFilename(&exe);
	String_InitArray(str, strBuffer);
	String_Format2(&str, UPDATE_SCRIPT, &exe, &exe);

	/* Can't use WriteLine, want \n as actual newline not code page 437 */
	res = Stream_WriteAllTo(&scriptPath, (const uint8_t*)str.buffer, str.length);
	if (res) { Logger_Warn(res, "saving update script"); return; }

	res = File_MarkExecutable(&scriptPath);
	if (res) Logger_Warn(res, "making update script executable");

	res = Process_StartShell();
	if (res) { Logger_Warn(res, "starting update script"); return; }
}
#endif
