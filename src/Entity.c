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
#include "Gui.h"
#include "Stream.h"
#include "Bitmap.h"
#include "Logger.h"
#include "Options.h"
#include "Errors.h"
#include "Utils.h"
#include "EntityRenderers.h"

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
		flags = e == &LocalPlayer_Instance.Base ? HTTP_FLAG_NOCACHE : 0;

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

	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
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

void Entities_RenderModels(double delta, float t) {
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

void Entities_Remove(EntityID id) {
	struct Entity* e = Entities.List[id];
	if (!e) return;

	Event_RaiseInt(&EntityEvents.Removed, id);
	e->VTABLE->Despawn(e);
	Entities.List[id] = NULL;

	/* TODO: Move to EntityEvents.Removed callback instead */
	if (TabList_EntityLinked_Get(id)) {
		TabList_Remove(id);
		TabList_EntityLinked_Reset(id);
	}
}

EntityID Entities_GetClosest(struct Entity* src) {
	Vec3 eyePos = Entity_GetEyePosition(src);
	Vec3 dir = Vec3_GetDirVector(src->Yaw * MATH_DEG2RAD, src->Pitch * MATH_DEG2RAD);
	float closestDist = MATH_POS_INF;
	EntityID targetId = ENTITIES_SELF_ID;

	float t0, t1;
	int i;

	for (i = 0; i < ENTITIES_SELF_ID; i++) { /* because we don't want to pick against local player */
		struct Entity* entity = Entities.List[i];
		if (!entity) continue;

		if (Intersection_RayIntersectsRotatedBox(eyePos, dir, entity, &t0, &t1) && t0 < closestDist) {
			closestDist = t0;
			targetId = (EntityID)i;
		}
	}
	return targetId;
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
struct LocalPlayer LocalPlayer_Instance;
static cc_bool hackPermMsgs;
float LocalPlayer_JumpHeight(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	return (float)PhysicsComp_CalcMaxHeight(p->Physics.JumpVel);
}

void LocalPlayer_SetInterpPosition(float t) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (!(p->Hacks.WOMStyleHacks && p->Hacks.Noclip)) {
		Vec3_Lerp(&p->Base.Position, &p->Base.prev.pos, &p->Base.next.pos, t);
	}
	Entity_LerpAngles(&p->Base, t);
}

static void LocalPlayer_HandleInput(float* xMoving, float* zMoving) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct HacksComp* hacks = &p->Hacks;
	struct LocalPlayerInput* input;

	if (Gui.InputGrab) {
		/* TODO: Don't always turn these off anytime a screen is opened, only do it on InputUp */
		p->Physics.Jumping = false; hacks->FlyingUp = false; hacks->FlyingDown = false;
		return;
	}

	/* keyboard input, touch, joystick, etc */
	for (input = &p->input; input; input = input->next) {
		input->GetMovement(xMoving, zMoving);
	}
	*xMoving *= 0.98f; 
	*zMoving *= 0.98f;

	p->Physics.Jumping = KeyBind_IsPressed(KEYBIND_JUMP);
	hacks->FlyingUp    = KeyBind_IsPressed(KEYBIND_FLY_UP);
	hacks->FlyingDown  = KeyBind_IsPressed(KEYBIND_FLY_DOWN);

	if (hacks->WOMStyleHacks && hacks->Enabled && hacks->CanNoclip) {
		if (hacks->Noclip) {
			/* need a { } block because it's a macro */
			Vec3_Set(p->Base.Velocity, 0,0,0);
		}
		HacksComp_SetNoclip(hacks, KeyBind_IsPressed(KEYBIND_NOCLIP));
	}
}

static void LocalPlayer_InputSet(int key, cc_bool pressed) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;

	if (pressed && !hacks->Enabled) return;
	if (KeyBind_Claims(KEYBIND_SPEED, key))      hacks->Speeding     = pressed;
	if (KeyBind_Claims(KEYBIND_HALF_SPEED, key)) hacks->HalfSpeeding = pressed;
}

static void LocalPlayer_InputDown(void* obj, int key, cc_bool was) {
	/* e.g. pressing Shift in chat input shouldn't turn on speeding */
	if (!Gui.InputGrab) LocalPlayer_InputSet(key, true);
}

static void LocalPlayer_InputUp(void* obj, int key) {
	LocalPlayer_InputSet(key, false);
}

static void LocalPlayer_SetLocation(struct Entity* e, struct LocationUpdate* update) {
	struct LocalPlayer* p = (struct LocalPlayer*)e;
	LocalInterpComp_SetLocation(&p->Interp, update);
}

static void LocalPlayer_Tick(struct Entity* e, double delta) {
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
	LocalPlayer_HandleInput(&xMoving, &zMoving);
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
	TiltComp_Update(&p->Tilt, delta);

	Entity_CheckSkin(&p->Base);
	SoundComp_Tick(wasOnGround);
}

static void LocalPlayer_RenderModel(struct Entity* e, double deltaTime, float t) {
	struct LocalPlayer* p = (struct LocalPlayer*)e;
	AnimatedComp_GetCurrent(e, t);
	TiltComp_GetCurrent(&p->Tilt, t);

	if (!Camera.Active->isThirdPerson) return;
	Model_Render(e->Model, e);
}

static cc_bool LocalPlayer_ShouldRenderName(struct Entity* e) {
	return Camera.Active->isThirdPerson;
}

static void LocalPlayer_CheckJumpVelocity(void* obj) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (!HacksComp_CanJumpHigher(&p->Hacks)) {
		p->Physics.JumpVel = p->Physics.ServerJumpVel;
	}
}

static void LocalPlayer_GetMovement(float* xMoving, float* zMoving) {
	if (KeyBind_IsPressed(KEYBIND_FORWARD)) *zMoving -= 1;
	if (KeyBind_IsPressed(KEYBIND_BACK))    *zMoving += 1;
	if (KeyBind_IsPressed(KEYBIND_LEFT))    *xMoving -= 1;
	if (KeyBind_IsPressed(KEYBIND_RIGHT))   *xMoving += 1;

	/* TODO: Move to separate LocalPlayerInputSource */
	if (!Input.JoystickMovement) return;
	*xMoving = Math_CosF(Input.JoystickAngle);
	*zMoving = Math_SinF(Input.JoystickAngle);
}

static const struct EntityVTABLE localPlayer_VTABLE = {
	LocalPlayer_Tick,        Player_Despawn,         LocalPlayer_SetLocation, Entity_GetColor,
	LocalPlayer_RenderModel, LocalPlayer_ShouldRenderName
};
static void LocalPlayer_Init(void) {
	struct LocalPlayer* p   = &LocalPlayer_Instance;
	struct HacksComp* hacks = &p->Hacks;

	Entity_Init(&p->Base);
	Entity_SetName(&p->Base, &Game_Username);
	Entity_SetSkin(&p->Base, &Game_Username);
	Event_Register_(&UserEvents.HackPermsChanged, NULL, LocalPlayer_CheckJumpVelocity);

	p->input.GetMovement = LocalPlayer_GetMovement;
	p->Collisions.Entity = &p->Base;
	HacksComp_Init(hacks);
	PhysicsComp_Init(&p->Physics, &p->Base);
	TiltComp_Init(&p->Tilt);

	p->Base.Flags |= ENTITY_FLAG_MODEL_RESTRICTED_SCALE;
	p->ReachDistance = 5.0f;
	p->Physics.Hacks = &p->Hacks;
	p->Physics.Collisions = &p->Collisions;
	p->Base.VTABLE   = &localPlayer_VTABLE;

	hacks->Enabled = !Game_PureClassic && Options_GetBool(OPT_HACKS_ENABLED, true);
	/* p->Base.Health = 20; TODO: survival mode stuff */
	if (Game_ClassicMode) return;

	hacks->SpeedMultiplier = Options_GetFloat(OPT_SPEED_FACTOR, 0.1f, 50.0f, 10.0f);
	hacks->PushbackPlacing = Options_GetBool(OPT_PUSHBACK_PLACING, false);
	hacks->NoclipSlide     = Options_GetBool(OPT_NOCLIP_SLIDE,     false);
	hacks->WOMStyleHacks   = Options_GetBool(OPT_WOM_STYLE_HACKS,  false);
	hacks->FullBlockStep   = Options_GetBool(OPT_FULL_BLOCK_STEP,  false);
	p->Physics.UserJumpVel = Options_GetFloat(OPT_JUMP_VELOCITY, 0.0f, 52.0f, 0.42f);
	p->Physics.JumpVel     = p->Physics.UserJumpVel;
	hackPermMsgs           = Options_GetBool(OPT_HACK_PERM_MSGS, true);
}

void LocalPlayer_ResetJumpVelocity(void) {
	struct LocalPlayer* p  = &LocalPlayer_Instance;
	cc_bool higher = HacksComp_CanJumpHigher(&p->Hacks);

	p->Physics.JumpVel       = higher ? p->Physics.UserJumpVel : 0.42f;
	p->Physics.ServerJumpVel = p->Physics.JumpVel;
}

static void LocalPlayer_Reset(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	p->ReachDistance = 5.0f;
	Vec3_Set(p->Base.Velocity, 0,0,0);
	LocalPlayer_ResetJumpVelocity();
}

static void LocalPlayer_OnNewMap(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	Vec3_Set(p->Base.Velocity, 0,0,0);
	Vec3_Set(p->OldVelocity,   0,0,0);

	p->_warnedRespawn = false;
	p->_warnedFly     = false;
	p->_warnedNoclip  = false;
	p->_warnedZoom    = false;
}

static cc_bool LocalPlayer_IsSolidCollide(BlockID b) { return Blocks.Collide[b] == COLLIDE_SOLID; }
static void LocalPlayer_DoRespawn(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
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

cc_bool LocalPlayer_HandleRespawn(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (p->Hacks.CanRespawn) {
		LocalPlayer_DoRespawn();
		return true;
	} else if (!p->_warnedRespawn) {
		p->_warnedRespawn = true;
		if (hackPermMsgs) Chat_AddRaw("&cRespawning is currently disabled");
	}
	return false;
}

cc_bool LocalPlayer_HandleSetSpawn(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (p->Hacks.CanRespawn) {

		if (!p->Hacks.CanNoclip && !p->Base.OnGround) {
			Chat_AddRaw("&cCannot set spawn midair when noclip is disabled");
			return false;
		}

		/* Spawn is normally centered to match vanilla Minecraft classic */
		if (!p->Hacks.CanNoclip) {
			p->Spawn   = p->Base.Position;
		} else {
			p->Spawn.x = Math_Floor(p->Base.Position.x) + 0.5f;
			p->Spawn.y = p->Base.Position.y;
			p->Spawn.z = Math_Floor(p->Base.Position.z) + 0.5f;
		}
		
		p->SpawnYaw   = p->Base.Yaw;
		p->SpawnPitch = p->Base.Pitch;
	}
	return LocalPlayer_HandleRespawn();
}

cc_bool LocalPlayer_HandleFly(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (p->Hacks.CanFly && p->Hacks.Enabled) {
		HacksComp_SetFlying(&p->Hacks, !p->Hacks.Flying);
		return true;
	} else if (!p->_warnedFly) {
		p->_warnedFly = true;
		if (hackPermMsgs) Chat_AddRaw("&cFlying is currently disabled");
	}
	return false;
}

cc_bool LocalPlayer_HandleNoclip(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
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

cc_bool LocalPlayer_HandleJump(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct HacksComp* hacks     = &p->Hacks;
	struct PhysicsComp* physics = &p->Physics;
	int maxJumps;

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

cc_bool LocalPlayer_CheckCanZoom(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (p->Hacks.CanFly) return true;

	if (!p->_warnedZoom) {
		p->_warnedZoom = true;
		if (hackPermMsgs) Chat_AddRaw("&cCannot zoom camera out as flying is currently disabled");
	}
	return false;
}

void LocalPlayer_MoveToSpawn(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct LocationUpdate update;

	update.flags = LU_HAS_POS | LU_HAS_YAW | LU_HAS_PITCH;
	update.pos   = p->Spawn;
	update.yaw   = p->SpawnYaw;
	update.pitch = p->SpawnPitch;

	p->Base.VTABLE->SetLocation(&p->Base, &update);
	/* TODO: This needs to be before new map... */
	Camera.CurrentPos = Camera.Active->GetPosition(0.0f);
}

void LocalPlayer_CalcDefaultSpawn(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	float x = (World.Width  / 2) + 0.5f; 
	float z = (World.Length / 2) + 0.5f;

	p->Spawn      = Respawn_FindSpawnPosition(x, z, p->Base.Size);
	p->SpawnYaw   = 0.0f;
	p->SpawnPitch = 0.0f;
}


/*########################################################################################################################*
*-------------------------------------------------------NetPlayer---------------------------------------------------------*
*#########################################################################################################################*/
struct NetPlayer NetPlayers_List[ENTITIES_SELF_ID];

static void NetPlayer_SetLocation(struct Entity* e, struct LocationUpdate* update) {
	struct NetPlayer* p = (struct NetPlayer*)e;
	NetInterpComp_SetLocation(&p->Interp, update, e);
}

static void NetPlayer_Tick(struct Entity* e, double delta) {
	struct NetPlayer* p = (struct NetPlayer*)e;
	NetInterpComp_AdvanceState(&p->Interp, e);

	Entity_CheckSkin(e);
	AnimatedComp_Update(e, e->prev.pos, e->next.pos, delta);
}

static void NetPlayer_RenderModel(struct Entity* e, double deltaTime, float t) {
	Vec3_Lerp(&e->Position, &e->prev.pos, &e->next.pos, t);
	Entity_LerpAngles(e, t);

	AnimatedComp_GetCurrent(e, t);
	e->ShouldRender = Model_ShouldRender(e);
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
	p->Base.VTABLE = &netPlayer_VTABLE;
}


/*########################################################################################################################*
*---------------------------------------------------Entities component----------------------------------------------------*
*#########################################################################################################################*/
static void Entities_Init(void) {
	Event_Register_(&GfxEvents.ContextLost, NULL, Entities_ContextLost);
	Event_Register_(&InputEvents.Down,      NULL, LocalPlayer_InputDown);
	Event_Register_(&InputEvents.Up,        NULL, LocalPlayer_InputUp);

	Entities.NamesMode = Options_GetEnum(OPT_NAMES_MODE, NAME_MODE_HOVERED,
		NameMode_Names, Array_Elems(NameMode_Names));
	if (Game_ClassicMode) Entities.NamesMode = NAME_MODE_HOVERED;

	Entities.ShadowsMode = Options_GetEnum(OPT_ENTITY_SHADOW, SHADOW_MODE_NONE,
		ShadowMode_Names, Array_Elems(ShadowMode_Names));
	if (Game_ClassicMode) Entities.ShadowsMode = SHADOW_MODE_NONE;

	Entities.List[ENTITIES_SELF_ID] = &LocalPlayer_Instance.Base;
	LocalPlayer_Init();
}

static void Entities_Free(void) {
	int i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) 
	{
		Entities_Remove((EntityID)i);
	}
}

struct IGameComponent Entities_Component = {
	Entities_Init,  /* Init  */
	Entities_Free,  /* Free  */
	LocalPlayer_Reset,    /* Reset */
	LocalPlayer_OnNewMap, /* OnNewMap */
};
