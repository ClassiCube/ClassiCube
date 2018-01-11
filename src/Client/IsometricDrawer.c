#include "IsometricDrawer.h"
#include "Drawer.h"
#include "GraphicsCommon.h"
#include "GraphicsAPI.h"
#include "Matrix.h"
#include "PackedCol.h"
#include "ExtMath.h"
#include "Block.h"
#include "TerrainAtlas.h"

Int32 iso_count;
Real32 iso_scale;
VertexP3fT2fC4b* iso_vertices;
GfxResourceID iso_vb;

bool iso_cacheInitalisesd;
PackedCol iso_colNormal, iso_colXSide, iso_colZSide, iso_colYBottom;
Real32 iso_cosX, iso_sinX, iso_cosY, iso_sinY;
Matrix iso_transform;

Vector3 iso_pos;
Int32 iso_lastTexIndex, iso_texIndex;

void IsometricDrawer_RotateX(Real32 cosA, Real32 sinA) {
	Real32 y = cosA * iso_pos.Y + sinA * iso_pos.Z;
	iso_pos.Z = -sinA * iso_pos.Y + cosA * iso_pos.Z;
	iso_pos.Y = y;
}

void IsometricDrawer_RotateY(Real32 cosA, Real32 sinA) {
	Real32 x = cosA * iso_pos.X - sinA * iso_pos.Z;
	iso_pos.Z = sinA * iso_pos.X + cosA * iso_pos.Z;
	iso_pos.X = x;
}

void IsometricDrawer_InitCache(void) {
	if (iso_cacheInitalisesd) return;
	iso_cacheInitalisesd = true;
	PackedCol white = PACKEDCOL_WHITE;
	iso_colNormal = white;
	PackedCol_GetShaded(iso_colNormal, &iso_colXSide, &iso_colZSide, &iso_colYBottom);

	Matrix rotY, rotX;
	Matrix_RotateY(&rotY, 45.0f * MATH_DEG2RAD);
	Matrix_RotateX(&rotX, -30.0f * MATH_DEG2RAD);
	Matrix_Mul(&iso_transform, &rotY, &rotX);

	iso_cosX = Math_Cos(30.0f * MATH_DEG2RAD);
	iso_sinX = Math_Sin(30.0f * MATH_DEG2RAD);
	iso_cosY = Math_Cos(-45.0f * MATH_DEG2RAD);
	iso_sinY = Math_Sin(-45.0f * MATH_DEG2RAD);
}

void IsometricDrawer_Flush(void) {
	if (iso_lastTexIndex != -1) {
		Gfx_BindTexture(Atlas1D_TexIds[iso_lastTexIndex]);
		GfxCommon_UpdateDynamicVb_IndexedTris(iso_vb, iso_vertices, iso_count);
	}

	iso_lastTexIndex = iso_texIndex;
	iso_count = 0;
}

TextureLoc IsometricDrawer_GetTexLoc(BlockID block, Face face) {
	TextureLoc texLoc = Block_GetTexLoc(block, face);
	iso_texIndex = Atlas1D_Index(texLoc);

	if (iso_lastTexIndex != iso_texIndex) IsometricDrawer_Flush();
	return texLoc;
}

#define AddVertex *iso_vertices = v; iso_vertices++;
void IsometricDrawer_SpriteZQuad(BlockID block, bool firstPart) {
	TextureLoc texLoc = Block_GetTexLoc(block, FACE_ZMAX);
	TextureRec rec = Atlas1D_TexRec(texLoc, 1, &iso_texIndex);
	if (iso_lastTexIndex != iso_texIndex) IsometricDrawer_Flush();

	VertexP3fT2fC4b v;
	v.Col = iso_colNormal;
	Block_Tint(v.Col, block);

	Real32 x1 = firstPart ? 0.5f : -0.1f, x2 = firstPart ? 1.1f : 0.5f;
	rec.U1 = firstPart ? 0.0f : 0.5f;
	rec.U2 = (firstPart ? 0.5f : 1.0f) * UV2_Scale;

	Real32 minX = iso_scale * (1.0f - x1   * 2.0f) + iso_pos.X;
	Real32 maxX = iso_scale * (1.0f - x2   * 2.0f) + iso_pos.X;
	Real32 minY = iso_scale * (1.0f - 0.0f * 2.0f) + iso_pos.Y;
	Real32 maxY = iso_scale * (1.0f - 1.1f * 2.0f) + iso_pos.Y;

	v.Z = iso_pos.Z;
	v.X = minX; v.Y = minY; v.U = rec.U2; v.V = rec.V2; AddVertex;
	            v.Y = maxY;               v.V = rec.V1; AddVertex;
	v.X = maxX;             v.U = rec.U1;               AddVertex;
	            v.Y = minY;               v.V = rec.V2; AddVertex;
}

void IsometricDrawer_SpriteXQuad(BlockID block, bool firstPart) {
	TextureLoc texLoc = Block_GetTexLoc(block, FACE_XMAX);
	TextureRec rec = Atlas1D_TexRec(texLoc, 1, &iso_texIndex);
	if (iso_lastTexIndex != iso_texIndex) IsometricDrawer_Flush();

	VertexP3fT2fC4b v;
	v.Col = iso_colNormal;
	Block_Tint(v.Col, block);

	Real32 z1 = firstPart ? 0.5f : -0.1f, z2 = firstPart ? 1.1f : 0.5f;
	rec.U1 = firstPart ? 0.0f : 0.5f;
	rec.U2 = (firstPart ? 0.5f : 1.0f) * UV2_Scale;

	Real32 minY = iso_scale * (1.0f - 0.0f * 2.0f) + iso_pos.Y;
	Real32 maxY = iso_scale * (1.0f - 1.1f * 2.0f) + iso_pos.Y;
	Real32 minZ = iso_scale * (1.0f - z1   * 2.0f) + iso_pos.Z;
	Real32 maxZ = iso_scale * (1.0f - z2   * 2.0f) + iso_pos.Z;

	v.X = iso_pos.X;
	v.Y = minY; v.Z = minZ; v.U = rec.U2; v.V = rec.V2; AddVertex;
	v.Y = maxY;                           v.V = rec.V1; AddVertex;
	            v.Z = maxZ; v.U = rec.U1;               AddVertex;
	v.Y = minY;                           v.V = rec.V2; AddVertex;
}

void IsometricDrawer_BeginBatch(VertexP3fT2fC4b* vertices, GfxResourceID vb) {
	IsometricDrawer_InitCache();
	iso_lastTexIndex = -1;
	iso_count = 0;
	iso_vertices = vertices;
	iso_vb = vb;

	Gfx_LoadMatrix(&iso_transform);
}

void IsometricDrawer_DrawBatch(BlockID block, Real32 size, Real32 x, Real32 y) {
	bool bright = Block_FullBright[block];
	if (Block_Draw[block] == DRAW_GAS) return;

	/* isometric coords size: cosY * -scale - sinY * scale */
	/* we need to divide by (2 * cosY), as the calling function expects size to be in pixels. */
	iso_scale = size / (2.0f * iso_cosY);

	/* screen to isometric coords (cos(-x) = cos(x), sin(-x) = -sin(x)) */
	iso_pos.X = x; iso_pos.Y = y; iso_pos.Z = 0.0f;
	IsometricDrawer_RotateX(iso_cosX, -iso_sinX);
	IsometricDrawer_RotateY(iso_cosY, -iso_sinY);

	/* See comment in GfxCommon_Draw2DTexture() */
	iso_pos.X -= 0.5f; iso_pos.Y -= 0.5f;

	if (Block_Draw[block] == DRAW_SPRITE) {
		IsometricDrawer_SpriteXQuad(block, true);
		IsometricDrawer_SpriteZQuad(block, true);

		IsometricDrawer_SpriteZQuad(block, false);
		IsometricDrawer_SpriteXQuad(block, false);
	} else {
		Drawer_MinBB = Block_MinBB[block]; Drawer_MinBB.Y = 1.0f - Drawer_MinBB.Y;
		Drawer_MaxBB = Block_MaxBB[block]; Drawer_MaxBB.Y = 1.0f - Drawer_MaxBB.Y;
		Vector3 min = Block_MinBB[block], max = Block_MaxBB[block];

		Drawer_X1 = iso_scale * (1.0f - min.X * 2.0f) + iso_pos.X; 
		Drawer_X2 = iso_scale * (1.0f - max.X * 2.0f) + iso_pos.X;
		Drawer_Y1 = iso_scale * (1.0f - min.Y * 2.0f) + iso_pos.Y; 
		Drawer_Y2 = iso_scale * (1.0f - max.Y * 2.0f) + iso_pos.Y;
		Drawer_Z1 = iso_scale * (1.0f - min.Z * 2.0f) + iso_pos.Z; 
		Drawer_Z2 = iso_scale * (1.0f - max.Z * 2.0f) + iso_pos.Z;

		Drawer_Tinted = Block_Tinted[block];
		Drawer_TintColour = Block_FogColour[block];

		Drawer_XMax(1, bright ? iso_colNormal : iso_colXSide, 
			IsometricDrawer_GetTexLoc(block, FACE_XMAX), &iso_vertices);
		Drawer_ZMin(1, bright ? iso_colNormal : iso_colZSide, 
			IsometricDrawer_GetTexLoc(block, FACE_ZMIN), &iso_vertices);
		Drawer_YMax(1, iso_colNormal, 
			IsometricDrawer_GetTexLoc(block, FACE_YMAX), &iso_vertices);
		iso_count += 4 * 3;
	}
}

void IsometricDrawer_EndBatch(void) {
	if (iso_count > 0) {
		if (iso_texIndex != iso_lastTexIndex) Gfx_BindTexture(Atlas1D_TexIds[iso_texIndex]);
		GfxCommon_UpdateDynamicVb_IndexedTris(iso_vb, iso_vertices, iso_count);

		iso_count = 0;
		iso_lastTexIndex = -1;
	}
	Gfx_LoadIdentityMatrix();
}