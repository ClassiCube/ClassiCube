#include "SelectionBox.h"
#include "ExtMath.h"
#include "Graphics.h"
#include "Event.h"
#include "Funcs.h"
#include "Game.h"
#include "Camera.h"

/* Data for a selection box. */
struct SelectionBox {
	Vec3 p0, p1;
	PackedCol color;
	float minDist, maxDist;
};

#define X0 0
#define X1 1
#define Y0 0
#define Y1 2
#define Z0 0
#define Z1 4

#define SelectionBox_Y(y) X0|y |Z0, X0|y |Z1, X1|y |Z1, X1|y |Z0,
#define SelectionBox_Z(z) X0|Y0|z , X0|Y1|z , X1|Y1|z , X1|Y0|z ,
#define SelectionBox_X(x) x |Y0|Z0, x |Y1|Z0, x |Y1|Z1, x |Y0|Z1,

static void BuildFaces(struct SelectionBox* box, struct VertexColoured* v) {
	static const cc_uint8 faceIndices[24] = {
		SelectionBox_Y(Y0) SelectionBox_Y(Y1) /* YMin, YMax */
		SelectionBox_Z(Z0) SelectionBox_Z(Z1) /* ZMin, ZMax */
		SelectionBox_X(X0) SelectionBox_X(X1) /* XMin, XMax */
	};
	PackedCol color;
	int i, flags;

	float offset = box->minDist < 32.0f * 32.0f ? (1/32.0f) : (1/16.0f);
	Vec3 coords[2];
	Vec3_Add1(&coords[0], &box->p0, -offset);
	Vec3_Add1(&coords[1], &box->p1,  offset);

	color = box->color;
	for (i = 0; i < Array_Elems(faceIndices); i++, v++) {
		flags  = faceIndices[i];
		v->x   = coords[(flags     ) & 1].x;
		v->y   = coords[(flags >> 1) & 1].y;
		v->z   = coords[(flags >> 2)    ].z;
		v->Col = color;
	}
}

static void BuildEdges(struct SelectionBox* box, struct VertexColoured* v) {
	static const cc_uint8 edgeIndices[24] = {
		X0|Y0|Z0, X1|Y0|Z0,  X1|Y0|Z0, X1|Y0|Z1,  X1|Y0|Z1, X0|Y0|Z1,  X0|Y0|Z1, X0|Y0|Z0, /* YMin */
		X0|Y1|Z0, X1|Y1|Z0,  X1|Y1|Z0, X1|Y1|Z1,  X1|Y1|Z1, X0|Y1|Z1,  X0|Y1|Z1, X0|Y1|Z0, /* YMax */
		X0|Y0|Z0, X0|Y1|Z0,  X1|Y0|Z0, X1|Y1|Z0,  X1|Y0|Z1, X1|Y1|Z1,  X0|Y0|Z1, X0|Y1|Z1, /* X/Z  */
	};
	PackedCol color;
	int i, flags;

	float offset = box->minDist < 32.0f * 32.0f ? (1/32.0f) : (1/16.0f);
	Vec3 coords[2];
	Vec3_Add1(&coords[0], &box->p0, -offset);
	Vec3_Add1(&coords[1], &box->p1,  offset);

	color = box->color;
	/* invert R/G/B for surrounding line */
	color = (color & PACKEDCOL_A_MASK) | (~color & PACKEDCOL_RGB_MASK);

	for (i = 0; i < Array_Elems(edgeIndices); i++, v++) {
		flags  = edgeIndices[i];
		v->x   = coords[(flags     ) & 1].x;
		v->y   = coords[(flags >> 1) & 1].y;
		v->z   = coords[(flags >> 2)    ].z;
		v->Col = color;
	}
}

static int CompareDists(struct SelectionBox* a, struct SelectionBox* b) {
	float aDist, bDist;
	if (a->minDist == b->minDist) {
		aDist = a->maxDist; bDist = b->maxDist;
	} else {
		aDist = a->minDist; bDist = b->minDist;
	}

	/* Reversed comparison order result, because we need to render back to front for alpha blending */
	if (aDist < bDist) return 1;
	if (aDist > bDist) return -1;
	return 0;
}

static void CalcDists(struct SelectionBox* box, Vec3 P) {
	float dx0 = (P.x - box->p0.x) * (P.x - box->p0.x), dx1 = (P.x - box->p1.x) * (P.x - box->p1.x);
	float dy0 = (P.y - box->p0.y) * (P.y - box->p0.y), dy1 = (P.y - box->p1.y) * (P.y - box->p1.y);
	float dz0 = (P.z - box->p0.z) * (P.z - box->p0.z), dz1 = (P.z - box->p1.z) * (P.z - box->p1.z);

	/* Distance to closest and furthest of the eight box corners */
	box->minDist = min(dx0, dx1) + min(dy0, dy1) + min(dz0, dz1);
	box->maxDist = max(dx0, dx1) + max(dy0, dy1) + max(dz0, dz1);
}


#define SELECTIONS_MAX 256
#define SELECTIONS_VERTICES 24
#define SELECTIONS_MAX_VERTICES SELECTIONS_MAX * SELECTIONS_VERTICES

static int selections_count;
static struct SelectionBox selections_list[SELECTIONS_MAX];
static cc_uint8 selections_ids[SELECTIONS_MAX];
static GfxResourceID selections_VB, selections_LineVB;

void Selections_Add(cc_uint8 id, const IVec3* p1, const IVec3* p2, PackedCol color) {
	struct SelectionBox sel;
	IVec3_ToVec3(&sel.p0, p1);
	IVec3_ToVec3(&sel.p1, p2);
	sel.color = color;

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

static void AllocateVertexBuffers(void) {
	selections_VB     = Gfx_CreateDynamicVb(VERTEX_FORMAT_COLOURED, SELECTIONS_MAX_VERTICES);
	selections_LineVB = Gfx_CreateDynamicVb(VERTEX_FORMAT_COLOURED, SELECTIONS_MAX_VERTICES);
}

static void Selections_QuickSort(int left, int right) {
	cc_uint8* values = selections_ids; cc_uint8 value;
	struct SelectionBox* keys = selections_list; struct SelectionBox key;

	while (left < right) {
		int i = left, j = right;
		struct SelectionBox* pivot = &keys[(i + j) >> 1];

		/* partition the list */
		while (i <= j) {
			while (CompareDists(pivot, &keys[i]) > 0) i++;
			while (CompareDists(pivot, &keys[j]) < 0) j--;
			QuickSort_Swap_KV_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(Selections_QuickSort)
	}
}

void Selections_Render(void) {
	struct VertexColoured* data;
	Vec3 cameraPos;
	int i, count;
	if (!selections_count) return;

	/* TODO: Proper selection box sorting. But this is very difficult because
	   we can have boxes within boxes, intersecting boxes, etc. Probably not worth it. */
	cameraPos = Camera.CurrentPos;
	for (i = 0; i < selections_count; i++) {
		CalcDists(&selections_list[i], cameraPos);
	}
	Selections_QuickSort(0, selections_count - 1);

	/* lazy init as most servers don't use this */
	if (!selections_VB) AllocateVertexBuffers();

	count = selections_count * SELECTIONS_VERTICES;
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);

	data = (struct VertexColoured*)Gfx_LockDynamicVb(selections_LineVB, 
										VERTEX_FORMAT_COLOURED, count);
	for (i = 0; i < selections_count; i++, data += SELECTIONS_VERTICES) {
		BuildEdges(&selections_list[i], data);
	}
	Gfx_UnlockDynamicVb(selections_LineVB);
	Gfx_DrawVb_Lines(count);

	data = (struct VertexColoured*)Gfx_LockDynamicVb(selections_VB, 
										VERTEX_FORMAT_COLOURED, count);
	for (i = 0; i < selections_count; i++, data += SELECTIONS_VERTICES) {
		BuildFaces(&selections_list[i], data);
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
static void OnInit(void) {
	Event_Register_(&GfxEvents.ContextLost, NULL, Selections_ContextLost);
}

static void OnReset(void) { selections_count = 0; }

static void OnFree(void) { Selections_ContextLost(NULL); }

struct IGameComponent Selections_Component = {
	OnInit,  /* Init  */
	OnFree,  /* Free  */
	OnReset, /* Reset */
	OnReset  /* OnNewMap */
};
