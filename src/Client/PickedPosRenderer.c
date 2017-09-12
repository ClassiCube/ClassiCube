#include "PickedPosRenderer.h"
#include "PackedCol.h"
#include "VertexStructs.h"
#include "GraphicsAPI.h"
#include "GraphicsEnums.h"
#include "GraphicsCommon.h"
#include "Game.h"
#include "Events.h"

GfxResourceID pickedPos_vb;
PackedCol pickedPos_col;
#define pickedPos_numVertices (16 * 6)
VertexP3fC4b pickedPos_vertices[pickedPos_numVertices];
VertexP3fC4b* pickedPos_ptr;

void PickedPosRenderer_ContextLost(void) {
	Gfx_DeleteVb(&pickedPos_vb);
}

void PickedPosRenderer_ContextRecreated(void) {
	pickedPos_vb = Gfx_CreateDynamicVb(VertexFormat_P3fC4b, pickedPos_numVertices);
}

void PickedPosRenderer_Init(void) {
	pickedPos_col = PackedCol_Create4(0, 0, 0, 102);
	PickedPosRenderer_ContextRecreated();
	Event_RegisterVoid(&GfxEvents_ContextLost, PickedPosRenderer_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, PickedPosRenderer_ContextRecreated);
}

void PickedPosRenderer_Free(void) {
	PickedPosRenderer_ContextLost();
	Event_UnregisterVoid(&GfxEvents_ContextLost, PickedPosRenderer_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, PickedPosRenderer_ContextRecreated);
}

void PickedPosRenderer_Render(Real64 delta) {
	if (Gfx_LostContext) return;

	Gfx_SetAlphaBlending(true);
	Gfx_SetDepthWrite(false);
	Gfx_SetBatchFormat(VertexFormat_P3fC4b);

	GfxCommon_UpdateDynamicVb_IndexedTris(pickedPos_vb, pickedPos_vertices, pickedPos_numVertices);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);
}

void PickedPosRenderer_XQuad(Real32 x, Real32 z1, Real32 y1, Real32 z2, Real32 y2) {
	VertexP3fC4b_Set(pickedPos_ptr, x, y1, z1, pickedPos_col); pickedPos_ptr++;
	VertexP3fC4b_Set(pickedPos_ptr, x, y2, z1, pickedPos_col); pickedPos_ptr++;
	VertexP3fC4b_Set(pickedPos_ptr, x, y2, z2, pickedPos_col); pickedPos_ptr++;
	VertexP3fC4b_Set(pickedPos_ptr, x, y1, z2, pickedPos_col); pickedPos_ptr++;
}

void PickedPosRenderer_ZQuad(Real32 z, Real32 x1, Real32 y1, Real32 x2, Real32 y2) {
	VertexP3fC4b_Set(pickedPos_ptr, x1, y1, z, pickedPos_col); pickedPos_ptr++;
	VertexP3fC4b_Set(pickedPos_ptr, x1, y2, z, pickedPos_col); pickedPos_ptr++;
	VertexP3fC4b_Set(pickedPos_ptr, x2, y2, z, pickedPos_col); pickedPos_ptr++;
	VertexP3fC4b_Set(pickedPos_ptr, x2, y1, z, pickedPos_col); pickedPos_ptr++;
}

void PickedPosRenderer_YQuad(Real32 y, Real32 x1, Real32 z1, Real32 x2, Real32 z2) {
	VertexP3fC4b_Set(pickedPos_ptr, x1, y, z1, pickedPos_col); pickedPos_ptr++;
	VertexP3fC4b_Set(pickedPos_ptr, x1, y, z2, pickedPos_col); pickedPos_ptr++;
	VertexP3fC4b_Set(pickedPos_ptr, x2, y, z2, pickedPos_col); pickedPos_ptr++;
	VertexP3fC4b_Set(pickedPos_ptr, x2, y, z1, pickedPos_col); pickedPos_ptr++;
}

void PickedPosRenderer_UpdateState(PickedPos* selected) {
	pickedPos_ptr = pickedPos_vertices;
	Vector3 delta;
	Vector3_Subtract(&delta, &Game_CurrentCameraPos, &selected->Min);
	Real32 dist = Vector3_LengthSquared(&delta);

	Real32 offset = 0.01f;
	if (dist < 4.0f * 4.0f) offset = 0.00625f;
	if (dist < 2.0f * 2.0f) offset = 0.00500f;

	Vector3 p1, p2;
	Vector3_Add1(&p1, &selected->Min, -offset);
	Vector3_Add1(&p2, &selected->Max, offset);

	Real32 size = 1.0f / 16.0f;
	if (dist < 32.0f * 32.0f) size = 1.0f / 32.0f;
	if (dist < 16.0f * 16.0f) size = 1.0f / 64.0f;
	if (dist <  8.0f *  8.0f) size = 1.0f / 96.0f;
	if (dist <  4.0f *  4.0f) size = 1.0f / 128.0f;
	if (dist <  2.0f *  2.0f) size = 1.0f / 192.0f;

	/* bottom face */
	PickedPosRenderer_YQuad(p1.Y, p1.X, p1.Z + size, p1.X + size, p2.Z - size);
	PickedPosRenderer_YQuad(p1.Y, p2.X, p1.Z + size, p2.X - size, p2.Z - size);
	PickedPosRenderer_YQuad(p1.Y, p1.X, p1.Z, p2.X, p1.Z + size);
	PickedPosRenderer_YQuad(p1.Y, p1.X, p2.Z, p2.X, p2.Z - size);

	/* top face */
	PickedPosRenderer_YQuad(p2.Y, p1.X, p1.Z + size, p1.X + size, p2.Z - size);
	PickedPosRenderer_YQuad(p2.Y, p2.X, p1.Z + size, p2.X - size, p2.Z - size);
	PickedPosRenderer_YQuad(p2.Y, p1.X, p1.Z, p2.X, p1.Z + size);
	PickedPosRenderer_YQuad(p2.Y, p1.X, p2.Z, p2.X, p2.Z - size);

	/* left face */
	PickedPosRenderer_XQuad(p1.X, p1.Z, p1.Y + size, p1.Z + size, p2.Y - size);
	PickedPosRenderer_XQuad(p1.X, p2.Z, p1.Y + size, p2.Z - size, p2.Y - size);
	PickedPosRenderer_XQuad(p1.X, p1.Z, p1.Y, p2.Z, p1.Y + size);
	PickedPosRenderer_XQuad(p1.X, p1.Z, p2.Y, p2.Z, p2.Y - size);

	/* right face */
	PickedPosRenderer_XQuad(p2.X, p1.Z, p1.Y + size, p1.Z + size, p2.Y - size);
	PickedPosRenderer_XQuad(p2.X, p2.Z, p1.Y + size, p2.Z - size, p2.Y - size);
	PickedPosRenderer_XQuad(p2.X, p1.Z, p1.Y, p2.Z, p1.Y + size);
	PickedPosRenderer_XQuad(p2.X, p1.Z, p2.Y, p2.Z, p2.Y - size);

	/* front face */
	PickedPosRenderer_ZQuad(p1.Z, p1.X, p1.Y + size, p1.X + size, p2.Y - size);
	PickedPosRenderer_ZQuad(p1.Z, p2.X, p1.Y + size, p2.X - size, p2.Y - size);
	PickedPosRenderer_ZQuad(p1.Z, p1.X, p1.Y, p2.X, p1.Y + size);
	PickedPosRenderer_ZQuad(p1.Z, p1.X, p2.Y, p2.X, p2.Y - size);

	/* back face */
	PickedPosRenderer_ZQuad(p2.Z, p1.X, p1.Y + size, p1.X + size, p2.Y - size);
	PickedPosRenderer_ZQuad(p2.Z, p2.X, p1.Y + size, p2.X - size, p2.Y - size);
	PickedPosRenderer_ZQuad(p2.Z, p1.X, p1.Y, p2.X, p1.Y + size);
	PickedPosRenderer_ZQuad(p2.Z, p1.X, p2.Y, p2.X, p2.Y - size);
}

IGameComponent PickedPosRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = PickedPosRenderer_Init;
	comp.Free = PickedPosRenderer_Free;
	return comp;
}