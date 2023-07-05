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

	Platform_Log2("Loaded terrain atlas: %i bmps, %i per bmp", &atlasesCount, &tilesPerAtlas);
	Bitmap_Allocate(&atlas1D, tileSize, tilesPerAtlas * tileSize);
	
	for (i = 0; i < atlasesCount; i++) {
		for (y = 0; y < tilesPerAtlas; y++, tile++) {
			atlasX = Atlas2D_TileX(tile) * tileSize;
			atlasY = Atlas2D_TileY(tile) * tileSize;

			Bitmap_UNSAFE_CopyBlock(atlasX, atlasY, 0, y * tileSize,
								&Atlas2D.Bmp, &atlas1D, tileSize);
		}
		Gfx_RecreateTexture(&Atlas1D.TexIds[i], &atlas1D, TEXTURE_FLAG_MANAGED | TEXTURE_FLAG_DYNAMIC, Gfx.Mipmaps);
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
	return Gfx_CreateTexture(element, 0, Gfx.Mipmaps);
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

static cc_bool createdCache, cacheInvalid;
static cc_bool UseDedicatedCache(cc_string* path, const cc_string* key) {
	cc_result res;
	Directory_GetCachePath(path);
	if (!path->length || cacheInvalid) return false;

	String_AppendConst(path, "/texturecache");
	res = Directory_Create(path);

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
	String_InitArray(mainPath, mainBuffer);
	String_InitArray(altPath,   altBuffer);

	MakeCachePath(&mainPath, &altPath, url);
	return File_Exists(&mainPath) || (altPath.length && File_Exists(&altPath));
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


/*########################################################################################################################*
*-------------------------------------------------------TexturePack-------------------------------------------------------*
*#########################################################################################################################*/
static char textureUrlBuffer[STRING_SIZE];
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
	cc_result res;

	Event_RaiseVoid(&TextureEvents.PackChanged);
	/* If context is lost, then trying to load textures will just fail */
	/* So defer loading the texture pack until context is restored */
	if (Gfx.LostContext) { needReload = true; return 0; }
	needReload = false;

	res = ExtractPng(stream);
	if (res == PNG_ERR_INVALID_SIG) {
		/* file isn't a .png image, probably a .zip archive then */
		res = Zip_Extract(stream, SelectZipEntry, ProcessZipEntry);

		if (res) Logger_SysWarn2(res, "extracting", path);
	} else if (res) {
		Logger_SysWarn2(res, "decoding", path);
	}
	return res;
}

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
	return res;
}

/* Extracts and updates cache for the downloaded texture pack */
static void ApplyDownloaded(struct HttpRequest* item) {
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
