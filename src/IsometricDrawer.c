#include "IsometricDrawer.h"
#include "Drawer.h"
#include "Graphics.h"
#include "PackedCol.h"
#include "ExtMath.h"
#include "Block.h"
#include "TexturePack.h"
#include "Block.h"

static float iso_scale;
static struct VertexTextured* iso_vertices;
static struct VertexTextured* iso_vertices_base;
static GfxResourceID iso_vb;

static cc_bool iso_cacheInited;
static PackedCol iso_color = PACKEDCOL_WHITE;
static PackedCol iso_colorXSide, iso_colorZSide, iso_colorYBottom;

#define iso_cosX  (0.86602540378443864f) /* cos(30  * MATH_DEG2RAD) */
#define iso_sinX  (0.50000000000000000f) /* sin(30  * MATH_DEG2RAD) */
#define iso_cosY  (0.70710678118654752f) /* cos(-45 * MATH_DEG2RAD) */
#define iso_sinY (-0.70710678118654752f) /* sin(-45 * MATH_DEG2RAD) */

static struct Matrix iso_transform;
static Vec3 iso_pos;
static int iso_lastTexIndex, iso_texIndex;

static void IsometricDrawer_RotateX(float cosA, float sinA) {
	float y   = cosA  * iso_pos.Y + sinA * iso_pos.Z;
	iso_pos.Z = -sinA * iso_pos.Y + cosA * iso_pos.Z;
	iso_pos.Y = y;
}

static void IsometricDrawer_RotateY(float cosA, float sinA) {
	float x   = cosA * iso_pos.X - sinA * iso_pos.Z;
	iso_pos.Z = sinA * iso_pos.X + cosA * iso_pos.Z;
	iso_pos.X = x;
}

static void IsometricDrawer_InitCache(void) {
	struct Matrix rotY, rotX;
	if (iso_cacheInited) return;

	iso_cacheInited = true;
	PackedCol_GetShaded(iso_color, 
		&iso_colorXSide, &iso_colorZSide, &iso_colorYBottom);

	Matrix_RotateY(&rotY,  45.0f * MATH_DEG2RAD);
	Matrix_RotateX(&rotX, -30.0f * MATH_DEG2RAD);
	Matrix_Mul(&iso_transform, &rotY, &rotX);
}

static void IsometricDrawer_Flush(void) {
	int count;
	if (iso_lastTexIndex != -1) {
		Gfx_BindTexture(Atlas1D.TexIds[iso_lastTexIndex]);
		count = (int)(iso_vertices - iso_vertices_base);
		Gfx_UpdateDynamicVb_IndexedTris(iso_vb, iso_vertices_base, count);
	}

	iso_lastTexIndex = iso_texIndex;
	iso_vertices     = iso_vertices_base;
}

static TextureLoc IsometricDrawer_GetTexLoc(BlockID block, Face face) {
	TextureLoc loc = Block_Tex(block, face);
	iso_texIndex   = Atlas1D_Index(loc);

	if (iso_lastTexIndex != iso_texIndex) IsometricDrawer_Flush();
	return loc;
}

static void IsometricDrawer_SpriteZQuad(BlockID block, cc_bool firstPart) {
	TextureLoc loc = Block_Tex(block, FACE_ZMAX);
	TextureRec rec = Atlas1D_TexRec(loc, 1, &iso_texIndex);

	struct VertexTextured v;
	float minX, maxX, minY, maxY;
	float x1, x2;

	if (iso_lastTexIndex != iso_texIndex) IsometricDrawer_Flush();
	v.Col = iso_color;
	Block_Tint(v.Col, block);

	x1 = firstPart ? 0.5f : -0.1f;
	x2 = firstPart ? 1.1f :  0.5f;
	rec.U1 = (firstPart ? 0.0f : 0.5f);
	rec.U2 = (firstPart ? 0.5f : 1.0f) * UV2_Scale;

	minX = iso_scale * (1.0f - x1   * 2.0f) + iso_pos.X;
	maxX = iso_scale * (1.0f - x2   * 2.0f) + iso_pos.X;
	minY = iso_scale * (1.0f - 0.0f * 2.0f) + iso_pos.Y;
	maxY = iso_scale * (1.0f - 1.1f * 2.0f) + iso_pos.Y;

	v.Z = iso_pos.Z;
	v.X = minX; v.Y = minY; v.U = rec.U2; v.V = rec.V2; *iso_vertices++ = v;
	            v.Y = maxY;               v.V = rec.V1; *iso_vertices++ = v;
	v.X = maxX;             v.U = rec.U1;               *iso_vertices++ = v;
	            v.Y = minY;               v.V = rec.V2; *iso_vertices++ = v;
}

static void IsometricDrawer_SpriteXQuad(BlockID block, cc_bool firstPart) {
	TextureLoc loc = Block_Tex(block, FACE_XMAX);
	TextureRec rec = Atlas1D_TexRec(loc, 1, &iso_texIndex);
	
	struct VertexTextured v;
	float minY, maxY, minZ, maxZ;
	float z1, z2;

	if (iso_lastTexIndex != iso_texIndex) IsometricDrawer_Flush();
	v.Col = iso_color;
	Block_Tint(v.Col, block);

	z1 = firstPart ? 0.5f : -0.1f;
	z2 = firstPart ? 1.1f :  0.5f;
	rec.U1 = (firstPart ? 0.0f : 0.5f);
	rec.U2 = (firstPart ? 0.5f : 1.0f) * UV2_Scale;

	minY = iso_scale * (1.0f - 0.0f * 2.0f) + iso_pos.Y;
	maxY = iso_scale * (1.0f - 1.1f * 2.0f) + iso_pos.Y;
	minZ = iso_scale * (1.0f - z1   * 2.0f) + iso_pos.Z;
	maxZ = iso_scale * (1.0f - z2   * 2.0f) + iso_pos.Z;

	v.X = iso_pos.X;
	v.Y = minY; v.Z = minZ; v.U = rec.U2; v.V = rec.V2; *iso_vertices++ = v;
	v.Y = maxY;                           v.V = rec.V1; *iso_vertices++ = v;
	            v.Z = maxZ; v.U = rec.U1;               *iso_vertices++ = v;
	v.Y = minY;                           v.V = rec.V2; *iso_vertices++ = v;
}

void IsometricDrawer_BeginBatch(struct VertexTextured* vertices, GfxResourceID vb) {
	IsometricDrawer_InitCache();
	iso_lastTexIndex = -1;
	iso_vertices = vertices;
	iso_vertices_base = vertices;
	iso_vb = vb;

	Gfx_LoadMatrix(MATRIX_VIEW, &iso_transform);
}

void IsometricDrawer_DrawBatch(BlockID block, float size, float x, float y) {
	cc_bool bright = Blocks.FullBright[block];
	Vec3 min, max;
	if (Blocks.Draw[block] == DRAW_GAS) return;

	/* isometric coords size: cosY * -scale - sinY * scale */
	/* we need to divide by (2 * cosY), as the calling function expects size to be in pixels. */
	iso_scale = size / (2.0f * iso_cosY);

	/* screen to isometric coords (cos(-x) = cos(x), sin(-x) = -sin(x)) */
	iso_pos.X = x; iso_pos.Y = y; iso_pos.Z = 0.0f;
	IsometricDrawer_RotateX(iso_cosX, -iso_sinX);
	IsometricDrawer_RotateY(iso_cosY, -iso_sinY);

	/* See comment in GfxCommon_Draw2DTexture() */
	iso_pos.X -= 0.5f; iso_pos.Y -= 0.5f;

	if (Blocks.Draw[block] == DRAW_SPRITE) {
		IsometricDrawer_SpriteXQuad(block, true);
		IsometricDrawer_SpriteZQuad(block, true);

		IsometricDrawer_SpriteZQuad(block, false);
		IsometricDrawer_SpriteXQuad(block, false);
	} else {
		Drawer.MinBB = Blocks.MinBB[block]; Drawer.MinBB.Y = 1.0f - Drawer.MinBB.Y;
		Drawer.MaxBB = Blocks.MaxBB[block]; Drawer.MaxBB.Y = 1.0f - Drawer.MaxBB.Y;
		min = Blocks.MinBB[block]; max = Blocks.MaxBB[block];

		Drawer.X1 = iso_scale * (1.0f - min.X * 2.0f) + iso_pos.X; 
		Drawer.X2 = iso_scale * (1.0f - max.X * 2.0f) + iso_pos.X;
		Drawer.Y1 = iso_scale * (1.0f - min.Y * 2.0f) + iso_pos.Y; 
		Drawer.Y2 = iso_scale * (1.0f - max.Y * 2.0f) + iso_pos.Y;
		Drawer.Z1 = iso_scale * (1.0f - min.Z * 2.0f) + iso_pos.Z; 
		Drawer.Z2 = iso_scale * (1.0f - max.Z * 2.0f) + iso_pos.Z;

		Drawer.Tinted  = Blocks.Tinted[block];
		Drawer.TintCol = Blocks.FogCol[block];

		Drawer_XMax(1, bright ? iso_color : iso_colorXSide,
			IsometricDrawer_GetTexLoc(block, FACE_XMAX), &iso_vertices);
		Drawer_ZMin(1, bright ? iso_color : iso_colorZSide,
			IsometricDrawer_GetTexLoc(block, FACE_ZMIN), &iso_vertices);
		Drawer_YMax(1, iso_color,
			IsometricDrawer_GetTexLoc(block, FACE_YMAX), &iso_vertices);
	}
}

void IsometricDrawer_EndBatch(void) {
	if (iso_vertices != iso_vertices_base) { 
		iso_lastTexIndex = iso_texIndex; 
		IsometricDrawer_Flush(); 
	}

	iso_lastTexIndex = -1;
	Gfx_LoadIdentityMatrix(MATRIX_VIEW);
}
