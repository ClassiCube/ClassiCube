#include "IsometricDrawer.h"
#include "Drawer.h"
#include "Graphics.h"
#include "PackedCol.h"
#include "ExtMath.h"
#include "Block.h"
#include "TexturePack.h"
#include "Block.h"
#include "Game.h"

static struct VertexTextured* iso_vertices;
static struct VertexTextured* iso_vertices_base;
static int* iso_state;
static int* iso_state_base;

static cc_bool iso_cacheInited;
static PackedCol iso_colorXSide, iso_colorZSide, iso_colorYBottom;

#define iso_cosX  (0.86602540378443864f) /* cos(30  * MATH_DEG2RAD) */
#define iso_sinX  (0.50000000000000000f) /* sin(30  * MATH_DEG2RAD) */
#define iso_cosY  (0.70710678118654752f) /* cos(-45 * MATH_DEG2RAD) */
#define iso_sinY (-0.70710678118654752f) /* sin(-45 * MATH_DEG2RAD) */

static struct Matrix iso_transform;
static float iso_posX, iso_posY;

static void IsometricDrawer_InitCache(void) {
	struct Matrix rotY, rotX;
	if (iso_cacheInited) return;

	iso_cacheInited = true;
	PackedCol_GetShaded(PACKEDCOL_WHITE,
		&iso_colorXSide, &iso_colorZSide, &iso_colorYBottom);

	Matrix_RotateY(&rotY,  45.0f * MATH_DEG2RAD);
	Matrix_RotateX(&rotX, -30.0f * MATH_DEG2RAD);
	Matrix_Mul(&iso_transform, &rotY, &rotX);
}

static TextureLoc IsometricDrawer_GetTexLoc(BlockID block, Face face) {
	TextureLoc loc = Block_Tex(block, face);
	*iso_state++   = Atlas1D_Index(loc);
	return loc;
}

static void IsometricDrawer_Flat(BlockID block, float size) {
	int texIndex;
	TextureLoc loc = Block_Tex(block, FACE_ZMAX);
	TextureRec rec = Atlas1D_TexRec(loc, 1, &texIndex);

	struct VertexTextured v;
	float minX, maxX, minY, maxY;

	*iso_state++ = texIndex;
	v.Col = PACKEDCOL_WHITE;
	Block_Tint(v.Col, block);

	/* Rescale by 0.70 in Classic mode to match vanilla size */
	/* Rescale by 0.88 in Enhanced mode to be slightly nicer */
	/*  Default selected size:  54px -> 48px */
	/*  Default inventory size: 36px -> 32px */
	/*  Default hotbar size:    28px -> 24px */
	float scale = Game_ClassicMode ? 0.70f : 0.88f;
	size = Math_Ceil(size * scale);
	minX = iso_posX - size; maxX = iso_posX + size;
	minY = iso_posY - size; maxY = iso_posY + size;

	v.Z = 0.0f;
	v.X = minX; v.Y = minY; v.U = rec.U1; v.V = rec.V1; *iso_vertices++ = v;
	            v.Y = maxY;               v.V = rec.V2; *iso_vertices++ = v;
	v.X = maxX;             v.U = rec.U2;               *iso_vertices++ = v;
	            v.Y = minY;               v.V = rec.V1; *iso_vertices++ = v;
}

static void IsometricDrawer_Angled(BlockID block, float size) {
	cc_bool bright;
	Vec3 min, max;
	struct VertexTextured* beg = iso_vertices;
	float x, y, scale;

	/* isometric coords size: cosY * -scale - sinY * scale */
	/* we need to divide by (2 * cosY), as the calling function expects size to be in pixels. */
	scale = size / (2.0f * iso_cosY);

	Drawer.MinBB = Blocks.MinBB[block]; Drawer.MinBB.Y = 1.0f - Drawer.MinBB.Y;
	Drawer.MaxBB = Blocks.MaxBB[block]; Drawer.MaxBB.Y = 1.0f - Drawer.MaxBB.Y;
	min = Blocks.MinBB[block]; max = Blocks.MaxBB[block];

	Drawer.X1 = scale * (1.0f - min.X * 2.0f);
	Drawer.X2 = scale * (1.0f - max.X * 2.0f);
	Drawer.Y1 = scale * (1.0f - min.Y * 2.0f);
	Drawer.Y2 = scale * (1.0f - max.Y * 2.0f);
	Drawer.Z1 = scale * (1.0f - min.Z * 2.0f);
	Drawer.Z2 = scale * (1.0f - max.Z * 2.0f);

	bright = Blocks.FullBright[block];
	Drawer.Tinted  = Blocks.Tinted[block];
	Drawer.TintCol = Blocks.FogCol[block];

	Drawer_XMax(1, bright ? PACKEDCOL_WHITE : iso_colorXSide,
		IsometricDrawer_GetTexLoc(block, FACE_XMAX), &iso_vertices);
	Drawer_ZMin(1, bright ? PACKEDCOL_WHITE : iso_colorZSide,
		IsometricDrawer_GetTexLoc(block, FACE_ZMIN), &iso_vertices);
	Drawer_YMax(1, PACKEDCOL_WHITE,
		IsometricDrawer_GetTexLoc(block, FACE_YMAX), &iso_vertices);

	for (struct VertexTextured* v = beg; v < iso_vertices; v++)
	{
		/* Cut down Vec3_Transform (row4 is always 0, and don't need Z) */
		struct Matrix* mat = &iso_transform;
		x = v->X * mat->row1.X + v->Y * mat->row2.X + v->Z * mat->row3.X;
		y = v->X * mat->row1.Y + v->Y * mat->row2.Y + v->Z * mat->row3.Y;

		v->X = x + iso_posX;
		v->Y = y + iso_posY;
	}
}

void IsometricDrawer_BeginBatch(struct VertexTextured* vertices, int* state) {
	IsometricDrawer_InitCache();
	iso_vertices      = vertices;
	iso_vertices_base = vertices;
	iso_state         = state;
	iso_state_base    = state;
}

void IsometricDrawer_AddBatch(BlockID block, float size, float x, float y) {
	struct VertexTextured* beg = iso_vertices;
	if (Blocks.Draw[block] == DRAW_GAS) return;

	/* See comment in GfxCommon_Draw2DTexture() for why 0.5 is subtracted */
	/* TODO only for Direct3D 9?? pass in registers? */
	iso_posX = x - 0.5f; 
	iso_posY = y - 0.5f;

	if (Blocks.Draw[block] == DRAW_SPRITE) {
		IsometricDrawer_Flat(block, size);
	} else {
		IsometricDrawer_Angled(block, size);
	}
}

static void IsometricDrawer_Render(GfxResourceID vb) {
	int curIdx, batchBeg, batchLen;
	int count, i;
	
	count = (int)(iso_vertices - iso_vertices_base);
	Gfx_SetDynamicVbData(vb, iso_vertices_base, count);

	curIdx   = iso_state_base[0];
	batchLen = 0;
	batchBeg = 0;

	for (i = 0; i < count / 4; i++, batchLen += 4) 
	{
		if (iso_state_base[i] == curIdx) continue;

		/* Flush previous batch */
		Gfx_BindTexture(Atlas1D.TexIds[curIdx]);
		Gfx_DrawVb_IndexedTris_Range(batchLen, batchBeg);

		/* Reset for next batch */
		curIdx   = iso_state_base[i];
		batchBeg = i * 4;
		batchLen = 0;
	}

	Gfx_BindTexture(Atlas1D.TexIds[curIdx]);
	Gfx_DrawVb_IndexedTris_Range(batchLen, batchBeg);
}

void IsometricDrawer_EndBatch(GfxResourceID vb) {
	if (iso_state != iso_state_base) {
		IsometricDrawer_Render(vb);
	}
	Gfx_LoadIdentityMatrix(MATRIX_VIEW);
}