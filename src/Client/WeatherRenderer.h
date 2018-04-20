#ifndef CC_WEATHERRENDERER_H
#define CC_WEATHERRENDERER_H
#include "GameStructs.h"
/* Renders rain and snow.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

Int16* Weather_Heightmap;
IGameComponent WeatherRenderer_MakeComponent(void);
void WeatherRenderer_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock);
void WeatherRenderer_Render(Real64 deltaTime);
#endif