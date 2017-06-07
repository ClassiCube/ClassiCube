#include "NormalBuilder.h"
#include "Block.h"
#include "Constants.h"
#include "Lighting.h"
#include "World.h"
#include "Drawer.h"
#include "TerrainAtlas1D.h"

Int32 NormalBuilder_StretchXLiquid(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block) {
	if (Builder_OccludedLiquid(chunkIndex)) return 0;
	Int32 count = 1;
	x++;
	chunkIndex++;
	countIndex += Face_Count;

	while (x < Builder_ChunkEndX && NormalBuilder_CanStretch(block, chunkIndex, x, y, z, Face_YMax) && !Builder_OccludedLiquid(chunkIndex)) {
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

bool NormalBuilder_CanStretch(BlockID initial, Int32 chunkIndex, Int32 x, Int32 y, Int32 z, Face face) {
	BlockID cur = Builder_Chunk[chunkIndex];
	return cur == initial
		&& !Block_IsFaceHidden(cur, Builder_Chunk[chunkIndex + Builder_Offsets[face]], face)
		&& (fullBright || (LightCol(Builder_X, Builder_Y, Builder_Z, face, initial) == LightCol(x, y, z, face, cur)));
}

PackedCol NormalBuilder_LightCol(Int32 x, Int32 y, Int32 z, Int32 face, BlockID block) {
	Int32 offset = (Block_LightOffset[block] >> face) & 1;
	switch (face) {
	case Face_XMin:
		return x < offset ? Lighting_OutsideXSide : Lighting_Col_XSide_Fast(x - offset, y, z);
	case Face_XMax:
		return x > (World_MaxX - offset) ? Lighting_OutsideXSide : Lighting_Col_XSide_Fast(x + offset, y, z);
	case Face_ZMin:
		return z < offset ? Lighting_OutsideZSide : Lighting_Col_ZSide_Fast(x, y, z - offset);
	case Face_ZMax:
		return z > (World_MaxZ - offset) ? Lighting_OutsideZSide : Lighting_Col_ZSide_Fast(x, y, z + offset);
	case Face_YMin:
		return y <= 0 ? Lighting_OutsideYBottom : Lighting_Col_YBottom_Fast(x, y - offset, z);
	case Face_YMax:
		return y >= World_MaxY ? Lighting_Outside : Lighting_Col_YTop_Fast(x, (y + 1) - offset, z);
	}
	return PackedCol_Black;
}

void NormalBuilder_RenderTile(Int32 index) {
	if (Block_Draw[Builder_Block] == DrawType_Sprite) {
		this.fullBright = Block_FullBright[Builder_Block];
		this.tinted = Block_Tinted[Builder_Block];

		Int32 count = Builder_Counts[index + Face_YMax];
		if (count != 0) DrawSprite(count);
		return;
	}

	Int32 leftCount = counts[index++], rightCount = counts[index++],
		frontCount = counts[index++], backCount = counts[index++],
		bottomCount = counts[index++], topCount = counts[index++];
	if (leftCount == 0 && rightCount == 0 && frontCount == 0 &&
		backCount == 0 && bottomCount == 0 && topCount == 0) return;

	bool fullBright = Block_FullBright[Builder_Block];
	bool isTranslucent = Block_Draw[Builder_Block] == DrawType_Translucent;
	Int32 lightFlags = Block_LightOffset[Builder_Block];

	Drawer_MinBB = Block_MinBB[Builder_Block]; Drawer_MinBB.Y = 1.0f - Drawer_MinBB.Y;
	Drawer_MaxBB = Block_MaxBB[Builder_Block]; Drawer_MaxBB.Y = 1.0f - Drawer_MaxBB.Y;

	Vector3 min = Block_RenderMinBB[Builder_Block], max = Block_RenderMaxBB[Builder_Block];
	Drawer_X1 = Builder_X + min.X; Drawer_Y1 = Builder_Y + min.Y; Drawer_Z1 = Builder_Z + min.Z;
	Drawer_X2 = Builder_X + max.X; Drawer_Y2 = Builder_Y + max.Y; Drawer_Z2 = Builder_Z + max.Z;

	Drawer_Tinted = Block_Tinted[Builder_Block];
	Drawer_TintColour = Block_FogColour[Builder_Block];

	if (leftCount != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_XMin);
		Int32 i = Atlas1D_Index(texLoc);
		Int32 offset = (lightFlags >> Face_XMin) & 1;

		DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
		Int32 col = fullBright ? FastColour.WhitePacked :
			Builder_X >= offset ? Lighting_Col_XSide_Fast(Builder_X - offset, Builder_Y, Builder_Z) : Lighting_OutsideXSide;
		Drawer_XMin(leftCount, col, texLoc, part.vertices, ref part.vIndex[Side.Left]);
	}

	if (rightCount != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_XMax);
		Int32 i = Atlas1D_Index(texLoc);
		Int32 offset = (lightFlags >> Face_XMax) & 1;

		DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
		Int32 col = fullBright ? FastColour.WhitePacked :
			Builder_X <= (World_MaxX - offset) ? Lighting_Col_XSide_Fast(Builder_X + offset, Builder_Y, Builder_Z) : Lighting_OutsideXSide;
		Drawer_XMax(rightCount, col, texLoc, part.vertices, ref part.vIndex[Side.Right]);
	}

	if (frontCount != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_ZMin);
		Int32 i = Atlas1D_Index(texLoc);
		Int32 offset = (lightFlags >> Face_ZMin) & 1;

		DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
		Int32 col = fullBright ? FastColour.WhitePacked :
			Builder_Z >= offset ? Lighting_Col_ZSide_Fast(Builder_X, Builder_Y, Builder_Z - offset) : Lighting_OutsideZSide;
		Drawer_ZMin(frontCount, col, texLoc, part.vertices, ref part.vIndex[Side.Front]);
	}

	if (backCount != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_ZMax);
		Int32 i = Atlas1D_Index(texLoc);
		Int32 offset = (lightFlags >> Face_ZMax) & 1;

		DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
		Int32 col = fullBright ? FastColour.WhitePacked :
			Builder_Z <= (World_MaxZ - offset) ? Lighting_LightCol_ZSide_Fast(Builder_X, Builder_Y, Builder_Z + offset) : Lighting_OutsideZSide;
		Drawer_ZMax(backCount, col, texLoc, part.vertices, ref part.vIndex[Side.Back]);
	}

	if (bottomCount != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_YMin);
		Int32 i = Atlas1D_Index(texLoc);
		Int32 offset = (lightFlags >> Face_YMin) & 1;

		DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
		Int32 col = fullBright ? FastColour.WhitePacked : Lighting_Col_YBottom_Fast(Builder_X, Builder_Y - offset, Builder_Z);
		Drawer_YMin(bottomCount, col, texLoc, part.vertices, ref part.vIndex[Side.Bottom]);
	}

	if (topCount != 0) {
		TextureLoc texLoc = Block_GetTexLoc(Builder_Block, Face_YMax);
		Int32 i = Atlas1D_Index(texLoc);
		Int32 offset = (lightFlags >> Face_YMax) & 1;

		DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
		Int32 col = fullBright ? FastColour.WhitePacked : Lighting_Col_YTop_Fast(Builder_X, (Builder_Y + 1) - offset, Builder_Z);
		Drawer_YMax(topCount, col, texLoc, part.vertices, ref part.vIndex[Side.Top]);
	}
}