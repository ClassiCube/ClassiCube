#include "TerrainAtlas1D.h"
#include "Funcs.h"
#include "GraphicsAPI.h"
#include "Bitmap.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Block.h"

TextureRec Atlas1D_TexRec(Int32 texId, Int32 uCount, Int32* index) {
	*index = texId / Atlas1D_ElementsPerAtlas1D;
	Int32 y = texId % Atlas1D_ElementsPerAtlas1D;

	// Adjust coords to be slightly inside - fixes issues with AMD/ATI cards.
	return TextureRec_FromRegion(
		0.0f, y * Atlas1D_InvElementSize, 
		(uCount - 1) + 15.99f / 16.0f, 
		(15.99f / 16.0f) * Atlas1D_InvElementSize);
}

void Atlas1D_UpdateState() {
	Int32 maxVerticalSize = min(4096, Gfx_MaxTextureDimensions);
	Int32 elementsPerFullAtlas = maxVerticalSize / Atlas2D_ElementSize;
	Int32 totalElements = Atlas2D_RowsCount * Atlas2D_ElementsPerRow;

	Int32 atlasesCount = Math_CeilDiv(totalElements, elementsPerFullAtlas);
	Atlas1D_ElementsPerAtlas1D = min(elementsPerFullAtlas, totalElements);
	Int32 atlas1DHeight = Math_NextPowOf2(Atlas1D_ElementsPerAtlas1D * Atlas2D_ElementSize);

	Atlas1D_Convert2DTo1D(atlasesCount, atlas1DHeight);
	Atlas1D_ElementsPerBitmap = atlas1DHeight / Atlas2D_ElementSize;
	Atlas1D_InvElementSize = 1.0f / Atlas1D_ElementsPerBitmap;
}

void Atlas1D_Convert2DTo1D(Int32 atlasesCount, Int32 atlas1DHeight) {
	Atlas1D_TexIdsCount = atlasesCount;
	UInt8 logBuffer[String_BufferSize(256)];
	String log = String_FromRawBuffer(logBuffer, 256);

	String_AppendConstant(&log, "Loaded new atlas: ");
	String_AppendInt32(&log, atlasesCount);
	String_AppendConstant(&log, " bmps, ");
	String_AppendInt32(&log, Atlas1D_ElementsPerBitmap);
	String_AppendConstant(&log, " per bmp");
	Platform_Log(log);

	Int32 index = 0, i;
	for (i = 0; i < atlasesCount; i++) {
		Atlas1D_Make1DTexture(i, atlas1DHeight, &index);
	}
}

void Atlas1D_Make1DTexture(Int32 i, Int32 atlas1DHeight, Int32* index) {
	Int32 elemSize = Atlas2D_ElementSize;
	Bitmap atlas1D;
	Bitmap_Allocate(&atlas1D, elemSize, atlas1DHeight);

	if (atlas1D.Scan0 == NULL) {
		ErrorHandler_Fail("Atlas1D_Make1DTexture - couldn't allocate memory");
	}
	Int32 index1D;

	for (index1D = 0; index1D < Atlas1D_ElementsPerAtlas1D; index1D++) {
		Int32 atlasX = (*index & 0x0F) * elemSize;
		Int32 atlasY = (*index >> 4) * elemSize;

		Bitmap_CopyBlock(atlasX, atlasY, 0, index1D * elemSize, 
						&Atlas2D_Bitmap, &atlas1D, elemSize);
		(*index)++;
	}

	Atlas1D_TexIds[i] = Gfx_CreateTexture(&atlas1D, true);
	Platform_MemFree(atlas1D.Scan0);
}

Int32 Atlas1D_UsedAtlasesCount() {
	Int32 maxTexId = 0;
	Int32 i;

	for (i = 0; i < Block_TexturesCount; i++) {
		maxTexId = max(maxTexId, Block_Textures[i]);
	}
	return Atlas1D_Index(maxTexId) + 1;
}

void Atlas1D_Free() {
	Int32 i;
	for (i = 0; i < Atlas1D_TexIdsCount; i++) {
		Gfx_DeleteTexture(&Atlas1D_TexIds[i]);
	}
}