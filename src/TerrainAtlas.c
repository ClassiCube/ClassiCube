#include "TerrainAtlas.h"
#include "Bitmap.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "GraphicsAPI.h"
#include "Platform.h"

void Atlas2D_UpdateState(Bitmap* bmp) {
	Atlas2D_Bitmap    = *bmp;
	Atlas2D_TileSize  = bmp->Width  / ATLAS2D_TILES_PER_ROW;
	Atlas2D_RowsCount = bmp->Height / Atlas2D_TileSize;
	Atlas2D_RowsCount = min(Atlas2D_RowsCount, ATLAS2D_MAX_ROWS_COUNT);
	Block_RecalculateSpriteBB();
}

static GfxResourceID Atlas2D_LoadTextureElement_Raw(TextureLoc texLoc, Bitmap* element) {
	int size = Atlas2D_TileSize;
	int x = Atlas2D_TileX(texLoc), y = Atlas2D_TileY(texLoc);
	if (y >= Atlas2D_RowsCount) return GFX_NULL;

	Bitmap_CopyBlock(x * size, y * size, 0, 0, &Atlas2D_Bitmap, element, size);
	return Gfx_CreateTexture(element, false, Gfx_Mipmaps);
}

GfxResourceID Atlas2D_LoadTile(TextureLoc texLoc) {
	int size = Atlas2D_TileSize;
	Bitmap tile;

	/* Try to allocate bitmap on stack if possible */
	if (size > 64) {
		Bitmap_Allocate(&tile, size, size);
		GfxResourceID texId = Atlas2D_LoadTextureElement_Raw(texLoc, &tile);
		Mem_Free(tile.Scan0);
		return texId;
	} else {
		uint8_t scan0[Bitmap_DataSize(64, 64)];
		Bitmap_Create(&tile, size, size, scan0);
		return Atlas2D_LoadTextureElement_Raw(texLoc, &tile);
	}
}

void Atlas2D_Free(void) {
	Mem_Free(Atlas2D_Bitmap.Scan0);
	Atlas2D_Bitmap.Scan0 = NULL;
}


TextureRec Atlas1D_TexRec(TextureLoc texLoc, int uCount, int* index) {
	*index = Atlas1D_Index(texLoc);
	int y  = Atlas1D_RowId(texLoc);

	/* Adjust coords to be slightly inside - fixes issues with AMD/ATI cards */
	TextureRec rec;
	rec.U1 = 0.0f; 
	rec.V1 = y * Atlas1D_InvTileSize;
	rec.U2 = (uCount - 1) + UV2_Scale;
	rec.V2 = rec.V1       + UV2_Scale * Atlas1D_InvTileSize;
	return rec;
}

static void Atlas1D_Convert2DTo1D(int atlasesCount, int atlas1DHeight) {
	Atlas1D_Count = atlasesCount;
	Platform_Log2("Loaded new atlas: %i bmps, %i per bmp", &atlasesCount, &Atlas1D_TilesPerAtlas);

	int tileSize = Atlas2D_TileSize;
	Bitmap atlas1D;
	Bitmap_Allocate(&atlas1D, tileSize, atlas1DHeight);

	int tile = 0, i, y;
	for (i = 0; i < atlasesCount; i++) {
		for (y = 0; y < Atlas1D_TilesPerAtlas; y++, tile++) {
			int atlasX = Atlas2D_TileX(tile) * tileSize;
			int atlasY = Atlas2D_TileY(tile) * tileSize;

			Bitmap_CopyBlock(atlasX, atlasY, 0, y * tileSize,
				&Atlas2D_Bitmap, &atlas1D, tileSize);
		}
		Atlas1D_TexIds[i] = Gfx_CreateTexture(&atlas1D, true, Gfx_Mipmaps);
	}
	Mem_Free(atlas1D.Scan0);
}

void Atlas1D_UpdateState(void) {
	int maxAtlasHeight   = min(4096, Gfx_MaxTexHeight);
	int maxTilesPerAtlas = maxAtlasHeight / Atlas2D_TileSize;
	int maxTiles         = Atlas2D_RowsCount * ATLAS2D_TILES_PER_ROW;

	Atlas1D_TilesPerAtlas = min(maxTilesPerAtlas, maxTiles);
	int atlasesCount    = Math_CeilDiv(maxTiles, Atlas1D_TilesPerAtlas);
	int atlasHeight     = Atlas1D_TilesPerAtlas * Atlas2D_TileSize;

	Atlas1D_InvTileSize = 1.0f / Atlas1D_TilesPerAtlas;
	Atlas1D_Mask        = Atlas1D_TilesPerAtlas - 1;
	Atlas1D_Shift       = Math_Log2(Atlas1D_TilesPerAtlas);

	Atlas1D_Convert2DTo1D(atlasesCount, atlasHeight);
}

int Atlas1D_UsedAtlasesCount(void) {
	TextureLoc maxTexLoc = 0;
	int i;

	for (i = 0; i < Array_Elems(Block_Textures); i++) {
		maxTexLoc = max(maxTexLoc, Block_Textures[i]);
	}
	return Atlas1D_Index(maxTexLoc) + 1;
}

void Atlas1D_Free(void) {
	int i;
	for (i = 0; i < Atlas1D_Count; i++) {
		Gfx_DeleteTexture(&Atlas1D_TexIds[i]);
	}
}
