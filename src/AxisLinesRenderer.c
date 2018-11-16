#include "AxisLinesRenderer.h"
#include "Graphics.h"
#include "Game.h"
#include "GraphicsCommon.h"
#include "SelectionBox.h"
#include "PackedCol.h"
#include "Camera.h"
#include "Event.h"
#include "Entity.h"

static GfxResourceID axisLines_vb;
#define AXISLINES_NUM_VERTICES 12
#define AXISLINES_THICKNESS (1.0f / 32.0f)
#define AXISLINES_LENGTH 3.0f

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
	static uint8_t indices[36] = {
		2,2,1, 2,2,3, 4,2,3, 4,2,1, /* X arrow */
		1,2,2, 1,2,4, 3,2,4, 3,2,2, /* Z arrow */
		1,2,3, 1,4,3, 3,4,1, 3,2,1, /* Y arrow */
	};
	static PackedCol cols[3] = {
		PACKEDCOL_CONST(255,   0,   0, 255), /* Red   */
		PACKEDCOL_CONST(  0, 255,   0, 255), /* Green */
		PACKEDCOL_CONST(  0,   0, 255, 255), /* Blue  */
	};

	Vector3 coords[5], pos;
	VertexP3fC4b vertices[AXISLINES_NUM_VERTICES];
	VertexP3fC4b* ptr = vertices;
	int i, count;

	if (!Game_ShowAxisLines || Gfx_LostContext) return;
	/* Don't do it in a ContextRecreated handler, because we only want VB recreated if ShowAxisLines in on. */
	if (!axisLines_vb) {
		axisLines_vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FC4B, AXISLINES_NUM_VERTICES);
	}

	Gfx_SetTexturing(false);
	pos   = LocalPlayer_Instance.Base.Position; pos.Y += 0.05f;
	count = Camera_Active->IsThirdPerson ? 12 : 8;
	 
	Vector3_Add1(&coords[0], &pos, -AXISLINES_LENGTH);
	Vector3_Add1(&coords[1], &pos, -AXISLINES_THICKNESS);
	coords[2] = pos;
	Vector3_Add1(&coords[3], &pos,  AXISLINES_THICKNESS);
	Vector3_Add1(&coords[4], &pos,  AXISLINES_LENGTH);

	for (i = 0; i < count; i++, ptr++) {
		ptr->X   = coords[indices[i*3 + 0]].X;
		ptr->Y   = coords[indices[i*3 + 1]].Y;
		ptr->Z   = coords[indices[i*3 + 2]].Z;
		ptr->Col = cols[i >> 2];
	}

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FC4B);
	GfxCommon_UpdateDynamicVb_IndexedTris(axisLines_vb, vertices, count);
}
