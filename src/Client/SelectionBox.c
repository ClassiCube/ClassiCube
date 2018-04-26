#include "SelectionBox.h"
#include "ExtMath.h"
#include "GraphicsAPI.h"
#include "GraphicsCommon.h"
#include "Event.h"
#include "Funcs.h"
#include "Game.h"

/* Data for a selection box. */
typedef struct SelectionBox_ {
	Vector3 Min, Max;
	PackedCol Col;
	Real32 MinDist, MaxDist;
} SelectionBox;

void SelectionBox_Render(SelectionBox* box, VertexP3fC4b** vertices, VertexP3fC4b** lineVertices) {
	Real32 offset = box->MinDist < 32.0f * 32.0f ? (1.0f / 32.0f) : (1.0f / 16.0f);
	Vector3 p1 = box->Min, p2 = box->Max;
	Vector3_Add1(&p1, &p1, -offset);
	Vector3_Add1(&p2, &p2,  offset);
	PackedCol col = box->Col;

	SelectionBox_HorQuad(vertices, col, p1.X, p1.Z, p2.X, p2.Z, p1.Y);       /* bottom */
	SelectionBox_HorQuad(vertices, col, p1.X, p1.Z, p2.X, p2.Z, p2.Y);       /* top */
	SelectionBox_VerQuad(vertices, col, p1.X, p1.Y, p1.Z, p2.X, p2.Y, p1.Z); /* sides */
	SelectionBox_VerQuad(vertices, col, p1.X, p1.Y, p2.Z, p2.X, p2.Y, p2.Z);
	SelectionBox_VerQuad(vertices, col, p1.X, p1.Y, p1.Z, p1.X, p2.Y, p2.Z);
	SelectionBox_VerQuad(vertices, col, p2.X, p1.Y, p1.Z, p2.X, p2.Y, p2.Z);

	col.R = (UInt8)~col.R; col.G = (UInt8)~col.G; col.B = (UInt8)~col.B;
	/* bottom face */
	SelectionBox_Line(lineVertices, col, p1.X, p1.Y, p1.Z, p2.X, p1.Y, p1.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p1.Y, p1.Z, p2.X, p1.Y, p2.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p1.Y, p2.Z, p1.X, p1.Y, p2.Z);
	SelectionBox_Line(lineVertices, col, p1.X, p1.Y, p2.Z, p1.X, p1.Y, p1.Z);
	/* top face */
	SelectionBox_Line(lineVertices, col, p1.X, p2.Y, p1.Z, p2.X, p2.Y, p1.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p2.Y, p1.Z, p2.X, p2.Y, p2.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p2.Y, p2.Z, p1.X, p2.Y, p2.Z);
	SelectionBox_Line(lineVertices, col, p1.X, p2.Y, p2.Z, p1.X, p2.Y, p1.Z);
	/* side faces */
	SelectionBox_Line(lineVertices, col, p1.X, p1.Y, p1.Z, p1.X, p2.Y, p1.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p1.Y, p1.Z, p2.X, p2.Y, p1.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p1.Y, p2.Z, p2.X, p2.Y, p2.Z);
	SelectionBox_Line(lineVertices, col, p1.X, p1.Y, p2.Z, p1.X, p2.Y, p2.Z);
}

void SelectionBox_VerQuad(VertexP3fC4b** vertices, PackedCol col,
	Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2) {
	VertexP3fC4b* ptr = *vertices;
	VertexP3fC4b v; v.Col = col;

	v.X = x1; v.Y = y1; v.Z = z1; *ptr++ = v;
			  v.Y = y2;           *ptr++ = v;
	v.X = x2;           v.Z = z2; *ptr++ = v;
			  v.Y = y1;           *ptr++ = v;
	*vertices = ptr;
}

void SelectionBox_HorQuad(VertexP3fC4b** vertices, PackedCol col,
	Real32 x1, Real32 z1, Real32 x2, Real32 z2, Real32 y) {
	VertexP3fC4b* ptr = *vertices;
	VertexP3fC4b v; v.Y = y; v.Col = col;

	v.X = x1; v.Z = z1; *ptr++ = v;
			  v.Z = z2; *ptr++ = v;
	v.X = x2;           *ptr++ = v;
			  v.Z = z1; *ptr++ = v;
	*vertices = ptr;
}

void SelectionBox_Line(VertexP3fC4b** vertices, PackedCol col,
	Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2) {
	VertexP3fC4b* ptr = *vertices;  
	VertexP3fC4b v; v.Col = col;

	v.X = x1; v.Y = y1; v.Z = z1; *ptr++ = v;
	v.X = x2; v.Y = y2; v.Z = z2; *ptr++ = v;
	*vertices = ptr;
}


Int32 SelectionBox_Compare(SelectionBox* a, SelectionBox* b) {	
	Real32 aDist, bDist;
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

void SelectionBox_UpdateDist(Vector3 p, Real32 x2, Real32 y2, Real32 z2, Real32* closest, Real32* furthest) {
	Real32 dx = x2 - p.X, dy = y2 - p.Y, dz = z2 - p.Z;
	Real32 dist = dx * dx + dy * dy + dz * dz;

	if (dist < *closest)  *closest  = dist;
	if (dist > *furthest) *furthest = dist;
}

void SelectionBox_Intersect(SelectionBox* box, Vector3 origin) {
	Vector3 min = box->Min, max = box->Max;
	Real32 closest = MATH_POS_INF, furthest = -MATH_POS_INF;
	/* Bottom corners */
	SelectionBox_UpdateDist(origin, min.X, min.Y, min.Z, &closest, &furthest);
	SelectionBox_UpdateDist(origin, max.X, min.Y, min.Z, &closest, &furthest);
	SelectionBox_UpdateDist(origin, max.X, min.Y, max.Z, &closest, &furthest);
	SelectionBox_UpdateDist(origin, min.X, min.Y, max.Z, &closest, &furthest);
	/* Top corners */
	SelectionBox_UpdateDist(origin, min.X, max.Y, min.Z, &closest, &furthest);
	SelectionBox_UpdateDist(origin, max.X, max.Y, min.Z, &closest, &furthest);
	SelectionBox_UpdateDist(origin, max.X, max.Y, max.Z, &closest, &furthest);
	SelectionBox_UpdateDist(origin, min.X, max.Y, max.Z, &closest, &furthest);
	box->MinDist = closest; box->MaxDist = furthest;
}


#define SELECTIONS_MAX 256
#define SELECTIONS_VERTICES 24
#define SELECTIONS_MAX_VERTICES SELECTIONS_MAX * SELECTIONS_VERTICES

UInt32 selections_count;
SelectionBox selections_list[SELECTIONS_MAX];
UInt8 selections_ids[SELECTIONS_MAX];
GfxResourceID selections_VB, selections_LineVB;
bool selections_used;

void Selections_Add(UInt8 id, Vector3I p1, Vector3I p2, PackedCol col) {	
	SelectionBox sel;
	Vector3I min, max;
	Vector3I_Min(&min, &p1, &p2); Vector3I_ToVector3(&sel.Min, &min);
	Vector3I_Max(&max, &p1, &p2); Vector3I_ToVector3(&sel.Max, &max);
	sel.Col = col;

	Selections_Remove(id);
	selections_list[selections_count] = sel;
	selections_ids[selections_count] = id;
	selections_count++;
}

void Selections_Remove(UInt8 id) {
	UInt32 i;
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

void Selections_ContextLost(void* obj) {
	Gfx_DeleteVb(&selections_VB);
	Gfx_DeleteVb(&selections_LineVB);
}

void Selections_ContextRecreated(void* obj) {
	if (!selections_used) return;
	selections_VB     = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FC4B, SELECTIONS_MAX_VERTICES);
	selections_LineVB = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FC4B, SELECTIONS_MAX_VERTICES);
}

void Selections_QuickSort(Int32 left, Int32 right) {
	UInt8* values = selections_ids;       UInt8 value;
	SelectionBox* keys = selections_list; SelectionBox key;
	while (left < right) {
		Int32 i = left, j = right;
		SelectionBox* pivot = &keys[(i + j) / 2];

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

void Selections_Render(Real64 delta) {
	if (selections_count == 0 || Gfx_LostContext) return;
	/* TODO: Proper selection box sorting. But this is very difficult because
	   we can have boxes within boxes, intersecting boxes, etc. Probably not worth it. */
	Vector3 camPos = Game_CurrentCameraPos;
	UInt32 i;
	for (i = 0; i < selections_count; i++) {
		SelectionBox_Intersect(&selections_list[i], camPos);
	}
	Selections_QuickSort(0, selections_count - 1);

	if (selections_VB == NULL) { /* lazy init as most servers don't use this */
		selections_used = true;
		Selections_ContextRecreated(NULL);
	}

	VertexP3fC4b vertices[SELECTIONS_MAX_VERTICES]; VertexP3fC4b* ptr = vertices;
	VertexP3fC4b lineVertices[SELECTIONS_MAX_VERTICES]; VertexP3fC4b* linePtr = lineVertices;
	for (i = 0; i < selections_count; i++) {
		SelectionBox_Render(&selections_list[i], &ptr, &linePtr);
	}

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FC4B);
	GfxCommon_UpdateDynamicVb_Lines(selections_LineVB, lineVertices,
		selections_count * SELECTIONS_VERTICES);

	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaBlending(true);
	GfxCommon_UpdateDynamicVb_IndexedTris(selections_VB, vertices,
		selections_count * SELECTIONS_VERTICES);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);
}

void Selections_Init(void) {
	Event_RegisterVoid(&GfxEvents_ContextLost,      NULL, Selections_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, NULL, Selections_ContextRecreated);
}

void Selections_Reset(void) {
	selections_count = 0;
}

void Selections_Free(void) {
	Selections_ContextLost(NULL);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      NULL, Selections_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, NULL, Selections_ContextRecreated);
}

IGameComponent Selections_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = Selections_Init;
	comp.Free = Selections_Free;
	comp.Reset = Selections_Reset;
	comp.OnNewMap = Selections_Reset;
	return comp;
}