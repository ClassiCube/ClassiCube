#include "TexturePack.h"
#include "String.h"
#include "Constants.h"
#include "Stream.h"
#include "Graphics.h"
#include "Event.h"
#include "Game.h"
#include "Funcs.h"
#include "Errors.h"
#include "Chat.h"
#include "ExtMath.h"
#include "Options.h"
#include "Logger.h"

#define LIQUID_ANIM_MAX 64
#define WATER_TEX_LOC 14
#define LAVA_TEX_LOC  30
static void Animations_Update(int loc, struct Bitmap* bmp, int stride);

#ifndef CC_BUILD_WEB
/* Based off the incredible work from https://dl.dropboxusercontent.com/u/12694594/lava.txt
	mirrored at https://github.com/UnknownShadow200/ClassiCube/wiki/Minecraft-Classic-lava-animation-algorithm
	Water animation originally written by cybertoon, big thanks!
*/
/*########################################################################################################################*
*-----------------------------------------------------Lava animation------------------------------------------------------*
*#########################################################################################################################*/
static float L_soupHeat[LIQUID_ANIM_MAX  * LIQUID_ANIM_MAX];
static float L_potHeat[LIQUID_ANIM_MAX   * LIQUID_ANIM_MAX];
static float L_flameHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
static RNGState L_rnd;
static cc_bool  L_rndInited;

static void LavaAnimation_Tick(void) {
	BitmapCol pixels[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
	BitmapCol* ptr = pixels;
	float soupHeat, potHeat, color;
	int size, mask, shift;
	int x, y, i = 0;
	struct Bitmap bmp;

	size  = min(Atlas2D.TileSize, LIQUID_ANIM_MAX);
	mask  = size - 1;
	shift = Math_Log2(size);

	if (!L_rndInited) {
		Random_SeedFromCurrentTime(&L_rnd);
		L_rndInited = true;
	}
	
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the color at this coordinate in the heatmap */

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
			color = 2.0f * L_soupHeat[i];
			Math_Clamp(color, 0.0f, 1.0f);

			*ptr = BitmapCol_Make(
				color * 100.0f + 155.0f,
				color * color * 255.0f,
				color * color * color * color * 128.0f,
				255);

			ptr++; i++;
		}
	}

	Bitmap_Init(bmp, size, size, pixels);
	Animations_Update(LAVA_TEX_LOC, &bmp, size);
}


/*########################################################################################################################*
*----------------------------------------------------Water animation------------------------------------------------------*
*#########################################################################################################################*/
static float W_soupHeat[LIQUID_ANIM_MAX  * LIQUID_ANIM_MAX];
static float W_potHeat[LIQUID_ANIM_MAX   * LIQUID_ANIM_MAX];
static float W_flameHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
static RNGState W_rnd;
static cc_bool  W_rndInited;

static void WaterAnimation_Tick(void) {
	BitmapCol pixels[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
	BitmapCol* ptr = pixels;
	float soupHeat, color;
	int size, mask, shift;
	int x, y, i = 0;
	struct Bitmap bmp;

	size  = min(Atlas2D.TileSize, LIQUID_ANIM_MAX);
	mask  = size - 1;
	shift = Math_Log2(size);

	if (!W_rndInited) {
		Random_SeedFromCurrentTime(&W_rnd);
		W_rndInited = true;
	}
	
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the color at this coordinate in the heatmap */
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
			color = W_soupHeat[i];
			Math_Clamp(color, 0.0f, 1.0f);
			color = color * color;

			*ptr = BitmapCol_Make(
				32.0f  + color * 32.0f,
				50.0f  + color * 64.0f,
				255,
				146.0f + color * 50.0f);

			ptr++; i++;
		}
	}

	Bitmap_Init(bmp, size, size, pixels);
	Animations_Update(WATER_TEX_LOC, &bmp, size);
}
#endif


/*########################################################################################################################*
*-------------------------------------------------------Animations--------------------------------------------------------*
*#########################################################################################################################*/
struct AnimationData {
	TextureLoc texLoc;       /* Tile (not pixel) coordinates in terrain.png */
	cc_uint16 frameX, frameY; /* Top left pixel coordinates of start frame in animations.png */
	cc_uint16 frameSize;      /* Size of each frame in pixel coordinates */
	cc_uint16 state;          /* Current animation frame index */
	cc_uint16 statesCount;    /* Total number of animation frames */
	cc_uint16 delay;          /* Delay in ticks until next frame is drawn */
	cc_uint16 frameDelay;     /* Delay between each frame */
};

static struct Bitmap anims_bmp;
static struct AnimationData anims_list[ATLAS1D_MAX_ATLASES];
static int anims_count;
static cc_bool anims_validated, useLavaAnim, useWaterAnim, alwaysLavaAnim, alwaysWaterAnim;
#define ANIM_MIN_ARGS 7

static void Animations_ReadDescription(struct Stream* stream, const cc_string* path) {
	cc_string line; char lineBuffer[STRING_SIZE * 2];
	cc_string parts[ANIM_MIN_ARGS];
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
		if (res) { Logger_SysWarn2(res, "reading from", path); break; }

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

static void Animations_Update(int texLoc, struct Bitmap* bmp, int stride) {
	int dstX = Atlas1D_Index(texLoc);
	int dstY = Atlas1D_RowId(texLoc) * Atlas2D.TileSize;
	GfxResourceID tex;

	tex = Atlas1D.TexIds[dstX];
	if (tex) Gfx_UpdateTexture(tex, 0, dstY, bmp, stride, Gfx.Mipmaps);
}

static void Animations_Apply(struct AnimationData* data) {
	struct Bitmap frame;
	int loc, size;
	if (data->delay) { data->delay--; return; }

	data->state++;
	data->state %= data->statesCount;
	data->delay  = data->frameDelay;

	loc = data->texLoc;
#ifndef CC_BUILD_WEB
	if (loc == LAVA_TEX_LOC  && useLavaAnim)  return;
	if (loc == WATER_TEX_LOC && useWaterAnim) return;
#endif

	size = data->frameSize;
	Bitmap_Init(frame, size, size, NULL);

	frame.scan0 = anims_bmp.scan0 
				+ data->frameY * anims_bmp.width
				+ (data->frameX + data->state * size);
	Animations_Update(loc, &frame, anims_bmp.width);
}

static cc_bool Animations_IsDefaultZip(void) {
	cc_string texPack;
	cc_bool optExists;
	if (TexturePack_Url.length) return false;

	optExists = Options_UNSAFE_Get(OPT_DEFAULT_TEX_PACK, &texPack);
	return !optExists || String_CaselessEqualsConst(&texPack, "default.zip");
}

static void Animations_Clear(void) {
	Mem_Free(anims_bmp.scan0);
	anims_count = 0;
	anims_bmp.scan0 = NULL;
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
		} else if (maxX > anims_bmp.width || maxY > anims_bmp.height) {
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
	int i;
#ifndef CC_BUILD_WEB
	if (useLavaAnim)  LavaAnimation_Tick();
	if (useWaterAnim) WaterAnimation_Tick();
#endif

	if (!anims_count) return;
	if (!anims_bmp.scan0) {
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
static void AnimationsPngProcess(struct Stream* stream, const cc_string* name) {
	cc_result res = Png_Decode(&anims_bmp, stream);
	if (!res) return;

	Logger_SysWarn2(res, "decoding", name);
	Mem_Free(anims_bmp.scan0);
	anims_bmp.scan0 = NULL;
}
static struct TextureEntry animations_entry = { "animations.png", AnimationsPngProcess };
static struct TextureEntry animations_txt   = { "animations.txt", Animations_ReadDescription };

static void UseWaterProcess(struct Stream* stream, const cc_string* name) {
	useWaterAnim    = true;
	alwaysWaterAnim = true;
}
static struct TextureEntry water_entry = { "usewateranim", UseWaterProcess };

static void UseLavaProcess(struct Stream* stream, const cc_string* name) {
	useLavaAnim    = true;
	alwaysLavaAnim = true;
}
static struct TextureEntry lava_entry = { "uselavaanim", UseLavaProcess };


static void OnPackChanged(void* obj) {
	Animations_Clear();
	useLavaAnim     = Animations_IsDefaultZip();
	useWaterAnim    = useLavaAnim;
	alwaysLavaAnim  = false;
	alwaysWaterAnim = false;
}
static void OnInit(void) {
	TextureEntry_Register(&animations_entry);
	TextureEntry_Register(&animations_txt);
	TextureEntry_Register(&water_entry);
	TextureEntry_Register(&lava_entry);

	ScheduledTask_Add(GAME_DEF_TICKS, Animations_Tick);
	Event_Register_(&TextureEvents.PackChanged, NULL, OnPackChanged);
}

struct IGameComponent Animations_Component = {
	OnInit,            /* Init  */
	Animations_Clear  /* Free  */
};
