#include "SkyboxRenderer.h"
#include "Camera.h"
#include "Event.h"
#include "Game.h"
#include "GraphicsAPI.h"
#include "PackedCol.h"
#include "VertexStructs.h"
#include "World.h"
#include "EnvRenderer.h"
#include "ExtMath.h"
#include "Stream.h"

GfxResourceID skybox_tex, skybox_vb;
#define SKYBOX_COUNT (6 * 4)
bool SkyboxRenderer_ShouldRender(void) { return skybox_tex && !EnvRenderer_Minimal; }

static void SkyboxRenderer_TexturePackChanged(void* obj) {
	Gfx_DeleteTexture(&skybox_tex);
}

static void SkyboxRenderer_FileChanged(void* obj, struct Stream* src) {
	if (String_CaselessEqualsConst(&src->Name, "skybox.png")) {
		Game_UpdateTexture(&skybox_tex, src, false);
	}
}

void SkyboxRenderer_Render(Real64 deltaTime) {
	if (skybox_vb == 0) return;
	Gfx_SetDepthWrite(false);
	Gfx_SetTexturing(true);
	Gfx_BindTexture(skybox_tex);
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);

	struct Matrix m = Matrix_Identity;
	struct Matrix rotX, rotY, view;

	/* Base skybox rotation */
	Real32 rotTime = (Real32)(Game_Accumulator * 2 * MATH_PI); /* So speed of 1 rotates whole skybox every second */
	Matrix_RotateY(&rotY, WorldEnv_SkyboxHorSpeed * rotTime);
	Matrix_MulBy(&m, &rotY);
	Matrix_RotateX(&rotX, WorldEnv_SkyboxVerSpeed * rotTime);
	Matrix_MulBy(&m, &rotX);

	/* Rotate around camera */
	Vector3 pos = Game_CurrentCameraPos, zero = Vector3_Zero;
	Game_CurrentCameraPos = zero;
	Camera_Active->GetView(&view);
	Matrix_MulBy(&m, &view);
	Game_CurrentCameraPos = pos;

	Gfx_LoadMatrix(&m);
	Gfx_BindVb(skybox_vb);
	Gfx_DrawVb_IndexedTris(SKYBOX_COUNT);

	Gfx_SetTexturing(false);
	Gfx_LoadMatrix(&Gfx_View);
	Gfx_SetDepthWrite(true);
}

VertexP3fT2fC4b skybox_vertices[SKYBOX_COUNT] = {
	/* Front quad */
	{  1, -1, -1, {0,0,0,0}, 0.25f, 1.00f }, { -1, -1, -1, {0,0,0,0}, 0.50f, 1.00f },
	{ -1,  1, -1, {0,0,0,0}, 0.50f, 0.50f }, {  1,  1, -1, {0,0,0,0}, 0.25f, 0.50f },
	/* Left quad */
	{  1, -1,  1, {0,0,0,0}, 0.00f, 1.00f }, {  1, -1, -1, {0,0,0,0}, 0.25f, 1.00f },
	{  1,  1, -1, {0,0,0,0}, 0.25f, 0.50f }, {  1,  1,  1, {0,0,0,0}, 0.00f, 0.50f },
	/* Back quad */
	{ -1, -1,  1, {0,0,0,0}, 0.75f, 1.00f }, {  1, -1,  1, {0,0,0,0}, 1.00f, 1.00f },
	{  1,  1,  1, {0,0,0,0}, 1.00f, 0.50f }, { -1,  1,  1, {0,0,0,0}, 0.75f, 0.50f },
	/* Right quad */
	{ -1, -1, -1, {0,0,0,0}, 0.50f, 1.00f }, { -1, -1,  1, {0,0,0,0}, 0.75f, 1.00f },
	{ -1,  1,  1, {0,0,0,0}, 0.75f, 0.50f }, { -1,  1, -1, {0,0,0,0}, 0.50f, 0.50f },
	/* Top quad */
	{ -1,  1, -1, {0,0,0,0}, 0.50f, 0.50f }, { -1,  1,  1, {0,0,0,0}, 0.50f, 0.00f },
	{  1,  1,  1, {0,0,0,0}, 0.25f, 0.00f }, {  1,  1, -1, {0,0,0,0}, 0.25f, 0.50f },
	/* Bottom quad */
	{ -1, -1, -1, {0,0,0,0}, 0.75f, 0.50f }, { -1, -1,  1, {0,0,0,0}, 0.75f, 0.00f },
	{  1, -1,  1, {0,0,0,0}, 0.50f, 0.00f }, {  1, -1, -1, {0,0,0,0}, 0.50f, 0.50f },
};

static void SkyboxRenderer_MakeVb(void) {
	if (Gfx_LostContext) return;
	Gfx_DeleteVb(&skybox_vb);

	Int32 i;
	for (i = 0; i < SKYBOX_COUNT; i++) { skybox_vertices[i].Col = WorldEnv_CloudsCol; }
	skybox_vb = Gfx_CreateVb(skybox_vertices, VERTEX_FORMAT_P3FT2FC4B, SKYBOX_COUNT);
}

static void SkyboxRenderer_ContextLost(void* obj) { Gfx_DeleteVb(&skybox_vb); }
static void SkyboxRenderer_ContextRecreated(void* obj) { SkyboxRenderer_MakeVb(); }

static void SkyboxRenderer_EnvVariableChanged(void* obj, Int32 envVar) {
	if (envVar != ENV_VAR_CLOUDS_COL) return;
	SkyboxRenderer_MakeVb();
}

static void SkyboxRenderer_Init(void) {
	Event_RegisterStream(&TextureEvents_FileChanged, NULL, SkyboxRenderer_FileChanged);
	Event_RegisterVoid(&TextureEvents_PackChanged,   NULL, SkyboxRenderer_TexturePackChanged);
	Event_RegisterInt(&WorldEvents_EnvVarChanged,  NULL, SkyboxRenderer_EnvVariableChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,       NULL, SkyboxRenderer_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,  NULL, SkyboxRenderer_ContextRecreated);
}

static void SkyboxRenderer_Reset(void) { Gfx_DeleteTexture(&skybox_tex); }

static void SkyboxRenderer_Free(void) {
	Gfx_DeleteTexture(&skybox_tex);
	SkyboxRenderer_ContextLost(NULL);

	Event_UnregisterStream(&TextureEvents_FileChanged, NULL, SkyboxRenderer_FileChanged);
	Event_UnregisterVoid(&TextureEvents_PackChanged,   NULL, SkyboxRenderer_TexturePackChanged);
	Event_UnregisterInt(&WorldEvents_EnvVarChanged,  NULL, SkyboxRenderer_EnvVariableChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,       NULL, SkyboxRenderer_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,  NULL, SkyboxRenderer_ContextRecreated);
}

void SkyboxRenderer_MakeComponent(struct IGameComponent* comp) {
	comp->Init = SkyboxRenderer_Init;
	comp->Free = SkyboxRenderer_Free;
	comp->OnNewMap = SkyboxRenderer_MakeVb; /* Need to recreate colour component of vertices */
	comp->Reset = SkyboxRenderer_Reset;
}
