#include "Launcher.h"
#include "Drawer2D.h"
#include "Game.h"
#include "Deflate.h"
#include "Stream.h"

struct LSCreen* Launcher_Screen;
bool Launcher_Dirty, Launcher_PendingRedraw;
Rect2D Launcher_DirtyArea;
Bitmap Launcher_Framebuffer;
 bool Launcher_ClassicBackground;

bool Launcher_ShouldExit, Launcher_ShouldUpdate;
TimeMS Launcher_PatchTime;
struct ServerListEntry* Launcher_PublicServers;
int Launcher_NumServers;


/*########################################################################################################################*
*---------------------------------------------------------Colours/Skin-----------------------------------------------------*
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

/*CC_NOINLINE static void Launcher_GetCol(const char* key, BitmapCol* col) {
	PackedCol tmp;
	string value = Options.Get(key, null);
	if (String.IsNullOrEmpty(value)) return;

	if (!PackedCol.TryParse(value, out col))
		col = defaultCol;
}

void Launcher_LoadSkin(void) {
	Launcher_GetCol("launcher-back-col",                   &Launcher_BackgroundCol);
	Launcher_GetCol("launcher-btn-border-col",             &Launcher_ButtonBorderCol);
	Launcher_GetCol("launcher-btn-fore-active-col",        &Launcher_ButtonForeActiveCol);
	Launcher_GetCol("launcher-btn-fore-inactive-col",      &Launcher_ButtonForeCol);
	Launcher_GetCol("launcher-btn-highlight-inactive-col", &Launcher_ButtonHighlightCol);
}

CC_NOINLINE static void Launcher_SetCol(const char* key, BitmapCol col) {
	PackedCol tmp;
	string value = Options.Get(key, null);
	if (String.IsNullOrEmpty(value)) return;

	if (!PackedCol.TryParse(value, out col))
		col = defaultCol;
}

void Launcher_SaveSkin(void) {
	Launcher_SetCol("launcher-back-col",                   Launcher_BackgroundCol);
	Launcher_SetCol("launcher-btn-border-col",             Launcher_ButtonBorderCol);
	Launcher_SetCol("launcher-btn-fore-active-col",        Launcher_ButtonForeActiveCol);
	Launcher_SetCol("launcher-btn-fore-inactive-col",      Launcher_ButtonForeCol);
	Launcher_SetCol("launcher-btn-highlight-inactive-col", Launcher_ButtonHighlightCol);
}*/


/*########################################################################################################################*
*----------------------------------------------------------Background-----------------------------------------------------*
*#########################################################################################################################*/
static FontDesc logoFont;
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
						TILESIZE, TILESIZE, 128, 64);
	Drawer2D_BmpScaled(&terrainBmp, 0, 0, TILESIZE, TILESIZE,
						bmp, 1 * tileSize, 0, tileSize, tileSize,
						TILESIZE, TILESIZE, 96, 96);					
}

static void Launcher_ProcessZipEntry(const String* path, struct Stream* data, struct ZipEntry* entry) {
	Bitmap bmp;
	ReturnCode res;

	if (String_CaselessEqualsConst(path, "default.png")) {
		if (fontBmp.Scan0) return;
		res = Png_Decode(&fontBmp, data);

		if (res) {
			Launcher_ShowError(res, "decoding default.png"); return;
		} else {
			Drawer2D_SetFontBitmap(&fontBmp);
			useBitmappedFont = !Options_GetBool(OPT_USE_CHAT_FONT, false);
		}
	} else if (String_CaselessEqualsConst(path, "terrain.png")) {
		if (terrainBmp.Scan0) return;
		res = Png_Decode(&bmp, data);

		if (res) {
			Launcher_ShowError(res, "decoding default.png"); return;
		} else {
			Drawer2D_SetFontBitmap(&bmp);
			Launcher_LoadTextures(&bmp);
		}
	}
}

static void Launcher_ExtractTexturePack(const String* path) {
	struct ZipState state;
	struct Stream stream;
	ReturnCode res;

	res = Stream_OpenFile(&stream, path);
	if (res) {
		Launcher_ShowError(res, "opening texture pack"); return;
	}

	Zip_Init(&state, &stream);
	state.SelectEntry  = Launcher_SelectZipEntry;
	state.ProcessEntry = Launcher_ProcessZipEntry;
	res = Zip_Extract(&state);

	if (res) {
		Launcher_ShowError(res, "extracting texture pack");
	}
	stream.Close(&stream);
}

/*void Launcher_TryLoadTexturePack(void) {
	if (Options.Get("nostalgia-classicbg", null) != null) {
		Launcher_ClassicBackground = Options_GetBool("nostalgia-classicbg", false);
	} else {
		Launcher_ClassicBackground = Options_GetBool(OPT_CLASSIC_MODE,      false);
	}

	string texPack = Options.Get(OptionsKey.DefaultTexturePack, "default.zip");
	string texPath = Path.Combine("texpacks", texPack);

	if (!Platform_FileExists(texPath)) {
		texPath = Path.Combine("texpacks", "default.zip");
	}
	if (!Platform_FileExists(texPath)) return;

	Launcher_ExtractTexturePack(texPath);
	// user selected texture pack is missing some required .png files
	if (!fontBmp.Scan0 || !terrainBmp.Scan0) {
		texPath = Path.Combine("texpacks", "default.zip");
		ExtractTexturePack(texPath);
	}
}*/

static void Launcher_ClearTile(int x, int y, int width, int height, int srcX) {
	Drawer2D_BmpTiled(&Launcher_Framebuffer, x, y, width, height,
		&terrainBmp, srcX, 0, TILESIZE, TILESIZE);
}

void Launcher_ResetArea(int x, int y, int width, int height) {
	if (Launcher_ClassicBackground && terrainBmp.Scan0) {
		Launcher_ClearTile(x, y, width, height, 0);
	} else {
		Gradient_Noise(&Launcher_Framebuffer, x, y, width, height, Launcher_BackgroundCol, 6);
	}
}

void Launcher_ResetPixels(void) {
	static String title_fore = String_FromConst("&eClassi&fCube");
	static String title_back = String_FromConst("&0Classi&0Cube");
	struct DrawTextArgs args;
	int x;

	if (Launcher_ClassicBackground && terrainBmp.Scan0) {
		Launcher_ClearTile(0,        0, Game_Width,               TILESIZE, TILESIZE);
		Launcher_ClearTile(0, TILESIZE, Game_Width, Game_Height - TILESIZE, 0);
	} else {
		Launcher_ResetArea(0, 0, Game_Width, Game_Height);
	}

	Drawer2D_BitmappedText = (useBitmappedFont || Launcher_ClassicBackground) && fontBmp.Scan0;
	DrawTextArgs_Make(&args, &title_fore, &logoFont, false);
	x = Game_Width / 2 - Drawer2D_MeasureText(&args).Width / 2;

	args.Text = title_back;
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, x + 4, 4);
	args.Text = title_fore;
	Drawer2D_DrawText(&Launcher_Framebuffer, &args, x, 0);

	Drawer2D_BitmappedText = false;
	Launcher_Dirty = true;
}
