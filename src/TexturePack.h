#ifndef CC_TEXPACKS_H
#define CC_TEXPACKS_H
#include "Bitmap.h"
/* Contains everything relating to texture packs.
	- Extracting the textures from a .zip archive
	- Caching terrain atlases and texture packs to avoid redundant downloads
	- Terrain atlas (including breaking it down into multiple 1D atlases)
   Copyright 2014-2021 ClassiCube | Licensed under BSD-3 
*/

struct Stream;
struct HttpRequest;
struct IGameComponent;
extern struct IGameComponent Textures_Component;

/* Number of tiles in each row */
#define ATLAS2D_TILES_PER_ROW 16
#define ATLAS2D_MASK 15
#define ATLAS2D_SHIFT 4
/* Maximum supported number of rows in the atlas. */
#ifdef EXTENDED_TEXTURES
#define ATLAS2D_MAX_ROWS_COUNT 32
#else
#define ATLAS2D_MAX_ROWS_COUNT 16
#endif
/* Maximum possible number of 1D terrain atlases. (worst case, each 1D atlas only has 1 tile) */
#define ATLAS1D_MAX_ATLASES (ATLAS2D_TILES_PER_ROW * ATLAS2D_MAX_ROWS_COUNT)

CC_VAR extern struct _Atlas2DData {
	/* Bitmap that contains the textures of all tiles. */
	/* Tiles are indexed left to right, top to bottom. */
	struct Bitmap Bmp;
	/* Size of each tile in pixels. (default 16x16) */
	int TileSize;
	/* Number of rows in the atlas. (default 16, can be 32) */
	int RowsCount;
} Atlas2D;

CC_VAR extern struct _Atlas1DData {
	/* Number of 1D atlases the atlas was split into. */
	int Count;
	/* Number of tiles in each 1D atlas. */
	int TilesPerAtlas;
	/* Converts a tile id into 1D atlas index, and index within that atlas. */
	int Mask, Shift;
	/* Texture V coord that equals the size of one tile. (i.e. 1/Atlas1D.TilesPerAtlas) */
	/* NOTE: The texture U coord that equals the size of one tile is 1. */
	float InvTileSize;
	/* Textures for each 1D atlas. Only Atlas1D_Count of these are valid. */
	GfxResourceID TexIds[ATLAS1D_MAX_ATLASES];
} Atlas1D;

extern cc_string TexturePack_Url;
#define TEXPACKS_DIR "texpacks"

#define Atlas2D_TileX(texLoc) ((texLoc) &  ATLAS2D_MASK)  /* texLoc % ATLAS2D_TILES_PER_ROW */
#define Atlas2D_TileY(texLoc) ((texLoc) >> ATLAS2D_SHIFT) /* texLoc / ATLAS2D_TILES_PER_ROW */
/* Returns the index of the given tile id within a 1D atlas */
#define Atlas1D_RowId(texLoc) ((texLoc)  & Atlas1D.Mask)  /* texLoc % Atlas1D_TilesPerAtlas */
/* Returns the index of the 1D atlas within the array of 1D atlases that contains the given tile id */
#define Atlas1D_Index(texLoc) ((texLoc) >> Atlas1D.Shift) /* texLoc / Atlas1D_TilesPerAtlas */

/* Loads the given tile into a new separate texture. */
GfxResourceID Atlas2D_LoadTile(TextureLoc texLoc);
/* Attempts to change the terrain atlas. (bitmap containing textures for all blocks) */
cc_bool Atlas_TryChange(struct Bitmap* bmp);
/* Returns the UV rectangle of the given tile id in the 1D atlases. */
/* That is, returns U1/U2/V1/V2 coords that make up the tile in a 1D atlas. */
/* index is set to the index of the 1D atlas that the tile is in. */
TextureRec Atlas1D_TexRec(TextureLoc texLoc, int uCount, int* index);

/* Whether the given URL is in list of accepted URLs. */
cc_bool TextureCache_HasAccepted(const cc_string* url);
/* Whether the given URL is in list of denied URLs. */
cc_bool TextureCache_HasDenied(const cc_string* url);
/* Adds the given URL to list of accepted URLs, then saves it. */
/* Accepted URLs are loaded without prompting the user. */
void TextureCache_Accept(const cc_string* url);
/* Adds the given URL to list of denied URLs, then saves it. */
/* Denied URLs are never loaded. */
void TextureCache_Deny(const cc_string* url);
/* Clears the list of denied URLs, returning number removed. */
int TextureCache_ClearDenied(void);

/* ID of texture pack http request */
extern int TexturePack_ReqID;
/* Sets the filename of the default texture pack used. */
void TexturePack_SetDefault(const cc_string* texPack);
/* If TexturePack_Url is empty, extracts user's default texture pack. */
/* Otherwise extracts the cached texture pack for that URL. */
void TexturePack_ExtractCurrent(cc_bool forceReload);
/* Extracts a texture pack or terrain.png from given downloaded data. */
/* Also updates cached data associated with the given URL. */
void TexturePack_Apply(struct HttpRequest* item);
/* If url is empty, extracts default texture pack. */
/* Else tries extracting cached texture pack for the given URL, */
/* then asynchronously downloads the texture pack from the given URL. */
CC_API void TexturePack_Extract(const cc_string* url);
#endif
