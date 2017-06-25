#include "Builder.h"
#include "Constants.h"
#include "WorldEnv.h"
#include "Block.h"
#include "Funcs.h"
#include "Lighting.h"
#include "World.h"
#include "Platform.h"
#include "MapRenderer.h"
#include "GraphicsAPI.h"

void Builder_Init(void) {
	Builder_WhiteCol = PackedCol_White;
	Builder_Offsets[Face_XMin] = -1;
	Builder_Offsets[Face_XMax] = 1;
	Builder_Offsets[Face_ZMin] = -EXTCHUNK_SIZE;
	Builder_Offsets[Face_ZMax] = EXTCHUNK_SIZE;
	Builder_Offsets[Face_YMin] = -EXTCHUNK_SIZE_2;
	Builder_Offsets[Face_YMax] = EXTCHUNK_SIZE_2;
}

void Builder_SetDefault(void) {
	Builder_StretchXLiquid = NULL;
	Builder_StretchX = NULL;
	Builder_StretchZ = NULL;
	Builder_RenderBlock = NULL;

	Builder_UseBitFlags = false;
	Builder_PreStretchTiles = Builder_DefaultPreStretchTiles;
	Builder_PostStretchTiles = Builder_DefaultPostStretchTiles;
}

void Builder_OnNewMapLoaded(void) {
	Builder_SidesLevel = max(0, WorldEnv_SidesHeight);
	Builder_EdgeLevel = max(0, WorldEnv_EdgeHeight);
}

void Builder_MakeChunk(ChunkInfo* info) {
	Int32 x = info->CentreX - 8, y = info->CentreY - 8, z = info->CentreZ - 8;
	bool allAir = false, hasMesh;
	hasMesh = Builder_BuildChunk(Builder_X, Builder_Y, Builder_Z, &allAir);
	info->AllAir = allAir;
	if (!hasMesh) return;

	Int32 i, partsIndex = MapRenderer_Pack(x >> CHUNK_SHIFT, y >> CHUNK_SHIFT, z >> CHUNK_SHIFT);
	bool hasNormal = false, hasTranslucent = false;

	for (i = 0; i < Atlas1D_Count; i++) {
		Int32 idx = partsIndex + i * MapRenderer_ChunksCount;
		Builder_SetPartInfo(&Builder_Parts[i], i, 
			idx, &hasNormal);
		Builder_SetPartInfo(&Builder_Parts[i + Atlas1D_MaxAtlasesCount], i,
			idx + MapRenderer_TranslucentBufferOffset, &hasTranslucent);
	}

	if (hasNormal) {
		info->NormalParts = &MapRenderer_PartsBuffer[partsIndex];
	}
	if (hasTranslucent) {
		info->NormalParts = &MapRenderer_PartsBuffer[partsIndex + MapRenderer_TranslucentBufferOffset];
	}

#if OCCLUSION
	if (info.NormalParts != null || info.TranslucentParts != null)
		info.occlusionFlags = (UInt8)ComputeOcclusion();
#endif
}


void Builder_AddSpriteVertices(BlockID block) {
	Int32 i = Atlas1D_Index(Block_GetTexLoc(block, Face_XMin));
	Builder1DPart* part = &Builder_Parts[i];
	part->sCount += 6 * 4;
	part->iCount += 6 * 4;
}

void Builder_AddVertices(BlockID block, Face face) {
	Int32 baseOffset = (Block_Draw[block] == DrawType_Translucent) * Atlas1D_MaxAtlasesCount;
	Int32 i = Atlas1D_Index(Block_GetTexLoc(block, face));
	Builder1DPart* part = &Builder_Parts[baseOffset + i];
	part->fCount[face] += 6;
	part->iCount += 6;
}

bool Builder_OccludedLiquid(Int32 chunkIndex) {
	chunkIndex += EXTCHUNK_SIZE_2; /* Checking y above */
	return
		Block_FullOpaque[Builder_Chunk[chunkIndex]]
		&& Block_Draw[Builder_Chunk[chunkIndex - EXTCHUNK_SIZE]] != DrawType_Gas
		&& Block_Draw[Builder_Chunk[chunkIndex - 1]] != DrawType_Gas
		&& Block_Draw[Builder_Chunk[chunkIndex + 1]] != DrawType_Gas
		&& Block_Draw[Builder_Chunk[chunkIndex + EXTCHUNK_SIZE]] != DrawType_Gas;
}

void Builder_DefaultPreStretchTiles(Int32 x1, Int32 y1, Int32 z1) {
	Int32 i;
	for (i = 0; i < Atlas1D_MaxAtlasesCount * 2; i++) {
		Builder1DPart_Reset(&Builder_Parts[i]);
	}
}

void Builder_DefaultPostStretchTiles(Int32 x1, Int32 y1, Int32 z1) {
	Int32 i;
	for (i = 0; i < Atlas1D_MaxAtlasesCount * 2; i++) {
		if (Builder_Parts[i].iCount == 0) continue;
		Builder1DPart_Prepare(&Builder_Parts[i]);
	}
}


void Builder_DrawSprite(Int32 count) {
	TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_XMax);
	Int32 i = Atlas1D_Index(texLoc);
	Real32 vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvElementSize;
	Real32 X = (Real32)Builder_X, Y = (Real32)Builder_Y, Z = (Real32)Builder_Z;

#define sHeight 1.0f
#define u1 0.0f
#define u2 UV2_Scale

	Real32 v1 = vOrigin, v2 = vOrigin + Atlas1D_InvElementSize * UV2_Scale;
	Builder1DPart* part = &Builder_Parts[i];
	PackedCol col = Builder_FullBright ? Builder_WhiteCol : Lighting_Col_Sprite_Fast(Builder_X, Builder_Y, Builder_Z);
	Block_Tint(col, Builder_Block);

	/* Draw Z axis */
	Int32 index = part->sOffset;
	VertexP3fT2fC4b_Set(&part->vertices[index + 0], X + 2.50f / 16.0f, Y, Z + 2.50f / 16.0f, u2, v2, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 1], X + 2.50f / 16.0f, Y + sHeight, Z + 2.50f / 16.0f, u2, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 2], X + 13.5f / 16.0f, Y + sHeight, Z + 13.5f / 16.0f, u1, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 3], X + 13.5f / 16.0f, Y, Z + 13.5f / 16.0f, u1, v2, col);

	/* Draw Z axis mirrored */
	index += part->sAdvance;
	VertexP3fT2fC4b_Set(&part->vertices[index + 0], X + 13.5f / 16.0f, Y, Z + 13.5f / 16.0f, u2, v2, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 1], X + 13.5f / 16.0f, Y + sHeight, Z + 13.5f / 16.0f, u2, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 2], X + 2.50f / 16.0f, Y + sHeight, Z + 2.50f / 16.0f, u1, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 3], X + 2.50f / 16.0f, Y, Z + 2.50f / 16.0f, u1, v2, col);

	/* Draw X axis */
	index += part->sAdvance;
	VertexP3fT2fC4b_Set(&part->vertices[index + 0], X + 2.50f / 16.0f, Y, Z + 13.5f / 16.0f, u2, v2, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 1], X + 2.50f / 16.0f, Y + sHeight, Z + 13.5f / 16.0f, u2, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 2], X + 13.5f / 16.0f, Y + sHeight, Z + 2.50f / 16.0f, u1, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 3], X + 13.5f / 16.0f, Y, Z + 2.50f / 16.0f, u1, v2, col);

	/* Draw X axis mirrored */
	index += part->sAdvance;
	VertexP3fT2fC4b_Set(&part->vertices[index + 0], X + 13.5f / 16.0f, Y, Z + 2.50f / 16.0f, u2, v2, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 1], X + 13.5f / 16.0f, Y + sHeight, Z + 2.50f / 16.0f, u2, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 2], X + 2.50f / 16.0f, Y + sHeight, Z + 13.5f / 16.0f, u1, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 3], X + 2.50f / 16.0f, Y, Z + 13.5f / 16.0f, u1, v2, col);

	part->sOffset += 4;
}


bool Builder_BuildChunk(Int32 x1, Int32 y1, Int32 z1, bool* allAir) {
	Builder_PreStretchTiles(x1, y1, z1);
	BlockID chunk[EXTCHUNK_SIZE_3]; Builder_Chunk = chunk;
	UInt8 counts[CHUNK_SIZE_3 * Face_Count]; Builder_Counts = counts;
	Int32 bitFlags[EXTCHUNK_SIZE_3]; Builder_BitFlags = bitFlags;

	Platform_MemSet(chunk, BlockID_Air, EXTCHUNK_SIZE_3 * sizeof(BlockID));
	if (Builder_ReadChunkData(x1, y1, z1, allAir)) return false;
	Platform_MemSet(counts, 1, CHUNK_SIZE_3 * Face_Count);

	Int32 xMax = min(World_Width, x1 + CHUNK_SIZE);
	Int32 yMax = min(World_Height, y1 + CHUNK_SIZE);
	Int32 zMax = min(World_Length, z1 + CHUNK_SIZE);

	Builder_ChunkEndX = xMax; Builder_ChunkEndZ = zMax;
	Builder_Stretch(x1, y1, z1);
	Builder_PostStretchTiles(x1, y1, z1);
	Int32 x, y, z, xx, yy, zz;

	for (y = y1, yy = 0; y < yMax; y++, yy++) {
		for (z = z1, zz = 0; z < zMax; z++, zz++) {

			Int32 chunkIndex = (yy + 1) * EXTCHUNK_SIZE_2 + (zz + 1) * EXTCHUNK_SIZE + (0 + 1);
			for (x = x1, xx = 0; x < xMax; x++, xx++) {
				Builder_Block = chunk[chunkIndex];
				if (Block_Draw[Builder_Block] != DrawType_Gas) {
					Int32 index = ((yy << 8) | (zz << 4) | xx) * Face_Count;
					Builder_X = x; Builder_Y = y; Builder_Z = z;
					Builder_ChunkIndex = chunkIndex;
					Builder_RenderBlock(index);
				}
				chunkIndex++;
			}
		}
	}
	return true;
}

bool Builder_ReadChunkData(Int32 x1, Int32 y1, Int32 z1, bool* outAllAir) {
	bool allAir = true, allSolid = true;
	Int32 xx, yy, zz;

	for (yy = -1; yy < 17; ++yy) {
		Int32 y = yy + y1;
		if (y < 0) continue;
		if (y >= World_Height) break;

		for (zz = -1; zz < 17; ++zz) {
			Int32 z = zz + z1;
			if (z < 0) continue;
			if (z >= World_Length) break;

			/* need to subtract 1 as index is pre incremented in for loop. */
			Int32 index = World_Pack(x1 - 1, y, z) - 1;
			Int32 chunkIndex = (yy + 1) * EXTCHUNK_SIZE_2 + (zz + 1) * EXTCHUNK_SIZE + (-1 + 1) - 1;

			for (xx = -1; xx < 17; ++xx) {
				Int32 x = xx + x1;
				++index;
				++chunkIndex;

				if (x < 0) continue;
				if (x >= World_Width) break;
				BlockID rawBlock = World_Blocks[index];

				allAir = allAir && Block_Draw[rawBlock] == DrawType_Gas;
				allSolid = allSolid && Block_FullOpaque[rawBlock];
				Builder_Chunk[chunkIndex] = rawBlock;
			}
		}
	}
	*outAllAir = allAir;

	if (x1 == 0 || y1 == 0 || z1 == 0 || x1 + CHUNK_SIZE >= World_Width ||
		y1 + CHUNK_SIZE >= World_Height || z1 + CHUNK_SIZE >= World_Length) allSolid = false;

	if (allAir || allSolid) return true;
	Lighting_LightHint(x1 - 1, z1 - 1);
	return false;
}

void Builder_SetPartInfo(Builder1DPart* part, Int32 i, Int32 partsIndex, bool* hasParts) {
	if (part->iCount == 0) return;
	ChunkPartInfo info;
	Int32 vertCount = (part->iCount / 6 * 4) + 2;

	info.VbId = Gfx_CreateVb(part->vertices, VertexFormat_P3fT2fC4b, vertCount);
	info.IndicesCount = part->iCount;

	info.XMinCount = (UInt16)part->fCount[Face_XMin];
	info.XMaxCount = (UInt16)part->fCount[Face_XMax];
	info.ZMinCount = (UInt16)part->fCount[Face_ZMin];
	info.ZMaxCount = (UInt16)part->fCount[Face_ZMax];
	info.YMinCount = (UInt16)part->fCount[Face_YMin];
	info.YMaxCount = (UInt16)part->fCount[Face_YMax];
	info.SpriteCount = part->sCount;

	*hasParts = true;
	MapRenderer_PartsBuffer[partsIndex] = info;
}

void Builder_Stretch(Int32 x1, Int32 y1, Int32 z1) {
	Int32 xMax = min(World_Width, x1 + CHUNK_SIZE);
	Int32 yMax = min(World_Height, y1 + CHUNK_SIZE);
	Int32 zMax = min(World_Length, z1 + CHUNK_SIZE);
#if OCCLUSION
	Int32 flags = ComputeOcclusion();
#endif
#if DEBUG_OCCLUSION
	FastColour col = new FastColour(60, 60, 60, 255);
	if ((flags & 1) != 0) col.R = 255; // x
	if ((flags & 4) != 0) col.G = 255; // y
	if ((flags & 2) != 0) col.B = 255; // z
	map.Sunlight = map.Shadowlight = col;
	map.SunlightXSide = map.ShadowlightXSide = col;
	map.SunlightZSide = map.ShadowlightZSide = col;
	map.SunlightYBottom = map.ShadowlightYBottom = col;
#endif

	Int32 x, y, z, xx, yy, zz;
	for (y = y1, yy = 0; y < yMax; y++, yy++) {
		for (z = z1, zz = 0; z < zMax; z++, zz++) {
			Int32 cIndex = (yy + 1) * EXTCHUNK_SIZE_2 + (zz + 1) * EXTCHUNK_SIZE + (-1 + 1);
			for (x = x1, xx = 0; x < xMax; x++, xx++) {
				cIndex++;
				BlockID b = Builder_Chunk[cIndex];
				if (Block_Draw[b] == DrawType_Gas) continue;
				Int32 index = ((yy << 8) | (zz << 4) | xx) * Face_Count;

				// Sprites only use one face to indicate stretching count, so we can take a shortcut here.
				// Note that sprites are not drawn with any of the DrawXFace, they are drawn using DrawSprite.
				if (Block_Draw[b] == DrawType_Sprite) {
					index += Face_YMax;
					if (Builder_Counts[index] != 0) {
						Builder_X = x; Builder_Y = y; Builder_Z = z;
						Builder_AddSpriteVertices(b);
						Builder_Counts[index] = 1;
					}
					continue;
				}

				Builder_X = x; Builder_Y = y; Builder_Z = z;
				Builder_FullBright = Block_FullBright[b];
#if USE16_BIT
				Int32 tileIdx = b << 12;
#else
				Int32 tileIdx = b << 8;
#endif
				/* All of these function calls are inlined as they can be called tens of millions to hundreds of millions of times. */

				if (Builder_Counts[index] == 0 ||
					(x == 0 && (y < Builder_SidesLevel || (b >= BlockID_Water && b <= BlockID_StillLava && y < Builder_EdgeLevel))) ||
					(x != 0 && (Block_Hidden[tileIdx + Builder_Chunk[cIndex - 1]] & (1 << Face_XMin)) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					Int32 count = Builder_StretchZ(index, x, y, z, cIndex, b, Face_XMin);
					Builder_AddVertices(b, Face_XMin);
					Builder_Counts[index] = (UInt8)count;
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(x == World_MaxX && (y < Builder_SidesLevel || (b >= BlockID_Water && b <= BlockID_StillLava && y < Builder_EdgeLevel))) ||
					(x != World_MaxX && (Block_Hidden[tileIdx + Builder_Chunk[cIndex + 1]] & (1 << Face_XMax)) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					Int32 count = Builder_StretchZ(index, x, y, z, cIndex, b, Face_XMax);
					Builder_AddVertices(b, Face_XMax);
					Builder_Counts[index] = (UInt8)count;
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(z == 0 && (y < Builder_SidesLevel || (b >= BlockID_Water && b <= BlockID_StillLava && y < Builder_EdgeLevel))) ||
					(z != 0 && (Block_Hidden[tileIdx + Builder_Chunk[cIndex - EXTCHUNK_SIZE]] & (1 << Face_ZMin)) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					Int32 count = Builder_StretchX(index, Builder_X, Builder_Y, Builder_Z, cIndex, b, Face_ZMin);
					Builder_AddVertices(b, Face_ZMin);
					Builder_Counts[index] = (UInt8)count;
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(z == World_MaxZ && (y < Builder_SidesLevel || (b >= BlockID_Water && b <= BlockID_StillLava && y < Builder_EdgeLevel))) ||
					(z != World_MaxZ && (Block_Hidden[tileIdx + Builder_Chunk[cIndex + EXTCHUNK_SIZE]] & (1 << Face_ZMax)) != 0)) {
					Builder_Counts[index] = 0;
				} else {
					Int32 count = Builder_StretchX(index, x, y, z, cIndex, b, Face_ZMax);
					Builder_AddVertices(b, Face_ZMax);
					Builder_Counts[index] = (UInt8)count;
				}

				index++;
				if (Builder_Counts[index] == 0 || y == 0 ||
					(Block_Hidden[tileIdx + Builder_Chunk[cIndex - EXTCHUNK_SIZE_2]] & (1 << Face_YMin)) != 0) {
					Builder_Counts[index] = 0;
				} else {
					Int32 count = Builder_StretchX(index, x, y, z, cIndex, b, Face_YMin);
					Builder_AddVertices(b, Face_YMin);
					Builder_Counts[index] = (UInt8)count;
				}

				index++;
				if (Builder_Counts[index] == 0 ||
					(Block_Hidden[tileIdx + Builder_Chunk[cIndex + EXTCHUNK_SIZE_2]] & (1 << Face_YMax)) != 0) {
					Builder_Counts[index] = 0;
				} else if (b < BlockID_Water || b > BlockID_StillLava) {
					Int32 count = Builder_StretchX(index, x, y, z, cIndex, b, Face_YMax);
					Builder_AddVertices(b, Face_YMax);
					Builder_Counts[index] = (UInt8)count;
				} else {
					Int32 count = Builder_StretchXLiquid(index, x, y, z, cIndex, b);
					if (count > 0) Builder_AddVertices(b, Face_YMax);
					Builder_Counts[index] = (UInt8)count;
				}
			}
		}
	}
}