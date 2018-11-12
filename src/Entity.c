#include "Entity.h"
#include "ExtMath.h"
#include "World.h"
#include "Block.h"
#include "Event.h"
#include "Game.h"
#include "Camera.h"
#include "Platform.h"
#include "Funcs.h"
#include "ModelCache.h"
#include "Graphics.h"
#include "Lighting.h"
#include "Drawer2D.h"
#include "Particle.h"
#include "GraphicsCommon.h"
#include "AsyncDownloader.h"
#include "Chat.h"
#include "Model.h"
#include "Input.h"
#include "Gui.h"
#include "Stream.h"
#include "Bitmap.h"

const char* NameMode_Names[NAME_MODE_COUNT]   = { "None", "Hovered", "All", "AllHovered", "AllUnscaled" };
const char* ShadowMode_Names[SHADOW_MODE_COUNT] = { "None", "SnapToBlock", "Circle", "CircleAll" };

/*########################################################################################################################*
*-----------------------------------------------------LocationUpdate------------------------------------------------------*
*#########################################################################################################################*/
float LocationUpdate_Clamp(float degrees) {
	while (degrees >= 360.0f) degrees -= 360.0f;
	while (degrees < 0.0f)    degrees += 360.0f;
	return degrees;
}

static struct LocationUpdate loc_empty;
void LocationUpdate_MakeOri(struct LocationUpdate* update, float rotY, float headX) {
	*update = loc_empty;
	update->Flags = LOCATIONUPDATE_FLAG_HEADX | LOCATIONUPDATE_FLAG_HEADY;
	update->HeadX = LocationUpdate_Clamp(headX);
	update->HeadY = LocationUpdate_Clamp(rotY);
}

void LocationUpdate_MakePos(struct LocationUpdate* update, Vector3 pos, bool rel) {
	*update = loc_empty;
	update->Flags = LOCATIONUPDATE_FLAG_POS;
	update->Pos   = pos;
	update->RelativePos = rel;
}

void LocationUpdate_MakePosAndOri(struct LocationUpdate* update, Vector3 pos, float rotY, float headX, bool rel) {
	*update = loc_empty;
	update->Flags = LOCATIONUPDATE_FLAG_POS | LOCATIONUPDATE_FLAG_HEADX | LOCATIONUPDATE_FLAG_HEADY;
	update->HeadX = LocationUpdate_Clamp(headX);
	update->HeadY = LocationUpdate_Clamp(rotY);
	update->Pos   = pos;
	update->RelativePos = rel;
}


/*########################################################################################################################*
*---------------------------------------------------------Entity----------------------------------------------------------*
*#########################################################################################################################*/
static PackedCol Entity_GetCol(struct Entity* e) {
	Vector3 eyePos = Entity_GetEyePosition(e);
	Vector3I P; Vector3I_Floor(&P, &eyePos);
	return World_IsValidPos_3I(P) ? Lighting_Col(P.X, P.Y, P.Z) : Env_SunCol;
}

void Entity_Init(struct Entity* e) {
	e->ModelScale = Vector3_Create1(1.0f);
	e->uScale = 1.0f;
	e->vScale = 1.0f;
	e->SkinNameRaw[0] = '\0';
}

Vector3 Entity_GetEyePosition(struct Entity* e) {
	Vector3 pos = e->Position; pos.Y += Entity_GetEyeHeight(e); return pos;
}

float Entity_GetEyeHeight(struct Entity* e) {
	return e->Model->GetEyeY(e) * e->ModelScale.Y;
}

void Entity_GetTransform(struct Entity* e, Vector3 pos, Vector3 scale, struct Matrix* m) {
	struct Matrix tmp;
	*m = Matrix_Identity;

	Matrix_Scale(&tmp, scale.X, scale.Y, scale.Z);
	Matrix_MulBy(m, &tmp);
	Matrix_RotateZ(&tmp, -e->RotZ * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_RotateX(&tmp, -e->RotX * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_RotateY(&tmp, -e->RotY * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_Translate(&tmp, pos.X, pos.Y, pos.Z);
	Matrix_MulBy(m, &tmp);
	/* return rotZ * rotX * rotY * scale * translate; */
}

void Entity_GetPickingBounds(struct Entity* e, struct AABB* bb) {
	AABB_Offset(bb, &e->ModelAABB, &e->Position);
}

void Entity_GetBounds(struct Entity* e, struct AABB* bb) {
	AABB_Make(bb, &e->Position, &e->Size);
}

static void Entity_ParseScale(struct Entity* e, const String* scale) {
	float value, maxScale;
	if (!Convert_TryParseFloat(scale, &value)) return;

	maxScale = e->Model->MaxScale;
	Math_Clamp(value, 0.01f, maxScale);
	e->ModelScale = Vector3_Create1(value);
}

static void Entity_SetBlockModel(struct Entity* e, const String* model) {
	static String block = String_FromConst("block");
	int raw = Block_Parse(model);

	if (raw == -1) {
		/* use default humanoid model */
		e->Model = ModelCache_Models[0].Instance;
	} else {	
		e->ModelBlock = (BlockID)raw;
		e->Model      = ModelCache_Get(&block);
	}
}

void Entity_SetModel(struct Entity* e, const String* model) {
	String name, scale, skin;
	e->ModelScale = Vector3_Create1(1.0f);
	String_UNSAFE_Separate(model, '|', &name, &scale);

	/* 'giant' model kept for backwards compatibility */
	if (String_CaselessEqualsConst(&name, "giant")) {
		name = String_FromReadonly("humanoid");
		e->ModelScale = Vector3_Create1(2.0f);
	}
	e->_ModelIsSheepNoFur = String_CaselessEqualsConst(&name, "sheep_nofur");

	e->ModelBlock   = BLOCK_AIR;
	e->Model        = ModelCache_Get(&name);
	e->MobTextureId = GFX_NULL;
	if (!e->Model) Entity_SetBlockModel(e, &name);

	Entity_ParseScale(e, &scale);
	e->Model->RecalcProperties(e);
	Entity_UpdateModelBounds(e);
	
	skin = String_FromRawArray(e->SkinNameRaw);
	if (Utils_IsUrlPrefix(&skin, 0)) { e->MobTextureId = e->TextureId; }
}

void Entity_UpdateModelBounds(struct Entity* e) {
	struct Model* model = e->Model;
	model->GetCollisionSize(&e->Size);
	Vector3_Mul3By(&e->Size, &e->ModelScale);

	struct AABB* bb = &e->ModelAABB;
	model->GetPickingBounds(bb);
	Vector3_Mul3By(&bb->Min, &e->ModelScale);
	Vector3_Mul3By(&bb->Max, &e->ModelScale);
}

bool Entity_TouchesAny(struct AABB* bounds, Entity_TouchesCondition condition) {
	Vector3I bbMin, bbMax;
	BlockID block;
	struct AABB blockBB;
	Vector3 v;
	int x, y, z;

	Vector3I_Floor(&bbMin, &bounds->Min);
	Vector3I_Floor(&bbMax, &bounds->Max);

	bbMin.X = max(bbMin.X, 0); bbMax.X = min(bbMax.X, World_MaxX);
	bbMin.Y = max(bbMin.Y, 0); bbMax.Y = min(bbMax.Y, World_MaxY);
	bbMin.Z = max(bbMin.Z, 0); bbMax.Z = min(bbMax.Z, World_MaxZ);

	for (y = bbMin.Y; y <= bbMax.Y; y++) { v.Y = (float)y;
		for (z = bbMin.Z; z <= bbMax.Z; z++) { v.Z = (float)z;
			for (x = bbMin.X; x <= bbMax.X; x++) { v.X = (float)x;

				block = World_GetBlock(x, y, z);
				Vector3_Add(&blockBB.Min, &v, &Block_MinBB[block]);
				Vector3_Add(&blockBB.Max, &v, &Block_MaxBB[block]);

				if (!AABB_Intersects(&blockBB, bounds)) continue;
				if (condition(block)) return true;
			}
		}
	}
	return false;
}

static bool Entity_IsRope(BlockID b) { return Block_ExtendedCollide[b] == COLLIDE_CLIMB_ROPE; }
bool Entity_TouchesAnyRope(struct Entity* e) {
	struct AABB bounds; Entity_GetBounds(e, &bounds);
	bounds.Max.Y += 0.5f / 16.0f;
	return Entity_TouchesAny(&bounds, Entity_IsRope);
}

static Vector3 entity_liqExpand = { 0.25f/16.0f, 0.0f/16.0f, 0.25f/16.0f };
static bool Entity_IsLava(BlockID b) { return Block_ExtendedCollide[b] == COLLIDE_LIQUID_LAVA; }
bool Entity_TouchesAnyLava(struct Entity* e) {
	struct AABB bounds; Entity_GetBounds(e, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, Entity_IsLava);
}

static bool Entity_IsWater(BlockID b) { return Block_ExtendedCollide[b] == COLLIDE_LIQUID_WATER; }
bool Entity_TouchesAnyWater(struct Entity* e) {
	struct AABB bounds; Entity_GetBounds(e, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, Entity_IsWater);
}


/*########################################################################################################################*
*--------------------------------------------------------Entities---------------------------------------------------------*
*#########################################################################################################################*/
static EntityID entities_closestId;
void Entities_Tick(struct ScheduledTask* task) {
	int i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		Entities_List[i]->VTABLE->Tick(Entities_List[i], task->Interval);
	}
}

void Entities_RenderModels(double delta, float t) {
	int i;
	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);
	
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		Entities_List[i]->VTABLE->RenderModel(Entities_List[i], delta, t);
	}
	Gfx_SetTexturing(false);
	Gfx_SetAlphaTest(false);
}
	

void Entities_RenderNames(double delta) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	bool hadFog;
	int i;

	if (Entities_NameMode == NAME_MODE_NONE) return;
	entities_closestId = Entities_GetCloset(&p->Base);
	if (!p->Hacks.CanSeeAllNames || Entities_NameMode != NAME_MODE_ALL) return;

	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);
	hadFog = Gfx_GetFog();
	if (hadFog) Gfx_SetFog(false);

	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		if (i != entities_closestId || i == ENTITIES_SELF_ID) {
			Entities_List[i]->VTABLE->RenderName(Entities_List[i]);
		}
	}

	Gfx_SetTexturing(false);
	Gfx_SetAlphaTest(false);
	if (hadFog) Gfx_SetFog(true);
}

void Entities_RenderHoveredNames(double delta) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	bool allNames, hadFog;
	int i;

	if (Entities_NameMode == NAME_MODE_NONE) return;
	allNames = !(Entities_NameMode == NAME_MODE_HOVERED || Entities_NameMode == NAME_MODE_ALL) 
		&& p->Hacks.CanSeeAllNames;

	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);
	Gfx_SetDepthTest(false);
	hadFog = Gfx_GetFog();
	if (hadFog) Gfx_SetFog(false);

	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		if ((i == entities_closestId || allNames) && i != ENTITIES_SELF_ID) {
			Entities_List[i]->VTABLE->RenderName(Entities_List[i]);
		}
	}

	Gfx_SetTexturing(false);
	Gfx_SetAlphaTest(false);
	Gfx_SetDepthTest(true);
	if (hadFog) Gfx_SetFog(true);
}

static void Entities_ContextLost(void* obj) {
	int i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		Entities_List[i]->VTABLE->ContextLost(Entities_List[i]);
	}
	Gfx_DeleteTexture(&ShadowComponent_ShadowTex);
}

static void Entities_ContextRecreated(void* obj) {
	int i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		Entities_List[i]->VTABLE->ContextRecreated(Entities_List[i]);
	}
}

static void Entities_ChatFontChanged(void* obj) {
	int i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;
		Player_UpdateNameTex((struct Player*)Entities_List[i]);
	}
}

void Entities_Init(void) {
	Event_RegisterVoid(&GfxEvents_ContextLost,      NULL, Entities_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, NULL, Entities_ContextRecreated);
	Event_RegisterVoid(&ChatEvents_FontChanged,     NULL, Entities_ChatFontChanged);

	Entities_NameMode = Options_GetEnum(OPT_NAMES_MODE, NAME_MODE_HOVERED,
		NameMode_Names, Array_Elems(NameMode_Names));
	if (Game_ClassicMode) Entities_NameMode = NAME_MODE_HOVERED;

	Entities_ShadowMode = Options_GetEnum(OPT_ENTITY_SHADOW, SHADOW_MODE_NONE,
		ShadowMode_Names, Array_Elems(ShadowMode_Names));
	if (Game_ClassicMode) Entities_ShadowMode = SHADOW_MODE_NONE;
}

void Entities_Free(void) {
	int i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		Entities_Remove((EntityID)i);
	}

	Event_UnregisterVoid(&GfxEvents_ContextLost,      NULL, Entities_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, NULL, Entities_ContextRecreated);
	Event_UnregisterVoid(&ChatEvents_FontChanged,     NULL, Entities_ChatFontChanged);

	if (ShadowComponent_ShadowTex) {
		Gfx_DeleteTexture(&ShadowComponent_ShadowTex);
	}
}

void Entities_Remove(EntityID id) {
	Event_RaiseInt(&EntityEvents_Removed, id);
	Entities_List[id]->VTABLE->Despawn(Entities_List[id]);
	Entities_List[id] = NULL;
}

EntityID Entities_GetCloset(struct Entity* src) {
	Vector3 eyePos = Entity_GetEyePosition(src);
	Vector3 dir = Vector3_GetDirVector(src->HeadY * MATH_DEG2RAD, src->HeadX * MATH_DEG2RAD);
	float closestDist = MATH_POS_INF;
	EntityID targetId = ENTITIES_SELF_ID;

	float t0, t1;
	int i;

	for (i = 0; i < ENTITIES_SELF_ID; i++) { /* because we don't want to pick against local player */
		struct Entity* entity = Entities_List[i];
		if (!entity) continue;

		if (Intersection_RayIntersectsRotatedBox(eyePos, dir, entity, &t0, &t1) && t0 < closestDist) {
			closestDist = t0;
			targetId = (EntityID)i;
		}
	}
	return targetId;
}

void Entities_DrawShadows(void) {
	int i;
	if (Entities_ShadowMode == SHADOW_MODE_NONE) return;
	ShadowComponent_BoundShadowTex = false;

	Gfx_SetAlphaArgBlend(true);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaBlending(true);
	Gfx_SetTexturing(true);

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	ShadowComponent_Draw(Entities_List[ENTITIES_SELF_ID]);

	if (Entities_ShadowMode == SHADOW_MODE_CIRCLE_ALL) {	
		for (i = 0; i < ENTITIES_SELF_ID; i++) {
			if (!Entities_List[i]) continue;
			if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;
			ShadowComponent_Draw(Entities_List[i]);
		}
	}

	Gfx_SetAlphaArgBlend(false);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);
	Gfx_SetTexturing(false);
}


/*########################################################################################################################*
*--------------------------------------------------------TabList----------------------------------------------------------*
*#########################################################################################################################*/
bool TabList_Valid(EntityID id) {
	return TabList_PlayerNames[id] || TabList_ListNames[id] || TabList_GroupNames[id];
}

static void TabList_RemoveAt(int index) {
	int i;
	StringsBuffer_Remove(&TabList_Buffer, index);

	for (i = 0; i < TABLIST_MAX_NAMES; i++) {
		if (TabList_PlayerNames[i] == index) { TabList_PlayerNames[i] = 0; }
		if (TabList_PlayerNames[i] > index)  { TabList_PlayerNames[i]--; }

		if (TabList_ListNames[i] == index) { TabList_ListNames[i] = 0; }
		if (TabList_ListNames[i] > index)  { TabList_ListNames[i]--; }

		if (TabList_GroupNames[i] == index) { TabList_GroupNames[i] = 0; }
		if (TabList_GroupNames[i] > index)  { TabList_GroupNames[i]--; }
	}
}

bool TabList_Remove(EntityID id) {
	if (!TabList_Valid(id)) return false;

	TabList_RemoveAt(TabList_PlayerNames[id]);
	TabList_RemoveAt(TabList_ListNames[id]);
	TabList_RemoveAt(TabList_GroupNames[id]);
	TabList_GroupRanks[id] = 0;
	return true;
}

void TabList_Set(EntityID id, const String* player, const String* list, const String* group, uint8_t rank) {
	String colorlessName; char colorlessBuffer[STRING_SIZE];

	String_InitArray(colorlessName, colorlessBuffer);
	String_AppendColorless(&colorlessName, player);
	TabList_Remove(id);

	TabList_PlayerNames[id] = TabList_Buffer.Count; StringsBuffer_Add(&TabList_Buffer, &colorlessName);
	TabList_ListNames[id]   = TabList_Buffer.Count; StringsBuffer_Add(&TabList_Buffer, list);
	TabList_GroupNames[id]  = TabList_Buffer.Count; StringsBuffer_Add(&TabList_Buffer, group);
	TabList_GroupRanks[id]  = rank;
}

static void TabList_Free(void) { StringsBuffer_Clear(&TabList_Buffer); }
static void TabList_Reset(void) {
	Mem_Set(TabList_PlayerNames, 0, sizeof(TabList_PlayerNames));
	Mem_Set(TabList_ListNames,   0, sizeof(TabList_ListNames));
	Mem_Set(TabList_GroupNames,  0, sizeof(TabList_GroupNames));
	Mem_Set(TabList_GroupRanks,  0, sizeof(TabList_GroupRanks));
	StringsBuffer_Clear(&TabList_Buffer);
}

void TabList_MakeComponent(struct IGameComponent* comp) {
	comp->Free  = TabList_Free;
	comp->Reset = TabList_Reset;
}


/*########################################################################################################################*
*---------------------------------------------------------Player----------------------------------------------------------*
*#########################################################################################################################*/
#define PLAYER_NAME_EMPTY_TEX -30000
static void Player_MakeNameTexture(struct Player* player) {
	struct DrawTextArgs args;
	Size2D size;
	Bitmap bmp;

	/* we want names to always be drawn not using the system font */
	bool bitmapped = Drawer2D_BitmappedText;
	Drawer2D_BitmappedText = true;
	String displayName = String_FromRawArray(player->DisplayNameRaw);

	Drawer2D_MakeFont(&args.Font, 24, FONT_STYLE_NORMAL);
	DrawTextArgs_Make(&args, &displayName, &args.Font, false);
	size = Drawer2D_MeasureText(&args);

	if (size.Width == 0) {
		player->NameTex.ID = GFX_NULL;
		player->NameTex.X  = PLAYER_NAME_EMPTY_TEX;
	} else {
		char buffer[STRING_SIZE];
		String shadowName = String_FromArray(buffer);

		size.Width += 3; size.Height += 3;
		Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
		{
			PackedCol origWhiteCol = Drawer2D_Cols['f'];

			Drawer2D_Cols['f'] = PackedCol_Create3(80, 80, 80);
			String_AppendColorless(&shadowName, &displayName);
			args.Text = shadowName;
			Drawer2D_DrawText(&bmp, &args, 3, 3);

			Drawer2D_Cols['f'] = origWhiteCol;
			args.Text = displayName;
			Drawer2D_DrawText(&bmp, &args, 0, 0);
		}
		Drawer2D_Make2DTexture(&player->NameTex, &bmp, size, 0, 0);
		Mem_Free(bmp.Scan0);
	}
	Drawer2D_BitmappedText = bitmapped;
}

void Player_UpdateNameTex(struct Player* player) {
	struct Entity* e = &player->Base;
	e->VTABLE->ContextLost(e);

	if (Gfx_LostContext) return;
	Player_MakeNameTexture(player);
}

static void Player_DrawName(struct Player* player) {
	struct Entity* e = &player->Base;
	struct Model* model = e->Model;
	Vector3 pos;
	float scale;
	VertexP3fT2fC4b vertices[4];

	if (player->NameTex.X == PLAYER_NAME_EMPTY_TEX) return;
	if (!player->NameTex.ID) Player_MakeNameTexture(player);
	Gfx_BindTexture(player->NameTex.ID);

	model->RecalcProperties(e);
	Vector3_TransformY(&pos, model->NameYOffset, &e->Transform);

	scale = model->NameScale * e->ModelScale.Y;
	scale = scale > 1.0f ? (1.0f / 70.0f) : (scale / 70.0f);
	Vector2 size = { player->NameTex.Width * scale, player->NameTex.Height * scale };

	if (Entities_NameMode == NAME_MODE_ALL_UNSCALED && LocalPlayer_Instance.Hacks.CanSeeAllNames) {
		/* Get W component of transformed position */
		struct Matrix mat;
		Matrix_Mul(&mat, &Gfx_View, &Gfx_Projection); /* TODO: This mul is slow, avoid it */
		float tempW = pos.X * mat.Row0.W + pos.Y * mat.Row1.W + pos.Z * mat.Row2.W + mat.Row3.W;
		size.X *= tempW * 0.2f; size.Y *= tempW * 0.2f;
	}

	TextureRec rec = { 0.0f, 0.0f, player->NameTex.U2, player->NameTex.V2 };
	PackedCol col = PACKEDCOL_WHITE;
	Particle_DoRender(&size, &pos, &rec, col, vertices);

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	GfxCommon_UpdateDynamicVb_IndexedTris(GfxCommon_texVb, vertices, 4);
}

static struct Player* Player_FirstOtherWithSameSkin(struct Player* player) {
	struct Entity* entity = &player->Base;
	String skin = String_FromRawArray(entity->SkinNameRaw);
	int i;

	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i] || Entities_List[i] == entity) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		struct Player* p = (struct Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->Base.SkinNameRaw);
		if (String_Equals(&skin, &pSkin)) return p;
	}
	return NULL;
}

static struct Player* Player_FirstOtherWithSameSkinAndFetchedSkin(struct Player* player) {
	struct Entity* entity = &player->Base;
	String skin = String_FromRawArray(entity->SkinNameRaw);
	int i;

	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i] || Entities_List[i] == entity) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		struct Player* p = (struct Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->Base.SkinNameRaw);
		if (p->FetchedSkin && String_Equals(&skin, &pSkin)) return p;
	}
	return NULL;
}

static void Player_ApplySkin(struct Player* player, struct Player* from) {
	struct Entity* dst = &player->Base;
	struct Entity* src = &from->Base;
	String skin;

	dst->TextureId = src->TextureId;	
	dst->SkinType  = src->SkinType;
	dst->uScale    = src->uScale;
	dst->vScale    = src->vScale;

	/* Custom mob textures */
	dst->MobTextureId = GFX_NULL;
	skin = String_FromRawArray(dst->SkinNameRaw);
	if (Utils_IsUrlPrefix(&skin, 0)) { dst->MobTextureId = dst->TextureId; }
}

void Player_ResetSkin(struct Player* player) {
	struct Entity* e = &player->Base;
	e->uScale = 1.0f; e->vScale = 1.0f;
	e->MobTextureId = GFX_NULL;
	e->TextureId    = GFX_NULL;
	e->SkinType     = SKIN_64x32;
}

/* Apply or reset skin, for all players with same skin */
static void Player_SetSkinAll(struct Player* player, bool reset) {
	struct Entity* entity = &player->Base;
	String skin = String_FromRawArray(entity->SkinNameRaw);
	int i;

	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		struct Player* p = (struct Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->Base.SkinNameRaw);
		if (!String_Equals(&skin, &pSkin)) continue;

		if (reset) {
			Player_ResetSkin(p);
		} else {
			Player_ApplySkin(p, player);
		}
	}
}

static void Player_ClearHat(Bitmap* bmp, uint8_t skinType) {
	int sizeX  = (bmp->Width / 64) * 32;
	int yScale = skinType == SKIN_64x32 ? 32 : 64;
	int sizeY  = (bmp->Height / yScale) * 16;
	int x, y;

	/* determine if we actually need filtering */
	for (y = 0; y < sizeY; y++) {
		uint32_t* row = Bitmap_GetRow(bmp, y);
		row += sizeX;
		for (x = 0; x < sizeX; x++) {
			uint8_t alpha = PackedCol_ARGB_A(row[x]);
			if (alpha != 255) return;
		}
	}

	/* only perform filtering when the entire hat is opaque */
	uint32_t fullWhite = PackedCol_ARGB(255, 255, 255, 255);
	uint32_t fullBlack = PackedCol_ARGB(0,   0,   0,   255);
	for (y = 0; y < sizeY; y++) {
		uint32_t* row = Bitmap_GetRow(bmp, y);
		row += sizeX;
		for (x = 0; x < sizeX; x++) {
			uint32_t pixel = row[x];
			if (pixel == fullWhite || pixel == fullBlack) row[x] = 0;
		}
	}
}

static void Player_EnsurePow2(struct Player* p, Bitmap* bmp) {
	uint32_t stride;
	int width, height;
	Bitmap scaled;
	int y;

	width  = Math_NextPowOf2(bmp->Width);
	height = Math_NextPowOf2(bmp->Height);
	if (width == bmp->Width && height == bmp->Height) return;

	Bitmap_Allocate(&scaled, width, height);
	p->Base.uScale = (float)bmp->Width  / width;
	p->Base.vScale = (float)bmp->Height / height;
	stride = bmp->Width * BITMAP_SIZEOF_PIXEL;

	for (y = 0; y < bmp->Height; y++) {
		uint32_t* src = Bitmap_GetRow(bmp, y);
		uint32_t* dst = Bitmap_GetRow(&scaled, y);
		Mem_Copy(dst, src, stride);
	}

	Mem_Free(bmp->Scan0);
	*bmp = scaled;
}

static void Player_CheckSkin(struct Player* p) {
	struct Entity* e = &p->Base;
	struct Player* first;
	String url, skin = String_FromRawArray(e->SkinNameRaw);

	struct AsyncRequest item;
	struct Stream mem;
	Bitmap bmp;
	ReturnCode res;

	if (!p->FetchedSkin && e->Model->UsesSkin) {
		first = Player_FirstOtherWithSameSkinAndFetchedSkin(p);
		if (!first) {
			AsyncDownloader_GetSkin(&skin, &skin);
		} else {
			Player_ApplySkin(p, first);
		}
		p->FetchedSkin = true;
	}
	
	if (!AsyncDownloader_Get(&skin, &item)) return;
	if (!item.ResultData) { Player_SetSkinAll(p, true); return; }
	Stream_ReadonlyMemory(&mem, item.ResultData, item.ResultSize);

	if ((res = Png_Decode(&bmp, &mem))) {
		url = String_FromRawArray(item.URL);
		Chat_LogError2(res, "decoding", &url);
		Mem_Free(bmp.Scan0); return;
	}

	Gfx_DeleteTexture(&e->TextureId);
	Player_SetSkinAll(p, true);
	Player_EnsurePow2(p, &bmp);
	e->SkinType = Utils_GetSkinType(&bmp);

	if (e->SkinType == SKIN_INVALID) {
		Player_SetSkinAll(p, true);
	} else {
		if (e->Model->UsesHumanSkin) {
			Player_ClearHat(&bmp, e->SkinType);
		}
		e->TextureId = Gfx_CreateTexture(&bmp, true, false);
		Player_SetSkinAll(p, false);
	}
	Mem_Free(bmp.Scan0);
}

static void Player_Despawn(struct Entity* e) {
	struct Player* player = (struct Player*)e;
	struct Player* first  = Player_FirstOtherWithSameSkin(player);

	if (!first) {
		Gfx_DeleteTexture(&e->TextureId);
		Player_ResetSkin(player);
	}
	e->VTABLE->ContextLost(e);
}

static void Player_ContextLost(struct Entity* e) {
	struct Player* player = (struct Player*)e;
	Gfx_DeleteTexture(&player->NameTex.ID);
	player->NameTex.X = 0; /* X is used as an 'empty name' flag */
}

static void Player_ContextRecreated(struct Entity* e) {
	struct Player* player = (struct Player*)e;
	Player_UpdateNameTex(player);
}

void Player_SetName(struct Player* p, const String* name, const String* skin) {
	String p_name = String_ClearedArray(p->DisplayNameRaw);
	String p_skin = String_ClearedArray(p->Base.SkinNameRaw);

	String_AppendString(&p_name, name);
	String_AppendString(&p_skin, skin);
}

static void Player_Init(struct Entity* e) {
	static String model = String_FromConst("humanoid");
	Entity_Init(e);

	e->StepSize   = 0.5f;
	e->EntityType = ENTITY_TYPE_PLAYER;
	Entity_SetModel(e, &model);
}


/*########################################################################################################################*
*------------------------------------------------------LocalPlayer--------------------------------------------------------*
*#########################################################################################################################*/
float LocalPlayer_JumpHeight(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	return (float)PhysicsComp_GetMaxHeight(p->Physics.JumpVel);
}

void LocalPlayer_CheckHacksConsistency(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	HacksComp_CheckConsistency(&p->Hacks);
	if (!HacksComp_CanJumpHigher(&p->Hacks)) {
		p->Physics.JumpVel = p->Physics.ServerJumpVel;
	}
}

void LocalPlayer_SetInterpPosition(float t) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (!(p->Hacks.WOMStyleHacks && p->Hacks.Noclip)) {
		Vector3_Lerp(&p->Base.Position, &p->Interp.Prev.Pos, &p->Interp.Next.Pos, t);
	}
	InterpComp_LerpAngles((struct InterpComp*)(&p->Interp), &p->Base, t);
}

static void LocalPlayer_HandleInput(float* xMoving, float* zMoving) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct HacksComp* hacks = &p->Hacks;

	if (Gui_GetActiveScreen()->HandlesAllInput) {
		p->Physics.Jumping = false; hacks->Speeding = false;
		hacks->FlyingUp    = false; hacks->FlyingDown = false;
	} else {
		if (KeyBind_IsPressed(KeyBind_Forward)) *zMoving -= 0.98f;
		if (KeyBind_IsPressed(KeyBind_Back))    *zMoving += 0.98f;
		if (KeyBind_IsPressed(KeyBind_Left))    *xMoving -= 0.98f;
		if (KeyBind_IsPressed(KeyBind_Right))   *xMoving += 0.98f;

		p->Physics.Jumping  = KeyBind_IsPressed(KeyBind_Jump);
		hacks->Speeding     = hacks->Enabled && KeyBind_IsPressed(KeyBind_Speed);
		hacks->HalfSpeeding = hacks->Enabled && KeyBind_IsPressed(KeyBind_HalfSpeed);
		hacks->FlyingUp     = KeyBind_IsPressed(KeyBind_FlyUp);
		hacks->FlyingDown   = KeyBind_IsPressed(KeyBind_FlyDown);

		if (hacks->WOMStyleHacks && hacks->Enabled && hacks->CanNoclip) {
			if (hacks->Noclip) p->Base.Velocity = Vector3_Zero;
			hacks->Noclip = KeyBind_IsPressed(KeyBind_NoClip);
		}
	}
}

static void LocalPlayer_SetLocation(struct Entity* e, struct LocationUpdate* update, bool interpolate) {
	struct LocalPlayer* p = (struct LocalPlayer*)e;
	LocalInterpComp_SetLocation(&p->Interp, update, interpolate);
}

void LocalPlayer_Tick(struct Entity* e, double delta) {
	struct LocalPlayer* p = (struct LocalPlayer*)e;
	struct HacksComp* hacks = &p->Hacks;
	float xMoving = 0, zMoving = 0;
	bool wasOnGround;
	Vector3 headingVelocity;

	if (!World_Blocks) return;
	e->StepSize = hacks->FullBlockStep && hacks->Enabled && hacks->CanAnyHacks && hacks->CanSpeed ? 1.0f : 0.5f;
	p->OldVelocity = e->Velocity;
	wasOnGround    = e->OnGround;

	LocalInterpComp_AdvanceState(&p->Interp);
	LocalPlayer_HandleInput(&xMoving, &zMoving);
	hacks->Floating = hacks->Noclip || hacks->Flying;
	if (!hacks->Floating && hacks->CanBePushed) PhysicsComp_DoEntityPush(e);

	/* Immediate stop in noclip mode */
	if (!hacks->NoclipSlide && (hacks->Noclip && xMoving == 0 && zMoving == 0)) {
		e->Velocity = Vector3_Zero;
	}

	PhysicsComp_UpdateVelocityState(&p->Physics);
	headingVelocity = Vector3_RotateY3(xMoving, 0, zMoving, e->HeadY * MATH_DEG2RAD);
	PhysicsComp_PhysicsTick(&p->Physics, headingVelocity);

	/* Fixes high jump, when holding down a movement key, jump, fly, then let go of fly key */
	if (p->Hacks.Floating) e->Velocity.Y = 0.0f;

	p->Interp.Next.Pos = e->Position; e->Position = p->Interp.Prev.Pos;
	AnimatedComp_Update(e, p->Interp.Prev.Pos, p->Interp.Next.Pos, delta);
	TiltComp_Update(&p->Tilt, delta);

	Player_CheckSkin((struct Player*)p);
	SoundComp_Tick(wasOnGround);
}

static void LocalPlayer_RenderModel(struct Entity* e, double deltaTime, float t) {
	struct LocalPlayer* p = (struct LocalPlayer*)e;
	AnimatedComp_GetCurrent(e, t);
	TiltComp_GetCurrent(&p->Tilt, t);

	if (!Camera_Active->IsThirdPerson) return;
	Model_Render(e->Model, e);
}

static void LocalPlayer_RenderName(struct Entity* e) {
	if (!Camera_Active->IsThirdPerson) return;
	Player_DrawName((struct Player*)e);
}

static void LocalPlayer_Init_(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct HacksComp* hacks = &p->Hacks;

	hacks->Enabled = !Game_PureClassic && Options_GetBool(OPT_HACKS_ENABLED, true);
	/* p->Base.Health = 20; TODO: survival mode stuff */
	if (Game_ClassicMode) return;

	hacks->SpeedMultiplier = Options_GetFloat(OPT_SPEED_FACTOR, 0.1f, 50.0f, 10.0f);
	hacks->PushbackPlacing = Options_GetBool(OPT_PUSHBACK_PLACING, false);
	hacks->NoclipSlide     = Options_GetBool(OPT_NOCLIP_SLIDE, false);
	hacks->WOMStyleHacks   = Options_GetBool(OPT_WOM_STYLE_HACKS, false);
	hacks->FullBlockStep   = Options_GetBool(OPT_FULL_BLOCK_STEP, false);
	p->Physics.UserJumpVel = Options_GetFloat(OPT_JUMP_VELOCITY, 0.0f, 52.0f, 0.42f);
	p->Physics.JumpVel     = p->Physics.UserJumpVel;
}

static void LocalPlayer_Reset(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	p->ReachDistance = 5.0f;
	p->Base.Velocity = Vector3_Zero;
	p->Physics.JumpVel       = 0.42f;
	p->Physics.ServerJumpVel = 0.42f;
	/* p->Base.Health = 20; TODO: survival mode stuff */
}

static void LocalPlayer_OnNewMap(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	p->Base.Velocity = Vector3_Zero;
	p->OldVelocity   = Vector3_Zero;

	p->_WarnedRespawn = false;
	p->_WarnedFly     = false;
	p->_WarnedNoclip  = false;
}

void LocalPlayer_MakeComponent(struct IGameComponent* comp) {
	comp->Init     = LocalPlayer_Init_;
	comp->Ready    = LocalPlayer_Reset;
	comp->OnNewMap = LocalPlayer_OnNewMap;
}

struct EntityVTABLE localPlayer_VTABLE = {
	LocalPlayer_Tick,        Player_Despawn,         LocalPlayer_SetLocation, Entity_GetCol,
	LocalPlayer_RenderModel, LocalPlayer_RenderName, Player_ContextLost,      Player_ContextRecreated,
};
void LocalPlayer_Init(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	Player_Init(&p->Base);
	Player_SetName((struct Player*)p, &Game_Username, &Game_Username);

	p->Collisions.Entity = &p->Base;
	HacksComp_Init(&p->Hacks);
	PhysicsComp_Init(&p->Physics, &p->Base);
	TiltComp_Init(&p->Tilt);

	p->ReachDistance = 5.0f;
	p->Physics.Hacks = &p->Hacks;
	p->Physics.Collisions = &p->Collisions;
	p->Base.VTABLE   = &localPlayer_VTABLE;
}

static bool LocalPlayer_IsSolidCollide(BlockID b) { return Block_Collide[b] == COLLIDE_SOLID; }
static void LocalPlayer_DoRespawn(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct LocationUpdate update;
	struct AABB bb;
	Vector3 spawn = p->Spawn;
	Vector3I pos;
	BlockID block;
	float height, spawnY;
	int y;

	if (!World_Blocks) return;
	Vector3I_Floor(&pos, &spawn);	

	/* Spawn player at highest valid position */
	if (World_IsValidPos_3I(pos)) {
		AABB_Make(&bb, &spawn, &p->Base.Size);
		for (y = pos.Y; y <= World_Height; y++) {
			spawnY = Respawn_HighestFreeY(&bb);

			if (spawnY == RESPAWN_NOT_FOUND) {
				block   = World_GetPhysicsBlock(pos.X, y, pos.Z);
				height  = Block_Collide[block] == COLLIDE_SOLID ? Block_MaxBB[block].Y : 0.0f;
				spawn.Y = y + height + ENTITY_ADJUSTMENT;
				break;
			}
			bb.Min.Y += 1.0f; bb.Max.Y += 1.0f;
		}
	}

	spawn.Y += 2.0f/16.0f;
	LocationUpdate_MakePosAndOri(&update, spawn, p->SpawnRotY, p->SpawnHeadX, false);
	p->Base.VTABLE->SetLocation(&p->Base, &update, false);
	p->Base.Velocity = Vector3_Zero;

	/* Update onGround, otherwise if 'respawn' then 'space' is pressed, you still jump into the air if onGround was true before */
	Entity_GetBounds(&p->Base, &bb);
	bb.Min.Y -= 0.01f; bb.Max.Y = bb.Min.Y;
	p->Base.OnGround = Entity_TouchesAny(&bb, LocalPlayer_IsSolidCollide);
}

static bool LocalPlayer_HandleRespawn(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (p->Hacks.CanRespawn) {
		LocalPlayer_DoRespawn();
		return true;
	} else if (!p->_WarnedRespawn) {
		p->_WarnedRespawn = true;
		if (!Game_ClassicMode) Chat_AddRaw("&cRespawning is currently disabled");
	}
	return false;
}

static bool LocalPlayer_HandleSetSpawn(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (p->Hacks.CanRespawn) {
		p->Spawn.X = Math_Floor(p->Base.Position.X) + 0.5f;
		p->Spawn.Y = p->Base.Position.Y;
		p->Spawn.Z = Math_Floor(p->Base.Position.Z) + 0.5f;
		p->SpawnRotY  = p->Base.RotY;
		p->SpawnHeadX = p->Base.HeadX;
	}
	return LocalPlayer_HandleRespawn();
}

static bool LocalPlayer_HandleFly(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (p->Hacks.CanFly && p->Hacks.Enabled) {
		p->Hacks.Flying = !p->Hacks.Flying;
		return true;
	} else if (!p->_WarnedFly) {
		p->_WarnedFly = true;
		if (!Game_ClassicMode) Chat_AddRaw("&cFlying is currently disabled");		
	}
	return false;
}

static bool LocalPlayer_HandleNoClip(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (p->Hacks.CanNoclip && p->Hacks.Enabled) {
		if (p->Hacks.WOMStyleHacks) return true; /* don't handle this here */
		if (p->Hacks.Noclip) p->Base.Velocity.Y = 0;

		p->Hacks.Noclip = !p->Hacks.Noclip;
		return true;
	} else if (!p->_WarnedNoclip) {
		p->_WarnedNoclip = true;
		if (!Game_ClassicMode) Chat_AddRaw("&cNoclip is currently disabled");		
	}
	return false;
}

bool LocalPlayer_HandlesKey(Key key) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct HacksComp* hacks = &p->Hacks;
	struct PhysicsComp* physics = &p->Physics;
	int maxJumps;

	if (key == KeyBind_Get(KeyBind_Respawn)) {
		return LocalPlayer_HandleRespawn();
	} else if (key == KeyBind_Get(KeyBind_SetSpawn)) {
		return LocalPlayer_HandleSetSpawn();
	} else if (key == KeyBind_Get(KeyBind_Fly)) {
		return LocalPlayer_HandleFly();
	} else if (key == KeyBind_Get(KeyBind_NoClip)) {
		return LocalPlayer_HandleNoClip();
	} else if (key == KeyBind_Get(KeyBind_Jump) && !p->Base.OnGround && !(hacks->Flying || hacks->Noclip)) {
		maxJumps = hacks->CanDoubleJump && hacks->WOMStyleHacks ? 2 : 0;
		maxJumps = max(maxJumps, hacks->MaxJumps - 1);

		if (physics->MultiJumps < maxJumps) {
			PhysicsComp_DoNormalJump(physics);
			physics->MultiJumps++;
		}
		return true;
	}
	return false;
}


/*########################################################################################################################*
*-------------------------------------------------------NetPlayer---------------------------------------------------------*
*#########################################################################################################################*/
static void NetPlayer_SetLocation(struct Entity* e, struct LocationUpdate* update, bool interpolate) {
	struct NetPlayer* p = (struct NetPlayer*)e;
	NetInterpComp_SetLocation(&p->Interp, update, interpolate);
}

static void NetPlayer_Tick(struct Entity* e, double delta) {
	struct NetPlayer* p = (struct NetPlayer*)e;
	Player_CheckSkin((struct Player*)p);
	NetInterpComp_AdvanceState(&p->Interp);
	AnimatedComp_Update(e, p->Interp.Prev.Pos, p->Interp.Next.Pos, delta);
}

static void NetPlayer_RenderModel(struct Entity* e, double deltaTime, float t) {
	struct NetPlayer* p = (struct NetPlayer*)e;
	Vector3_Lerp(&e->Position, &p->Interp.Prev.Pos, &p->Interp.Next.Pos, t);
	InterpComp_LerpAngles((struct InterpComp*)(&p->Interp), e, t);

	AnimatedComp_GetCurrent(e, t);
	p->ShouldRender = Model_ShouldRender(e);
	if (p->ShouldRender) Model_Render(e->Model, e);
}

static void NetPlayer_RenderName(struct Entity* e) {
	struct NetPlayer* p = (struct NetPlayer*)e;
	float distance;
	int threshold;
	if (!p->ShouldRender) return;

	distance  = Model_RenderDistance(e);
	threshold = Entities_NameMode == NAME_MODE_ALL_UNSCALED ? 8192 * 8192 : 32 * 32;
	if (distance <= (float)threshold) Player_DrawName((struct Player*)p);
}

struct EntityVTABLE netPlayer_VTABLE = {
	NetPlayer_Tick,        Player_Despawn,       NetPlayer_SetLocation, Entity_GetCol,
	NetPlayer_RenderModel, NetPlayer_RenderName, Player_ContextLost,    Player_ContextRecreated,
};
void NetPlayer_Init(struct NetPlayer* p, const String* displayName, const String* skinName) {
	Mem_Set(p, 0, sizeof(struct NetPlayer));
	Player_Init(&p->Base);
	Player_SetName((struct Player*)p, displayName, skinName);
	p->Base.VTABLE = &netPlayer_VTABLE;
}
