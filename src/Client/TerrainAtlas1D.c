#include "TerrainAtlas1D.h"
#include "Funcs.h"
#include "GraphicsAPI.h"
#include "Bitmap.h"

TextureRec TerrainAtlas1D_TexRec(Int32 texId, Int32 uCount, Int32* index) {
	*index = texId / TerrainAtlas1D_ElementsPerAtlas1D;
	Int32 y = texId % TerrainAtlas1D_ElementsPerAtlas1D;

	// Adjust coords to be slightly inside - fixes issues with AMD/ATI cards.
	return TextureRec_FromRegion(
		0.0f, y * TerrainAtlas1D_InvElementSize, 
		(uCount - 1) + 15.99f / 16.0f, 
		(15.99f / 16.0f) * TerrainAtlas1D_InvElementSize);
}

void TerrainAtlas1D_UpdateState() {
	Int32 maxVerticalSize = min(4096, Gfx_MaxTextureDimensions);
	Int32 elementsPerFullAtlas = maxVerticalSize / TerrainAtlas2D_ElementSize;
	Int32 totalElements = TerrainAtlas2D_RowsCount * TerrainAtlas2D_ElementsPerRow;

	Int32 atlasesCount = Utils.CeilDiv(totalElements, elementsPerFullAtlas);
	TerrainAtlas1D_ElementsPerAtlas1D = min(elementsPerFullAtlas, totalElements);
	Int32 atlas1DHeight = Utils.NextPowerOf2(TerrainAtlas1D_ElementsPerAtlas1D * TerrainAtlas2D_ElementSize);

	TerrainAtlas1D_Convert2DTo1D(atlasesCount, atlas1DHeight);
	TerrainAtlas1D_ElementsPerBitmap = atlas1DHeight / TerrainAtlas2D_ElementSize;
	TerrainAtlas1D_InvElementSize = 1.0f / TerrainAtlas1D_ElementsPerBitmap;
}

void TerrainAtlas1D_Convert2DTo1D(Int32 atlasesCount, Int32 atlas1DHeight) {
	TerrainAtlas1D_TexIdsCount = atlasesCount;
	UInt8 logBuffer[String_BufferSize(256)];
	String log = String_FromRawBuffer(logBuffer, 256);
	Utils.LogDebug("Loaded new atlas: {0} bmps, {1} per bmp", atlasesCount, elementsPerAtlas1D);

	Int32 index = 0, i;
	for (i = 0; i < atlasesCount; i++) {
		TerrainAtlas1D_Make1DTexture(i, atlas1DHeight, &index);
	}
}

void TerrainAtlas1D_Make1DTexture(Int32 i, Int32 atlas1DHeight, Int32* index) {
	Int32 elemSize = TerrainAtlas2D_ElementSize;
	Bitmap atlas1D = Platform.CreateBmp(atlas2D.TileSize, atlas1DHeight);
	Int32 index1D;

	for (index1D = 0; index1D < TerrainAtlas1D_ElementsPerAtlas1D; index1D++) {
		Int32 atlasX = (*index & 0x0F) * elemSize;
		Int32 atlasY = (*index >> 4) * elemSize;

		Bitmap_CopyBlock(atlasX, atlasY, 0, index1D * elemSize, 
						&TerrainAtlas2D_Bitmap, &atlas1D, elemSize);
		(*index)++;
	}
	TerrainAtlas1D_TexIds[i] = Gfx_CreateTexture(&atlas1D, true);
}

Int32 TerrainAtlas1D_UsedAtlasesCount() {
	Int32 maxTexId = 0;
	Int32 i;

	for (i = 0; i < Block_TexturesCount; i++) {
		maxTexId = max(maxTexId, Block_Textures[i]);
	}
	return TerrainAtlas1D_Get1DIndex(maxTexId) + 1;
}

void TerrainAtlas1D_Free() {
	Int32 i;
	for (i = 0; i < TerrainAtlas1D_TexIdsCount; i++) {
		Gfx_DeleteTexture(&TerrainAtlas1D_TexIds[i]);
	}
}