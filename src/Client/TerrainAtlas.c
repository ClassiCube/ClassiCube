#include "TerrainAtlas.h"
#include "Bitmap.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "GraphicsAPI.h"
#include "Platform.h"

void Atlas2D_UpdateState(Bitmap* bmp) {
	Atlas2D_Bitmap = *bmp;
	Atlas2D_TileSize = bmp->Width / ATLAS2D_TILES_PER_ROW;
	Block_RecalculateSpriteBB();
}

static GfxResourceID Atlas2D_LoadTextureElement_Raw(TextureLoc texLoc, Bitmap* element) {
	Int32 size = Atlas2D_TileSize;
	Int32 x = Atlas2D_TileX(texLoc), y = Atlas2D_TileY(texLoc);
	Bitmap_CopyBlock(x * size, y * size, 0, 0, &Atlas2D_Bitmap, element, size);

	return Gfx_CreateTexture(element, false, Gfx_Mipmaps);
}

GfxResourceID Atlas2D_LoadTile(TextureLoc texLoc) {
	Int32 size = Atlas2D_TileSize;
	Bitmap tile;

	/* Try to allocate bitmap on stack if possible */
	if (size > 64) {
		Bitmap_Allocate(&tile, size, size);
		GfxResourceID texId = Atlas2D_LoadTextureElement_Raw(texLoc, &tile);
		Platform_MemFree(&tile.Scan0);
		return texId;
	} else {
		UInt8 scan0[Bitmap_DataSize(64, 64)];
		Bitmap_Create(&tile, size, size, scan0);
		return Atlas2D_LoadTextureElement_Raw(texLoc, &tile);
	}
}

void Atlas2D_Free(void) {
	Platform_MemFree(&Atlas2D_Bitmap.Scan0);
}


TextureRec Atlas1D_TexRec(TextureLoc texLoc, Int32 uCount, Int32* index) {
	*index  = Atlas1D_Index(texLoc);
	Int32 y = Atlas1D_RowId(texLoc);

	/* Adjust coords to be slightly inside - fixes issues with AMD/ATI cards. */
	TextureRec rec;
	rec.U1 = 0.0f; 
	rec.V1 = y * Atlas1D_InvTileSize;
	rec.U2 = (uCount - 1) + UV2_Scale;
	rec.V2 = rec.V1 + UV2_Scale * Atlas1D_InvTileSize;
	return rec;
}

static void Atlas1D_Make1DTexture(Int32 i, Int32 atlas1DHeight, Int32* index) {
	Int32 tileSize = Atlas2D_TileSize;
	Bitmap atlas1D;
	Bitmap_Allocate(&atlas1D, tileSize, atlas1DHeight);

	Int32 index1D;
	for (index1D = 0; index1D < Atlas1D_TilesPerAtlas; index1D++) {
		Int32 atlasX = (*index % ATLAS2D_TILES_PER_ROW) * tileSize;
		Int32 atlasY = (*index / ATLAS2D_TILES_PER_ROW) * tileSize;

		Bitmap_CopyBlock(atlasX, atlasY, 0, index1D * tileSize,
			&Atlas2D_Bitmap, &atlas1D, tileSize);
		(*index)++;
	}

	Atlas1D_TexIds[i] = Gfx_CreateTexture(&atlas1D, true, Gfx_Mipmaps);
	Platform_MemFree(&atlas1D.Scan0);
}

static void Atlas1D_Convert2DTo1D(Int32 atlasesCount, Int32 atlas1DHeight) {
	Atlas1D_Count = atlasesCount;
	Platform_Log2("Loaded new atlas: %i bmps, %i per bmp", &atlasesCount, &Atlas1D_TilesPerAtlas);

	Int32 index = 0, i;
	for (i = 0; i < atlasesCount; i++) {
		Atlas1D_Make1DTexture(i, atlas1DHeight, &index);
	}
}

void Atlas1D_UpdateState(void) {
	Int32 maxAtlasHeight = min(4096, Gfx_MaxTextureDimensions);
	Int32 maxTilesPerAtlas = maxAtlasHeight / Atlas2D_TileSize;
	Int32 maxTiles = ATLAS2D_ROWS_COUNT * ATLAS2D_TILES_PER_ROW;

	Atlas1D_TilesPerAtlas = min(maxTilesPerAtlas, maxTiles);
	Int32 atlasesCount = Math_CeilDiv(maxTiles, Atlas1D_TilesPerAtlas);
	Int32 atlasHeight = Atlas1D_TilesPerAtlas * Atlas2D_TileSize;

	Atlas1D_InvTileSize = 1.0f / Atlas1D_TilesPerAtlas;
	Atlas1D_Convert2DTo1D(atlasesCount, atlasHeight);
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
