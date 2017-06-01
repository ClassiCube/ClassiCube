#ifndef CS_SKYBOXRENDERER_H
#define CS_SKYBOXRENDERER_H
#include "Typedefs.h"
#include "Stream.h"
#include "WorldEvents.h"
#include "GameStructs.h"
/* Renders a skybox.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Creates game component implementation. */
IGameComponent SkyboxRenderer_MakeGameComponent(void);

/* Renders skybox in the world. */
void SkyboxRenderer_Render(Real64 deltaTime);


static void SkyboxRenderer_Init(void);
static void SkyboxRenderer_Reset(void);
static void SkyboxRenderer_Free(void);


static void SkyboxRenderer_MakeVb(void);


static void SkyboxRenderer_EnvVariableChanged(EnvVar envVar);
static void SkyboxRenderer_TexturePackChanged(void);
static void SkyboxRenderer_FileChanged(Stream* stream);
static void SkyboxRenderer_ContextLost(void);
static void SkyboxRenderer_ContextRecreated(void);
#endif