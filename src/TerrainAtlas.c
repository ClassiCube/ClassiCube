#include "TerrainAtlas.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Graphics.h"
#include "Platform.h"

Bitmap Atlas_Bitmap;
int Atlas_TileSize, Atlas_RowsCount;
int Atlas1D_Count, Atlas1D_TilesPerAtlas;
int Atlas1D_Mask, Atlas1D_Shift;
float Atlas1D_InvTileSize;
GfxResourceID Atlas1D_TexIds[ATLAS1D_MAX_ATLASES];

TextureRec Atlas1D_TexRec(TextureLoc texLoc, int uCount, int* index) {
	TextureRec rec;
	int y  = Atlas1D_RowId(texLoc);
	*index = Atlas1D_Index(texLoc);

	/* Adjust coords to be slightly inside - fixes issues with AMD/ATI cards */	
	rec.U1 = 0.0f; 
	rec.V1 = y * Atlas1D_InvTileSize;
	rec.U2 = (uCount - 1) + UV2_Scale;
	rec.V2 = rec.V1       + UV2_Scale * Atlas1D_InvTileSize;
	return rec;
}

static void Atlas_Convert2DTo1D(void) {
	int tileSize      = Atlas_TileSize;
	int tilesPerAtlas = Atlas1D_TilesPerAtlas;
	int atlasesCount  = Atlas1D_Count;
	Bitmap atlas1D;
	int atlasX, atlasY;
	int tile = 0, i, y;

	Platform_Log2("Loaded new atlas: %i bmps, %i per bmp", &atlasesCount, &tilesPerAtlas);
	Bitmap_Allocate(&atlas1D, tileSize, tilesPerAtlas * tileSize);
	
	for (i = 0; i < atlasesCount; i++) {
		for (y = 0; y < tilesPerAtlas; y++, tile++) {
			atlasX = Atlas2D_TileX(tile) * tileSize;
			atlasY = Atlas2D_TileY(tile) * tileSize;

			Bitmap_CopyBlock(atlasX, atlasY, 0, y * tileSize,
				&Atlas_Bitmap, &atlas1D, tileSize);
		}
		Atlas1D_TexIds[i] = Gfx_CreateTexture(&atlas1D, true, Gfx_Mipmaps);
	}
	Mem_Free(atlas1D.Scan0);
}

static void Atlas_Update1D(void) {
	int maxAtlasHeight, maxTilesPerAtlas, maxTiles;

	maxAtlasHeight   = min(4096, Gfx_MaxTexHeight);
	maxTilesPerAtlas = maxAtlasHeight / Atlas_TileSize;
	maxTiles         = Atlas_RowsCount * ATLAS2D_TILES_PER_ROW;

	Atlas1D_TilesPerAtlas = min(maxTilesPerAtlas, maxTiles);
	Atlas1D_Count = Math_CeilDiv(maxTiles, Atlas1D_TilesPerAtlas);

	Atlas1D_InvTileSize = 1.0f / Atlas1D_TilesPerAtlas;
	Atlas1D_Mask  = Atlas1D_TilesPerAtlas - 1;
	Atlas1D_Shift = Math_Log2(Atlas1D_TilesPerAtlas);
}

void Atlas_Update(Bitmap* bmp) {
	Atlas_Bitmap    = *bmp;
	Atlas_TileSize  = bmp->Width  / ATLAS2D_TILES_PER_ROW;
	Atlas_RowsCount = bmp->Height / Atlas_TileSize;
	Atlas_RowsCount = min(Atlas_RowsCount, ATLAS2D_MAX_ROWS_COUNT);

	Block_RecalculateAllSpriteBB();
	Atlas_Update1D();
	Atlas_Convert2DTo1D();
}

static GfxResourceID Atlas_LoadTile_Raw(TextureLoc texLoc, Bitmap* element) {
	int size = Atlas_TileSize;
	int x = Atlas2D_TileX(texLoc), y = Atlas2D_TileY(texLoc);
	if (y >= Atlas_RowsCount) return GFX_NULL;

	Bitmap_CopyBlock(x * size, y * size, 0, 0, &Atlas_Bitmap, element, size);
	return Gfx_CreateTexture(element, false, Gfx_Mipmaps);
}

GfxResourceID Atlas_LoadTile(TextureLoc texLoc) {
	int tileSize = Atlas_TileSize;
	Bitmap tile;
	GfxResourceID texId;
	uint8_t scan0[Bitmap_DataSize(64, 64)];

	/* Try to allocate bitmap on stack if possible */
	if (tileSize > 64) {
		Bitmap_Allocate(&tile, tileSize, tileSize);
		texId = Atlas_LoadTile_Raw(texLoc, &tile);
		Mem_Free(tile.Scan0);
		return texId;
	} else {	
		Bitmap_Create(&tile, tileSize, tileSize, scan0);
		return Atlas_LoadTile_Raw(texLoc, &tile);
	}
}

void Atlas_Free(void) {
	int i;
	Mem_Free(Atlas_Bitmap.Scan0);
	Atlas_Bitmap.Scan0 = NULL;

	for (i = 0; i < Atlas1D_Count; i++) {
		Gfx_DeleteTexture(&Atlas1D_TexIds[i]);
	}
}
