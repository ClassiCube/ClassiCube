#include "Builder.h"
#include "Constants.h"
#include "WorldEnv.h"
#include "Block.h"
#include "Funcs.h"
#include "Lighting.h"

void Builder_Init(void) {
	Builder_WhiteCol = PackedCol_White;
	Builder_Offsets[Face_XMin] = -1;
	Builder_Offsets[Face_XMax] =  1;
	Builder_Offsets[Face_ZMin] = -EXTCHUNK_SIZE;
	Builder_Offsets[Face_ZMax] =  EXTCHUNK_SIZE;
	Builder_Offsets[Face_YMin] = -EXTCHUNK_SIZE_2;
	Builder_Offsets[Face_YMax] =  EXTCHUNK_SIZE_2;
}

void Builder_SetDefault(void) {
	Builder_StretchXLiquid = NULL;
	Builder_StretchX = NULL;
	Builder_StretchZ = NULL;
	Builder_RenderBlock = NULL;

	Builder_PreStretchTiles = Builder_DefaultPreStretchTiles;
	Builder_PostStretchTiles = Builder_DefaultPostStretchTiles;
}

void Builder_OnNewMapLoaded(void) {
	Builder_SidesLevel = max(0, WorldEnv_SidesHeight);
	Builder_EdgeLevel  = max(0, WorldEnv_EdgeHeight);
}

void Builder_MakeChunk(ChunkInfo* info) {
	Int32 x = info->CentreX - 8, y = info->CentreY - 8, z = info->CentreZ - 8;
	bool allAir = false, hasMesh;
	hasMesh = Builder_BuildChunk(x, y, z, &allAir);
	info->AllAir = allAir;
	if (!hasMesh) return;

	Int32 i;
	for (i = 0; i < Atlas1D_TexIdsCount; i++) {
		Builder_SetPartInfo(&Builder_Parts[i],                           i, &info->NormalParts);
		Builder_SetPartInfo(&Builder_Parts[i + Atlas1D_MaxAtlasesCount], i, &info->TranslucentParts);
	}

#if OCCLUSION
	if (info.NormalParts != null || info.TranslucentParts != null)
		info.occlusionFlags = (byte)ComputeOcclusion();
#endif
}




void Builder_AddSpriteVertices(BlockID block) {
	Int32 i = Atlas1D_Index(Block_GetTexLoc(block, Face_XMin));
	Builder1DPart* part = &Builder_Parts[i];
	part->sCount += 6 * 4;
	part->iCount += 6 * 4;
}

void Builder_AddVertices(BlockID block, Int32 count, Face face) {
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
		&& Block_Draw[Builder_Chunk[chunkIndex - 1]]             != DrawType_Gas
		&& Block_Draw[Builder_Chunk[chunkIndex + 1]]             != DrawType_Gas
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
	VertexP3fT2fC4b_Set(&part->vertices[index + 0], X + 2.50f / 16.0f, Y,           Z + 2.50f / 16.0f, u2, v2, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 1], X + 2.50f / 16.0f, Y + sHeight, Z + 2.50f / 16.0f, u2, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 2], X + 13.5f / 16.0f, Y + sHeight, Z + 13.5f / 16.0f, u1, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 3], X + 13.5f / 16.0f, Y,           Z + 13.5f / 16.0f, u1, v2, col);

	/* Draw Z axis mirrored */
	index += part->sAdvance;
	VertexP3fT2fC4b_Set(&part->vertices[index + 0], X + 13.5f / 16.0f, Y,           Z + 13.5f / 16.0f, u2, v2, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 1], X + 13.5f / 16.0f, Y + sHeight, Z + 13.5f / 16.0f, u2, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 2], X + 2.50f / 16.0f, Y + sHeight, Z + 2.50f / 16.0f, u1, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 3], X + 2.50f / 16.0f, Y,           Z + 2.50f / 16.0f, u1, v2, col);

	/* Draw X axis */
	index += part->sAdvance;
	VertexP3fT2fC4b_Set(&part->vertices[index + 0], X + 2.50f / 16.0f, Y,           Z + 13.5f / 16.0f, u2, v2, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 1], X + 2.50f / 16.0f, Y + sHeight, Z + 13.5f / 16.0f, u2, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 2], X + 13.5f / 16.0f, Y + sHeight, Z + 2.50f / 16.0f, u1, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 3], X + 13.5f / 16.0f, Y,           Z + 2.50f / 16.0f, u1, v2, col);

	/* Draw X axis mirrored */
	index += part->sAdvance;
	VertexP3fT2fC4b_Set(&part->vertices[index + 0], X + 13.5f / 16.0f, Y,           Z + 2.50f / 16.0f, u2, v2, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 1], X + 13.5f / 16.0f, Y + sHeight, Z + 2.50f / 16.0f, u2, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 2], X + 2.50f / 16.0f, Y + sHeight, Z + 13.5f / 16.0f, u1, v1, col);
	VertexP3fT2fC4b_Set(&part->vertices[index + 3], X + 2.50f / 16.0f, Y,           Z + 13.5f / 16.0f, u1, v2, col);

	part->sOffset += 4;
}