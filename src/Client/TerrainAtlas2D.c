#include "TerrainAtlas2D.h"
#include "Platform.h"
#include "Block.h"
#include "GraphicsAPI.h"

void Atlas2D_UpdateState(Bitmap bmp) {
	Atlas2D_Bitmap = bmp;
	Atlas2D_ElementSize = bmp.Width / Atlas2D_ElementsPerRow;
	Block_RecalculateSpriteBB(&bmp);
}

Int32 Atlas2D_LoadTextureElement(Int32 index) {
	Int32 size = Atlas2D_ElementSize;
	Bitmap element = Platform.CreateBmp(size, size);

	int x = index % Atlas2D_ElementsPerRow, y = index / Atlas2D_ElementsPerRow;
	Bitmap_CopyBlock(x * size, y * size, 0, 0, 
		&Atlas2D_Bitmap, &element, size);
	return Gfx_CreateTexture(&element, true);
}

void Atlas2D_Free() {
	if (Atlas2D_Bitmap.Scan0 == NULL) return;
	Platform_MemFree(Atlas2D_Bitmap.Scan0);
}