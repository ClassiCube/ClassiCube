#include "Animations.h"
#include "ExtMath.h"
#include "Random.h"
#include "TerrainAtlas.h"
#include "Platform.h"
#include "Event.h"
#include "Funcs.h"
#include "GraphicsAPI.h"
#include "Chat.h"
#include "World.h"
#define LIQUID_ANIM_MAX 64

Real32 L_soupHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
Real32 L_potHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
Real32 L_flameHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
Random L_rnd;
bool L_rndInitalised;

void LavaAnimation_Tick(UInt32* ptr, Int32 size) {
	if (!L_rndInitalised) {
		Random_InitFromCurrentTime(&L_rnd);
		L_rndInitalised = true;
	}
	Int32 mask = size - 1;
	Int32 shift = Math_Log2(size);

	Int32 x, y, i = 0;
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the colour at this coordinate in the heatmap */
			Int32 xx = x + (Int32)(1.2f * Math_Sin(y * 22.5f * MATH_DEG2RAD));
			Int32 yy = y + (Int32)(1.2f * Math_Sin(x * 22.5f * MATH_DEG2RAD));
			Real32 lSoupHeat =
				L_soupHeat[((yy - 1) & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[((yy - 1) & mask) << shift | (xx & mask)] +
				L_soupHeat[((yy - 1) & mask) << shift | ((xx + 1) & mask)] +

				L_soupHeat[(yy & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[(yy & mask) << shift | (xx & mask)] +
				L_soupHeat[(yy & mask) << shift | ((xx + 1) & mask)] +

				L_soupHeat[((yy + 1) & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[((yy + 1) & mask) << shift | (xx & mask)] +
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

			if (Random_Float(&L_rnd) <= 0.005f)
				L_flameHeat[i] = 1.5f * 0.01f;

			/* Output the pixel */
			Real32 col = 2.0f * L_soupHeat[i];
			col = col < 0.0f ? 0.0f : col;
			col = col > 1.0f ? 1.0f : col;

			Real32 r = col * 100.0f + 155.0f;
			Real32 g = col * col * 255.0f;
			Real32 b = col * col * col * col * 128.0f;
			*ptr = 0xFF000000UL | ((UInt8)r << 16) | ((UInt8)g << 8) | (UInt8)b;

			ptr++; i++;
		}
	}
}


Real32 W_soupHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
Real32 W_potHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
Real32 W_flameHeat[LIQUID_ANIM_MAX * LIQUID_ANIM_MAX];
Random W_rnd;
bool W_rndInitalised;

void WaterAnimation_Tick(UInt32* ptr, Int32 size) {
	if (!W_rndInitalised) {
		Random_InitFromCurrentTime(&W_rnd);
		W_rndInitalised = true;
	}
	Int32 mask = size - 1;
	Int32 shift = Math_Log2(size);

	Int32 x, y, i = 0;
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the colour at this coordinate in the heatmap */
			Real32 lSoupHeat =
				W_soupHeat[y << shift | ((x - 1) & mask)] +
				W_soupHeat[y << shift | x] +
				W_soupHeat[y << shift | ((x + 1) & mask)];

			W_soupHeat[i] = lSoupHeat / 3.3f + W_potHeat[i] * 0.8f;
			W_potHeat[i] += W_flameHeat[i] * 0.05f;
			if (W_potHeat[i] < 0.0f) W_potHeat[i] = 0.0f;
			W_flameHeat[i] -= 0.1f;

			if (Random_Float(&W_rnd) <= 0.05f)
				W_flameHeat[i] = 0.5f;

			/* Output the pixel */
			Real32 col = W_soupHeat[i];
			col = col < 0 ? 0 : col;
			col = col > 1 ? 1 : col;
			col = col * col;

			Real32 r = 32.0f + col * 32.0f;
			Real32 g = 50.0f + col * 64.0f;
			Real32 a = 146.0f + col * 50.0f;

			*ptr = ((UInt8)a << 24) | ((UInt8)r << 16) | ((UInt8)g << 8) | 0xFFUL;

			ptr++; i++;
		}
	}
}


typedef struct AnimationData_ {
	TextureLoc TileX, TileY; /* Tile (not pixel) coordinates in terrain.png */
	UInt16 FrameX, FrameY;   /* Top left pixel coordinates of start frame in animatons.png */
	UInt16 FrameSize;        /* Size of each frame in pixel coordinates */
	UInt16 State;            /* Current animation frame index */
	UInt16 StatesCount;      /* Total number of animation frames */
	Int16 Tick, TickDelay;
} AnimationData;

Bitmap anims_bmp;
AnimationData anims_list[ATLAS1D_MAX_ATLASES_COUNT];
UInt32 anims_count;
bool anims_validated, anims_useLavaAnim, anims_useWaterAnim;

void Animations_LogFail(STRING_TRANSIENT String* line, const UInt8* raw) {
	UInt8 msgBuffer[String_BufferSize(128)];
	String msg = String_InitAndClearArray(msgBuffer);

	String_AppendConst(&msg, raw);
	String_Append(&msg, ':'); String_Append(&msg, ' ');
	String_AppendString(&msg, line);
	Chat_Add(&msg);
}

void Animations_ReadDescription(Stream* stream) {
	UInt8 lineBuffer[String_BufferSize(128)];
	String line = String_InitAndClearArray(lineBuffer);
	String parts[7];

	while (Stream_ReadLine(stream, &line)) {
		if (line.length == 0 || line.buffer[0] == '#') continue;
		AnimationData data;
		UInt32 partsCount = Array_NumElements(parts);	
		String_UNSAFE_Split(&line, ' ', parts, &partsCount);

		if (partsCount < 7) {
			Animations_LogFail(&line, "Not enough arguments for anim"); continue;
		}		
		if (!Convert_TryParseUInt8(&parts[0], &data.TileX) || data.TileX >= 16) {
			Animations_LogFail(&line, "Invalid anim tile X coord"); continue;
		}
		if (!Convert_TryParseUInt8(&parts[1], &data.TileY) || data.TileY >= 16) {
			Animations_LogFail(&line, "Invalid anim tile Y coord"); continue;
		}
		if (!Convert_TryParseUInt16(&parts[2], &data.FrameX)) {
			Animations_LogFail(&line, "Invalid anim frame X coord"); continue;
		}
		if (!Convert_TryParseUInt16(&parts[3], &data.FrameY)) {
			Animations_LogFail(&line, "Invalid anim frame Y coord"); continue;
		}
		if (!Convert_TryParseUInt16(&parts[4], &data.FrameSize)) {
			Animations_LogFail(&line, "Invalid anim frame size"); continue;
		}
		if (!Convert_TryParseUInt16(&parts[5], &data.StatesCount)) {
			Animations_LogFail(&line, "Invalid anim states count"); continue;
		}
		if (!Convert_TryParseInt16(&parts[6], &data.TickDelay)) {
			Animations_LogFail(&line, "Invalid anim tick delay"); continue;
		}

		if (anims_count == Array_NumElements(anims_list)) {
			ErrorHandler_Fail("Too many animations in animations.txt");
		}
		anims_list[anims_count++] = data;
	}
}

/* TODO: should we use 128 size here? */
#define ANIMS_FAST_SIZE 64
void Animations_Draw(AnimationData* data, Int32 texId, Int32 size) {
	UInt8 buffer[Bitmap_DataSize(ANIMS_FAST_SIZE, ANIMS_FAST_SIZE)];
	UInt8* ptr = buffer;
	if (size > ANIMS_FAST_SIZE) {
		/* cannot allocate memory on the stack for very big animation.png frames */
		ptr = Platform_MemAlloc(Bitmap_DataSize(size, size));
		if (ptr == NULL) ErrorHandler_Fail("Failed to allocate memory for anim frame");
	}

	Int32 index = Atlas1D_Index(texId);
	Int32 rowNum = Atlas1D_RowId(texId);
	Bitmap animPart; Bitmap_Create(&animPart, size, size, buffer);

	if (data == NULL) {
		if (texId == 30) {
			LavaAnimation_Tick((UInt32*)animPart.Scan0, size);
		} else if (texId == 14) {
			WaterAnimation_Tick((UInt32*)animPart.Scan0, size);
		}
	} else {
		Int32 x = data->FrameX + data->State * size;
		Bitmap_CopyBlock(x, data->FrameY, 0, 0, &anims_bmp, &animPart, size);
	}

	Int32 y = rowNum * Atlas2D_ElementSize;
	Gfx_UpdateTexturePart(Atlas1D_TexIds[index], 0, y, &animPart, Gfx_Mipmaps);
	if (size > ANIMS_FAST_SIZE) Platform_MemFree(ptr);
}

void Animations_Apply(AnimationData* data) {
	data->Tick--;
	if (data->Tick >= 0) return;
	data->State++;
	data->State %= data->StatesCount;
	data->Tick = data->TickDelay;

	Int32 texId = (data->TileY << 4) | data->TileX;
	if (texId == 30 && anims_useLavaAnim) return;
	if (texId == 14 && anims_useWaterAnim) return;
	Animations_Draw(data, texId, data->FrameSize);
}

bool Animations_IsDefaultZip(void) {
	if (World_TextureUrl.length > 0) return false;
	UInt8 texPackBuffer[String_BufferSize(STRING_SIZE)];
	String texPack = String_InitAndClearArray(texPackBuffer);

	Options_Get(OPTION_DEFAULT_TEX_PACK, &texPack);
	String defaultZip = String_FromConst("default.zip");
	return texPack.length == 0 || String_CaselessEquals(&texPack, &defaultZip);
}

void Animations_Clear(void) {
	anims_count = 0;
	if (anims_bmp.Scan0 == NULL) return;
	Platform_MemFree(anims_bmp.Scan0);
	anims_bmp.Scan0 = NULL;
	anims_validated = false;
}

void Animations_Validate(void) {
	anims_validated = true;
	UInt8 msgBuffer[String_BufferSize(128)];
	String msg = String_InitAndClearArray(msgBuffer);
	UInt32 i, j;

	for (i = 0; i < anims_count; i++) {
		AnimationData data = anims_list[i];
		Int32 maxY = data.FrameY + data.FrameSize;
		Int32 maxX = data.FrameX + data.FrameSize * data.StatesCount;
		String_Clear(&msg);

		if (data.FrameSize > Atlas2D_ElementSize) {		
			String_AppendConst(&msg, "&cAnimation frames for tile (");
			String_AppendInt32(&msg, data.TileX); String_AppendConst(&msg, ", "); String_AppendInt32(&msg, data.TileY);
			String_AppendConst(&msg, ") are bigger than the size of a tile in terrain.png");
		} else if (maxX > anims_bmp.Width || maxY > anims_bmp.Height) {
			String_AppendConst(&msg, "&cSome of the animation frames for tile (");
			String_AppendInt32(&msg, data.TileX); String_AppendConst(&msg, ", "); String_AppendInt32(&msg, data.TileY);
			String_AppendConst(&msg, ") are at coordinates outside animations.png");
		} else {
			continue;
		}

		/* Remove this animation from the list */
		for (j = i; j < anims_count - 1; j++) {
			anims_list[j] = anims_list[j + 1];
		}
		i--; anims_count--;
		Chat_Add(&msg);
	}
}


void Animations_Tick(ScheduledTask* task) {
	if (anims_useLavaAnim) {
		Int32 size = min(Atlas2D_ElementSize, 64);
		Animations_Draw(NULL, 30, size);
	}
	if (anims_useWaterAnim) {
		Int32 size = min(Atlas2D_ElementSize, 64);
		Animations_Draw(NULL, 14, size);
	}
	if (anims_count == 0) return;

	if (anims_bmp.Scan0 == NULL) {
		String w1 = String_FromConst("&cCurrent texture pack specifies it uses animations,"); Chat_Add(&w1);
		String w2 = String_FromConst("&cbut is missing animations.png");                      Chat_Add(&w2);
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

void Animations_PackChanged(void* obj) {
	Animations_Clear();
	anims_useLavaAnim = Animations_IsDefaultZip();
	anims_useWaterAnim = anims_useLavaAnim;
}

void Animations_FileChanged(void* obj, Stream* stream) {
	String animPng = String_FromConst("animation.png");
	String animsPng = String_FromConst("animations.png");
	String animTxt = String_FromConst("animation.txt");
	String animsTxt = String_FromConst("animations.txt");
	String lavaAnim = String_FromConst("uselavaanim");
	String waterAnim = String_FromConst("usewateranim");

	String* name = &stream->Name;
	if (String_CaselessEquals(name, &animPng) || String_CaselessEquals(name, &animsPng)) {
		Bitmap_DecodePng(&anims_bmp, stream);
	} else if (String_CaselessEquals(name, &animTxt) || String_CaselessEquals(name, &animsTxt)) {
		Animations_ReadDescription(stream);
	} else if (String_CaselessEquals(name, &lavaAnim)) {
		anims_useLavaAnim = true;
	} else if (String_CaselessEquals(name, &waterAnim)) {
		anims_useWaterAnim = true;
	}
}

void Animations_Init(void) {
	Event_RegisterVoid(&TextureEvents_PackChanged,   NULL, Animations_PackChanged);
	Event_RegisterStream(&TextureEvents_FileChanged, NULL, Animations_FileChanged);
}

void Animations_Free(void) {
	Animations_Clear();
	Event_UnregisterVoid(&TextureEvents_PackChanged,   NULL, Animations_PackChanged);
	Event_UnregisterStream(&TextureEvents_FileChanged, NULL, Animations_FileChanged);
}

IGameComponent Animations_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = Animations_Init;
	comp.Free = Animations_Free;
	return comp;
}