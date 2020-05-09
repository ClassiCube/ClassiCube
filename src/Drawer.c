#include "Drawer.h"
#include "TexturePack.h"
#include "Constants.h"
#include "Graphics.h"
struct _DrawerData Drawer;

void Drawer_XMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = Drawer.MinBB.Z;
	float u2 = (count - 1) + Drawer.MaxBB.Z * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.Y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.Y * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.X = Drawer.X1; v.Col = col;

	v.Y = Drawer.Y2; v.Z = Drawer.Z2 + (count - 1); v.U = u2; v.V = v1; *ptr++ = v;
	v.Z = Drawer.Z1;							    v.U = u1;           *ptr++ = v;
	v.Y = Drawer.Y1;										  v.V = v2; *ptr++ = v;
	v.Z = Drawer.Z2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_XMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = (count - Drawer.MinBB.Z);
	float u2 = (1 - Drawer.MaxBB.Z) * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.Y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.Y * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.X = Drawer.X2; v.Col = col;

	v.Y = Drawer.Y2; v.Z = Drawer.Z1; v.U = u1; v.V = v1; *ptr++ = v;
	v.Z = Drawer.Z2 + (count - 1);    v.U = u2;           *ptr++ = v;
	v.Y = Drawer.Y1;                            v.V = v2; *ptr++ = v;
	v.Z = Drawer.Z1;                  v.U = u1;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_ZMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = (count - Drawer.MinBB.X);
	float u2 = (1 - Drawer.MaxBB.X) * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.Y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.Y * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.Z = Drawer.Z1; v.Col = col;

	v.X = Drawer.X2 + (count - 1); v.Y = Drawer.Y1; v.U = u2; v.V = v2; *ptr++ = v;
	v.X = Drawer.X1;                                v.U = u1;           *ptr++ = v;
	v.Y = Drawer.Y2;                                          v.V = v1; *ptr++ = v;
	v.X = Drawer.X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_ZMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = Drawer.MinBB.X;
	float u2 = (count - 1) + Drawer.MaxBB.X * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.Y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.Y * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.Z = Drawer.Z2; v.Col = col;

	v.X = Drawer.X2 + (count - 1); v.Y = Drawer.Y2; v.U = u2; v.V = v1; *ptr++ = v;
	v.X = Drawer.X1;                                v.U = u1;           *ptr++ = v;
	v.Y = Drawer.Y1;                                          v.V = v2; *ptr++ = v;
	v.X = Drawer.X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_YMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;

	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;
	float u1 = Drawer.MinBB.X;
	float u2 = (count - 1) + Drawer.MaxBB.X * UV2_Scale;
	float v1 = vOrigin + Drawer.MinBB.Z * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MaxBB.Z * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.Y = Drawer.Y1; v.Col = col;

	v.X = Drawer.X2 + (count - 1); v.Z = Drawer.Z2; v.U = u2; v.V = v2; *ptr++ = v;
	v.X = Drawer.X1;                                v.U = u1;           *ptr++ = v;
	v.Z = Drawer.Z1;                                          v.V = v1; *ptr++ = v;
	v.X = Drawer.X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_YMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = Drawer.MinBB.X;
	float u2 = (count - 1) + Drawer.MaxBB.X * UV2_Scale;
	float v1 = vOrigin + Drawer.MinBB.Z * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MaxBB.Z * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.Y = Drawer.Y2; v.Col = col;

	v.X = Drawer.X2 + (count - 1); v.Z = Drawer.Z1; v.U = u2; v.V = v1; *ptr++ = v;
	v.X = Drawer.X1;                                v.U = u1;           *ptr++ = v;
	v.Z = Drawer.Z2;                                          v.V = v2; *ptr++ = v;
	v.X = Drawer.X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}
