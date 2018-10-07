#include "AxisLinesRenderer.h"
#include "GraphicsAPI.h"
#include "Game.h"
#include "GraphicsCommon.h"
#include "SelectionBox.h"
#include "PackedCol.h"
#include "Camera.h"
#include "Event.h"
#include "Entity.h"

GfxResourceID axisLines_vb;
#define axisLines_numVertices 12
#define axisLines_size (1.0f / 32.0f)
#define axisLines_length 3.0f

static void AxisLinesRenderer_ContextLost(void* obj) {
	Gfx_DeleteVb(&axisLines_vb);
}

static void AxisLinesRenderer_Init(void) {
	Event_RegisterVoid(&GfxEvents_ContextLost, NULL, AxisLinesRenderer_ContextLost);
}

static void AxisLinesRenderer_Free(void) {
	AxisLinesRenderer_ContextLost(NULL);
	Event_UnregisterVoid(&GfxEvents_ContextLost, NULL, AxisLinesRenderer_ContextLost);
}

void AxisLinesRenderer_MakeComponent(struct IGameComponent* comp) {
	comp->Init = AxisLinesRenderer_Init;
	comp->Free = AxisLinesRenderer_Free;
}

void AxisLinesRenderer_Render(double delta) {
	if (!Game_ShowAxisLines || Gfx_LostContext) return;
	/* Don't do it in a ContextRecreated handler, because we only want VB recreated if ShowAxisLines in on. */
	if (!axisLines_vb) {
		axisLines_vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FC4B, axisLines_numVertices);
	}

	Gfx_SetTexturing(false);
	Vector3 P = LocalPlayer_Instance.Base.Position; P.Y += 0.05f;
	VertexP3fC4b vertices[axisLines_numVertices];
	
	Vector3 coords[5]; coords[2] = P;
	Vector3_Add1(&coords[0], &P, -axisLines_length);
	Vector3_Add1(&coords[1], &P, -axisLines_size);
	Vector3_Add1(&coords[3], &P,  axisLines_size);
	Vector3_Add1(&coords[4], &P,  axisLines_length);
	
	static uint8_t indices[36] = {
		2,2,1, 2,2,3, 4,2,3, 4,2,1, /* X arrow */
		1,2,2, 1,2,4, 3,2,4, 3,2,2, /* Z arrow */
		1,2,3, 1,4,3, 3,4,1, 3,2,1, /* Y arrow */
	};

	static PackedCol cols[3] = { PACKEDCOL_RED, PACKEDCOL_BLUE, PACKEDCOL_GREEN };
	int i, count = Camera_Active->IsThirdPerson ? 12 : 8;
	VertexP3fC4b* ptr = vertices;

	for (i = 0; i < count; i++, ptr++) {
		ptr->X = coords[indices[i*3 + 0]].X;
		ptr->Y = coords[indices[i*3 + 1]].Y;
		ptr->Z = coords[indices[i*3 + 2]].Z;
		ptr->Col = cols[i >> 2];
	}

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FC4B);
	GfxCommon_UpdateDynamicVb_IndexedTris(axisLines_vb, vertices, count);
}
