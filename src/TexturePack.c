#include "TexturePack.h"
#include "String.h"
#include "Constants.h"
#include "Stream.h"
#include "World.h"
#include "Graphics.h"
#include "Event.h"
#include "Game.h"
#include "Http.h"
#include "Platform.h"
#include "Deflate.h"
#include "Funcs.h"
#include "ExtMath.h"
#include "Options.h"
#include "Logger.h"
#include "Utils.h"
#include "Chat.h" /* TODO avoid this include */
#include "Errors.h"

/* Simple fallback terrain for when no texture packs are available at all */
static BitmapCol fallback_terrain[16 * 8] = {
	BitmapColor_RGB( 96, 144,  85), BitmapColor_RGB(129, 128, 127), BitmapColor_RGB(123,  87,  66), BitmapColor_RGB(174, 124,  74), BitmapColor_RGB(184, 151, 105), BitmapColor_RGB(200, 200, 197), BitmapColor_RGB(175, 173, 173), BitmapColor_RGB(153, 101,  75), 
	BitmapColor_RGB(118, 111, 101), BitmapColor_RGB( 61,  20,  11), BitmapColor_RGB(179,  67,  23), BitmapColor_RGB(154, 128,  89), BitmapColor_RGB(163,   2,  29), BitmapColor_RGB(203, 206,   2), BitmapCol_Make(86,144,216,128), BitmapColor_RGB( 38,  88,  41),
	/* 16*/
	BitmapColor_RGB(165, 163, 159), BitmapColor_RGB( 37,  48,  61), BitmapColor_RGB(227, 223, 151), BitmapColor_RGB(160, 152, 147), BitmapColor_RGB( 90,  71,  58), BitmapColor_RGB(173, 135,  87), BitmapColor_RGB( 38,  98,  37), BitmapColor_RGB(225, 229, 235), 
	BitmapColor_RGB(246, 231,  23), BitmapColor_RGB(225, 218, 157), BitmapColor_RGB(247, 243, 234), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB(226,  18,  18), BitmapColor_RGB(172, 131, 101), BitmapColor_RGB(255, 122,  31), BitmapColor_RGB( 79, 120,  79),
	/* 32 */
	BitmapColor_RGB(129, 128, 127), BitmapColor_RGB(189, 151, 134), BitmapColor_RGB( 53,  44,  61), BitmapColor_RGB(180, 151, 102), BitmapColor_RGB(165, 163, 159), BitmapColor_RGB( 20,  20,  33), BitmapColor_RGB(243, 139,  28), BitmapColor_RGB(193, 197, 202), 
	BitmapColor_RGB(235, 188,  32), BitmapColor_RGB(203, 193, 135), BitmapColor_RGB(224, 220, 212), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB(174, 124,  74),
	/* 48 */
	BitmapColor_RGB(175, 148,  43), BitmapColor_RGB(188, 225, 231), BitmapColor_RGB(238, 245, 245), BitmapCol_Make(205,232,252,128),BitmapColor_RGB(153, 150, 149), BitmapColor_RGB(105,  80,  54), BitmapColor_RGB(236, 236, 240), BitmapColor_RGB(161, 165, 170),
	BitmapColor_RGB(225, 146,  30), BitmapColor_RGB(203, 193, 135), BitmapColor_RGB(247, 243, 234), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158),
	/* 64 */
	BitmapColor_RGB(217,  35,  35), BitmapColor_RGB(219, 137,  13), BitmapColor_RGB(224, 224,   0), BitmapColor_RGB(128, 221,   2), BitmapColor_RGB( 13, 217,  13), BitmapColor_RGB(  8, 218, 133), BitmapColor_RGB(  4, 219, 219), BitmapColor_RGB( 89, 175, 219),
	BitmapColor_RGB(122, 122, 217), BitmapColor_RGB(131,  39, 225), BitmapColor_RGB(178,  69, 230), BitmapColor_RGB(227,  52, 227), BitmapColor_RGB(227,  41, 133), BitmapColor_RGB( 73,  73,  73), BitmapColor_RGB(151, 151, 151), BitmapColor_RGB(227, 227, 227),
	/* 80 */
	BitmapColor_RGB(220, 127, 162), BitmapColor_RGB( 42,  66,   8), BitmapColor_RGB( 75,  37,  11), BitmapColor_RGB( 24,  37, 149), BitmapColor_RGB( 29, 113, 149), BitmapColor_RGB(155, 161, 174), BitmapColor_RGB(167,  41,  13), BitmapColor_RGB( 57, 115, 158), 
	BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158),
	/* 96 */
	BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), 
	BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134),
	/* 112 */
	BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), 
	BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158), BitmapColor_RGB( 52,  90, 134), BitmapColor_RGB( 57, 115, 158),
};

static void LoadFallbackAtlas(void) {
	struct Bitmap bmp, src;
	src.width  = 16;
	src.height = 8;
	src.scan0  = fallback_terrain;
	
	if (Gfx.MinTexWidth || Gfx.MinTexHeight) {
		Bitmap_Allocate(&bmp, 16 * Gfx.MinTexWidth, 8 * Gfx.MinTexHeight);
		Bitmap_Scale(&bmp, &src, 0, 0, 16, 8);
		Atlas_TryChange(&bmp);
	} else {
		Atlas_TryChange(&src);
	}
}

/*########################################################################################################################*
*------------------------------------------------------TerrainAtlas-------------------------------------------------------*
*#########################################################################################################################*/
struct _Atlas2DData Atlas2D;
struct _Atlas1DData Atlas1D;
int TexturePack_ReqID;

TextureRec Atlas1D_TexRec(TextureLoc texLoc, int uCount, int* index) {
	TextureRec rec;
	int y  = Atlas1D_RowId(texLoc);
	*index = Atlas1D_Index(texLoc);

	/* Adjust coords to be slightly inside - fixes issues with AMD/ATI cards */	
	rec.u1 = 0.0f; 
	rec.v1 = y * Atlas1D.InvTileSize;
	rec.u2 = (uCount - 1) + UV2_Scale;
	rec.v2 = rec.v1       + UV2_Scale * Atlas1D.InvTileSize;
	return rec;
}


static void Atlas1D_Load(int index, struct Bitmap* atlas1D) {
	int tileSize      = Atlas2D.TileSize;
	int tilesPerAtlas = Atlas1D.TilesPerAtlas;
	int y, tile = index * tilesPerAtlas;
	int atlasX, atlasY;
	
	for (y = 0; y < tilesPerAtlas; y++, tile++) 
	{
		atlasX = Atlas2D_TileX(tile) * tileSize;
		atlasY = Atlas2D_TileY(tile) * tileSize;

		Bitmap_UNSAFE_CopyBlock(atlasX, atlasY, 0, y * tileSize,
							&Atlas2D.Bmp, atlas1D, tileSize);
	}
	Gfx_RecreateTexture(&Atlas1D.TexIds[index], atlas1D, TEXTURE_FLAG_MANAGED | TEXTURE_FLAG_DYNAMIC, Gfx.Mipmaps);
}

/* TODO: always do this? */
#ifdef CC_BUILD_LOWMEM
static void Atlas1D_LoadBlock(int index) {
	int tileSize      = Atlas2D.TileSize;
	int tilesPerAtlas = Atlas1D.TilesPerAtlas;
	struct Bitmap atlas1D;

	Platform_Log2("Lazy load atlas #%i (%i per bmp)", &index, &tilesPerAtlas);
	Bitmap_Allocate(&atlas1D, tileSize, tilesPerAtlas * tileSize);
	
	Atlas1D_Load(index, &atlas1D);
	Mem_Free(atlas1D.scan0);
}

void Atlas1D_Bind(int index) {
	if (index < Atlas1D.Count && !Atlas1D.TexIds[index])
		Atlas1D_LoadBlock(index);
	Gfx_BindTexture(Atlas1D.TexIds[index]);
}

static void Atlas_Convert2DTo1D(void) {
	int tilesPerAtlas = Atlas1D.TilesPerAtlas;
	int atlasesCount  = Atlas1D.Count;
	Platform_Log2("Terrain atlas: %i bmps, %i per bmp", &atlasesCount, &tilesPerAtlas);
}
#else
void Atlas1D_Bind(int index) {
	Gfx_BindTexture(Atlas1D.TexIds[index]);
}

static void Atlas_Convert2DTo1D(void) {
	int tileSize      = Atlas2D.TileSize;
	int tilesPerAtlas = Atlas1D.TilesPerAtlas;
	int atlasesCount  = Atlas1D.Count;
	struct Bitmap atlas1D;
	int i;

	Platform_Log2("Loaded terrain atlas: %i bmps, %i per bmp", &atlasesCount, &tilesPerAtlas);
	Bitmap_Allocate(&atlas1D, tileSize, tilesPerAtlas * tileSize);
	
	for (i = 0; i < atlasesCount; i++) 
	{
		Atlas1D_Load(i, &atlas1D);
	}
	Mem_Free(atlas1D.scan0);
}
#endif

static void Atlas_Update1D(void) {
	int maxAtlasHeight, maxTilesPerAtlas, maxTiles;
	int maxTexHeight = Gfx.MaxTexHeight;

	/* E.g. a graphics backend may support textures up to 256 x 256 */
	/*   dimension wise, but only have enough storage for 16 x 256 */
	if (Gfx.MaxTexSize) {
		int maxCurHeight = Gfx.MaxTexSize / Atlas2D.TileSize;
		maxTexHeight     = min(maxTexHeight, maxCurHeight);
	}

	maxAtlasHeight   = min(4096, maxTexHeight);
	maxTilesPerAtlas = maxAtlasHeight / Atlas2D.TileSize;
	maxTiles         = Atlas2D.RowsCount * ATLAS2D_TILES_PER_ROW;

	Atlas1D.TilesPerAtlas = min(maxTilesPerAtlas, maxTiles);
	Atlas1D.Count = Math_CeilDiv(maxTiles, Atlas1D.TilesPerAtlas);

	Atlas1D.InvTileSize = 1.0f / Atlas1D.TilesPerAtlas;
	Atlas1D.Mask  = Atlas1D.TilesPerAtlas - 1;
	Atlas1D.Shift = Math_ilog2(Atlas1D.TilesPerAtlas);
}

/* Loads the given atlas and converts it into an array of 1D atlases. */
static void Atlas_Update(struct Bitmap* bmp) {
	Atlas2D.Bmp       = *bmp;
	Atlas2D.TileSize  = bmp->width  / ATLAS2D_TILES_PER_ROW;
	Atlas2D.RowsCount = bmp->height / Atlas2D.TileSize;
	Atlas2D.RowsCount = min(Atlas2D.RowsCount, ATLAS2D_MAX_ROWS_COUNT);

	Atlas_Update1D();
	Atlas_Convert2DTo1D();
}

GfxResourceID Atlas2D_LoadTile(TextureLoc texLoc) {
	int size = Atlas2D.TileSize;
	struct Bitmap tile;

	int x = Atlas2D_TileX(texLoc), y = Atlas2D_TileY(texLoc);
	if (y >= Atlas2D.RowsCount) return 0;

	tile.scan0  = Bitmap_GetRow(&Atlas2D.Bmp, y * size) + (x * size);
	tile.width  = size;
	tile.height = size;
	return Gfx_CreateTexture2(&tile, Atlas2D.Bmp.width, 0, Gfx.Mipmaps);
}

static void Atlas2D_Free(void) {
	if (Atlas2D.Bmp.scan0 != fallback_terrain)
		Mem_Free(Atlas2D.Bmp.scan0);

	Atlas2D.Bmp.scan0 = NULL;
	Atlas2D.RowsCount = 0;
}

static void Atlas1D_Free(void) {
	int i;
	for (i = 0; i < Atlas1D.Count; i++) {
		Gfx_DeleteTexture(&Atlas1D.TexIds[i]);
	}
}

cc_bool Atlas_TryChange(struct Bitmap* atlas) {
	static const cc_string terrain = String_FromConst("terrain.png");
	int tileSize;

	if (!Game_ValidateBitmapPow2(&terrain, atlas)) return false;
	tileSize = atlas->width / ATLAS2D_TILES_PER_ROW;

	if (tileSize <= 0) {
		Chat_AddRaw("&cUnable to use terrain.png from the texture pack.");
		Chat_AddRaw("&c It must be 16 or more pixels wide.");
		return false;
	}
	if (atlas->height < tileSize) {
		Chat_AddRaw("&cUnable to use terrain.png from the texture pack.");
		Chat_AddRaw("&c It must have at least one row in it.");
		return false;
	}

	if (!Gfx_CheckTextureSize(tileSize, tileSize, 0)) {
		Chat_AddRaw("&cUnable to use terrain.png from the texture pack.");
		Chat_Add4("&c Tile size is (%i,%i), your GPU supports (%i,%i) at most.", 
			&tileSize, &tileSize, &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
		return false;
	}

	if (atlas->height < atlas->width) {
		/* Probably wouldn't want to use these, but you still can technically */
		Chat_AddRaw("&cHeight of terrain.png is less than its width.");
		Chat_AddRaw("&c Some tiles will therefore appear completely white.");
	}
	if (atlas->width > Gfx.MaxTexWidth) {
		/* Super HD textures probably won't work great on this GPU */
		Chat_AddRaw("&cYou may experience significantly reduced performance.");
		Chat_Add4("&c terrain.png size is (%i,%i), your GPU supports (%i,%i) at most.", 
			&atlas->width, &atlas->height, &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
	}

	if (Gfx.LostContext) return false;
	Atlas1D_Free();
	Atlas2D_Free();

	Atlas_Update(atlas);
	Event_RaiseVoid(&TextureEvents.AtlasChanged);
	return true;
}


/*########################################################################################################################*
*------------------------------------------------------TextureUrls--------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_NETWORKING
static struct StringsBuffer acceptedList, deniedList;
#define ACCEPTED_TXT "texturecache/acceptedurls.txt"
#define DENIED_TXT   "texturecache/deniedurls.txt"

static void TextureUrls_Init(void) {
	EntryList_UNSAFE_Load(&acceptedList, ACCEPTED_TXT);
	EntryList_UNSAFE_Load(&deniedList,   DENIED_TXT);
}

cc_bool TextureUrls_HasAccepted(const cc_string* url) { return EntryList_Find(&acceptedList, url, ' ') >= 0; }
cc_bool TextureUrls_HasDenied(const cc_string* url)   { return EntryList_Find(&deniedList,   url, ' ') >= 0; }

void TextureUrls_Accept(const cc_string* url) {
	EntryList_Set(&acceptedList, url, &String_Empty, ' '); 
	EntryList_Save(&acceptedList, ACCEPTED_TXT);
}

void TextureUrls_Deny(const cc_string* url) {
	EntryList_Set(&deniedList,  url, &String_Empty, ' '); 
	EntryList_Save(&deniedList, DENIED_TXT);
}

int TextureUrls_ClearDenied(void) {
	int count = deniedList.count;
	StringsBuffer_Clear(&deniedList);
	EntryList_Save(&deniedList, DENIED_TXT);
	return count;
}
#else
static void TextureUrls_Init(void) { }

cc_bool TextureUrls_HasAccepted(const cc_string* url) { return false; }
cc_bool TextureUrls_HasDenied(const cc_string* url)   { return false; }

void TextureUrls_Accept(const cc_string* url) { }

void TextureUrls_Deny(const cc_string* url) { }

int TextureUrls_ClearDenied(void) { return 0; }
#endif


/*########################################################################################################################*
*------------------------------------------------------TextureCache-------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_NETWORKING
static struct StringsBuffer etagCache, lastModCache;
#define ETAGS_TXT    "texturecache/etags.txt"
#define LASTMOD_TXT  "texturecache/lastmodified.txt"

static void TextureCache_Init(void) {
	EntryList_UNSAFE_Load(&etagCache,    ETAGS_TXT);
	EntryList_UNSAFE_Load(&lastModCache, LASTMOD_TXT);
}

CC_INLINE static void HashUrl(cc_string* key, const cc_string* url) {
	String_AppendUInt32(key, Utils_CRC32((const cc_uint8*)url->buffer, url->length));
}

static cc_bool createdCache, cacheInvalid;
static cc_bool UseDedicatedCache(cc_string* path, const cc_string* key) {
	cc_result res;
	cc_filepath str;
	Directory_GetCachePath(path);
	if (!path->length || cacheInvalid) return false;

	String_AppendConst(path, "/texturecache");
	Platform_EncodePath(&str, path);
	res = Directory_Create(&str);

	/* Check if something is deleting the cache directory behind our back */
	/*  (Several users have reported this happening on some Android devices) */
	if (createdCache && res == 0) {
		Chat_AddRaw("&cSomething has deleted system managed cache folder");
		Chat_AddRaw("  &cFalling back to caching to game folder instead..");
		cacheInvalid = true;
	}
	if (res == 0) createdCache = true;

	String_Format1(path, "/%s", key);
	return !cacheInvalid;
}

CC_NOINLINE static void MakeCachePath(cc_string* mainPath, cc_string* altPath, const cc_string* url) {
	cc_string key; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);
	HashUrl(&key, url);
	
	if (UseDedicatedCache(mainPath, &key)) {
		/* If using dedicated cache directory, also fallback to default cache directory */
		String_Format1(altPath,  "texturecache/%s",  &key);
	} else {
		mainPath->length = 0;
		String_Format1(mainPath, "texturecache/%s",  &key);
	}
}

/* Returns non-zero if given URL has been cached */
static int IsCached(const cc_string* url) {
	cc_string mainPath; char mainBuffer[FILENAME_SIZE];
	cc_string altPath;  char  altBuffer[FILENAME_SIZE];
	cc_filepath mainStr, altStr;
	
	String_InitArray(mainPath, mainBuffer);
	String_InitArray(altPath,   altBuffer);

	MakeCachePath(&mainPath, &altPath, url);
	Platform_EncodePath(&mainStr, &mainPath);
	Platform_EncodePath(&altStr,  &altPath);

	return File_Exists(&mainStr) || (altPath.length && File_Exists(&altStr));
}

/* Attempts to open the cached data stream for the given url */
static cc_bool OpenCachedData(const cc_string* url, struct Stream* stream) {
	cc_string mainPath; char mainBuffer[FILENAME_SIZE];
	cc_string altPath;  char  altBuffer[FILENAME_SIZE];
	cc_result res;
	String_InitArray(mainPath, mainBuffer);
	String_InitArray(altPath,   altBuffer);
	

	MakeCachePath(&mainPath, &altPath, url);
	res = Stream_OpenFile(stream, &mainPath);

	/* try fallback cache if can't find in main cache */
	if (res == ReturnCode_FileNotFound && altPath.length)
		res = Stream_OpenFile(stream, &altPath);

	if (res == ReturnCode_FileNotFound) return false;
	if (res) { Logger_SysWarn2(res, "opening cache for", url); return false; }
	return true;
}

CC_NOINLINE static cc_string GetCachedTag(const cc_string* url, struct StringsBuffer* list) {
	cc_string key; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	HashUrl(&key, url);
	return EntryList_UNSAFE_Get(list, &key, ' ');
}

static cc_string GetCachedLastModified(const cc_string* url) {
	int i;
	cc_string entry = GetCachedTag(url, &lastModCache);
	/* Entry used to be a timestamp of C# DateTime ticks since 01/01/0001 */
	/* Check whether timestamp entry is old or new format */
	for (i = 0; i < entry.length; i++) {
		if (entry.buffer[i] < '0' || entry.buffer[i] > '9') return entry;
	}

	/* Entry is all digits, so the old unsupported format */
	entry.length = 0; return entry;
}

static cc_string GetCachedETag(const cc_string* url) {
	return GetCachedTag(url, &etagCache);
}

CC_NOINLINE static void SetCachedTag(const cc_string* url, struct StringsBuffer* list,
									 const cc_string* data, const char* file) {
	cc_string key; char keyBuffer[STRING_INT_CHARS];
	if (!data->length) return;

	String_InitArray(key, keyBuffer);
	HashUrl(&key, url);
	EntryList_Set(list, &key, data, ' ');
	EntryList_Save(list, file);
}

/* Updates cached data, ETag, and Last-Modified for the given URL */
static void UpdateCache(struct HttpRequest* req) {
	cc_string url, altPath, value;
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_result res;
	url = String_FromRawArray(req->url);

	value = String_FromRawArray(req->etag);
	SetCachedTag(&url, &etagCache,    &value, ETAGS_TXT);
	value = String_FromRawArray(req->lastModified);
	SetCachedTag(&url, &lastModCache, &value, LASTMOD_TXT);

	String_InitArray(path, pathBuffer);
	altPath = String_Empty;
	MakeCachePath(&path, &altPath, &url);

	res = Stream_WriteAllTo(&path, req->data, req->size);
	if (res) { Logger_SysWarn2(res, "caching", &url); }
}
#else
static void TextureCache_Init(void) {
}

/* Returns non-zero if given URL has been cached */
static int IsCached(const cc_string* url) {
	return false;
}

/* Attempts to open the cached data stream for the given url */
static cc_bool OpenCachedData(const cc_string* url, struct Stream* stream) {
	return false;
}

static cc_string GetCachedLastModified(const cc_string* url) {
	return String_Empty;
}

static cc_string GetCachedETag(const cc_string* url) {
	return String_Empty;
}

/* Updates cached data, ETag, and Last-Modified for the given URL */
static void UpdateCache(struct HttpRequest* req) { }
#endif


/*########################################################################################################################*
*-------------------------------------------------------TexturePack-------------------------------------------------------*
*#########################################################################################################################*/
static char textureUrlBuffer[URL_MAX_SIZE];
static char texpackPathBuffer[FILENAME_SIZE];

cc_string TexturePack_Url  = String_FromArray(textureUrlBuffer);
cc_string TexturePack_Path = String_FromArray(texpackPathBuffer);
cc_bool TexturePack_DefaultMissing;

void TexturePack_SetDefault(const cc_string* texPack) {
	TexturePack_Path.length = 0;
	String_Format1(&TexturePack_Path, "texpacks/%s", texPack);
	Options_Set(OPT_DEFAULT_TEX_PACK, texPack);
}

cc_result TexturePack_ExtractDefault(DefaultZipCallback callback) {
	cc_result res = ReturnCode_FileNotFound;
	const char* defaults[3];
	cc_string path;
	int i;

	defaults[0] = Game_Version.DefaultTexpack;
	defaults[1] = "texpacks/default.zip";
	defaults[2] = "texpacks/classicube.zip";

	for (i = 0; i < Array_Elems(defaults); i++) 
	{
		path = String_FromReadonly(defaults[i]);
		res  = callback(&path);
		if (!res) return 0;
	}
	return res;
}


static cc_bool SelectZipEntry(const cc_string* path) { return true; }
static cc_result ProcessZipEntry(const cc_string* path, struct Stream* stream, struct ZipEntry* source) {
	cc_string name = *path;
	Utils_UNSAFE_GetFilename(&name);
	Event_RaiseEntry(&TextureEvents.FileChanged, stream, &name);
	return 0;
}

static cc_result ExtractPng(struct Stream* stream) {
	struct Bitmap bmp;
	cc_result res = Png_Decode(&bmp, stream);
	if (!res && Atlas_TryChange(&bmp)) return 0;

	Mem_Free(bmp.scan0);
	return res;
}

static cc_bool needReload;
static cc_result ExtractFrom(struct Stream* stream, const cc_string* path) {
	struct ZipEntry entries[512];
	cc_result res;

	Event_RaiseVoid(&TextureEvents.PackChanged);
	/* If context is lost, then trying to load textures will just fail */
	/* So defer loading the texture pack until context is restored */
	if (Gfx.LostContext) { needReload = true; return 0; }
	needReload = false;

	res = ExtractPng(stream);
	if (res == PNG_ERR_INVALID_SIG) {
		/* file isn't a .png image, probably a .zip archive then */
		res = Zip_Extract(stream, SelectZipEntry, ProcessZipEntry,
							entries, Array_Elems(entries));

		if (res) Logger_SysWarn2(res, "extracting", path);
	} else if (res) {
		Logger_SysWarn2(res, "decoding", path);
	}
	return res;
}

#if defined CC_BUILD_PS1 || defined CC_BUILD_SATURN
#include "../misc/ps1/classicubezip.h"

static cc_result ExtractFromFile(const cc_string* path) {
	struct Stream stream;
	Stream_ReadonlyMemory(&stream, ccTextures, ccTextures_length);

	return ExtractFrom(&stream, path);
}
#else
static cc_result ExtractFromFile(const cc_string* path) {
	struct Stream stream;
	cc_result res;

	res = Stream_OpenFile(&stream, path);
	if (res) { Logger_SysWarn2(res, "opening", path); return res; }

	res = ExtractFrom(&stream, path);
	/* No point logging error for closing readonly file */
	(void)stream.Close(&stream);
	return res;
}
#endif

static cc_result ExtractUserTextures(void) {
	cc_string path;
	cc_result res;

	/* TODO: Log error for multiple default texture pack extract failure */
	res = TexturePack_ExtractDefault(ExtractFromFile);
	/* Game shows a warning dialog if default textures are missing */
	TexturePack_DefaultMissing = res == ReturnCode_FileNotFound;

	path = TexturePack_Path;
	if (String_CaselessEqualsConst(&path, "texpacks/default.zip")) path.length = 0;
	if (Game_ClassicMode || path.length == 0) return res;

	/* override default textures with user's selected texture pack */
	return ExtractFromFile(&path);
}

static cc_bool usingDefault;
cc_result TexturePack_ExtractCurrent(cc_bool forceReload) {
	cc_string url = TexturePack_Url;
	struct Stream stream;
	cc_result res = 0;

	/* don't pointlessly load default texture pack */
	if (!usingDefault || forceReload) {
		res = ExtractUserTextures();
		usingDefault = true;
	}

	if (url.length && OpenCachedData(&url, &stream)) {
		res = ExtractFrom(&stream, &url);
		usingDefault = false;

		/* No point logging error for closing readonly file */
		(void)stream.Close(&stream);
	}

	/* Use fallback terrain texture with 1 pixel per tile */
	if (!Atlas2D.Bmp.scan0) LoadFallbackAtlas();
	return res;
}

/* Extracts and updates cache for the downloaded texture pack */
static void ApplyDownloaded(struct HttpRequest* item) {
	struct Stream mem;
	cc_string url;

	url = String_FromRawArray(item->url);
	if (!Platform_ReadonlyFilesystem) UpdateCache(item);
	/* Took too long to download and is no longer active texture pack */
	if (!String_Equals(&TexturePack_Url, &url)) return;

	Stream_ReadonlyMemory(&mem, item->data, item->size);
	ExtractFrom(&mem, &url);
	usingDefault = false;
}

void TexturePack_CheckPending(void) {
	struct HttpRequest item;
	if (!Http_GetResult(TexturePack_ReqID, &item)) return;

	if (item.success) {
		ApplyDownloaded(&item);
	} else if (item.result) {
		Http_LogError("trying to download texture pack", &item);
	} else if (item.statusCode == 200 || item.statusCode == 304) {
		/* Empty responses is okay for these status codes, so don't log an error */
	} else if (item.statusCode == 404) {
		Chat_AddRaw("&c404 Not Found error when trying to download texture pack");
		Chat_AddRaw("  &cThe texture pack URL may be incorrect or no longer exist");
	} else if (item.statusCode == 401 || item.statusCode == 403) {
		Chat_Add1("&c%i Not Authorised error when trying to download texture pack", &item.statusCode);
		Chat_AddRaw("  &cThe texture pack URL may not be publicly shared");
	} else {
		Chat_Add1("&c%i error when trying to download texture pack", &item.statusCode);
	}
	HttpRequest_Free(&item);
}

/* Asynchronously downloads the given texture pack */
static void DownloadAsync(const cc_string* url) {
	cc_string etag = String_Empty;
	cc_string time = String_Empty;

	/* Only retrieve etag/last-modified headers if the file exists */
	/* This inconsistency can occur if user deleted some cached files */
	if (IsCached(url)) {
		time = GetCachedLastModified(url);
		etag = GetCachedETag(url);
	}

	Http_TryCancel(TexturePack_ReqID);
	TexturePack_ReqID = Http_AsyncGetDataEx(url, HTTP_FLAG_PRIORITY, &time, &etag, NULL);
}

void TexturePack_Extract(const cc_string* url) {
	if (url->length) DownloadAsync(url);

	if (String_Equals(url, &TexturePack_Url)) return;
	String_Copy(&TexturePack_Url, url);
	TexturePack_ExtractCurrent(false);
}

static struct TextureEntry* entries_head;
static struct TextureEntry* entries_tail;

void TextureEntry_Register(struct TextureEntry* entry) {
	LinkedList_Append(entry, entries_head, entries_tail);
}


/*########################################################################################################################*
*---------------------------------------------------Textures component----------------------------------------------------*
*#########################################################################################################################*/
static void TerrainPngProcess(struct Stream* stream, const cc_string* name) {
	struct Bitmap bmp;
	cc_result res = Png_Decode(&bmp, stream);

	if (res) {
		Logger_SysWarn2(res, "decoding", name);
		Mem_Free(bmp.scan0);
	} else if (!Atlas_TryChange(&bmp)) {
		Mem_Free(bmp.scan0);
	}
}
static struct TextureEntry terrain_entry = { "terrain.png", TerrainPngProcess };


static void OnFileChanged(void* obj, struct Stream* stream, const cc_string* name) {
	struct TextureEntry* e;

	for (e = entries_head; e; e = e->next) {
		if (!String_CaselessEqualsConst(name, e->filename)) continue;

		e->Callback(stream, name);
		return;
	}
}

static void OnContextLost(void* obj) {
	if (!Gfx.ManagedTextures) Atlas1D_Free();
}

static void OnContextRecreated(void* obj) {
	if (!Gfx.ManagedTextures || needReload) {
		TexturePack_ExtractCurrent(true);
	}
}

static void OnInit(void) {
	cc_string file;
	Event_Register_(&TextureEvents.FileChanged,  NULL, OnFileChanged);
	Event_Register_(&GfxEvents.ContextLost,      NULL, OnContextLost);
	Event_Register_(&GfxEvents.ContextRecreated, NULL, OnContextRecreated);

	TexturePack_Path.length = 0;
	if (Options_UNSAFE_Get(OPT_DEFAULT_TEX_PACK, &file)) {
		String_Format1(&TexturePack_Path, "texpacks/%s", &file);
	}
	
	/* TODO temp hack to fix mobile, need to properly fix */
	/*  issue is that Drawer2D_Component.Init is called from Launcher,*/
	/*  which called TextureEntry_Register, whoops*/
	entries_head = NULL;

	TextureEntry_Register(&terrain_entry);
	Utils_EnsureDirectory("texpacks");
	Utils_EnsureDirectory("texturecache");
	TextureCache_Init();
	TextureUrls_Init();
}

static void OnReset(void) {
	if (!TexturePack_Url.length) return;
	TexturePack_Url.length = 0;
	TexturePack_ExtractCurrent(false);
}

static void OnFree(void) {
	OnContextLost(NULL);
	Atlas2D_Free();
	TexturePack_Url.length = 0;
	entries_head = NULL;
}

struct IGameComponent Textures_Component = {
	OnInit, /* Init  */
	OnFree, /* Free  */
	OnReset /* Reset */
};
