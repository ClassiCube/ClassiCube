#include "SkyboxRenderer.h"
#include "Camera.h"
#include "Events.h"
#include "Game.h"
#include "GraphicsAPI.h"
#include "GraphicsEnums.h"
#include "Events.h"
#include "PackedCol.h"
#include "2DStructs.h"
#include "VertexStructs.h"
#include "World.h"
#include "EnvRenderer.h"

GfxResourceID skybox_tex, skybox_vb = -1;
#define SKYBOX_COUNT (6 * 4)

bool SkyboxRenderer_ShouldRender(void) {
	return skybox_tex > 0 && !EnvRenderer_Minimal;
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
	Matrix_MulBy(&m, &Camera_TiltM);
	Gfx_LoadMatrix(&m);

	Gfx_BindVb(skybox_vb);
	Gfx_DrawVb_IndexedTris(SKYBOX_COUNT);

	Gfx_SetTexturing(false);
	Gfx_LoadMatrix(&Game_View);
	Gfx_SetDepthWrite(true);
}

void SkyboxRenderer_MakeVb(void) {
	if (Gfx_LostContext) return;
	Gfx_DeleteVb(&skybox_vb);
	VertexP3fT2fC4b vertices[SKYBOX_COUNT];
	#define pos 1.0f
	VertexP3fT2fC4b v; v.Colour = WorldEnv_CloudsCol;

	/* Render the front quad */
	                        v.Z = -pos;
	v.X =  pos; v.Y = -pos;             v.U = 0.25f; v.V = 1.00f; vertices[ 0] = v;
	v.X = -pos;                         v.U = 0.50f;              vertices[ 1] = v;
	            v.Y =  pos;                          v.V = 0.50f; vertices[ 2] = v;
	v.X =  pos;                         v.U = 0.25f;              vertices[ 3] = v;
	
	/* Render the left quad */
	v.X =  pos;
	            v.Y = -pos; v.Z =  pos; v.U = 0.00f; v.V = 1.00f; vertices[ 4] = v;
	                        v.Z = -pos; v.U = 0.25f;              vertices[ 5] = v;
	            v.Y =  pos;                          v.V = 0.50f; vertices[ 6] = v;
	                        v.Z =  pos; v.U = 0.00f;              vertices[ 7] = v;
	
	/* Render the back quad */
	                        v.Z =  pos;
	v.X = -pos; v.Y = -pos;             v.U = 0.75f; v.V = 1.00f; vertices[ 8] = v;
	v.X =  pos;                         v.U = 1.00f;              vertices[ 9] = v;
	            v.Y =  pos;                          v.V = 0.50f; vertices[10] = v;
	v.X = -pos;                         v.U = 0.75f;              vertices[11] = v;
	
	/* Render the right quad */
	v.X = -pos;
	            v.Y = -pos; v.Z = -pos; v.U = 0.50f; v.V = 1.00f; vertices[12] = v;
	                        v.Z =  pos; v.U = 0.75f;              vertices[13] = v;
	            v.Y =  pos;                          v.V = 0.50f; vertices[14] = v;
	                        v.Z = -pos; v.U = 0.50f;              vertices[15] = v;
	
	/* Render the top quad */
	            v.Y =  pos;
	v.X = -pos;             v.Z = -pos;                           vertices[16] = v;
	                        v.Z =  pos;              v.V = 0.00f; vertices[17] = v;
	v.X =  pos;                         v.U = 0.25f;              vertices[18] = v;
	                        v.Z = -pos;              v.V = 0.50f; vertices[19] = v;
	
	/* Render the bottom quad */
	            v.Y = -pos;
	v.X = -pos;             v.Z = -pos; v.U = 0.75f;              vertices[20] = v;
	                        v.Z =  pos;              v.V = 0.00f; vertices[21] = v;
	v.X =  pos;                         v.U = 0.50f;              vertices[22] = v;
	                        v.Z = -pos;              v.V = 0.50f; vertices[23] = v;

	skybox_vb = Gfx_CreateVb(vertices, VertexFormat_P3fT2fC4b, SKYBOX_COUNT);
}

void SkyboxRenderer_ContextLost(void) { Gfx_DeleteVb(&skybox_vb); }
void SkyboxRenderer_ContextRecreated(void) { SkyboxRenderer_MakeVb(); }

void SkyboxRenderer_EnvVariableChanged(EnvVar envVar) {
	if (envVar != EnvVar_CloudsCol) return;
	SkyboxRenderer_MakeVb();
}

void SkyboxRenderer_TexturePackChanged(void) {
	Gfx_DeleteTexture(&skybox_tex);
}

void SkyboxRenderer_Init(void) {
	Event_RegisterStream(&TextureEvents_FileChanged, &SkyboxRenderer_FileChanged);
	Event_RegisterVoid(&TextureEvents_PackChanged, &SkyboxRenderer_TexturePackChanged);
	Event_RegisterInt32(&WorldEvents_EnvVarChanged, &SkyboxRenderer_EnvVariableChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost, &SkyboxRenderer_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, &SkyboxRenderer_ContextRecreated);
}

void SkyboxRenderer_Reset(void) { Gfx_DeleteTexture(&skybox_tex); }

void SkyboxRenderer_Free(void) {
	Gfx_DeleteTexture(&skybox_tex);
	SkyboxRenderer_ContextLost();

	Event_UnregisterStream(&TextureEvents_FileChanged, &SkyboxRenderer_FileChanged);
	Event_UnregisterVoid(&TextureEvents_PackChanged, &SkyboxRenderer_TexturePackChanged);
	Event_UnregisterInt32(&WorldEvents_EnvVarChanged, &SkyboxRenderer_EnvVariableChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost, &SkyboxRenderer_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, &SkyboxRenderer_ContextRecreated);
}

IGameComponent SkyboxRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = SkyboxRenderer_Init;
	comp.Free = SkyboxRenderer_Free;
	comp.OnNewMap = SkyboxRenderer_MakeVb; /* Need to recreate colour component of vertices */
	comp.Reset = SkyboxRenderer_Reset;
	return comp;
}