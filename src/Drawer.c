#include "Drawer.h"
#include "TexturePack.h"
#include "Constants.h"
#include "Graphics.h"
struct _DrawerData Drawer;

void Drawer_XMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = Drawer.MinBB.z;
	float u2 = (count - 1) + Drawer.MaxBB.z * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.y * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.x = Drawer.X1; v.Col = col;

	v.y = Drawer.Y2; v.z = Drawer.Z2 + (count - 1); v.U = u2; v.V = v1; *ptr++ = v;
	v.z = Drawer.Z1;							    v.U = u1;           *ptr++ = v;
	v.y = Drawer.Y1;										  v.V = v2; *ptr++ = v;
	v.z = Drawer.Z2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_XMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = (count - Drawer.MinBB.z);
	float u2 = (1 - Drawer.MaxBB.z) * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.y * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.x = Drawer.X2; v.Col = col;

	v.y = Drawer.Y2; v.z = Drawer.Z1; v.U = u1; v.V = v1; *ptr++ = v;
	v.z = Drawer.Z2 + (count - 1);    v.U = u2;           *ptr++ = v;
	v.y = Drawer.Y1;                            v.V = v2; *ptr++ = v;
	v.z = Drawer.Z1;                  v.U = u1;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_ZMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = (count - Drawer.MinBB.x);
	float u2 = (1 - Drawer.MaxBB.x) * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.y * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.z = Drawer.Z1; v.Col = col;

	v.x = Drawer.X2 + (count - 1); v.y = Drawer.Y1; v.U = u2; v.V = v2; *ptr++ = v;
	v.x = Drawer.X1;                                v.U = u1;           *ptr++ = v;
	v.y = Drawer.Y2;                                          v.V = v1; *ptr++ = v;
	v.x = Drawer.X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_ZMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = Drawer.MinBB.x;
	float u2 = (count - 1) + Drawer.MaxBB.x * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.y * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.z = Drawer.Z2; v.Col = col;

	v.x = Drawer.X2 + (count - 1); v.y = Drawer.Y2; v.U = u2; v.V = v1; *ptr++ = v;
	v.x = Drawer.X1;                                v.U = u1;           *ptr++ = v;
	v.y = Drawer.Y1;                                          v.V = v2; *ptr++ = v;
	v.x = Drawer.X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_YMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;

	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;
	float u1 = Drawer.MinBB.x;
	float u2 = (count - 1) + Drawer.MaxBB.x * UV2_Scale;
	float v1 = vOrigin + Drawer.MinBB.z * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MaxBB.z * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.y = Drawer.Y1; v.Col = col;

	v.x = Drawer.X2 + (count - 1); v.z = Drawer.Z2; v.U = u2; v.V = v2; *ptr++ = v;
	v.x = Drawer.X1;                                v.U = u1;           *ptr++ = v;
	v.z = Drawer.Z1;                                          v.V = v1; *ptr++ = v;
	v.x = Drawer.X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}

void Drawer_YMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* ptr = *vertices; struct VertexTextured v;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = Drawer.MinBB.x;
	float u2 = (count - 1) + Drawer.MaxBB.x * UV2_Scale;
	float v1 = vOrigin + Drawer.MinBB.z * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MaxBB.z * Atlas1D.InvTileSize * UV2_Scale;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);
	v.y = Drawer.Y2; v.Col = col;

	v.x = Drawer.X2 + (count - 1); v.z = Drawer.Z1; v.U = u2; v.V = v1; *ptr++ = v;
	v.x = Drawer.X1;                                v.U = u1;           *ptr++ = v;
	v.z = Drawer.Z2;                                          v.V = v2; *ptr++ = v;
	v.x = Drawer.X2 + (count - 1);                  v.U = u2;           *ptr++ = v;
	*vertices = ptr;
}
