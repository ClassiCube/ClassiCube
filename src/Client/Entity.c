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
#include "ErrorHandler.h"
#include "IModel.h"

const UInt8* NameMode_Names[5]   = { "None", "Hovered", "All", "AllHovered", "AllUnscaled" };
const UInt8* ShadowMode_Names[4] = { "None", "SnapToBlock", "Circle", "CircleAll" };

Real32 LocationUpdate_Clamp(Real32 degrees) {
	degrees = Math_Mod(degrees, 360.0f);
	if (degrees < 0) degrees += 360.0f;
	return degrees;
}

void LocationUpdate_Construct(LocationUpdate* update, Real32 x, Real32 y, Real32 z,
	Real32 rotX, Real32 rotY, Real32 rotZ, Real32 headX, bool incPos, bool relPos) {
	update->Pos = Vector3_Create3(x, y, z);
	update->RotX = LocationUpdate_Clamp(rotX);
	update->RotY = LocationUpdate_Clamp(rotY);
	update->RotZ = LocationUpdate_Clamp(rotZ);
	update->HeadX = LocationUpdate_Clamp(headX);
	update->IncludesPosition = incPos;
	update->RelativePosition = relPos;
}

#define exc MATH_POS_INF
void LocationUpdate_Empty(LocationUpdate* update) {
	LocationUpdate_Construct(update, 0.0f, 0.0f, 0.0f, exc, exc, exc, exc, false, false);
}

void LocationUpdate_MakeOri(LocationUpdate* update, Real32 rotY, Real32 headX) {
	LocationUpdate_Construct(update, 0.0f, 0.0f, 0.0f, exc, rotY, exc, headX, false, false);
}

void LocationUpdate_MakePos(LocationUpdate* update, Vector3 pos, bool rel) {
	LocationUpdate_Construct(update, pos.X, pos.Y, pos.Z, exc, exc, exc, exc, true, rel);
}

void LocationUpdate_MakePosAndOri(LocationUpdate* update, Vector3 pos, Real32 rotY, Real32 headX, bool rel) {
	LocationUpdate_Construct(update, pos.X, pos.Y, pos.Z, exc, rotY, exc, headX, true, rel);
}


EntityVTABLE entityDefaultVTABLE;
PackedCol Entity_DefaultGetCol(Entity* entity) {
	Vector3 eyePos = Entity_GetEyePosition(entity);
	Vector3I P; Vector3I_Floor(&P, &eyePos);
	return World_IsValidPos_3I(P) ? Lighting_Col(P.X, P.Y, P.Z) : Lighting_Outside;
}
void Entity_NullFunc(Entity* entity) {}

void Entity_Init(Entity* entity) {
	Platform_MemSet(entity, 0, sizeof(Entity));
	entity->ModelScale = Vector3_Create1(1.0f);
	entity->uScale = 1.0f;
	entity->vScale = 1.0f;

	entityDefaultVTABLE.ContextLost      = Entity_NullFunc;
	entityDefaultVTABLE.ContextRecreated = Entity_NullFunc;
	entityDefaultVTABLE.GetCol           = Entity_DefaultGetCol;
	entity->VTABLE = &entityDefaultVTABLE;
}

Vector3 Entity_GetEyePosition(Entity* entity) {
	Vector3 pos = entity->Position;
	pos.Y += entity->Model->GetEyeY(entity) * entity->ModelScale.Y;
	return pos;
}

void Entity_GetTransform(Entity* entity, Vector3 pos, Vector3 scale) {
	Matrix* m = &entity->Transform;
	*m = Matrix_Identity;
	Matrix tmp;

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

void Entity_GetPickingBounds(Entity* entity, AABB* bb) {
	AABB_Offset(bb, &entity->ModelAABB, &entity->Position);
}

void Entity_GetBounds(Entity* entity, AABB* bb) {
	AABB_Make(bb, &entity->Position, &entity->Size);
}

void Entity_ParseScale(Entity* entity, String scale) {
	if (scale.buffer == NULL) return;
	Real32 value;
	if (!Convert_TryParseReal32(&scale, &value)) return;

	Real32 maxScale = entity->Model->MaxScale;
	Math_Clamp(value, 0.01f, maxScale);
	entity->ModelScale = Vector3_Create1(value);
}

void Entity_SetModel(Entity* entity, STRING_PURE String* model) {
	entity->ModelScale = Vector3_Create1(1.0f);
	entity->ModelBlock = BLOCK_AIR;
	String entModel = String_FromRawArray(entity->ModelNameRaw);
	String_Clear(&entModel);

	Int32 sep = String_IndexOf(model, '|', 0);
	String name, scale;
	if (sep == -1) {
		name  = *model;
		scale = String_MakeNull();	
	} else {
		name  = String_UNSAFE_SubstringAt(model, sep + 1);
		scale = String_UNSAFE_Substring(model, 0, sep);
	}

	/* 'giant' model kept for backwards compatibility */
	String giant = String_FromConst("giant");
	if (String_CaselessEquals(model, &giant)) {
		String_AppendConst(&entModel, "humanoid");
		entity->ModelScale = Vector3_Create1(2.0f);
	} else if (Convert_TryParseUInt8(model, &entity->ModelBlock)) {
		String_AppendConst(&entModel, "block");
	} else {
		String_AppendString(&entModel, &name);
	}

	entity->Model = ModelCache_Get(&entModel);
	Entity_ParseScale(entity, scale);
	entity->MobTextureId = NULL;

	entity->Model->RecalcProperties(entity);
	Entity_UpdateModelBounds(entity);
}

void Entity_UpdateModelBounds(Entity* entity) {
	IModel* model = entity->Model;
	Vector3 baseSize = model->GetCollisionSize();
	Vector3_Multiply3(&entity->Size, &baseSize, &entity->ModelScale);

	AABB* bb = &entity->ModelAABB;
	model->GetPickingBounds(bb);
	Vector3_Multiply3(&bb->Min, &bb->Min, &entity->ModelScale);
	Vector3_Multiply3(&bb->Max, &bb->Max, &entity->ModelScale);
}

bool Entity_TouchesAny(AABB* bounds, bool (*touches_condition)(BlockID block__)) {
	Vector3I bbMin, bbMax;
	Vector3I_Floor(&bbMin, &bounds->Min);
	Vector3I_Floor(&bbMax, &bounds->Max);
	AABB blockBB;
	Vector3 v;

	bbMin.X = max(bbMin.X, 0); bbMax.X = min(bbMax.X, World_MaxX);
	bbMin.Y = max(bbMin.Y, 0); bbMax.Y = min(bbMax.Y, World_MaxY);
	bbMin.Z = max(bbMin.Z, 0); bbMax.Z = min(bbMax.Z, World_MaxZ);

	Int32 x, y, z;
	for (y = bbMin.Y; y <= bbMax.Y; y++) {
		v.Y = (Real32)y;
		for (z = bbMin.Z; z <= bbMax.Z; z++) {
			v.Z = (Real32)z;
			for (x = bbMin.X; x <= bbMax.X; x++) {
				v.X = (Real32)x;

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

bool Entity_IsRope(BlockID b) { return Block_ExtendedCollide[b] == COLLIDE_CLIMB_ROPE; }
bool Entity_TouchesAnyRope(Entity* entity) {
	AABB bounds; Entity_GetBounds(entity, &bounds);
	bounds.Max.Y += 0.5f / 16.0f;
	return Entity_TouchesAny(&bounds, Entity_IsRope);
}

Vector3 entity_liqExpand = { 0.25f / 16.0f, 0.0f / 16.0f, 0.25f / 16.0f };

bool Entity_IsLava(BlockID b) { return Block_ExtendedCollide[b] == COLLIDE_LIQUID_LAVA; }
bool Entity_TouchesAnyLava(Entity* entity) {
	AABB bounds; Entity_GetBounds(entity, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, Entity_IsLava);
}

bool Entity_IsWater(BlockID b) { return Block_ExtendedCollide[b] == COLLIDE_LIQUID_WATER; }
bool Entity_TouchesAnyWater(Entity* entity) {
	AABB bounds; Entity_GetBounds(entity, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, Entity_IsWater);
}


EntityID closestId;
void Entities_Tick(ScheduledTask* task) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		Entities_List[i]->VTABLE->Tick(Entities_List[i], task);
	}
}

void Entities_RenderModels(Real64 delta, Real32 t) {
	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		Entities_List[i]->VTABLE->RenderModel(Entities_List[i], delta, t);
	}
	Gfx_SetTexturing(false);
	Gfx_SetAlphaTest(false);
}
	

void Entities_RenderNames(Real64 delta) {
	if (Entities_NameMode == NAME_MODE_NONE) return;
	LocalPlayer* p = &LocalPlayer_Instance;
	closestId = Entities_GetCloset(&p->Base);
	if (!p->Hacks.CanSeeAllNames || Entities_NameMode != NAME_MODE_ALL) return;

	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);
	bool hadFog = Gfx_GetFog();
	if (hadFog) Gfx_SetFog(false);

	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		if (i != closestId || i == ENTITIES_SELF_ID) {
			Entities_List[i]->VTABLE->RenderName(Entities_List[i]);
		}
	}

	Gfx_SetTexturing(false);
	Gfx_SetAlphaTest(false);
	if (hadFog) Gfx_SetFog(true);
}

void Entities_RenderHoveredNames(Real64 delta) {
	if (Entities_NameMode == NAME_MODE_NONE) return;
	LocalPlayer* p = &LocalPlayer_Instance;
	bool allNames = !(Entities_NameMode == NAME_MODE_HOVERED || Entities_NameMode == NAME_MODE_ALL)
		&& p->Hacks.CanSeeAllNames;

	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);
	Gfx_SetDepthTest(false);
	bool hadFog = Gfx_GetFog();
	if (hadFog) Gfx_SetFog(false);

	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		if ((i == closestId || allNames) && i != ENTITIES_SELF_ID) {
			Entities_List[i]->VTABLE->RenderName(Entities_List[i]);
		}
	}

	Gfx_SetTexturing(false);
	Gfx_SetAlphaTest(false);
	Gfx_SetDepthTest(true);
	if (hadFog) Gfx_SetFog(true);
}

void Entities_ContextLost(void* obj) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		Entities_List[i]->VTABLE->ContextLost(Entities_List[i]);
	}
	Gfx_DeleteTexture(&ShadowComponent_ShadowTex);
}

void Entities_ContextRecreated(void* obj) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		Entities_List[i]->VTABLE->ContextRecreated(Entities_List[i]);
	}
}

void Entities_ChatFontChanged(void* obj) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;
		Player_UpdateName((Player*)Entities_List[i]);
	}
}

void Entities_Init(void) {
	Event_RegisterVoid(&GfxEvents_ContextLost,      NULL, Entities_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, NULL, Entities_ContextRecreated);
	Event_RegisterVoid(&ChatEvents_FontChanged,     NULL, Entities_ChatFontChanged);

	Entities_NameMode = Options_GetEnum(OPTION_NAMES_MODE, NAME_MODE_HOVERED,
		NameMode_Names, Array_Elems(NameMode_Names));
	if (Game_ClassicMode) Entities_NameMode = NAME_MODE_HOVERED;

	Entities_ShadowMode = Options_GetEnum(OPTION_ENTITY_SHADOW, SHADOW_MODE_NONE,
		ShadowMode_Names, Array_Elems(ShadowMode_Names));
	if (Game_ClassicMode) Entities_ShadowMode = SHADOW_MODE_NONE;
}

void Entities_Free(void) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		Entities_Remove((EntityID)i);
	}

	Event_UnregisterVoid(&GfxEvents_ContextLost,      NULL, Entities_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, NULL, Entities_ContextRecreated);
	Event_UnregisterVoid(&ChatEvents_FontChanged,     NULL, Entities_ChatFontChanged);

	if (ShadowComponent_ShadowTex != NULL) {
		Gfx_DeleteTexture(&ShadowComponent_ShadowTex);
	}
}

void Entities_Remove(EntityID id) {
	Event_RaiseEntityID(&EntityEvents_Removed, id);
	Entities_List[id]->VTABLE->Despawn(Entities_List[id]);
	Entities_List[id] = NULL;
}

EntityID Entities_GetCloset(Entity* src) {
	Vector3 eyePos = Entity_GetEyePosition(src);
	Vector3 dir = Vector3_GetDirVector(src->HeadY * MATH_DEG2RAD, src->HeadX * MATH_DEG2RAD);
	Real32 closestDist = MATH_POS_INF;
	EntityID targetId = ENTITIES_SELF_ID;

	UInt32 i;
	for (i = 0; i < ENTITIES_SELF_ID; i++) { /* because we don't want to pick against local player */
		Entity* entity = Entities_List[i];
		if (entity == NULL) continue;

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
		UInt32 i;
		for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
			if (Entities_List[i] == NULL) continue;
			if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;
			ShadowComponent_Draw(Entities_List[i]);
		}
	}

	Gfx_SetAlphaArgBlend(false);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);
	Gfx_SetTexturing(false);
}

bool TabList_Valid(EntityID id) {
	return TabList_PlayerNames[id] > 0 || TabList_ListNames[id] > 0 || TabList_GroupNames[id] > 0;
}

bool TabList_Remove(EntityID id) {
	if (!TabList_Valid(id)) return false;

	StringsBuffer_Remove(&TabList_Buffer, TabList_PlayerNames[id]); TabList_PlayerNames[id] = 0;
	StringsBuffer_Remove(&TabList_Buffer, TabList_ListNames[id]);   TabList_ListNames[id]   = 0; 
	StringsBuffer_Remove(&TabList_Buffer, TabList_GroupNames[id]);  TabList_GroupNames[id]  = 0;
	TabList_GroupRanks[id] = 0;
	return true;
}

void TabList_Set(EntityID id, STRING_PURE String* player, STRING_PURE String* list, STRING_PURE String* group, UInt8 rank) {
	UInt8 playerNameBuffer[String_BufferSize(STRING_SIZE)];
	String playerName = String_InitAndClearArray(playerNameBuffer);
	String_AppendColorless(&playerName, player);

	TabList_PlayerNames[id] = TabList_Buffer.Count; StringsBuffer_Add(&TabList_Buffer, &playerName);
	TabList_ListNames[id] = TabList_Buffer.Count;   StringsBuffer_Add(&TabList_Buffer, list);
	TabList_GroupNames[id] = TabList_Buffer.Count;  StringsBuffer_Add(&TabList_Buffer, group);
	TabList_GroupRanks[id] = rank;
}

void TabList_Init(void) { StringsBuffer_Init(&TabList_Buffer); }
void TabList_Free(void) { StringsBuffer_Free(&TabList_Buffer); }
void TabList_Reset(void) {
	Platform_MemSet(TabList_PlayerNames, 0, sizeof(TabList_PlayerNames));
	Platform_MemSet(TabList_ListNames,   0, sizeof(TabList_ListNames));
	Platform_MemSet(TabList_GroupNames,  0, sizeof(TabList_GroupNames));
	Platform_MemSet(TabList_GroupRanks,  0, sizeof(TabList_GroupRanks));
	/* TODO: Should we be trying to free the buffer here? */
	StringsBuffer_UNSAFE_Reset(&TabList_Buffer);
}

IGameComponent TabList_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = TabList_Init;
	comp.Free = TabList_Free;
	comp.Reset = TabList_Reset;
	return comp;
}


#define PLAYER_NAME_EMPTY_TEX -30000
void Player_MakeNameTexture(Player* player) {
	FontDesc font; 
	Platform_MakeFont(&font, &Game_FontName, 24, FONT_STYLE_NORMAL);

	String displayName = String_FromRawArray(player->DisplayNameRaw);
	DrawTextArgs args; 
	DrawTextArgs_Make(&args, &displayName, &font, false);

	/* we want names to always be drawn not using the system font */
	bool bitmapped = Drawer2D_UseBitmappedChat;
	Drawer2D_UseBitmappedChat = true;
	Size2D size = Drawer2D_MeasureText(&args);

	if (size.Width == 0) {
		player->NameTex = Texture_MakeInvalid();
		player->NameTex.X = PLAYER_NAME_EMPTY_TEX;
	} else {
		UInt8 buffer[String_BufferSize(STRING_SIZE * 2)];
		String shadowName = String_InitAndClearArray(buffer);

		size.Width += 3; size.Height += 3;
		Bitmap bmp; Bitmap_AllocatePow2(&bmp, size.Width, size.Height);
		Drawer2D_Begin(&bmp);

		Drawer2D_Cols[0xFF] = PackedCol_Create3(80, 80, 80);
		String_Append(&shadowName, 0xFF);
		String_AppendColorless(&shadowName, &displayName);
		args.Text = shadowName;
		Drawer2D_DrawText(&args, 3, 3);

		Drawer2D_Cols[0xFF] = PackedCol_Create4(0, 0, 0, 0);
		args.Text = displayName;
		Drawer2D_DrawText(&args, 0, 0);

		Drawer2D_End();
		player->NameTex = Drawer2D_Make2DTexture(&bmp, size, 0, 0);
	}

	Drawer2D_UseBitmappedChat = bitmapped;
}

void Player_UpdateName(Player* player) {
	Entity* entity = &player->Base;
	entity->VTABLE->ContextLost(entity);

	if (Gfx_LostContext) return;
	Player_MakeNameTexture(player);
}

void Player_DrawName(Player* player) {
	Entity* entity = &player->Base;
	IModel* model = entity->Model;

	if (player->NameTex.X == PLAYER_NAME_EMPTY_TEX) return;
	if (player->NameTex.ID == NULL) Player_MakeNameTexture(player);
	Gfx_BindTexture(player->NameTex.ID);

	Vector3 pos;
	model->RecalcProperties(entity);
	Vector3_TransformY(&pos, model->NameYOffset, &entity->Transform);

	Real32 scale = model->NameScale * entity->ModelScale.Y;
	scale = scale > 1.0f ? (1.0f / 70.0f) : (scale / 70.0f);
	Vector2 size = { player->NameTex.Width * scale, player->NameTex.Height * scale };

	if (Entities_NameMode == NAME_MODE_ALL_UNSCALED && LocalPlayer_Instance.Hacks.CanSeeAllNames) {
		/* Get W component of transformed position */
		Matrix mat;
		Matrix_Mul(&mat, &Gfx_View, &Gfx_Projection); /* TODO: This mul is slow, avoid it */
		Real32 tempW = pos.X * mat.Row0.W + pos.Y * mat.Row1.W + pos.Z * mat.Row2.W + mat.Row3.W;
		size.X *= tempW * 0.2f; size.Y *= tempW * 0.2f;
	}

	VertexP3fT2fC4b vertices[4]; VertexP3fT2fC4b* ptr = vertices;
	TextureRec rec = { 0.0f, 0.0f, player->NameTex.U2, player->NameTex.V2 };
	PackedCol col = PACKEDCOL_WHITE;
	Particle_DoRender(&size, &pos, &rec, col, &ptr);

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	GfxCommon_UpdateDynamicVb_IndexedTris(GfxCommon_texVb, ptr, 4);
}

Player* Player_FirstOtherWithSameSkin(Player* player) {
	Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL || Entities_List[i] == entity) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		Player* p = (Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->SkinNameRaw);
		if (String_Equals(&skin, &pSkin)) return p;
	}
	return NULL;
}

Player* Player_FirstOtherWithSameSkinAndFetchedSkin(Player* player) {
	Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL || Entities_List[i] == entity) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		Player* p = (Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->SkinNameRaw);
		if (p->FetchedSkin && String_Equals(&skin, &pSkin)) return p;
	}
	return NULL;
}

void Player_ApplySkin(Player* player, Player* from) {
	Entity* dst = &player->Base;
	Entity* src = &from->Base;

	dst->TextureId    = src->TextureId;	
	dst->SkinType     = src->SkinType;
	dst->uScale       = src->uScale;
	dst->vScale       = src->vScale;

	/* Custom mob textures */
	dst->MobTextureId = NULL;
	String skin = String_FromRawArray(player->SkinNameRaw);
	if (Utils_IsUrlPrefix(&skin, 0)) {
		dst->MobTextureId = dst->TextureId;
	}
}

void Player_ResetSkin(Player* player) {
	Entity* entity = &player->Base;
	entity->uScale = 1; entity->vScale = 1.0f;
	entity->MobTextureId = NULL;
	entity->TextureId    = NULL;
	entity->SkinType = Game_DefaultPlayerSkinType;
}

/* Apply or reset skin, for all players with same skin */
void Player_SetSkinAll(Player* player, bool reset) {
	Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL || Entities_List[i] == entity) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		Player* p = (Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->SkinNameRaw);
		if (!String_Equals(&skin, &pSkin)) continue;

		if (reset) {
			Player_ResetSkin(p);
		} else {
			Player_ApplySkin(p, player);
		}
	}
}

void Player_ClearHat(Bitmap bmp, UInt8 skinType) {
	Int32 sizeX = (bmp.Width / 64) * 32;
	Int32 yScale = skinType == SKIN_TYPE_64x32 ? 32 : 64;
	Int32 sizeY = (bmp.Height / yScale) * 16;
	Int32 x, y;

	/* determine if we actually need filtering */
	for (y = 0; y < sizeY; y++) {
		UInt32* row = Bitmap_GetRow(&bmp, y);
		row += sizeX;
		for (x = 0; x < sizeX; x++) {
			UInt8 alpha = (UInt8)(row[x] >> 24);
			if (alpha != 255) return;
		}
	}

	/* only perform filtering when the entire hat is opaque */
	UInt32 fullWhite = PackedCol_ARGB(255, 255, 255, 255);
	UInt32 fullBlack = PackedCol_ARGB(0,   0,   0,   255);
	for (y = 0; y < sizeY; y++) {
		UInt32* row = Bitmap_GetRow(&bmp, y);
		row += sizeX;
		for (x = 0; x < sizeX; x++) {
			UInt32 pixel = row[x];
			if (pixel == fullWhite || pixel == fullBlack) row[x] = 0;
		}
	}
}

void Player_EnsurePow2(Player* player, Bitmap* bmp) {
	Int32 width  = Math_NextPowOf2(bmp->Width);
	Int32 height = Math_NextPowOf2(bmp->Height);
	if (width == bmp->Width && height == bmp->Height) return;

	Bitmap scaled; Bitmap_Allocate(&scaled, width, height);
	if (scaled.Scan0 == NULL) {
		ErrorHandler_Fail("Failed to allocate memory for resizing player skin");
	}

	Int32 y;
	for (y = 0; y < bmp->Height; y++) {
		Bitmap_CopyRow(y, y, bmp, &scaled, bmp->Width);
	}

	Entity* entity = &player->Base;
	entity->uScale = (Real32)bmp->Width  / width;
	entity->vScale = (Real32)bmp->Height / height;

	Platform_MemFree(bmp->Scan0);
	*bmp = scaled;
}

void Player_CheckSkin(Player* player) {
	Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);

	if (!player->FetchedSkin && entity->Model->UsesSkin) {
		Player* first = Player_FirstOtherWithSameSkinAndFetchedSkin(player);
		if (first == NULL) {
			AsyncDownloader_DownloadSkin(&skin, &skin);
		} else {
			Player_ApplySkin(player, first);
		}
		player->FetchedSkin = true;
	}

	AsyncRequest item;
	if (!AsyncDownloader_Get(&skin, &item)) return;
	Bitmap bmp = item.ResultBitmap;
	if (bmp.Scan0 == NULL) { Player_SetSkinAll(player, true); return; }

	Gfx_DeleteTexture(&entity->TextureId);
	Player_SetSkinAll(player, true);
	Player_EnsurePow2(player, &bmp);
	entity->SkinType = Utils_GetSkinType(&bmp);

	if (entity->SkinType == SKIN_TYPE_INVALID) {
		Player_SetSkinAll(player, true);
	} else {
		if (entity->Model->UsesHumanSkin) {
			Player_ClearHat(bmp, entity->SkinType);
		}
		entity->TextureId = Gfx_CreateTexture(&bmp, true, false);
		Player_SetSkinAll(player, false);
	}
	Platform_MemFree(bmp.Scan0);
}

void Player_Despawn(Entity* entity) {
	Player* player = (Player*)entity;
	Player* first = Player_FirstOtherWithSameSkin(player);
	if (first == NULL) {
		Gfx_DeleteTexture(&entity->TextureId);
		player->NameTex = Texture_MakeInvalid();
		Player_ResetSkin(player);
	}
	entity->VTABLE->ContextLost(entity);
}

void Player_ContextLost(Entity* entity) {
	Player* player = (Player*)entity;
	Gfx_DeleteTexture(&player->NameTex.ID);
	player->NameTex = Texture_MakeInvalid();
}

void Player_ContextRecreated(Entity* entity) {
	Player* player = (Player*)entity;
	Player_UpdateName(player);
}

EntityVTABLE playerDefaultVTABLE;
void Player_Init(Player* player) {
	/* TODO should we just remove the memset from entity_Init and player_init?? */
	Platform_MemSet(player + sizeof(Entity), 0, sizeof(Player) - sizeof(Entity));
	Entity* entity = &player->Base;
	Entity_Init(entity);
	entity->StepSize = 0.5f;
	entity->EntityType = ENTITY_TYPE_PLAYER;
	String model = String_FromConst("humanoid");
	Entity_SetModel(entity, &model);

	playerDefaultVTABLE = *entity->VTABLE;
	entity->VTABLE = &playerDefaultVTABLE;
	entity->VTABLE->ContextLost      = Player_ContextLost;
	entity->VTABLE->ContextRecreated = Player_ContextRecreated;
	entity->VTABLE->Despawn          = Player_Despawn;
}
