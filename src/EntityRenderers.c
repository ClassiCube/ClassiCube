#include "EntityRenderers.h"
#include "Entity.h"
#include "Bitmap.h"
#include "Block.h"
#include "Event.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Game.h"
#include "Graphics.h"
#include "Model.h"
#include "World.h"

/*########################################################################################################################*
*------------------------------------------------------Entity Shadow------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool shadows_boundTex;
static GfxResourceID shadows_tex;
static float shadow_radius, shadow_uvScale;
struct ShadowData { float Y; BlockID Block; cc_uint8 A; };

static cc_bool lequal(float a, float b) { return a < b || Math_AbsF(a - b) < 0.001f; }
static void EntityShadow_DrawCoords(struct VertexTextured** vertices, struct Entity* e, struct ShadowData* data, float x1, float z1, float x2, float z2) {
	PackedCol col;
	struct VertexTextured* v;
	Vec3 cen;
	float u1, v1, u2, v2;

	if (lequal(x2, x1) || lequal(z2, z1)) return;
	cen = e->Position;

	u1 = (x1 - cen.X) * shadow_uvScale + 0.5f;
	v1 = (z1 - cen.Z) * shadow_uvScale + 0.5f;
	u2 = (x2 - cen.X) * shadow_uvScale + 0.5f;
	v2 = (z2 - cen.Z) * shadow_uvScale + 0.5f;
	if (u2 <= 0.0f || v2 <= 0.0f || u1 >= 1.0f || v1 >= 1.0f) return;

	x1 = max(x1, cen.X - shadow_radius); u1 = u1 >= 0.0f ? u1 : 0.0f;
	z1 = max(z1, cen.Z - shadow_radius); v1 = v1 >= 0.0f ? v1 : 0.0f;
	x2 = min(x2, cen.X + shadow_radius); u2 = u2 <= 1.0f ? u2 : 1.0f;
	z2 = min(z2, cen.Z + shadow_radius); v2 = v2 <= 1.0f ? v2 : 1.0f;

	v   = *vertices;
	col = PackedCol_Make(255, 255, 255, data->A);

	v->X = x1; v->Y = data->Y; v->Z = z1; v->Col = col; v->U = u1; v->V = v1; v++;
	v->X = x2; v->Y = data->Y; v->Z = z1; v->Col = col; v->U = u2; v->V = v1; v++;
	v->X = x2; v->Y = data->Y; v->Z = z2; v->Col = col; v->U = u2; v->V = v2; v++;
	v->X = x1; v->Y = data->Y; v->Z = z2; v->Col = col; v->U = u1; v->V = v2; v++;

	*vertices = v;
}

static void EntityShadow_DrawSquareShadow(struct VertexTextured** vertices, float y, float x, float z) {
	PackedCol col = PackedCol_Make(255, 255, 255, 220);
	float     uv1 = 63/128.0f, uv2 = 64/128.0f;
	struct VertexTextured* v = *vertices;

	v->X = x;     v->Y = y; v->Z = z;     v->Col = col; v->U = uv1; v->V = uv1; v++;
	v->X = x + 1; v->Y = y; v->Z = z;     v->Col = col; v->U = uv2; v->V = uv1; v++;
	v->X = x + 1; v->Y = y; v->Z = z + 1; v->Col = col; v->U = uv2; v->V = uv2; v++;
	v->X = x;     v->Y = y; v->Z = z + 1; v->Col = col; v->U = uv1; v->V = uv2; v++;

	*vertices = v;
}

/* Shadow may extend down multiple blocks vertically */
/* If so, shadow on a block must be 'chopped up' to avoid a shadow underneath block above this one */
static void EntityShadow_DrawCircle(struct VertexTextured** vertices, struct Entity* e, struct ShadowData* data, float x, float z) {
	Vec3 min, max, nMin, nMax;
	int i;
	x = (float)Math_Floor(x); z = (float)Math_Floor(z);
	min = Blocks.MinBB[data[0].Block]; max = Blocks.MaxBB[data[0].Block];

	EntityShadow_DrawCoords(vertices, e, &data[0], x + min.X, z + min.Z, x + max.X, z + max.Z);
	for (i = 1; i < 4; i++) {
		if (data[i].Block == BLOCK_AIR) return;
		nMin = Blocks.MinBB[data[i].Block]; nMax = Blocks.MaxBB[data[i].Block];

		EntityShadow_DrawCoords(vertices, e, &data[i], x +  min.X, z + nMin.Z, x +  max.X, z +  min.Z);
		EntityShadow_DrawCoords(vertices, e, &data[i], x +  min.X, z +  max.Z, x +  max.X, z + nMax.Z);

		EntityShadow_DrawCoords(vertices, e, &data[i], x + nMin.X, z + nMin.Z, x +  min.X, z + nMax.Z);
		EntityShadow_DrawCoords(vertices, e, &data[i], x +  max.X, z + nMin.Z, x + nMax.X, z + nMax.Z);
		min = nMin; max = nMax;
	}
}

static void EntityShadow_CalcAlpha(float playerY, struct ShadowData* data) {
	float height = playerY - data->Y;
	if (height <= 6.0f) {
		data->A = (cc_uint8)(160 - 160 * height / 6.0f);
		data->Y += 1.0f / 64.0f; return;
	}

	data->A = 0;
	if (height <= 16.0f)      data->Y += 1.0f / 64.0f;
	else if (height <= 32.0f) data->Y += 1.0f / 16.0f;
	else if (height <= 96.0f) data->Y += 1.0f / 8.0f;
	else data->Y += 1.0f / 4.0f;
}

static cc_bool EntityShadow_GetBlocks(struct Entity* e, int x, int y, int z, struct ShadowData* data) {
	struct ShadowData zeroData = { 0 };
	struct ShadowData* cur;
	float posY, topY;
	cc_bool outside;
	BlockID block; cc_uint8 draw;
	int i;

	for (i = 0; i < 4; i++) { data[i] = zeroData; }
	cur     = data;
	posY    = e->Position.Y;
	outside = !World_ContainsXZ(x, z);

	for (i = 0; y >= 0 && i < 4; y--) {
		if (!outside) {
			block = World_GetBlock(x, y, z);
		} else if (y == Env.EdgeHeight - 1) {
			block = Blocks.Draw[Env.EdgeBlock] == DRAW_GAS  ? BLOCK_AIR : BLOCK_BEDROCK;
		} else if (y == Env_SidesHeight - 1) {
			block = Blocks.Draw[Env.SidesBlock] == DRAW_GAS ? BLOCK_AIR : BLOCK_BEDROCK;
		} else {
			block = BLOCK_AIR;
		}

		draw = Blocks.Draw[block];
		if (draw == DRAW_GAS || draw == DRAW_SPRITE || Blocks.IsLiquid[block]) continue;
		topY = y + Blocks.MaxBB[block].Y;
		if (topY >= posY + 0.01f) continue;

		cur->Block = block; cur->Y = topY;
		EntityShadow_CalcAlpha(posY, cur);
		i++; cur++;

		/* Check if the casted shadow will continue on further down. */
		if (Blocks.MinBB[block].X == 0.0f && Blocks.MaxBB[block].X == 1.0f &&
			Blocks.MinBB[block].Z == 0.0f && Blocks.MaxBB[block].Z == 1.0f) return true;
	}

	if (i < 4) {
		cur->Block = Env.EdgeBlock; cur->Y = 0.0f;
		EntityShadow_CalcAlpha(posY, cur);
		i++; cur++;
	}
	return true;
}

static void EntityShadow_Draw(struct Entity* e) {
	struct VertexTextured vertices[128];
	struct VertexTextured* ptr;
	struct ShadowData data[4];
	GfxResourceID vb;
	Vec3 pos;
	float radius;
	int y, count;
	int x1, z1, x2, z2;

	pos = e->Position;
	if (pos.Y < 0.0f) return;
	y = min((int)pos.Y, World.MaxY);

	radius = 7.0f * min(e->ModelScale.Y, 1.0f) * e->Model->shadowScale;
	shadow_radius  = radius / 16.0f;
	shadow_uvScale = 16.0f / (radius * 2.0f);

	/* TODO: Should shadow component use its own VB? */
	ptr = vertices;
	if (Entities.ShadowsMode == SHADOW_MODE_SNAP_TO_BLOCK) {
		vb = Gfx_texVb;
		x1 = Math_Floor(pos.X); z1 = Math_Floor(pos.Z);
		if (!EntityShadow_GetBlocks(e, x1, y, z1, data)) return;

		EntityShadow_DrawSquareShadow(&ptr, data[0].Y, x1, z1);
	} else {
		vb = Models.Vb;
		x1 = Math_Floor(pos.X - shadow_radius); z1 = Math_Floor(pos.Z - shadow_radius);
		x2 = Math_Floor(pos.X + shadow_radius); z2 = Math_Floor(pos.Z + shadow_radius);

		if (EntityShadow_GetBlocks(e, x1, y, z1, data) && data[0].A > 0) {
			EntityShadow_DrawCircle(&ptr, e, data, (float)x1, (float)z1);
		}
		if (x1 != x2 && EntityShadow_GetBlocks(e, x2, y, z1, data) && data[0].A > 0) {
			EntityShadow_DrawCircle(&ptr, e, data, (float)x2, (float)z1);
		}
		if (z1 != z2 && EntityShadow_GetBlocks(e, x1, y, z2, data) && data[0].A > 0) {
			EntityShadow_DrawCircle(&ptr, e, data, (float)x1, (float)z2);
		}
		if (x1 != x2 && z1 != z2 && EntityShadow_GetBlocks(e, x2, y, z2, data) && data[0].A > 0) {
			EntityShadow_DrawCircle(&ptr, e, data, (float)x2, (float)z2);
		}
	}

	if (ptr == vertices) return;

	if (!shadows_boundTex) {
		Gfx_BindTexture(shadows_tex);
		shadows_boundTex = true;
	}

	count = (int)(ptr - vertices);
	Gfx_UpdateDynamicVb_IndexedTris(vb, vertices, count);
}


/*########################################################################################################################*
*-----------------------------------------------------Entity Shadows------------------------------------------------------*
*#########################################################################################################################*/
#define sh_size 128
#define sh_half (sh_size / 2)

static void EntityShadows_MakeTexture(void) {
	BitmapCol pixels[sh_size * sh_size];
	BitmapCol color = BitmapCol_Make(0, 0, 0, 200);
	struct Bitmap bmp;
	cc_uint32 x, y;

	Bitmap_Init(bmp, sh_size, sh_size, pixels);
	for (y = 0; y < sh_size; y++) {
		BitmapCol* row = Bitmap_GetRow(&bmp, y);

		for (x = 0; x < sh_size; x++) {
			double dist =
				(sh_half - (x + 0.5)) * (sh_half - (x + 0.5)) +
				(sh_half - (y + 0.5)) * (sh_half - (y + 0.5));
			row[x] = dist < sh_half * sh_half ? color : 0;
		}
	}
	Gfx_RecreateTexture(&shadows_tex, &bmp, 0, false);
}

void EntityShadows_Render(void) {
	int i;
	if (Entities.ShadowsMode == SHADOW_MODE_NONE) return;

	shadows_boundTex = false;
	if (!shadows_tex) EntityShadows_MakeTexture();

	Gfx_SetAlphaArgBlend(true);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaBlending(true);

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	EntityShadow_Draw(Entities.List[ENTITIES_SELF_ID]);

	if (Entities.ShadowsMode == SHADOW_MODE_CIRCLE_ALL) {	
		for (i = 0; i < ENTITIES_SELF_ID; i++) 
		{
			if (!Entities.List[i] || !Entities.List[i]->ShouldRender) continue;
			EntityShadow_Draw(Entities.List[i]);
		}
	}

	Gfx_SetAlphaArgBlend(false);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);
}


/*########################################################################################################################*
*-----------------------------------------------Entity renderers component------------------------------------------------*
*#########################################################################################################################*/
static void EntityRenderers_ContextLost(void* obj) {
	Gfx_DeleteTexture(&shadows_tex);
}

static void EntityRenderers_Init(void) {
	Event_Register_(&GfxEvents.ContextLost, NULL, EntityRenderers_ContextLost);
}

static void EntityRenderers_Free(void) {
	Gfx_DeleteTexture(&shadows_tex);
}

struct IGameComponent EntityRenderers_Component = {
	EntityRenderers_Init,  /* Init  */
	EntityRenderers_Free   /* Free  */
};
