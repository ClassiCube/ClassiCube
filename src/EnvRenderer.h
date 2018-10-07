#ifndef CC_ENVRENDERER_H
#define CC_ENVRENDERER_H
#include "Core.h"
/* Renders environment of the map. (clouds, sky, fog, map sides/edges, skybox, rain/snow)
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;

void EnvRenderer_RenderSky(double deltaTime);
void EnvRenderer_RenderClouds(double deltaTime);
void EnvRenderer_UpdateFog(void);

void EnvRenderer_RenderMapSides(double delta);
void EnvRenderer_RenderMapEdges(double delta);
void EnvRenderer_RenderSkybox(double deltaTime);
bool EnvRenderer_ShouldRenderSkybox(void);

int16_t* Weather_Heightmap;
void EnvRenderer_OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock);
void EnvRenderer_RenderWeather(double deltaTime);

bool EnvRenderer_Legacy, EnvRenderer_Minimal;
void EnvRenderer_UseMinimalMode(bool minimal);
void EnvRenderer_UseLegacyMode(bool legacy);
void EnvRenderer_MakeComponent(struct IGameComponent* comp);
#endif
