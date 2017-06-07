#include "TerrainAtlas2D.h"
#include "Platform.h"
#include "Block.h"
#include "GraphicsAPI.h"

void Atlas2D_UpdateState(Bitmap bmp) {
	Atlas2D_Bitmap = bmp;
	Atlas2D_ElementSize = bmp.Width / Atlas2D_ElementsPerRow;
	Block_RecalculateSpriteBB();
}

Int32 Atlas2D_LoadTextureElement(TextureLoc texLoc) {
	Int32 size = Atlas2D_ElementSize;
	Bitmap element;

	/* Try to allocate bitmap on stack if possible */
	if (size > 64) {
		Bitmap_Allocate(&element, size, size);
		if (element.Scan0 == NULL) {
			ErrorHandler_Fail("Atlas2D_LoadTextureElement - failed to allocate memory");
		}

		Int32 texId = Atlas2D_LoadTextureElement_Raw(texLoc, &element);
		Platform_MemFree(element.Scan0);
		return texId;
	} else {
		// TODO: does this even work??
		UInt8 scan0[Bitmap_DataSize(64, 64)];
		Bitmap_Create(&element, size, size, 64 * Bitmap_PixelBytesSize, scan0);
		return Atlas2D_LoadTextureElement_Raw(texLoc, &element);
	}
}

Int32 Atlas2D_LoadTextureElement_Raw(TextureLoc texLoc, Bitmap* element) {
	Int32 size = Atlas2D_ElementSize;
	Int32 x = texLoc % Atlas2D_ElementsPerRow, y = texLoc / Atlas2D_ElementsPerRow;
	Bitmap_CopyBlock(x * size, y * size, 0, 0,
		&Atlas2D_Bitmap, element, size);

	return Gfx_CreateTexture(element, true);
}

void Atlas2D_Free(void) {
	if (Atlas2D_Bitmap.Scan0 == NULL) return;
	Platform_MemFree(Atlas2D_Bitmap.Scan0);
}