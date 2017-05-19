#include "Typedefs.h"
#include "WorldEnv.h"
#include "EventHandler.h"
#include "MiscEvents.h"
#include "GraphicsAPI.h"
#include "GraphicsEnums.h"
#include "VertexStructs.h"
#include "GameProps.h"
#include "PackedCol.h"
#include "WorldEvents.h"
#include "Camera.h"
#include "TextureRec.h"

Int32 skybox_tex, skybox_vb = -1;
#define skybox_count (6 * 4)

bool SkyboxRenderer_ShouldRender() {
	return skybox_tex > 0 && !EnvRenderer_IsMinimal;
}

void SkyboxRenderer_Init() {
	game.Events.TextureChanged += TextureChanged;
	game.Events.TexturePackChanged += TexturePackChanged;
	game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
	game.Graphics.ContextLost += ContextLost;
	game.Graphics.ContextRecreated += ContextRecreated;
	SkyboxRenderer_ContextRecreated();
}

void SkyboxRenderer_Reset() { Gfx_DeleteTexture(&skybox_tex); }
void SkyboxRenderer_OnNewMap() { SkyboxRenderer_MakeVb(); }

void SkyboxRenderer_Free() {
	Gfx_DeleteTexture(&skybox_tex);
	SkyboxRenderer_ContextLost();

	game.Events.TextureChanged -= TextureChanged;
	game.Events.TexturePackChanged -= TexturePackChanged;
	game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
	game.Graphics.ContextLost -= ContextLost;
	game.Graphics.ContextRecreated -= ContextRecreated;
}

void SkyboxRenderer_EnvVariableChanged(Int32 envVar) {
	if (envVar != EnvVar_CloudsCol) return;
	SkyboxRenderer_MakeVb();
}

void SkyboxRenderer_TexturePackChanged() {
	Gfx_DeleteTexture(&skybox_tex);
}

void SkyboxRenderer_TextureChanged(object sender, TextureEventArgs e) {
	if (e.Name == "skybox.png")
		game.UpdateTexture(ref tex, e.Name, e.Data, false);
}

void SkyboxRenderer_Render(Real64 deltaTime) {
	if (skybox_vb == -1) return;
	Gfx_SetDepthWrite(false);
	Gfx_SetTexturing(false);
	Gfx_BindTexture(skybox_tex);
	Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);

	Vector3 pos = Game_CurrentCameraPos;
	Matrix m = Matrix_Identity;
	Vector2 rotation = Camera_ActiveCamera->GetCameraOrientation();

	Matrix rotX, rotY;
	Matrix_RotateY(rotation.Y, &rotY); /* Yaw */
	Matrix_Mul(&m, &rotY, &m);
	Matrix_RotateX(rotation.X, &rotX); /* Pitch */
	Matrix_Mul(&m, &rotY, &m);
	/* Tilt skybox too. */
	Matrix_Mul(&m, &Camera_ActiveCamera->tiltM, &m);
	Gfx_LoadMatrix(&m);

	Gfx_BindVb(skybox_vb);
	Gfx_DrawIndexedVb(DrawMode_Triangles, skybox_count * 6 / 4, 0);

	Gfx_SetTexturing(false);
	Gfx_LoadMatrix(&Game_View);
	Gfx_SetDepthWrite(true);
}

void SkyboxRenderer_ContextLost() { Gfx_DeleteVb(&skybox_vb); }
void SkyboxRenderer_ContextRecreated() { SkyboxRenderer_MakeVb(); }

void SkyboxRenderer_MakeVb() {
	if (Gfx_LostContext) return;
	Gfx_DeleteVb(&skybox_vb);
	VertexP3fT2fC4b vertices[skybox_count];
	#define pos 0.5f
	TextureRec rec;
	PackedCol col = WorldEnv_CloudsCol;

	// Render the front quad
	rec = TextureRec_FromRegion(0.25f, 0.5f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[ 0],  pos, -pos, -pos, rec.U1, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[ 1], -pos, -pos, -pos, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[ 2], -pos,  pos, -pos, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[ 3],  pos,  pos, -pos, rec.U1, rec.V1, col);

	// Render the left quad
	rec = TextureRec_FromRegion(0.00f, 0.5f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[ 4],  pos, -pos,  pos, rec.U1, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[ 5],  pos, -pos, -pos, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[ 6],  pos,  pos, -pos, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[ 7],  pos,  pos,  pos, rec.U1, rec.V1, col);

	// Render the back quad
	rec = TextureRec_FromRegion(0.75f, 0.5f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[ 8], -pos, -pos,  pos, rec.U1, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[ 9],  pos, -pos,  pos, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[10],  pos,  pos,  pos, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[11], -pos,  pos,  pos, rec.U1, rec.V1, col);

	// Render the right quad
	rec = TextureRec_FromRegion(0.50f, 0.5f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[12], -pos, -pos, -pos, rec.U1, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[13], -pos, -pos, pos, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[14], -pos, pos, pos, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[15], -pos, pos, -pos, rec.U1, rec.V1, col);

	// Render the top quad
	rec = TextureRec_FromRegion(0.25f, 0.0f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[16], -pos, pos, -pos, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[17], -pos, pos, pos, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[18], pos, pos, pos, rec.U1, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[19], pos, pos, -pos, rec.U1, rec.V2, col);

	// Render the bottom quad
	rec = TextureRec_FromRegion(0.50f, 0.0f, 0.25f, 0.5f);
	VertexP3fT2fC4b_Set(&vertices[20], -pos, -pos, -pos, rec.U2, rec.V2, col);
	VertexP3fT2fC4b_Set(&vertices[21], -pos, -pos,  pos, rec.U2, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[22],  pos, -pos,  pos, rec.U1, rec.V1, col);
	VertexP3fT2fC4b_Set(&vertices[23],  pos, -pos, -pos, rec.U1, rec.V2, col);

	skybox_vb = Gfx_CreateVb(vertices, VertexFormat_P3fT2fC4b, skybox_count);
}