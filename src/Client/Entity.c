#include "Entity.h"
#include "ExtMath.h"
#include "World.h"
#include "Block.h"
#include "Event.h"
#include "Game.h"
#include "Player.h"
#include "Camera.h"
#include "Platform.h"
#include "Funcs.h"
#include "ModelCache.h"
#include "GraphicsAPI.h"
#include "Intersection.h"
#include "Lighting.h"

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

	entityDefaultVTABLE.ContextLost = Entity_NullFunc;
	entityDefaultVTABLE.ContextRecreated = Entity_NullFunc;
	entityDefaultVTABLE.GetCol = Entity_DefaultGetCol;
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

bool Entity_TouchesAny(AABB* bounds, TouchesAny_Condition condition) {
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
				if (condition(block)) return true;
			}
		}
	}
	return false;
}

bool Entity_IsRope(BlockID b) { return b == BLOCK_ROPE; }
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
	closestId = Entities_GetCloset(&p->Base.Base);
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

void Entities_ContextLost(void) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		Entities_List[i]->VTABLE->ContextLost(Entities_List[i]);
	}
}

void Entities_ContextRecreated(void) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		Entities_List[i]->VTABLE->ContextRecreated(Entities_List[i]);
	}
}

void Entities_ChatFontChanged(void) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;
		Player_UpdateName((Player*)Entities_List[i]);
	}
}

void Entities_Init(void) {
	Event_RegisterVoid(&GfxEvents_ContextLost, Entities_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, Entities_ContextRecreated);
	Event_RegisterVoid(&ChatEvents_FontChanged, Entities_ChatFontChanged);

	Entities_NameMode = Options_GetEnum(OPTION_NAMES_MODE, NAME_MODE_HOVERED,
		NameMode_Names, Array_NumElements(NameMode_Names));
	if (Game_ClassicMode) Entities_NameMode = NAME_MODE_HOVERED;

	Entities_ShadowMode = Options_GetEnum(OPTION_ENTITY_SHADOW, SHADOW_MODE_NONE,
		ShadowMode_Names, Array_NumElements(ShadowMode_Names));
	if (Game_ClassicMode) Entities_ShadowMode = SHADOW_MODE_NONE;
}

void Entities_Free(void) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL) continue;
		Entities_Remove((EntityID)i);
	}

	Event_UnregisterVoid(&GfxEvents_ContextLost, Entities_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, Entities_ContextRecreated);
	Event_UnregisterVoid(&ChatEvents_FontChanged, Entities_ChatFontChanged);

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
	for (i = 0; i < ENTITIES_SELF_ID; i++) { /* -1 because we don't want to pick against local player */
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