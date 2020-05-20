#include "TexturePack.h"
#include "Constants.h"
#include "Platform.h"
#include "Stream.h"
#include "World.h"
#include "Graphics.h"
#include "Event.h"
#include "Game.h"
#include "Http.h"
#include "Platform.h"
#include "Deflate.h"
#include "Stream.h"
#include "Funcs.h"
#include "Errors.h"
#include "Chat.h"
#include "ExtMath.h"
#include "Chat.h"
#include "Options.h"
#include "Logger.h"
#include "GameStructs.h"

#define LIQUID_ANIM_MAX 64
#define WATER_TEX_LOC 14
#define LAVA_TEX_LOC  30

#ifndef CC_BUILD_WEB
/* Based off the incredible work from https://dl.dropboxusercontent.com/u/12694594/lava.txt
	mirrored at https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-lava-animation-algorithm
	Water animation originally written by cybertoon, big thanks!
*/
/*########################################################################################################################*
*-----------------------------------------------------Lava animation------------------------------------------------------*
*#########################################################################################################################*/
static float L_soupHeat[LIQUID_ANIM_MAX  * LIQUID_ANIM_MAX];
static float L_potHeat[LIQUID_ANIM_MAX   * LIQUID_ANIM_MAX];
static float L_flameHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
static RNGState L_rnd;
static cc_bool  L_rndInitalised;

static void LavaAnimation_Tick(BitmapCol* ptr, int size) {
	int mask = size - 1, shift = Math_Log2(size);
	float soupHeat, potHeat, col;
	int x, y, i = 0;

	if (!L_rndInitalised) {
		Random_SeedFromCurrentTime(&L_rnd);
		L_rndInitalised = true;
	}
	
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the colour at this coordinate in the heatmap */

			/* Lookup table for (int)(1.2 * sin([ANGLE] * 22.5 * MATH_DEG2RAD)); */
			/* [ANGLE] is integer x/y, so repeats every 16 intervals */
			static cc_int8 sin_adj_table[16] = { 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 };
			int xx = x + sin_adj_table[y & 0xF], yy = y + sin_adj_table[x & 0xF];

			soupHeat =
				L_soupHeat[((yy - 1) & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[((yy - 1) & mask) << shift | (xx       & mask)] +
				L_soupHeat[((yy - 1) & mask) << shift | ((xx + 1) & mask)] +

				L_soupHeat[(yy & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[(yy & mask) << shift | (xx       & mask)] +
				L_soupHeat[(yy & mask) << shift | ((xx + 1) & mask)] +

				L_soupHeat[((yy + 1) & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[((yy + 1) & mask) << shift | (xx       & mask)] +
				L_soupHeat[((yy + 1) & mask) << shift | ((xx + 1) & mask)];

			potHeat =
				L_potHeat[i] +                                          /* x    , y     */
				L_potHeat[y << shift | ((x + 1) & mask)] +              /* x + 1, y     */
				L_potHeat[((y + 1) & mask) << shift | x] +              /* x    , y + 1 */
				L_potHeat[((y + 1) & mask) << shift | ((x + 1) & mask)];/* x + 1, y + 1 */

			L_soupHeat[i] = soupHeat * 0.1f + potHeat * 0.2f;

			L_potHeat[i] += L_flameHeat[i];
			if (L_potHeat[i] < 0.0f) L_potHeat[i] = 0.0f;

			L_flameHeat[i] -= 0.06f * 0.01f;
			if (Random_Float(&L_rnd) <= 0.005f) L_flameHeat[i] = 1.5f * 0.01f;

			/* Output the pixel */
			col = 2.0f * L_soupHeat[i];
			Math_Clamp(col, 0.0f, 1.0f);

			*ptr = BitmapCol_Make(
				col * 100.0f + 155.0f,
				col * col * 255.0f,
				col * col * col * col * 128.0f,
				255);

			ptr++; i++;
		}
	}
}


/*########################################################################################################################*
*----------------------------------------------------Water animation------------------------------------------------------*
*#########################################################################################################################*/
static float W_soupHeat[LIQUID_ANIM_MAX  * LIQUID_ANIM_MAX];
static float W_potHeat[LIQUID_ANIM_MAX   * LIQUID_ANIM_MAX];
static float W_flameHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
static RNGState W_rnd;
static cc_bool  W_rndInitalised;

static void WaterAnimation_Tick(BitmapCol* ptr, int size) {
	int mask = size - 1, shift = Math_Log2(size);
	float soupHeat, col;
	int x, y, i = 0;

	if (!W_rndInitalised) {
		Random_SeedFromCurrentTime(&W_rnd);
		W_rndInitalised = true;
	}
	
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the colour at this coordinate in the heatmap */
			soupHeat =
				W_soupHeat[y << shift | ((x - 1) & mask)] +
				W_soupHeat[y << shift | x               ] +
				W_soupHeat[y << shift | ((x + 1) & mask)];

			W_soupHeat[i] = soupHeat / 3.3f + W_potHeat[i] * 0.8f;

			W_potHeat[i] += W_flameHeat[i];
			if (W_potHeat[i] < 0.0f) W_potHeat[i] = 0.0f;

			W_flameHeat[i] -= 0.1f * 0.05f;
			if (Random_Float(&W_rnd) <= 0.05f) W_flameHeat[i] = 0.5f * 0.05f;

			/* Output the pixel */
			col = W_soupHeat[i];
			Math_Clamp(col, 0.0f, 1.0f);
			col = col * col;

			*ptr = BitmapCol_Make(
				32.0f  + col * 32.0f,
				50.0f  + col * 64.0f,
				255,
				146.0f + col * 50.0f);

			ptr++; i++;
		}
	}
}
#endif


/*########################################################################################################################*
*-------------------------------------------------------Animations--------------------------------------------------------*
*#########################################################################################################################*/
struct AnimationData {
	TextureLoc texLoc;       /* Tile (not pixel) coordinates in terrain.png */
	cc_uint16 frameX, frameY; /* Top left pixel coordinates of start frame in animatons.png */
	cc_uint16 frameSize;      /* Size of each frame in pixel coordinates */
	cc_uint16 state;          /* Current animation frame index */
	cc_uint16 statesCount;    /* Total number of animation frames */
	cc_uint16 delay;          /* Delay in ticks until next frame is drawn */
	cc_uint16 frameDelay;     /* Delay between each frame */
};

static Bitmap anims_bmp;
static struct AnimationData anims_list[ATLAS1D_MAX_ATLASES];
static int anims_count;
static cc_bool anims_validated, useLavaAnim, useWaterAnim, alwaysLavaAnim, alwaysWaterAnim;
#define ANIM_MIN_ARGS 7

static void Animations_ReadDescription(struct Stream* stream, const String* path) {
	String line; char lineBuffer[STRING_SIZE * 2];
	String parts[ANIM_MIN_ARGS];
	int count;
	struct AnimationData data = { 0 };
	cc_uint8 tileX, tileY;

	cc_uint8 buffer[2048]; 
	struct Stream buffered;
	cc_result res;

	String_InitArray(line, lineBuffer);
	/* ReadLine reads single byte at a time */
	Stream_ReadonlyBuffered(&buffered, stream, buffer, sizeof(buffer));

	for (;;) {
		res = Stream_ReadLine(&buffered, &line);
		if (res == ERR_END_OF_STREAM) break;
		if (res) { Logger_Warn2(res, "reading from", path); break; }

		if (!line.length || line.buffer[0] == '#') continue;
		count = String_UNSAFE_Split(&line, ' ', parts, ANIM_MIN_ARGS);
		if (count < ANIM_MIN_ARGS) {
			Chat_Add1("&cNot enough arguments for anim: %s", &line); continue;
		}

		if (!Convert_ParseUInt8(&parts[0], &tileX) || tileX >= ATLAS2D_TILES_PER_ROW) {
			Chat_Add1("&cInvalid anim tile X coord: %s", &parts[0]); continue;
		}
		if (!Convert_ParseUInt8(&parts[1], &tileY) || tileY >= ATLAS2D_MAX_ROWS_COUNT) {
			Chat_Add1("&cInvalid anim tile Y coord: %s", &parts[1]); continue;
		}
		if (!Convert_ParseUInt16(&parts[2], &data.frameX)) {
			Chat_Add1("&cInvalid anim frame X coord: %s", &parts[2]); continue;
		}
		if (!Convert_ParseUInt16(&parts[3], &data.frameY)) {
			Chat_Add1("&cInvalid anim frame Y coord: %s", &parts[3]); continue;
		}
		if (!Convert_ParseUInt16(&parts[4], &data.frameSize) || !data.frameSize) {
			Chat_Add1("&cInvalid anim frame size: %s", &parts[4]); continue;
		}
		if (!Convert_ParseUInt16(&parts[5], &data.statesCount)) {
			Chat_Add1("&cInvalid anim states count: %s", &parts[5]); continue;
		}
		if (!Convert_ParseUInt16(&parts[6], &data.frameDelay)) {
			Chat_Add1("&cInvalid anim frame delay: %s", &parts[6]); continue;
		}

		if (anims_count == Array_Elems(anims_list)) {
			Chat_AddRaw("&cCannot show over 512 animations"); return;
		}

		data.texLoc = tileX + (tileY * ATLAS2D_TILES_PER_ROW);
		anims_list[anims_count++] = data;
	}
}

#define ANIMS_FAST_SIZE 64
static void Animations_Draw(struct AnimationData* data, TextureLoc texLoc, int size) {
	int dstX = Atlas1D_Index(texLoc), srcX;
	int dstY = Atlas1D_RowId(texLoc) * Atlas2D.TileSize;
	GfxResourceID tex;

	cc_uint8 buffer[Bitmap_DataSize(ANIMS_FAST_SIZE, ANIMS_FAST_SIZE)];
	cc_uint8* ptr = buffer;
	Bitmap frame;

	/* cannot allocate memory on the stack for very big animation.png frames */
	if (size > ANIMS_FAST_SIZE) {	
		ptr = (cc_uint8*)Mem_Alloc(size * size, 4, "anim frame");
	}
	Bitmap_Init(frame, size, size, ptr);

	if (!data) {
#ifndef CC_BUILD_WEB
		if (texLoc == LAVA_TEX_LOC) {
			LavaAnimation_Tick((BitmapCol*)frame.Scan0, size);
		} else if (texLoc == WATER_TEX_LOC) {
			WaterAnimation_Tick((BitmapCol*)frame.Scan0, size);
		}
#endif
	} else {
		srcX = data->frameX + data->state * size;
		Bitmap_UNSAFE_CopyBlock(srcX, data->frameY, 0, 0, &anims_bmp, &frame, size);
	}

	tex = Atlas1D.TexIds[dstX];
	if (tex) { Gfx_UpdateTexturePart(tex, 0, dstY, &frame, Gfx.Mipmaps); }
	if (size > ANIMS_FAST_SIZE) Mem_Free(ptr);
}

static void Animations_Apply(struct AnimationData* data) {
	TextureLoc loc;
	if (data->delay) { data->delay--; return; }

	data->state++;
	data->state %= data->statesCount;
	data->delay  = data->frameDelay;

	loc = data->texLoc;
#ifndef CC_BUILD_WEB
	if (loc == LAVA_TEX_LOC  && useLavaAnim)  return;
	if (loc == WATER_TEX_LOC && useWaterAnim) return;
#endif
	Animations_Draw(data, loc, data->frameSize);
}

static cc_bool Animations_IsDefaultZip(void) {
	String texPack;
	cc_bool optExists;
	if (World_TextureUrl.length) return false;

	optExists = Options_UNSAFE_Get(OPT_DEFAULT_TEX_PACK, &texPack);
	return !optExists || String_CaselessEqualsConst(&texPack, "default.zip");
}

static void Animations_Clear(void) {
	Mem_Free(anims_bmp.Scan0);
	anims_count = 0;
	anims_bmp.Scan0 = NULL;
	anims_validated = false;
}

static void Animations_Validate(void) {
	struct AnimationData data;
	int maxX, maxY, tileX, tileY;
	int i, j;

	anims_validated = true;
	for (i = 0; i < anims_count; i++) {
		data  = anims_list[i];

		maxX  = data.frameX + data.frameSize * data.statesCount;
		maxY  = data.frameY + data.frameSize;
		tileX = Atlas2D_TileX(data.texLoc); 
		tileY = Atlas2D_TileY(data.texLoc);

		if (data.frameSize > Atlas2D.TileSize || tileY >= Atlas2D.RowsCount) {
			Chat_Add2("&cAnimation frames for tile (%i, %i) are bigger than the size of a tile in terrain.png", &tileX, &tileY);
		} else if (maxX > anims_bmp.Width || maxY > anims_bmp.Height) {
			Chat_Add2("&cSome of the animation frames for tile (%i, %i) are at coordinates outside animations.png", &tileX, &tileY);
		} else {
			/* if user has water/lava animations in their default.zip, disable built-in */
			/* However, 'usewateranim' and 'uselavaanim' files should always disable use */
			/* of custom water/lava animations, even when they exist in animations.png */
			if (data.texLoc == LAVA_TEX_LOC  && !alwaysLavaAnim)  useLavaAnim  = false;
			if (data.texLoc == WATER_TEX_LOC && !alwaysWaterAnim) useWaterAnim = false;		
			continue;
		}

		/* Remove this animation from the list */
		for (j = i; j < anims_count - 1; j++) {
			anims_list[j] = anims_list[j + 1];
		}
		i--; anims_count--;
	}
}

static void Animations_Tick(struct ScheduledTask* task) {
	int i, size;

#ifndef CC_BUILD_WEB
	if (useLavaAnim) {
		size = min(Atlas2D.TileSize, 64);
		Animations_Draw(NULL, LAVA_TEX_LOC, size);
	}
	if (useWaterAnim) {
		size = min(Atlas2D.TileSize, 64);
		Animations_Draw(NULL, WATER_TEX_LOC, size);
	}
#endif

	if (!anims_count) return;
	if (!anims_bmp.Scan0) {
		Chat_AddRaw("&cCurrent texture pack specifies it uses animations,");
		Chat_AddRaw("&cbut is missing animations.png");
		anims_count = 0; return;
	}

	/* deferred, because when reading animations.txt, might not have read animations.png yet */
	if (!anims_validated) Animations_Validate();
	for (i = 0; i < anims_count; i++) {
		Animations_Apply(&anims_list[i]);
	}
}


/*########################################################################################################################*
*--------------------------------------------------Animations component---------------------------------------------------*
*#########################################################################################################################*/
static void OnPackChanged(void* obj) {
	Animations_Clear();
	useLavaAnim     = Animations_IsDefaultZip();
	useWaterAnim    = useLavaAnim;
	alwaysLavaAnim  = false;
	alwaysWaterAnim = false;
}

static void OnFileChanged(void* obj, struct Stream* stream, const String* name) {
	cc_result res;
	if (String_CaselessEqualsConst(name, "animations.png")) {
		res = Png_Decode(&anims_bmp, stream);
		if (!res) return;

		Logger_Warn2(res, "decoding", name);
		Mem_Free(anims_bmp.Scan0);
		anims_bmp.Scan0 = NULL;
	} else if (String_CaselessEqualsConst(name, "animations.txt")) {
		Animations_ReadDescription(stream, name);
	} else if (String_CaselessEqualsConst(name, "uselavaanim")) {
		useLavaAnim    = true;
		alwaysLavaAnim = true;
	} else if (String_CaselessEqualsConst(name, "usewateranim")) {
		useWaterAnim    = true;
		alwaysWaterAnim = true;
	}
}

static void Animations_Init(void) {
	ScheduledTask_Add(GAME_DEF_TICKS, Animations_Tick);
	Event_RegisterVoid(&TextureEvents.PackChanged,  NULL, OnPackChanged);
	Event_RegisterEntry(&TextureEvents.FileChanged, NULL, OnFileChanged);
}

static void Animations_Free(void) {
	Animations_Clear();
	Event_UnregisterVoid(&TextureEvents.PackChanged,  NULL, OnPackChanged);
	Event_UnregisterEntry(&TextureEvents.FileChanged, NULL, OnFileChanged);
}

struct IGameComponent Animations_Component = {
	Animations_Init, /* Init  */
	Animations_Free  /* Free  */
};


/*########################################################################################################################*
*------------------------------------------------------TerrainAtlas-------------------------------------------------------*
*#########################################################################################################################*/
struct _Atlas2DData Atlas2D;
struct _Atlas1DData Atlas1D;

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
	Bitmap atlas1D;
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
	Mem_Free(atlas1D.Scan0);
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

void Atlas_Update(Bitmap* bmp) {
	Atlas2D.Bmp       = *bmp;
	Atlas2D.TileSize  = bmp->Width  / ATLAS2D_TILES_PER_ROW;
	Atlas2D.RowsCount = bmp->Height / Atlas2D.TileSize;
	Atlas2D.RowsCount = min(Atlas2D.RowsCount, ATLAS2D_MAX_ROWS_COUNT);

	Atlas_Update1D();
	Atlas_Convert2DTo1D();
}

static GfxResourceID Atlas_LoadTile_Raw(TextureLoc texLoc, Bitmap* element) {
	int size = Atlas2D.TileSize;
	int x = Atlas2D_TileX(texLoc), y = Atlas2D_TileY(texLoc);
	if (y >= Atlas2D.RowsCount) return 0;

	Bitmap_UNSAFE_CopyBlock(x * size, y * size, 0, 0, &Atlas2D.Bmp, element, size);
	return Gfx_CreateTexture(element, false, Gfx.Mipmaps);
}

GfxResourceID Atlas2D_LoadTile(TextureLoc texLoc) {
	int tileSize = Atlas2D.TileSize;
	Bitmap tile;
	GfxResourceID texId;
	cc_uint8 scan0[Bitmap_DataSize(64, 64)];

	/* Try to allocate bitmap on stack if possible */
	if (tileSize > 64) {
		Bitmap_Allocate(&tile, tileSize, tileSize);
		texId = Atlas_LoadTile_Raw(texLoc, &tile);
		Mem_Free(tile.Scan0);
		return texId;
	} else {	
		Bitmap_Init(tile, tileSize, tileSize, scan0);
		return Atlas_LoadTile_Raw(texLoc, &tile);
	}
}

void Atlas_Free(void) {
	int i;
	Mem_Free(Atlas2D.Bmp.Scan0);
	Atlas2D.Bmp.Scan0 = NULL;

	for (i = 0; i < Atlas1D.Count; i++) {
		Gfx_DeleteTexture(&Atlas1D.TexIds[i]);
	}
}


/*########################################################################################################################*
*------------------------------------------------------TextureCache-------------------------------------------------------*
*#########################################################################################################################*/
static struct EntryList acceptedList, deniedList, etagCache, lastModifiedCache;

void TextureCache_Init(void) {
	EntryList_Init(&acceptedList,      "texturecache/acceptedurls.txt", ' ');
	EntryList_Init(&deniedList,        "texturecache/deniedurls.txt",   ' ');
	EntryList_Init(&etagCache,         "texturecache/etags.txt",        ' ');
	EntryList_Init(&lastModifiedCache, "texturecache/lastmodified.txt", ' ');
}

cc_bool TextureCache_HasAccepted(const String* url) { return EntryList_Find(&acceptedList, url) >= 0; }
cc_bool TextureCache_HasDenied(const String* url)   { return EntryList_Find(&deniedList,   url) >= 0; }

void TextureCache_Accept(const String* url) { 
	EntryList_Set(&acceptedList, url, &String_Empty); 
	EntryList_Save(&acceptedList);
}
void TextureCache_Deny(const String* url) { 
	EntryList_Set(&deniedList,   url, &String_Empty); 
	EntryList_Save(&deniedList);
}

int TextureCache_ClearDenied(void) {
	int count = deniedList.entries.count;
	StringsBuffer_Clear(&deniedList.entries);
	EntryList_Save(&deniedList);
	return count;
}

CC_INLINE static void TextureCache_HashUrl(String* key, const String* url) {
	String_AppendUInt32(key, Utils_CRC32((const cc_uint8*)url->buffer, url->length));
}

CC_NOINLINE static void TextureCache_MakePath(String* path, const String* url) {
	String key; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	TextureCache_HashUrl(&key, url);
	String_Format1(path, "texturecache/%s", &key);
}

/* Returns non-zero if given URL has been cached */
static int TextureCache_Has(const String* url) {
	String path; char pathBuffer[FILENAME_SIZE];
	String_InitArray(path, pathBuffer);

	TextureCache_MakePath(&path, url);
	return File_Exists(&path);
}

/* Attempts to get the cached data stream for the given url */
static cc_bool TextureCache_Get(const String* url, struct Stream* stream) {
	String path; char pathBuffer[FILENAME_SIZE];
	cc_result res;

	String_InitArray(path, pathBuffer);
	TextureCache_MakePath(&path, url);
	res = Stream_OpenFile(stream, &path);

	if (res == ReturnCode_FileNotFound) return false;
	if (res) { Logger_Warn2(res, "opening cache for", url); return false; }
	return true;
}

CC_NOINLINE static String TextureCache_GetFromTags(const String* url, struct EntryList* list) {
	String key; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	TextureCache_HashUrl(&key, url);
	return EntryList_UNSAFE_Get(list, &key);
}

static String TextureCache_GetLastModified(const String* url) {
	int i;
	String entry = TextureCache_GetFromTags(url, &lastModifiedCache);
	/* Entry used to be a timestamp of C# ticks since 01/01/0001 */
	/* Check if this is new format */
	for (i = 0; i < entry.length; i++) {
		if (entry.buffer[i] < '0' || entry.buffer[i] > '9') return entry;
	}

	/* Entry is all digits, so the old unsupported format */
	entry.length = 0; return entry;
}

static String TextureCache_GetETag(const String* url) {
	return TextureCache_GetFromTags(url, &etagCache);
}

CC_NOINLINE static void TextureCache_SetEntry(const String* url, const String* data, struct EntryList* list) {
	String key; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	TextureCache_HashUrl(&key, url);
	EntryList_Set(list, &key, data);
	EntryList_Save(list);
}

static void TextureCache_SetETag(const String* url, const String* etag) {
	if (!etag->length) return;
	TextureCache_SetEntry(url, etag, &etagCache);
}

static void TextureCache_SetLastModified(const String* url, const String* time) {
	if (!time->length) return;
	TextureCache_SetEntry(url, time, &lastModifiedCache);
}

/* Updates cached data, ETag, and Last-Modified for the given URL */
static void TextureCache_Update(struct HttpRequest* req) {
	String path, url; char pathBuffer[FILENAME_SIZE];
	cc_result res;
	url = String_FromRawArray(req->url);

	path = String_FromRawArray(req->etag);
	TextureCache_SetETag(&url, &path);
	path = String_FromRawArray(req->lastModified);
	TextureCache_SetLastModified(&url, &path);

	String_InitArray(path, pathBuffer);
	TextureCache_MakePath(&path, &url);
	res = Stream_WriteAllTo(&path, req->data, req->size);
	if (res) { Logger_Warn2(res, "caching", &url); }
}


/*########################################################################################################################*
*-------------------------------------------------------TexturePack-------------------------------------------------------*
*#########################################################################################################################*/
static char defTexPackBuffer[STRING_SIZE];
static String defTexPack = String_FromArray(defTexPackBuffer);
static const String defaultZip = String_FromConst("default.zip");

/* Retrieves the filename of the default texture pack used. */
/* NOTE: Returns default.zip if classic mode or selected pack does not exist. */
static String TexturePack_UNSAFE_GetDefault(void) {
	String texPath; char texPathBuffer[STRING_SIZE];
	String_InitArray(texPath, texPathBuffer);

	String_Format1(&texPath, "texpacks/%s", &defTexPack);
	return File_Exists(&texPath) && !Game_ClassicMode ? defTexPack : defaultZip;
}

void TexturePack_SetDefault(const String* texPack) {
	String_Copy(&defTexPack, texPack);
	Options_Set(OPT_DEFAULT_TEX_PACK, texPack);
}

static cc_result TexturePack_ProcessZipEntry(const String* path, struct Stream* stream, struct ZipState* s) {
	String name = *path; 
	Utils_UNSAFE_GetFilename(&name);
	Event_RaiseEntry(&TextureEvents.FileChanged, stream, &name);
	return 0;
}

/* Extracts all the files from a stream representing a .zip archive */
static cc_result TexturePack_ExtractZip(struct Stream* stream) {
	struct ZipState state;
	Event_RaiseVoid(&TextureEvents.PackChanged);
	if (Gfx.LostContext) return 0;
	
	Zip_Init(&state, stream);
	state.ProcessEntry = TexturePack_ProcessZipEntry;
	return Zip_Extract(&state);
}

/* Changes the current terrain atlas from a stream representing a .png image */
/* Raises TextureEvents.PackChanged, so behaves as a .zip with only terrain.png in it */
static cc_result TexturePack_ExtractPng(struct Stream* stream) {
	Bitmap bmp; 
	cc_result res = Png_Decode(&bmp, stream);

	if (!res) {
		Event_RaiseVoid(&TextureEvents.PackChanged);
		if (Game_ChangeTerrainAtlas(&bmp)) return 0;
	}

	Mem_Free(bmp.Scan0);
	return res;
}

/* Extracts a .zip texture pack from the given file */
static void TexturePack_ExtractZip_File(const String* filename) {
	String path; char pathBuffer[FILENAME_SIZE];
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

	res = TexturePack_ExtractZip(&stream);
	if (res) { Logger_Warn2(res, "extracting", &path); }

	res = stream.Close(&stream);
	if (res) { Logger_Warn2(res, "closing", &path); }

#ifdef CC_BUILD_WEB
	Platform_SetDefaultCurrentDirectory(0, NULL);
#endif
}

void TexturePack_ExtractInitial(void) {
	String texPack;
	Options_Get(OPT_DEFAULT_TEX_PACK, &defTexPack, "default.zip");
	TexturePack_ExtractZip_File(&defaultZip);

	/* in case the user's default texture pack doesn't have all required textures */
	texPack = TexturePack_UNSAFE_GetDefault();
	if (!String_CaselessEqualsConst(&texPack, "default.zip")) {
		TexturePack_ExtractZip_File(&texPack);
	}
}

static cc_bool texturePackDefault = true;
void TexturePack_ExtractCurrent(cc_bool forceReload) {
	String url = World_TextureUrl, file;
	struct Stream stream;
	cc_bool zip;
	cc_result res;

	if (!url.length || !TextureCache_Get(&url, &stream)) {
		/* don't pointlessly load default texture pack */
		if (texturePackDefault && !forceReload) return;
		file = TexturePack_UNSAFE_GetDefault();

		TexturePack_ExtractZip_File(&file);
		texturePackDefault = true;
	} else {
		zip = String_ContainsConst(&url, ".zip");
		res = zip ? TexturePack_ExtractZip(&stream) : TexturePack_ExtractPng(&stream);
		if (res) Logger_Warn2(res, zip ? "extracting" : "decoding", &url);

		res = stream.Close(&stream);
		if (res) Logger_Warn2(res, "closing cache for", &url);
		texturePackDefault = false;
	}
}

void TexturePack_Apply(struct HttpRequest* item) {
	String url;
	cc_uint8* data; cc_uint32 len;
	struct Stream mem;
	cc_bool png;
	cc_result res;

	url = String_FromRawArray(item->url);
	TextureCache_Update(item);
	/* Took too long to download and is no longer active texture pack */
	if (!String_Equals(&World_TextureUrl, &url)) return;

	data = item->data;
	len  = item->size;
	Stream_ReadonlyMemory(&mem, data, len);

	png = Png_Detect(data, len);
	res = png ? TexturePack_ExtractPng(&mem) : TexturePack_ExtractZip(&mem);

	if (res) Logger_Warn2(res, png ? "decoding" : "extracting", &url);
	texturePackDefault = false;
}

void TexturePack_DownloadAsync(const String* url, const String* id) {
	String etag = String_Empty;
	String time = String_Empty;

	/* Only retrieve etag/last-modified headers if the file exists */
	/* This inconsistency can occur if user deleted some cached files */
	if (TextureCache_Has(url)) {
		time = TextureCache_GetLastModified(url);
		etag = TextureCache_GetETag(url);
	}
	Http_AsyncGetDataEx(url, true, id, &time, &etag, NULL);
}
