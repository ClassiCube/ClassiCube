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
#include "Particle.h"
#include "Drawer2D.h"

/*########################################################################################################################*
*------------------------------------------------------Entity Shadow------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool shadows_boundTex;
static GfxResourceID shadows_VB;
static GfxResourceID shadows_tex;
static float shadow_radius, shadow_uvScale;
struct ShadowData { float y; BlockID Block; cc_uint8 A; };

/* Circle shadows extend at most 4 blocks vertically */
#define SHADOW_MAX_RANGE 4 
/* Circle shadows on blocks underneath the top block can be chopped up into at most 4 pieces */
#define SHADOW_MAX_PER_SUB_BLOCK (4 * 4) 
/* Circle shadows use at most:
   - 4 vertices for top most block
   - MAX_PER_SUB_BLOCK for everyblock underneath the top block */
#define SHADOW_MAX_PER_COLUMN (4 + SHADOW_MAX_PER_SUB_BLOCK * (SHADOW_MAX_RANGE - 1))
/* Circle shadows may be split across (x,z), (x,z+1), (x+1,z), (x+1,z+1) */
#define SHADOW_MAX_VERTS 4 * SHADOW_MAX_PER_COLUMN

static cc_bool lequal(float a, float b) { return a < b || Math_AbsF(a - b) < 0.001f; }
static void EntityShadow_DrawCoords(struct VertexTextured** vertices, struct Entity* e, struct ShadowData* data, float x1, float z1, float x2, float z2) {
	PackedCol col;
	struct VertexTextured* v;
	Vec3 cen;
	float u1, v1, u2, v2;

	if (lequal(x2, x1) || lequal(z2, z1)) return;
	cen = e->Position;

	u1 = (x1 - cen.x) * shadow_uvScale + 0.5f;
	v1 = (z1 - cen.z) * shadow_uvScale + 0.5f;
	u2 = (x2 - cen.x) * shadow_uvScale + 0.5f;
	v2 = (z2 - cen.z) * shadow_uvScale + 0.5f;
	if (u2 <= 0.0f || v2 <= 0.0f || u1 >= 1.0f || v1 >= 1.0f) return;

	x1 = max(x1, cen.x - shadow_radius); u1 = u1 >= 0.0f ? u1 : 0.0f;
	z1 = max(z1, cen.z - shadow_radius); v1 = v1 >= 0.0f ? v1 : 0.0f;
	x2 = min(x2, cen.x + shadow_radius); u2 = u2 <= 1.0f ? u2 : 1.0f;
	z2 = min(z2, cen.z + shadow_radius); v2 = v2 <= 1.0f ? v2 : 1.0f;

	v   = *vertices;
	col = PackedCol_Make(255, 255, 255, data->A);

	v->x = x1; v->y = data->y; v->z = z1; v->Col = col; v->U = u1; v->V = v1; v++;
	v->x = x2; v->y = data->y; v->z = z1; v->Col = col; v->U = u2; v->V = v1; v++;
	v->x = x2; v->y = data->y; v->z = z2; v->Col = col; v->U = u2; v->V = v2; v++;
	v->x = x1; v->y = data->y; v->z = z2; v->Col = col; v->U = u1; v->V = v2; v++;

	*vertices = v;
}

static void EntityShadow_DrawSquareShadow(struct VertexTextured** vertices, float y, float x, float z) {
	PackedCol col = PackedCol_Make(255, 255, 255, 220);
	float     uv1 = 63/128.0f, uv2 = 64/128.0f;
	struct VertexTextured* v = *vertices;

	v->x = x;     v->y = y; v->z = z;     v->Col = col; v->U = uv1; v->V = uv1; v++;
	v->x = x + 1; v->y = y; v->z = z;     v->Col = col; v->U = uv2; v->V = uv1; v++;
	v->x = x + 1; v->y = y; v->z = z + 1; v->Col = col; v->U = uv2; v->V = uv2; v++;
	v->x = x;     v->y = y; v->z = z + 1; v->Col = col; v->U = uv1; v->V = uv2; v++;

	*vertices = v;
}

/* Shadow may extend down multiple blocks vertically */
/* If so, shadow on a block must be 'chopped up' to avoid a shadow underneath block above this one */
static void EntityShadow_DrawCircle(struct VertexTextured** vertices, struct Entity* e, struct ShadowData* data, float x, float z) {
	Vec3 min, max, nMin, nMax;
	int i;
	x = (float)Math_Floor(x); z = (float)Math_Floor(z);
	min = Blocks.MinBB[data[0].Block]; max = Blocks.MaxBB[data[0].Block];

	EntityShadow_DrawCoords(vertices, e, &data[0], x + min.x, z + min.z, x + max.x, z + max.z);
	for (i = 1; i < 4; i++) 
	{
		if (data[i].Block == BLOCK_AIR) return;
		nMin = Blocks.MinBB[data[i].Block]; nMax = Blocks.MaxBB[data[i].Block];

		EntityShadow_DrawCoords(vertices, e, &data[i], x +  min.x, z + nMin.z, x +  max.x, z +  min.z);
		EntityShadow_DrawCoords(vertices, e, &data[i], x +  min.x, z +  max.z, x +  max.x, z + nMax.z);

		EntityShadow_DrawCoords(vertices, e, &data[i], x + nMin.x, z + nMin.z, x +  min.x, z + nMax.z);
		EntityShadow_DrawCoords(vertices, e, &data[i], x +  max.x, z + nMin.z, x + nMax.x, z + nMax.z);
		min = nMin; max = nMax;
	}
}

static void EntityShadow_CalcAlpha(float playerY, struct ShadowData* data) {
	float height = playerY - data->y;
	if (height <= 6.0f) {
		data->A = (cc_uint8)(160 - 160 * height / 6.0f);
		data->y += 1.0f / 64.0f; return;
	}

	data->A = 0;
	if (height <= 16.0f)      data->y += 1.0f / 64.0f;
	else if (height <= 32.0f) data->y += 1.0f / 16.0f;
	else if (height <= 96.0f) data->y += 1.0f / 8.0f;
	else data->y += 1.0f / 4.0f;
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
	posY    = e->Position.y;
	outside = !World_ContainsXZ(x, z);

	for (i = 0; y >= 0 && i < 4; y--) 
	{
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
		topY = y + Blocks.MaxBB[block].y;
		if (topY >= posY + 0.01f) continue;

		cur->Block = block; cur->y = topY;
		EntityShadow_CalcAlpha(posY, cur);
		i++; cur++;

		/* Check if the casted shadow will continue on further down. */
		if (Blocks.MinBB[block].x == 0.0f && Blocks.MaxBB[block].x == 1.0f &&
			Blocks.MinBB[block].z == 0.0f && Blocks.MaxBB[block].z == 1.0f) return true;
	}

	if (i < 4) {
		cur->Block = Env.EdgeBlock; cur->y = 0.0f;
		EntityShadow_CalcAlpha(posY, cur);
		i++; cur++;
	}
	return true;
}

static void EntityShadow_Draw(struct Entity* e) {
	struct VertexTextured vertices[128]; /* TODO this is less than maxVertes */
	struct VertexTextured* ptr;
	struct ShadowData data[4];
	Vec3 pos;
	float radius;
	int y, count;
	int x1, z1, x2, z2;

	pos = e->Position;
	if (pos.y < 0.0f) return;
	y = min((int)pos.y, World.MaxY);

	radius = 7.0f * min(e->ModelScale.y, 1.0f) * e->Model->shadowScale;
	shadow_radius  = radius / 16.0f;
	shadow_uvScale = 16.0f / (radius * 2.0f);

	ptr = vertices;
	if (Entities.ShadowsMode == SHADOW_MODE_SNAP_TO_BLOCK) {
		x1 = Math_Floor(pos.x); z1 = Math_Floor(pos.z);
		if (!EntityShadow_GetBlocks(e, x1, y, z1, data)) return;

		EntityShadow_DrawSquareShadow(&ptr, data[0].y, x1, z1);
	} else {
		x1 = Math_Floor(pos.x - shadow_radius); z1 = Math_Floor(pos.z - shadow_radius);
		x2 = Math_Floor(pos.x + shadow_radius); z2 = Math_Floor(pos.z + shadow_radius);

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
	Gfx_SetDynamicVbData(shadows_VB, vertices, count);
	Gfx_DrawVb_IndexedTris(count);
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
	shadows_tex = Gfx_CreateTexture(&bmp, 0, false);
}

void EntityShadows_Render(void) {
	int i;
	if (Entities.ShadowsMode == SHADOW_MODE_NONE) return;

	shadows_boundTex = false;
	if (!shadows_tex) 
		EntityShadows_MakeTexture();
	if (!shadows_VB)
		shadows_VB = Gfx_CreateDynamicVb(VERTEX_FORMAT_TEXTURED, SHADOW_MAX_VERTS);

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
*-----------------------------------------------------Entity nametag------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID names_VB;
#define NAME_IS_EMPTY -30000
#define NAME_OFFSET 3 /* offset of back layer of name above an entity */

static void MakeNameTexture(struct Entity* e) {
	cc_string colorlessName; char colorlessBuffer[STRING_SIZE];
	BitmapCol shadowColor = BitmapCol_Make(80, 80, 80, 255);
	BitmapCol origWhiteColor;

	struct DrawTextArgs args;
	struct FontDesc font;
	struct Context2D ctx;
	int width, height;
	cc_string name;

	/* Names are always drawn using default.png font */
	Font_MakeBitmapped(&font, 24, FONT_FLAGS_NONE);
	/* Don't want DPI scaling or padding */
	font.size = 24; font.height = 24;

	name = String_FromRawArray(e->NameRaw);
	DrawTextArgs_Make(&args, &name, &font, false);
	width = Drawer2D_TextWidth(&args);

	if (!width) {
		e->NameTex.ID = 0;
		e->NameTex.x  = NAME_IS_EMPTY;
	} else {
		String_InitArray(colorlessName, colorlessBuffer);
		width  += NAME_OFFSET; 
		height = Drawer2D_TextHeight(&args) + NAME_OFFSET;

		Context2D_Alloc(&ctx, width, height);
		{
			origWhiteColor = Drawer2D.Colors['f'];

			Drawer2D.Colors['f'] = shadowColor;
			Drawer2D_WithoutColors(&colorlessName, &name);
			args.text = colorlessName;
			Context2D_DrawText(&ctx, &args, NAME_OFFSET, NAME_OFFSET);

			Drawer2D.Colors['f'] = origWhiteColor;
			args.text = name;
			Context2D_DrawText(&ctx, &args, 0, 0);
		}
		Context2D_MakeTexture(&e->NameTex, &ctx);
		Context2D_Free(&ctx);
	}
}

static void DrawName(struct Entity* e) {
	struct VertexTextured* vertices;
	struct Model* model;
	struct Matrix mat;
	Vec3 pos;
	float scale;
	Vec2 size;

	if (!e->VTABLE->ShouldRenderName(e)) return;
	if (e->NameTex.x == NAME_IS_EMPTY)   return;
	if (!e->NameTex.ID) MakeNameTexture(e);
	Gfx_BindTexture(e->NameTex.ID);

	if (!names_VB)
		names_VB = Gfx_CreateDynamicVb(VERTEX_FORMAT_TEXTURED, 4);

	model = e->Model;
	Vec3_TransformY(&pos, model->GetNameY(e), &e->Transform);

	scale  = e->ModelScale.y;
	scale  = scale > 1.0f ? (1.0f/70.0f) : (scale/70.0f);
	size.x = e->NameTex.Width * scale; size.y = e->NameTex.Height * scale;

	if (Entities.NamesMode == NAME_MODE_ALL_UNSCALED && LocalPlayer_Instance.Hacks.CanSeeAllNames) {			
		Matrix_Mul(&mat, &Gfx.View, &Gfx.Projection); /* TODO: This mul is slow, avoid it */
		/* Get W component of transformed position */
		scale = pos.x * mat.row1.w + pos.y * mat.row2.w + pos.z * mat.row3.w + mat.row4.w;
		size.x *= scale * 0.2f; size.y *= scale * 0.2f;
	}

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);

	vertices = (struct VertexTextured*)Gfx_LockDynamicVb(names_VB, VERTEX_FORMAT_TEXTURED, 4);
	Particle_DoRender(&size, &pos, &e->NameTex.uv, PACKEDCOL_WHITE, vertices);
	Gfx_UnlockDynamicVb(names_VB);

	Gfx_DrawVb_IndexedTris(4);
}

void EntityNames_Delete(struct Entity* e) {
	Gfx_DeleteTexture(&e->NameTex.ID);
	e->NameTex.x = 0; /* X is used as an 'empty name' flag */
}


/*########################################################################################################################*
*-----------------------------------------------------Names rendering-----------------------------------------------------*
*#########################################################################################################################*/
static EntityID closestEntityId;

void EntityNames_Render(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	cc_bool hadFog;
	int i;

	if (Entities.NamesMode == NAME_MODE_NONE) return;
	closestEntityId = Entities_GetClosest(&p->Base);
	if (!p->Hacks.CanSeeAllNames || Entities.NamesMode != NAME_MODE_ALL) return;

	Gfx_SetAlphaTest(true);
	hadFog = Gfx_GetFog();
	if (hadFog) Gfx_SetFog(false);

	for (i = 0; i < ENTITIES_MAX_COUNT; i++) 
	{
		if (!Entities.List[i]) continue;
		if (i != closestEntityId || i == ENTITIES_SELF_ID) {
			DrawName(Entities.List[i]);
		}
	}

	Gfx_SetAlphaTest(false);
	if (hadFog) Gfx_SetFog(true);
}

void EntityNames_RenderHovered(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	cc_bool allNames, hadFog;
	int i;

	if (Entities.NamesMode == NAME_MODE_NONE) return;
	allNames = !(Entities.NamesMode == NAME_MODE_HOVERED || Entities.NamesMode == NAME_MODE_ALL) 
		&& p->Hacks.CanSeeAllNames;

	Gfx_SetAlphaTest(true);
	Gfx_SetDepthTest(false);
	hadFog = Gfx_GetFog();
	if (hadFog) Gfx_SetFog(false);

	for (i = 0; i < ENTITIES_MAX_COUNT; i++) 
	{
		if (!Entities.List[i]) continue;
		if ((i == closestEntityId || allNames) && i != ENTITIES_SELF_ID) {
			DrawName(Entities.List[i]);
		}
	}

	Gfx_SetAlphaTest(false);
	Gfx_SetDepthTest(true);
	if (hadFog) Gfx_SetFog(true);
}

static void DeleteAllNameTextures(void) {
	int i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) 
	{
		if (!Entities.List[i]) continue;
		EntityNames_Delete(Entities.List[i]);
	}
}

static void EntityNames_ChatFontChanged(void* obj) {
	DeleteAllNameTextures();
}


/*########################################################################################################################*
*-----------------------------------------------Entity renderers component------------------------------------------------*
*#########################################################################################################################*/
static void EntityRenderers_ContextLost(void* obj) {
	Gfx_DeleteTexture(&shadows_tex);
	Gfx_DeleteDynamicVb(&shadows_VB);
	
	Gfx_DeleteDynamicVb(&names_VB);
	DeleteAllNameTextures();
}

static void EntityRenderers_Init(void) {
	Event_Register_(&GfxEvents.ContextLost,  NULL, EntityRenderers_ContextLost);
	Event_Register_(&ChatEvents.FontChanged, NULL, EntityNames_ChatFontChanged);
}

static void EntityRenderers_Free(void) {
	EntityRenderers_ContextLost(NULL);
}

struct IGameComponent EntityRenderers_Component = {
	EntityRenderers_Init,  /* Init  */
	EntityRenderers_Free   /* Free  */
};
