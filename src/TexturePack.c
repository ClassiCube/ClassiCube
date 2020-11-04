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
	rec.U1 = 0.0f; 
	rec.V1 = y * Atlas1D.InvTileSize;
	rec.U2 = (uCount - 1) + UV2_Scale;
	rec.V2 = rec.V1       + UV2_Scale * Atlas1D.InvTileSize;
	return rec;
}

static void Atlas_Convert2DTo1D(void) {
	int tileSize      = Atlas2D.TileSize;
	int tilesPerAtlas = Atlas1D.TilesPerAtlas;
	int atlasesCount  = Atlas1D.Count;
	struct Bitmap atlas1D;
	int atlasX, atlasY;
	int tile = 0, i, y;

	Platform_Log2("Loaded new atlas: %i bmps, %i per bmp", &atlasesCount, &tilesPerAtlas);
	Bitmap_Allocate(&atlas1D, tileSize, tilesPerAtlas * tileSize);
	
	for (i = 0; i < atlasesCount; i++) {
		for (y = 0; y < tilesPerAtlas; y++, tile++) {
			atlasX = Atlas2D_TileX(tile) * tileSize;
			atlasY = Atlas2D_TileY(tile) * tileSize;

			Bitmap_UNSAFE_CopyBlock(atlasX, atlasY, 0, y * tileSize,
							&Atlas2D.Bmp, &atlas1D, tileSize);
		}
		Atlas1D.TexIds[i] = Gfx_CreateTexture(&atlas1D, true, Gfx.Mipmaps);
	}
	Mem_Free(atlas1D.scan0);
}

static void Atlas_Update1D(void) {
	int maxAtlasHeight, maxTilesPerAtlas, maxTiles;

	maxAtlasHeight   = min(4096, Gfx.MaxTexHeight);
	maxTilesPerAtlas = maxAtlasHeight / Atlas2D.TileSize;
	maxTiles         = Atlas2D.RowsCount * ATLAS2D_TILES_PER_ROW;

	Atlas1D.TilesPerAtlas = min(maxTilesPerAtlas, maxTiles);
	Atlas1D.Count = Math_CeilDiv(maxTiles, Atlas1D.TilesPerAtlas);

	Atlas1D.InvTileSize = 1.0f / Atlas1D.TilesPerAtlas;
	Atlas1D.Mask  = Atlas1D.TilesPerAtlas - 1;
	Atlas1D.Shift = Math_Log2(Atlas1D.TilesPerAtlas);
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

static GfxResourceID Atlas_LoadTile_Raw(TextureLoc texLoc, struct Bitmap* element) {
	int size = Atlas2D.TileSize;
	int x = Atlas2D_TileX(texLoc), y = Atlas2D_TileY(texLoc);
	if (y >= Atlas2D.RowsCount) return 0;

	Bitmap_UNSAFE_CopyBlock(x * size, y * size, 0, 0, &Atlas2D.Bmp, element, size);
	return Gfx_CreateTexture(element, false, Gfx.Mipmaps);
}

GfxResourceID Atlas2D_LoadTile(TextureLoc texLoc) {
	BitmapCol pixels[64 * 64];
	int size = Atlas2D.TileSize;
	struct Bitmap tile;
	GfxResourceID texId;

	/* Try to allocate bitmap on stack if possible */
	if (size > 64) {
		Bitmap_Allocate(&tile, size, size);
		texId = Atlas_LoadTile_Raw(texLoc, &tile);
		Mem_Free(tile.scan0);
		return texId;
	} else {	
		Bitmap_Init(tile, size, size, pixels);
		return Atlas_LoadTile_Raw(texLoc, &tile);
	}
}

static void Atlas2D_Free(void) {
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
	if (!Game_ValidateBitmap(&terrain, atlas)) return false;

	if (atlas->height < atlas->width) {
		Chat_AddRaw("&cUnable to use terrain.png from the texture pack.");
		Chat_AddRaw("&c Its height is less than its width.");
		return false;
	}
	if (atlas->width < ATLAS2D_TILES_PER_ROW) {
		Chat_AddRaw("&cUnable to use terrain.png from the texture pack.");
		Chat_AddRaw("&c It must be 16 or more pixels wide.");
		return false;
	}

	if (Gfx.LostContext) return false;
	Atlas1D_Free();
	Atlas2D_Free();

	Atlas_Update(atlas);
	Event_RaiseVoid(&TextureEvents.AtlasChanged);
	return true;
}


/*########################################################################################################################*
*------------------------------------------------------TextureCache-------------------------------------------------------*
*#########################################################################################################################*/
static struct StringsBuffer acceptedList, deniedList, etagCache, lastModCache;
#define ACCEPTED_TXT "texturecache/acceptedurls.txt"
#define DENIED_TXT   "texturecache/deniedurls.txt"
#define ETAGS_TXT    "texturecache/etags.txt"
#define LASTMOD_TXT  "texturecache/lastmodified.txt"

/* Initialises cache state (loading various lists) */
static void TextureCache_Init(void) {
	EntryList_UNSAFE_Load(&acceptedList, ACCEPTED_TXT);
	EntryList_UNSAFE_Load(&deniedList,   DENIED_TXT);
	EntryList_UNSAFE_Load(&etagCache,    ETAGS_TXT);
	EntryList_UNSAFE_Load(&lastModCache, LASTMOD_TXT);
}

cc_bool TextureCache_HasAccepted(const cc_string* url) { return EntryList_Find(&acceptedList, url, ' ') >= 0; }
cc_bool TextureCache_HasDenied(const cc_string* url)   { return EntryList_Find(&deniedList,   url, ' ') >= 0; }

void TextureCache_Accept(const cc_string* url) {
	EntryList_Set(&acceptedList, url, &String_Empty, ' '); 
	EntryList_Save(&acceptedList, ACCEPTED_TXT);
}
void TextureCache_Deny(const cc_string* url) {
	EntryList_Set(&deniedList,  url, &String_Empty, ' '); 
	EntryList_Save(&deniedList, DENIED_TXT);
}

int TextureCache_ClearDenied(void) {
	int count = deniedList.count;
	StringsBuffer_Clear(&deniedList);
	EntryList_Save(&deniedList, DENIED_TXT);
	return count;
}

CC_INLINE static void HashUrl(cc_string* key, const cc_string* url) {
	String_AppendUInt32(key, Utils_CRC32((const cc_uint8*)url->buffer, url->length));
}

CC_NOINLINE static void MakeCachePath(cc_string* path, const cc_string* url) {
	cc_string key; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	HashUrl(&key, url);
	String_Format1(path, "texturecache/%s", &key);
}

/* Returns non-zero if given URL has been cached */
static int IsCached(const cc_string* url) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	String_InitArray(path, pathBuffer);

	MakeCachePath(&path, url);
	return File_Exists(&path);
}

/* Attempts to open the cached data stream for the given url */
static cc_bool OpenCachedData(const cc_string* url, struct Stream* stream) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_result res;

	String_InitArray(path, pathBuffer);
	MakeCachePath(&path, url);
	res = Stream_OpenFile(stream, &path);

	if (res == ReturnCode_FileNotFound) return false;
	if (res) { Logger_Warn2(res, "opening cache for", url); return false; }
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
	/* Entry used to be a timestamp of C# ticks since 01/01/0001 */
	/* Check if this is new format */
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
	cc_string path, url; char pathBuffer[FILENAME_SIZE];
	cc_result res;
	url = String_FromRawArray(req->url);

	path = String_FromRawArray(req->etag);
	SetCachedTag(&url, &etagCache, &path, ETAGS_TXT);
	path = String_FromRawArray(req->lastModified);
	SetCachedTag(&url, &lastModCache, &path, LASTMOD_TXT);

	String_InitArray(path, pathBuffer);
	MakeCachePath(&path, &url);
	res = Stream_WriteAllTo(&path, req->data, req->size);
	if (res) { Logger_Warn2(res, "caching", &url); }
}


/*########################################################################################################################*
*-------------------------------------------------------TexturePack-------------------------------------------------------*
*#########################################################################################################################*/
static char defTexPackBuffer[STRING_SIZE];
static char textureUrlBuffer[STRING_SIZE];
static cc_string defTexPack = String_FromArray(defTexPackBuffer);
cc_string TexturePack_Url   = String_FromArray(textureUrlBuffer);
static const cc_string defaultZip = String_FromConst("default.zip");

void TexturePack_SetDefault(const cc_string* texPack) {
	String_Copy(&defTexPack, texPack);
	Options_Set(OPT_DEFAULT_TEX_PACK, texPack);
}

static cc_result ProcessZipEntry(const cc_string* path, struct Stream* stream, struct ZipState* s) {
	cc_string name = *path;
	Utils_UNSAFE_GetFilename(&name);
	Event_RaiseEntry(&TextureEvents.FileChanged, stream, &name);
	return 0;
}

static cc_result ExtractZip(struct Stream* stream) {
	struct ZipState state;
	Zip_Init(&state, stream);
	state.ProcessEntry = ProcessZipEntry;
	return Zip_Extract(&state);
}

static cc_result ExtractPng(struct Stream* stream) {
	struct Bitmap bmp;
	cc_result res = Png_Decode(&bmp, stream);
	if (!res && Atlas_TryChange(&bmp)) return 0;

	Mem_Free(bmp.scan0);
	return res;
}

static cc_bool needReload;
static void ExtractFrom(struct Stream* stream, const cc_string* path) {
	cc_result res;

	Event_RaiseVoid(&TextureEvents.PackChanged);
	/* If context is lost, then trying to load textures will just fail */
	/* So defer loading the texture pack until context is restored */
	if (Gfx.LostContext) { needReload = true; return; }
	needReload = false;

	if (String_ContainsConst(path, ".zip")) {
		res = ExtractZip(stream);
		if (res) Logger_Warn2(res, "extracting", path);
	} else {
		res = ExtractPng(stream);
		if (res) Logger_Warn2(res, "decoding", path);
	}
}

static void ExtractFromFile(const cc_string* filename) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	struct Stream stream;
	cc_result res;

	/* TODO: This is an ugly hack to load textures from memory. */
	/* We need to mount /classicube to IndexedDB, but texpacks folder */
	/* should be read from memory. I don't know how to do that, */
	/* since mounting /classicube/texpacks to memory doesn't work.. */
#ifdef CC_BUILD_WEB
	extern int chdir(const char* path);
	chdir("/");
#endif

	String_InitArray(path, pathBuffer);
	String_Format1(&path, "texpacks/%s", filename);

	res = Stream_OpenFile(&stream, &path);
	if (res) { Logger_Warn2(res, "opening", &path); return; }
	ExtractFrom(&stream, &path);

	res = stream.Close(&stream);
	if (res) { Logger_Warn2(res, "closing", &path); }

#ifdef CC_BUILD_WEB
	Platform_SetDefaultCurrentDirectory(0, NULL);
#endif
}

static void ExtractDefault(void) {
	cc_string texPack = Game_ClassicMode ? defaultZip : defTexPack;
	ExtractFromFile(&defaultZip);

	/* in case the user's default texture pack doesn't have all required textures */
	if (!String_CaselessEquals(&texPack, &defaultZip)) ExtractFromFile(&texPack);
}

static cc_bool usingDefault;
void TexturePack_ExtractCurrent(cc_bool forceReload) {
	cc_string url = TexturePack_Url;
	struct Stream stream;
	cc_result res;

	/* don't pointlessly load default texture pack */
	if (!usingDefault || forceReload) {
		ExtractDefault();
		usingDefault = true;
	}

	if (url.length && OpenCachedData(&url, &stream)) {
		ExtractFrom(&stream, &url);
		usingDefault = false;

		res = stream.Close(&stream);
		if (res) Logger_Warn2(res, "closing cache for", &url);
	}
}

void TexturePack_Apply(struct HttpRequest* item) {
	struct Stream mem;
	cc_string url;

	url = String_FromRawArray(item->url);
	UpdateCache(item);
	/* Took too long to download and is no longer active texture pack */
	if (!String_Equals(&TexturePack_Url, &url)) return;

	Stream_ReadonlyMemory(&mem, item->data, item->size);
	ExtractFrom(&mem, &url);
	usingDefault = false;
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
	TexturePack_ReqID = Http_AsyncGetDataEx(url, true, &time, &etag, NULL);
}

void TexturePack_Extract(const cc_string* url) {
	if (url->length) DownloadAsync(url);

	if (String_Equals(url, &TexturePack_Url)) return;
	String_Copy(&TexturePack_Url, url);
	TexturePack_ExtractCurrent(false);
}


/*########################################################################################################################*
*---------------------------------------------------Textures component----------------------------------------------------*
*#########################################################################################################################*/
static void OnFileChanged(void* obj, struct Stream* stream, const cc_string* name) {
	struct Bitmap bmp;
	cc_result res;

	if (!String_CaselessEqualsConst(name, "terrain.png")) return;
	res = Png_Decode(&bmp, stream);

	if (res) {
		Logger_Warn2(res, "decoding", name);
		Mem_Free(bmp.scan0);
	} else if (!Atlas_TryChange(&bmp)) {
		Mem_Free(bmp.scan0);
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
	Event_Register_(&TextureEvents.FileChanged,  NULL, OnFileChanged);
	Event_Register_(&GfxEvents.ContextLost,      NULL, OnContextLost);
	Event_Register_(&GfxEvents.ContextRecreated, NULL, OnContextRecreated);

	Options_Get(OPT_DEFAULT_TEX_PACK, &defTexPack, "default.zip");
	TextureCache_Init();
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
}

struct IGameComponent Textures_Component = {
	OnInit, /* Init  */
	OnFree, /* Free  */
	OnReset /* Reset */
};
