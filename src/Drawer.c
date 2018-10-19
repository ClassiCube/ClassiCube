#include "Drawer.h"
#include "TerrainAtlas.h"
#include "Constants.h"

/* Performance critical, use macro to ensure always inlined. */
#define ApplyTint \
if (Drawer_Tinted) {\
col.R = (uint8_t)(col.R * Drawer_TintColour.R / 255);\
col.G = (uint8_t)(col.G * Drawer_TintColour.G / 255);\
col.B = (uint8_t)(col.B * Drawer_TintColour.B / 255);\
}


void Drawer_XMin(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices) {
	VertexP3fT2fC4b* ptr = *vertices; VertexP3fT2fC4b v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = Drawer_MinBB.Z;
	float u2 = (count - 1) + Drawer_MaxBB.Z * UV2_Scale;
	float v1 = vOrigin + Drawer_MaxBB.Y * Atlas1D_InvTileSize;
	float v2 = vOrigin + Drawer_MinBB.Y * Atlas1D_InvTileSize * UV2_Scale;

	ApplyTint;
	v.X = Drawer_X1; v.Col = col;

	v.Y = Drawer_Y2; v.Z = Drawer_Z2 + (count - 1); v.U = u2; v.V = v1; *ptr++ = v;
	v.Z = Drawer_Z1;							    v.U = u1;           *ptr++ = v;
	v.Y = Drawer_Y1;										  v.V = v2; *ptr++ = v;
	v.Z = Drawer_Z2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_XMax(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices) {
	VertexP3fT2fC4b* ptr = *vertices; VertexP3fT2fC4b v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = (count - Drawer_MinBB.Z);
	float u2 = (1 - Drawer_MaxBB.Z) * UV2_Scale;
	float v1 = vOrigin + Drawer_MaxBB.Y * Atlas1D_InvTileSize;
	float v2 = vOrigin + Drawer_MinBB.Y * Atlas1D_InvTileSize * UV2_Scale;

	ApplyTint;
	v.X = Drawer_X2; v.Col = col;

	v.Y = Drawer_Y2; v.Z = Drawer_Z1; v.U = u1; v.V = v1; *ptr++ = v;
	v.Z = Drawer_Z2 + (count - 1);    v.U = u2;           *ptr++ = v;
	v.Y = Drawer_Y1;                            v.V = v2; *ptr++ = v;
	v.Z = Drawer_Z1;                  v.U = u1;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_ZMin(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices) {
	VertexP3fT2fC4b* ptr = *vertices; VertexP3fT2fC4b v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = (count - Drawer_MinBB.X);
	float u2 = (1 - Drawer_MaxBB.X) * UV2_Scale;
	float v1 = vOrigin + Drawer_MaxBB.Y * Atlas1D_InvTileSize;
	float v2 = vOrigin + Drawer_MinBB.Y * Atlas1D_InvTileSize * UV2_Scale;

	ApplyTint;
	v.Z = Drawer_Z1; v.Col = col;

	v.X = Drawer_X2 + (count - 1); v.Y = Drawer_Y1; v.U = u2; v.V = v2; *ptr++ = v;
	v.X = Drawer_X1;                                v.U = u1;           *ptr++ = v;
	v.Y = Drawer_Y2;                                          v.V = v1; *ptr++ = v;
	v.X = Drawer_X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_ZMax(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices) {
	VertexP3fT2fC4b* ptr = *vertices; VertexP3fT2fC4b v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = Drawer_MinBB.X;
	float u2 = (count - 1) + Drawer_MaxBB.X * UV2_Scale;
	float v1 = vOrigin + Drawer_MaxBB.Y * Atlas1D_InvTileSize;
	float v2 = vOrigin + Drawer_MinBB.Y * Atlas1D_InvTileSize * UV2_Scale;

	ApplyTint;
	v.Z = Drawer_Z2; v.Col = col;

	v.X = Drawer_X2 + (count - 1); v.Y = Drawer_Y2; v.U = u2; v.V = v1; *ptr++ = v;
	v.X = Drawer_X1;                                v.U = u1;           *ptr++ = v;
	v.Y = Drawer_Y1;                                          v.V = v2; *ptr++ = v;
	v.X = Drawer_X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_YMin(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices) {
	VertexP3fT2fC4b* ptr = *vertices; VertexP3fT2fC4b v;

	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;
	float u1 = Drawer_MinBB.X;
	float u2 = (count - 1) + Drawer_MaxBB.X * UV2_Scale;
	float v1 = vOrigin + Drawer_MinBB.Z * Atlas1D_InvTileSize;
	float v2 = vOrigin + Drawer_MaxBB.Z * Atlas1D_InvTileSize * UV2_Scale;

	ApplyTint;
	v.Y = Drawer_Y1; v.Col = col;

	v.X = Drawer_X2 + (count - 1); v.Z = Drawer_Z2; v.U = u2; v.V = v2; *ptr++ = v;
	v.X = Drawer_X1;                                v.U = u1;           *ptr++ = v;
	v.Z = Drawer_Z1;                                          v.V = v1; *ptr++ = v;
	v.X = Drawer_X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_YMax(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices) {
	VertexP3fT2fC4b* ptr = *vertices; VertexP3fT2fC4b v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D_InvTileSize;

	float u1 = Drawer_MinBB.X;
	float u2 = (count - 1) + Drawer_MaxBB.X * UV2_Scale;
	float v1 = vOrigin + Drawer_MinBB.Z * Atlas1D_InvTileSize;
	float v2 = vOrigin + Drawer_MaxBB.Z * Atlas1D_InvTileSize * UV2_Scale;

	ApplyTint;
	v.Y = Drawer_Y2; v.Col = col;

	v.X = Drawer_X2 + (count - 1); v.Z = Drawer_Z1; v.U = u2; v.V = v1; *ptr++ = v;
	v.X = Drawer_X1;                                v.U = u1;           *ptr++ = v;
	v.Z = Drawer_Z2;                                          v.V = v2; *ptr++ = v;
	v.X = Drawer_X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}
