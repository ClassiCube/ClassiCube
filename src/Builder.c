#include "Builder.h"
#include "Constants.h"
#include "World.h"
#include "Funcs.h"
#include "Lighting.h"
#include "Platform.h"
#include "MapRenderer.h"
#include "Graphics.h"
#include "ErrorHandler.h"
#include "Drawer.h"
#include "ExtMath.h"
#include "ChunkUpdater.h"
#include "BlockID.h"
#include "Block.h"
#include "PackedCol.h"
#include "TerrainAtlas.h"
#include "VertexStructs.h"

/* Packs an index into the 16x16x16 count array. Coordinates range from 0 to 15. */
#define Builder_PackCount(xx, yy, zz) ((((yy) << 8) | ((zz) << 4) | (xx)) * FACE_COUNT)
/* Packs an index into the 18x18x18 chunk array. Coordinates range from -1 to 16. */
#define Builder_PackChunk(xx, yy, zz) (((yy) + 1) * EXTCHUNK_SIZE_2 + ((zz) + 1) * EXTCHUNK_SIZE + ((xx) + 1))

BlockID* Builder_Chunk;
uint8_t* Builder_Counts;
int* Builder_BitFlags;
bool Builder_UseBitFlags;
int Builder_X, Builder_Y, Builder_Z;
BlockID Builder_Block;
int Builder_ChunkIndex;
bool Builder_FullBright;
bool Builder_Tinted;
int Builder_ChunkEndX, Builder_ChunkEndZ;
int Builder_Offsets[FACE_COUNT] = { -1,1, -EXTCHUNK_SIZE,EXTCHUNK_SIZE, -EXTCHUNK_SIZE_2,EXTCHUNK_SIZE_2 };

int (*Builder_StretchXLiquid)(int countIndex, int x, int y, int z, int chunkIndex, BlockID block);
int (*Builder_StretchX)(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face);
int (*Builder_StretchZ)(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face);
void (*Builder_RenderBlock)(int countsIndex);
void (*Builder_PreStretchTiles)(int x1, int y1, int z1);
void (*Builder_PostStretchTiles)(int x1, int y1, int z1);

/* Contains state for vertices for a portion of a chunk mesh (vertices that are in a 1D atlas) */
struct Builder1DPart {
	VertexP3fT2fC4b* fVertices[FACE_COUNT];
	int fCount[FACE_COUNT];
	int sCount, sOffset, sAdvance;
};

/* Part builder data, for both normal and translucent parts.
The first ATLAS1D_MAX_ATLASES parts are for normal parts, remainder are for translucent parts. */
struct Builder1DPart Builder_Parts[ATLAS1D_MAX_ATLASES * 2];
VertexP3fT2fC4b* Builder_Vertices;
int Builder_VerticesElems;

static int Builder1DPart_VerticesCount(struct Builder1DPart* part) {
	int i, count = part->sCount;
	for (i = 0; i < FACE_COUNT; i++) { count += part->fCount[i]; }
	return count;
}

static void Builder1DPart_CalcOffsets(struct Builder1DPart* part, int* offset) {
	int pos = *offset, i;
	part->sOffset = pos;
	part->sAdvance = part->sCount >> 2;

	pos += part->sCount;
	for (i = 0; i < FACE_COUNT; i++) {
		part->fVertices[i] = &Builder_Vertices[pos];
		pos += part->fCount[i];
	}
	*offset = pos;
}

static int Builder_TotalVerticesCount(void) {
	int i, count = 0;
	for (i = 0; i < ATLAS1D_MAX_ATLASES * 2; i++) {
		count += Builder1DPart_VerticesCount(&Builder_Parts[i]);
	}
	return count;
}


/*########################################################################################################################*
*----------------------------------------------------Base mesh builder----------------------------------------------------*
*#########################################################################################################################*/
static void Builder_AddSpriteVertices(BlockID block) {
	int i = Atlas1D_Index(Block_GetTex(block, FACE_XMIN));
	struct Builder1DPart* part = &Builder_Parts[i];
	part->sCount += 4 * 4;
}

static void Builder_AddVertices(BlockID block, Face face) {
	int baseOffset = (Block_Draw[block] == DRAW_TRANSLUCENT) * ATLAS1D_MAX_ATLASES;
	int i = Atlas1D_Index(Block_GetTex(block, face));
	struct Builder1DPart* part = &Builder_Parts[baseOffset + i];
	part->fCount[face] += 4;
}

static void Builder_SetPartInfo(struct Builder1DPart* part, int* offset, struct ChunkPartInfo* info, bool* hasParts) {
	int vCount = Builder1DPart_VerticesCount(part);
	info->Offset = -1;
	if (!vCount) return;

	info->Offset = *offset;
	*offset += vCount;
	*hasParts = true;

#ifdef CC_BUILD_GL11
	info->Vb = Gfx_CreateVb(&Builder_Vertices[info->Offset], VERTEX_FORMAT_P3FT2FC4B, vCount);
#endif

	info->Counts[FACE_XMIN] = part->fCount[FACE_XMIN];
	info->Counts[FACE_XMAX] = part->fCount[FACE_XMAX];
	info->Counts[FACE_ZMIN] = part->fCount[FACE_ZMIN];
	info->Counts[FACE_ZMAX] = part->fCount[FACE_ZMAX];
	info->Counts[FACE_YMIN] = part->fCount[FACE_YMIN];
	info->Counts[FACE_YMAX] = part->fCount[FACE_YMAX];
	info->SpriteCount       = part->sCount;
}


static void Builder_Stretch(int x1, int y1, int z1) {
	int xMax = min(World_Width,  x1 + CHUNK_SIZE);
	int yMax = min(World_Height, y1 + CHUNK_SIZE);
	int zMax = min(World_Length, z1 + CHUNK_SIZE);

	int cIndex, index, tileIdx, count;
	BlockID b;
	int x, y, z, xx, yy, zz;

#ifdef OCCLUSION
	int flags = ComputeOcclusion();
#endif
#ifdef DEBUG_OCCLUSION
	FastColour col = new FastColour(60, 60, 60, 255);
	if (flags & 1) col.R = 255; /* x */
	if (flags & 4) col.G = 255; /* y */
	if (flags & 2) col.B = 255; /* z */
	map.Sunlight = map.Shadowlight = col;
	map.SunlightXSide = map.ShadowlightXSide = col;
	map.SunlightZSide = map.ShadowlightZSide = col;
	map.SunlightYBottom = map.ShadowlightYBottom = col;
#endif
	
	for (y = y1, yy = 0; y < yMax; y++, yy++) {
		for (z = z1, zz = 0; z < zMax; z++, zz++) {
			cIndex = Builder_PackChunk(0, yy, zz);

			for (x = x1, xx = 0; x < xMax; x++, xx++, cIndex++) {
				b = Builder_Chunk[cIndex];
				if (Block_Draw[b] == DRAW_GAS) continue;
				index = Builder_PackCount(xx, yy, zz);

				/* Sprites only use one face to indicate stretching count, so we can take a shortcut here.
				Note that sprites are not drawn with any of the DrawXFace, they are drawn using DrawSprite. */
				if (Block_Draw[b] == DRAW_SPRITE) {
					index += FACE_YMAX;
					if (Builder_Counts[index]) {
						Builder_X = x; Builder_Y = y; Builder_Z = z;
						Builder_AddSpriteVertices(b);
						Builder_Counts[index] = 1;
					}
					continue;
				}

				Builder_X = x; Builder_Y = y; Builder_Z = z;
				Builder_FullBright = Block_FullBright[b];
				tileIdx = b * BLOCK_COUNT;
				/* All of these function calls are inlined as they can be called tens of millions to hundreds of millions of times. */

				if (Builder_Counts[index] == 0 ||
					(x == 0 && (y < Builder_SidesLevel || (b >= BLOCK_WATER && b <= BLOCK_STILL_LAVA && y < Builder_EdgeLevel))) ||
					(x != 0 && (Block_Hidden[tileIdx + Builder_Chunk[cIndex - 1]] & (1 << FACE_XMIN)) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					count = Builder_StretchZ(index, x, y, z, cIndex, b, FACE_XMIN);
					Builder_AddVertices(b, FACE_XMIN);
					Builder_Counts[index] = count;
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(x == World_MaxX && (y < Builder_SidesLevel || (b >= BLOCK_WATER && b <= BLOCK_STILL_LAVA && y < Builder_EdgeLevel))) ||
					(x != World_MaxX && (Block_Hidden[tileIdx + Builder_Chunk[cIndex + 1]] & (1 << FACE_XMAX)) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					count = Builder_StretchZ(index, x, y, z, cIndex, b, FACE_XMAX);
					Builder_AddVertices(b, FACE_XMAX);
					Builder_Counts[index] = count;
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(z == 0 && (y < Builder_SidesLevel || (b >= BLOCK_WATER && b <= BLOCK_STILL_LAVA && y < Builder_EdgeLevel))) ||
					(z != 0 && (Block_Hidden[tileIdx + Builder_Chunk[cIndex - EXTCHUNK_SIZE]] & (1 << FACE_ZMIN)) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					count = Builder_StretchX(index, Builder_X, Builder_Y, Builder_Z, cIndex, b, FACE_ZMIN);
					Builder_AddVertices(b, FACE_ZMIN);
					Builder_Counts[index] = count;
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(z == World_MaxZ && (y < Builder_SidesLevel || (b >= BLOCK_WATER && b <= BLOCK_STILL_LAVA && y < Builder_EdgeLevel))) ||
					(z != World_MaxZ && (Block_Hidden[tileIdx + Builder_Chunk[cIndex + EXTCHUNK_SIZE]] & (1 << FACE_ZMAX)) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					count = Builder_StretchX(index, x, y, z, cIndex, b, FACE_ZMAX);
					Builder_AddVertices(b, FACE_ZMAX);
					Builder_Counts[index] = count;
				}

				index++;
				if (Builder_Counts[index] == 0 || y == 0 ||
					(Block_Hidden[tileIdx + Builder_Chunk[cIndex - EXTCHUNK_SIZE_2]] & (1 << FACE_YMIN)) != 0) {
					Builder_Counts[index] = 0;
				} else {
					count = Builder_StretchX(index, x, y, z, cIndex, b, FACE_YMIN);
					Builder_AddVertices(b, FACE_YMIN);
					Builder_Counts[index] = count;
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(Block_Hidden[tileIdx + Builder_Chunk[cIndex + EXTCHUNK_SIZE_2]] & (1 << FACE_YMAX)) != 0) {
					Builder_Counts[index] = 0;
				} else if (b < BLOCK_WATER || b > BLOCK_STILL_LAVA) {
					count = Builder_StretchX(index, x, y, z, cIndex, b, FACE_YMAX);
					Builder_AddVertices(b, FACE_YMAX);
					Builder_Counts[index] = count;
				} else {
					count = Builder_StretchXLiquid(index, x, y, z, cIndex, b);
					if (count > 0) Builder_AddVertices(b, FACE_YMAX);
					Builder_Counts[index] = count;
				}
			}
		}
	}
}

#define Builder_ReadChunkBody(get_block)\
for (yy = -1; yy < 17; ++yy) {\
	y = yy + y1;\
	if (y < 0) continue;\
	if (y >= World_Height) break;\
\
	for (zz = -1; zz < 17; ++zz) {\
		z = zz + z1;\
		if (z < 0) continue;\
		if (z >= World_Length) break;\
\
		index  = World_Pack(x1 - 1, y, z);\
		cIndex = Builder_PackChunk(-1, yy, zz);\
\
		for (xx = -1; xx < 17; ++xx, ++index, ++cIndex) {\
			x = xx + x1;\
			if (x < 0) continue;\
			if (x >= World_Width) break;\
\
			block    = get_block;\
			allAir   = allAir   && Block_Draw[block] == DRAW_GAS;\
			allSolid = allSolid && Block_FullOpaque[block];\
			Builder_Chunk[cIndex] = block;\
		}\
	}\
}

static void Builder_ReadChunkData(int x1, int y1, int z1, bool* outAllAir, bool* outAllSolid) {
	bool allAir = true, allSolid = true;
	int index, cIndex;
	BlockID block;
	int xx, yy, zz, x, y, z;

#ifndef EXTENDED_BLOCKS
	Builder_ReadChunkBody(World_Blocks[index]);
#else
	if (Block_UsedCount <= 256) {
		Builder_ReadChunkBody(World_Blocks[index]);
	} else {
		Builder_ReadChunkBody(World_Blocks[index] | (World_Blocks2[index] << 8));
	}
#endif

	*outAllAir   = allAir;
	*outAllSolid = allSolid;
}

static bool Builder_BuildChunk(int x1, int y1, int z1, bool* allAir) {
	BlockID chunk[EXTCHUNK_SIZE_3]; 
	uint8_t counts[CHUNK_SIZE_3 * FACE_COUNT]; 
	int bitFlags[EXTCHUNK_SIZE_3];

	bool allSolid;
	int xMax, yMax, zMax;
	int cIndex, index;
	int x, y, z, xx, yy, zz;

	Builder_Chunk  = chunk;
	Builder_Counts = counts;
	Builder_BitFlags = bitFlags;
	Builder_PreStretchTiles(x1, y1, z1);

	Mem_Set(chunk, BLOCK_AIR, EXTCHUNK_SIZE_3 * sizeof(BlockID));
	Builder_ReadChunkData(x1, y1, z1, allAir, &allSolid);

	if (x1 == 0 || y1 == 0 || z1 == 0 || x1 + CHUNK_SIZE >= World_Width ||
		y1 + CHUNK_SIZE >= World_Height || z1 + CHUNK_SIZE >= World_Length) allSolid = false;

	if (*allAir || allSolid) return false;
	Lighting_LightHint(x1 - 1, z1 - 1);

	Mem_Set(counts, 1, CHUNK_SIZE_3 * FACE_COUNT);
	xMax = min(World_Width,  x1 + CHUNK_SIZE);
	yMax = min(World_Height, y1 + CHUNK_SIZE);
	zMax = min(World_Length, z1 + CHUNK_SIZE);

	Builder_ChunkEndX = xMax; Builder_ChunkEndZ = zMax;
	Builder_Stretch(x1, y1, z1);
	Builder_PostStretchTiles(x1, y1, z1);

	for (y = y1, yy = 0; y < yMax; y++, yy++) {
		for (z = z1, zz = 0; z < zMax; z++, zz++) {
			cIndex = Builder_PackChunk(0, yy, zz);

			for (x = x1, xx = 0; x < xMax; x++, xx++, cIndex++) {
				Builder_Block = chunk[cIndex];
				if (Block_Draw[Builder_Block] == DRAW_GAS) continue;

				index = Builder_PackCount(xx, yy, zz);
				Builder_X = x; Builder_Y = y; Builder_Z = z;
				Builder_ChunkIndex = cIndex;
				Builder_RenderBlock(index);
			}
		}
	}
	return true;
}

void Builder_MakeChunk(struct ChunkInfo* info) {
	int x = info->CentreX - 8, y = info->CentreY - 8, z = info->CentreZ - 8;
	bool allAir = false, hasMesh;
	int totalVerts;

	hasMesh = Builder_BuildChunk(x, y, z, &allAir);
	info->AllAir = allAir;
	if (!hasMesh) return;

	totalVerts = Builder_TotalVerticesCount();
	if (!totalVerts) return;
#ifndef CC_BUILD_GL11
	/* add an extra element to fix crashing on some GPUs */
	info->Vb = Gfx_CreateVb(Builder_Vertices, VERTEX_FORMAT_P3FT2FC4B, totalVerts + 1);
#endif

	int i, offset = 0, partsIndex = MapRenderer_Pack(x >> CHUNK_SHIFT, y >> CHUNK_SHIFT, z >> CHUNK_SHIFT);
	bool hasNormal = false, hasTranslucent = false;

	for (i = 0; i < MapRenderer_1DUsedCount; i++) {
		int j = i + ATLAS1D_MAX_ATLASES;
		int curIdx = partsIndex + i * MapRenderer_ChunksCount;

		Builder_SetPartInfo(&Builder_Parts[i], &offset, &MapRenderer_PartsNormal[curIdx],      &hasNormal);
		Builder_SetPartInfo(&Builder_Parts[j], &offset, &MapRenderer_PartsTranslucent[curIdx], &hasTranslucent);
	}

	if (hasNormal) {
		info->NormalParts = &MapRenderer_PartsNormal[partsIndex];
	}
	if (hasTranslucent) {
		info->TranslucentParts = &MapRenderer_PartsTranslucent[partsIndex];
	}

#ifdef OCCLUSION
	if (info.NormalParts != null || info.TranslucentParts != null)
		info.occlusionFlags = (uint8_t)ComputeOcclusion();
#endif
}

static bool Builder_OccludedLiquid(int chunkIndex) {
	chunkIndex += EXTCHUNK_SIZE_2; /* Checking y above */
	return
		Block_FullOpaque[Builder_Chunk[chunkIndex]]
		&& Block_Draw[Builder_Chunk[chunkIndex - EXTCHUNK_SIZE]] != DRAW_GAS
		&& Block_Draw[Builder_Chunk[chunkIndex - 1]] != DRAW_GAS
		&& Block_Draw[Builder_Chunk[chunkIndex + 1]] != DRAW_GAS
		&& Block_Draw[Builder_Chunk[chunkIndex + EXTCHUNK_SIZE]] != DRAW_GAS;
}

static void Builder_DefaultPreStretchTiles(int x1, int y1, int z1) {
	Mem_Set(Builder_Parts, 0, sizeof(Builder_Parts));
}

static void Builder_DefaultPostStretchTiles(int x1, int y1, int z1) {
	int i, j, vertsCount = Builder_TotalVerticesCount();
	if (vertsCount > Builder_VerticesElems) {
		Mem_Free(Builder_Vertices);
		/* ensure buffer can be accessed with 64 bytes alignment by putting 2 extra vertices at end. */
		Builder_Vertices = Mem_Alloc(vertsCount + 2, sizeof(VertexP3fT2fC4b), "chunk vertices");
		Builder_VerticesElems = vertsCount;
	}

	vertsCount = 0;
	for (i = 0; i < ATLAS1D_MAX_ATLASES; i++) {
		j = i + ATLAS1D_MAX_ATLASES;

		Builder1DPart_CalcOffsets(&Builder_Parts[i], &vertsCount);
		Builder1DPart_CalcOffsets(&Builder_Parts[j], &vertsCount);
	}
}

RNGState spriteRng;
static void Builder_DrawSprite(int count) {
	TextureLoc texLoc = Block_GetTex(Builder_Block, FACE_XMAX);
	int i = Atlas1D_Index(texLoc);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;
	float X = (float)Builder_X, Y = (float)Builder_Y, Z = (float)Builder_Z;

#define s_u1 0.0f
#define s_u2 UV2_Scale
	float x1 = (float)X + 2.50f / 16.0f, y1 = (float)Y,        z1 = (float)Z + 2.50f / 16.0f;
	float x2 = (float)X + 13.5f / 16.0f, y2 = (float)Y + 1.0f, z2 = (float)Z + 13.5f / 16.0f;
	float v1 = vOrigin, v2 = vOrigin + Atlas1D_InvTileSize * UV2_Scale;

	uint8_t offsetType = Block_SpriteOffset[Builder_Block];
	if (offsetType >= 6 && offsetType <= 7) {
		Random_SetSeed(&spriteRng, (Builder_X + 1217 * Builder_Z) & 0x7fffffff);
		float valX = Random_Range(&spriteRng, -3, 3 + 1) / 16.0f;
		float valY = Random_Range(&spriteRng, 0,  3 + 1) / 16.0f;
		float valZ = Random_Range(&spriteRng, -3, 3 + 1) / 16.0f;

#define s_stretch 1.7f / 16.0f
		x1 += valX - s_stretch; x2 += valX + s_stretch;
		z1 += valZ - s_stretch; z2 += valZ + s_stretch;
		if (offsetType == 7) { y1 -= valY; y2 -= valY; }
	}
	
	struct Builder1DPart* part = &Builder_Parts[i];
	PackedCol white = PACKEDCOL_WHITE;
	PackedCol col = Builder_FullBright ? white : Lighting_Col_Sprite_Fast(Builder_X, Builder_Y, Builder_Z);
	Block_Tint(col, Builder_Block);
	VertexP3fT2fC4b v; v.Col = col;

	/* Draw Z axis */
	int index = part->sOffset;
	v.X = x1; v.Y = y1; v.Z = z1; v.U = s_u2; v.V = v2; Builder_Vertices[index + 0] = v;
	          v.Y = y2;                       v.V = v1; Builder_Vertices[index + 1] = v;
	v.X = x2;           v.Z = z2; v.U = s_u1;           Builder_Vertices[index + 2] = v;
	          v.Y = y1;                       v.V = v2; Builder_Vertices[index + 3] = v;

	/* Draw Z axis mirrored */
	index += part->sAdvance;
	v.X = x2; v.Y = y1; v.Z = z2; v.U = s_u2;           Builder_Vertices[index + 0] = v;
	          v.Y = y2;                       v.V = v1; Builder_Vertices[index + 1] = v;
	v.X = x1;           v.Z = z1; v.U = s_u1;           Builder_Vertices[index + 2] = v;
	          v.Y = y1;                       v.V = v2; Builder_Vertices[index + 3] = v;

	/* Draw X axis */
	index += part->sAdvance;
	v.X = x1; v.Y = y1; v.Z = z2; v.U = s_u2;           Builder_Vertices[index + 0] = v;
	          v.Y = y2;                       v.V = v1; Builder_Vertices[index + 1] = v;
	v.X = x2;           v.Z = z1; v.U = s_u1;           Builder_Vertices[index + 2] = v;
	          v.Y = y1;                       v.V = v2; Builder_Vertices[index + 3] = v;

	/* Draw X axis mirrored */
	index += part->sAdvance;
	v.X = x2; v.Y = y1; v.Z = z1; v.U = s_u2;           Builder_Vertices[index + 0] = v;
	          v.Y = y2;                       v.V = v1; Builder_Vertices[index + 1] = v;
	v.X = x1;           v.Z = z2; v.U = s_u1;           Builder_Vertices[index + 2] = v;
	          v.Y = y1;                       v.V = v2; Builder_Vertices[index + 3] = v;

	part->sOffset += 4;
}

void Builder_Init(void) {
	Builder_Offsets[FACE_XMIN] = -1;
	Builder_Offsets[FACE_XMAX] = 1;
	Builder_Offsets[FACE_ZMIN] = -EXTCHUNK_SIZE;
	Builder_Offsets[FACE_ZMAX] = EXTCHUNK_SIZE;
	Builder_Offsets[FACE_YMIN] = -EXTCHUNK_SIZE_2;
	Builder_Offsets[FACE_YMAX] = EXTCHUNK_SIZE_2;
}

void Builder_OnNewMapLoaded(void) {
	Builder_SidesLevel = max(0, Env_SidesHeight);
	Builder_EdgeLevel  = max(0, Env_EdgeHeight);
}


/*########################################################################################################################*
*--------------------------------------------------Normal mesh builder----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol Normal_LightCol(int x, int y, int z, Face face, BlockID block) {
	int offset = (Block_LightOffset[block] >> face) & 1;
	switch (face) {
	case FACE_XMIN:
		return x < offset                ? Env_SunXSide : Lighting_Col_XSide_Fast(x - offset, y, z);
	case FACE_XMAX:
		return x > (World_MaxX - offset) ? Env_SunXSide : Lighting_Col_XSide_Fast(x + offset, y, z);
	case FACE_ZMIN:
		return z < offset                ? Env_SunZSide : Lighting_Col_ZSide_Fast(x, y, z - offset);
	case FACE_ZMAX:
		return z >(World_MaxZ - offset)  ? Env_SunZSide : Lighting_Col_ZSide_Fast(x, y, z + offset);
	case FACE_YMIN:
		return y <= 0                    ? Env_SunYMin  : Lighting_Col_YMin_Fast(x, y - offset, z);
	case FACE_YMAX:
		return y >= World_MaxY           ? Env_SunCol   : Lighting_Col_YMax_Fast(x, (y + 1) - offset, z);
	}

	PackedCol black = PACKEDCOL_BLACK; return black;
}

static bool Normal_CanStretch(BlockID initial, int chunkIndex, int x, int y, int z, Face face) {
	BlockID cur = Builder_Chunk[chunkIndex];
	PackedColUnion initCol, curCol;

	if (cur != initial || Block_IsFaceHidden(cur, Builder_Chunk[chunkIndex + Builder_Offsets[face]], face)) return false;
	if (Builder_FullBright) return true;

	initCol.C = Normal_LightCol(Builder_X, Builder_Y, Builder_Z, face, initial);
	curCol.C  = Normal_LightCol(x, y, z, face, cur);
	return initCol.Raw == curCol.Raw;
}

static int NormalBuilder_StretchXLiquid(int countIndex, int x, int y, int z, int chunkIndex, BlockID block) {
	if (Builder_OccludedLiquid(chunkIndex)) return 0;
	int count = 1;
	x++;
	chunkIndex++;
	countIndex += FACE_COUNT;
	bool stretchTile = (Block_CanStretch[block] & (1 << FACE_YMAX)) != 0;

	while (x < Builder_ChunkEndX && stretchTile && Normal_CanStretch(block, chunkIndex, x, y, z, FACE_YMAX) && !Builder_OccludedLiquid(chunkIndex)) {
		Builder_Counts[countIndex] = 0;
		count++;
		x++;
		chunkIndex++;
		countIndex += FACE_COUNT;
	}
	return count;
}

static int NormalBuilder_StretchX(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face) {
	int count = 1;
	x++;
	chunkIndex++;
	countIndex += FACE_COUNT;
	bool stretchTile = (Block_CanStretch[block] & (1 << face)) != 0;

	while (x < Builder_ChunkEndX && stretchTile && Normal_CanStretch(block, chunkIndex, x, y, z, face)) {
		Builder_Counts[countIndex] = 0;
		count++;
		x++;
		chunkIndex++;
		countIndex += FACE_COUNT;
	}
	return count;
}

static int NormalBuilder_StretchZ(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face) {
	int count = 1;
	z++;
	chunkIndex += EXTCHUNK_SIZE;
	countIndex += CHUNK_SIZE * FACE_COUNT;
	bool stretchTile = (Block_CanStretch[block] & (1 << face)) != 0;

	while (z < Builder_ChunkEndZ && stretchTile && Normal_CanStretch(block, chunkIndex, x, y, z, face)) {
		Builder_Counts[countIndex] = 0;
		count++;
		z++;
		chunkIndex += EXTCHUNK_SIZE;
		countIndex += CHUNK_SIZE * FACE_COUNT;
	}
	return count;
}

static void NormalBuilder_RenderBlock(int index) {
	TextureLoc texLoc;	
	struct Builder1DPart* part;
	PackedCol col;

	int count_XMin, count_XMax, count_ZMin;
	int count_ZMax, count_YMin, count_YMax;
	int offset, count;

	if (Block_Draw[Builder_Block] == DRAW_SPRITE) {
		Builder_FullBright = Block_FullBright[Builder_Block];
		Builder_Tinted = Block_Tinted[Builder_Block];

		count = Builder_Counts[index + FACE_YMAX];
		if (count) Builder_DrawSprite(count);
		return;
	}

	count_XMin = Builder_Counts[index + FACE_XMIN];
	count_XMax = Builder_Counts[index + FACE_XMAX];
	count_ZMin = Builder_Counts[index + FACE_ZMIN];
	count_ZMax = Builder_Counts[index + FACE_ZMAX];
	count_YMin = Builder_Counts[index + FACE_YMIN];
	count_YMax = Builder_Counts[index + FACE_YMAX];

	if (count_XMin == 0 && count_XMax == 0 && count_ZMin == 0 &&
		count_ZMax == 0 && count_YMin == 0 && count_YMax == 0) return;

	bool fullBright = Block_FullBright[Builder_Block];
	int baseOffset  = (Block_Draw[Builder_Block] == DRAW_TRANSLUCENT) * ATLAS1D_MAX_ATLASES;
	int lightFlags  = Block_LightOffset[Builder_Block];

	Drawer_MinBB = Block_MinBB[Builder_Block]; Drawer_MinBB.Y = 1.0f - Drawer_MinBB.Y;
	Drawer_MaxBB = Block_MaxBB[Builder_Block]; Drawer_MaxBB.Y = 1.0f - Drawer_MaxBB.Y;

	Vector3 min = Block_RenderMinBB[Builder_Block], max = Block_RenderMaxBB[Builder_Block];
	Drawer_X1 = Builder_X + min.X; Drawer_Y1 = Builder_Y + min.Y; Drawer_Z1 = Builder_Z + min.Z;
	Drawer_X2 = Builder_X + max.X; Drawer_Y2 = Builder_Y + max.Y; Drawer_Z2 = Builder_Z + max.Z;

	Drawer_Tinted     = Block_Tinted[Builder_Block];
	Drawer_TintColour = Block_FogCol[Builder_Block];
	PackedCol white = PACKEDCOL_WHITE;

	if (count_XMin) {
		texLoc = Block_GetTex(Builder_Block, FACE_XMIN);
		offset = (lightFlags >> FACE_XMIN) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(texLoc)];

		col = fullBright ? white :
			Builder_X >= offset ? Lighting_Col_XSide_Fast(Builder_X - offset, Builder_Y, Builder_Z) : Env_SunXSide;
		Drawer_XMin(count_XMin, col, texLoc, &part->fVertices[FACE_XMIN]);
	}

	if (count_XMax) {
		texLoc = Block_GetTex(Builder_Block, FACE_XMAX);
		offset = (lightFlags >> FACE_XMAX) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(texLoc)];

		col = fullBright ? white :
			Builder_X <= (World_MaxX - offset) ? Lighting_Col_XSide_Fast(Builder_X + offset, Builder_Y, Builder_Z) : Env_SunXSide;
		Drawer_XMax(count_XMax, col, texLoc, &part->fVertices[FACE_XMAX]);
	}

	if (count_ZMin) {
		texLoc = Block_GetTex(Builder_Block, FACE_ZMIN);
		offset = (lightFlags >> FACE_ZMIN) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(texLoc)];

		col = fullBright ? white :
			Builder_Z >= offset ? Lighting_Col_ZSide_Fast(Builder_X, Builder_Y, Builder_Z - offset) : Env_SunZSide;
		Drawer_ZMin(count_ZMin, col, texLoc, &part->fVertices[FACE_ZMIN]);
	}

	if (count_ZMax) {
		texLoc = Block_GetTex(Builder_Block, FACE_ZMAX);
		offset = (lightFlags >> FACE_ZMAX) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(texLoc)];

		col = fullBright ? white :
			Builder_Z <= (World_MaxZ - offset) ? Lighting_Col_ZSide_Fast(Builder_X, Builder_Y, Builder_Z + offset) : Env_SunZSide;
		Drawer_ZMax(count_ZMax, col, texLoc, &part->fVertices[FACE_ZMAX]);
	}

	if (count_YMin) {
		texLoc = Block_GetTex(Builder_Block, FACE_YMIN);
		offset = (lightFlags >> FACE_YMIN) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(texLoc)];

		col = fullBright ? white : Lighting_Col_YMin_Fast(Builder_X, Builder_Y - offset, Builder_Z);
		Drawer_YMin(count_YMin, col, texLoc, &part->fVertices[FACE_YMIN]);
	}

	if (count_YMax) {
		texLoc = Block_GetTex(Builder_Block, FACE_YMAX);
		offset = (lightFlags >> FACE_YMAX) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(texLoc)];

		col = fullBright ? white : Lighting_Col_YMax_Fast(Builder_X, (Builder_Y + 1) - offset, Builder_Z);
		Drawer_YMax(count_YMax, col, texLoc, &part->fVertices[FACE_YMAX]);
	}
}

static void Builder_SetDefault(void) {
	Builder_StretchXLiquid = NULL;
	Builder_StretchX       = NULL;
	Builder_StretchZ       = NULL;
	Builder_RenderBlock    = NULL;

	Builder_UseBitFlags      = false;
	Builder_PreStretchTiles  = Builder_DefaultPreStretchTiles;
	Builder_PostStretchTiles = Builder_DefaultPostStretchTiles;
}

void NormalBuilder_SetActive(void) {
	Builder_SetDefault();
	Builder_StretchXLiquid = NormalBuilder_StretchXLiquid;
	Builder_StretchX       = NormalBuilder_StretchX;
	Builder_StretchZ       = NormalBuilder_StretchZ;
	Builder_RenderBlock    = NormalBuilder_RenderBlock;
}


/*########################################################################################################################*
*-------------------------------------------------Advanced mesh builder---------------------------------------------------*
*#########################################################################################################################*/
Vector3 adv_minBB, adv_maxBB;
bool adv_isTranslucent;
int adv_initBitFlags, adv_lightFlags, adv_baseOffset;
int* adv_bitFlags;
float adv_x1, adv_y1, adv_z1, adv_x2, adv_y2, adv_z2;
PackedCol adv_lerp[5], adv_lerpX[5], adv_lerpZ[5], adv_lerpY[5];

enum ADV_MASK {
	/* z-1 cube points */
	xM1_yM1_zM1, xM1_yCC_zM1, xM1_yP1_zM1,
	xCC_yM1_zM1, xCC_yCC_zM1, xCC_yP1_zM1,
	xP1_yM1_zM1, xP1_yCC_zM1, xP1_yP1_zM1,
	/* z cube points */
	xM1_yM1_zCC, xM1_yCC_zCC, xM1_yP1_zCC,
	xCC_yM1_zCC, xCC_yCC_zCC, xCC_yP1_zCC,
	xP1_yM1_zCC, xP1_yCC_zCC, xP1_yP1_zCC,
	/* z+1 cube points */
	xM1_yM1_zP1, xM1_yCC_zP1, xM1_yP1_zP1,
	xCC_yM1_zP1, xCC_yCC_zP1, xCC_yP1_zP1,
	xP1_yM1_zP1, xP1_yCC_zP1, xP1_yP1_zP1,
};

static int Adv_Lit(int x, int y, int z, int cIndex) {
	int flags, offset, lightHeight;
	BlockID block;
	if (y < 0 || y >= World_Height) return 7; /* all faces lit */

	/* TODO: check sides height (if sides > edges), check if edge block casts a shadow */
	if (x < 0 || z < 0 || x >= World_Width || z >= World_Length) {
		return y >= Builder_EdgeLevel ? 7 : y == (Builder_EdgeLevel - 1) ? 6 : 0;
	}

	flags = 0;
	block = Builder_Chunk[cIndex];
	lightHeight    = Lighting_Heightmap[Lighting_Pack(x, z)];
	adv_lightFlags = Block_LightOffset[block];

	/* Use fact Light(Y.YMin) == Light((Y-1).YMax) */
	offset = (adv_lightFlags >> FACE_YMIN) & 1;
	flags |= ((y - offset) > lightHeight ? 1 : 0);

	/* Light is same for all the horizontal faces */
	flags |= (y > lightHeight ? 2 : 0);

	/* Use fact Light((Y+1).YMin) == Light(Y.YMax) */
	offset = (adv_lightFlags >> FACE_YMAX) & 1;
	flags |= ((y - offset) >= lightHeight ? 4 : 0);

	/* Dynamic lighting */
	if (Block_FullBright[block])                       flags |= 5;
	if (Block_FullBright[Builder_Chunk[cIndex + 324]]) flags |= 4;
	if (Block_FullBright[Builder_Chunk[cIndex - 324]]) flags |= 1;
	return flags;
}

static int Adv_ComputeLightFlags(int x, int y, int z, int cIndex) {
	if (Builder_FullBright) return (1 << xP1_yP1_zP1) - 1; /* all faces fully bright */

	return
		Adv_Lit(x - 1, y, z - 1, cIndex - 1 - 18) << xM1_yM1_zM1 |
		Adv_Lit(x - 1, y, z,     cIndex - 1)      << xM1_yM1_zCC |
		Adv_Lit(x - 1, y, z + 1, cIndex - 1 + 18) << xM1_yM1_zP1 |
		Adv_Lit(x,     y, z - 1, cIndex + 0 - 18) << xCC_yM1_zM1 |
		Adv_Lit(x,     y, z,     cIndex + 0)      << xCC_yM1_zCC |
		Adv_Lit(x,     y, z + 1, cIndex + 0 + 18) << xCC_yM1_zP1 |
		Adv_Lit(x + 1, y, z - 1, cIndex + 1 - 18) << xP1_yM1_zM1 |
		Adv_Lit(x + 1, y, z,     cIndex + 1)      << xP1_yM1_zCC |
		Adv_Lit(x + 1, y, z + 1, cIndex + 1 + 18) << xP1_yM1_zP1;
}

int adv_masks[FACE_COUNT] = {
	/* XMin face */
	(1 << xM1_yM1_zM1) | (1 << xM1_yM1_zCC) | (1 << xM1_yM1_zP1) |
	(1 << xM1_yCC_zM1) | (1 << xM1_yCC_zCC) | (1 << xM1_yCC_zP1) |
	(1 << xM1_yP1_zM1) | (1 << xM1_yP1_zCC) | (1 << xM1_yP1_zP1),
	/* XMax face */
	(1 << xP1_yM1_zM1) | (1 << xP1_yM1_zCC) | (1 << xP1_yM1_zP1) |
	(1 << xP1_yP1_zM1) | (1 << xP1_yP1_zCC) | (1 << xP1_yP1_zP1) |
	(1 << xP1_yCC_zM1) | (1 << xP1_yCC_zCC) | (1 << xP1_yCC_zP1),
	/* ZMin face */
	(1 << xM1_yM1_zM1) | (1 << xCC_yM1_zM1) | (1 << xP1_yM1_zM1) |
	(1 << xM1_yCC_zM1) | (1 << xCC_yCC_zM1) | (1 << xP1_yCC_zM1) |
	(1 << xM1_yP1_zM1) | (1 << xCC_yP1_zM1) | (1 << xP1_yP1_zM1),
	/* ZMax face */
	(1 << xM1_yM1_zP1) | (1 << xCC_yM1_zP1) | (1 << xP1_yM1_zP1) |
	(1 << xM1_yCC_zP1) | (1 << xCC_yCC_zP1) | (1 << xP1_yCC_zP1) |
	(1 << xM1_yP1_zP1) | (1 << xCC_yP1_zP1) | (1 << xP1_yP1_zP1),
	/* YMin face */
	(1 << xM1_yM1_zM1) | (1 << xM1_yM1_zCC) | (1 << xM1_yM1_zP1) |
	(1 << xCC_yM1_zM1) | (1 << xCC_yM1_zCC) | (1 << xCC_yM1_zP1) |
	(1 << xP1_yM1_zM1) | (1 << xP1_yM1_zCC) | (1 << xP1_yM1_zP1),
	/* YMax face */
	(1 << xM1_yP1_zM1) | (1 << xM1_yP1_zCC) | (1 << xM1_yP1_zP1) |
	(1 << xCC_yP1_zM1) | (1 << xCC_yP1_zCC) | (1 << xCC_yP1_zP1) |
	(1 << xP1_yP1_zM1) | (1 << xP1_yP1_zCC) | (1 << xP1_yP1_zP1),
};


static bool Adv_CanStretch(BlockID initial, int chunkIndex, int x, int y, int z, Face face) {
	BlockID cur = Builder_Chunk[chunkIndex];
	adv_bitFlags[chunkIndex] = Adv_ComputeLightFlags(x, y, z, chunkIndex);

	return cur == initial
		&& !Block_IsFaceHidden(cur, Builder_Chunk[chunkIndex + Builder_Offsets[face]], face)
		&& (adv_initBitFlags == adv_bitFlags[chunkIndex]
		/* Check that this face is either fully bright or fully in shadow */
		&& (adv_initBitFlags == 0 || (adv_initBitFlags & adv_masks[face]) == adv_masks[face]));
}

static int Adv_StretchXLiquid(int countIndex, int x, int y, int z, int chunkIndex, BlockID block) {
	if (Builder_OccludedLiquid(chunkIndex)) return 0;
	adv_initBitFlags = Adv_ComputeLightFlags(x, y, z, chunkIndex);
	adv_bitFlags[chunkIndex] = adv_initBitFlags;

	int count = 1;
	x++;
	chunkIndex++;
	countIndex += FACE_COUNT;
	bool stretchTile = (Block_CanStretch[block] & (1 << FACE_YMAX)) != 0;

	while (x < Builder_ChunkEndX && stretchTile && Adv_CanStretch(block, chunkIndex, x, y, z, FACE_YMAX) && !Builder_OccludedLiquid(chunkIndex)) {
		Builder_Counts[countIndex] = 0;
		count++;
		x++;
		chunkIndex++;
		countIndex += FACE_COUNT;
	}
	return count;
}

static int Adv_StretchX(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face) {
	adv_initBitFlags = Adv_ComputeLightFlags(x, y, z, chunkIndex);
	adv_bitFlags[chunkIndex] = adv_initBitFlags;

	int count = 1;
	x++;
	chunkIndex++;
	countIndex += FACE_COUNT;
	bool stretchTile = (Block_CanStretch[block] & (1 << face)) != 0;

	while (x < Builder_ChunkEndX && stretchTile && Adv_CanStretch(block, chunkIndex, x, y, z, face)) {
		Builder_Counts[countIndex] = 0;
		count++;
		x++;
		chunkIndex++;
		countIndex += FACE_COUNT;
	}
	return count;
}

static int Adv_StretchZ(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face) {
	adv_initBitFlags = Adv_ComputeLightFlags(x, y, z, chunkIndex);
	adv_bitFlags[chunkIndex] = adv_initBitFlags;

	int count = 1;
	z++;
	chunkIndex += EXTCHUNK_SIZE;
	countIndex += CHUNK_SIZE * FACE_COUNT;
	bool stretchTile = (Block_CanStretch[block] & (1 << face)) != 0;

	while (z < Builder_ChunkEndZ && stretchTile && Adv_CanStretch(block, chunkIndex, x, y, z, face)) {
		Builder_Counts[countIndex] = 0;
		count++;
		z++;
		chunkIndex += EXTCHUNK_SIZE;
		countIndex += CHUNK_SIZE * FACE_COUNT;
	}
	return count;
}


#define Adv_CountBits(F, a, b, c, d) (((F >> a) & 1) + ((F >> b) & 1) + ((F >> c) & 1) + ((F >> d) & 1))
#define Adv_Tint(c) c.R = (uint8_t)(c.R * tint.R / 255); c.G = (uint8_t)(c.G * tint.G / 255); c.B = (uint8_t)(c.B * tint.B / 255);

static void Adv_DrawXMin(int count) {
	TextureLoc texLoc = Block_GetTex(Builder_Block, FACE_XMIN);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = adv_minBB.Z, u2 = (count - 1) + adv_maxBB.Z * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.Y * Atlas1D_InvTileSize;
	float v2 = vOrigin + adv_minBB.Y * Atlas1D_InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aY0_Z0 = Adv_CountBits(F, xM1_yM1_zM1, xM1_yCC_zM1, xM1_yM1_zCC, xM1_yCC_zCC);
	int aY0_Z1 = Adv_CountBits(F, xM1_yM1_zP1, xM1_yCC_zP1, xM1_yM1_zCC, xM1_yCC_zCC);
	int aY1_Z0 = Adv_CountBits(F, xM1_yP1_zM1, xM1_yCC_zM1, xM1_yP1_zCC, xM1_yCC_zCC);
	int aY1_Z1 = Adv_CountBits(F, xM1_yP1_zP1, xM1_yCC_zP1, xM1_yP1_zCC, xM1_yCC_zCC);

	PackedCol white = PACKEDCOL_WHITE;
	PackedCol col0_0 = Builder_FullBright ? white : adv_lerpX[aY0_Z0], col1_0 = Builder_FullBright ? white : adv_lerpX[aY1_Z0];
	PackedCol col1_1 = Builder_FullBright ? white : adv_lerpX[aY1_Z1], col0_1 = Builder_FullBright ? white : adv_lerpX[aY0_Z1];
	if (Builder_Tinted) {
		PackedCol tint = Block_FogCol[Builder_Block];
		Adv_Tint(col0_0); Adv_Tint(col1_0); Adv_Tint(col1_1); Adv_Tint(col0_1);
	}

	VertexP3fT2fC4b* vertices = part->fVertices[FACE_XMIN];
	VertexP3fT2fC4b v; v.X = adv_x1;
	if (aY0_Z0 + aY1_Z1 > aY0_Z1 + aY1_Z0) {
		v.Y = adv_y2; v.Z = adv_z1;               v.U = u1; v.V = v1; v.Col = col1_0; *vertices++ = v;
		v.Y = adv_y1;                                       v.V = v2; v.Col = col0_0; *vertices++ = v;
		              v.Z = adv_z2 + (count - 1); v.U = u2;           v.Col = col0_1; *vertices++ = v;
		v.Y = adv_y2;                                       v.V = v1; v.Col = col1_1; *vertices++ = v;
	} else {
		v.Y = adv_y2; v.Z = adv_z2 + (count - 1); v.U = u2; v.V = v1; v.Col = col1_1; *vertices++ = v;
		              v.Z = adv_z1;               v.U = u1;           v.Col = col1_0; *vertices++ = v;
		v.Y = adv_y1;                                       v.V = v2; v.Col = col0_0; *vertices++ = v;
		              v.Z = adv_z2 + (count - 1); v.U = u2;           v.Col = col0_1; *vertices++ = v;
	}
	part->fVertices[FACE_XMIN] = vertices;
}

static void Adv_DrawXMax(int count) {
	TextureLoc texLoc = Block_GetTex(Builder_Block, FACE_XMAX);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = (count - adv_minBB.Z), u2 = (1 - adv_maxBB.Z) * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.Y * Atlas1D_InvTileSize;
	float v2 = vOrigin + adv_minBB.Y * Atlas1D_InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aY0_Z0 = Adv_CountBits(F, xP1_yM1_zM1, xP1_yCC_zM1, xP1_yM1_zCC, xP1_yCC_zCC);
	int aY0_Z1 = Adv_CountBits(F, xP1_yM1_zP1, xP1_yCC_zP1, xP1_yM1_zCC, xP1_yCC_zCC);
	int aY1_Z0 = Adv_CountBits(F, xP1_yP1_zM1, xP1_yCC_zM1, xP1_yP1_zCC, xP1_yCC_zCC);
	int aY1_Z1 = Adv_CountBits(F, xP1_yP1_zP1, xP1_yCC_zP1, xP1_yP1_zCC, xP1_yCC_zCC);

	PackedCol white = PACKEDCOL_WHITE;
	PackedCol col0_0 = Builder_FullBright ? white : adv_lerpX[aY0_Z0], col1_0 = Builder_FullBright ? white : adv_lerpX[aY1_Z0];
	PackedCol col1_1 = Builder_FullBright ? white : adv_lerpX[aY1_Z1], col0_1 = Builder_FullBright ? white : adv_lerpX[aY0_Z1];
	if (Builder_Tinted) {
		PackedCol tint = Block_FogCol[Builder_Block];
		Adv_Tint(col0_0); Adv_Tint(col1_0); Adv_Tint(col1_1); Adv_Tint(col0_1);
	}

	VertexP3fT2fC4b* vertices = part->fVertices[FACE_XMAX];
	VertexP3fT2fC4b v; v.X = adv_x2;
	if (aY0_Z0 + aY1_Z1 > aY0_Z1 + aY1_Z0) {
		v.Y = adv_y2; v.Z = adv_z1;               v.U = u1; v.V = v1; v.Col = col1_0; *vertices++ = v;
		              v.Z = adv_z2 + (count - 1); v.U = u2;           v.Col = col1_1; *vertices++ = v;
		v.Y = adv_y1;                                       v.V = v2; v.Col = col0_1; *vertices++ = v;
		              v.Z = adv_z1;               v.U = u1;           v.Col = col0_0; *vertices++ = v;
	} else {
		v.Y = adv_y2; v.Z = adv_z2 + (count - 1); v.U = u2; v.V = v1; v.Col = col1_1; *vertices++ = v;
		v.Y = adv_y1;                                       v.V = v2; v.Col = col0_1; *vertices++ = v;
		              v.Z = adv_z1;               v.U = u1;           v.Col = col0_0; *vertices++ = v;
		v.Y = adv_y2;                                       v.V = v1; v.Col = col1_0; *vertices++ = v;
	}
	part->fVertices[FACE_XMAX] = vertices;
}

static void Adv_DrawZMin(int count) {
	TextureLoc texLoc = Block_GetTex(Builder_Block, FACE_ZMIN);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = (count - adv_minBB.X), u2 = (1 - adv_maxBB.X) * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.Y * Atlas1D_InvTileSize;
	float v2 = vOrigin + adv_minBB.Y * Atlas1D_InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aX0_Y0 = Adv_CountBits(F, xM1_yM1_zM1, xM1_yCC_zM1, xCC_yM1_zM1, xCC_yCC_zM1);
	int aX0_Y1 = Adv_CountBits(F, xM1_yP1_zM1, xM1_yCC_zM1, xCC_yP1_zM1, xCC_yCC_zM1);
	int aX1_Y0 = Adv_CountBits(F, xP1_yM1_zM1, xP1_yCC_zM1, xCC_yM1_zM1, xCC_yCC_zM1);
	int aX1_Y1 = Adv_CountBits(F, xP1_yP1_zM1, xP1_yCC_zM1, xCC_yP1_zM1, xCC_yCC_zM1);

	PackedCol white = PACKEDCOL_WHITE;
	PackedCol col0_0 = Builder_FullBright ? white : adv_lerpZ[aX0_Y0], col1_0 = Builder_FullBright ? white : adv_lerpZ[aX1_Y0];
	PackedCol col1_1 = Builder_FullBright ? white : adv_lerpZ[aX1_Y1], col0_1 = Builder_FullBright ? white : adv_lerpZ[aX0_Y1];
	if (Builder_Tinted) {
		PackedCol tint = Block_FogCol[Builder_Block];
		Adv_Tint(col0_0); Adv_Tint(col1_0); Adv_Tint(col1_1); Adv_Tint(col0_1);
	}

	VertexP3fT2fC4b* vertices = part->fVertices[FACE_ZMIN];
	VertexP3fT2fC4b v; v.Z = adv_z1;
	if (aX1_Y1 + aX0_Y0 > aX0_Y1 + aX1_Y0) {
		v.X = adv_x2 + (count - 1); v.Y = adv_y1; v.U = u2; v.V = v2; v.Col = col1_0; *vertices++ = v;
		v.X = adv_x1;                             v.U = u1;           v.Col = col0_0; *vertices++ = v;
		                            v.Y = adv_y2;           v.V = v1; v.Col = col0_1; *vertices++ = v;
		v.X = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_1; *vertices++ = v;
	} else {
		v.X = adv_x1;               v.Y = adv_y1; v.U = u1; v.V = v2; v.Col = col0_0; *vertices++ = v;
		                            v.Y = adv_y2;           v.V = v1; v.Col = col0_1; *vertices++ = v;
		v.X = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_1; *vertices++ = v;
		                            v.Y = adv_y1;           v.V = v2; v.Col = col1_0; *vertices++ = v;
	}
	part->fVertices[FACE_ZMIN] = vertices;
}

static void Adv_DrawZMax(int count) {
	TextureLoc texLoc = Block_GetTex(Builder_Block, FACE_ZMAX);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = adv_minBB.X, u2 = (count - 1) + adv_maxBB.X * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.Y * Atlas1D_InvTileSize;
	float v2 = vOrigin + adv_minBB.Y * Atlas1D_InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aX0_Y0 = Adv_CountBits(F, xM1_yM1_zP1, xM1_yCC_zP1, xCC_yM1_zP1, xCC_yCC_zP1);
	int aX1_Y0 = Adv_CountBits(F, xP1_yM1_zP1, xP1_yCC_zP1, xCC_yM1_zP1, xCC_yCC_zP1);
	int aX0_Y1 = Adv_CountBits(F, xM1_yP1_zP1, xM1_yCC_zP1, xCC_yP1_zP1, xCC_yCC_zP1);
	int aX1_Y1 = Adv_CountBits(F, xP1_yP1_zP1, xP1_yCC_zP1, xCC_yP1_zP1, xCC_yCC_zP1);

	PackedCol white = PACKEDCOL_WHITE;
	PackedCol col1_1 = Builder_FullBright ? white : adv_lerpZ[aX1_Y1], col1_0 = Builder_FullBright ? white : adv_lerpZ[aX1_Y0];
	PackedCol col0_0 = Builder_FullBright ? white : adv_lerpZ[aX0_Y0], col0_1 = Builder_FullBright ? white : adv_lerpZ[aX0_Y1];
	if (Builder_Tinted) {
		PackedCol tint = Block_FogCol[Builder_Block];
		Adv_Tint(col0_0); Adv_Tint(col1_0); Adv_Tint(col1_1); Adv_Tint(col0_1);
	}

	VertexP3fT2fC4b* vertices = part->fVertices[FACE_ZMAX];
	VertexP3fT2fC4b v; v.Z = adv_z2;
	if (aX1_Y1 + aX0_Y0 > aX0_Y1 + aX1_Y0) {
		v.X = adv_x1;               v.Y = adv_y2; v.U = u1; v.V = v1; v.Col = col0_1; *vertices++ = v;
		                            v.Y = adv_y1;           v.V = v2; v.Col = col0_0; *vertices++ = v;
		v.X = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_0; *vertices++ = v;
		                            v.Y = adv_y2;           v.V = v1; v.Col = col1_1; *vertices++ = v;
	} else {
		v.X = adv_x2 + (count - 1); v.Y = adv_y2; v.U = u2; v.V = v1; v.Col = col1_1; *vertices++ = v;
		v.X = adv_x1;                             v.U = u1;           v.Col = col0_1; *vertices++ = v;
		                            v.Y = adv_y1;           v.V = v2; v.Col = col0_0; *vertices++ = v;
		v.X = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_0; *vertices++ = v;
	}
	part->fVertices[FACE_ZMAX] = vertices;
}

static void Adv_DrawYMin(int count) {
	TextureLoc texLoc = Block_GetTex(Builder_Block, FACE_YMIN);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = adv_minBB.X, u2 = (count - 1) + adv_maxBB.X * UV2_Scale;
	float v1 = vOrigin + adv_minBB.Z * Atlas1D_InvTileSize;
	float v2 = vOrigin + adv_maxBB.Z * Atlas1D_InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aX0_Z0 = Adv_CountBits(F, xM1_yM1_zM1, xM1_yM1_zCC, xCC_yM1_zM1, xCC_yM1_zCC);
	int aX1_Z0 = Adv_CountBits(F, xP1_yM1_zM1, xP1_yM1_zCC, xCC_yM1_zM1, xCC_yM1_zCC);
	int aX0_Z1 = Adv_CountBits(F, xM1_yM1_zP1, xM1_yM1_zCC, xCC_yM1_zP1, xCC_yM1_zCC);
	int aX1_Z1 = Adv_CountBits(F, xP1_yM1_zP1, xP1_yM1_zCC, xCC_yM1_zP1, xCC_yM1_zCC);

	PackedCol white = PACKEDCOL_WHITE;
	PackedCol col0_1 = Builder_FullBright ? white : adv_lerpY[aX0_Z1], col1_1 = Builder_FullBright ? white : adv_lerpY[aX1_Z1];
	PackedCol col1_0 = Builder_FullBright ? white : adv_lerpY[aX1_Z0], col0_0 = Builder_FullBright ? white : adv_lerpY[aX0_Z0];
	if (Builder_Tinted) {
		PackedCol tint = Block_FogCol[Builder_Block];
		Adv_Tint(col0_0); Adv_Tint(col1_0); Adv_Tint(col1_1); Adv_Tint(col0_1);
	}

	VertexP3fT2fC4b* vertices = part->fVertices[FACE_YMIN];
	VertexP3fT2fC4b v; v.Y = adv_y1;
	if (aX0_Z1 + aX1_Z0 > aX0_Z0 + aX1_Z1) {
		v.X = adv_x2 + (count - 1); v.Z = adv_z2; v.U = u2; v.V = v2; v.Col = col1_1; *vertices++ = v;
		v.X = adv_x1;                             v.U = u1;           v.Col = col0_1; *vertices++ = v;
		                            v.Z = adv_z1;           v.V = v1; v.Col = col0_0; *vertices++ = v;
		v.X = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_0; *vertices++ = v;
	} else {
		v.X = adv_x1;               v.Z = adv_z2; v.U = u1; v.V = v2; v.Col = col0_1; *vertices++ = v;
		                            v.Z = adv_z1;           v.V = v1; v.Col = col0_0; *vertices++ = v;
		v.X = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_0; *vertices++ = v;
		                            v.Z = adv_z2;           v.V = v2; v.Col = col1_1; *vertices++ = v;
	}
	part->fVertices[FACE_YMIN] = vertices;
}

static void Adv_DrawYMax(int count) {
	TextureLoc texLoc = Block_GetTex(Builder_Block, FACE_YMAX);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = adv_minBB.X, u2 = (count - 1) + adv_maxBB.X * UV2_Scale;
	float v1 = vOrigin + adv_minBB.Z * Atlas1D_InvTileSize;
	float v2 = vOrigin + adv_maxBB.Z * Atlas1D_InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aX0_Z0 = Adv_CountBits(F, xM1_yP1_zM1, xM1_yP1_zCC, xCC_yP1_zM1, xCC_yP1_zCC);
	int aX1_Z0 = Adv_CountBits(F, xP1_yP1_zM1, xP1_yP1_zCC, xCC_yP1_zM1, xCC_yP1_zCC);
	int aX0_Z1 = Adv_CountBits(F, xM1_yP1_zP1, xM1_yP1_zCC, xCC_yP1_zP1, xCC_yP1_zCC);
	int aX1_Z1 = Adv_CountBits(F, xP1_yP1_zP1, xP1_yP1_zCC, xCC_yP1_zP1, xCC_yP1_zCC);

	PackedCol white = PACKEDCOL_WHITE;
	PackedCol col0_0 = Builder_FullBright ? white : adv_lerp[aX0_Z0], col1_0 = Builder_FullBright ? white : adv_lerp[aX1_Z0];
	PackedCol col1_1 = Builder_FullBright ? white : adv_lerp[aX1_Z1], col0_1 = Builder_FullBright ? white : adv_lerp[aX0_Z1];
	if (Builder_Tinted) {
		PackedCol tint = Block_FogCol[Builder_Block];
		Adv_Tint(col0_0); Adv_Tint(col1_0); Adv_Tint(col1_1); Adv_Tint(col0_1);
	}

	VertexP3fT2fC4b* vertices = part->fVertices[FACE_YMAX];
	VertexP3fT2fC4b v; v.Y = adv_y2;
	if (aX0_Z0 + aX1_Z1 > aX0_Z1 + aX1_Z0) {
		v.X = adv_x2 + (count - 1); v.Z = adv_z1; v.U = u2; v.V = v1; v.Col = col1_0; *vertices++ = v;
		v.X = adv_x1;                             v.U = u1;           v.Col = col0_0; *vertices++ = v;
		                            v.Z = adv_z2;           v.V = v2; v.Col = col0_1; *vertices++ = v;
		v.X = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_1; *vertices++ = v;
	} else {
		v.X = adv_x1;               v.Z = adv_z1; v.U = u1; v.V = v1; v.Col = col0_0; *vertices++ = v;
		                            v.Z = adv_z2;           v.V = v2; v.Col = col0_1; *vertices++ = v;
		v.X = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_1; *vertices++ = v;
		                            v.Z = adv_z1;           v.V = v1; v.Col = col1_0; *vertices++ = v;
	}
	part->fVertices[FACE_YMAX] = vertices;
}

static void Adv_RenderBlock(int index) {
	Vector3 min, max;
	int count_XMin, count_XMax, count_ZMin;
	int count_ZMax, count_YMin, count_YMax;
	int count;

	if (Block_Draw[Builder_Block] == DRAW_SPRITE) {
		Builder_FullBright = Block_FullBright[Builder_Block];
		Builder_Tinted = Block_Tinted[Builder_Block];

		count = Builder_Counts[index + FACE_YMAX];
		if (count) Builder_DrawSprite(count);
		return;
	}

	count_XMin = Builder_Counts[index + FACE_XMIN];
	count_XMax = Builder_Counts[index + FACE_XMAX];
	count_ZMin = Builder_Counts[index + FACE_ZMIN];
	count_ZMax = Builder_Counts[index + FACE_ZMAX];
	count_YMin = Builder_Counts[index + FACE_YMIN];
	count_YMax = Builder_Counts[index + FACE_YMAX];

	if (count_XMin == 0 && count_XMax == 0 && count_ZMin == 0 &&
		count_ZMax == 0 && count_YMin == 0 && count_YMax == 0) return;

	Builder_FullBright = Block_FullBright[Builder_Block];
	adv_baseOffset = (Block_Draw[Builder_Block] == DRAW_TRANSLUCENT) * ATLAS1D_MAX_ATLASES;
	adv_lightFlags = Block_LightOffset[Builder_Block];
	Builder_Tinted = Block_Tinted[Builder_Block];

	min = Block_RenderMinBB[Builder_Block]; max = Block_RenderMaxBB[Builder_Block];
	adv_x1 = Builder_X + min.X; adv_y1 = Builder_Y + min.Y; adv_z1 = Builder_Z + min.Z;
	adv_x2 = Builder_X + max.X; adv_y2 = Builder_Y + max.Y; adv_z2 = Builder_Z + max.Z;

	adv_minBB = Block_MinBB[Builder_Block]; adv_maxBB = Block_MaxBB[Builder_Block];
	adv_minBB.Y = 1.0f - adv_minBB.Y; adv_maxBB.Y = 1.0f - adv_maxBB.Y;

	if (count_XMin) Adv_DrawXMin(count_XMin);
	if (count_XMax) Adv_DrawXMax(count_XMax);
	if (count_ZMin) Adv_DrawZMin(count_ZMin);
	if (count_ZMax) Adv_DrawZMax(count_ZMax);
	if (count_YMin) Adv_DrawYMin(count_YMin);
	if (count_YMax) Adv_DrawYMax(count_YMax);
}

static void Adv_PreStretchTiles(int x1, int y1, int z1) {
	Builder_DefaultPreStretchTiles(x1, y1, z1);
	adv_bitFlags = Builder_BitFlags;
	int i;

	for (i = 0; i <= 4; i++) {
		adv_lerp[i]  = PackedCol_Lerp(Env_ShadowCol,   Env_SunCol,   i / 4.0f);
		adv_lerpX[i] = PackedCol_Lerp(Env_ShadowXSide, Env_SunXSide, i / 4.0f);
		adv_lerpZ[i] = PackedCol_Lerp(Env_ShadowZSide, Env_SunZSide, i / 4.0f);
		adv_lerpY[i] = PackedCol_Lerp(Env_ShadowYMin,  Env_SunYMin,  i / 4.0f);
	}
}

void AdvBuilder_SetActive(void) {
	Builder_SetDefault();
	Builder_StretchXLiquid  = Adv_StretchXLiquid;
	Builder_StretchX        = Adv_StretchX;
	Builder_StretchZ        = Adv_StretchZ;
	Builder_RenderBlock     = Adv_RenderBlock;
	Builder_PreStretchTiles = Adv_PreStretchTiles;
}
