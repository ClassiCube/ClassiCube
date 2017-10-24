#ifndef CC_WEATHERRENDERER_H
#define CC_WEATHERRENDERER_H
#include "Typedefs.h"
#include "Stream.h"
#include "GameStructs.h"
/* Renders rain and snow.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

Int16* Weather_Heightmap;
/* Creates game component implementation. */
IGameComponent WeatherRenderer_MakeGameComponent(void);
/* Invokes to update state of rain/snow heightmap when a block is changed in the world. */
void WeatherRenderer_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock);
/* Renders weather in the world. */
void WeatherRenderer_Render(Real64 deltaTime);
#endif