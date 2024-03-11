#include "AxisLinesRenderer.h"
#include "Graphics.h"
#include "Game.h"
#include "SelectionBox.h"
#include "PackedCol.h"
#include "Camera.h"
#include "Event.h"
#include "Entity.h"
#include "ExtMath.h"

cc_bool AxisLinesRenderer_Enabled;
static GfxResourceID axisLines_vb;
#define AXISLINES_NUM_VERTICES 12
#define AXISLINES_THICKNESS (1.0f / 32.0f)
#define AXISLINES_LENGTH 3.0f

void AxisLinesRenderer_Render(void) {
	static const cc_uint8 indices[36] = {
		2,2,1, 2,2,3, 4,2,3, 4,2,1, /* X arrow */
		1,2,2, 1,2,4, 3,2,4, 3,2,2, /* Z arrow */
		1,2,3, 1,4,3, 3,4,1, 3,2,1, /* Y arrow */
	};
	static const PackedCol colors[] = {
		PackedCol_Make(255,   0,   0, 255), /* Red   */
		PackedCol_Make(  0,   0, 255, 255), /* Blue  */
		PackedCol_Make(  0, 255,   0, 255), /* Green */
	};

	struct VertexColoured* v;
	Vec3 coords[5], pos, dirVector;
	int i, count;
	float axisLengthScale, axisThicknessScale;

	if (!AxisLinesRenderer_Enabled) return;
	/* Don't do it in a ContextRecreated handler, because we only want VB recreated if ShowAxisLines in on. */
	if (!axisLines_vb) {
		axisLines_vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_COLOURED, AXISLINES_NUM_VERTICES);
	}
	
	if (Camera.Active->isThirdPerson) {
		pos = LocalPlayer_Instance.Base.Position;
		axisLengthScale = 1;
		axisThicknessScale = 1;
		pos.y += 0.05f;
	} else {
		pos = Camera.CurrentPos;
		dirVector = Vec3_GetDirVector(LocalPlayer_Instance.Base.Yaw * MATH_DEG2RAD, LocalPlayer_Instance.Base.Pitch * MATH_DEG2RAD);
		Vec3_Mul1(&dirVector, &dirVector, 0.5f);
		Vec3_Add(&pos, &dirVector, &pos);
		axisLengthScale = 1.0f / 32.0f;
		axisThicknessScale = 1.0f / 8.0f;
	}
	count =  12;
	 
	Vec3_Add1(&coords[0], &pos, -AXISLINES_LENGTH    * axisLengthScale);
	Vec3_Add1(&coords[1], &pos, -AXISLINES_THICKNESS * axisThicknessScale);
	coords[2] = pos;
	Vec3_Add1(&coords[3], &pos,  AXISLINES_THICKNESS * axisThicknessScale);
	Vec3_Add1(&coords[4], &pos,  AXISLINES_LENGTH  	 * axisLengthScale);

	v = (struct VertexColoured*)Gfx_LockDynamicVb(axisLines_vb, 
									VERTEX_FORMAT_COLOURED, AXISLINES_NUM_VERTICES);
	for (i = 0; i < count; i++, v++) 
	{
		v->x   = coords[indices[i*3 + 0]].x;
		v->y   = coords[indices[i*3 + 1]].y;
		v->z   = coords[indices[i*3 + 2]].z;
		v->Col = colors[i >> 2];
	}

	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	Gfx_UnlockDynamicVb(axisLines_vb);
	Gfx_DrawVb_IndexedTris(count);
}


/*########################################################################################################################*
*-----------------------------------------------AxisLinesRenderer component-----------------------------------------------*
*#########################################################################################################################*/
static void OnContextLost(void* obj) {
	Gfx_DeleteDynamicVb(&axisLines_vb);
}

static void OnInit(void) {
	Event_Register_(&GfxEvents.ContextLost, NULL, OnContextLost);
}

static void OnFree(void) { OnContextLost(NULL); }

struct IGameComponent AxisLinesRenderer_Component = {
	OnInit, /* Init */
	OnFree, /* Free */
};
