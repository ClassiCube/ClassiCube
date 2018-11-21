#include "TerrainAtlas.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Graphics.h"
#include "Platform.h"

void Atlas2D_UpdateState(Bitmap* bmp) {
	Atlas2D_Bitmap    = *bmp;
	Atlas2D_TileSize  = bmp->Width  / ATLAS2D_TILES_PER_ROW;
	Atlas2D_RowsCount = bmp->Height / Atlas2D_TileSize;
	Atlas2D_RowsCount = min(Atlas2D_RowsCount, ATLAS2D_MAX_ROWS_COUNT);
	Block_RecalculateAllSpriteBB();
}

static GfxResourceID Atlas2D_LoadTextureElement_Raw(TextureLoc texLoc, Bitmap* element) {
	int size = Atlas2D_TileSize;
	int x = Atlas2D_TileX(texLoc), y = Atlas2D_TileY(texLoc);
	if (y >= Atlas2D_RowsCount) return GFX_NULL;

	Bitmap_CopyBlock(x * size, y * size, 0, 0, &Atlas2D_Bitmap, element, size);
	return Gfx_CreateTexture(element, false, Gfx_Mipmaps);
}

GfxResourceID Atlas2D_LoadTile(TextureLoc texLoc) {
	int tileSize = Atlas2D_TileSize;
	Bitmap tile;
	GfxResourceID texId;
	uint8_t scan0[Bitmap_DataSize(64, 64)];

	/* Try to allocate bitmap on stack if possible */
	if (tileSize > 64) {
		Bitmap_Allocate(&tile, tileSize, tileSize);
		texId = Atlas2D_LoadTextureElement_Raw(texLoc, &tile);
		Mem_Free(tile.Scan0);
		return texId;
	} else {	
		Bitmap_Create(&tile, tileSize, tileSize, scan0);
		return Atlas2D_LoadTextureElement_Raw(texLoc, &tile);
	}
}

void Atlas2D_Free(void) {
	Mem_Free(Atlas2D_Bitmap.Scan0);
	Atlas2D_Bitmap.Scan0 = NULL;
}


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

static void Atlas1D_Convert2DTo1D(int atlasesCount, int atlas1DHeight) {
	int tileSize = Atlas2D_TileSize;
	Bitmap atlas1D;
	int atlasX, atlasY;
	int tile = 0, i, y;

	Atlas1D_Count = atlasesCount;
	Platform_Log2("Loaded new atlas: %i bmps, %i per bmp", &atlasesCount, &Atlas1D_TilesPerAtlas);
	Bitmap_Allocate(&atlas1D, tileSize, atlas1DHeight);
	
	for (i = 0; i < atlasesCount; i++) {
		for (y = 0; y < Atlas1D_TilesPerAtlas; y++, tile++) {
			atlasX = Atlas2D_TileX(tile) * tileSize;
			atlasY = Atlas2D_TileY(tile) * tileSize;

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
	TextureLoc maxLoc = 0;
	int i;

	for (i = 0; i < Array_Elems(Block_Textures); i++) {
		maxLoc = max(maxLoc, Block_Textures[i]);
	}
	return Atlas1D_Index(maxLoc) + 1;
}

void Atlas1D_Free(void) {
	int i;
	for (i = 0; i < Atlas1D_Count; i++) {
		Gfx_DeleteTexture(&Atlas1D_TexIds[i]);
	}
}
