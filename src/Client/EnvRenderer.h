#ifndef CC_ENVRENDERER_H
#define CC_ENVRENDERER_H
#include "Typedefs.h"
#include "GameStructs.h"
#include "VertexStructs.h"
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
#endif