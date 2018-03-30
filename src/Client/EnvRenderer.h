#ifndef CC_ENVRENDERER_H
#define CC_ENVRENDERER_H
#include "GameStructs.h"
/* Renders environment of the map. (clouds, sky, fog)
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

IGameComponent EnvRenderer_MakeGameComponent(void);
bool EnvRenderer_Legacy;
bool EnvRenderer_Minimal;
void EnvRenderer_UseLegacyMode(bool legacy);
void EnvRenderer_UseMinimalMode(bool minimal);
void EnvRenderer_Render(Real64 delta);
#endif