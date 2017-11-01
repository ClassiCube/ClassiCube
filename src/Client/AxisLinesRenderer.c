#include "AxisLinesRenderer.h"
#include "GraphicsAPI.h"
#include "Game.h"
#include "GraphicsEnums.h"
#include "GraphicsCommon.h"
#include "SelectionBox.h"
#include "PackedCol.h"
#include "Camera.h"
#include "Player.h"
#include "Events.h"

GfxResourceID axisLines_vb;
#define axisLines_numVertices 12
#define axisLines_size (1.0f / 32.0f)
#define axisLines_length 3.0f

void AxisLinesRenderer_ContextLost(void) {
	Gfx_DeleteVb(&axisLines_vb);
}

void AxisLinesRenderer_Init(void) {
	Event_RegisterVoid(&GfxEvents_ContextLost, AxisLinesRenderer_ContextLost);
}

void AxisLinesRenderer_Free(void) {
	AxisLinesRenderer_ContextLost();
	Event_UnregisterVoid(&GfxEvents_ContextLost, AxisLinesRenderer_ContextLost);
}

IGameComponent AxisLinesRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = AxisLinesRenderer_Init;
	comp.Free = AxisLinesRenderer_Free;
	return comp;
}

void AxisLinesRenderer_Render(Real64 delta) {
	if (!Game_ShowAxisLines || Gfx_LostContext) return;
	/* Don't do it in a ContextRecreated handler, because we only want VB recreated if ShowAxisLines in on. */
	if (axisLines_vb == NULL) {
		axisLines_vb = Gfx_CreateDynamicVb(VertexFormat_P3fC4b, axisLines_numVertices);
	}

	Gfx_SetTexturing(false);
	Vector3 P = LocalPlayer_Instance.Base.Base.Position; P.Y += 0.05f;
	VertexP3fC4b vertices[axisLines_numVertices];
	VertexP3fC4b* ptr = vertices;

	PackedCol red = PACKEDCOL_RED;
	SelectionBox_HorQuad(&ptr, red,
		P.X,                    P.Z - axisLines_size, 
		P.X + axisLines_length, P.Z + axisLines_size,
		P.Y);

	PackedCol blue = PACKEDCOL_BLUE;
	SelectionBox_HorQuad(&ptr, blue,
		P.X - axisLines_size, P.Z, 
		P.X + axisLines_size, P.Z + axisLines_length, 
		P.Y);

	if (Camera_ActiveCamera->IsThirdPerson) {
		PackedCol green = PACKEDCOL_GREEN;
		SelectionBox_VerQuad(&ptr, green,
			P.X - axisLines_size, P.Y,                    P.Z + axisLines_size, 
			P.X + axisLines_size, P.Y + axisLines_length, P.Z - axisLines_size);
	}

	Gfx_SetBatchFormat(VertexFormat_P3fC4b);
	GfxCommon_UpdateDynamicVb_IndexedTris(axisLines_vb, vertices, axisLines_numVertices);
}