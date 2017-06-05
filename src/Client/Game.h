#ifndef CS_GAME_H
#define CS_GAME_H
#include "Typedefs.h"
#include "Stream.h"
#include "GraphicsEnums.h"
/* Represents the game.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Called when projection matrix is updated. */
void Game_UpdateProjection(void);

/* Updates the block at the given coordinates. */
void Game_UpdateBlock(Int32 x, Int32 y, Int32 z, BlockID block);

/* Updates the given texture. */
void Game_UpdateTexture(GfxResourceID* texId, Stream* src, bool setSkinType);

/* Performs thread sleeping to limit the FPS. */
static void Game_LimitFPS(void);

/* Frees all resources held by the game. */
void Game_Free(void);
#endif