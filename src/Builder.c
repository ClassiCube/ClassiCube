#include "Builder.h"
#include "Constants.h"
#include "World.h"
#include "Funcs.h"
#include "Lighting.h"
#include "Platform.h"
#include "MapRenderer.h"
#include "Graphics.h"
#include "Drawer.h"
#include "ExtMath.h"
#include "Block.h"
#include "PackedCol.h"
#include "TexturePack.h"
#include "Game.h"
#include "Options.h"

int Builder_SidesLevel, Builder_EdgeLevel;
/* Packs an index into the 16x16x16 count array. Coordinates range from 0 to 15. */
#define Builder_PackCount(xx, yy, zz) ((((yy) << 8) | ((zz) << 4) | (xx)) * FACE_COUNT)
/* Packs an index into the 18x18x18 chunk array. Coordinates range from -1 to 16. */
#define Builder_PackChunk(xx, yy, zz) (((yy) + 1) * EXTCHUNK_SIZE_2 + ((zz) + 1) * EXTCHUNK_SIZE + ((xx) + 1))

static BlockID* Builder_Chunk;
static cc_uint8* Builder_Counts;
static int* Builder_BitFlags;
static int Builder_X, Builder_Y, Builder_Z;
static BlockID Builder_Block;
static int Builder_ChunkIndex;
static cc_bool Builder_FullBright;
static int Builder_ChunkEndX, Builder_ChunkEndZ;
static int Builder_Offsets[FACE_COUNT] = { -1,1, -EXTCHUNK_SIZE,EXTCHUNK_SIZE, -EXTCHUNK_SIZE_2,EXTCHUNK_SIZE_2 };

static int (*Builder_StretchXLiquid)(int countIndex, int x, int y, int z, int chunkIndex, BlockID block);
static int (*Builder_StretchX)(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face);
static int (*Builder_StretchZ)(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face);
static void (*Builder_RenderBlock)(int countsIndex, int x, int y, int z);
static void (*Builder_PrePrepareChunk)(void);
static void (*Builder_PostPrepareChunk)(void);

/* Contains state for vertices for a portion of a chunk mesh (vertices that are in a 1D atlas) */
struct Builder1DPart {
	/* Union to save on memory, since chunk building is divided into counting then building phases */
	union FaceData {
		struct VertexTextured* vertices[FACE_COUNT];
		int count[FACE_COUNT];
	} faces;
	int sCount, sOffset;
};

/* Part builder data, for both normal and translucent parts.
The first ATLAS1D_MAX_ATLASES parts are for normal parts, remainder are for translucent parts. */
static CC_BIG_VAR struct Builder1DPart Builder_Parts[ATLAS1D_MAX_ATLASES * 2];
static struct VertexTextured* Builder_Vertices;

static int Builder1DPart_VerticesCount(struct Builder1DPart* part) {
	int i, count = part->sCount;
	for (i = 0; i < FACE_COUNT; i++) { count += part->faces.count[i]; }
	return count;
}

static int Builder1DPart_CalcOffsets(struct Builder1DPart* part, int offset) {
	int i, counts[FACE_COUNT];
	part->sOffset = offset;

	/* Have to copy first due to count and vertices fields being a union */
	for (i = 0; i < FACE_COUNT; i++)
	{
		counts[i] = part->faces.count[i];
	}

	offset += part->sCount;
	for (i = 0; i < FACE_COUNT; i++) 
	{
		part->faces.vertices[i] = &Builder_Vertices[offset];
		offset += counts[i];
	}
	return offset;
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
static void AddSpriteVertices(BlockID block) {
	int i = Atlas1D_Index(Block_Tex(block, FACE_XMAX));
	struct Builder1DPart* part = &Builder_Parts[i];
	part->sCount += 4 * 4;
}

static void AddVertices(BlockID block, Face face) {
	int baseOffset = (Blocks.Draw[block] == DRAW_TRANSLUCENT) * ATLAS1D_MAX_ATLASES;
	int i = Atlas1D_Index(Block_Tex(block, face));
	struct Builder1DPart* part = &Builder_Parts[baseOffset + i];
	part->faces.count[face] += 4;
}

#ifdef CC_BUILD_GL11
static void BuildPartVbs(struct ChunkPartInfo* info) {
	/* Sprites vertices are stored before chunk face sides */
	int i, count, offset = info->offset + info->spriteCount;
	for (i = 0; i < FACE_COUNT; i++) {
		count = info->counts[i];

		if (count) {
			info->vbs[i] = Gfx_CreateVb2(&Builder_Vertices[offset], VERTEX_FORMAT_TEXTURED, count);
			offset += count;
		} else {
			info->vbs[i] = 0;
		}
	}

	count  = info->spriteCount;
	offset = info->offset;
	if (count) {
		info->vbs[i] = Gfx_CreateVb2(&Builder_Vertices[offset], VERTEX_FORMAT_TEXTURED, count);
	} else {
		info->vbs[i] = 0;
	}
}
#endif

static cc_bool SetPartInfo(struct Builder1DPart* part, int* offset, struct ChunkPartInfo* info) {
	int vCount = Builder1DPart_VerticesCount(part);
	info->offset = -1;
	if (!vCount) return false;

	info->offset = *offset;
	*offset += vCount;

	info->counts[FACE_XMIN] = part->faces.count[FACE_XMIN];
	info->counts[FACE_XMAX] = part->faces.count[FACE_XMAX];
	info->counts[FACE_ZMIN] = part->faces.count[FACE_ZMIN];
	info->counts[FACE_ZMAX] = part->faces.count[FACE_ZMAX];
	info->counts[FACE_YMIN] = part->faces.count[FACE_YMIN];
	info->counts[FACE_YMAX] = part->faces.count[FACE_YMAX];
	info->spriteCount       = part->sCount;
	return true;
}


static void PrepareChunk(int x1, int y1, int z1) {
	int xMax = min(World.Width,  x1 + CHUNK_SIZE);
	int yMax = min(World.Height, y1 + CHUNK_SIZE);
	int zMax = min(World.Length, z1 + CHUNK_SIZE);

	int cIndex, index, tileIdx;
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
				if (Blocks.Draw[b] == DRAW_GAS) continue;
				index = Builder_PackCount(xx, yy, zz);

				/* Sprites can't be stretched, nor can then be they hidden by other blocks. */
				/* Note sprites are drawn using DrawSprite and not with any of the DrawXFace. */
				if (Blocks.Draw[b] == DRAW_SPRITE) { AddSpriteVertices(b); continue; }

				Builder_X = x; Builder_Y = y; Builder_Z = z;
				Builder_FullBright = Blocks.Brightness[b];
				tileIdx = b * BLOCK_COUNT;
				/* All of these function calls are inlined as they can be called tens of millions to hundreds of millions of times. */

				if (Builder_Counts[index] == 0 ||
					(x == 0 && (y < Builder_SidesLevel || (b >= BLOCK_WATER && b <= BLOCK_STILL_LAVA && y < Builder_EdgeLevel))) ||
					(x != 0 && (Blocks.Hidden[tileIdx + Builder_Chunk[cIndex - 1]] & FACE_BIT_XMIN) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					Builder_Counts[index] = Builder_StretchZ(index, x, y, z, cIndex, b, FACE_XMIN);
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(x == World.MaxX && (y < Builder_SidesLevel || (b >= BLOCK_WATER && b <= BLOCK_STILL_LAVA && y < Builder_EdgeLevel))) ||
					(x != World.MaxX && (Blocks.Hidden[tileIdx + Builder_Chunk[cIndex + 1]] & FACE_BIT_XMAX) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					Builder_Counts[index] = Builder_StretchZ(index, x, y, z, cIndex, b, FACE_XMAX);
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(z == 0 && (y < Builder_SidesLevel || (b >= BLOCK_WATER && b <= BLOCK_STILL_LAVA && y < Builder_EdgeLevel))) ||
					(z != 0 && (Blocks.Hidden[tileIdx + Builder_Chunk[cIndex - EXTCHUNK_SIZE]] & FACE_BIT_ZMIN) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					Builder_Counts[index] = Builder_StretchX(index, x, y, z, cIndex, b, FACE_ZMIN);
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(z == World.MaxZ && (y < Builder_SidesLevel || (b >= BLOCK_WATER && b <= BLOCK_STILL_LAVA && y < Builder_EdgeLevel))) ||
					(z != World.MaxZ && (Blocks.Hidden[tileIdx + Builder_Chunk[cIndex + EXTCHUNK_SIZE]] & FACE_BIT_ZMAX) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					Builder_Counts[index] = Builder_StretchX(index, x, y, z, cIndex, b, FACE_ZMAX);
				}

				index++;
				if (Builder_Counts[index] == 0 || y == 0 ||
					(Blocks.Hidden[tileIdx + Builder_Chunk[cIndex - EXTCHUNK_SIZE_2]] & FACE_BIT_YMIN) != 0) {
					Builder_Counts[index] = 0;
				} else {
					Builder_Counts[index] = Builder_StretchX(index, x, y, z, cIndex, b, FACE_YMIN);
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(Blocks.Hidden[tileIdx + Builder_Chunk[cIndex + EXTCHUNK_SIZE_2]] & FACE_BIT_YMAX) != 0) {
					Builder_Counts[index] = 0;
				} else if (b < BLOCK_WATER || b > BLOCK_STILL_LAVA) {
					Builder_Counts[index] = Builder_StretchX(index, x, y, z, cIndex, b, FACE_YMAX);
				} else {
					Builder_Counts[index] = Builder_StretchXLiquid(index, x, y, z, cIndex, b);
				}
			}
		}
	}
}

#define ReadChunkBody(get_block)\
for (yy = -1; yy < 17; ++yy) {\
	y = yy + y1;\
	for (zz = -1; zz < 17; ++zz) {\
\
		index  = World_Pack(x1 - 1, y, z1 + zz);\
		cIndex = Builder_PackChunk(-1, yy, zz);\
		for (xx = -1; xx < 17; ++xx, ++index, ++cIndex) {\
\
			block    = get_block;\
			allAir   = allAir   && Blocks.Draw[block] == DRAW_GAS;\
			allSolid = allSolid && Blocks.FullOpaque[block];\
			Builder_Chunk[cIndex] = block;\
		}\
	}\
}

static cc_bool ReadChunkData(int x1, int y1, int z1, cc_bool* outAllAir) {
	BlockRaw* blocks = World.Blocks;
	BlockRaw* blocks2;
	cc_bool allAir = true, allSolid = true;
	int index, cIndex;
	BlockID block;
	int xx, yy, zz, y;

#ifndef EXTENDED_BLOCKS
	ReadChunkBody(blocks[index]);
#else
	if (World.IDMask <= 0xFF) {
		ReadChunkBody(blocks[index]);
	} else {
		blocks2 = World.Blocks2;
		ReadChunkBody(blocks[index] | (blocks2[index] << 8));
	}
#endif

	*outAllAir = allAir;
	return allSolid;
}

#define ReadBorderChunkBody(get_block)\
for (yy = -1; yy < 17; ++yy) {\
	y = yy + y1;\
	if (y < 0) continue;\
	if (y >= World.Height) break;\
\
	for (zz = -1; zz < 17; ++zz) {\
		z = zz + z1;\
		if (z < 0) continue;\
		if (z >= World.Length) break;\
\
		index  = World_Pack(x1 - 1, y, z);\
		cIndex = Builder_PackChunk(-1, yy, zz);\
\
		for (xx = -1; xx < 17; ++xx, ++index, ++cIndex) {\
			x = xx + x1;\
			if (x < 0) continue;\
			if (x >= World.Width) break;\
\
			block  = get_block;\
			allAir = allAir && Blocks.Draw[block] == DRAW_GAS;\
			Builder_Chunk[cIndex] = block;\
		}\
	}\
}

static cc_bool ReadBorderChunkData(int x1, int y1, int z1, cc_bool* outAllAir) {
	BlockRaw* blocks = World.Blocks;
	BlockRaw* blocks2;
	cc_bool allAir = true;
	int index, cIndex;
	BlockID block;
	int xx, yy, zz, x, y, z;

#ifndef EXTENDED_BLOCKS
	ReadBorderChunkBody(blocks[index]);
#else
	if (World.IDMask <= 0xFF) {
		ReadBorderChunkBody(blocks[index]);
	} else {
		blocks2 = World.Blocks2;
		ReadBorderChunkBody(blocks[index] | (blocks2[index] << 8));
	}
#endif

	*outAllAir = allAir;
	return false;
}

static void OutputChunkPartsMeta(int x, int y, int z, struct ChunkInfo* info) {
	cc_bool hasNorm, hasTran;
	int partsIndex;
	int i, j, curIdx, offset;
	
	partsIndex = World_ChunkPack(x >> CHUNK_SHIFT, y >> CHUNK_SHIFT, z >> CHUNK_SHIFT);
	offset  = 0;
	hasNorm = false;
	hasTran = false;

	for (i = 0; i < MapRenderer_1DUsedCount; i++) {
		j = i + ATLAS1D_MAX_ATLASES;
		curIdx = partsIndex + i * World.ChunksCount;

		hasNorm |= SetPartInfo(&Builder_Parts[i], &offset, &MapRenderer_PartsNormal[curIdx]);
		hasTran |= SetPartInfo(&Builder_Parts[j], &offset, &MapRenderer_PartsTranslucent[curIdx]);
	}

	if (hasNorm) {
		info->normalParts      = &MapRenderer_PartsNormal[partsIndex];
	}
	if (hasTran) {
		info->translucentParts = &MapRenderer_PartsTranslucent[partsIndex];
	}
}

void Builder_MakeChunk(struct ChunkInfo* info) {
#ifdef CC_BUILD_TINYSTACK
	static BlockID chunk[EXTCHUNK_SIZE_3]; 
	static cc_uint8 counts[CHUNK_SIZE_3 * FACE_COUNT]; 
#else
	BlockID chunk[EXTCHUNK_SIZE_3]; 
	cc_uint8 counts[CHUNK_SIZE_3 * FACE_COUNT]; 
#endif

#ifdef CC_BUILD_ADVLIGHTING
	int bitFlags[EXTCHUNK_SIZE_3];
#else
	int bitFlags[1];
#endif

	cc_bool allAir, allSolid, onBorder;
	int xMax, yMax, zMax, totalVerts;
	int cIndex, index;
	int x, y, z, xx, yy, zz;
	int x1 = info->centreX - 8, y1 = info->centreY - 8, z1 = info->centreZ - 8;

	Builder_Chunk  = chunk;
	Builder_Counts = counts;
	Builder_BitFlags = bitFlags;
	Builder_PrePrepareChunk();
	
	onBorder = 
		x1 == 0 || y1 == 0 || z1 == 0   || x1 + CHUNK_SIZE >= World.Width ||
		y1 + CHUNK_SIZE >= World.Height || z1 + CHUNK_SIZE >= World.Length;

	if (onBorder) {
		/* less optimal case here */
		Mem_Set(chunk, BLOCK_AIR, EXTCHUNK_SIZE_3 * sizeof(BlockID));
		allSolid = ReadBorderChunkData(x1, y1, z1, &allAir);
	} else {
		allSolid = ReadChunkData(x1, y1, z1, &allAir);
	}

	info->allAir = allAir;
	if (allAir || allSolid) return;
	Lighting.LightHint(x1 - 1, y1 - 1, z1 - 1);

	Mem_Set(counts, 1, CHUNK_SIZE_3 * FACE_COUNT);
	xMax = min(World.Width,  x1 + CHUNK_SIZE);
	yMax = min(World.Height, y1 + CHUNK_SIZE);
	zMax = min(World.Length, z1 + CHUNK_SIZE);

	Builder_ChunkEndX = xMax; Builder_ChunkEndZ = zMax;
	PrepareChunk(x1, y1, z1);

	totalVerts = Builder_TotalVerticesCount();
	if (!totalVerts) return;
	
	OutputChunkPartsMeta(x1, y1, z1, info);
#ifdef OCCLUSION
	if (info.NormalParts != null || info.TranslucentParts != null)
		info.occlusionFlags = (cc_uint8)ComputeOcclusion();
#endif

#ifndef CC_BUILD_GL11
	/* add an extra element to fix crashing on some GPUs */
	info->vb = Gfx_CreateVb(VERTEX_FORMAT_TEXTURED, totalVerts + 1);
	Builder_Vertices = (struct VertexTextured*)Gfx_LockVb(info->vb,
													VERTEX_FORMAT_TEXTURED, totalVerts + 1);
#else
	/* NOTE: Relies on assumption vb is ignored by GL11 Gfx_LockVb implementation */
	Builder_Vertices = (struct VertexTextured*)Gfx_LockVb(0, 
													VERTEX_FORMAT_TEXTURED, totalVerts + 1);
#endif
	Builder_PostPrepareChunk();
	/* now render the chunk */

	for (y = y1, yy = 0; y < yMax; y++, yy++) {
		for (z = z1, zz = 0; z < zMax; z++, zz++) {
			cIndex = Builder_PackChunk(0, yy, zz);

			for (x = x1, xx = 0; x < xMax; x++, xx++, cIndex++) {
				Builder_Block = chunk[cIndex];
				if (Blocks.Draw[Builder_Block] == DRAW_GAS) continue;

				index = Builder_PackCount(xx, yy, zz);
				Builder_ChunkIndex = cIndex;
				Builder_RenderBlock(index, x, y, z);
			}
		}
	}

#ifdef CC_BUILD_GL11
	cIndex = World_ChunkPack(x1 >> CHUNK_SHIFT, y1 >> CHUNK_SHIFT, z1 >> CHUNK_SHIFT);

	for (index = 0; index < MapRenderer_1DUsedCount; index++) {
		int curIdx = cIndex + index * World.ChunksCount;

		BuildPartVbs(&MapRenderer_PartsNormal[curIdx]);
		BuildPartVbs(&MapRenderer_PartsTranslucent[curIdx]);
	}
#else
	Gfx_UnlockVb(info->vb);
#endif
}

static cc_bool Builder_OccludedLiquid(int chunkIndex) {
	chunkIndex += EXTCHUNK_SIZE_2; /* Checking y above */
	return
		Blocks.FullOpaque[Builder_Chunk[chunkIndex]]
		&& Blocks.Draw[Builder_Chunk[chunkIndex - EXTCHUNK_SIZE]] != DRAW_GAS
		&& Blocks.Draw[Builder_Chunk[chunkIndex - 1]] != DRAW_GAS
		&& Blocks.Draw[Builder_Chunk[chunkIndex + 1]] != DRAW_GAS
		&& Blocks.Draw[Builder_Chunk[chunkIndex + EXTCHUNK_SIZE]] != DRAW_GAS;
}

static void DefaultPrePrepateChunk(void) {
	Mem_Set(Builder_Parts, 0, sizeof(Builder_Parts));
}

static void DefaultPostStretchChunk(void) {
	int i, j, offset;
	offset = 0;
	for (i = 0; i < ATLAS1D_MAX_ATLASES; i++) {
		j = i + ATLAS1D_MAX_ATLASES;

		offset = Builder1DPart_CalcOffsets(&Builder_Parts[i], offset);
		offset = Builder1DPart_CalcOffsets(&Builder_Parts[j], offset);
	}
}

static RNGState spriteRng;
static void Builder_DrawSprite(int x, int y, int z) {
	struct Builder1DPart* part;
	struct VertexTextured* v;
	cc_uint8 offsetType;
	cc_bool bright;
	PackedCol color;
	TextureLoc loc;
	float v1, v2;

	float X, Y, Z;
	float valX, valY, valZ;
	float x1,y1,z1, x2,y2,z2;
	
	X  = (float)x; Y = (float)y; Z = (float)z;
	x1 = X + 2.50f/16.0f; y1 = Y;        z1 = Z + 2.50f/16.0f;
	x2 = X + 13.5f/16.0f; y2 = Y + 1.0f; z2 = Z + 13.5f/16.0f;

#define s_u1 0.0f
#define s_u2 UV2_Scale
	loc = Block_Tex(Builder_Block, FACE_XMAX);
	v1  = Atlas1D_RowId(loc) * Atlas1D.InvTileSize;
	v2  = v1 + Atlas1D.InvTileSize * UV2_Scale;

	offsetType = Blocks.SpriteOffset[Builder_Block];
	if (offsetType >= 6 && offsetType <= 7) {
		Random_Seed(&spriteRng, (x + 1217 * z) & 0x7fffffff);
		valX = Random_Range(&spriteRng, -3, 3 + 1) / 16.0f;
		valY = Random_Range(&spriteRng, 0,  3 + 1) / 16.0f;
		valZ = Random_Range(&spriteRng, -3, 3 + 1) / 16.0f;

		x1 += valX - 1.7f/16.0f; x2 += valX + 1.7f/16.0f;
		z1 += valZ - 1.7f/16.0f; z2 += valZ + 1.7f/16.0f;
		if (offsetType == 7) { y1 -= valY; y2 -= valY; }
	}
	
	bright = Blocks.Brightness[Builder_Block];
	part   = &Builder_Parts[Atlas1D_Index(loc)];
	color  = bright ? PACKEDCOL_WHITE : Lighting.Color_Sprite_Fast(x, y, z);
	Block_Tint(color, Builder_Block);

	/* Draw Z axis */
	v = &Builder_Vertices[part->sOffset];
	v->x = x1; v->y = y1; v->z = z1; v->Col = color; v->U = s_u2; v->V = v2; v++;
	v->x = x1; v->y = y2; v->z = z1; v->Col = color; v->U = s_u2; v->V = v1; v++;
	v->x = x2; v->y = y2; v->z = z2; v->Col = color; v->U = s_u1; v->V = v1; v++;
	v->x = x2; v->y = y1; v->z = z2; v->Col = color; v->U = s_u1; v->V = v2; v++;

	/* Draw Z axis mirrored */
	v -= 4; v += part->sCount >> 2;
	v->x = x2; v->y = y1; v->z = z2; v->Col = color; v->U = s_u2; v->V = v2; v++;
	v->x = x2; v->y = y2; v->z = z2; v->Col = color; v->U = s_u2; v->V = v1; v++;
	v->x = x1; v->y = y2; v->z = z1; v->Col = color; v->U = s_u1; v->V = v1; v++;
	v->x = x1; v->y = y1; v->z = z1; v->Col = color; v->U = s_u1; v->V = v2; v++;

	/* Draw X axis */
	v -= 4; v += part->sCount >> 2;
	v->x = x1; v->y = y1; v->z = z2; v->Col = color; v->U = s_u2; v->V = v2; v++;
	v->x = x1; v->y = y2; v->z = z2; v->Col = color; v->U = s_u2; v->V = v1; v++;
	v->x = x2; v->y = y2; v->z = z1; v->Col = color; v->U = s_u1; v->V = v1; v++;
	v->x = x2; v->y = y1; v->z = z1; v->Col = color; v->U = s_u1; v->V = v2; v++;

	/* Draw X axis mirrored */
	v -= 4; v += part->sCount >> 2;
	v->x = x2; v->y = y1; v->z = z1; v->Col = color; v->U = s_u2; v->V = v2; v++;
	v->x = x2; v->y = y2; v->z = z1; v->Col = color; v->U = s_u2; v->V = v1; v++;
	v->x = x1; v->y = y2; v->z = z2; v->Col = color; v->U = s_u1; v->V = v1; v++;
	v->x = x1; v->y = y1; v->z = z2; v->Col = color; v->U = s_u1; v->V = v2; v++;

	part->sOffset += 4;
}


/*########################################################################################################################*
*--------------------------------------------------Normal mesh builder----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol Normal_LightColor(int x, int y, int z, Face face, BlockID block) {
	int offset = (Blocks.LightOffset[block] >> face) & 1;

	switch (face) {
	case FACE_XMIN:
		return x < offset                ? Env.SunXSide : Lighting.Color_XSide_Fast(x - offset, y, z);
	case FACE_XMAX:
		return x > (World.MaxX - offset) ? Env.SunXSide : Lighting.Color_XSide_Fast(x + offset, y, z);
	case FACE_ZMIN:
		return z < offset                ? Env.SunZSide : Lighting.Color_ZSide_Fast(x, y, z - offset);
	case FACE_ZMAX:
		return z > (World.MaxZ - offset) ? Env.SunZSide : Lighting.Color_ZSide_Fast(x, y, z + offset);

	case FACE_YMIN:
		return Lighting.Color_YMin_Fast(x, y - offset, z);		
	case FACE_YMAX:
		return Lighting.Color_YMax_Fast(x, y + offset, z);
	}
	return 0; /* should never happen */
}

static cc_bool Normal_CanStretch(BlockID initial, int chunkIndex, int x, int y, int z, Face face) {
	BlockID cur = Builder_Chunk[chunkIndex];

	if (cur != initial || Block_IsFaceHidden(cur, Builder_Chunk[chunkIndex + Builder_Offsets[face]], face)) return false;
	if (Builder_FullBright) return true;

	return Normal_LightColor(Builder_X, Builder_Y, Builder_Z, face, initial) == Normal_LightColor(x, y, z, face, cur);
}

static int NormalBuilder_StretchXLiquid(int countIndex, int x, int y, int z, int chunkIndex, BlockID block) {
	int count = 1; cc_bool stretchTile;
	if (Builder_OccludedLiquid(chunkIndex)) return 0;
	
	x++;
	chunkIndex++;
	countIndex += FACE_COUNT;
	stretchTile = (Blocks.CanStretch[block] & (1 << FACE_YMAX)) != 0;

	while (x < Builder_ChunkEndX && stretchTile && Normal_CanStretch(block, chunkIndex, x, y, z, FACE_YMAX) && !Builder_OccludedLiquid(chunkIndex)) {
		Builder_Counts[countIndex] = 0;
		count++;
		x++;
		chunkIndex++;
		countIndex += FACE_COUNT;
	}
	AddVertices(block, FACE_YMAX);
	return count;
}

static int NormalBuilder_StretchX(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face) {
	int count = 1; cc_bool stretchTile;
	x++;
	chunkIndex++;
	countIndex += FACE_COUNT;
	stretchTile = (Blocks.CanStretch[block] & (1 << face)) != 0;

	while (x < Builder_ChunkEndX && stretchTile && Normal_CanStretch(block, chunkIndex, x, y, z, face)) {
		Builder_Counts[countIndex] = 0;
		count++;
		x++;
		chunkIndex++;
		countIndex += FACE_COUNT;
	}
	AddVertices(block, face);
	return count;
}

static int NormalBuilder_StretchZ(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face) {
	int count = 1; cc_bool stretchTile;
	z++;
	chunkIndex += EXTCHUNK_SIZE;
	countIndex += CHUNK_SIZE * FACE_COUNT;
	stretchTile = (Blocks.CanStretch[block] & (1 << face)) != 0;

	while (z < Builder_ChunkEndZ && stretchTile && Normal_CanStretch(block, chunkIndex, x, y, z, face)) {
		Builder_Counts[countIndex] = 0;
		count++;
		z++;
		chunkIndex += EXTCHUNK_SIZE;
		countIndex += CHUNK_SIZE * FACE_COUNT;
	}
	AddVertices(block, face);
	return count;
}

static void NormalBuilder_RenderBlock(int index, int x, int y, int z) {	
	/* counters */
	int count_XMin, count_XMax, count_ZMin;
	int count_ZMax, count_YMin, count_YMax;

	/* block state */
	Vec3 min, max;
	int baseOffset, lightFlags;
	cc_bool fullBright;

	/* per-face state */
	struct Builder1DPart* part;
	TextureLoc loc;
	PackedCol col;
	int offset;

	if (Blocks.Draw[Builder_Block] == DRAW_SPRITE) {
		Builder_DrawSprite(x, y, z); return;
	}

	count_XMin = Builder_Counts[index + FACE_XMIN];
	count_XMax = Builder_Counts[index + FACE_XMAX];
	count_ZMin = Builder_Counts[index + FACE_ZMIN];
	count_ZMax = Builder_Counts[index + FACE_ZMAX];
	count_YMin = Builder_Counts[index + FACE_YMIN];
	count_YMax = Builder_Counts[index + FACE_YMAX];

	if (!count_XMin && !count_XMax && !count_ZMin &&
		!count_ZMax && !count_YMin && !count_YMax) return;

	fullBright = Blocks.Brightness[Builder_Block];
	baseOffset = (Blocks.Draw[Builder_Block] == DRAW_TRANSLUCENT) * ATLAS1D_MAX_ATLASES;
	lightFlags = Blocks.LightOffset[Builder_Block];

	Drawer.MinBB = Blocks.MinBB[Builder_Block]; Drawer.MinBB.y = 1.0f - Drawer.MinBB.y;
	Drawer.MaxBB = Blocks.MaxBB[Builder_Block]; Drawer.MaxBB.y = 1.0f - Drawer.MaxBB.y;

	min = Blocks.RenderMinBB[Builder_Block]; max = Blocks.RenderMaxBB[Builder_Block];
	Drawer.X1 = x + min.x; Drawer.Y1 = y + min.y; Drawer.Z1 = z + min.z;
	Drawer.X2 = x + max.x; Drawer.Y2 = y + max.y; Drawer.Z2 = z + max.z;

	Drawer.Tinted  = Blocks.Tinted[Builder_Block];
	Drawer.TintCol = Blocks.FogCol[Builder_Block];

	if (count_XMin) {
		loc    = Block_Tex(Builder_Block, FACE_XMIN);
		offset = (lightFlags >> FACE_XMIN) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(loc)];

		col = fullBright ? PACKEDCOL_WHITE :
			x >= offset ? Lighting.Color_XSide_Fast(x - offset, y, z) : Env.SunXSide;
		Drawer_XMin(count_XMin, col, loc, &part->faces.vertices[FACE_XMIN]);
	}

	if (count_XMax) {
		loc    = Block_Tex(Builder_Block, FACE_XMAX);
		offset = (lightFlags >> FACE_XMAX) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(loc)];

		col = fullBright ? PACKEDCOL_WHITE :
			x <= (World.MaxX - offset) ? Lighting.Color_XSide_Fast(x + offset, y, z) : Env.SunXSide;
		Drawer_XMax(count_XMax, col, loc, &part->faces.vertices[FACE_XMAX]);
	}

	if (count_ZMin) {
		loc    = Block_Tex(Builder_Block, FACE_ZMIN);
		offset = (lightFlags >> FACE_ZMIN) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(loc)];

		col = fullBright ? PACKEDCOL_WHITE :
			z >= offset ? Lighting.Color_ZSide_Fast(x, y, z - offset) : Env.SunZSide;
		Drawer_ZMin(count_ZMin, col, loc, &part->faces.vertices[FACE_ZMIN]);
	}

	if (count_ZMax) {
		loc    = Block_Tex(Builder_Block, FACE_ZMAX);
		offset = (lightFlags >> FACE_ZMAX) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(loc)];

		col = fullBright ? PACKEDCOL_WHITE :
			z <= (World.MaxZ - offset) ? Lighting.Color_ZSide_Fast(x, y, z + offset) : Env.SunZSide;
		Drawer_ZMax(count_ZMax, col, loc, &part->faces.vertices[FACE_ZMAX]);
	}

	if (count_YMin) {
		loc    = Block_Tex(Builder_Block, FACE_YMIN);
		offset = (lightFlags >> FACE_YMIN) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(loc)];

		col = fullBright ? PACKEDCOL_WHITE : Lighting.Color_YMin_Fast(x, y - offset, z);
		Drawer_YMin(count_YMin, col, loc, &part->faces.vertices[FACE_YMIN]);
	}

	if (count_YMax) {
		loc    = Block_Tex(Builder_Block, FACE_YMAX);
		offset = (lightFlags >> FACE_YMAX) & 1;
		part   = &Builder_Parts[baseOffset + Atlas1D_Index(loc)];

		col = fullBright ? PACKEDCOL_WHITE : Lighting.Color_YMax_Fast(x, y + offset, z);
		Drawer_YMax(count_YMax, col, loc, &part->faces.vertices[FACE_YMAX]);
	}
}

static void Builder_SetDefault(void) {
	Builder_StretchXLiquid = NULL;
	Builder_StretchX       = NULL;
	Builder_StretchZ       = NULL;
	Builder_RenderBlock    = NULL;

	Builder_PrePrepareChunk  = DefaultPrePrepateChunk;
	Builder_PostPrepareChunk = DefaultPostStretchChunk;
}

static void NormalBuilder_SetActive(void) {
	Builder_SetDefault();
	Builder_StretchXLiquid = NormalBuilder_StretchXLiquid;
	Builder_StretchX       = NormalBuilder_StretchX;
	Builder_StretchZ       = NormalBuilder_StretchZ;
	Builder_RenderBlock    = NormalBuilder_RenderBlock;
}


/*########################################################################################################################*
*-------------------------------------------------Advanced mesh builder---------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_ADVLIGHTING
static Vec3 adv_minBB, adv_maxBB;
static int adv_initBitFlags, adv_baseOffset;
static int* adv_bitFlags;
static float adv_x1, adv_y1, adv_z1, adv_x2, adv_y2, adv_z2;
static PackedCol adv_lerp[5], adv_lerpX[5], adv_lerpZ[5], adv_lerpY[5];
static cc_bool adv_tinted;

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

/* Bit-or the Adv_Lit flags with these to set the appropriate light values */
#define LIT_M1 (1 << 0)
#define LIT_CC (1 << 1)
#define LIT_P1 (1 << 2)
/* Returns a 3 bit value where */
/* - bit 0 set: Y-1 is in light */
/* - bit 1 set: Y   is in light */
/* - bit 2 set: Y+1 is in light */
static int Adv_Lit(int x, int y, int z, int cIndex) {
	int flags, offset, lightFlags;
	BlockID block;
	if (y < 0 || y >= World.Height) return LIT_M1 | LIT_CC | LIT_P1; /* all faces lit */

	/* TODO: check sides height (if sides > edges), check if edge block casts a shadow */
	if (!World_ContainsXZ(x, z)) {
		return y >= Builder_EdgeLevel ? LIT_M1 | LIT_CC | LIT_P1 : y == (Builder_EdgeLevel - 1) ? LIT_CC | LIT_P1 : 0;
	}

	flags = 0;
	block = Builder_Chunk[cIndex];
	lightFlags = Blocks.LightOffset[block];

	/* TODO using LIGHT_FLAG_SHADES_FROM_BELOW is wrong here, */
	/*  but still produces less broken results than YMIN/YMAX */

	/* Use fact Light(Y.YMin) == Light((Y-1).YMax) */
	offset = (lightFlags >> LIGHT_FLAG_SHADES_FROM_BELOW) & 1;
	flags |= Lighting.IsLit_Fast(x, y - offset, z) ? LIT_M1 : 0;

	/* Light is same for all the horizontal faces */
	flags |= Lighting.IsLit_Fast(x, y, z) ? LIT_CC : 0;

	/* Use fact Light((Y+1).YMin) == Light(Y.YMax) */
	offset = (lightFlags >> LIGHT_FLAG_SHADES_FROM_BELOW) & 1;
	flags |= Lighting.IsLit_Fast(x, (y + 1) - offset, z) ? LIT_P1 : 0;

	/* If a block is fullbright, it should also look as if that spot is lit */
	if (Blocks.Brightness[Builder_Chunk[cIndex - 324]]) flags |= LIT_M1;
	if (Blocks.Brightness[block])                       flags |= LIT_CC;
	if (Blocks.Brightness[Builder_Chunk[cIndex + 324]]) flags |= LIT_P1;
	
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

static int adv_masks[FACE_COUNT] = {
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


static cc_bool Adv_CanStretch(BlockID initial, int chunkIndex, int x, int y, int z, Face face) {
	BlockID cur = Builder_Chunk[chunkIndex];
	adv_bitFlags[chunkIndex] = Adv_ComputeLightFlags(x, y, z, chunkIndex);

	return cur == initial
		&& !Block_IsFaceHidden(cur, Builder_Chunk[chunkIndex + Builder_Offsets[face]], face)
		&& (adv_initBitFlags == adv_bitFlags[chunkIndex]
		/* Check that this face is either fully bright or fully in shadow */
		&& (adv_initBitFlags == 0 || (adv_initBitFlags & adv_masks[face]) == adv_masks[face]));
}

static int Adv_StretchXLiquid(int countIndex, int x, int y, int z, int chunkIndex, BlockID block) {
	int count = 1; cc_bool stretchTile;
	if (Builder_OccludedLiquid(chunkIndex)) return 0;
	adv_initBitFlags = Adv_ComputeLightFlags(x, y, z, chunkIndex);
	adv_bitFlags[chunkIndex] = adv_initBitFlags;

	x++;
	chunkIndex++;
	countIndex += FACE_COUNT;
	stretchTile = (Blocks.CanStretch[block] & (1 << FACE_YMAX)) != 0;

	while (x < Builder_ChunkEndX && stretchTile && Adv_CanStretch(block, chunkIndex, x, y, z, FACE_YMAX) && !Builder_OccludedLiquid(chunkIndex)) {
		Builder_Counts[countIndex] = 0;
		count++;
		x++;
		chunkIndex++;
		countIndex += FACE_COUNT;
	}
	AddVertices(block, FACE_YMAX);
	return count;
}

static int Adv_StretchX(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face) {
	int count = 1; cc_bool stretchTile;
	adv_initBitFlags = Adv_ComputeLightFlags(x, y, z, chunkIndex);
	adv_bitFlags[chunkIndex] = adv_initBitFlags;
	
	x++;
	chunkIndex++;
	countIndex += FACE_COUNT;
	stretchTile = (Blocks.CanStretch[block] & (1 << face)) != 0;

	while (x < Builder_ChunkEndX && stretchTile && Adv_CanStretch(block, chunkIndex, x, y, z, face)) {
		Builder_Counts[countIndex] = 0;
		count++;
		x++;
		chunkIndex++;
		countIndex += FACE_COUNT;
	}
	AddVertices(block, face);
	return count;
}

static int Adv_StretchZ(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face) {
	int count = 1; cc_bool stretchTile;
	adv_initBitFlags = Adv_ComputeLightFlags(x, y, z, chunkIndex);
	adv_bitFlags[chunkIndex] = adv_initBitFlags;

	z++;
	chunkIndex += EXTCHUNK_SIZE;
	countIndex += CHUNK_SIZE * FACE_COUNT;
	stretchTile = (Blocks.CanStretch[block] & (1 << face)) != 0;

	while (z < Builder_ChunkEndZ && stretchTile && Adv_CanStretch(block, chunkIndex, x, y, z, face)) {
		Builder_Counts[countIndex] = 0;
		count++;
		z++;
		chunkIndex += EXTCHUNK_SIZE;
		countIndex += CHUNK_SIZE * FACE_COUNT;
	}
	AddVertices(block, face);
	return count;
}


#define Adv_CountBits(F, a, b, c, d) (((F >> a) & 1) + ((F >> b) & 1) + ((F >> c) & 1) + ((F >> d) & 1))

static void Adv_DrawXMin(int count) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_XMIN);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = adv_minBB.z, u2 = (count - 1) + adv_maxBB.z * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_minBB.y * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aY0_Z0 = Adv_CountBits(F, xM1_yM1_zM1, xM1_yCC_zM1, xM1_yM1_zCC, xM1_yCC_zCC);
	int aY0_Z1 = Adv_CountBits(F, xM1_yM1_zP1, xM1_yCC_zP1, xM1_yM1_zCC, xM1_yCC_zCC);
	int aY1_Z0 = Adv_CountBits(F, xM1_yP1_zM1, xM1_yCC_zM1, xM1_yP1_zCC, xM1_yCC_zCC);
	int aY1_Z1 = Adv_CountBits(F, xM1_yP1_zP1, xM1_yCC_zP1, xM1_yP1_zCC, xM1_yCC_zCC);

	PackedCol tint, white = PACKEDCOL_WHITE;
	PackedCol col0_0 = Builder_FullBright ? white : adv_lerpX[aY0_Z0], col1_0 = Builder_FullBright ? white : adv_lerpX[aY1_Z0];
	PackedCol col1_1 = Builder_FullBright ? white : adv_lerpX[aY1_Z1], col0_1 = Builder_FullBright ? white : adv_lerpX[aY0_Z1];
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_XMIN];
	v.x = adv_x1;
	if (aY0_Z0 + aY1_Z1 > aY0_Z1 + aY1_Z0) {
		v.y = adv_y2; v.z = adv_z1;               v.U = u1; v.V = v1; v.Col = col1_0; *vertices++ = v;
		v.y = adv_y1;                                       v.V = v2; v.Col = col0_0; *vertices++ = v;
		              v.z = adv_z2 + (count - 1); v.U = u2;           v.Col = col0_1; *vertices++ = v;
		v.y = adv_y2;                                       v.V = v1; v.Col = col1_1; *vertices++ = v;
	} else {
		v.y = adv_y2; v.z = adv_z2 + (count - 1); v.U = u2; v.V = v1; v.Col = col1_1; *vertices++ = v;
		              v.z = adv_z1;               v.U = u1;           v.Col = col1_0; *vertices++ = v;
		v.y = adv_y1;                                       v.V = v2; v.Col = col0_0; *vertices++ = v;
		              v.z = adv_z2 + (count - 1); v.U = u2;           v.Col = col0_1; *vertices++ = v;
	}
	part->faces.vertices[FACE_XMIN] = vertices;
}

static void Adv_DrawXMax(int count) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_XMAX);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = (count - adv_minBB.z), u2 = (1 - adv_maxBB.z) * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_minBB.y * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aY0_Z0 = Adv_CountBits(F, xP1_yM1_zM1, xP1_yCC_zM1, xP1_yM1_zCC, xP1_yCC_zCC);
	int aY0_Z1 = Adv_CountBits(F, xP1_yM1_zP1, xP1_yCC_zP1, xP1_yM1_zCC, xP1_yCC_zCC);
	int aY1_Z0 = Adv_CountBits(F, xP1_yP1_zM1, xP1_yCC_zM1, xP1_yP1_zCC, xP1_yCC_zCC);
	int aY1_Z1 = Adv_CountBits(F, xP1_yP1_zP1, xP1_yCC_zP1, xP1_yP1_zCC, xP1_yCC_zCC);

	PackedCol tint, white = PACKEDCOL_WHITE;
	PackedCol col0_0 = Builder_FullBright ? white : adv_lerpX[aY0_Z0], col1_0 = Builder_FullBright ? white : adv_lerpX[aY1_Z0];
	PackedCol col1_1 = Builder_FullBright ? white : adv_lerpX[aY1_Z1], col0_1 = Builder_FullBright ? white : adv_lerpX[aY0_Z1];
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_XMAX];
	v.x = adv_x2;
	if (aY0_Z0 + aY1_Z1 > aY0_Z1 + aY1_Z0) {
		v.y = adv_y2; v.z = adv_z1;               v.U = u1; v.V = v1; v.Col = col1_0; *vertices++ = v;
		              v.z = adv_z2 + (count - 1); v.U = u2;           v.Col = col1_1; *vertices++ = v;
		v.y = adv_y1;                                       v.V = v2; v.Col = col0_1; *vertices++ = v;
		              v.z = adv_z1;               v.U = u1;           v.Col = col0_0; *vertices++ = v;
	} else {
		v.y = adv_y2; v.z = adv_z2 + (count - 1); v.U = u2; v.V = v1; v.Col = col1_1; *vertices++ = v;
		v.y = adv_y1;                                       v.V = v2; v.Col = col0_1; *vertices++ = v;
		              v.z = adv_z1;               v.U = u1;           v.Col = col0_0; *vertices++ = v;
		v.y = adv_y2;                                       v.V = v1; v.Col = col1_0; *vertices++ = v;
	}
	part->faces.vertices[FACE_XMAX] = vertices;
}

static void Adv_DrawZMin(int count) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_ZMIN);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = (count - adv_minBB.x), u2 = (1 - adv_maxBB.x) * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_minBB.y * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aX0_Y0 = Adv_CountBits(F, xM1_yM1_zM1, xM1_yCC_zM1, xCC_yM1_zM1, xCC_yCC_zM1);
	int aX0_Y1 = Adv_CountBits(F, xM1_yP1_zM1, xM1_yCC_zM1, xCC_yP1_zM1, xCC_yCC_zM1);
	int aX1_Y0 = Adv_CountBits(F, xP1_yM1_zM1, xP1_yCC_zM1, xCC_yM1_zM1, xCC_yCC_zM1);
	int aX1_Y1 = Adv_CountBits(F, xP1_yP1_zM1, xP1_yCC_zM1, xCC_yP1_zM1, xCC_yCC_zM1);

	PackedCol tint, white = PACKEDCOL_WHITE;
	PackedCol col0_0 = Builder_FullBright ? white : adv_lerpZ[aX0_Y0], col1_0 = Builder_FullBright ? white : adv_lerpZ[aX1_Y0];
	PackedCol col1_1 = Builder_FullBright ? white : adv_lerpZ[aX1_Y1], col0_1 = Builder_FullBright ? white : adv_lerpZ[aX0_Y1];
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_ZMIN];
	v.z = adv_z1;
	if (aX1_Y1 + aX0_Y0 > aX0_Y1 + aX1_Y0) {
		v.x = adv_x2 + (count - 1); v.y = adv_y1; v.U = u2; v.V = v2; v.Col = col1_0; *vertices++ = v;
		v.x = adv_x1;                             v.U = u1;           v.Col = col0_0; *vertices++ = v;
		                            v.y = adv_y2;           v.V = v1; v.Col = col0_1; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_1; *vertices++ = v;
	} else {
		v.x = adv_x1;               v.y = adv_y1; v.U = u1; v.V = v2; v.Col = col0_0; *vertices++ = v;
		                            v.y = adv_y2;           v.V = v1; v.Col = col0_1; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_1; *vertices++ = v;
		                            v.y = adv_y1;           v.V = v2; v.Col = col1_0; *vertices++ = v;
	}
	part->faces.vertices[FACE_ZMIN] = vertices;
}

static void Adv_DrawZMax(int count) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_ZMAX);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = adv_minBB.x, u2 = (count - 1) + adv_maxBB.x * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_minBB.y * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aX0_Y0 = Adv_CountBits(F, xM1_yM1_zP1, xM1_yCC_zP1, xCC_yM1_zP1, xCC_yCC_zP1);
	int aX1_Y0 = Adv_CountBits(F, xP1_yM1_zP1, xP1_yCC_zP1, xCC_yM1_zP1, xCC_yCC_zP1);
	int aX0_Y1 = Adv_CountBits(F, xM1_yP1_zP1, xM1_yCC_zP1, xCC_yP1_zP1, xCC_yCC_zP1);
	int aX1_Y1 = Adv_CountBits(F, xP1_yP1_zP1, xP1_yCC_zP1, xCC_yP1_zP1, xCC_yCC_zP1);

	PackedCol tint, white = PACKEDCOL_WHITE;
	PackedCol col1_1 = Builder_FullBright ? white : adv_lerpZ[aX1_Y1], col1_0 = Builder_FullBright ? white : adv_lerpZ[aX1_Y0];
	PackedCol col0_0 = Builder_FullBright ? white : adv_lerpZ[aX0_Y0], col0_1 = Builder_FullBright ? white : adv_lerpZ[aX0_Y1];
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_ZMAX];
	v.z = adv_z2;
	if (aX1_Y1 + aX0_Y0 > aX0_Y1 + aX1_Y0) {
		v.x = adv_x1;               v.y = adv_y2; v.U = u1; v.V = v1; v.Col = col0_1; *vertices++ = v;
		                            v.y = adv_y1;           v.V = v2; v.Col = col0_0; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_0; *vertices++ = v;
		                            v.y = adv_y2;           v.V = v1; v.Col = col1_1; *vertices++ = v;
	} else {
		v.x = adv_x2 + (count - 1); v.y = adv_y2; v.U = u2; v.V = v1; v.Col = col1_1; *vertices++ = v;
		v.x = adv_x1;                             v.U = u1;           v.Col = col0_1; *vertices++ = v;
		                            v.y = adv_y1;           v.V = v2; v.Col = col0_0; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_0; *vertices++ = v;
	}
	part->faces.vertices[FACE_ZMAX] = vertices;
}

static void Adv_DrawYMin(int count) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_YMIN);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = adv_minBB.x, u2 = (count - 1) + adv_maxBB.x * UV2_Scale;
	float v1 = vOrigin + adv_minBB.z * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_maxBB.z * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aX0_Z0 = Adv_CountBits(F, xM1_yM1_zM1, xM1_yM1_zCC, xCC_yM1_zM1, xCC_yM1_zCC);
	int aX1_Z0 = Adv_CountBits(F, xP1_yM1_zM1, xP1_yM1_zCC, xCC_yM1_zM1, xCC_yM1_zCC);
	int aX0_Z1 = Adv_CountBits(F, xM1_yM1_zP1, xM1_yM1_zCC, xCC_yM1_zP1, xCC_yM1_zCC);
	int aX1_Z1 = Adv_CountBits(F, xP1_yM1_zP1, xP1_yM1_zCC, xCC_yM1_zP1, xCC_yM1_zCC);

	PackedCol tint, white = PACKEDCOL_WHITE;
	PackedCol col0_1 = Builder_FullBright ? white : adv_lerpY[aX0_Z1], col1_1 = Builder_FullBright ? white : adv_lerpY[aX1_Z1];
	PackedCol col1_0 = Builder_FullBright ? white : adv_lerpY[aX1_Z0], col0_0 = Builder_FullBright ? white : adv_lerpY[aX0_Z0];
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_YMIN];
	v.y = adv_y1;
	if (aX0_Z1 + aX1_Z0 > aX0_Z0 + aX1_Z1) {
		v.x = adv_x2 + (count - 1); v.z = adv_z2; v.U = u2; v.V = v2; v.Col = col1_1; *vertices++ = v;
		v.x = adv_x1;                             v.U = u1;           v.Col = col0_1; *vertices++ = v;
		                            v.z = adv_z1;           v.V = v1; v.Col = col0_0; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_0; *vertices++ = v;
	} else {
		v.x = adv_x1;               v.z = adv_z2; v.U = u1; v.V = v2; v.Col = col0_1; *vertices++ = v;
		                            v.z = adv_z1;           v.V = v1; v.Col = col0_0; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_0; *vertices++ = v;
		                            v.z = adv_z2;           v.V = v2; v.Col = col1_1; *vertices++ = v;
	}
	part->faces.vertices[FACE_YMIN] = vertices;
}

static void Adv_DrawYMax(int count) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_YMAX);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = adv_minBB.x, u2 = (count - 1) + adv_maxBB.x * UV2_Scale;
	float v1 = vOrigin + adv_minBB.z * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_maxBB.z * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	int F = adv_bitFlags[Builder_ChunkIndex];
	int aX0_Z0 = Adv_CountBits(F, xM1_yP1_zM1, xM1_yP1_zCC, xCC_yP1_zM1, xCC_yP1_zCC);
	int aX1_Z0 = Adv_CountBits(F, xP1_yP1_zM1, xP1_yP1_zCC, xCC_yP1_zM1, xCC_yP1_zCC);
	int aX0_Z1 = Adv_CountBits(F, xM1_yP1_zP1, xM1_yP1_zCC, xCC_yP1_zP1, xCC_yP1_zCC);
	int aX1_Z1 = Adv_CountBits(F, xP1_yP1_zP1, xP1_yP1_zCC, xCC_yP1_zP1, xCC_yP1_zCC);

	PackedCol tint, white = PACKEDCOL_WHITE;
	PackedCol col0_0 = Builder_FullBright ? white : adv_lerp[aX0_Z0], col1_0 = Builder_FullBright ? white : adv_lerp[aX1_Z0];
	PackedCol col1_1 = Builder_FullBright ? white : adv_lerp[aX1_Z1], col0_1 = Builder_FullBright ? white : adv_lerp[aX0_Z1];
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_YMAX];
	v.y = adv_y2;
	if (aX0_Z0 + aX1_Z1 > aX0_Z1 + aX1_Z0) {
		v.x = adv_x2 + (count - 1); v.z = adv_z1; v.U = u2; v.V = v1; v.Col = col1_0; *vertices++ = v;
		v.x = adv_x1;                             v.U = u1;           v.Col = col0_0; *vertices++ = v;
		                            v.z = adv_z2;           v.V = v2; v.Col = col0_1; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_1; *vertices++ = v;
	} else {
		v.x = adv_x1;               v.z = adv_z1; v.U = u1; v.V = v1; v.Col = col0_0; *vertices++ = v;
		                            v.z = adv_z2;           v.V = v2; v.Col = col0_1; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_1; *vertices++ = v;
		                            v.z = adv_z1;           v.V = v1; v.Col = col1_0; *vertices++ = v;
	}
	part->faces.vertices[FACE_YMAX] = vertices;
}

static void Adv_RenderBlock(int index, int x, int y, int z) {
	Vec3 min, max;
	int count_XMin, count_XMax, count_ZMin;
	int count_ZMax, count_YMin, count_YMax;

	if (Blocks.Draw[Builder_Block] == DRAW_SPRITE) {
		Builder_DrawSprite(x, y, z); return;
	}

	count_XMin = Builder_Counts[index + FACE_XMIN];
	count_XMax = Builder_Counts[index + FACE_XMAX];
	count_ZMin = Builder_Counts[index + FACE_ZMIN];
	count_ZMax = Builder_Counts[index + FACE_ZMAX];
	count_YMin = Builder_Counts[index + FACE_YMIN];
	count_YMax = Builder_Counts[index + FACE_YMAX];

	if (!count_XMin && !count_XMax && !count_ZMin &&
		!count_ZMax && !count_YMin && !count_YMax) return;

	Builder_FullBright = Blocks.Brightness[Builder_Block];
	adv_baseOffset = (Blocks.Draw[Builder_Block] == DRAW_TRANSLUCENT) * ATLAS1D_MAX_ATLASES;
	adv_tinted     = Blocks.Tinted[Builder_Block];

	min = Blocks.RenderMinBB[Builder_Block]; max = Blocks.RenderMaxBB[Builder_Block];
	adv_x1 = x + min.x; adv_y1 = y + min.y; adv_z1 = z + min.z;
	adv_x2 = x + max.x; adv_y2 = y + max.y; adv_z2 = z + max.z;

	adv_minBB = Blocks.MinBB[Builder_Block]; adv_maxBB = Blocks.MaxBB[Builder_Block];
	adv_minBB.y = 1.0f - adv_minBB.y; adv_maxBB.y = 1.0f - adv_maxBB.y;

	if (count_XMin) Adv_DrawXMin(count_XMin);
	if (count_XMax) Adv_DrawXMax(count_XMax);
	if (count_ZMin) Adv_DrawZMin(count_ZMin);
	if (count_ZMax) Adv_DrawZMax(count_ZMax);
	if (count_YMin) Adv_DrawYMin(count_YMin);
	if (count_YMax) Adv_DrawYMax(count_YMax);
}

static void Adv_PrePrepareChunk(void) {
	int i;
	DefaultPrePrepateChunk();
	adv_bitFlags = Builder_BitFlags;

	for (i = 0; i <= 4; i++) {
		adv_lerp[i]  = PackedCol_Lerp(Env.ShadowCol,   Env.SunCol,   i / 4.0f);
		adv_lerpX[i] = PackedCol_Lerp(Env.ShadowXSide, Env.SunXSide, i / 4.0f);
		adv_lerpZ[i] = PackedCol_Lerp(Env.ShadowZSide, Env.SunZSide, i / 4.0f);
		adv_lerpY[i] = PackedCol_Lerp(Env.ShadowYMin,  Env.SunYMin,  i / 4.0f);
	}
}

static void AdvBuilder_SetActive(void) {
	Builder_SetDefault();
	Builder_StretchXLiquid  = Adv_StretchXLiquid;
	Builder_StretchX        = Adv_StretchX;
	Builder_StretchZ        = Adv_StretchZ;
	Builder_RenderBlock     = Adv_RenderBlock;
	Builder_PrePrepareChunk = Adv_PrePrepareChunk;
}
#else
static void AdvBuilder_SetActive(void) { NormalBuilder_SetActive(); }
#endif



/*########################################################################################################################*
*-------------------------------------------------Modern mesh builder-----------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_ADVLIGHTING
/* Fast color averaging wizardy from https://stackoverflow.com/questions/8440631/how-would-you-average-two-32-bit-colors-packed-into-an-integer */
#define AVERAGE(a, b)   ( ((((a) ^ (b)) & 0xfefefefe) >> 1) + ((a) & (b)) )

static cc_bool Modern_IsOccluded(int x, int y, int z) {
	BlockID block = World_SafeGetBlock(x, y, z);
	if (Blocks.Brightness[block] > 0) { return false; }
	/* If the block we're pulling colors from is solid, return a darker version of original and increment how many are like this */
	if (Blocks.FullOpaque[block] || (Blocks.Draw[block] == DRAW_TRANSPARENT && Blocks.BlocksLight[block] && Blocks.LightOffset[block] == 0xFF)) {
		return true;
	}
	return false;
}

static cc_bool Modern_CanStretch(BlockID initial, int chunkIndex, int x, int y, int z, Face face) {
	return false;
}

static int Modern_StretchXLiquid(int countIndex, int x, int y, int z, int chunkIndex, BlockID block) {
	int count = 1;
	if (Builder_OccludedLiquid(chunkIndex)) return 0;
	AddVertices(block, FACE_YMAX);
	return count;
}

static int Modern_StretchX(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face) {
	int count = 1;
	AddVertices(block, face);
	return count;
}

static int Modern_StretchZ(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, Face face) {
	int count = 1;
	AddVertices(block, face);
	return count;
}

static PackedCol Modern_GetColorX(PackedCol orig, int x, int y, int z, int oY, int oZ) {
	cc_bool xOccluded =  Modern_IsOccluded(x, y + oY, z     );
	cc_bool zOccluded =  Modern_IsOccluded(x, y     , z + oZ);
	cc_bool xzOccluded = Modern_IsOccluded(x, y + oY, z + oZ);

	PackedCol CoX = xOccluded ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color_XSide_Fast(x, y + oY, z     );
	PackedCol CoZ = zOccluded ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color_XSide_Fast(x, y     , z + oZ);
	PackedCol CoXoZ = (xzOccluded || (xOccluded && zOccluded)) ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color_XSide_Fast(x, y + oY, z + oZ);

	PackedCol ab = AVERAGE(CoX, CoZ);
	PackedCol cd = AVERAGE(CoXoZ, orig);
	return AVERAGE(ab, cd);
}
static void Modern_DrawXMin(int count, int x, int y, int z) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_XMIN);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = adv_minBB.z, u2 = (count - 1) + adv_maxBB.z * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_minBB.y * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	PackedCol tint, white = PACKEDCOL_WHITE;
	int offset = 1;// (Blocks.LightOffset[Builder_Block] >> FACE_XMIN) & 1;
	PackedCol orig = Lighting.Color_XSide_Fast(x-offset, y, z);
	PackedCol col0_0 = Builder_FullBright ? white : Modern_GetColorX(orig, x-offset, y, z, -1, -1);
	PackedCol col1_0 = Builder_FullBright ? white : Modern_GetColorX(orig, x-offset, y, z, 1, -1);
	PackedCol col1_1 = Builder_FullBright ? white : Modern_GetColorX(orig, x-offset, y, z, 1, 1);
	PackedCol col0_1 = Builder_FullBright ? white : Modern_GetColorX(orig, x-offset, y, z, -1, 1);
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_XMIN];
	v.x = adv_x1;
		v.y = adv_y2; v.z = adv_z2 + (count - 1); v.U = u2; v.V = v1; v.Col = col1_1; *vertices++ = v;
		              v.z = adv_z1;               v.U = u1;           v.Col = col1_0; *vertices++ = v;
		v.y = adv_y1;                                       v.V = v2; v.Col = col0_0; *vertices++ = v;
		              v.z = adv_z2 + (count - 1); v.U = u2;           v.Col = col0_1; *vertices++ = v;
	part->faces.vertices[FACE_XMIN] = vertices;
}

static void Modern_DrawXMax(int count, int x, int y, int z) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_XMAX);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = (count - adv_minBB.z), u2 = (1 - adv_maxBB.z) * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_minBB.y * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	PackedCol tint, white = PACKEDCOL_WHITE;
	int offset = 1;// (Blocks.LightOffset[Builder_Block] >> FACE_XMAX) & 1;
	PackedCol orig = Lighting.Color_XSide_Fast(x+offset, y, z);
	PackedCol col0_0 = Builder_FullBright ? white : Modern_GetColorX(orig, x+offset, y, z, -1, -1);
	PackedCol col1_0 = Builder_FullBright ? white : Modern_GetColorX(orig, x+offset, y, z, 1, -1);
	PackedCol col1_1 = Builder_FullBright ? white : Modern_GetColorX(orig, x+offset, y, z, 1, 1);
	PackedCol col0_1 = Builder_FullBright ? white : Modern_GetColorX(orig, x+offset, y, z, -1, 1);
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_XMAX];
	v.x = adv_x2;
		v.y = adv_y2; v.z = adv_z2 + (count - 1); v.U = u2; v.V = v1; v.Col = col1_1; *vertices++ = v;
		v.y = adv_y1;                                       v.V = v2; v.Col = col0_1; *vertices++ = v;
		              v.z = adv_z1;               v.U = u1;           v.Col = col0_0; *vertices++ = v;
		v.y = adv_y2;                                       v.V = v1; v.Col = col1_0; *vertices++ = v;
	part->faces.vertices[FACE_XMAX] = vertices;
}

static PackedCol Modern_GetColorZ(PackedCol orig, int x, int y, int z, int oX, int oY) {
	cc_bool xOccluded  = Modern_IsOccluded(x + oX, y     , z);
	cc_bool zOccluded  = Modern_IsOccluded(x,      y + oY, z);
	cc_bool xzOccluded = Modern_IsOccluded(x + oX, y + oY, z);

	PackedCol CoX   =                                xOccluded ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color_ZSide_Fast(x + oX, y     , z);
	PackedCol CoZ   =                                zOccluded ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color_ZSide_Fast(x     , y + oY, z);
	PackedCol CoXoZ = (xzOccluded || (xOccluded && zOccluded)) ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color_ZSide_Fast(x + oX, y + oY, z);

	PackedCol ab = AVERAGE(CoX, CoZ);
	PackedCol cd = AVERAGE(CoXoZ, orig);
	return AVERAGE(ab, cd);
}
static void Modern_DrawZMin(int count, int x, int y, int z) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_ZMIN);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = (count - adv_minBB.x), u2 = (1 - adv_maxBB.x) * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_minBB.y * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	PackedCol tint, white = PACKEDCOL_WHITE;
	int offset = 1;// (Blocks.LightOffset[Builder_Block] >> FACE_ZMIN) & 1;
	PackedCol orig = Lighting.Color_ZSide_Fast(x, y, z-offset);
	PackedCol col0_0 = Builder_FullBright ? white : Modern_GetColorZ(orig, x, y, z-offset, -1, -1);
	PackedCol col1_0 = Builder_FullBright ? white : Modern_GetColorZ(orig, x, y, z-offset, 1, -1);
	PackedCol col1_1 = Builder_FullBright ? white : Modern_GetColorZ(orig, x, y, z-offset, 1, 1);
	PackedCol col0_1 = Builder_FullBright ? white : Modern_GetColorZ(orig, x, y, z-offset, -1, 1);
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_ZMIN];
	v.z = adv_z1;
		v.x = adv_x1;               v.y = adv_y1; v.U = u1; v.V = v2; v.Col = col0_0; *vertices++ = v;
		                            v.y = adv_y2;           v.V = v1; v.Col = col0_1; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_1; *vertices++ = v;
		                            v.y = adv_y1;           v.V = v2; v.Col = col1_0; *vertices++ = v;
	part->faces.vertices[FACE_ZMIN] = vertices;
}

static void Modern_DrawZMax(int count, int x, int y, int z) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_ZMAX);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = adv_minBB.x, u2 = (count - 1) + adv_maxBB.x * UV2_Scale;
	float v1 = vOrigin + adv_maxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_minBB.y * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	PackedCol tint, white = PACKEDCOL_WHITE;
	int offset = 1;// (Blocks.LightOffset[Builder_Block] >> FACE_ZMAX) & 1;
	PackedCol orig = Lighting.Color_ZSide_Fast(x, y, z+offset);
	PackedCol col0_0 = Builder_FullBright ? white : Modern_GetColorZ(orig, x, y, z+offset, -1, -1);
	PackedCol col1_0 = Builder_FullBright ? white : Modern_GetColorZ(orig, x, y, z+offset, 1, -1);
	PackedCol col1_1 = Builder_FullBright ? white : Modern_GetColorZ(orig, x, y, z+offset, 1, 1);
	PackedCol col0_1 = Builder_FullBright ? white : Modern_GetColorZ(orig, x, y, z+offset, -1, 1);
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_ZMAX];
	v.z = adv_z2;
		v.x = adv_x2 + (count - 1); v.y = adv_y2; v.U = u2; v.V = v1; v.Col = col1_1; *vertices++ = v;
		v.x = adv_x1;                             v.U = u1;           v.Col = col0_1; *vertices++ = v;
		                            v.y = adv_y1;           v.V = v2; v.Col = col0_0; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_0; *vertices++ = v;
	part->faces.vertices[FACE_ZMAX] = vertices;
}

static PackedCol Modern_GetColorYMin(PackedCol orig, int x, int y, int z, int oX, int oZ) {
	cc_bool xOccluded  = Modern_IsOccluded(x + oX, y, z     );
	cc_bool zOccluded  = Modern_IsOccluded(x,      y, z + oZ);
	cc_bool xzOccluded = Modern_IsOccluded(x + oX, y, z + oZ);

	PackedCol CoX   =                                xOccluded ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color_YMin_Fast(x + oX, y, z     );
	PackedCol CoZ   =                                zOccluded ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color_YMin_Fast(x     , y, z + oZ);
	PackedCol CoXoZ = (xzOccluded || (xOccluded && zOccluded)) ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color_YMin_Fast(x + oX, y, z + oZ);

	PackedCol ab = AVERAGE(CoX, CoZ);
	PackedCol cd = AVERAGE(CoXoZ, orig);
	return AVERAGE(ab, cd);
}
static void Modern_DrawYMin(int count, int x, int y, int z) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_YMIN);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = adv_minBB.x, u2 = (count - 1) + adv_maxBB.x * UV2_Scale;
	float v1 = vOrigin + adv_minBB.z * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_maxBB.z * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	PackedCol tint, white = PACKEDCOL_WHITE;
	int offset = 1;// (Blocks.LightOffset[Builder_Block] >> FACE_YMIN) & 1;
	PackedCol orig = Lighting.Color_YMin_Fast(x, y-offset, z);
	PackedCol col0_0 = Builder_FullBright ? white : Modern_GetColorYMin(orig, x, y-offset, z, -1, -1);
	PackedCol col1_0 = Builder_FullBright ? white : Modern_GetColorYMin(orig, x, y-offset, z,  1, -1);
	PackedCol col1_1 = Builder_FullBright ? white : Modern_GetColorYMin(orig, x, y-offset, z,  1,  1);
	PackedCol col0_1 = Builder_FullBright ? white : Modern_GetColorYMin(orig, x, y-offset, z, -1,  1);
	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_YMIN];
	v.y = adv_y1;
		v.x = adv_x1;               v.z = adv_z2; v.U = u1; v.V = v2; v.Col = col0_1; *vertices++ = v;
		                            v.z = adv_z1;           v.V = v1; v.Col = col0_0; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_0; *vertices++ = v;
		                            v.z = adv_z2;           v.V = v2; v.Col = col1_1; *vertices++ = v;
	part->faces.vertices[FACE_YMIN] = vertices;
}

static PackedCol Modern_GetColorYMax(PackedCol orig, int x, int y, int z, int oX, int oZ) {
	cc_bool xOccluded  = Modern_IsOccluded(x + oX, y, z     );
	cc_bool zOccluded  = Modern_IsOccluded(x,      y, z + oZ);
	cc_bool xzOccluded = Modern_IsOccluded(x + oX, y, z + oZ);

	PackedCol CoX   =                                xOccluded ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color(x + oX, y, z     );
	PackedCol CoZ   =                                zOccluded ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color(x     , y, z + oZ);
	PackedCol CoXoZ = (xzOccluded || (xOccluded && zOccluded)) ? PackedCol_Scale(orig, FANCY_AO) : Lighting.Color(x + oX, y, z + oZ);

	PackedCol ab = AVERAGE(CoX, CoZ);
	PackedCol cd = AVERAGE(CoXoZ, orig);
	return AVERAGE(ab, cd);
}
static void Modern_DrawYMax(int count, int x, int y, int z) {
	TextureLoc texLoc = Block_Tex(Builder_Block, FACE_YMAX);
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = adv_minBB.x, u2 = (count - 1) + adv_maxBB.x * UV2_Scale;
	float v1 = vOrigin + adv_minBB.z * Atlas1D.InvTileSize;
	float v2 = vOrigin + adv_maxBB.z * Atlas1D.InvTileSize * UV2_Scale;
	struct Builder1DPart* part = &Builder_Parts[adv_baseOffset + Atlas1D_Index(texLoc)];

	PackedCol tint, white = PACKEDCOL_WHITE;
	int offset = 1;// (Blocks.LightOffset[Builder_Block] >> FACE_YMAX) & 1;
	PackedCol orig = Lighting.Color(x, y+offset, z);
	PackedCol col0_0 = Builder_FullBright ? white : Modern_GetColorYMax(orig, x, y+offset, z, -1, -1);
	PackedCol col1_0 = Builder_FullBright ? white : Modern_GetColorYMax(orig, x, y+offset, z,  1, -1);
	PackedCol col1_1 = Builder_FullBright ? white : Modern_GetColorYMax(orig, x, y+offset, z,  1,  1);
	PackedCol col0_1 = Builder_FullBright ? white : Modern_GetColorYMax(orig, x, y+offset, z, -1,  1);

	struct VertexTextured* vertices, v;

	if (adv_tinted) {
		tint   = Blocks.FogCol[Builder_Block];
		col0_0 = PackedCol_Tint(col0_0, tint); col1_0 = PackedCol_Tint(col1_0, tint);
		col1_1 = PackedCol_Tint(col1_1, tint); col0_1 = PackedCol_Tint(col0_1, tint);
	}

	vertices = part->faces.vertices[FACE_YMAX];
	v.y = adv_y2;
		v.x = adv_x1;               v.z = adv_z1; v.U = u1; v.V = v1; v.Col = col0_0; *vertices++ = v;
		                            v.z = adv_z2;           v.V = v2; v.Col = col0_1; *vertices++ = v;
		v.x = adv_x2 + (count - 1);               v.U = u2;           v.Col = col1_1; *vertices++ = v;
		                            v.z = adv_z1;           v.V = v1; v.Col = col1_0; *vertices++ = v;
	part->faces.vertices[FACE_YMAX] = vertices;
}

static void Modern_RenderBlock(int index, int x, int y, int z) {
	Vec3 min, max;
	int count_XMin, count_XMax, count_ZMin;
	int count_ZMax, count_YMin, count_YMax;

	if (Blocks.Draw[Builder_Block] == DRAW_SPRITE) {
		Builder_DrawSprite(x, y, z); return;
	}

	count_XMin = Builder_Counts[index + FACE_XMIN];
	count_XMax = Builder_Counts[index + FACE_XMAX];
	count_ZMin = Builder_Counts[index + FACE_ZMIN];
	count_ZMax = Builder_Counts[index + FACE_ZMAX];
	count_YMin = Builder_Counts[index + FACE_YMIN];
	count_YMax = Builder_Counts[index + FACE_YMAX];

	if (!count_XMin && !count_XMax && !count_ZMin &&
		!count_ZMax && !count_YMin && !count_YMax) return;

	Builder_FullBright = Blocks.Brightness[Builder_Block];
	adv_baseOffset = (Blocks.Draw[Builder_Block] == DRAW_TRANSLUCENT) * ATLAS1D_MAX_ATLASES;
	adv_tinted = Blocks.Tinted[Builder_Block];

	min = Blocks.RenderMinBB[Builder_Block]; max = Blocks.RenderMaxBB[Builder_Block];
	adv_x1 = x + min.x; adv_y1 = y + min.y; adv_z1 = z + min.z;
	adv_x2 = x + max.x; adv_y2 = y + max.y; adv_z2 = z + max.z;

	adv_minBB = Blocks.MinBB[Builder_Block]; adv_maxBB = Blocks.MaxBB[Builder_Block];
	adv_minBB.y = 1.0f - adv_minBB.y; adv_maxBB.y = 1.0f - adv_maxBB.y;

	if (count_XMin) Modern_DrawXMin(count_XMin, x, y, z);
	if (count_XMax) Modern_DrawXMax(count_XMax, x, y, z);
	if (count_ZMin) Modern_DrawZMin(count_ZMin, x, y, z);
	if (count_ZMax) Modern_DrawZMax(count_ZMax, x, y, z);
	if (count_YMin) Modern_DrawYMin(count_YMin, x, y, z);
	if (count_YMax) Modern_DrawYMax(count_YMax, x, y, z);
}

static void Modern_PrePrepareChunk(void) {
	DefaultPrePrepateChunk();
	adv_bitFlags = Builder_BitFlags;
}

static void ModernBuilder_SetActive(void) {
	Builder_SetDefault();
	Builder_StretchXLiquid =  Modern_StretchXLiquid;
	Builder_StretchX =        Modern_StretchX;
	Builder_StretchZ =        Modern_StretchZ;
	Builder_RenderBlock =     Modern_RenderBlock;
	Builder_PrePrepareChunk = Modern_PrePrepareChunk;
}
#else
static void ModernBuilder_SetActive(void) { NormalBuilder_SetActive(); }
#endif

/*########################################################################################################################*
*---------------------------------------------------Builder interface-----------------------------------------------------*
*#########################################################################################################################*/
cc_bool Builder_SmoothLighting;
void Builder_ApplyActive(void) {
	if (Builder_SmoothLighting) {
		if (Lighting_Mode != LIGHTING_MODE_CLASSIC) {
			ModernBuilder_SetActive();
		}
		else {
			AdvBuilder_SetActive();
		}
	} else {
		NormalBuilder_SetActive();
	}
}

static void OnInit(void) {
	Builder_Offsets[FACE_XMIN] = -1;
	Builder_Offsets[FACE_XMAX] =  1;
	Builder_Offsets[FACE_ZMIN] = -EXTCHUNK_SIZE;
	Builder_Offsets[FACE_ZMAX] =  EXTCHUNK_SIZE;
	Builder_Offsets[FACE_YMIN] = -EXTCHUNK_SIZE_2;
	Builder_Offsets[FACE_YMAX] =  EXTCHUNK_SIZE_2;

	if (!Game_ClassicMode) Builder_SmoothLighting = Options_GetBool(OPT_SMOOTH_LIGHTING, false);
	Builder_ApplyActive();
}

static void OnNewMapLoaded(void) {
	Builder_SidesLevel = max(0, Env_SidesHeight);
	Builder_EdgeLevel  = max(0, Env.EdgeHeight);
}

struct IGameComponent Builder_Component = {
	OnInit, /* Init */
	NULL, /* Free */
	NULL, /* Reset */
	NULL, /* OnNewMap */
	OnNewMapLoaded /* OnNewMapLoaded */
};
