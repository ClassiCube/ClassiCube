#include "SkyboxRenderer.h"
#include "Camera.h"
#include "EventHandler.h"
#include "Game.h"
#include "GameProps.h"
#include "GraphicsAPI.h"
#include "GraphicsEnums.h"
#include "MiscEvents.h"
#include "PackedCol.h"
#include "TextureRec.h"
#include "VertexStructs.h"
#include "WorldEnv.h"
#include "EnvRenderer.h"

GfxResourceID skybox_tex, skybox_vb = -1;
#define skybox_count (6 * 4)

IGameComponent SkyboxRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = SkyboxRenderer_Init;
	comp.Free = SkyboxRenderer_Free;
	comp.OnNewMap = SkyboxRenderer_MakeVb; /* Need to recreate colour component of vertices */
	comp.Reset = SkyboxRenderer_Reset;
	return comp;
}

bool SkyboxRenderer_ShouldRender(void) {
	return skybox_tex > 0 && !EnvRenderer_Minimal;
}

void SkyboxRenderer_Init(void) {
	EventHandler_RegisterStream(TextureEvents_FileChanged, &SkyboxRenderer_FileChanged);
	EventHandler_RegisterVoid(TextureEvents_PackChanged, &SkyboxRenderer_TexturePackChanged);
	EventHandler_RegisterInt32(WorldEvents_EnvVarChanged, &SkyboxRenderer_EnvVariableChanged);
	EventHandler_RegisterVoid(Gfx_ContextLost, &SkyboxRenderer_ContextLost);
	EventHandler_RegisterVoid(Gfx_ContextRecreated, &SkyboxRenderer_ContextRecreated);
}

void SkyboxRenderer_Reset(void) { Gfx_DeleteTexture(&skybox_tex); }

void SkyboxRenderer_Free(void) {
	Gfx_DeleteTexture(&skybox_tex);
	SkyboxRenderer_ContextLost();

	EventHandler_UnregisterStream(TextureEvents_FileChanged, &SkyboxRenderer_FileChanged);
	EventHandler_UnregisterVoid(TextureEvents_PackChanged, &SkyboxRenderer_TexturePackChanged);
	EventHandler_UnregisterInt32(WorldEvents_EnvVarChanged, &SkyboxRenderer_EnvVariableChanged);
	EventHandler_UnregisterVoid(Gfx_ContextLost, &SkyboxRenderer_ContextLost);
	EventHandler_UnregisterVoid(Gfx_ContextRecreated, &SkyboxRenderer_ContextRecreated);
}

void SkyboxRenderer_EnvVariableChanged(EnvVar envVar) {
	if (envVar != EnvVar_CloudsCol) return;
	SkyboxRenderer_MakeVb();
}

void SkyboxRenderer_TexturePackChanged(void) {
	Gfx_DeleteTexture(&skybox_tex);
}

void SkyboxRenderer_FileChanged(Stream* src) {
	String skybox = String_FromConstant("skybox.png");
	if (String_Equals(&src->Name, &skybox)) {
		Game_UpdateTexture(&skybox_tex, src, false);
	}
}

void SkyboxRenderer_Render(Real64 deltaTime) {
	if (skybox_vb == -1) return;
	Gfx_SetDepthWrite(false);
	Gfx_SetTexturing(false);
	Gfx_BindTexture(skybox_tex);
	Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);

	Matrix m = Matrix_Identity;
	Vector2 rotation = Camera_ActiveCamera->GetCameraOrientation();

	Matrix rotX, rotY;
	Matrix_RotateY(&rotY, rotation.Y); /* Yaw */
	Matrix_MulBy(&m, &rotY);
	Matrix_RotateX(&rotX, rotation.X); /* Pitch */
	Matrix_MulBy(&m, &rotX);
	/* Tilt skybox too. */
	Matrix_MulBy(&m, &Camera_ActiveCamera->tiltM);
	Gfx_LoadMatrix(&m);

	Gfx_BindVb(skybox_vb);
	Gfx_DrawIndexedVb(DrawMode_Triangles, skybox_count * 6 / 4, 0);

	Gfx_SetTexturing(false);
	Gfx_LoadMatrix(&Game_View);
	Gfx_SetDepthWrite(true);
}

void SkyboxRenderer_ContextLost(void) { Gfx_DeleteVb(&skybox_vb); }
void SkyboxRenderer_ContextRecreated(void) { SkyboxRenderer_MakeVb(); }

void SkyboxRenderer_MakeVb(void) {
	if (Gfx_LostContext) return;
	Gfx_DeleteVb(&skybox_vb);
	VertexP3fT2fC4b vertices[skybox_count];
	#define extent 0.5f
	TextureRec rec;
	PackedCol col = WorldEnv_CloudsCol;

	/* Render the front quad */
	rec = TextureRec_FromRegion(0.25f, 0.5f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[ 0],  extent, -extent, -extent, rec.U1, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[ 1], -extent, -extent, -extent, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[ 2], -extent,  extent, -extent, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[ 3],  extent,  extent, -extent, rec.U1, rec.V1, col);

	/* Render the left quad */
	rec = TextureRec_FromRegion(0.00f, 0.5f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[ 4],  extent, -extent,  extent, rec.U1, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[ 5],  extent, -extent, -extent, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[ 6],  extent,  extent, -extent, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[ 7],  extent,  extent,  extent, rec.U1, rec.V1, col);

	/* Render the back quad */
	rec = TextureRec_FromRegion(0.75f, 0.5f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[ 8], -extent, -extent,  extent, rec.U1, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[ 9],  extent, -extent,  extent, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[10],  extent,  extent,  extent, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[11], -extent,  extent,  extent, rec.U1, rec.V1, col);

	/* Render the right quad */
	rec = TextureRec_FromRegion(0.50f, 0.5f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[12], -extent, -extent, -extent, rec.U1, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[13], -extent, -extent, extent, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[14], -extent, extent, extent, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[15], -extent, extent, -extent, rec.U1, rec.V1, col);

	/* Render the top quad */
	rec = TextureRec_FromRegion(0.25f, 0.0f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[16], -extent, extent, -extent, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[17], -extent, extent, extent, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[18], extent, extent, extent, rec.U1, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[19], extent, extent, -extent, rec.U1, rec.V2, col);

	/* Render the bottom quad */
	rec = TextureRec_FromRegion(0.50f, 0.0f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[20], -extent, -extent, -extent, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[21], -extent, -extent,  extent, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[22],  extent, -extent,  extent, rec.U1, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[23],  extent, -extent, -extent, rec.U1, rec.V2, col);

	skybox_vb = Gfx_CreateVb(vertices, VertexFormat_P3fT2fC4b, skybox_count);
}