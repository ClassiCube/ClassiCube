#ifndef CC_SKYBOXRENDERER_H
#define CC_SKYBOXRENDERER_H
#include "Typedefs.h"
#include "Stream.h"
#include "Events.h"
#include "GameStructs.h"
/* Renders a skybox.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Creates game component implementation. */
IGameComponent SkyboxRenderer_MakeGameComponent(void);
/* Renders skybox in the world. */
void SkyboxRenderer_Render(Real64 deltaTime);
/* Returns whether skybox should render in the world. */
bool SkyboxRenderer_ShouldRender(void);
#endif