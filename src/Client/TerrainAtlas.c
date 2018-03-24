#include "TerrainAtlas.h"
#include "Bitmap.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "GraphicsAPI.h"
#include "Platform.h"

void Atlas2D_UpdateState(Bitmap bmp) {
	Atlas2D_Bitmap = bmp;
	Atlas2D_ElementSize = bmp.Width / ATLAS2D_ELEMENTS_PER_ROW;
	Block_RecalculateSpriteBB();
}

GfxResourceID Atlas2D_LoadTextureElement_Raw(TextureLoc texLoc, Bitmap* element) {
	Int32 size = Atlas2D_ElementSize;
	Int32 x = texLoc % ATLAS2D_ELEMENTS_PER_ROW, y = texLoc / ATLAS2D_ELEMENTS_PER_ROW;
	Bitmap_CopyBlock(x * size, y * size, 0, 0,
		&Atlas2D_Bitmap, element, size);

	return Gfx_CreateTexture(element, false, Gfx_Mipmaps);
}

GfxResourceID Atlas2D_LoadTextureElement(TextureLoc texLoc) {
	Int32 size = Atlas2D_ElementSize;
	Bitmap element;

	/* Try to allocate bitmap on stack if possible */
	if (size > 64) {
		Bitmap_Allocate(&element, size, size);
		GfxResourceID texId = Atlas2D_LoadTextureElement_Raw(texLoc, &element);
		Platform_MemFree(element.Scan0);
		return texId;
	} else {
		// TODO: does this even work??
		UInt8 scan0[Bitmap_DataSize(64, 64)];
		Bitmap_Create(&element, size, size, scan0);
		return Atlas2D_LoadTextureElement_Raw(texLoc, &element);
	}
}

void Atlas2D_Free(void) {
	if (Atlas2D_Bitmap.Scan0 == NULL) return;
	Platform_MemFree(Atlas2D_Bitmap.Scan0);
}


TextureRec Atlas1D_TexRec(TextureLoc texLoc, Int32 uCount, Int32* index) {
	*index = texLoc / Atlas1D_ElementsPerAtlas;
	Int32 y = texLoc % Atlas1D_ElementsPerAtlas;

	/* Adjust coords to be slightly inside - fixes issues with AMD/ATI cards. */
	TextureRec rec;
	rec.U1 = 0.0f; 
	rec.V1 = y * Atlas1D_InvElementSize;
	rec.U2 = (uCount - 1) + UV2_Scale;
	rec.V2 = rec.V1 + UV2_Scale * Atlas1D_InvElementSize;
	return rec;
}

void Atlas1D_Make1DTexture(Int32 i, Int32 atlas1DHeight, Int32* index) {
	Int32 elemSize = Atlas2D_ElementSize;
	Bitmap atlas1D;
	Bitmap_Allocate(&atlas1D, elemSize, atlas1DHeight);

	Int32 index1D;
	for (index1D = 0; index1D < Atlas1D_ElementsPerAtlas; index1D++) {
		Int32 atlasX = (*index & 0x0F) * elemSize;
		Int32 atlasY = (*index >> 4) * elemSize;

		Bitmap_CopyBlock(atlasX, atlasY, 0, index1D * elemSize,
			&Atlas2D_Bitmap, &atlas1D, elemSize);
		(*index)++;
	}

	Atlas1D_TexIds[i] = Gfx_CreateTexture(&atlas1D, true, Gfx_Mipmaps);
	Platform_MemFree(atlas1D.Scan0);
}

void Atlas1D_Convert2DTo1D(Int32 atlasesCount, Int32 atlas1DHeight) {
	Atlas1D_Count = atlasesCount;
	UInt8 logBuffer[String_BufferSize(STRING_SIZE * 2)];
	String log = String_InitAndClearArray(logBuffer);

	String_AppendConst(&log, "Loaded new atlas: ");
	String_AppendInt32(&log, atlasesCount);
	String_AppendConst(&log, " bmps, ");
	String_AppendInt32(&log, Atlas1D_ElementsPerBitmap);
	String_AppendConst(&log, " per bmp");
	Platform_Log(&log);

	Int32 index = 0, i;
	for (i = 0; i < atlasesCount; i++) {
		Atlas1D_Make1DTexture(i, atlas1DHeight, &index);
	}
}

void Atlas1D_UpdateState(void) {
	Int32 maxVerticalSize = min(4096, Gfx_MaxTextureDimensions);
	Int32 elementsPerFullAtlas = maxVerticalSize / Atlas2D_ElementSize;
	Int32 totalElements = ATLAS2D_ROWS_COUNT * ATLAS2D_ELEMENTS_PER_ROW;

	Int32 atlasesCount = Math_CeilDiv(totalElements, elementsPerFullAtlas);
	Atlas1D_ElementsPerAtlas = min(elementsPerFullAtlas, totalElements);
	Int32 atlas1DHeight = Math_NextPowOf2(Atlas1D_ElementsPerAtlas * Atlas2D_ElementSize);

	Atlas1D_Convert2DTo1D(atlasesCount, atlas1DHeight);
	Atlas1D_ElementsPerBitmap = atlas1DHeight / Atlas2D_ElementSize;
	Atlas1D_InvElementSize = 1.0f / Atlas1D_ElementsPerBitmap;
}

Int32 Atlas1D_UsedAtlasesCount(void) {
	TextureLoc maxTexLoc = 0;
	Int32 i;

	for (i = 0; i < Array_Elems(Block_Textures); i++) {
		maxTexLoc = max(maxTexLoc, Block_Textures[i]);
	}
	return Atlas1D_Index(maxTexLoc) + 1;
}

void Atlas1D_Free(void) {
	Int32 i;
	for (i = 0; i < Atlas1D_Count; i++) {
		Gfx_DeleteTexture(&Atlas1D_TexIds[i]);
	}
}