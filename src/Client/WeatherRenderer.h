#ifndef CS_WEATHERRENDERER_H
#define CS_WEATHERRENDERER_H
#include "Typedefs.h"
#include "Stream.h"
#include "GameStructs.h"
/* Renders rain and snow.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Creates game component implementation. */
IGameComponent WeatherRenderer_MakeGameComponent(void);

/* Invokes to update state of rain/snow heightmap when a block is changed in the world. */
void WeatherRenderer_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock);

/* Renders weather in the world. */
void WeatherRenderer_Render(Real64 deltaTime);


static void WeatherRenderer_Init(void);
static void WeatherRenderer_Reset(void);
static void WeatherRenderer_Free(void);


static void WeatherRenderer_InitHeightmap(void);

static Real32 WeatherRenderer_AlphaAt(Real32 x);

static Real32 WeatherRenderer_RainHeight(Int32 x, Int32 z);

static Int32 WeatherRenderer_CalcHeightAt(Int32 x, Int32 maxY, Int32 z, Int32 index);


static void WeatherRenderer_FileChanged(Stream* stream);
static void WeatherRenderer_ContextLost(void);
static void WeatherRenderer_ContextRecreated(void);
#endif