#ifndef CC_BORDERSRENDERER_H
#define CC_BORDERSRENDERER_H
#include "Typedefs.h"
#include "GameStructs.h"
#include "VertexStructs.h"
#include "Event.h"
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
#endif