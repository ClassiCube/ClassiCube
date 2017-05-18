#ifndef CS_GAME_H
#define CS_GAME_H
#include "Typedefs.h"
/* Represents the game.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Called when projection matrix is updated. */
void Game_UpdateProjection();

/* Updates the block at the given coordinates. */
void Game_UpdateBlock(Int32 x, Int32 y, Int32 z, BlockID block);

/* Performs thread sleeping to limit the FPS. */
static void Game_LimitFPS();

/* Frees all resources held by the game. */
void Game_Free();
#endif