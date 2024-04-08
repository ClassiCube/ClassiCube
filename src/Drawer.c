#include "Drawer.h"
#include "TexturePack.h"
#include "Constants.h"
#include "Graphics.h"
struct _DrawerData Drawer;

void Drawer_XMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* v = *vertices;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = Drawer.MinBB.z;
	float u2 = (count - 1) + Drawer.MaxBB.z * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.y * Atlas1D.InvTileSize * UV2_Scale;

	float x1 = Drawer.X1;
	float y1 = Drawer.Y1, y2 = Drawer.Y2;
	float z1 = Drawer.Z1, z2 = Drawer.Z2 + (count - 1);

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);

	v->x = x1; v->y = y2; v->z = z2; v->Col = col; v->U = u2; v->V = v1; v++;
	v->x = x1; v->y = y2; v->z = z1; v->Col = col; v->U = u1; v->V = v1; v++;
	v->x = x1; v->y = y1; v->z = z1; v->Col = col; v->U = u1; v->V = v2; v++;
	v->x = x1; v->y = y1; v->z = z2; v->Col = col; v->U = u2; v->V = v2; v++;
	*vertices = v;
}

void Drawer_XMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* v = *vertices;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = (count - Drawer.MinBB.z);
	float u2 = (1 - Drawer.MaxBB.z) * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.y * Atlas1D.InvTileSize * UV2_Scale;

	float x2 = Drawer.X2;
	float y1 = Drawer.Y1, y2 = Drawer.Y2;
	float z1 = Drawer.Z1, z2 = Drawer.Z2 + (count - 1);

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);

	v->x = x2; v->y = y2; v->z = z1; v->Col = col; v->U = u1; v->V = v1; v++;
	v->x = x2; v->y = y2; v->z = z2; v->Col = col; v->U = u2; v->V = v1; v++;
	v->x = x2; v->y = y1; v->z = z2; v->Col = col; v->U = u2; v->V = v2; v++;
	v->x = x2; v->y = y1; v->z = z1; v->Col = col; v->U = u1; v->V = v2; v++;
	*vertices = v;
}

void Drawer_ZMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* v = *vertices;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = (count - Drawer.MinBB.x);
	float u2 = (1 - Drawer.MaxBB.x) * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.y * Atlas1D.InvTileSize * UV2_Scale;

	float x1 = Drawer.X1, x2 = Drawer.X2 + (count - 1);
	float y1 = Drawer.Y1, y2 = Drawer.Y2;
	float z1 = Drawer.Z1;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);

	v->x = x2; v->y = y1; v->z = z1; v->Col = col; v->U = u2; v->V = v2; v++;
	v->x = x1; v->y = y1; v->z = z1; v->Col = col; v->U = u1; v->V = v2; v++;
	v->x = x1; v->y = y2; v->z = z1; v->Col = col; v->U = u1; v->V = v1; v++;
	v->x = x2; v->y = y2; v->z = z1; v->Col = col; v->U = u2; v->V = v1; v++;
	*vertices = v;
}

void Drawer_ZMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* v = *vertices;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = Drawer.MinBB.x;
	float u2 = (count - 1) + Drawer.MaxBB.x * UV2_Scale;
	float v1 = vOrigin + Drawer.MaxBB.y * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MinBB.y * Atlas1D.InvTileSize * UV2_Scale;

	float x1 = Drawer.X1, x2 = Drawer.X2 + (count - 1);
	float y1 = Drawer.Y1, y2 = Drawer.Y2;
	float z2 = Drawer.Z2;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);

	v->x = x2; v->y = y2; v->z = z2; v->Col = col; v->U = u2; v->V = v1; v++;
	v->x = x1; v->y = y2; v->z = z2; v->Col = col; v->U = u1; v->V = v1; v++;
	v->x = x1; v->y = y1; v->z = z2; v->Col = col; v->U = u1; v->V = v2; v++;
	v->x = x2; v->y = y1; v->z = z2; v->Col = col; v->U = u2; v->V = v2; v++;
	*vertices = v;
}

void Drawer_YMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* v = *vertices;

	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;
	float u1 = Drawer.MinBB.x;
	float u2 = (count - 1) + Drawer.MaxBB.x * UV2_Scale;
	float v1 = vOrigin + Drawer.MinBB.z * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MaxBB.z * Atlas1D.InvTileSize * UV2_Scale;

	float x1 = Drawer.X1, x2 = Drawer.X2 + (count - 1);
	float y1 = Drawer.Y1;
	float z1 = Drawer.Z1, z2 = Drawer.Z2;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);

	v->x = x2; v->y = y1; v->z = z2; v->Col = col; v->U = u2; v->V = v2; v++;
	v->x = x1; v->y = y1; v->z = z2; v->Col = col; v->U = u1; v->V = v2; v++;
	v->x = x1; v->y = y1; v->z = z1; v->Col = col; v->U = u1; v->V = v1; v++;
	v->x = x2; v->y = y1; v->z = z1; v->Col = col; v->U = u2; v->V = v1; v++;
	*vertices = v;
}

void Drawer_YMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices) {
	struct VertexTextured* v = *vertices;
	float vOrigin = Atlas1D_RowId(texLoc) * Atlas1D.InvTileSize;

	float u1 = Drawer.MinBB.x;
	float u2 = (count - 1) + Drawer.MaxBB.x * UV2_Scale;
	float v1 = vOrigin + Drawer.MinBB.z * Atlas1D.InvTileSize;
	float v2 = vOrigin + Drawer.MaxBB.z * Atlas1D.InvTileSize * UV2_Scale;

	float x1 = Drawer.X1, x2 = Drawer.X2 + (count - 1);
	float y2 = Drawer.Y2;
	float z1 = Drawer.Z1, z2 = Drawer.Z2;

	if (Drawer.Tinted) col = PackedCol_Tint(col, Drawer.TintCol);

	v->x = x2; v->y = y2; v->z = z1; v->Col = col; v->U = u2; v->V = v1; v++;
	v->x = x1; v->y = y2; v->z = z1; v->Col = col; v->U = u1; v->V = v1; v++;
	v->x = x1; v->y = y2; v->z = z2; v->Col = col; v->U = u1; v->V = v2; v++;
	v->x = x2; v->y = y2; v->z = z2; v->Col = col; v->U = u2; v->V = v2; v++;
	*vertices = v;
}
