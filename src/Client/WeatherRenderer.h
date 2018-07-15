#ifndef CC_WEATHERRENDERER_H
#define CC_WEATHERRENDERER_H
#include "Typedefs.h"
/* Renders rain and snow.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct IGameComponent;

Int16* Weather_Heightmap;
void WeatherRenderer_MakeComponent(struct IGameComponent* comp);
void WeatherRenderer_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock);
void WeatherRenderer_Render(Real64 deltaTime);
#endif
