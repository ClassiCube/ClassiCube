#ifndef CS_ENVRENDERER_H
#define CS_ENVRENDERER_H
#include "Typedefs.h"
#include "GameStructs.h"
#include "VertexStructs.h"
#include "WorldEvents.h"
/* Renders environment of the map. (clouds, sky, fog)
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Creates game component implementation. */
IGameComponent EnvRenderer_MakeGameComponent(void);

/* Whether environment renderer breaks quads up into smaller 512x512 quads. */
bool EnvRenderer_Legacy;

/* Whether environment renderer does not render sky, clouds, or smooth fog. */
bool EnvRenderer_Minimal;

/* Sets whather legacy renderering mode is used. */
void EnvRenderer_UseLegacyMode(bool legacy);

/* Sets whather minimal renderering mode is used. */
void EnvRenderer_UseMinimalMode(bool minimal);

/* Renders the environemnt of the map. */
void EnvRenderer_Render(Real64 delta);

static void EnvRenderer_Init(void);
static void EnvRenderer_Reset(void);
static void EnvRenderer_OnNewMapLoaded(void);
static void EnvRenderer_Free(void);

static BlockID EnvRenderer_BlockOn(Real32* fogDensity, PackedCol* fogCol);

static Real32 EnvRenderer_BlendFactor(Real32 x);


static void EnvRenderer_EnvVariableChanged(EnvVar envVar);
static void EnvRenderer_ContextLost(void);
static void EnvRenderer_ContextRecreated(void);


static void EnvRenderer_RenderMainEnv(Real64 deltaTime);

static void EnvRenderer_ResetAllEnv(void);

static void EnvRenderer_RenderClouds(Real64 delta);

static void EnvRenderer_UpdateFog(void);

static void EnvRenderer_ResetClouds(void);

static void EnvRenderer_ResetSky(void);

static void EnvRenderer_RebuildClouds(Int32 extent, Int32 axisSize);

static void EnvRenderer_RebuildSky(Int32 extent, Int32 axisSize);

static void EnvRenderer_DrawSkyY(Int32 x1, Int32 z1, Int32 x2, Int32 z2, Int32 y, Int32 axisSize, PackedCol col, VertexP3fC4b* vertices);

static void EnvRenderer_DrawCloudsY(Int32 x1, Int32 z1, Int32 x2, Int32 z2, Int32 y, Int32 axisSize, PackedCol col, VertexP3fT2fC4b* vertices);
#endif