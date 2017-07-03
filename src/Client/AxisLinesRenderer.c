#include "AxisLinesRenderer.h"
#include "GraphicsAPI.h"
#include "GameProps.h"
#include "GraphicsEnums.h"
#include "GraphicsCommon.h"
#include "SelectionBox.h"
#include "PackedCol.h"
#include "Camera.h"
#include "LocalPlayer.h"

GfxResourceID axisLines_vb = -1;
#define axisLines_numVertices 12
#define axisLines_size (1.0f / 32.0f)
#define axisLines_length 3.0f

IGameComponent AxisLinesRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = AxisLinesRenderer_Init;
	comp.Free = AxisLinesRenderer_Free;
	return comp;
}

void AxisLinesRenderer_Render(Real64 delta) {
	if (!Game_ShowAxisLines || Gfx_LostContext) return;
	/* Don't do it in a ContextRecreated handler, because we only want VB recreated if ShowAxisLines in on. */
	if (axisLines_vb == -1) {
		axisLines_vb = Gfx_CreateDynamicVb(VertexFormat_P3fC4b, axisLines_numVertices);
	}

	Gfx_SetTexturing(false);
	Vector3 P = LocalPlayer_Instance.Base.Base.Position; P.Y += 0.05f;
	VertexP3fC4b vertices[axisLines_numVertices];
	VertexP3fC4b* ptr = vertices;

	SelectionBox_HorQuad(&ptr, PackedCol_Red,
		P.X,                    P.Z - axisLines_size, 
		P.X + axisLines_length, P.Z + axisLines_size,
		P.Y);
	SelectionBox_HorQuad(&ptr, PackedCol_Blue,
		P.X - axisLines_size, P.Z, 
		P.X + axisLines_size, P.Z + axisLines_length, 
		P.Y);

	if (Camera_ActiveCamera->IsThirdPerson) {
		SelectionBox_VerQuad(&ptr, PackedCol_Green,
			P.X - axisLines_size, P.Y,                    P.Z + axisLines_size, 
			P.X + axisLines_size, P.Y + axisLines_length, P.Z - axisLines_size);
	}

	Gfx_SetBatchFormat(VertexFormat_P3fC4b);
	GfxCommon_UpdateDynamicVb_IndexedTris(axisLines_vb, vertices, axisLines_numVertices);
}


void AxisLinesRenderer_Init(void) {
	EventHandler_RegisterVoid(Gfx_ContextLost, AxisLinesRenderer_ContextLost);
}

void AxisLinesRenderer_Free(void) {
	AxisLinesRenderer_ContextLost();
	EventHandler_UnregisterVoid(Gfx_ContextLost, AxisLinesRenderer_ContextLost);
}

void AxisLinesRenderer_ContextLost(void) {
	Gfx_DeleteVb(&axisLines_vb);
}