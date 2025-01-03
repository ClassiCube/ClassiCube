#include "Entity.h"
#include "ExtMath.h"
#include "World.h"
#include "Block.h"
#include "Event.h"
#include "Game.h"
#include "Camera.h"
#include "Platform.h"
#include "Funcs.h"
#include "Graphics.h"
#include "Lighting.h"
#include "Http.h"
#include "Chat.h"
#include "Model.h"
#include "Input.h"
#include "InputHandler.h"
#include "Gui.h"
#include "Stream.h"
#include "Bitmap.h"
#include "Logger.h"
#include "Options.h"
#include "Errors.h"
#include "Utils.h"
#include "EntityRenderers.h"
#include "Protocol.h"

const char* const NameMode_Names[NAME_MODE_COUNT]   = { "None", "Hovered", "All", "AllHovered", "AllUnscaled" };
const char* const ShadowMode_Names[SHADOW_MODE_COUNT] = { "None", "SnapToBlock", "Circle", "CircleAll" };


/*########################################################################################################################*
*---------------------------------------------------------Entity----------------------------------------------------------*
*#########################################################################################################################*/
static PackedCol Entity_GetColor(struct Entity* e) {
	Vec3 eyePos = Entity_GetEyePosition(e);
	IVec3 pos; IVec3_Floor(&pos, &eyePos);
	return Lighting.Color(pos.x, pos.y, pos.z);
}

void Entity_Init(struct Entity* e) {
	static const cc_string model = String_FromConst("humanoid");
	Vec3_Set(e->ModelScale, 1,1,1);
	e->Flags      = ENTITY_FLAG_HAS_MODELVB;
	e->uScale     = 1.0f;
	e->vScale     = 1.0f;
	e->_skinReqID = 0;
	e->SkinRaw[0] = '\0';
	e->NameRaw[0] = '\0';
	Entity_SetModel(e, &model);
}

void Entity_SetName(struct Entity* e, const cc_string* name) {
	EntityNames_Delete(e);
	String_CopyToRawArray(e->NameRaw, name);
}

Vec3 Entity_GetEyePosition(struct Entity* e) {
	Vec3 pos = e->Position; pos.y += Entity_GetEyeHeight(e); return pos;
}

float Entity_GetEyeHeight(struct Entity* e) {
	return e->Model->GetEyeY(e) * e->ModelScale.y;
}

void Entity_GetTransform(struct Entity* e, Vec3 pos, Vec3 scale, struct Matrix* m) {
	struct Matrix tmp;
	Matrix_Scale(m, scale.x, scale.y, scale.z);

	if (e->RotZ) {
		Matrix_RotateZ( &tmp, -e->RotZ * MATH_DEG2RAD);
		Matrix_MulBy(m, &tmp);
	}
	if (e->RotX) {
		Matrix_RotateX( &tmp, -e->RotX * MATH_DEG2RAD);
		Matrix_MulBy(m, &tmp);
	}
	if (e->RotY) {
		Matrix_RotateY( &tmp, -e->RotY * MATH_DEG2RAD);
		Matrix_MulBy(m, &tmp);
	}

	Matrix_Translate(&tmp, pos.x, pos.y, pos.z);
	Matrix_MulBy(m,  &tmp);
	/* return scale * rotZ * rotX * rotY * translate; */
}

void Entity_GetPickingBounds(struct Entity* e, struct AABB* bb) {
	AABB_Offset(bb, &e->ModelAABB, &e->Position);
}

void Entity_GetBounds(struct Entity* e, struct AABB* bb) {
	AABB_Make(bb, &e->Position, &e->Size);
}

static void Entity_ParseScale(struct Entity* e, const cc_string* scale) {
	float value;
	if (!Convert_ParseFloat(scale, &value)) return;
	value = max(value, 0.001f);

	/* local player doesn't allow giant model scales */
	/* (can't climb stairs, extremely CPU intensive collisions) */
	if (e->Flags & ENTITY_FLAG_MODEL_RESTRICTED_SCALE) {
		value = min(value, e->Model->maxScale);
	}
	Vec3_Set(e->ModelScale, value,value,value);
}

static void Entity_SetBlockModel(struct Entity* e, const cc_string* model) {
	static const cc_string block = String_FromConst("block");
	int raw = Block_Parse(model);

	if (raw == -1) {
		/* use default humanoid model */
		e->Model      = Models.Human;
	} else {	
		e->ModelBlock = (BlockID)raw;
		e->Model      = Model_Get(&block);
	}
}

void Entity_SetModel(struct Entity* e, const cc_string* model) {
	cc_string name, scale;
	Vec3_Set(e->ModelScale, 1,1,1);
	String_UNSAFE_Separate(model, '|', &name, &scale);

	/* 'giant' model kept for backwards compatibility */
	if (String_CaselessEqualsConst(&name, "giant")) {
		name = String_FromReadonly("humanoid");
		Vec3_Set(e->ModelScale, 2,2,2);
	}

	e->ModelBlock = BLOCK_AIR;
	e->Model      = Model_Get(&name);
	if (!e->Model) Entity_SetBlockModel(e, &name);

	Entity_ParseScale(e, &scale);
	Entity_UpdateModelBounds(e);

	if (e->Flags & ENTITY_FLAG_HAS_MODELVB)
		Gfx_DeleteDynamicVb(&e->ModelVB);
}

void Entity_UpdateModelBounds(struct Entity* e) {
	struct Model* model = e->Model;
	model->GetCollisionSize(e);
	model->GetPickingBounds(e);

	Vec3_Mul3By(&e->Size,          &e->ModelScale);
	Vec3_Mul3By(&e->ModelAABB.Min, &e->ModelScale);
	Vec3_Mul3By(&e->ModelAABB.Max, &e->ModelScale);
}

cc_bool Entity_TouchesAny(struct AABB* bounds, Entity_TouchesCondition condition) {
	IVec3 bbMin, bbMax;
	BlockID block;
	struct AABB blockBB;
	Vec3 v;
	int x, y, z;

	IVec3_Floor(&bbMin, &bounds->Min);
	IVec3_Floor(&bbMax, &bounds->Max);

	bbMin.x = max(bbMin.x, 0); bbMax.x = min(bbMax.x, World.MaxX);
	bbMin.y = max(bbMin.y, 0); bbMax.y = min(bbMax.y, World.MaxY);
	bbMin.z = max(bbMin.z, 0); bbMax.z = min(bbMax.z, World.MaxZ);

	for (y = bbMin.y; y <= bbMax.y; y++) { v.y = (float)y;
		for (z = bbMin.z; z <= bbMax.z; z++) { v.z = (float)z;
			for (x = bbMin.x; x <= bbMax.x; x++) { v.x = (float)x;

				block = World_GetBlock(x, y, z);
				Vec3_Add(&blockBB.Min, &v, &Blocks.MinBB[block]);
				Vec3_Add(&blockBB.Max, &v, &Blocks.MaxBB[block]);

				if (!AABB_Intersects(&blockBB, bounds)) continue;
				if (condition(block)) return true;
			}
		}
	}
	return false;
}

static cc_bool IsRopeCollide(BlockID b) { return Blocks.ExtendedCollide[b] == COLLIDE_CLIMB; }
cc_bool Entity_TouchesAnyRope(struct Entity* e) {
	struct AABB bounds; Entity_GetBounds(e, &bounds);
	bounds.Max.y += 0.5f / 16.0f;
	return Entity_TouchesAny(&bounds, IsRopeCollide);
}

static const Vec3 entity_liqExpand = { 0.25f/16.0f, 0.0f/16.0f, 0.25f/16.0f };
static cc_bool IsLavaCollide(BlockID b) { return Blocks.ExtendedCollide[b] == COLLIDE_LAVA; }
cc_bool Entity_TouchesAnyLava(struct Entity* e) {
	struct AABB bounds; Entity_GetBounds(e, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, IsLavaCollide);
}

static cc_bool IsWaterCollide(BlockID b) { return Blocks.ExtendedCollide[b] == COLLIDE_WATER; }
cc_bool Entity_TouchesAnyWater(struct Entity* e) {
	struct AABB bounds; Entity_GetBounds(e, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, IsWaterCollide);
}


/*########################################################################################################################*
*------------------------------------------------------Entity skins-------------------------------------------------------*
*#########################################################################################################################*/
static struct Entity* Entity_FirstOtherWithSameSkinAndFetchedSkin(struct Entity* except) {
	struct Entity* e;
	cc_string skin, eSkin;
	int i;

	skin = String_FromRawArray(except->SkinRaw);
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities.List[i] || Entities.List[i] == except) continue;

		e     = Entities.List[i];
		eSkin = String_FromRawArray(e->SkinRaw);
		if (e->SkinFetchState && String_Equals(&skin, &eSkin)) return e;
	}
	return NULL;
}

/* Copies skin data from another entity */
static void Entity_CopySkin(struct Entity* dst, struct Entity* src) {
	dst->TextureId    = src->TextureId;	
	dst->SkinType     = src->SkinType;
	dst->uScale       = src->uScale;
	dst->vScale       = src->vScale;
	dst->MobTextureId = src->MobTextureId;
}

/* Resets skin data for the given entity */
static void Entity_ResetSkin(struct Entity* e) {
	e->uScale = 1.0f; e->vScale = 1.0f;
	e->MobTextureId = 0;
	e->TextureId    = 0;
	e->SkinType     = SKIN_64x32;
}

/* Copies or resets skin data for all entity with same skin */
static void Entity_SetSkinAll(struct Entity* source, cc_bool reset) {
	struct Entity* e;
	cc_string skin, eSkin;
	int i;

	skin = String_FromRawArray(source->SkinRaw);
	source->MobTextureId = Utils_IsUrlPrefix(&skin) ? source->TextureId : 0;

	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (!Entities.List[i]) continue;

		e     = Entities.List[i];
		eSkin = String_FromRawArray(e->SkinRaw);
		if (!String_Equals(&skin, &eSkin)) continue;

		if (reset) {
			Entity_ResetSkin(e);
		} else {
			Entity_CopySkin(e, source);
		}
		e->SkinFetchState = SKIN_FETCH_COMPLETED;
	}
}

/* Clears hat area from a skin bitmap if it's completely white or black,
   so skins edited with Microsoft Paint or similiar don't have a solid hat */
static void Entity_ClearHat(struct Bitmap* bmp, cc_uint8 skinType) {
	int sizeX  = (bmp->width / 64) * 32;
	int yScale = skinType == SKIN_64x32 ? 32 : 64;
	int sizeY  = (bmp->height / yScale) * 16;
	int x, y;

	/* determine if we actually need filtering */
	for (y = 0; y < sizeY; y++) {
		BitmapCol* row = Bitmap_GetRow(bmp, y) + sizeX;
		for (x = 0; x < sizeX; x++) {
			if (BitmapCol_A(row[x]) != 255) return;
		}
	}

	/* only perform filtering when the entire hat is opaque */
	for (y = 0; y < sizeY; y++) {
		BitmapCol* row = Bitmap_GetRow(bmp, y) + sizeX;
		for (x = 0; x < sizeX; x++) {
			BitmapCol c = row[x];
			if (c == BITMAPCOLOR_WHITE || c == BITMAPCOLOR_BLACK) row[x] = 0;
		}
	}
}

/* Ensures skin is a power of two size, resizing if needed. */
static cc_result EnsurePow2Skin(struct Entity* e, struct Bitmap* bmp) {
	struct Bitmap scaled;
	cc_uint32 stride;
	int width, height;
	int y;

	width  = Math_NextPowOf2(bmp->width);
	height = Math_NextPowOf2(bmp->height);
	if (width == bmp->width && height == bmp->height) return 0;

	Bitmap_TryAllocate(&scaled, width, height);
	if (!scaled.scan0) return ERR_OUT_OF_MEMORY;

	e->uScale = (float)bmp->width  / width;
	e->vScale = (float)bmp->height / height;
	stride = bmp->width * 4;

	for (y = 0; y < bmp->height; y++) {
		BitmapCol* src = Bitmap_GetRow(bmp, y);
		BitmapCol* dst = Bitmap_GetRow(&scaled, y);
		Mem_Copy(dst, src, stride);
	}

	Mem_Free(bmp->scan0);
	*bmp = scaled;
	return 0;
}

static cc_result ApplySkin(struct Entity* e, struct Bitmap* bmp, struct Stream* src, cc_string* skin) {
	cc_result res;
	if ((res = Png_Decode(bmp, src))) return res;

	Gfx_DeleteTexture(&e->TextureId);
	Entity_SetSkinAll(e, true);
	if ((res = EnsurePow2Skin(e, bmp))) return res;
	e->SkinType = Utils_CalcSkinType(bmp);

	if (!Gfx_CheckTextureSize(bmp->width, bmp->height, 0)) {
		Chat_Add1("&cSkin %s is too large", skin);
	} else {
		if (e->Model->flags & MODEL_FLAG_CLEAR_HAT)
			Entity_ClearHat(bmp, e->SkinType);

		e->TextureId = Gfx_CreateTexture(bmp, TEXTURE_FLAG_MANAGED, false);
		Entity_SetSkinAll(e, false);
	}
	return 0;
}

static void LogInvalidSkin(cc_result res, const cc_string* skin, const cc_uint8* data, int size) {
	cc_string msg; char msgBuffer[256];
	String_InitArray(msg, msgBuffer);

	Logger_FormatWarn2(&msg, res, "decoding skin", skin, Platform_DescribeError);
	if (res != PNG_ERR_INVALID_SIG) { Logger_WarnFunc(&msg); return; }

	String_AppendConst(&msg, " (got ");
	String_AppendAll(  &msg, data, min(size, 8));
	String_AppendConst(&msg, ")");
	Logger_WarnFunc(&msg);
}

static void Entity_CheckSkin(struct Entity* e) {
	struct Entity* first;
	struct HttpRequest item;
	struct Stream mem;
	struct Bitmap bmp;
	cc_string skin;
	cc_uint8 flags;
	cc_result res;

	/* Don't check skin if don't have to */
	if (!e->Model->usesSkin) return;
	if (e->SkinFetchState == SKIN_FETCH_COMPLETED) return;
	skin = String_FromRawArray(e->SkinRaw);

	if (!e->SkinFetchState) {
		first = Entity_FirstOtherWithSameSkinAndFetchedSkin(e);
		flags = e == &LocalPlayer_Instances[0].Base ? HTTP_FLAG_NOCACHE : 0;

		if (!first) {
			e->_skinReqID     = Http_AsyncGetSkin(&skin, flags);
			e->SkinFetchState = SKIN_FETCH_DOWNLOADING;
		} else {
			Entity_CopySkin(e, first);
			e->SkinFetchState = SKIN_FETCH_COMPLETED;
			return;
		}
	}

	if (!Http_GetResult(e->_skinReqID, &item)) return;

	if (!item.success) {
		Entity_SetSkinAll(e, true);
	} else {
		Stream_ReadonlyMemory(&mem, item.data, item.size);

		if ((res = ApplySkin(e, &bmp, &mem, &skin))) {
			LogInvalidSkin(res, &skin, item.data, item.size);
		}
		Mem_Free(bmp.scan0);
	}
	HttpRequest_Free(&item);
}

/* Returns true if no other entities are sharing this skin texture */
static cc_bool CanDeleteTexture(struct Entity* except) {
	int i;
	if (!except->TextureId) return false;

	for (i = 0; i < ENTITIES_MAX_COUNT; i++)
	{
		if (!Entities.List[i] || Entities.List[i] == except)  continue;
		if (Entities.List[i]->TextureId == except->TextureId) return false;
	}
	return true;
}

CC_NOINLINE static void DeleteSkin(struct Entity* e) {
	if (CanDeleteTexture(e)) Gfx_DeleteTexture(&e->TextureId);

	Entity_ResetSkin(e);
	e->SkinFetchState = 0;
}

void Entity_SetSkin(struct Entity* e, const cc_string* skin) {
	cc_string tmp; char tmpBuffer[STRING_SIZE];
	DeleteSkin(e);

	if (Utils_IsUrlPrefix(skin)) {
		tmp = *skin;
	} else {
		String_InitArray(tmp, tmpBuffer);
		String_AppendColorless(&tmp, skin);
	}
	String_CopyToRawArray(e->SkinRaw, &tmp);
}

void Entity_LerpAngles(struct Entity* e, float t) {
	struct EntityLocation* prev = &e->prev;
	struct EntityLocation* next = &e->next;

	e->Pitch = Math_LerpAngle(prev->pitch, next->pitch, t);
	e->Yaw   = Math_LerpAngle(prev->yaw,   next->yaw,   t);
	e->RotX  = Math_LerpAngle(prev->rotX,  next->rotX,  t);
	e->RotY  = Math_LerpAngle(prev->rotY,  next->rotY,  t);
	e->RotZ  = Math_LerpAngle(prev->rotZ,  next->rotZ,  t);
}


/*########################################################################################################################*
*--------------------------------------------------------Entities---------------------------------------------------------*
*#########################################################################################################################*/
struct _EntitiesData Entities;

void Entities_Tick(struct ScheduledTask* task) {
	int i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++)
	{
		if (!Entities.List[i]) continue;
		Entities.List[i]->VTABLE->Tick(Entities.List[i], task->interval);
	}
}

void Entities_RenderModels(float delta, float t) {
	int i;
	Gfx_SetAlphaTest(true);
	
	for (i = 0; i < ENTITIES_MAX_COUNT; i++)
	{
		if (!Entities.List[i]) continue;
		Entities.List[i]->VTABLE->RenderModel(Entities.List[i], delta, t);
	}
	Gfx_SetAlphaTest(false);
}

static void Entities_ContextLost(void* obj) {
	struct Entity* entity;
	int i;

	for (i = 0; i < ENTITIES_MAX_COUNT; i++)
	{
		entity = Entities.List[i];
		if (!entity) continue;

		if (entity->Flags & ENTITY_FLAG_HAS_MODELVB)
			Gfx_DeleteDynamicVb(&entity->ModelVB);

		if (!Gfx.ManagedTextures)
			DeleteSkin(entity);
	}
}
/* No OnContextCreated, skin textures remade when needed */

void Entities_Remove(int id) {
	struct Entity* e = Entities.List[id];
	if (!e) return;

	Event_RaiseInt(&EntityEvents.Removed, id);
	e->VTABLE->Despawn(e);
	Entities.List[id] = NULL;

	/* TODO: Move to EntityEvents.Removed callback instead */
	if (id < TABLIST_MAX_NAMES && TabList_EntityLinked_Get(id)) {
		TabList_Remove(id);
		TabList_EntityLinked_Reset(id);
	}
}

int Entities_GetClosest(struct Entity* src) {
	Vec3 eyePos = Entity_GetEyePosition(src);
	Vec3 dir    = Vec3_GetDirVector(src->Yaw * MATH_DEG2RAD, src->Pitch * MATH_DEG2RAD);
	float closestDist = -200; /* NOTE: was previously positive infinity */
	int targetID = -1;

	float t0, t1;
	int i;

	for (i = 0; i < ENTITIES_MAX_COUNT; i++) /* because we don't want to pick against local player */
	{
		struct Entity* e = Entities.List[i];
		if (!e || e == &Entities.CurPlayer->Base) continue;
		if (!Intersection_RayIntersectsRotatedBox(eyePos, dir, e, &t0, &t1)) continue;

		if (targetID == -1 || t0 < closestDist) {
			closestDist = t0;
			targetID    = i;
		}
	}
	return targetID;
}

static void Player_Despawn(struct Entity* e) {
	DeleteSkin(e);
	EntityNames_Delete(e);

	if (e->Flags & ENTITY_FLAG_HAS_MODELVB)
		Gfx_DeleteDynamicVb(&e->ModelVB);
}


/*########################################################################################################################*
*--------------------------------------------------------TabList----------------------------------------------------------*
*#########################################################################################################################*/
struct _TabListData TabList;

/* Removes the names from the names buffer for the given id. */
static void TabList_Delete(EntityID id) {
	int i, index;
	index = TabList.NameOffsets[id];
	if (!index) return;

	StringsBuffer_Remove(&TabList._buffer, index - 1);
	StringsBuffer_Remove(&TabList._buffer, index - 2);
	StringsBuffer_Remove(&TabList._buffer, index - 3);

	/* Indices after this entry need to be shifted down */
	for (i = 0; i < TABLIST_MAX_NAMES; i++) {
		if (TabList.NameOffsets[i] > index) TabList.NameOffsets[i] -= 3;
	}
}

void TabList_Remove(EntityID id) {
	TabList_Delete(id);
	TabList.NameOffsets[id] = 0;
	TabList.GroupRanks[id]  = 0;
	Event_RaiseInt(&TabListEvents.Removed, id);
}

void TabList_Set(EntityID id, const cc_string* player_, const cc_string* list, const cc_string* group, cc_uint8 rank) {
	cc_string oldPlayer, oldList, oldGroup;
	cc_uint8 oldRank;
	struct Event_Int* events;

	/* Player name shouldn't have colour codes */
	/*  (intended for e.g. tab autocomplete) */
	cc_string player; char playerBuffer[STRING_SIZE];
	String_InitArray(player, playerBuffer);
	String_AppendColorless(&player, player_);
	
	if (TabList.NameOffsets[id]) {
		oldPlayer = TabList_UNSAFE_GetPlayer(id);
		oldList   = TabList_UNSAFE_GetList(id);
		oldGroup  = TabList_UNSAFE_GetGroup(id);
		oldRank   = TabList.GroupRanks[id];

		/* Don't redraw the tab list if nothing changed */
		if (String_Equals(&player, &oldPlayer) && String_Equals(list, &oldList)
			&& String_Equals(group, &oldGroup) && rank == oldRank) return;

		events = &TabListEvents.Changed;
	} else {
		events = &TabListEvents.Added;
	}
	TabList_Delete(id);

	StringsBuffer_Add(&TabList._buffer, &player);
	StringsBuffer_Add(&TabList._buffer, list);
	StringsBuffer_Add(&TabList._buffer, group);

	TabList.NameOffsets[id] = TabList._buffer.count;
	TabList.GroupRanks[id]  = rank;
	Event_RaiseInt(events, id);
}

static void Tablist_Init(void) {
	TabList_Set(ENTITIES_SELF_ID, &Game_Username, &Game_Username, &String_Empty, 0);
}

static void TabList_Clear(void) {
	Mem_Set(TabList.NameOffsets, 0, sizeof(TabList.NameOffsets));
	Mem_Set(TabList.GroupRanks,  0, sizeof(TabList.GroupRanks));
	StringsBuffer_Clear(&TabList._buffer);
}

struct IGameComponent TabList_Component = {
	Tablist_Init,  /* Init  */
	TabList_Clear, /* Free  */
	TabList_Clear  /* Reset */
};


/*########################################################################################################################*
*------------------------------------------------------LocalPlayer--------------------------------------------------------*
*#########################################################################################################################*/
struct LocalPlayer LocalPlayer_Instances[MAX_LOCAL_PLAYERS];
static cc_bool hackPermMsgs;
static struct LocalPlayerInput* sources_head;
static struct LocalPlayerInput* sources_tail;

void LocalPlayerInput_Add(struct LocalPlayerInput* source) {
	LinkedList_Append(source, sources_head, sources_tail);
}

void LocalPlayerInput_Remove(struct LocalPlayerInput* source) {
	struct LocalPlayerInput* cur;
	LinkedList_Remove(source, cur, sources_head, sources_tail);
}

float LocalPlayer_JumpHeight(struct LocalPlayer* p) {
	return (float)PhysicsComp_CalcMaxHeight(p->Physics.JumpVel);
}

void LocalPlayer_SetInterpPosition(struct LocalPlayer* p, float t) {
	if (!(p->Hacks.WOMStyleHacks && p->Hacks.Noclip)) {
		Vec3_Lerp(&p->Base.Position, &p->Base.prev.pos, &p->Base.next.pos, t);
	}
	Entity_LerpAngles(&p->Base, t);
}

static void LocalPlayer_HandleInput(struct LocalPlayer* p, float* xMoving, float* zMoving) {
	struct HacksComp* hacks = &p->Hacks;
	struct LocalPlayerInput* input;
	if (Gui.InputGrab) return;

	/* keyboard input, touch, joystick, etc */
	for (input = sources_head; input; input = input->next) {
		input->GetMovement(p, xMoving, zMoving);
	}
	*xMoving *= 0.98f;
	*zMoving *= 0.98f;

	if (hacks->WOMStyleHacks && hacks->Enabled && hacks->CanNoclip) {
		if (hacks->Noclip) {
			/* need a { } block because it's a macro */
			Vec3_Set(p->Base.Velocity, 0,0,0);
		}
		HacksComp_SetNoclip(hacks, hacks->_noclipping);
	}
}

static void LocalPlayer_SetLocation(struct Entity* e, struct LocationUpdate* update) {
	struct LocalPlayer* p = (struct LocalPlayer*)e;
	LocalInterpComp_SetLocation(&p->Interp, update, e);
}

static void LocalPlayer_Tick(struct Entity* e, float delta) {
	struct LocalPlayer* p = (struct LocalPlayer*)e;
	struct HacksComp* hacks = &p->Hacks;
	float xMoving = 0, zMoving = 0;
	cc_bool wasOnGround;
	Vec3 headingVelocity;

	if (!World.Loaded) return;
	p->Collisions.StepSize = hacks->FullBlockStep && hacks->Enabled && hacks->CanSpeed ? 1.0f : 0.5f;
	p->OldVelocity = e->Velocity;
	wasOnGround    = e->OnGround;

	LocalInterpComp_AdvanceState(&p->Interp, e);
	LocalPlayer_HandleInput(p, &xMoving, &zMoving);
	hacks->Floating = hacks->Noclip || hacks->Flying;
	if (!hacks->Floating && hacks->CanBePushed) PhysicsComp_DoEntityPush(e);

	/* Immediate stop in noclip mode */
	if (!hacks->NoclipSlide && (hacks->Noclip && xMoving == 0 && zMoving == 0)) {
		Vec3_Set(e->Velocity, 0,0,0);
	}

	PhysicsComp_UpdateVelocityState(&p->Physics);
	headingVelocity = Vec3_RotateY3(xMoving, 0, zMoving, e->Yaw * MATH_DEG2RAD);
	PhysicsComp_PhysicsTick(&p->Physics, headingVelocity);

	/* Fixes high jump, when holding down a movement key, jump, fly, then let go of fly key */
	if (p->Hacks.Floating) e->Velocity.y = 0.0f;

	e->next.pos = e->Position; e->Position = e->prev.pos;
	AnimatedComp_Update(e, e->prev.pos, e->next.pos, delta);
	TiltComp_Update(p, &p->Tilt, delta);

	Entity_CheckSkin(&p->Base);
	SoundComp_Tick(p, wasOnGround);
}

static void LocalPlayer_RenderModel(struct Entity* e, float delta, float t) {
	struct LocalPlayer* p = (struct LocalPlayer*)e;
	AnimatedComp_GetCurrent(e, t);
	TiltComp_GetCurrent(p, &p->Tilt, t);

	if (!Camera.Active->isThirdPerson && p == Entities.CurPlayer) return;
	Model_Render(e->Model, e);
}

static cc_bool LocalPlayer_ShouldRenderName(struct Entity* e) {
	return Camera.Active->isThirdPerson;
}

static void LocalPlayer_CheckJumpVelocity(void* obj) {
	struct LocalPlayer* p = (struct LocalPlayer*)obj;
	if (!HacksComp_CanJumpHigher(&p->Hacks)) {
		p->Physics.JumpVel = p->Physics.ServerJumpVel;
	}
}

static const struct EntityVTABLE localPlayer_VTABLE = {
	LocalPlayer_Tick,        Player_Despawn,         LocalPlayer_SetLocation, Entity_GetColor,
	LocalPlayer_RenderModel, LocalPlayer_ShouldRenderName
};
static void LocalPlayer_Init(struct LocalPlayer* p, int index) {
	struct HacksComp* hacks = &p->Hacks;

	Entity_Init(&p->Base);
	Entity_SetName(&p->Base, &Game_Username);
	Entity_SetSkin(&p->Base, &Game_Username);
	Event_Register_(&UserEvents.HackPermsChanged, p, LocalPlayer_CheckJumpVelocity);

	p->Collisions.Entity = &p->Base;
	HacksComp_Init(hacks);
	PhysicsComp_Init(&p->Physics, &p->Base);
	TiltComp_Init(&p->Tilt);

	p->Base.Flags |= ENTITY_FLAG_MODEL_RESTRICTED_SCALE;
	p->ReachDistance = 5.0f;
	p->Physics.Hacks = &p->Hacks;
	p->Physics.Collisions = &p->Collisions;
	p->Base.VTABLE   = &localPlayer_VTABLE;
	p->index = index;

	hacks->Enabled = !Game_PureClassic && Options_GetBool(OPT_HACKS_ENABLED, true);
	if (Game_ClassicMode) return;

	hacks->SpeedMultiplier = Options_GetFloat(OPT_SPEED_FACTOR,  0.1f, 50.0f, 10.0f);
	hacks->PushbackPlacing = Options_GetBool(OPT_PUSHBACK_PLACING, false);
	hacks->NoclipSlide     = Options_GetBool(OPT_NOCLIP_SLIDE,     false);
	hacks->WOMStyleHacks   = Options_GetBool(OPT_WOM_STYLE_HACKS,  false);
	hacks->FullBlockStep   = Options_GetBool(OPT_FULL_BLOCK_STEP,  false);
	p->Physics.UserJumpVel = Options_GetFloat(OPT_JUMP_VELOCITY, 0.0f, 52.0f, 0.42f);
	p->Physics.JumpVel     = p->Physics.UserJumpVel;
	hackPermMsgs           = Options_GetBool(OPT_HACK_PERM_MSGS, true);
}

void LocalPlayer_ResetJumpVelocity(struct LocalPlayer* p) {
	cc_bool higher = HacksComp_CanJumpHigher(&p->Hacks);

	p->Physics.JumpVel       = higher ? p->Physics.UserJumpVel : 0.42f;
	p->Physics.ServerJumpVel = p->Physics.JumpVel;
}

static void LocalPlayer_Reset(struct LocalPlayer* p) {
	p->ReachDistance = 5.0f;
	Vec3_Set(p->Base.Velocity, 0,0,0);
	LocalPlayer_ResetJumpVelocity(p);
}

static void LocalPlayers_Reset(void) {
	int i;
	for (i = 0; i < Game_NumStates; i++)
	{
		LocalPlayer_Reset(&LocalPlayer_Instances[i]);
	}
}

static void LocalPlayer_OnNewMap(struct LocalPlayer* p) {
	Vec3_Set(p->Base.Velocity, 0,0,0);
	Vec3_Set(p->OldVelocity,   0,0,0);

	p->_warnedRespawn = false;
	p->_warnedFly     = false;
	p->_warnedNoclip  = false;
	p->_warnedZoom    = false;
}

static void LocalPlayers_OnNewMap(void) {
	int i;
	for (i = 0; i < Game_NumStates; i++)
	{
		LocalPlayer_OnNewMap(&LocalPlayer_Instances[i]);
	}
}

static cc_bool LocalPlayer_IsSolidCollide(BlockID b) { return Blocks.Collide[b] == COLLIDE_SOLID; }

static void LocalPlayer_DoRespawn(struct LocalPlayer* p) {
	struct LocationUpdate update;
	struct AABB bb;
	Vec3 spawn = p->Spawn;
	IVec3 pos;
	BlockID block;
	float height, spawnY;
	int y;

	if (!World.Loaded) return;
	IVec3_Floor(&pos, &spawn);	

	/* Spawn player at highest solid position to match vanilla Minecraft classic */
	/* Only when player can noclip, since this can let you 'clip' to above solid blocks */
	if (p->Hacks.CanNoclip) {
		AABB_Make(&bb, &spawn, &p->Base.Size);
		for (y = pos.y; y <= World.Height; y++) {
			spawnY = Respawn_HighestSolidY(&bb);

			if (spawnY == RESPAWN_NOT_FOUND) {
				block   = World_SafeGetBlock(pos.x, y, pos.z);
				height  = Blocks.Collide[block] == COLLIDE_SOLID ? Blocks.MaxBB[block].y : 0.0f;
				spawn.y = y + height + ENTITY_ADJUSTMENT;
				break;
			}
			bb.Min.y += 1.0f; bb.Max.y += 1.0f;
		}
	}

	struct EntityLocation* prev = &p->Base.prev;
	CPE_SendNotifyPositionAction(3, prev->pos.x, prev->pos.y, prev->pos.z);

	/* Adjust the position to be slightly above the ground, so that */
	/*  it's obvious to the player that they are being respawned */
	spawn.y += 2.0f/16.0f;

	update.flags = LU_HAS_POS | LU_HAS_YAW | LU_HAS_PITCH;
	update.pos   = spawn;
	update.yaw   = p->SpawnYaw;
	update.pitch = p->SpawnPitch;
	p->Base.VTABLE->SetLocation(&p->Base, &update);

	Vec3_Set(p->Base.Velocity, 0,0,0);
	/* Update onGround, otherwise if 'respawn' then 'space' is pressed, you still jump into the air if onGround was true before */
	Entity_GetBounds(&p->Base, &bb);
	bb.Min.y -= 0.01f; bb.Max.y = bb.Min.y;
	p->Base.OnGround = Entity_TouchesAny(&bb, LocalPlayer_IsSolidCollide);
}

static cc_bool LocalPlayer_HandleRespawn(int key, struct InputDevice* device) {
	struct LocalPlayer* p = &LocalPlayer_Instances[device->mappedIndex];
	if (Gui.InputGrab) return false;
	
	if (p->Hacks.CanRespawn) {
		LocalPlayer_DoRespawn(p);
		return true;
	} else if (!p->_warnedRespawn) {
		p->_warnedRespawn = true;
		if (hackPermMsgs) Chat_AddRaw("&cRespawning is currently disabled");
	}
	return false;
}

static cc_bool LocalPlayer_HandleSetSpawn(int key, struct InputDevice* device) {
	struct LocalPlayer* p = &LocalPlayer_Instances[device->mappedIndex];
	if (Gui.InputGrab) return false;
	
	if (p->Hacks.CanRespawn) {

		if (!p->Hacks.CanNoclip && !p->Base.OnGround) {
			Chat_AddRaw("&cCannot set spawn midair when noclip is disabled");
			return false;
		}

		/* Spawn is normally centered to match vanilla Minecraft classic */
		if (!p->Hacks.CanNoclip) {
			/* Don't want to use Position because it is interpolated between prev and next. */
			/* This means it can be halfway between stepping up a stair and clip through the floor. */
			p->Spawn   = p->Base.prev.pos;
		} else {
			p->Spawn.x = Math_Floor(p->Base.Position.x) + 0.5f;
			p->Spawn.y = p->Base.Position.y;
			p->Spawn.z = Math_Floor(p->Base.Position.z) + 0.5f;
		}
		
		p->SpawnYaw   = p->Base.Yaw;
		if (!Game_ClassicMode) p->SpawnPitch = p->Base.Pitch;

		CPE_SendNotifyPositionAction(4, p->Spawn.x, p->Spawn.y, p->Spawn.z);
	}
	return LocalPlayer_HandleRespawn(key, device);
}

static cc_bool LocalPlayer_HandleFly(int key, struct InputDevice* device) {
	struct LocalPlayer* p = &LocalPlayer_Instances[device->mappedIndex];
	if (Gui.InputGrab) return false;

	if (p->Hacks.CanFly && p->Hacks.Enabled) {
		HacksComp_SetFlying(&p->Hacks, !p->Hacks.Flying);
		return true;
	} else if (!p->_warnedFly) {
		p->_warnedFly = true;
		if (hackPermMsgs) Chat_AddRaw("&cFlying is currently disabled");
	}
	return false;
}

static cc_bool LocalPlayer_HandleNoclip(int key, struct InputDevice* device) {
	struct LocalPlayer* p = &LocalPlayer_Instances[device->mappedIndex];
	p->Hacks._noclipping = true;
	if (Gui.InputGrab) return false;

	if (p->Hacks.CanNoclip && p->Hacks.Enabled) {
		if (p->Hacks.WOMStyleHacks) return true; /* don't handle this here */
		if (p->Hacks.Noclip) p->Base.Velocity.y = 0;

		HacksComp_SetNoclip(&p->Hacks, !p->Hacks.Noclip);
		return true;
	} else if (!p->_warnedNoclip) {
		p->_warnedNoclip = true;
		if (hackPermMsgs) Chat_AddRaw("&cNoclip is currently disabled");
	}
	return false;
}

static cc_bool LocalPlayer_HandleJump(int key, struct InputDevice* device) {
	struct LocalPlayer* p = &LocalPlayer_Instances[device->mappedIndex];
	struct HacksComp* hacks     = &p->Hacks;
	struct PhysicsComp* physics = &p->Physics;
	int maxJumps;
	if (Gui.InputGrab) return false;
	physics->Jumping = true;

	if (!p->Base.OnGround && !(hacks->Flying || hacks->Noclip)) {
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


static cc_bool LocalPlayer_TriggerHalfSpeed(int key, struct InputDevice* device) {
	struct HacksComp* hacks = &LocalPlayer_Instances[device->mappedIndex].Hacks;
	cc_bool touch = device->type == INPUT_DEVICE_TOUCH;
	if (Gui.InputGrab) return false;

	hacks->HalfSpeeding = (!touch || !hacks->HalfSpeeding) && hacks->Enabled;
	return true;
}

static cc_bool LocalPlayer_TriggerSpeed(int key, struct InputDevice* device) {
	struct HacksComp* hacks = &LocalPlayer_Instances[device->mappedIndex].Hacks;
	cc_bool touch = device->type == INPUT_DEVICE_TOUCH;
	if (Gui.InputGrab) return false;

	hacks->Speeding = (!touch || !hacks->Speeding) && hacks->Enabled;
	return true;
}

static void LocalPlayer_ReleaseHalfSpeed(int key, struct InputDevice* device) {
	struct HacksComp* hacks = &LocalPlayer_Instances[device->mappedIndex].Hacks;
	if (device->type != INPUT_DEVICE_TOUCH) hacks->HalfSpeeding = false;
}

static void LocalPlayer_ReleaseSpeed(int key, struct InputDevice* device) {
	struct HacksComp* hacks = &LocalPlayer_Instances[device->mappedIndex].Hacks;
	if (device->type != INPUT_DEVICE_TOUCH) hacks->Speeding = false;
}


static cc_bool LocalPlayer_TriggerFlyUp(int key, struct InputDevice* device) {
	struct HacksComp* hacks = &LocalPlayer_Instances[device->mappedIndex].Hacks;
	if (Gui.InputGrab) return false;
	
	hacks->FlyingUp = true;
	return hacks->CanFly && hacks->Enabled;
}

static cc_bool LocalPlayer_TriggerFlyDown(int key, struct InputDevice* device) {
	struct HacksComp* hacks = &LocalPlayer_Instances[device->mappedIndex].Hacks;
	if (Gui.InputGrab) return false;
	
	hacks->FlyingDown = true;
	return hacks->CanFly && hacks->Enabled;
}

static void LocalPlayer_ReleaseFlyUp(int key, struct InputDevice* device) {
	LocalPlayer_Instances[device->mappedIndex].Hacks.FlyingUp   = false;
}

static void LocalPlayer_ReleaseFlyDown(int key, struct InputDevice* device) {
	LocalPlayer_Instances[device->mappedIndex].Hacks.FlyingDown = false;
}

static void LocalPlayer_ReleaseJump(int key, struct InputDevice* device) {
	LocalPlayer_Instances[device->mappedIndex].Physics.Jumping = false;
}

static void LocalPlayer_ReleaseNoclip(int key, struct InputDevice* device) {
	LocalPlayer_Instances[device->mappedIndex].Hacks._noclipping = false;
}

static void LocalPlayer_HookBinds(void) {
	Bind_OnTriggered[BIND_RESPAWN]   = LocalPlayer_HandleRespawn;
	Bind_OnTriggered[BIND_SET_SPAWN] = LocalPlayer_HandleSetSpawn;
	Bind_OnTriggered[BIND_FLY]       = LocalPlayer_HandleFly;
	Bind_OnTriggered[BIND_NOCLIP]    = LocalPlayer_HandleNoclip;
	Bind_OnTriggered[BIND_JUMP]      = LocalPlayer_HandleJump;

	Bind_OnTriggered[BIND_HALF_SPEED] = LocalPlayer_TriggerHalfSpeed;
	Bind_OnTriggered[BIND_SPEED]      = LocalPlayer_TriggerSpeed;
	Bind_OnReleased[BIND_HALF_SPEED]  = LocalPlayer_ReleaseHalfSpeed;
	Bind_OnReleased[BIND_SPEED]       = LocalPlayer_ReleaseSpeed;

	Bind_OnTriggered[BIND_FLY_UP]   = LocalPlayer_TriggerFlyUp;
	Bind_OnTriggered[BIND_FLY_DOWN] = LocalPlayer_TriggerFlyDown;
	Bind_OnReleased[BIND_FLY_UP]    = LocalPlayer_ReleaseFlyUp;
	Bind_OnReleased[BIND_FLY_DOWN]  = LocalPlayer_ReleaseFlyDown;

	Bind_OnReleased[BIND_JUMP]    = LocalPlayer_ReleaseJump;
	Bind_OnReleased[BIND_NOCLIP]  = LocalPlayer_ReleaseNoclip;
}

cc_bool LocalPlayer_CheckCanZoom(struct LocalPlayer* p) {
	if (p->Hacks.CanFly) return true;

	if (!p->_warnedZoom) {
		p->_warnedZoom = true;
		if (hackPermMsgs) Chat_AddRaw("&cCannot zoom camera out as flying is currently disabled");
	}
	return false;
}

void LocalPlayers_MoveToSpawn(struct LocationUpdate* update) {
	struct LocalPlayer* p;
	int i;
	
	for (i = 0; i < Game_NumStates; i++)
	{
		p = &LocalPlayer_Instances[i];
		p->Base.VTABLE->SetLocation(&p->Base, update);
		
		if (update->flags & LU_HAS_POS)   p->Spawn      = update->pos;
		if (update->flags & LU_HAS_YAW)   p->SpawnYaw   = update->yaw;
		if (update->flags & LU_HAS_PITCH) p->SpawnPitch = update->pitch;
	}
	
	/* TODO: This needs to be before new map... */
	Camera.CurrentPos = Camera.Active->GetPosition(0.0f);
}

void LocalPlayer_CalcDefaultSpawn(struct LocalPlayer* p, struct LocationUpdate* update) {
	float x = (World.Width  / 2) + 0.5f;
	float z = (World.Length / 2) + 0.5f;

	update->flags = LU_HAS_POS | LU_HAS_YAW | LU_HAS_PITCH;
	update->pos   = Respawn_FindSpawnPosition(x, z, p->Base.Size);
	update->yaw   = 0.0f;
	update->pitch = 0.0f;
}


/*########################################################################################################################*
*-------------------------------------------------------NetPlayer---------------------------------------------------------*
*#########################################################################################################################*/
struct NetPlayer NetPlayers_List[MAX_NET_PLAYERS];

static void NetPlayer_SetLocation(struct Entity* e, struct LocationUpdate* update) {
	struct NetPlayer* p = (struct NetPlayer*)e;
	NetInterpComp_SetLocation(&p->Interp, update, e);
}

static void NetPlayer_Tick(struct Entity* e, float delta) {
	struct NetPlayer* p = (struct NetPlayer*)e;
	NetInterpComp_AdvanceState(&p->Interp, e);

	Entity_CheckSkin(e);
	AnimatedComp_Update(e, e->prev.pos, e->next.pos, delta);
}

static void NetPlayer_RenderModel(struct Entity* e, float delta, float t) {
	Vec3_Lerp(&e->Position, &e->prev.pos, &e->next.pos, t);
	Entity_LerpAngles(e, t);

	AnimatedComp_GetCurrent(e, t);
	e->ShouldRender = Model_ShouldRender(e);
	/* Original classic only shows players up to 64 blocks away */
	if (Game_ClassicMode) e->ShouldRender &= Model_RenderDistance(e) <= 64 * 64;

	if (e->ShouldRender) Model_Render(e->Model, e);
}

static cc_bool NetPlayer_ShouldRenderName(struct Entity* e) {
	float distance;
	int threshold;
	if (!e->ShouldRender) return false;

	distance  = Model_RenderDistance(e);
	threshold = Entities.NamesMode == NAME_MODE_ALL_UNSCALED ? 8192 * 8192 : 32 * 32;
	return distance <= (float)threshold;
}

static const struct EntityVTABLE netPlayer_VTABLE = {
	NetPlayer_Tick,        Player_Despawn,       NetPlayer_SetLocation, Entity_GetColor,
	NetPlayer_RenderModel, NetPlayer_ShouldRenderName
};
void NetPlayer_Init(struct NetPlayer* p) {
	Mem_Set(p, 0, sizeof(struct NetPlayer));
	Entity_Init(&p->Base);
	p->Base.Flags |= ENTITY_FLAG_CLASSIC_ADJUST;
	p->Base.VTABLE = &netPlayer_VTABLE;
}


/*########################################################################################################################*
*---------------------------------------------------Entities component----------------------------------------------------*
*#########################################################################################################################*/
static void Entities_Init(void) {
	int i;
	Event_Register_(&GfxEvents.ContextLost, NULL, Entities_ContextLost);

	Entities.NamesMode = Options_GetEnum(OPT_NAMES_MODE, NAME_MODE_HOVERED,
		NameMode_Names, Array_Elems(NameMode_Names));
	if (Game_ClassicMode) Entities.NamesMode = NAME_MODE_HOVERED;

	Entities.ShadowsMode = Options_GetEnum(OPT_ENTITY_SHADOW, SHADOW_MODE_NONE,
		ShadowMode_Names, Array_Elems(ShadowMode_Names));
	if (Game_ClassicMode) Entities.ShadowsMode = SHADOW_MODE_NONE;

	for (i = 0; i < Game_NumStates; i++)
	{
		LocalPlayer_Init(&LocalPlayer_Instances[i], i);
		Entities.List[MAX_NET_PLAYERS + i] = &LocalPlayer_Instances[i].Base;
	}
	for (; i < MAX_LOCAL_PLAYERS; i++)
	{
		Entities.List[MAX_NET_PLAYERS + i] = NULL;
	}
	Entities.CurPlayer = &LocalPlayer_Instances[0];
	LocalPlayer_HookBinds();
}

static void Entities_Free(void) {
	int i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++)
	{
		Entities_Remove(i);
	}
	sources_head = NULL;
}

struct IGameComponent Entities_Component = {
	Entities_Init,  /* Init  */
	Entities_Free,  /* Free  */
	LocalPlayers_Reset,    /* Reset */
	LocalPlayers_OnNewMap, /* OnNewMap */
};
