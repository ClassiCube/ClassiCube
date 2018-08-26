#include "Animations.h"
#include "ExtMath.h"
#include "TerrainAtlas.h"
#include "Platform.h"
#include "Event.h"
#include "Funcs.h"
#include "GraphicsAPI.h"
#include "Chat.h"
#include "World.h"
#include "Options.h"
#include "ErrorHandler.h"
#include "Errors.h"
#define LIQUID_ANIM_MAX 64

/*########################################################################################################################*
*-----------------------------------------------------Lava animation------------------------------------------------------*
*#########################################################################################################################*/
Real32 L_soupHeat[LIQUID_ANIM_MAX  * LIQUID_ANIM_MAX];
Real32 L_potHeat[LIQUID_ANIM_MAX   * LIQUID_ANIM_MAX];
Real32 L_flameHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
Random L_rnd;
bool L_rndInitalised;

static void LavaAnimation_Tick(UInt32* ptr, Int32 size) {
	if (!L_rndInitalised) {
		Random_InitFromCurrentTime(&L_rnd);
		L_rndInitalised = true;
	}
	Int32 mask = size - 1, shift = Math_Log2(size);

	Int32 x, y, i = 0;
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the colour at this coordinate in the heatmap */

			/* Lookup table for (Int32)(1.2f * Math_SinF([ANGLE] * 22.5f * MATH_DEG2RAD)); */
			/* [ANGLE] is integer x/y, so repeats every 16 intervals */
			static Int8 sin_adj_table[16] = { 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 };
			Int32 xx = x + sin_adj_table[y & 0xF], yy = y + sin_adj_table[x & 0xF];

			Real32 lSoupHeat =
				L_soupHeat[((yy - 1) & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[((yy - 1) & mask) << shift | (xx       & mask)] +
				L_soupHeat[((yy - 1) & mask) << shift | ((xx + 1) & mask)] +

				L_soupHeat[(yy & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[(yy & mask) << shift | (xx       & mask)] +
				L_soupHeat[(yy & mask) << shift | ((xx + 1) & mask)] +

				L_soupHeat[((yy + 1) & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[((yy + 1) & mask) << shift | (xx       & mask)] +
				L_soupHeat[((yy + 1) & mask) << shift | ((xx + 1) & mask)];

			Real32 lPotHeat =
				L_potHeat[i] +                                          /* x    , y     */
				L_potHeat[y << shift | ((x + 1) & mask)] +              /* x + 1, y     */
				L_potHeat[((y + 1) & mask) << shift | x] +              /* x    , y + 1 */
				L_potHeat[((y + 1) & mask) << shift | ((x + 1) & mask)];/* x + 1, y + 1 */

			L_soupHeat[i] = lSoupHeat * 0.1f + lPotHeat * 0.2f;

			L_potHeat[i] += L_flameHeat[i];
			if (L_potHeat[i] < 0.0f) L_potHeat[i] = 0.0f;

			L_flameHeat[i] -= 0.06f * 0.01f;
			if (Random_Float(&L_rnd) <= 0.005f) L_flameHeat[i] = 1.5f * 0.01f;

			/* Output the pixel */
			Real32 col = 2.0f * L_soupHeat[i];
			Math_Clamp(col, 0.0f, 1.0f);

			UInt8 r = (UInt8)(col * 100.0f + 155.0f);
			UInt8 g = (UInt8)(col * col * 255.0f);
			UInt8 b = (UInt8)(col * col * col * col * 128.0f);
			*ptr = PackedCol_ARGB(r, g, b, 255);

			ptr++; i++;
		}
	}
}


/*########################################################################################################################*
*----------------------------------------------------Water animation------------------------------------------------------*
*#########################################################################################################################*/
Real32 W_soupHeat[LIQUID_ANIM_MAX  * LIQUID_ANIM_MAX];
Real32 W_potHeat[LIQUID_ANIM_MAX   * LIQUID_ANIM_MAX];
Real32 W_flameHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
Random W_rnd;
bool W_rndInitalised;

static void WaterAnimation_Tick(UInt32* ptr, Int32 size) {
	if (!W_rndInitalised) {
		Random_InitFromCurrentTime(&W_rnd);
		W_rndInitalised = true;
	}
	Int32 mask = size - 1, shift = Math_Log2(size);

	Int32 x, y, i = 0;
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the colour at this coordinate in the heatmap */
			Real32 wSoupHeat =
				W_soupHeat[y << shift | ((x - 1) & mask)] +
				W_soupHeat[y << shift | x               ] +
				W_soupHeat[y << shift | ((x + 1) & mask)];

			W_soupHeat[i] = wSoupHeat / 3.3f + W_potHeat[i] * 0.8f;

			W_potHeat[i] += W_flameHeat[i];
			if (W_potHeat[i] < 0.0f) W_potHeat[i] = 0.0f;

			W_flameHeat[i] -= 0.1f * 0.05f;
			if (Random_Float(&W_rnd) <= 0.05f) W_flameHeat[i] = 0.5f * 0.05f;

			/* Output the pixel */
			Real32 col = W_soupHeat[i];
			Math_Clamp(col, 0.0f, 1.0f);
			col = col * col;

			UInt8 r = (UInt8)(32.0f  + col * 32.0f);
			UInt8 g = (UInt8)(50.0f  + col * 64.0f);
			UInt8 a = (UInt8)(146.0f + col * 50.0f);
			*ptr = PackedCol_ARGB(r, g, 255, a);

			ptr++; i++;
		}
	}
}


/*########################################################################################################################*
*-------------------------------------------------------Animations--------------------------------------------------------*
*#########################################################################################################################*/
struct AnimationData {
	TextureLoc TexLoc;     /* Tile (not pixel) coordinates in terrain.png */
	UInt16 FrameX, FrameY; /* Top left pixel coordinates of start frame in animatons.png */
	UInt16 FrameSize;      /* Size of each frame in pixel coordinates */
	UInt16 State;          /* Current animation frame index */
	UInt16 StatesCount;    /* Total number of animation frames */
	Int16 Tick, TickDelay;
};

struct Bitmap anims_bmp;
struct AnimationData anims_list[ATLAS1D_MAX_ATLASES];
UInt32 anims_count;
bool anims_validated, anims_useLavaAnim, anims_useWaterAnim;

static void Animations_ReadDescription(struct Stream* stream, STRING_PURE String* path) {
	UChar lineBuffer[String_BufferSize(128)];
	String line = String_InitAndClearArray(lineBuffer);
	String parts[7];
	

	/* ReadLine reads single byte at a time */
	UInt8 buffer[2048]; struct Stream buffered;
	Stream_ReadonlyBuffered(&buffered, stream, buffer, sizeof(buffer));
	ReturnCode res;

	for (;;) {
		res = Stream_ReadLine(&buffered, &line);
		if (res == ERR_END_OF_STREAM) break;
		if (res) { Chat_LogError(res, "reading from", path); break; }

		if (!line.length || line.buffer[0] == '#') continue;
		struct AnimationData data = { 0 };
		UInt8 tileX, tileY;

		Int32 partsCount = Array_Elems(parts);	
		String_UNSAFE_Split(&line, ' ', parts, &partsCount);

		if (partsCount < 7) {
			Chat_Add1("&cNot enough arguments for anim: %s", &line); continue;
		}		
		if (!Convert_TryParseUInt8(&parts[0], &tileX) || tileX >= ATLAS2D_TILES_PER_ROW) {
			Chat_Add1("&cInvalid anim tile X coord: %s", &line); continue;
		}
		if (!Convert_TryParseUInt8(&parts[1], &tileY) || tileY >= ATLAS2D_MAX_ROWS_COUNT) {
			Chat_Add1("&cInvalid anim tile Y coord: %s", &line); continue;
		}
		if (!Convert_TryParseUInt16(&parts[2], &data.FrameX)) {
			Chat_Add1("&cInvalid anim frame X coord: %s", &line); continue;
		}
		if (!Convert_TryParseUInt16(&parts[3], &data.FrameY)) {
			Chat_Add1("&cInvalid anim frame Y coord: %s", &line); continue;
		}
		if (!Convert_TryParseUInt16(&parts[4], &data.FrameSize)) {
			Chat_Add1("&cInvalid anim frame size: %s", &line); continue;
		}
		if (!Convert_TryParseUInt16(&parts[5], &data.StatesCount)) {
			Chat_Add1("&cInvalid anim states count: %s", &line); continue;
		}
		if (!Convert_TryParseInt16(&parts[6], &data.TickDelay)) {
			Chat_Add1("&cInvalid anim tick delay: %s", &line); continue;
		}

		if (anims_count == Array_Elems(anims_list)) {
			Chat_AddRaw("&cCannot show over 256 animations"); return;
		}

		data.TexLoc = tileX + (tileY * ATLAS2D_TILES_PER_ROW);
		anims_list[anims_count++] = data;
	}
}

/* TODO: should we use 128 size here? */
#define ANIMS_FAST_SIZE 64
static void Animations_Draw(struct AnimationData* data, TextureLoc texLoc, Int32 size) {
	UInt8 buffer[Bitmap_DataSize(ANIMS_FAST_SIZE, ANIMS_FAST_SIZE)];
	UInt8* ptr = buffer;
	if (size > ANIMS_FAST_SIZE) {
		/* cannot allocate memory on the stack for very big animation.png frames */
		ptr = Mem_Alloc(size * size, BITMAP_SIZEOF_PIXEL, "anim frame");
	}

	Int32 index_1D = Atlas1D_Index(texLoc);
	Int32 rowId_1D = Atlas1D_RowId(texLoc);
	struct Bitmap animPart; Bitmap_Create(&animPart, size, size, buffer);

	if (!data) {
		if (texLoc == 30) {
			LavaAnimation_Tick((UInt32*)animPart.Scan0, size);
		} else if (texLoc == 14) {
			WaterAnimation_Tick((UInt32*)animPart.Scan0, size);
		}
	} else {
		Int32 x = data->FrameX + data->State * size;
		Bitmap_CopyBlock(x, data->FrameY, 0, 0, &anims_bmp, &animPart, size);
	}

	GfxResourceID tex = Atlas1D_TexIds[index_1D];
	Int32 dstY = rowId_1D * Atlas2D_TileSize;
	if (tex) { Gfx_UpdateTexturePart(tex, 0, dstY, &animPart, Gfx_Mipmaps); }
	if (size > ANIMS_FAST_SIZE) Mem_Free(ptr);
}

static void Animations_Apply(struct AnimationData* data) {
	data->Tick--;
	if (data->Tick >= 0) return;
	data->State++;
	data->State %= data->StatesCount;
	data->Tick = data->TickDelay;

	TextureLoc texLoc = data->TexLoc;
	if (texLoc == 30 && anims_useLavaAnim) return;
	if (texLoc == 14 && anims_useWaterAnim) return;
	Animations_Draw(data, texLoc, data->FrameSize);
}

static bool Animations_IsDefaultZip(void) {
	if (World_TextureUrl.length) return false;
	UChar texPackBuffer[String_BufferSize(STRING_SIZE)];
	String texPack = String_InitAndClearArray(texPackBuffer);

	Options_Get(OPT_DEFAULT_TEX_PACK, &texPack, "default.zip");
	return String_CaselessEqualsConst(&texPack, "default.zip");
}

static void Animations_Clear(void) {
	Mem_Free(anims_bmp.Scan0);
	anims_count = 0;
	anims_bmp.Scan0 = NULL;
	anims_validated = false;
}

static void Animations_Validate(void) {
	anims_validated = true;
	UInt32 i, j;

	for (i = 0; i < anims_count; i++) {
		struct AnimationData data = anims_list[i];
		Int32 maxY = data.FrameY + data.FrameSize;
		Int32 maxX = data.FrameX + data.FrameSize * data.StatesCount;
		Int32 tileX = Atlas2D_TileX(data.TexLoc), tileY = Atlas2D_TileY(data.TexLoc);

		if (data.FrameSize > Atlas2D_TileSize || tileY >= Atlas2D_RowsCount) {
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


void Animations_Tick(struct ScheduledTask* task) {
	if (anims_useLavaAnim) {
		Int32 size = min(Atlas2D_TileSize, 64);
		Animations_Draw(NULL, 30, size);
	}
	if (anims_useWaterAnim) {
		Int32 size = min(Atlas2D_TileSize, 64);
		Animations_Draw(NULL, 14, size);
	}
	if (!anims_count) return;

	if (!anims_bmp.Scan0) {
		Chat_AddRaw("&cCurrent texture pack specifies it uses animations,");
		Chat_AddRaw("&cbut is missing animations.png");
		anims_count = 0;
		return;
	}

	/* deferred, because when reading animations.txt, might not have read animations.png yet */
	if (!anims_validated) Animations_Validate();
	UInt32 i;
	for (i = 0; i < anims_count; i++) {
		Animations_Apply(&anims_list[i]);
	}
}

static void Animations_PackChanged(void* obj) {
	Animations_Clear();
	anims_useLavaAnim = Animations_IsDefaultZip();
	anims_useWaterAnim = anims_useLavaAnim;
}

static void Animations_FileChanged(void* obj, struct Stream* stream, String* name) {
	if (String_CaselessEqualsConst(name, "animations.png")) {
		ReturnCode res = Bitmap_DecodePng(&anims_bmp, stream);
		if (!res) return;

		Chat_LogError(res, "decoding", name);
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
	Event_RegisterVoid(&TextureEvents_PackChanged,  NULL, Animations_PackChanged);
	Event_RegisterEntry(&TextureEvents_FileChanged, NULL, Animations_FileChanged);
}

static void Animations_Free(void) {
	Animations_Clear();
	Event_UnregisterVoid(&TextureEvents_PackChanged,  NULL, Animations_PackChanged);
	Event_UnregisterEntry(&TextureEvents_FileChanged, NULL, Animations_FileChanged);
}

void Animations_MakeComponent(struct IGameComponent* comp) {
	comp->Init = Animations_Init;
	comp->Free = Animations_Free;
}
