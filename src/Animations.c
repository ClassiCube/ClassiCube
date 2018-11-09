#include "Animations.h"
#include "ExtMath.h"
#include "TerrainAtlas.h"
#include "Platform.h"
#include "Event.h"
#include "Funcs.h"
#include "Graphics.h"
#include "Chat.h"
#include "World.h"
#include "Options.h"
#include "ErrorHandler.h"
#include "Errors.h"
#include "Stream.h"
#include "Bitmap.h"
#define LIQUID_ANIM_MAX 64

/*########################################################################################################################*
*-----------------------------------------------------Lava animation------------------------------------------------------*
*#########################################################################################################################*/
static float L_soupHeat[LIQUID_ANIM_MAX  * LIQUID_ANIM_MAX];
static float L_potHeat[LIQUID_ANIM_MAX   * LIQUID_ANIM_MAX];
static float L_flameHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
static RNGState L_rnd;
static bool L_rndInitalised;

static void LavaAnimation_Tick(uint32_t* ptr, int size) {
	int mask = size - 1, shift = Math_Log2(size);
	float soupHeat, potHeat, col;
	uint8_t r, g, b;
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

			r = (uint8_t)(col * 100.0f + 155.0f);
			g = (uint8_t)(col * col * 255.0f);
			b = (uint8_t)(col * col * col * col * 128.0f);
			*ptr = PackedCol_ARGB(r, g, b, 255);

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

static void WaterAnimation_Tick(uint32_t* ptr, int size) {
	int mask = size - 1, shift = Math_Log2(size);
	float soupHeat, col;
	uint8_t r, g, a;
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

			r = (uint8_t)(32.0f  + col * 32.0f);
			g = (uint8_t)(50.0f  + col * 64.0f);
			a = (uint8_t)(146.0f + col * 50.0f);
			*ptr = PackedCol_ARGB(r, g, 255, a);

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
			Chat_AddRaw("&cCannot show over 512 animations"); return;
		}

		data.TexLoc = tileX + (tileY * ATLAS2D_TILES_PER_ROW);
		anims_list[anims_count++] = data;
	}
}

#define ANIMS_FAST_SIZE 64
static void Animations_Draw(struct AnimationData* data, TextureLoc texLoc, int size) {
	int dstX = Atlas1D_Index(texLoc), srcX;
	int dstY = Atlas1D_RowId(texLoc) * Atlas2D_TileSize;
	GfxResourceID tex;

	uint8_t buffer[Bitmap_DataSize(ANIMS_FAST_SIZE, ANIMS_FAST_SIZE)];
	uint8_t* ptr = buffer;
	Bitmap frame;

	/* cannot allocate memory on the stack for very big animation.png frames */
	if (size > ANIMS_FAST_SIZE) {	
		ptr = Mem_Alloc(size * size, BITMAP_SIZEOF_PIXEL, "anim frame");
	}
	Bitmap_Create(&frame, size, size, ptr);

	if (!data) {
		if (texLoc == 30) {
			LavaAnimation_Tick((uint32_t*)frame.Scan0, size);
		} else if (texLoc == 14) {
			WaterAnimation_Tick((uint32_t*)frame.Scan0, size);
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
	String texPack; char texPackBuffer[STRING_SIZE];
	if (World_TextureUrl.length) return false;

	String_InitArray(texPack, texPackBuffer);
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
	int i, size;

	if (anims_useLavaAnim) {
		size = min(Atlas2D_TileSize, 64);
		Animations_Draw(NULL, 30, size);
	}
	if (anims_useWaterAnim) {
		size = min(Atlas2D_TileSize, 64);
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
