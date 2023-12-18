#include "SelOutlineRenderer.h"
#include "PackedCol.h"
#include "Graphics.h"
#include "Game.h"
#include "Event.h"
#include "Picking.h"
#include "Funcs.h"
#include "Camera.h"
#include "Options.h"

static GfxResourceID selOutline_vb;
static float base_size;
static PackedCol color;
#define SELOUTLINE_NUM_VERTICES (16 * 6)

#define SelOutline_Y(y)\
0,y,1,  0,y,2,  1,y,2,  1,y,1,\
3,y,1,  3,y,2,  2,y,2,  2,y,1,\
0,y,0,  0,y,1,  3,y,1,  3,y,0,\
0,y,3,  0,y,2,  3,y,2,  3,y,3,

#define SelOutline_X(x)\
x,1,0,  x,2,0,  x,2,1,  x,1,1,\
x,1,3,  x,2,3,  x,2,2,  x,1,2,\
x,0,0,  x,1,0,  x,1,3,  x,0,3,\
x,3,0,  x,2,0,  x,2,3,  x,3,3,

#define SelOutline_Z(z)\
0,1,z,  0,2,z,  1,2,z,  1,1,z,\
3,1,z,  3,2,z,  2,2,z,  2,1,z,\
0,0,z,  0,1,z,  3,1,z,  3,0,z,\
0,3,z,  0,2,z,  3,2,z,  3,3,z,


static void BuildMesh(struct RayTracer* selected) {
	static const cc_uint8 indices[288] = {
		SelOutline_Y(0) SelOutline_Y(3) /* YMin, YMax */
		SelOutline_X(0) SelOutline_X(3) /* XMin, XMax */
		SelOutline_Z(0) SelOutline_Z(3) /* ZMin, ZMax */
	};
	
	struct VertexColoured* ptr;
	int i;
	Vec3 delta;
	float dist, offset;
	float size, scale;
	Vec3 coords[4];

	Vec3_Sub(&delta, &Camera.CurrentPos, &selected->Min);
	dist = Vec3_LengthSquared(&delta);

	offset = 0.01f;
	if (dist < 4.0f * 4.0f) offset = 0.00625f;
	if (dist < 2.0f * 2.0f) offset = 0.00500f;

	scale = 1.0f / 16.0f;
	if (dist < 32.0f * 32.0f) scale = 1.0f / 32.0f;
	if (dist < 16.0f * 16.0f) scale = 1.0f / 64.0f;
	if (dist <  8.0f *  8.0f) scale = 1.0f / 96.0f;
	if (dist <  4.0f *  4.0f) scale = 1.0f / 128.0f;
	if (dist <  2.0f *  2.0f) scale = 1.0f / 192.0f;
	size = base_size * scale;
	
	/*  How a face is laid out: 
	                 #--#-------#--#<== OUTER_MAX (3)
	                 |  |       |  |
	                 |  #-------#<===== INNER_MAX (2)
	                 |  |       |  |
					 |  |       |  |
	                 |  |       |  |
	(1) INNER_MIN =====>#-------#  |
	                 |  |       |  |
	(0) OUTER_MIN ==>#--#-------#--#

	- these are used to fake thick lines, by making the lines appear slightly inset
	- note: actual difference between inner and outer is much smaller than the diagram
	*/
	Vec3_Add1(&coords[0], &selected->Min, -offset);
	Vec3_Add1(&coords[1], &coords[0],      size);
	Vec3_Add1(&coords[3], &selected->Max,  offset);
	Vec3_Add1(&coords[2], &coords[3],     -size);
	
	ptr = (struct VertexColoured*)Gfx_LockDynamicVb(selOutline_vb, 
									VERTEX_FORMAT_COLOURED, SELOUTLINE_NUM_VERTICES);
	for (i = 0; i < Array_Elems(indices); i += 3, ptr++) 
	{
		ptr->x   = coords[indices[i + 0]].x;
		ptr->y   = coords[indices[i + 1]].y;
		ptr->z   = coords[indices[i + 2]].z;
		ptr->Col = color;
	}
	Gfx_UnlockDynamicVb(selOutline_vb);
}

void SelOutlineRenderer_Render(struct RayTracer* selected, cc_bool dirty) {
	if (Gfx.LostContext) return;

	if (!selOutline_vb)
		selOutline_vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_COLOURED, SELOUTLINE_NUM_VERTICES);
	
	Gfx_SetAlphaBlending(true);
	Gfx_SetDepthWrite(false);
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);

	if (dirty) BuildMesh(selected);
	else Gfx_BindDynamicVb(selOutline_vb);

	Gfx_DrawVb_IndexedTris(SELOUTLINE_NUM_VERTICES);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);
}


/*########################################################################################################################*
*-----------------------------------------------PickedPosRenderer component-----------------------------------------------*
*#########################################################################################################################*/
static void OnContextLost(void* obj) {
	Gfx_DeleteDynamicVb(&selOutline_vb);
}

static void OnInit(void) {
	int opacity;
	cc_uint8 rgb[3];
	Event_Register_(&GfxEvents.ContextLost, NULL, OnContextLost);

	base_size = Options_GetFloat(OPT_SELECTED_BLOCK_OUTLINE_SCALE, 1, 16, 1);
	opacity   = Options_GetInt(OPT_SELECTED_BLOCK_OUTLINE_OPACITY, 0, 255, 102);

	if (Options_GetColor(OPT_SELECTED_BLOCK_OUTLINE_COLOR, rgb)) {
		color = PackedCol_Make(rgb[0], rgb[1], rgb[2], opacity);
	} else {
		color = PackedCol_Make(0, 0, 0, opacity); /* Black by default */
	}
}

static void OnFree(void) { OnContextLost(NULL); }

struct IGameComponent SelOutlineRenderer_Component = {
	OnInit, /* Init */
	OnFree, /* Free */
};
