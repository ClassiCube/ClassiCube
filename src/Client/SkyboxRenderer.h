#ifndef CC_SKYBOXRENDERER_H
#define CC_SKYBOXRENDERER_H
#include "GameStructs.h"
/* Renders a skybox.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

IGameComponent SkyboxRenderer_MakeGameComponent(void);
void SkyboxRenderer_Render(Real64 deltaTime);
bool SkyboxRenderer_ShouldRender(void);
#endif