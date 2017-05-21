#ifndef CS_SKYBOXRENDERER_H
#define CS_SKYBOXRENDERER_H
#include "Typedefs.h"
#include "Stream.h"
#include "WorldEvents.h"
/* Renders a skybox.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Renders skybox in the world. */
void SkyboxRenderer_Render(Real64 deltaTime);


static void SkyboxRenderer_Init();

static void SkyboxRenderer_Reset();

static void SkyboxRenderer_Free();


static void SkyboxRenderer_MakeVb();


static void SkyboxRenderer_EnvVariableChanged(EnvVar envVar);

static void SkyboxRenderer_TexturePackChanged();

static void SkyboxRenderer_FileChanged(Stream* stream);

static void SkyboxRenderer_ContextLost();

static void SkyboxRenderer_ContextRecreated();
#endif