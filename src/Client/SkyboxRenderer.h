#ifndef CC_SKYBOXRENDERER_H
#define CC_SKYBOXRENDERER_H
#include "Typedefs.h"
/* Renders a skybox.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct IGameComponent;
void SkyboxRenderer_MakeComponent(struct IGameComponent* comp);
void SkyboxRenderer_Render(Real64 deltaTime);
bool SkyboxRenderer_ShouldRender(void);
#endif
