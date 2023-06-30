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

static cc_bool iso_cacheInited;
static PackedCol iso_colorXSide, iso_colorZSide, iso_colorYBottom;
static float iso_posX, iso_posY;

#define iso_cosX  (0.86602540378443864f) /* cos(30  * MATH_DEG2RAD) */
#define iso_sinX  (0.50000000000000000f) /* sin(30  * MATH_DEG2RAD) */
#define iso_cosY  (0.70710678118654752f) /* cos(-45 * MATH_DEG2RAD) */
#define iso_sinY (-0.70710678118654752f) /* sin(-45 * MATH_DEG2RAD) */

static void IsometricDrawer_InitCache(void) {
	if (iso_cacheInited) return;

	iso_cacheInited = true;
	PackedCol_GetShaded(PACKEDCOL_WHITE,
		&iso_colorXSide, &iso_colorZSide, &iso_colorYBottom);
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
	float scale;

	*iso_state++ = texIndex;
	v.Col = PACKEDCOL_WHITE;
	Block_Tint(v.Col, block);

	/* Rescale by 0.70 in Classic mode to match vanilla size */
	/* Rescale by 0.88 in Enhanced mode to be slightly nicer */
	/*  Default selected size:  54px -> 48px */
	/*  Default inventory size: 36px -> 32px */
	/*  Default hotbar size:    28px -> 24px */
	scale = Game_ClassicMode ? 0.70f : 0.88f;
	size  = Math_Ceil(size * scale);
	minX  = iso_posX - size; maxX = iso_posX + size;
	minY  = iso_posY - size; maxY = iso_posY + size;

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
	struct VertexTextured* v;
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

	for (v = beg; v < iso_vertices; v++)
	{
		/* Cut down form of: */
		/*   Matrix_RotateY(&rotY,  45.0f * MATH_DEG2RAD); */
		/*   Matrix_RotateX(&rotX, -30.0f * MATH_DEG2RAD); */
		/*   Matrix_Mul(&iso_transform, &rotY, &rotX); */
		/*   ...                                       */
		/*   Vec3 vec = { v.X, v.Y, v.Z }; */
		/*   Vec3_Transform(&vec, &vec, &iso_transform); */
		/* With all unnecessary operations either simplified or removed */
		x = v->X * iso_cosY                              + v->Z * -iso_sinY;
		y = v->X * iso_sinX * iso_sinY + v->Y * iso_cosX + v->Z * iso_sinX * iso_cosY;

		v->X = x + iso_posX;
		v->Y = y + iso_posY;
	}
}

void IsometricDrawer_BeginBatch(struct VertexTextured* vertices, int* state) {
	IsometricDrawer_InitCache();
	iso_vertices      = vertices;
	iso_vertices_base = vertices;
	iso_state         = state; /* TODO just store TextureLoc ??? */
}

void IsometricDrawer_AddBatch(BlockID block, float size, float x, float y) {
	struct VertexTextured* beg = iso_vertices;
	if (Blocks.Draw[block] == DRAW_GAS) return;
	
	iso_posX = x; iso_posY = y;
	/* See comment in Gfx_Make2DQuad() for why 0.5 is subtracted in D3D9 */
	/* TODO pass as arguments? test diff */
#ifdef CC_BUILD_D3D9
	iso_posX -= 0.5f; iso_posY -= 0.5f;
#endif

	if (Blocks.Draw[block] == DRAW_SPRITE) {
		IsometricDrawer_Flat(block, size);
	} else {
		IsometricDrawer_Angled(block, size);
	}
}

int IsometricDrawer_EndBatch(void) {
	return (int)(iso_vertices - iso_vertices_base);
}

void IsometricDrawer_Render(int count, int offset, int* state) {
	int i, curIdx, batchBeg, batchLen;

	curIdx   = state[0];
	batchLen = 0;
	batchBeg = offset;

	for (i = 0; i < count / 4; i++, batchLen += 4) 
	{
		if (state[i] == curIdx) continue;

		/* Flush previous batch */
		Gfx_BindTexture(Atlas1D.TexIds[curIdx]);
		Gfx_DrawVb_IndexedTris_Range(batchLen, batchBeg);

		/* Reset for next batch */
		curIdx   = state[i];
		batchBeg += batchLen;
		batchLen = 0;
	}

	Gfx_BindTexture(Atlas1D.TexIds[curIdx]);
	Gfx_DrawVb_IndexedTris_Range(batchLen, batchBeg);
}
