#include "SkyboxRenderer.h"
#include "Camera.h"
#include "Event.h"
#include "Game.h"
#include "GraphicsAPI.h"
#include "GraphicsEnums.h"
#include "PackedCol.h"
#include "2DStructs.h"
#include "VertexStructs.h"
#include "World.h"
#include "EnvRenderer.h"
#include "ExtMath.h"

GfxResourceID skybox_tex, skybox_vb;
#define SKYBOX_COUNT (6 * 4)

bool SkyboxRenderer_ShouldRender(void) {
	return skybox_tex > 0 && !EnvRenderer_Minimal;
}

void SkyboxRenderer_TexturePackChanged(void* obj) {
	Gfx_DeleteTexture(&skybox_tex);
	WorldEnv_SkyboxClouds = false;
}

void SkyboxRenderer_FileChanged(void* obj, Stream* src) {
	String skybox = String_FromConst("skybox.png");
	String useclouds = String_FromConst("useclouds");

	if (String_CaselessEquals(&src->Name, &skybox)) {
		Game_UpdateTexture(&skybox_tex, src, false);
	} else if (String_CaselessEquals(&src->Name, &useclouds)) {
		WorldEnv_SkyboxClouds = true;
	}
}

void SkyboxRenderer_Render(Real64 deltaTime) {
	if (skybox_vb == 0) return;
	Gfx_SetDepthWrite(false);
	Gfx_SetTexturing(false);
	Gfx_BindTexture(skybox_tex);
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);

	Matrix m = Matrix_Identity;
	Matrix rotX, rotY;

	/* Base skybox rotation */
	Real32 rotTime = (Real32)(Game_Accumulator * 2 * MATH_PI); /* So speed of 1 rotates whole skybox every second */
	Matrix_RotateY(&rotY, WorldEnv_SkyboxHorSpeed * rotTime);
	Matrix_MulBy(&m, &rotY);
	Matrix_RotateX(&rotX, WorldEnv_SkyboxVerSpeed * rotTime);
	Matrix_MulBy(&m, &rotX);

	/* Rotate around camera */
	Vector2 rotation = Camera_ActiveCamera->GetCameraOrientation();
	Matrix_RotateY(&rotY, rotation.Y); /* Camera yaw */
	Matrix_MulBy(&m, &rotY);
	Matrix_RotateX(&rotX, rotation.X); /* Camera pitch */
	Matrix_MulBy(&m, &rotX);
	/* Tilt skybox too. */
	Matrix_MulBy(&m, &Camera_TiltM);

	Gfx_LoadMatrix(&m);
	Gfx_BindVb(skybox_vb);
	Gfx_DrawVb_IndexedTris(SKYBOX_COUNT);

	Gfx_SetTexturing(false);
	Gfx_LoadMatrix(&Gfx_View);
	Gfx_SetDepthWrite(true);
}

void SkyboxRenderer_MakeVb(void) {
	if (Gfx_LostContext) return;
	Gfx_DeleteVb(&skybox_vb);
	VertexP3fT2fC4b vertices[SKYBOX_COUNT];
	#define pos 1.0f
	VertexP3fT2fC4b v; v.Col = WorldEnv_CloudsCol;

	/* Render the front quad */	                       
	v.X =  pos; v.Y = -pos; v.Z = -pos; v.U = 0.25f; v.V = 1.00f; vertices[ 0] = v;
	v.X = -pos;                         v.U = 0.50f;              vertices[ 1] = v;
	            v.Y =  pos;                          v.V = 0.50f; vertices[ 2] = v;
	v.X =  pos;                         v.U = 0.25f;              vertices[ 3] = v;
	
	/* Render the left quad */	
	v.X =  pos; v.Y = -pos; v.Z =  pos; v.U = 0.00f; v.V = 1.00f; vertices[ 4] = v;
	                        v.Z = -pos; v.U = 0.25f;              vertices[ 5] = v;
	            v.Y =  pos;                          v.V = 0.50f; vertices[ 6] = v;
	                        v.Z =  pos; v.U = 0.00f;              vertices[ 7] = v;
	
	/* Render the back quad */                        
	v.X = -pos; v.Y = -pos; v.Z =  pos; v.U = 0.75f; v.V = 1.00f; vertices[ 8] = v;
	v.X =  pos;                         v.U = 1.00f;              vertices[ 9] = v;
	            v.Y =  pos;                          v.V = 0.50f; vertices[10] = v;
	v.X = -pos;                         v.U = 0.75f;              vertices[11] = v;
	
	/* Render the right quad */	
	v.X = -pos; v.Y = -pos; v.Z = -pos; v.U = 0.50f; v.V = 1.00f; vertices[12] = v;
	                        v.Z =  pos; v.U = 0.75f;              vertices[13] = v;
	            v.Y =  pos;                          v.V = 0.50f; vertices[14] = v;
	                        v.Z = -pos; v.U = 0.50f;              vertices[15] = v;
	
	/* Render the top quad */	            
	v.X = -pos; v.Y =  pos; v.Z = -pos;                           vertices[16] = v;
	                        v.Z =  pos;              v.V = 0.00f; vertices[17] = v;
	v.X =  pos;                         v.U = 0.25f;              vertices[18] = v;
	                        v.Z = -pos;              v.V = 0.50f; vertices[19] = v;
	
	/* Render the bottom quad */	            
	v.X = -pos; v.Y = -pos; v.Z = -pos; v.U = 0.75f;              vertices[20] = v;
	                        v.Z =  pos;              v.V = 0.00f; vertices[21] = v;
	v.X =  pos;                         v.U = 0.50f;              vertices[22] = v;
	                        v.Z = -pos;              v.V = 0.50f; vertices[23] = v;

	skybox_vb = Gfx_CreateVb(vertices, VERTEX_FORMAT_P3FT2FC4B, SKYBOX_COUNT);
}

void SkyboxRenderer_ContextLost(void* obj) { Gfx_DeleteVb(&skybox_vb); }
void SkyboxRenderer_ContextRecreated(void* obj) { SkyboxRenderer_MakeVb(); }

void SkyboxRenderer_EnvVariableChanged(void* obj, Int32 envVar) {
	if (envVar != ENV_VAR_CLOUDS_COL) return;
	SkyboxRenderer_MakeVb();
}

void SkyboxRenderer_Init(void) {
	Event_RegisterStream(&TextureEvents_FileChanged, NULL, &SkyboxRenderer_FileChanged);
	Event_RegisterVoid(&TextureEvents_PackChanged,   NULL, &SkyboxRenderer_TexturePackChanged);
	Event_RegisterInt32(&WorldEvents_EnvVarChanged,  NULL, &SkyboxRenderer_EnvVariableChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,       NULL, &SkyboxRenderer_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,  NULL, &SkyboxRenderer_ContextRecreated);
}

void SkyboxRenderer_Reset(void) { Gfx_DeleteTexture(&skybox_tex); }

void SkyboxRenderer_Free(void) {
	Gfx_DeleteTexture(&skybox_tex);
	SkyboxRenderer_ContextLost(NULL);

	Event_UnregisterStream(&TextureEvents_FileChanged, NULL, &SkyboxRenderer_FileChanged);
	Event_UnregisterVoid(&TextureEvents_PackChanged,   NULL, &SkyboxRenderer_TexturePackChanged);
	Event_UnregisterInt32(&WorldEvents_EnvVarChanged,  NULL, &SkyboxRenderer_EnvVariableChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,       NULL, &SkyboxRenderer_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,  NULL, &SkyboxRenderer_ContextRecreated);
}

IGameComponent SkyboxRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = SkyboxRenderer_Init;
	comp.Free = SkyboxRenderer_Free;
	comp.OnNewMap = SkyboxRenderer_MakeVb; /* Need to recreate colour component of vertices */
	comp.Reset = SkyboxRenderer_Reset;
	return comp;
}