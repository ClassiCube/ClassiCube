#ifndef CC_BORDERSRENDERER_H
#define CC_BORDERSRENDERER_H
#include "GameStructs.h"
/* Renders map sides and map edges (horizon) as large quads.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

IGameComponent BordersRenderer_MakeGameComponent(void);
bool BordersRenderer_Legacy;
void BordersRenderer_UseLegacyMode(bool legacy);
void BordersRenderer_RenderSides(Real64 delta);
void BordersRenderer_RenderEdges(Real64 delta);
#endif