#ifndef CC_ENVRENDERER_H
#define CC_ENVRENDERER_H
#include "Core.h"
/* Renders environment of the map. (clouds, sky, fog, map sides/edges, skybox, rain/snow)
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;

void EnvRenderer_RenderSky(Real64 deltaTime);
void EnvRenderer_RenderClouds(Real64 deltaTime);
void EnvRenderer_UpdateFog(void);

void EnvRenderer_RenderMapSides(Real64 delta);
void EnvRenderer_RenderMapEdges(Real64 delta);
void EnvRenderer_RenderSkybox(Real64 deltaTime);
bool EnvRenderer_ShouldRenderSkybox(void);

Int16* Weather_Heightmap;
void EnvRenderer_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock);
void EnvRenderer_RenderWeather(Real64 deltaTime);

bool EnvRenderer_Legacy, EnvRenderer_Minimal;
void EnvRenderer_UseMinimalMode(bool minimal);
void EnvRenderer_UseLegacyMode(bool legacy);
void EnvRenderer_MakeComponent(struct IGameComponent* comp);
#endif
