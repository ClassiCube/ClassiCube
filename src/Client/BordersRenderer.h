#ifndef CS_BORDERSRENDERER_H
#define CS_BORDERSRENDERER_H
#include "Typedefs.h"
#include "GameStructs.h"
#include "VertexStructs.h"
#include "WorldEvents.h"
#include "GraphicsEnums.h"
/* Renders map sides and map edges (horizon) as large quads.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Creates game component implementation. */
IGameComponent BordersRenderer_MakeGameComponent(void);

/* Whether borders renderer breaks quads up into smaller 512x512 quads. */
bool BordersRenderer_Legacy;

/* Sets whather legacy renderering mode is used. */
void BordersRenderer_UseLegacyMode(bool legacy);

/* Renders the sides of the map. */
void BordersRenderer_RenderSides(Real64 delta);

/* Renders the horizon/edges of the map. */
void BordersRenderer_RenderEdges(Real64 delta);


static void BordersRenderer_Init(void);
static void BordersRenderer_Free(void);
static void BordersRenderer_Reset(void);


static void BordersRenderer_EnvVariableChanged(EnvVar envVar);
static void BordersRenderer_ContextLost(void);
static void BordersRenderer_ContextRecreated(void);


static void BordersRenderer_ResetTextures(void);

static void BordersRenderer_ResetSidesAndEdges(void);

static void BordersRenderer_ResetSides(void);

static void BordersRenderer_ResetEdges(void);


static void BordersRenderer_RebuildSides(Int32 y, Int32 axisSize);

static void BordersRenderer_RebuildEdges(Int32 y, Int32 axisSize);

static void BordersRenderer_DrawX(Int32 x, Int32 z1, Int32 z2, Int32 y1, Int32 y2, Int32 axisSize, PackedCol col, VertexP3fT2fC4b** v);

static void BordersRenderer_DrawZ(Int32 z, Int32 x1, Int32 x2, Int32 y1, Int32 y2, Int32 axisSize, PackedCol col, VertexP3fT2fC4b** v);

static void BordersRenderer_DrawY(Int32 x1, Int32 z1, Int32 x2, Int32 z2, Real32 y, Int32 axisSize, PackedCol col, Real32 offset, Real32 yOffset, VertexP3fT2fC4b** v);


static void BordersRenderer_CalculateRects(Int32 extent);

static void BordersRenderer_MakeTexture(GfxResourceID* texId, TextureLoc* lastTexLoc, BlockID block);
#endif