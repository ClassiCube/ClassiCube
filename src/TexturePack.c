#include "TexturePack.h"
#include "Constants.h"
#include "Platform.h"
#include "Stream.h"
#include "World.h"
#include "Graphics.h"
#include "Event.h"
#include "Game.h"
#include "AsyncDownloader.h"
#include "Platform.h"
#include "Deflate.h"
#include "Stream.h"
#include "Funcs.h"
#include "Errors.h"
#include "Chat.h"
#include "ExtMath.h"
#include "Chat.h"
#include "Options.h"

#define LIQUID_ANIM_MAX 64
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
static bool L_rndInitalised;

static void LavaAnimation_Tick(BitmapCol* ptr, int size) {
	int mask = size - 1, shift = Math_Log2(size);
	float soupHeat, potHeat, col;
	int x, y, i = 0;

	if (!L_rndInitalised) {
		Random_InitFromCurrentTime(&L_rnd);
		L_rndInitalised = true;
	}
	
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the colour at this coordinate in the heatmap */

			/* Lookup table for (int)(1.2 * sin([ANGLE] * 22.5 * MATH_DEG2RAD)); */
			/* [ANGLE] is integer x/y, so repeats every 16 intervals */
			static int8_t sin_adj_table[16] = { 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 };
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

			ptr->R = (uint8_t)(col * 100.0f + 155.0f);
			ptr->G = (uint8_t)(col * col * 255.0f);
			ptr->B = (uint8_t)(col * col * col * col * 128.0f);
			ptr->A = 255;

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
static bool W_rndInitalised;

static void WaterAnimation_Tick(BitmapCol* ptr, int size) {
	int mask = size - 1, shift = Math_Log2(size);
	float soupHeat, col;
	int x, y, i = 0;

	if (!W_rndInitalised) {
		Random_InitFromCurrentTime(&W_rnd);
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

			ptr->R = (uint8_t)(32.0f  + col * 32.0f);
			ptr->G = (uint8_t)(50.0f  + col * 64.0f);
			ptr->A = (uint8_t)(146.0f + col * 50.0f);
			ptr->B = 255;

			ptr++; i++;
		}
	}
}


/*########################################################################################################################*
*-------------------------------------------------------Animations--------------------------------------------------------*
*#########################################################################################################################*/
struct AnimationData {
	TextureLoc TexLoc;       /* Tile (not pixel) coordinates in terrain.png */
	uint16_t FrameX, FrameY; /* Top left pixel coordinates of start frame in animatons.png */
	uint16_t FrameSize;      /* Size of each frame in pixel coordinates */
	uint16_t State;          /* Current animation frame index */
	uint16_t StatesCount;    /* Total number of animation frames */
	int16_t  Tick, TickDelay;
};

static Bitmap anims_bmp;
static struct AnimationData anims_list[ATLAS1D_MAX_ATLASES];
static int anims_count;
static bool anims_validated, anims_useLavaAnim, anims_useWaterAnim;
#define ANIM_MIN_ARGS 7

static void Animations_ReadDescription(struct Stream* stream, const String* path) {
	String line; char lineBuffer[STRING_SIZE * 2];
	String parts[ANIM_MIN_ARGS];
	int count;
	struct AnimationData data = { 0 };
	uint8_t tileX, tileY;

	uint8_t buffer[2048]; 
	struct Stream buffered;
	ReturnCode res;

	String_InitArray(line, lineBuffer);
	/* ReadLine reads single byte at a time */
	Stream_ReadonlyBuffered(&buffered, stream, buffer, sizeof(buffer));

	for (;;) {
		res = Stream_ReadLine(&buffered, &line);
		if (res == ERR_END_OF_STREAM) break;
		if (res) { Chat_LogError2(res, "reading from", path); break; }

		if (!line.length || line.buffer[0] == '#') continue;
		count = String_UNSAFE_Split(&line, ' ', parts, ANIM_MIN_ARGS);
		if (count < ANIM_MIN_ARGS) {
			Chat_Add1("&cNot enough arguments for anim: %s", &line); continue;
		}

		if (!Convert_ParseUInt8(&parts[0], &tileX) || tileX >= ATLAS2D_TILES_PER_ROW) {
			Chat_Add1("&cInvalid anim tile X coord: %s", &line); continue;
		}
		if (!Convert_ParseUInt8(&parts[1], &tileY) || tileY >= ATLAS2D_MAX_ROWS_COUNT) {
			Chat_Add1("&cInvalid anim tile Y coord: %s", &line); continue;
		}
		if (!Convert_ParseUInt16(&parts[2], &data.FrameX)) {
			Chat_Add1("&cInvalid anim frame X coord: %s", &line); continue;
		}
		if (!Convert_ParseUInt16(&parts[3], &data.FrameY)) {
			Chat_Add1("&cInvalid anim frame Y coord: %s", &line); continue;
		}
		if (!Convert_ParseUInt16(&parts[4], &data.FrameSize)) {
			Chat_Add1("&cInvalid anim frame size: %s", &line); continue;
		}
		if (!Convert_ParseUInt16(&parts[5], &data.StatesCount)) {
			Chat_Add1("&cInvalid anim states count: %s", &line); continue;
		}
		if (!Convert_ParseInt16(&parts[6], &data.TickDelay)) {
			Chat_Add1("&cInvalid anim tick delay: %s", &line); continue;
		}

		if (anims_count == Array_Elems(anims_list)) {
			Chat_AddRaw("&cCannot show over 512 animations"); return;
		}

		data.TexLoc = tileX + (tileY * ATLAS2D_TILES_PER_ROW);
		anims_list[anims_count++] = data;
	}
}

#define ANIMS_FAST_SIZE 64
static void Animations_Draw(struct AnimationData* data, TextureLoc texLoc, int size) {
	int dstX = Atlas1D_Index(texLoc), srcX;
	int dstY = Atlas1D_RowId(texLoc) * Atlas_TileSize;
	GfxResourceID tex;

	uint8_t buffer[Bitmap_DataSize(ANIMS_FAST_SIZE, ANIMS_FAST_SIZE)];
	uint8_t* ptr = buffer;
	Bitmap frame;

	/* cannot allocate memory on the stack for very big animation.png frames */
	if (size > ANIMS_FAST_SIZE) {	
		ptr = Mem_Alloc(size * size, 4, "anim frame");
	}
	Bitmap_Create(&frame, size, size, ptr);

	if (!data) {
		if (texLoc == 30) {
			LavaAnimation_Tick((BitmapCol*)frame.Scan0, size);
		} else if (texLoc == 14) {
			WaterAnimation_Tick((BitmapCol*)frame.Scan0, size);
		}
	} else {
		srcX = data->FrameX + data->State * size;
		Bitmap_CopyBlock(srcX, data->FrameY, 0, 0, &anims_bmp, &frame, size);
	}

	tex = Atlas1D_TexIds[dstX];
	if (tex) { Gfx_UpdateTexturePart(tex, 0, dstY, &frame, Gfx_Mipmaps); }
	if (size > ANIMS_FAST_SIZE) Mem_Free(ptr);
}

static void Animations_Apply(struct AnimationData* data) {
	TextureLoc loc;
	data->Tick--;
	if (data->Tick >= 0) return;

	data->State++;
	data->State %= data->StatesCount;
	data->Tick   = data->TickDelay;

	loc = data->TexLoc;
	if (loc == 30 && anims_useLavaAnim) return;
	if (loc == 14 && anims_useWaterAnim) return;
	Animations_Draw(data, loc, data->FrameSize);
}

static bool Animations_IsDefaultZip(void) {	
	String texPack;
	bool optExists;
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

		maxX  = data.FrameX + data.FrameSize * data.StatesCount;
		maxY  = data.FrameY + data.FrameSize;
		tileX = Atlas2D_TileX(data.TexLoc); 
		tileY = Atlas2D_TileY(data.TexLoc);

		if (data.FrameSize > Atlas_TileSize || tileY >= Atlas_RowsCount) {
			Chat_Add2("&cAnimation frames for tile (%i, %i) are bigger than the size of a tile in terrain.png", &tileX, &tileY);
		} else if (maxX > anims_bmp.Width || maxY > anims_bmp.Height) {
			Chat_Add2("&cSome of the animation frames for tile (%i, %i) are at coordinates outside animations.png", &tileX, &tileY);
		} else {
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

	if (anims_useLavaAnim) {
		size = min(Atlas_TileSize, 64);
		Animations_Draw(NULL, 30, size);
	}
	if (anims_useWaterAnim) {
		size = min(Atlas_TileSize, 64);
		Animations_Draw(NULL, 14, size);
	}

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
static void Animations_PackChanged(void* obj) {
	Animations_Clear();
	anims_useLavaAnim = Animations_IsDefaultZip();
	anims_useWaterAnim = anims_useLavaAnim;
}

static void Animations_FileChanged(void* obj, struct Stream* stream, const String* name) {
	ReturnCode res;
	if (String_CaselessEqualsConst(name, "animations.png")) {
		res = Png_Decode(&anims_bmp, stream);
		if (!res) return;

		Chat_LogError2(res, "decoding", name);
		Mem_Free(anims_bmp.Scan0);
		anims_bmp.Scan0 = NULL;
	} else if (String_CaselessEqualsConst(name, "animations.txt")) {
		Animations_ReadDescription(stream, name);
	} else if (String_CaselessEqualsConst(name, "uselavaanim")) {
		anims_useLavaAnim = true;
	} else if (String_CaselessEqualsConst(name, "usewateranim")) {
		anims_useWaterAnim = true;
	}
}

static void Animations_Init(void) {
	ScheduledTask_Add(GAME_DEF_TICKS, Animations_Tick);
	Event_RegisterVoid(&TextureEvents_PackChanged,  NULL, Animations_PackChanged);
	Event_RegisterEntry(&TextureEvents_FileChanged, NULL, Animations_FileChanged);
}

static void Animations_Free(void) {
	Animations_Clear();
	Event_UnregisterVoid(&TextureEvents_PackChanged,  NULL, Animations_PackChanged);
	Event_UnregisterEntry(&TextureEvents_FileChanged, NULL, Animations_FileChanged);
}

struct IGameComponent Animations_Component = {
	Animations_Init, /* Init  */
	Animations_Free  /* Free  */
};


/*########################################################################################################################*
*------------------------------------------------------TerrainAtlas-------------------------------------------------------*
*#########################################################################################################################*/
Bitmap Atlas_Bitmap;
int Atlas_TileSize, Atlas_RowsCount;
int Atlas1D_Count, Atlas1D_TilesPerAtlas;
int Atlas1D_Mask, Atlas1D_Shift;
float Atlas1D_InvTileSize;
GfxResourceID Atlas1D_TexIds[ATLAS1D_MAX_ATLASES];

TextureRec Atlas1D_TexRec(TextureLoc texLoc, int uCount, int* index) {
	TextureRec rec;
	int y  = Atlas1D_RowId(texLoc);
	*index = Atlas1D_Index(texLoc);

	/* Adjust coords to be slightly inside - fixes issues with AMD/ATI cards */	
	rec.U1 = 0.0f; 
	rec.V1 = y * Atlas1D_InvTileSize;
	rec.U2 = (uCount - 1) + UV2_Scale;
	rec.V2 = rec.V1       + UV2_Scale * Atlas1D_InvTileSize;
	return rec;
}

static void Atlas_Convert2DTo1D(void) {
	int tileSize      = Atlas_TileSize;
	int tilesPerAtlas = Atlas1D_TilesPerAtlas;
	int atlasesCount  = Atlas1D_Count;
	Bitmap atlas1D;
	int atlasX, atlasY;
	int tile = 0, i, y;

	Platform_Log2("Loaded new atlas: %i bmps, %i per bmp", &atlasesCount, &tilesPerAtlas);
	Bitmap_Allocate(&atlas1D, tileSize, tilesPerAtlas * tileSize);
	
	for (i = 0; i < atlasesCount; i++) {
		for (y = 0; y < tilesPerAtlas; y++, tile++) {
			atlasX = Atlas2D_TileX(tile) * tileSize;
			atlasY = Atlas2D_TileY(tile) * tileSize;

			Bitmap_CopyBlock(atlasX, atlasY, 0, y * tileSize,
				&Atlas_Bitmap, &atlas1D, tileSize);
		}
		Atlas1D_TexIds[i] = Gfx_CreateTexture(&atlas1D, true, Gfx_Mipmaps);
	}
	Mem_Free(atlas1D.Scan0);
}

static void Atlas_Update1D(void) {
	int maxAtlasHeight, maxTilesPerAtlas, maxTiles;

	maxAtlasHeight   = min(4096, Gfx_MaxTexHeight);
	maxTilesPerAtlas = maxAtlasHeight / Atlas_TileSize;
	maxTiles         = Atlas_RowsCount * ATLAS2D_TILES_PER_ROW;

	Atlas1D_TilesPerAtlas = min(maxTilesPerAtlas, maxTiles);
	Atlas1D_Count = Math_CeilDiv(maxTiles, Atlas1D_TilesPerAtlas);

	Atlas1D_InvTileSize = 1.0f / Atlas1D_TilesPerAtlas;
	Atlas1D_Mask  = Atlas1D_TilesPerAtlas - 1;
	Atlas1D_Shift = Math_Log2(Atlas1D_TilesPerAtlas);
}

void Atlas_Update(Bitmap* bmp) {
	Atlas_Bitmap    = *bmp;
	Atlas_TileSize  = bmp->Width  / ATLAS2D_TILES_PER_ROW;
	Atlas_RowsCount = bmp->Height / Atlas_TileSize;
	Atlas_RowsCount = min(Atlas_RowsCount, ATLAS2D_MAX_ROWS_COUNT);

	Atlas_Update1D();
	Atlas_Convert2DTo1D();
}

static GfxResourceID Atlas_LoadTile_Raw(TextureLoc texLoc, Bitmap* element) {
	int size = Atlas_TileSize;
	int x = Atlas2D_TileX(texLoc), y = Atlas2D_TileY(texLoc);
	if (y >= Atlas_RowsCount) return GFX_NULL;

	Bitmap_CopyBlock(x * size, y * size, 0, 0, &Atlas_Bitmap, element, size);
	return Gfx_CreateTexture(element, false, Gfx_Mipmaps);
}

GfxResourceID Atlas_LoadTile(TextureLoc texLoc) {
	int tileSize = Atlas_TileSize;
	Bitmap tile;
	GfxResourceID texId;
	uint8_t scan0[Bitmap_DataSize(64, 64)];

	/* Try to allocate bitmap on stack if possible */
	if (tileSize > 64) {
		Bitmap_Allocate(&tile, tileSize, tileSize);
		texId = Atlas_LoadTile_Raw(texLoc, &tile);
		Mem_Free(tile.Scan0);
		return texId;
	} else {	
		Bitmap_Create(&tile, tileSize, tileSize, scan0);
		return Atlas_LoadTile_Raw(texLoc, &tile);
	}
}

void Atlas_Free(void) {
	int i;
	Mem_Free(Atlas_Bitmap.Scan0);
	Atlas_Bitmap.Scan0 = NULL;

	for (i = 0; i < Atlas1D_Count; i++) {
		Gfx_DeleteTexture(&Atlas1D_TexIds[i]);
	}
}


/*########################################################################################################################*
*------------------------------------------------------TextureCache-------------------------------------------------------*
*#########################################################################################################################*/
/* Because I didn't store milliseconds in original C# client */
#define TEXCACHE_TICKS_PER_MS 10000
static struct EntryList cache_accepted, cache_denied, cache_eTags, cache_lastModified;

void TextureCache_Init(void) {
	EntryList_Init(&cache_accepted,     "texturecache", "acceptedurls.txt", ' ');
	EntryList_Init(&cache_denied,       "texturecache", "deniedurls.txt",   ' ');
	EntryList_Init(&cache_eTags,        "texturecache", "etags.txt",        ' ');
	EntryList_Init(&cache_lastModified, "texturecache", "lastmodified.txt", ' ');
}

bool TextureCache_HasAccepted(const String* url) { return EntryList_Find(&cache_accepted, url) >= 0; }
bool TextureCache_HasDenied(const String* url)   { return EntryList_Find(&cache_denied,   url) >= 0; }

void TextureCache_Accept(const String* url)      { 
	EntryList_Set(&cache_accepted, url, &String_Empty); 
	EntryList_Save(&cache_accepted);
}
void TextureCache_Deny(const String* url)        { 
	EntryList_Set(&cache_denied,   url, &String_Empty); 
	EntryList_Save(&cache_denied);
}

CC_NOINLINE static void TextureCache_MakePath(String* path, const String* url) {
	String key; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	String_AppendUInt32(&key, Utils_CRC32(url->buffer, url->length));
	String_Format1(path, "texturecache/%s", &key);
}

bool TextureCache_Has(const String* url) {
	String path; char pathBuffer[FILENAME_SIZE];
	String_InitArray(path, pathBuffer);

	TextureCache_MakePath(&path, url);
	return File_Exists(&path);
}

bool TextureCache_Get(const String* url, struct Stream* stream) {
	String path; char pathBuffer[FILENAME_SIZE];
	ReturnCode res;

	String_InitArray(path, pathBuffer);
	TextureCache_MakePath(&path, url);
	res = Stream_OpenFile(stream, &path);

	if (res == ReturnCode_FileNotFound) return false;
	if (res) { Chat_LogError2(res, "opening cache for", url); return false; }
	return true;
}

void TexturePack_GetFromTags(const String* url, String* result, struct EntryList* list) {
	String key, value; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	String_AppendUInt32(&key, Utils_CRC32(url->buffer, url->length));	
	value = EntryList_UNSAFE_Get(list, &key);
	if (value.length) String_AppendString(result, &value);
}

void TextureCache_GetLastModified(const String* url, TimeMS* time) {
	String path;  char pathBuffer[FILENAME_SIZE];
	String entry; char entryBuffer[STRING_SIZE];
	ReturnCode res;

	String_InitArray(entry, entryBuffer);
	TexturePack_GetFromTags(url, &entry, &cache_lastModified);

	if (entry.length && Convert_ParseUInt64(&entry, time)) {
		*time /= TEXCACHE_TICKS_PER_MS;
	} else {
		String_InitArray(path, pathBuffer);
		TextureCache_MakePath(&path, url);

		res = File_GetModifiedTime(&path, time);
		if (res) { Chat_LogError2(res, "getting last modified time of", url); *time = 0; }
	}
}

void TextureCache_GetETag(const String* url, String* etag) {
	TexturePack_GetFromTags(url, etag, &cache_eTags);
}

void TextureCache_Set(const String* url, uint8_t* data, uint32_t length) {
	String path; char pathBuffer[FILENAME_SIZE];
	ReturnCode res;

	String_InitArray(path, pathBuffer);
	TextureCache_MakePath(&path, url);
	if (!Utils_EnsureDirectory("texturecache")) return;
	
	res = Stream_WriteAllTo(&path, data, length);
	if (res) { Chat_LogError2(res, "caching", url); }
}

CC_NOINLINE static void TextureCache_SetEntry(const String* url, const String* data, struct EntryList* list) {
	String key; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	String_AppendUInt32(&key, Utils_CRC32(url->buffer, url->length));
	EntryList_Set(list, &key, data);
	EntryList_Save(list);
}

void TextureCache_SetETag(const String* url, const String* etag) {
	if (!etag->length) return;
	TextureCache_SetEntry(url, etag, &cache_eTags);
}

void TextureCache_SetLastModified(const String* url, const TimeMS* lastModified) {
	String data; char dataBuffer[STRING_SIZE];
	uint64_t ticks = (*lastModified) * TEXCACHE_TICKS_PER_MS;
	if (!ticks) return;

	String_InitArray(data, dataBuffer);
	String_AppendUInt64(&data, ticks);
	TextureCache_SetEntry(url, &data, &cache_lastModified);
}


/*########################################################################################################################*
*-------------------------------------------------------TexturePack-------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode TexturePack_ProcessZipEntry(const String* path, struct Stream* stream, struct ZipState* s) {
	String name = *path; 
	Utils_UNSAFE_GetFilename(&name);
	Event_RaiseEntry(&TextureEvents_FileChanged, stream, &name);
	return 0;
}

static ReturnCode TexturePack_ExtractZip(struct Stream* stream) {
	struct ZipState state;
	Event_RaiseVoid(&TextureEvents_PackChanged);
	if (Gfx_LostContext) return 0;
	
	Zip_Init(&state, stream);
	state.ProcessEntry = TexturePack_ProcessZipEntry;
	return Zip_Extract(&state);
}

void TexturePack_ExtractZip_File(const String* filename) {
	String path; char pathBuffer[FILENAME_SIZE];
	struct Stream stream;
	ReturnCode res;

	String_InitArray(path, pathBuffer);
	String_Format1(&path, "texpacks/%s", filename);

	res = Stream_OpenFile(&stream, &path);
	if (res) { Chat_LogError2(res, "opening", &path); return; }

	res = TexturePack_ExtractZip(&stream);
	if (res) { Chat_LogError2(res, "extracting", &path); }

	res = stream.Close(&stream);
	if (res) { Chat_LogError2(res, "closing", &path); }
}

ReturnCode TexturePack_ExtractTerrainPng(struct Stream* stream) {
	Bitmap bmp; 
	ReturnCode res = Png_Decode(&bmp, stream);

	if (!res) {
		Event_RaiseVoid(&TextureEvents_PackChanged);
		if (Game_ChangeTerrainAtlas(&bmp)) return 0;
	}

	Mem_Free(bmp.Scan0);
	return res;
}

void TexturePack_ExtractDefault(void) {
	String texPack; char texPackBuffer[STRING_SIZE];

	String_InitArray(texPack, texPackBuffer);
	Game_GetDefaultTexturePack(&texPack);

	TexturePack_ExtractZip_File(&texPack);
	World_TextureUrl.length = 0;
}

void TexturePack_ExtractCurrent(const String* url) {
	const static String zipExt = String_FromConst(".zip");
	struct Stream stream;
	bool zip;
	ReturnCode res = 0;

	if (!url->length) { TexturePack_ExtractDefault(); return; }
	
	if (!TextureCache_Get(url, &stream)) {
		/* e.g. 404 errors */
		if (World_TextureUrl.length) TexturePack_ExtractDefault();
	} else {
		if (!String_Equals(url, &World_TextureUrl)) {
			zip = String_ContainsString(url, &zipExt);
			String_Copy(&World_TextureUrl, url);

			res = zip ? TexturePack_ExtractZip(&stream) :
						TexturePack_ExtractTerrainPng(&stream);		
			if (res) Chat_LogError2(res, zip ? "extracting" : "decoding", url);
		}

		res = stream.Close(&stream);
		if (res) { Chat_LogError2(res, "closing cache for", url); }
	}
}

void TexturePack_Extract_Req(struct AsyncRequest* item) {
	String url, etag;
	void* data; uint32_t len;
	struct Stream mem;
	bool png;
	ReturnCode res;

	url  = String_FromRawArray(item->URL);
	String_Copy(&World_TextureUrl, &url);
	data = item->Data;
	len  = item->Size;

	etag = String_FromRawArray(item->Etag);
	TextureCache_Set(&url, data, len);
	TextureCache_SetETag(&url, &etag);
	TextureCache_SetLastModified(&url, &item->LastModified);

	Stream_ReadonlyMemory(&mem, data, len);
	png = Png_Detect(data, len);
	res = png ? TexturePack_ExtractTerrainPng(&mem) 
			 : TexturePack_ExtractZip(&mem);

	if (res) Chat_LogError2(res, png ? "decoding" : "extracting", &url);
	ASyncRequest_Free(item);
}
