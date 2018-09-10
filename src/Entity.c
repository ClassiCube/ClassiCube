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
#include "GraphicsAPI.h"
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

const char* NameMode_Names[NAME_MODE_COUNT]   = { "None", "Hovered", "All", "AllHovered", "AllUnscaled" };
const char* ShadowMode_Names[SHADOW_MODE_COUNT] = { "None", "SnapToBlock", "Circle", "CircleAll" };

/*########################################################################################################################*
*-----------------------------------------------------LocationUpdate------------------------------------------------------*
*#########################################################################################################################*/
Real32 LocationUpdate_Clamp(Real32 degrees) {
	while (degrees >= 360.0f) degrees -= 360.0f;
	while (degrees < 0.0f)    degrees += 360.0f;
	return degrees;
}

struct LocationUpdate loc_empty;
void LocationUpdate_MakeOri(struct LocationUpdate* update, Real32 rotY, Real32 headX) {
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

void LocationUpdate_MakePosAndOri(struct LocationUpdate* update, Vector3 pos, Real32 rotY, Real32 headX, bool rel) {
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
struct EntityVTABLE entity_VTABLE;
static PackedCol Entity_DefaultGetCol(struct Entity* entity) {
	Vector3 eyePos = Entity_GetEyePosition(entity);
	Vector3I P; Vector3I_Floor(&P, &eyePos);
	return World_IsValidPos_3I(P) ? Lighting_Col(P.X, P.Y, P.Z) : Lighting_Outside;
}
static void Entity_NullFunc(struct Entity* entity) {}

void Entity_Init(struct Entity* entity) {
	entity->ModelScale = Vector3_Create1(1.0f);
	entity->uScale = 1.0f;
	entity->vScale = 1.0f;

	entity->VTABLE = &entity_VTABLE;
	entity->VTABLE->ContextLost      = Entity_NullFunc;
	entity->VTABLE->ContextRecreated = Entity_NullFunc;
	entity->VTABLE->GetCol           = Entity_DefaultGetCol;
}

Vector3 Entity_GetEyePosition(struct Entity* entity) {
	Vector3 pos = entity->Position; pos.Y += Entity_GetEyeHeight(entity); return pos;
}

Real32 Entity_GetEyeHeight(struct Entity* entity) {
	return entity->Model->GetEyeY(entity) * entity->ModelScale.Y;
}

void Entity_GetTransform(struct Entity* entity, Vector3 pos, Vector3 scale, struct Matrix* m) {
	*m = Matrix_Identity;
	struct Matrix tmp;

	Matrix_Scale(&tmp, scale.X, scale.Y, scale.Z);
	Matrix_MulBy(m, &tmp);
	Matrix_RotateZ(&tmp, -entity->RotZ * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_RotateX(&tmp, -entity->RotX * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_RotateY(&tmp, -entity->RotY * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_Translate(&tmp, pos.X, pos.Y, pos.Z);
	Matrix_MulBy(m, &tmp);
	/* return rotZ * rotX * rotY * scale * translate; */
}

void Entity_GetPickingBounds(struct Entity* entity, struct AABB* bb) {
	AABB_Offset(bb, &entity->ModelAABB, &entity->Position);
}

void Entity_GetBounds(struct Entity* entity, struct AABB* bb) {
	AABB_Make(bb, &entity->Position, &entity->Size);
}

static void Entity_ParseScale(struct Entity* entity, String scale) {
	if (!scale.length) return;
	Real32 value;
	if (!Convert_TryParseReal32(&scale, &value)) return;

	Real32 maxScale = entity->Model->MaxScale;
	Math_Clamp(value, 0.01f, maxScale);
	entity->ModelScale = Vector3_Create1(value);
}

void Entity_SetModel(struct Entity* entity, STRING_PURE String* model) {
	entity->ModelScale = Vector3_Create1(1.0f);
	entity->ModelBlock = BLOCK_AIR;

	String name, scale;
	if (!String_UNSAFE_Separate(model, '|', &name, &scale)) {
		name  = *model;
		scale = String_MakeNull();	
	}

	/* 'giant' model kept for backwards compatibility */
	if (String_CaselessEqualsConst(&name, "giant")) {
		name = String_FromReadonly("humanoid");
		entity->ModelScale = Vector3_Create1(2.0f);
	} else if (Convert_TryParseUInt8(&name, &entity->ModelBlock)) {
		name = String_FromReadonly("block");
	}

	entity->Model = ModelCache_Get(&name);
	Entity_ParseScale(entity, scale);
	entity->MobTextureId = NULL;

	entity->Model->RecalcProperties(entity);
	Entity_UpdateModelBounds(entity);
	entity->ModelIsSheepNoFur = String_CaselessEqualsConst(&name, "sheep_nofur");
}

void Entity_UpdateModelBounds(struct Entity* entity) {
	struct Model* model = entity->Model;
	model->GetCollisionSize(&entity->Size);
	Vector3_Mul3By(&entity->Size, &entity->ModelScale);

	struct AABB* bb = &entity->ModelAABB;
	model->GetPickingBounds(bb);
	Vector3_Mul3By(&bb->Min, &entity->ModelScale);
	Vector3_Mul3By(&bb->Max, &entity->ModelScale);
}

bool Entity_TouchesAny(struct AABB* bounds, bool (*touches_condition)(BlockID block__)) {
	Vector3I bbMin, bbMax;
	Vector3I_Floor(&bbMin, &bounds->Min);
	Vector3I_Floor(&bbMax, &bounds->Max);

	bbMin.X = max(bbMin.X, 0); bbMax.X = min(bbMax.X, World_MaxX);
	bbMin.Y = max(bbMin.Y, 0); bbMax.Y = min(bbMax.Y, World_MaxY);
	bbMin.Z = max(bbMin.Z, 0); bbMax.Z = min(bbMax.Z, World_MaxZ);

	struct AABB blockBB;
	Vector3 v;
	Int32 x, y, z;
	for (y = bbMin.Y; y <= bbMax.Y; y++) { v.Y = (Real32)y;
		for (z = bbMin.Z; z <= bbMax.Z; z++) { v.Z = (Real32)z;
			for (x = bbMin.X; x <= bbMax.X; x++) { v.X = (Real32)x;
				BlockID block = World_GetBlock(x, y, z);
				Vector3_Add(&blockBB.Min, &v, &Block_MinBB[block]);
				Vector3_Add(&blockBB.Max, &v, &Block_MaxBB[block]);

				if (!AABB_Intersects(&blockBB, bounds)) continue;
				if (touches_condition(block)) return true;
			}
		}
	}
	return false;
}

static bool Entity_IsRope(BlockID b) { return Block_ExtendedCollide[b] == COLLIDE_CLIMB_ROPE; }
bool Entity_TouchesAnyRope(struct Entity* entity) {
	struct AABB bounds; Entity_GetBounds(entity, &bounds);
	bounds.Max.Y += 0.5f / 16.0f;
	return Entity_TouchesAny(&bounds, Entity_IsRope);
}

Vector3 entity_liqExpand = { 0.25f / 16.0f, 0.0f / 16.0f, 0.25f / 16.0f };
static bool Entity_IsLava(BlockID b) { return Block_ExtendedCollide[b] == COLLIDE_LIQUID_LAVA; }
bool Entity_TouchesAnyLava(struct Entity* entity) {
	struct AABB bounds; Entity_GetBounds(entity, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, Entity_IsLava);
}

static bool Entity_IsWater(BlockID b) { return Block_ExtendedCollide[b] == COLLIDE_LIQUID_WATER; }
bool Entity_TouchesAnyWater(struct Entity* entity) {
	struct AABB bounds; Entity_GetBounds(entity, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, Entity_IsWater);
}


/*########################################################################################################################*
*--------------------------------------------------------Entities---------------------------------------------------------*
*#########################################################################################################################*/
EntityID entities_closestId;
void Entities_Tick(struct ScheduledTask* task) {
	Int32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		Entities_List[i]->VTABLE->Tick(Entities_List[i], task->Interval);
	}
}

void Entities_RenderModels(Real64 delta, Real32 t) {
	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);
	Int32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		Entities_List[i]->VTABLE->RenderModel(Entities_List[i], delta, t);
	}
	Gfx_SetTexturing(false);
	Gfx_SetAlphaTest(false);
}
	

void Entities_RenderNames(Real64 delta) {
	if (Entities_NameMode == NAME_MODE_NONE) return;
	struct LocalPlayer* p = &LocalPlayer_Instance;
	entities_closestId = Entities_GetCloset(&p->Base);
	if (!p->Hacks.CanSeeAllNames || Entities_NameMode != NAME_MODE_ALL) return;

	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);
	bool hadFog = Gfx_GetFog();
	if (hadFog) Gfx_SetFog(false);

	Int32 i;
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

void Entities_RenderHoveredNames(Real64 delta) {
	if (Entities_NameMode == NAME_MODE_NONE) return;
	struct LocalPlayer* p = &LocalPlayer_Instance;
	bool allNames = !(Entities_NameMode == NAME_MODE_HOVERED || Entities_NameMode == NAME_MODE_ALL)
		&& p->Hacks.CanSeeAllNames;

	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);
	Gfx_SetDepthTest(false);
	bool hadFog = Gfx_GetFog();
	if (hadFog) Gfx_SetFog(false);

	Int32 i;
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
	Int32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		Entities_List[i]->VTABLE->ContextLost(Entities_List[i]);
	}
	Gfx_DeleteTexture(&ShadowComponent_ShadowTex);
}

static void Entities_ContextRecreated(void* obj) {
	Int32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		Entities_List[i]->VTABLE->ContextRecreated(Entities_List[i]);
	}
}

static void Entities_ChatFontChanged(void* obj) {
	Int32 i;
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
	Int32 i;
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
	Real32 closestDist = MATH_POS_INF;
	EntityID targetId = ENTITIES_SELF_ID;

	Int32 i;
	for (i = 0; i < ENTITIES_SELF_ID; i++) { /* because we don't want to pick against local player */
		struct Entity* entity = Entities_List[i];
		if (!entity) continue;

		Real32 t0, t1;
		if (Intersection_RayIntersectsRotatedBox(eyePos, dir, entity, &t0, &t1) && t0 < closestDist) {
			closestDist = t0;
			targetId = (EntityID)i;
		}
	}
	return targetId;
}

void Entities_DrawShadows(void) {
	if (Entities_ShadowMode == SHADOW_MODE_NONE) return;
	ShadowComponent_BoundShadowTex = false;

	Gfx_SetAlphaArgBlend(true);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaBlending(true);
	Gfx_SetTexturing(true);

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	ShadowComponent_Draw(Entities_List[ENTITIES_SELF_ID]);
	if (Entities_ShadowMode == SHADOW_MODE_CIRCLE_ALL) {
		Int32 i;
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

void TabList_RemoveAt(UInt32 index) {
	UInt32 i;
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

void TabList_Set(EntityID id, STRING_PURE String* player, STRING_PURE String* list, STRING_PURE String* group, UInt8 rank) {
	char playerNameBuffer[STRING_SIZE];
	String playerName = String_FromArray(playerNameBuffer);
	String_AppendColorless(&playerName, player);
	TabList_Remove(id);

	TabList_PlayerNames[id] = TabList_Buffer.Count; StringsBuffer_Add(&TabList_Buffer, &playerName);
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
	struct FontDesc font;
	Font_Make(&font, &Game_FontName, 24, FONT_STYLE_NORMAL);

	String displayName = String_FromRawArray(player->DisplayNameRaw);
	struct DrawTextArgs args;
	DrawTextArgs_Make(&args, &displayName, &font, false);

	/* we want names to always be drawn not using the system font */
	bool bitmapped = Drawer2D_UseBitmappedChat;
	Drawer2D_UseBitmappedChat = true;
	struct Size2D size = Drawer2D_MeasureText(&args);

	if (size.Width == 0) {
		player->NameTex.ID = NULL;
		player->NameTex.X  = PLAYER_NAME_EMPTY_TEX;
	} else {
		char buffer[STRING_SIZE];
		String shadowName = String_FromArray(buffer);

		size.Width += 3; size.Height += 3;
		struct Bitmap bmp; Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
		Drawer2D_Begin(&bmp);
		{
			PackedCol origWhiteCol = Drawer2D_Cols['f'];

			Drawer2D_Cols['f'] = PackedCol_Create3(80, 80, 80);
			String_AppendColorless(&shadowName, &displayName);
			args.Text = shadowName;
			Drawer2D_DrawText(&args, 3, 3);

			Drawer2D_Cols['f'] = origWhiteCol;
			args.Text = displayName;
			Drawer2D_DrawText(&args, 0, 0);
		}
		Drawer2D_End();

		Drawer2D_Make2DTexture(&player->NameTex, &bmp, size, 0, 0);
		Mem_Free(bmp.Scan0);
	}
	Drawer2D_UseBitmappedChat = bitmapped;
}

void Player_UpdateNameTex(struct Player* player) {
	struct Entity* entity = &player->Base;
	entity->VTABLE->ContextLost(entity);

	if (Gfx_LostContext) return;
	Player_MakeNameTexture(player);
}

static void Player_DrawName(struct Player* player) {
	struct Entity* entity = &player->Base;
	struct Model* model = entity->Model;

	if (player->NameTex.X == PLAYER_NAME_EMPTY_TEX) return;
	if (!player->NameTex.ID) Player_MakeNameTexture(player);
	Gfx_BindTexture(player->NameTex.ID);

	Vector3 pos;
	model->RecalcProperties(entity);
	Vector3_TransformY(&pos, model->NameYOffset, &entity->Transform);

	Real32 scale = model->NameScale * entity->ModelScale.Y;
	scale = scale > 1.0f ? (1.0f / 70.0f) : (scale / 70.0f);
	Vector2 size = { player->NameTex.Width * scale, player->NameTex.Height * scale };

	if (Entities_NameMode == NAME_MODE_ALL_UNSCALED && LocalPlayer_Instance.Hacks.CanSeeAllNames) {
		/* Get W component of transformed position */
		struct Matrix mat;
		Matrix_Mul(&mat, &Gfx_View, &Gfx_Projection); /* TODO: This mul is slow, avoid it */
		Real32 tempW = pos.X * mat.Row0.W + pos.Y * mat.Row1.W + pos.Z * mat.Row2.W + mat.Row3.W;
		size.X *= tempW * 0.2f; size.Y *= tempW * 0.2f;
	}

	VertexP3fT2fC4b vertices[4];
	struct TextureRec rec = { 0.0f, 0.0f, player->NameTex.U2, player->NameTex.V2 };
	PackedCol col = PACKEDCOL_WHITE;
	Particle_DoRender(&size, &pos, &rec, col, vertices);

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	GfxCommon_UpdateDynamicVb_IndexedTris(GfxCommon_texVb, vertices, 4);
}

static struct Player* Player_FirstOtherWithSameSkin(struct Player* player) {
	struct Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);
	Int32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i] || Entities_List[i] == entity) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		struct Player* p = (struct Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->SkinNameRaw);
		if (String_Equals(&skin, &pSkin)) return p;
	}
	return NULL;
}

static struct Player* Player_FirstOtherWithSameSkinAndFetchedSkin(struct Player* player) {
	struct Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);
	Int32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i] || Entities_List[i] == entity) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		struct Player* p = (struct Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->SkinNameRaw);
		if (p->FetchedSkin && String_Equals(&skin, &pSkin)) return p;
	}
	return NULL;
}

static void Player_ApplySkin(struct Player* player, struct Player* from) {
	struct Entity* dst = &player->Base;
	struct Entity* src = &from->Base;

	dst->TextureId = src->TextureId;	
	dst->SkinType  = src->SkinType;
	dst->uScale    = src->uScale;
	dst->vScale    = src->vScale;

	/* Custom mob textures */
	dst->MobTextureId = NULL;
	String skin = String_FromRawArray(player->SkinNameRaw);
	if (Utils_IsUrlPrefix(&skin, 0)) {
		dst->MobTextureId = dst->TextureId;
	}
}

void Player_ResetSkin(struct Player* player) {
	struct Entity* entity = &player->Base;
	entity->uScale = 1.0f; entity->vScale = 1.0f;
	entity->MobTextureId = NULL;
	entity->TextureId    = NULL;
	entity->SkinType = SKIN_TYPE_64x32;
}

/* Apply or reset skin, for all players with same skin */
static void Player_SetSkinAll(struct Player* player, bool reset) {
	String skin = String_FromRawArray(player->SkinNameRaw);
	Int32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities_List[i]) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		struct Player* p = (struct Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->SkinNameRaw);
		if (!String_Equals(&skin, &pSkin)) continue;

		if (reset) {
			Player_ResetSkin(p);
		} else {
			Player_ApplySkin(p, player);
		}
	}
}

static void Player_ClearHat(struct Bitmap* bmp, UInt8 skinType) {
	Int32 sizeX = (bmp->Width / 64) * 32;
	Int32 yScale = skinType == SKIN_TYPE_64x32 ? 32 : 64;
	Int32 sizeY = (bmp->Height / yScale) * 16;
	Int32 x, y;

	/* determine if we actually need filtering */
	for (y = 0; y < sizeY; y++) {
		UInt32* row = Bitmap_GetRow(bmp, y);
		row += sizeX;
		for (x = 0; x < sizeX; x++) {
			UInt8 alpha = PackedCol_ARGB_A(row[x]);
			if (alpha != 255) return;
		}
	}

	/* only perform filtering when the entire hat is opaque */
	UInt32 fullWhite = PackedCol_ARGB(255, 255, 255, 255);
	UInt32 fullBlack = PackedCol_ARGB(0,   0,   0,   255);
	for (y = 0; y < sizeY; y++) {
		UInt32* row = Bitmap_GetRow(bmp, y);
		row += sizeX;
		for (x = 0; x < sizeX; x++) {
			UInt32 pixel = row[x];
			if (pixel == fullWhite || pixel == fullBlack) row[x] = 0;
		}
	}
}

static void Player_EnsurePow2(struct Player* player, struct Bitmap* bmp) {
	Int32 width  = Math_NextPowOf2(bmp->Width);
	Int32 height = Math_NextPowOf2(bmp->Height);
	if (width == bmp->Width && height == bmp->Height) return;

	struct Bitmap scaled; Bitmap_Allocate(&scaled, width, height);
	Int32 y;
	UInt32 stride = (UInt32)(bmp->Width) * BITMAP_SIZEOF_PIXEL;
	for (y = 0; y < bmp->Height; y++) {
		UInt32* src = Bitmap_GetRow(bmp, y);
		UInt32* dst = Bitmap_GetRow(&scaled, y);
		Mem_Copy(dst, src, stride);
	}

	struct Entity* entity = &player->Base;
	entity->uScale = (Real32)bmp->Width  / width;
	entity->vScale = (Real32)bmp->Height / height;

	Mem_Free(bmp->Scan0);
	*bmp = scaled;
}

static void Player_CheckSkin(struct Player* player) {
	struct Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);

	if (!player->FetchedSkin && entity->Model->UsesSkin) {
		struct Player* first = Player_FirstOtherWithSameSkinAndFetchedSkin(player);
		if (!first) {
			AsyncDownloader_GetSkin(&skin, &skin);
		} else {
			Player_ApplySkin(player, first);
		}
		player->FetchedSkin = true;
	}

	struct AsyncRequest item;
	if (!AsyncDownloader_Get(&skin, &item)) return;
	if (!item.ResultData) { Player_SetSkinAll(player, true); return; }

	String url = String_FromRawArray(item.URL);
	struct Stream mem; struct Bitmap bmp;
	Stream_ReadonlyMemory(&mem, item.ResultData, item.ResultSize);

	ReturnCode res = Bitmap_DecodePng(&bmp, &mem);
	if (res) {
		Chat_LogError(res, "decoding", &url);
		Mem_Free(bmp.Scan0); return;
	}

	Gfx_DeleteTexture(&entity->TextureId);
	Player_SetSkinAll(player, true);
	Player_EnsurePow2(player, &bmp);
	entity->SkinType = Utils_GetSkinType(&bmp);

	if (entity->SkinType == SKIN_TYPE_INVALID) {
		Player_SetSkinAll(player, true);
	} else {
		if (entity->Model->UsesHumanSkin) {
			Player_ClearHat(&bmp, entity->SkinType);
		}
		entity->TextureId = Gfx_CreateTexture(&bmp, true, false);
		Player_SetSkinAll(player, false);
	}
	Mem_Free(bmp.Scan0);
}

static void Player_Despawn(struct Entity* entity) {
	struct Player* player = (struct Player*)entity;
	struct Player* first = Player_FirstOtherWithSameSkin(player);
	if (!first) {
		Gfx_DeleteTexture(&entity->TextureId);
		Player_ResetSkin(player);
	}
	entity->VTABLE->ContextLost(entity);
}

static void Player_ContextLost(struct Entity* entity) {
	struct Player* player = (struct Player*)entity;
	Gfx_DeleteTexture(&player->NameTex.ID);
	player->NameTex.X = 0; /* X is used as an 'empty name' flag */
}

static void Player_ContextRecreated(struct Entity* entity) {
	struct Player* player = (struct Player*)entity;
	Player_UpdateNameTex(player);
}

void Player_SetName(struct Player* player, STRING_PURE String* name, STRING_PURE String* skin) {
	String p_name = String_ClearedArray(player->DisplayNameRaw);
	String_AppendString(&p_name, name);
	String p_skin = String_ClearedArray(player->SkinNameRaw);
	String_AppendString(&p_skin, skin);
}

struct EntityVTABLE player_VTABLE;
void Player_Init(struct Player* player) {
	struct Entity* entity = &player->Base;
	Entity_Init(entity);
	entity->StepSize = 0.5f;
	entity->EntityType = ENTITY_TYPE_PLAYER;
	String model = String_FromConst("humanoid");
	Entity_SetModel(entity, &model);

	player_VTABLE  = *entity->VTABLE;
	entity->VTABLE = &player_VTABLE;
	entity->VTABLE->ContextLost      = Player_ContextLost;
	entity->VTABLE->ContextRecreated = Player_ContextRecreated;
	entity->VTABLE->Despawn          = Player_Despawn;
}


/*########################################################################################################################*
*------------------------------------------------------LocalPlayer--------------------------------------------------------*
*#########################################################################################################################*/
Real32 LocalPlayer_JumpHeight(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	return (Real32)PhysicsComp_GetMaxHeight(p->Physics.JumpVel);
}

void LocalPlayer_CheckHacksConsistency(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	HacksComp_CheckConsistency(&p->Hacks);
	if (!HacksComp_CanJumpHigher(&p->Hacks)) {
		p->Physics.JumpVel = p->Physics.ServerJumpVel;
	}
}

void LocalPlayer_SetInterpPosition(Real32 t) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (!(p->Hacks.WOMStyleHacks && p->Hacks.Noclip)) {
		Vector3_Lerp(&p->Base.Position, &p->Interp.Prev.Pos, &p->Interp.Next.Pos, t);
	}
	InterpComp_LerpAngles((struct InterpComp*)(&p->Interp), &p->Base, t);
}

static void LocalPlayer_HandleInput(Real32* xMoving, Real32* zMoving) {
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
			if (hacks->Noclip) { Vector3 zero = Vector3_Zero; p->Base.Velocity = zero; }
			hacks->Noclip = KeyBind_IsPressed(KeyBind_NoClip);
		}
	}
}

static void LocalPlayer_SetLocation(struct Entity* entity, struct LocationUpdate* update, bool interpolate) {
	struct LocalPlayer* p = (struct LocalPlayer*)entity;
	LocalInterpComp_SetLocation(&p->Interp, update, interpolate);
}

void LocalPlayer_Tick(struct Entity* entity, Real64 delta) {
	if (!World_Blocks) return;
	struct LocalPlayer* p = (struct LocalPlayer*)entity;
	struct HacksComp* hacks = &p->Hacks;

	entity->StepSize = hacks->FullBlockStep && hacks->Enabled && hacks->CanAnyHacks && hacks->CanSpeed ? 1.0f : 0.5f;
	p->OldVelocity   = entity->Velocity;
	Real32 xMoving = 0, zMoving = 0;
	LocalInterpComp_AdvanceState(&p->Interp);
	bool wasOnGround = entity->OnGround;

	LocalPlayer_HandleInput(&xMoving, &zMoving);
	hacks->Floating = hacks->Noclip || hacks->Flying;
	if (!hacks->Floating && hacks->CanBePushed) PhysicsComp_DoEntityPush(entity);

	/* Immediate stop in noclip mode */
	if (!hacks->NoclipSlide && (hacks->Noclip && xMoving == 0 && zMoving == 0)) {
		Vector3 zero = Vector3_Zero; entity->Velocity = zero;
	}

	PhysicsComp_UpdateVelocityState(&p->Physics);
	Vector3 headingVelocity = Vector3_RotateY3(xMoving, 0, zMoving, entity->HeadY * MATH_DEG2RAD);
	PhysicsComp_PhysicsTick(&p->Physics, headingVelocity);

	/* Fixes high jump, when holding down a movement key, jump, fly, then let go of fly key */
	if (p->Hacks.Floating) entity->Velocity.Y = 0.0f;

	p->Interp.Next.Pos = entity->Position; entity->Position = p->Interp.Prev.Pos;
	AnimatedComp_Update(entity, p->Interp.Prev.Pos, p->Interp.Next.Pos, delta);
	TiltComp_Update(&p->Tilt, delta);

	Player_CheckSkin((struct Player*)p);
	SoundComp_Tick(wasOnGround);
}

static void LocalPlayer_RenderModel(struct Entity* entity, Real64 deltaTime, Real32 t) {
	struct LocalPlayer* p = (struct LocalPlayer*)entity;
	AnimatedComp_GetCurrent(entity, t);
	TiltComp_GetCurrent(&p->Tilt, t);

	if (!Camera_Active->IsThirdPerson) return;
	Model_Render(entity->Model, entity);
}

static void LocalPlayer_RenderName(struct Entity* entity) {
	if (!Camera_Active->IsThirdPerson) return;
	Player_DrawName((struct Player*)entity);
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
	Vector3 zero = Vector3_Zero; p->Base.Velocity = zero;
	p->Physics.JumpVel       = 0.42f;
	p->Physics.ServerJumpVel = 0.42f;
	/* p->Base.Health = 20; TODO: survival mode stuff */
}

static void LocalPlayer_OnNewMap(void) {
	Vector3 zero = Vector3_Zero; 
	LocalPlayer_Instance.Base.Velocity = zero;
	LocalPlayer_Instance.OldVelocity   = zero;
}

void LocalPlayer_MakeComponent(struct IGameComponent* comp) {
	comp->Init     = LocalPlayer_Init_;
	comp->Ready    = LocalPlayer_Reset;
	comp->OnNewMap = LocalPlayer_OnNewMap;
}

struct EntityVTABLE localplayer_VTABLE;
void LocalPlayer_Init(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	Mem_Set(p, 0, sizeof(struct LocalPlayer));
	Player_Init((struct Player*)p);
	Player_SetName((struct Player*)p, &Game_Username, &Game_Username);

	p->Collisions.Entity = &p->Base;
	HacksComp_Init(&p->Hacks);
	PhysicsComp_Init(&p->Physics, &p->Base);
	TiltComp_Init(&p->Tilt);

	p->ReachDistance = 5.0f;
	p->Physics.Hacks = &p->Hacks;
	p->Physics.Collisions = &p->Collisions;

	struct Entity* entity = &p->Base;
	localplayer_VTABLE = *entity->VTABLE;
	entity->VTABLE     = &localplayer_VTABLE;

	entity->VTABLE->SetLocation = LocalPlayer_SetLocation;
	entity->VTABLE->Tick        = LocalPlayer_Tick;
	entity->VTABLE->RenderModel = LocalPlayer_RenderModel;
	entity->VTABLE->RenderName  = LocalPlayer_RenderName;
}

static bool LocalPlayer_IsSolidCollide(BlockID b) { return Block_Collide[b] == COLLIDE_SOLID; }
static void LocalPlayer_DoRespawn(void) {
	if (!World_Blocks) return;
	struct LocalPlayer* p = &LocalPlayer_Instance;
	Vector3 spawn = p->Spawn;
	Vector3I P; Vector3I_Floor(&P, &spawn);
	struct AABB bb;

	/* Spawn player at highest valid position */
	if (World_IsValidPos_3I(P)) {
		AABB_Make(&bb, &spawn, &p->Base.Size);
		Int32 y;
		for (y = P.Y; y <= World_Height; y++) {
			Real32 spawnY = Respawn_HighestFreeY(&bb);
			if (spawnY == RESPAWN_NOT_FOUND) {
				BlockID block = World_GetPhysicsBlock(P.X, y, P.Z);
				Real32 height = Block_Collide[block] == COLLIDE_SOLID ? Block_MaxBB[block].Y : 0.0f;
				spawn.Y = y + height + ENTITY_ADJUSTMENT;
				break;
			}
			bb.Min.Y += 1.0f; bb.Max.Y += 1.0f;
		}
	}

	spawn.Y += 2.0f / 16.0f;
	struct LocationUpdate update; LocationUpdate_MakePosAndOri(&update, spawn, p->SpawnRotY, p->SpawnHeadX, false);
	p->Base.VTABLE->SetLocation(&p->Base, &update, false);
	Vector3 zero = Vector3_Zero; p->Base.Velocity = zero;

	/* Update onGround, otherwise if 'respawn' then 'space' is pressed, you still jump into the air if onGround was true before */
	Entity_GetBounds(&p->Base, &bb);
	bb.Min.Y -= 0.01f; bb.Max.Y = bb.Min.Y;
	p->Base.OnGround = Entity_TouchesAny(&bb, LocalPlayer_IsSolidCollide);
}

bool LocalPlayer_HandlesKey(Int32 key) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct HacksComp* hacks = &p->Hacks;
	struct PhysicsComp* physics = &p->Physics;

	if (key == KeyBind_Get(KeyBind_Respawn) && hacks->CanRespawn) {
		LocalPlayer_DoRespawn();
	} else if (key == KeyBind_Get(KeyBind_SetSpawn) && hacks->CanRespawn) {
		p->Spawn = p->Base.Position;
		p->Spawn.X = Math_Floor(p->Spawn.X) + 0.5f;
		p->Spawn.Z = Math_Floor(p->Spawn.Z) + 0.5f;
		p->SpawnRotY = p->Base.RotY;
		p->SpawnHeadX = p->Base.HeadX;
		LocalPlayer_DoRespawn();
	} else if (key == KeyBind_Get(KeyBind_Fly) && hacks->CanFly && hacks->Enabled) {
		hacks->Flying = !hacks->Flying;
	} else if (key == KeyBind_Get(KeyBind_NoClip) && hacks->CanNoclip && hacks->Enabled && !hacks->WOMStyleHacks) {
		if (hacks->Noclip) p->Base.Velocity.Y = 0;
		hacks->Noclip = !hacks->Noclip;
	} else if (key == KeyBind_Get(KeyBind_Jump) && !p->Base.OnGround && !(hacks->Flying || hacks->Noclip)) {
		Int32 maxJumps = hacks->CanDoubleJump && hacks->WOMStyleHacks ? 2 : 0;
		maxJumps = max(maxJumps, hacks->MaxJumps - 1);

		if (physics->MultiJumps < maxJumps) {
			PhysicsComp_DoNormalJump(physics);
			physics->MultiJumps++;
		}
	} else {
		return false;
	}
	return true;
}


/*########################################################################################################################*
*-------------------------------------------------------NetPlayer---------------------------------------------------------*
*#########################################################################################################################*/
static void NetPlayer_SetLocation(struct Entity* entity, struct LocationUpdate* update, bool interpolate) {
	struct NetPlayer* p = (struct NetPlayer*)entity;
	NetInterpComp_SetLocation(&p->Interp, update, interpolate);
}

void NetPlayer_Tick(struct Entity* entity, Real64 delta) {
	struct NetPlayer* p = (struct NetPlayer*)entity;
	Player_CheckSkin((struct Player*)p);
	NetInterpComp_AdvanceState(&p->Interp);
	AnimatedComp_Update(entity, p->Interp.Prev.Pos, p->Interp.Next.Pos, delta);
}

static void NetPlayer_RenderModel(struct Entity* entity, Real64 deltaTime, Real32 t) {
	struct NetPlayer* p = (struct NetPlayer*)entity;
	Vector3_Lerp(&entity->Position, &p->Interp.Prev.Pos, &p->Interp.Next.Pos, t);
	InterpComp_LerpAngles((struct InterpComp*)(&p->Interp), entity, t);

	AnimatedComp_GetCurrent(entity, t);
	p->ShouldRender = Model_ShouldRender(entity);
	if (p->ShouldRender) Model_Render(entity->Model, entity);
}

static void NetPlayer_RenderName(struct Entity* entity) {
	struct NetPlayer* p = (struct NetPlayer*)entity;
	if (!p->ShouldRender) return;

	Real32 dist = Model_RenderDistance(entity);
	Int32 threshold = Entities_NameMode == NAME_MODE_ALL_UNSCALED ? 8192 * 8192 : 32 * 32;
	if (dist <= (Real32)threshold) Player_DrawName((struct Player*)p);
}

struct EntityVTABLE netplayer_VTABLE;
void NetPlayer_Init(struct NetPlayer* player, STRING_PURE String* displayName, STRING_PURE String* skinName) {
	Mem_Set(player, 0, sizeof(struct NetPlayer));
	Player_Init((struct Player*)player);
	Player_SetName((struct Player*)player, displayName, skinName);

	struct Entity* entity = &player->Base;
	netplayer_VTABLE = *entity->VTABLE;
	entity->VTABLE   = &netplayer_VTABLE;

	entity->VTABLE->SetLocation = NetPlayer_SetLocation;
	entity->VTABLE->Tick        = NetPlayer_Tick;
	entity->VTABLE->RenderModel = NetPlayer_RenderModel;
	entity->VTABLE->RenderName  = NetPlayer_RenderName;
}
