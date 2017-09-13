#include "Builder.h"
#include "Constants.h"
#include "World.h"
#include "Funcs.h"
#include "Lighting.h"
#include "Platform.h"
#include "MapRenderer.h"
#include "GraphicsAPI.h"
#include "ErrorHandler.h"
#include "Drawer.h"

void Builder1DPart_Prepare(Builder1DPart* part) {
	part->sOffset = 0;
	part->sAdvance = (part->sCount / 4);

	/* ensure buffer can be accessed with 64 bytes alignment by putting 2 extra vertices at end. */
	Int32 vCount = Builder1DPart_VerticesCount(part);
	if (vCount > part->verticesBufferCount) {
		if (part->vertices != NULL)
			Platform_MemFree(part->vertices);

		part->vertices = Platform_MemAlloc((vCount + 2) * sizeof(VertexP3fT2fC4b));
		part->verticesBufferCount = vCount;
		if (part->vertices == NULL) {
			ErrorHandler_Fail("Builder1DPart_Prepare - failed to allocate memory");
		}
	}

	Int32 offset = part->sCount, i;
	for (i = 0; i < Face_Count; i++) {
		part->fVertices[i] = &part->vertices[offset];
		offset += part->fCount[i];
	}
}

void Builder1DPart_Reset(Builder1DPart* part) {
	part->sCount = 0; part->sOffset = 0; part->sAdvance = 0;

	Int32 i;
	for (i = 0; i < Face_Count; i++) {
		part->fVertices[i] = NULL;
		part->fCount[i] = 0;
	}
}

Int32 Builder1DPart_VerticesCount(Builder1DPart* part) {
	Int32 count = part->sCount;
	Int32 i;
	for (i = 0; i < Face_Count; i++) { count += part->fCount[i]; }
	return count;
}

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

void Builder_AddSpriteVertices(BlockID block) {
	Int32 i = Atlas1D_Index(Block_GetTexLoc(block, Face_XMin));
	Builder1DPart* part = &Builder_Parts[i];
	part->sCount += 4 * 4;
}

void Builder_AddVertices(BlockID block, Face face) {
	Int32 baseOffset = (Block_Draw[block] == DrawType_Translucent) * Atlas1D_MaxAtlasesCount;
	Int32 i = Atlas1D_Index(Block_GetTexLoc(block, face));
	Builder1DPart* part = &Builder_Parts[baseOffset + i];
	part->fCount[face] += 4;
}

void Builder_SetPartInfo(Builder1DPart* part, Int32 i, Int32 partsIndex, bool* hasParts) {
	Int32 vCount = Builder1DPart_VerticesCount(part);
	if (vCount == 0) return;

	ChunkPartInfo info;
	info.VbId = Gfx_CreateVb(part->vertices, VertexFormat_P3fT2fC4b, vCount);
	info.VerticesCount = vCount;

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

				/* Sprites only use one face to indicate stretching count, so we can take a shortcut here.
				Note that sprites are not drawn with any of the DrawXFace, they are drawn using DrawSprite. */
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
		Int32 vCount = Builder1DPart_VerticesCount(&Builder_Parts[i]);
		if (vCount == 0) continue;
		Builder1DPart_Prepare(&Builder_Parts[i]);
	}
}

void Builder_DrawSprite(Int32 count) {
	TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_XMax);
	Int32 i = Atlas1D_Index(texLoc);
	Real32 vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvElementSize;
	Real32 X = (Real32)Builder_X, Y = (Real32)Builder_Y, Z = (Real32)Builder_Z;

#define u1 0.0f
#define u2 UV2_Scale
	Real32 x1 = (Real32)X + 2.50f / 16.0f, y1 = (Real32)Y,        z1 = (Real32)Z + 2.50f / 16.0f;
	Real32 x2 = (Real32)X + 13.5f / 16.0f, y2 = (Real32)Y + 1.0f, z2 = (Real32)Z + 13.5f / 16.0f;
	Real32 v1 = vOrigin, v2 = vOrigin + Atlas1D_InvElementSize * UV2_Scale;
	
	Builder1DPart* part = &Builder_Parts[i];
	PackedCol col = Builder_FullBright ? Builder_WhiteCol : Lighting_Col_Sprite_Fast(Builder_X, Builder_Y, Builder_Z);
	Block_Tint(col, Builder_Block);
	VertexP3fT2fC4b v; v.Colour = col;

	/* Draw Z axis */
	Int32 index = part->sOffset;
	v.X = x1; v.Y = y1; v.Z = z1; v.U = u2; v.V = v2; part->vertices[index + 0] = v;
	          v.Y = y2;                     v.V = v1; part->vertices[index + 1] = v;
	v.X = x2;           v.Z = z2; v.U = u1;           part->vertices[index + 2] = v;
	          v.Y = y1;                     v.V = v2; part->vertices[index + 3] = v;

	/* Draw Z axis mirrored */
	index += part->sAdvance;
	v.X = x2; v.Y = y1; v.Z = z2; v.U = u2;           part->vertices[index + 0] = v;
	          v.Y = y2;                     v.V = v1; part->vertices[index + 1] = v;
	v.X = x1;           v.Z = z1; v.U = u1;           part->vertices[index + 2] = v;
	          v.Y = y1;                     v.V = v2; part->vertices[index + 3] = v;

	/* Draw X axis */
	index += part->sAdvance;
	v.X = x1; v.Y = y1; v.Z = z2; v.U = u2;           part->vertices[index + 0] = v;
	          v.Y = y2;                     v.V = v1; part->vertices[index + 1] = v;
	v.X = x2;           v.Z = z1; v.U = u1;           part->vertices[index + 2] = v;
	          v.Y = y1;                     v.V = v2; part->vertices[index + 3] = v;

	/* Draw X axis mirrored */
	index += part->sAdvance;
	v.X = x2; v.Y = y1; v.Z = z1; v.U = u2;           part->vertices[index + 0] = v;
	          v.Y = y2;                     v.V = v1; part->vertices[index + 1] = v;
	v.X = x1;           v.Z = z2; v.U = u1;           part->vertices[index + 2] = v;
	          v.Y = y1;                     v.V = v2; part->vertices[index + 3] = v;

	part->sOffset += 4;
}


PackedCol NormalBuilder_LightCol(Int32 x, Int32 y, Int32 z, Int32 face, BlockID block) {
	Int32 offset = (Block_LightOffset[block] >> face) & 1;
	switch (face) {
	case Face_XMin:
		return x < offset ? Lighting_OutsideXSide : Lighting_Col_XSide_Fast(x - offset, y, z);
	case Face_XMax:
		return x >(World_MaxX - offset) ? Lighting_OutsideXSide : Lighting_Col_XSide_Fast(x + offset, y, z);
	case Face_ZMin:
		return z < offset ? Lighting_OutsideZSide : Lighting_Col_ZSide_Fast(x, y, z - offset);
	case Face_ZMax:
		return z >(World_MaxZ - offset) ? Lighting_OutsideZSide : Lighting_Col_ZSide_Fast(x, y, z + offset);
	case Face_YMin:
		return y <= 0 ? Lighting_OutsideYBottom : Lighting_Col_YBottom_Fast(x, y - offset, z);
	case Face_YMax:
		return y >= World_MaxY ? Lighting_Outside : Lighting_Col_YTop_Fast(x, (y + 1) - offset, z);
	}
	return PackedCol_Black;
}

bool NormalBuilder_CanStretch(BlockID initial, Int32 chunkIndex, Int32 x, Int32 y, Int32 z, Face face) {
	BlockID cur = Builder_Chunk[chunkIndex];
	return cur == initial
		&& !Block_IsFaceHidden(cur, Builder_Chunk[chunkIndex + Builder_Offsets[face]], face)
		&& (Builder_FullBright || (NormalBuilder_LightCol(Builder_X, Builder_Y, Builder_Z, face, initial).Packed == NormalBuilder_LightCol(x, y, z, face, cur).Packed));
}

Int32 NormalBuilder_StretchXLiquid(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block) {
	if (Builder_OccludedLiquid(chunkIndex)) return 0;
	Int32 count = 1;
	x++;
	chunkIndex++;
	countIndex += Face_Count;
	bool stretchTile = (Block_CanStretch[block] & (1 << Face_YMax)) != 0;

	while (x < Builder_ChunkEndX && stretchTile && NormalBuilder_CanStretch(block, chunkIndex, x, y, z, Face_YMax) && !Builder_OccludedLiquid(chunkIndex)) {
		Builder_Counts[countIndex] = 0;
		count++;
		x++;
		chunkIndex++;
		countIndex += Face_Count;
	}
	return count;
}

Int32 NormalBuilder_StretchX(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Face face) {
	Int32 count = 1;
	x++;
	chunkIndex++;
	countIndex += Face_Count;
	bool stretchTile = (Block_CanStretch[block] & (1 << face)) != 0;

	while (x < Builder_ChunkEndX && stretchTile && NormalBuilder_CanStretch(block, chunkIndex, x, y, z, face)) {
		Builder_Counts[countIndex] = 0;
		count++;
		x++;
		chunkIndex++;
		countIndex += Face_Count;
	}
	return count;
}

Int32 NormalBuilder_StretchZ(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Face face) {
	Int32 count = 1;
	z++;
	chunkIndex += EXTCHUNK_SIZE;
	countIndex += CHUNK_SIZE * Face_Count;
	bool stretchTile = (Block_CanStretch[block] & (1 << face)) != 0;

	while (z < Builder_ChunkEndZ && stretchTile && NormalBuilder_CanStretch(block, chunkIndex, x, y, z, face)) {
		Builder_Counts[countIndex] = 0;
		count++;
		z++;
		chunkIndex += EXTCHUNK_SIZE;
		countIndex += CHUNK_SIZE * Face_Count;
	}
	return count;
}

void NormalBuilder_RenderBlock(Int32 index) {
	if (Block_Draw[Builder_Block] == DrawType_Sprite) {
		Builder_FullBright = Block_FullBright[Builder_Block];
		Builder_Tinted = Block_Tinted[Builder_Block];

		Int32 count = Builder_Counts[index + Face_YMax];
		if (count != 0) Builder_DrawSprite(count);
		return;
	}

	Int32 count_XMin = Builder_Counts[index + Face_XMin];
	Int32 count_XMax = Builder_Counts[index + Face_XMax];
	Int32 count_ZMin = Builder_Counts[index + Face_ZMin];
	Int32 count_ZMax = Builder_Counts[index + Face_ZMax];
	Int32 count_YMin = Builder_Counts[index + Face_YMin];
	Int32 count_YMax = Builder_Counts[index + Face_YMax];

	if (count_XMin == 0 && count_XMax == 0 && count_ZMin == 0 &&
		count_ZMax == 0 && count_YMin == 0 && count_YMax == 0) return;


	bool fullBright = Block_FullBright[Builder_Block];
	Int32 partOffset = (Block_Draw[Builder_Block] == DrawType_Translucent) * Atlas1D_MaxAtlasesCount;
	Int32 lightFlags = Block_LightOffset[Builder_Block];

	Drawer_MinBB = Block_MinBB[Builder_Block]; Drawer_MinBB.Y = 1.0f - Drawer_MinBB.Y;
	Drawer_MaxBB = Block_MaxBB[Builder_Block]; Drawer_MaxBB.Y = 1.0f - Drawer_MaxBB.Y;

	Vector3 min = Block_RenderMinBB[Builder_Block], max = Block_RenderMaxBB[Builder_Block];
	Drawer_X1 = Builder_X + min.X; Drawer_Y1 = Builder_Y + min.Y; Drawer_Z1 = Builder_Z + min.Z;
	Drawer_X2 = Builder_X + max.X; Drawer_Y2 = Builder_Y + max.Y; Drawer_Z2 = Builder_Z + max.Z;

	Drawer_Tinted = Block_Tinted[Builder_Block];
	Drawer_TintColour = Block_FogColour[Builder_Block];


	if (count_XMin != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_XMin);
		Int32 offset = (lightFlags >> Face_XMin) & 1;
		Builder1DPart* part = &Builder_Parts[partOffset + Atlas1D_Index(texLoc)];

		PackedCol col = fullBright ? Builder_WhiteCol :
			Builder_X >= offset ? Lighting_Col_XSide_Fast(Builder_X - offset, Builder_Y, Builder_Z) : Lighting_OutsideXSide;
		Drawer_XMin(count_XMin, col, texLoc, &part->fVertices[Face_XMin]);
	}

	if (count_XMax != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_XMax);
		Int32 offset = (lightFlags >> Face_XMax) & 1;
		Builder1DPart* part = &Builder_Parts[partOffset + Atlas1D_Index(texLoc)];

		PackedCol col = fullBright ? Builder_WhiteCol :
			Builder_X <= (World_MaxX - offset) ? Lighting_Col_XSide_Fast(Builder_X + offset, Builder_Y, Builder_Z) : Lighting_OutsideXSide;
		Drawer_XMax(count_XMax, col, texLoc, &part->fVertices[Face_XMax]);
	}

	if (count_ZMin != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_ZMin);
		Int32 offset = (lightFlags >> Face_ZMin) & 1;
		Builder1DPart* part = &Builder_Parts[partOffset + Atlas1D_Index(texLoc)];

		PackedCol col = fullBright ? Builder_WhiteCol :
			Builder_Z >= offset ? Lighting_Col_ZSide_Fast(Builder_X, Builder_Y, Builder_Z - offset) : Lighting_OutsideZSide;
		Drawer_ZMin(count_ZMin, col, texLoc, &part->fVertices[Face_ZMin]);
	}

	if (count_ZMax != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_ZMax);
		Int32 offset = (lightFlags >> Face_ZMax) & 1;
		Builder1DPart* part = &Builder_Parts[partOffset + Atlas1D_Index(texLoc)];

		PackedCol col = fullBright ? Builder_WhiteCol :
			Builder_Z <= (World_MaxZ - offset) ? Lighting_Col_ZSide_Fast(Builder_X, Builder_Y, Builder_Z + offset) : Lighting_OutsideZSide;
		Drawer_ZMax(count_ZMax, col, texLoc, &part->fVertices[Face_ZMax]);
	}

	if (count_YMin != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_YMin);
		Int32 offset = (lightFlags >> Face_YMin) & 1;
		Builder1DPart* part = &Builder_Parts[partOffset + Atlas1D_Index(texLoc)];

		PackedCol col = fullBright ? Builder_WhiteCol : Lighting_Col_YBottom_Fast(Builder_X, Builder_Y - offset, Builder_Z);
		Drawer_YMin(count_YMin, col, texLoc, &part->fVertices[Face_YMin]);
	}

	if (count_YMax != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_YMax);
		Int32 offset = (lightFlags >> Face_YMax) & 1;
		Builder1DPart* part = &Builder_Parts[partOffset + Atlas1D_Index(texLoc)];

		PackedCol col = fullBright ? Builder_WhiteCol : Lighting_Col_YTop_Fast(Builder_X, (Builder_Y + 1) - offset, Builder_Z);
		Drawer_YMax(count_YMax, col, texLoc, &part->fVertices[Face_YMax]);
	}
}

void NormalBuilder_SetActive(void) {
	Builder_SetDefault();
	Builder_StretchXLiquid = NormalBuilder_StretchXLiquid;
	Builder_StretchX = NormalBuilder_StretchX;
	Builder_StretchZ = NormalBuilder_StretchZ;
	Builder_RenderBlock = NormalBuilder_RenderBlock;
}