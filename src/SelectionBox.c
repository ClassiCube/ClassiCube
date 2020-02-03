#include "SelectionBox.h"
#include "ExtMath.h"
#include "Graphics.h"
#include "Event.h"
#include "Funcs.h"
#include "Game.h"
#include "Camera.h"

/* Data for a selection box. */
struct SelectionBox {
	Vec3 Min, Max;
	PackedCol Col;
	float MinDist, MaxDist;
};

#define SelectionBox_Y(y) 0,y,0, 0,y,1, 1,y,1, 1,y,0,
#define SelectionBox_Z(z) 0,0,z, 0,1,z, 1,1,z, 1,0,z,
#define SelectionBox_X(x) x,0,0, x,1,0, x,1,1, x,0,1,

static void SelectionBox_RenderFaces(struct SelectionBox* box, VertexP3fC4b** faceVertices) {
	static const cc_uint8 faceIndices[72] = {
		SelectionBox_Y(0) SelectionBox_Y(1) /* YMin, YMax */
		SelectionBox_Z(0) SelectionBox_Z(1) /* ZMin, ZMax */
		SelectionBox_X(0) SelectionBox_X(1) /* XMin, XMax */
	};
	VertexP3fC4b* v;
	PackedCol col;
	int i;

	float offset = box->MinDist < 32.0f * 32.0f ? (1/32.0f) : (1/16.0f);
	Vec3 coords[2];
	Vec3_Add1(&coords[0], &box->Min, -offset);
	Vec3_Add1(&coords[1], &box->Max,  offset);

	col = box->Col;
	v   = *faceVertices;
	for (i = 0; i < Array_Elems(faceIndices); i += 3, v++) {
		v->X   = coords[faceIndices[i + 0]].X;
		v->Y   = coords[faceIndices[i + 1]].Y;
		v->Z   = coords[faceIndices[i + 2]].Z;
		v->Col = col;
	}
	*faceVertices = v;
}

static void SelectionBox_RenderEdges(struct SelectionBox* box, VertexP3fC4b** edgeVertices) {
	static const cc_uint8 edgeIndices[72] = {
		0,0,0, 1,0,0,  1,0,0, 1,0,1,  1,0,1, 0,0,1,  0,0,1, 0,0,0, /* YMin */
		0,1,0, 1,1,0,  1,1,0, 1,1,1,  1,1,1, 0,1,1,  0,1,1, 0,1,0, /* YMax */
		0,0,0, 0,1,0,  1,0,0, 1,1,0,  1,0,1, 1,1,1,  0,0,1, 0,1,1, /* X/Z  */
	};

	VertexP3fC4b* v;
	PackedCol col;
	int i;

	float offset = box->MinDist < 32.0f * 32.0f ? (1/32.0f) : (1/16.0f);
	Vec3 coords[2];
	Vec3_Add1(&coords[0], &box->Min, -offset);
	Vec3_Add1(&coords[1], &box->Max,  offset);

	col = box->Col;
	/* invert R/G/B for surrounding line */
	col = (col & PACKEDCOL_A_MASK) | (~col & PACKEDCOL_RGB_MASK);
	v   = *edgeVertices;
	for (i = 0; i < Array_Elems(edgeIndices); i += 3, v++) {
		v->X   = coords[edgeIndices[i + 0]].X;
		v->Y   = coords[edgeIndices[i + 1]].Y;
		v->Z   = coords[edgeIndices[i + 2]].Z;
		v->Col = col;
	}
	*edgeVertices = v;
}

static int SelectionBox_Compare(struct SelectionBox* a, struct SelectionBox* b) {
	float aDist, bDist;
	if (a->MinDist == b->MinDist) {
		aDist = a->MaxDist; bDist = b->MaxDist;
	} else {
		aDist = a->MinDist; bDist = b->MinDist;
	}

	/* Reversed comparison order result, because we need to render back to front for alpha blending */
	if (aDist < bDist) return 1;
	if (aDist > bDist) return -1;	
	return 0;
}

static void SelectionBox_Intersect(struct SelectionBox* box, Vec3 P) {
	float dx1 = (P.X - box->Min.X) * (P.X - box->Min.X), dx2 = (P.X - box->Max.X) * (P.X - box->Max.X);
	float dy1 = (P.Y - box->Min.Y) * (P.Y - box->Min.Y), dy2 = (P.Y - box->Max.Y) * (P.Y - box->Max.Y);
	float dz1 = (P.Z - box->Min.Z) * (P.Z - box->Min.Z), dz2 = (P.Z - box->Max.Z) * (P.Z - box->Max.Z);

	/* Distance to closest and furthest of the eight box corners */
	box->MinDist = min(dx1, dx2) + min(dy1, dy2) + min(dz1, dz2);
	box->MaxDist = max(dx1, dx2) + max(dy1, dy2) + max(dz1, dz2);
}


#define SELECTIONS_MAX 256
#define SELECTIONS_VERTICES 24
#define SELECTIONS_MAX_VERTICES SELECTIONS_MAX * SELECTIONS_VERTICES

static int selections_count;
static struct SelectionBox selections_list[SELECTIONS_MAX];
static cc_uint8 selections_ids[SELECTIONS_MAX];
static GfxResourceID selections_VB, selections_LineVB;
static cc_bool selections_used;

void Selections_Add(cc_uint8 id, IVec3 p1, IVec3 p2, PackedCol col) {
	struct SelectionBox sel;
	IVec3 min, max;
	IVec3_Min(&min, &p1, &p2); IVec3_ToVec3(&sel.Min, &min);
	IVec3_Max(&max, &p1, &p2); IVec3_ToVec3(&sel.Max, &max);
	sel.Col = col;

	Selections_Remove(id);
	selections_list[selections_count] = sel;
	selections_ids[selections_count]  = id;
	selections_count++;
}

void Selections_Remove(cc_uint8 id) {
	int i;
	for (i = 0; i < selections_count; i++) {
		if (selections_ids[i] != id) continue;

		for (; i < selections_count - 1; i++) {
			selections_list[i] = selections_list[i + 1];
			selections_ids[i]  = selections_ids[i + 1];
		}

		selections_count--;
		return;
	}
}

static void Selections_ContextLost(void* obj) {
	Gfx_DeleteDynamicVb(&selections_VB);
	Gfx_DeleteDynamicVb(&selections_LineVB);
}

static void Selections_ContextRecreated(void* obj) {
	if (!selections_used) return;
	selections_VB     = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FC4B, SELECTIONS_MAX_VERTICES);
	selections_LineVB = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FC4B, SELECTIONS_MAX_VERTICES);
}

static void Selections_QuickSort(int left, int right) {
	cc_uint8* values = selections_ids; cc_uint8 value;
	struct SelectionBox* keys = selections_list; struct SelectionBox key;

	while (left < right) {
		int i = left, j = right;
		struct SelectionBox* pivot = &keys[(i + j) >> 1];

		/* partition the list */
		while (i <= j) {
			while (SelectionBox_Compare(pivot, &keys[i]) > 0) i++;
			while (SelectionBox_Compare(pivot, &keys[j]) < 0) j--;
			QuickSort_Swap_KV_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(Selections_QuickSort)
	}
}

void Selections_Render(void) {
	VertexP3fC4b* data;
	Vec3 cameraPos;
	int i, count;
	if (!selections_count || Gfx.LostContext) return;

	/* TODO: Proper selection box sorting. But this is very difficult because
	   we can have boxes within boxes, intersecting boxes, etc. Probably not worth it. */
	cameraPos = Camera.CurrentPos;
	for (i = 0; i < selections_count; i++) {
		SelectionBox_Intersect(&selections_list[i], cameraPos);
	}
	Selections_QuickSort(0, selections_count - 1);

	if (!selections_VB) { /* lazy init as most servers don't use this */
		selections_used = true;
		Selections_ContextRecreated(NULL);
	}
	count = selections_count * SELECTIONS_VERTICES;
	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FC4B);

	data = Gfx_LockDynamicVb(selections_LineVB, VERTEX_FORMAT_P3FC4B, count);
	for (i = 0; i < selections_count; i++) {
		SelectionBox_RenderEdges(&selections_list[i], &data);
	}
	Gfx_UnlockDynamicVb(selections_LineVB);
	Gfx_DrawVb_Lines(count);

	data = Gfx_LockDynamicVb(selections_VB, VERTEX_FORMAT_P3FC4B, count);
	for (i = 0; i < selections_count; i++) {
		SelectionBox_RenderFaces(&selections_list[i], &data);
	}
	Gfx_UnlockDynamicVb(selections_VB);

	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaBlending(true);
	Gfx_DrawVb_IndexedTris(count);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);
}


/*########################################################################################################################*
*--------------------------------------------------Selections component---------------------------------------------------*
*#########################################################################################################################*/
static void Selections_Init(void) {
	Event_RegisterVoid(&GfxEvents.ContextLost,      NULL, Selections_ContextLost);
	Event_RegisterVoid(&GfxEvents.ContextRecreated, NULL, Selections_ContextRecreated);
}

static void Selections_Reset(void) {
	selections_count = 0;
}

static void Selections_Free(void) {
	Selections_ContextLost(NULL);
	Event_UnregisterVoid(&GfxEvents.ContextLost,      NULL, Selections_ContextLost);
	Event_UnregisterVoid(&GfxEvents.ContextRecreated, NULL, Selections_ContextRecreated);
}

struct IGameComponent Selections_Component = {
	Selections_Init,  /* Init  */
	Selections_Free,  /* Free  */
	Selections_Reset, /* Reset */
	Selections_Reset  /* OnNewMap */
};
