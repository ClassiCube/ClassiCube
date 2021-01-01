#ifndef CC_ENTITY_H
#define CC_ENTITY_H
#include "EntityComponents.h"
#include "Physics.h"
#include "Constants.h"
#include "PackedCol.h"
#include "String.h"
/* Represents an in-game entity.
   Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/
struct Model;
struct IGameComponent;
struct ScheduledTask;
extern struct IGameComponent TabList_Component;
extern struct IGameComponent Entities_Component;

/* Offset used to avoid floating point roundoff errors. */
#define ENTITY_ADJUSTMENT 0.001f
#define ENTITIES_MAX_COUNT 256
#define ENTITIES_SELF_ID 255

enum NameMode {
	NAME_MODE_NONE, NAME_MODE_HOVERED, NAME_MODE_ALL, NAME_MODE_ALL_HOVERED, NAME_MODE_ALL_UNSCALED, NAME_MODE_COUNT
};
extern const char* const NameMode_Names[NAME_MODE_COUNT];

enum ShadowMode {
	SHADOW_MODE_NONE, SHADOW_MODE_SNAP_TO_BLOCK, SHADOW_MODE_CIRCLE, SHADOW_MODE_CIRCLE_ALL, SHADOW_MODE_COUNT
};
extern const char* const ShadowMode_Names[SHADOW_MODE_COUNT];

enum EntityType { ENTITY_TYPE_NONE, ENTITY_TYPE_PLAYER };

#define LOCATIONUPDATE_POS   0x01
#define LOCATIONUPDATE_PITCH 0x02
#define LOCATIONUPDATE_YAW   0x04
#define LOCATIONUPDATE_ROTX  0x08
#define LOCATIONUPDATE_ROTZ  0x10
/* Represents a location update for an entity. Can be a relative position, full position, and/or an orientation update. */
struct LocationUpdate {
	Vec3 Pos;
	float Pitch, Yaw, RotX, RotZ;
	cc_uint8 Flags;
	cc_bool RelativePos;
};

/* Clamps the given angle so it lies between [0, 360). */
float LocationUpdate_Clamp(float degrees);
/* Makes a location update only containing yaw and pitch. */
void LocationUpdate_MakeOri(struct LocationUpdate* update, float yaw, float pitch);
/* Makes a location update only containing position */
void LocationUpdate_MakePos(struct LocationUpdate* update, Vec3 pos, cc_bool rel);
/* Makes a location update containing position, yaw and pitch. */
void LocationUpdate_MakePosAndOri(struct LocationUpdate* update, Vec3 pos, float yaw, float pitch, cc_bool rel);

struct Entity;
struct EntityVTABLE {
	void (*Tick)(struct Entity* e, double delta);
	void (*Despawn)(struct Entity* e);
	void (*SetLocation)(struct Entity* e, struct LocationUpdate* update, cc_bool interpolate);
	PackedCol (*GetCol)(struct Entity* e);
	void (*RenderModel)(struct Entity* e, double deltaTime, float t);
	void (*RenderName)(struct Entity* e);
};

/* Skin is still being downloaded asynchronously */
#define SKIN_FETCH_DOWNLOADING 1
/* Skin was downloaded or copied from another entity with the same skin. */
#define SKIN_FETCH_COMPLETED   2

/* Contains a model, along with position, velocity, and rotation. May also contain other fields and properties. */
struct Entity {
	const struct EntityVTABLE* VTABLE;
	Vec3 Position;
	/* NOTE: Do NOT change order of yaw/pitch, this will break models in plugins */
	float Pitch, Yaw, RotX, RotY, RotZ;
	Vec3 Velocity;

	struct Model* Model;
	BlockID ModelBlock; /* BlockID, if model name was originally a valid block. */
	cc_bool ModelRestrictedScale; /* true to restrict model scale (needed for local player, giant model collisions are too costly) */
	cc_bool ShouldRender;
	struct AABB ModelAABB;
	Vec3 ModelScale, Size;
	int _skinReqID;
	
	cc_uint8 SkinType;
	cc_uint8 SkinFetchState;
	cc_bool NoShade, OnGround;
	GfxResourceID TextureId, MobTextureId;
	float uScale, vScale;
	struct Matrix Transform;

	struct AnimatedComp Anim;
	char SkinRaw[STRING_SIZE];
	char NameRaw[STRING_SIZE];
	struct Texture NameTex;
};
typedef cc_bool (*Entity_TouchesCondition)(BlockID block);

/* Initialises non-zero fields of the given entity. */
void Entity_Init(struct Entity* e);
/* Gets the position of the eye of the given entity's model. */
Vec3 Entity_GetEyePosition(struct Entity* e);
/* Returns the height of the eye of the given entity's model. */
/* (i.e. distance to eye from feet/base of the model) */
float Entity_GetEyeHeight(struct Entity* e);
/* Calculates the transformation matrix applied when rendering the given entity. */
CC_API void Entity_GetTransform(struct Entity* e, Vec3 pos, Vec3 scale, struct Matrix* m);
void Entity_GetPickingBounds(struct Entity* e, struct AABB* bb);
/* Gets the current collision bounds of the given entity. */
void Entity_GetBounds(struct Entity* e, struct AABB* bb);
/* Sets the model of the entity. (i.e its appearance) */
CC_API void Entity_SetModel(struct Entity* e, const cc_string* model);
/* Updates cached Size and ModelAABB of the given entity. */
/* NOTE: Only needed when manually changing Model or ModelScale. */
/* Entity_SetModel already calls this method. */
void Entity_UpdateModelBounds(struct Entity* e);

/* Whether the given entity is touching any blocks meeting the given condition .*/
CC_API cc_bool Entity_TouchesAny(struct AABB* bb, Entity_TouchesCondition cond);
cc_bool Entity_TouchesAnyRope(struct Entity* e);
cc_bool Entity_TouchesAnyLava(struct Entity* e);
cc_bool Entity_TouchesAnyWater(struct Entity* e);

/* Sets the nametag above the given entity's head */
void Entity_SetName(struct Entity* e, const cc_string* name);
/* Sets the skin name of the given entity. */
void Entity_SetSkin(struct Entity* e, const cc_string* skin);

/* Global data for all entities */
/* (Actual entities may point to NetPlayers_List or elsewhere) */
CC_VAR extern struct _EntitiesData {
	struct Entity* List[ENTITIES_MAX_COUNT];
	cc_uint8 NamesMode, ShadowsMode;
} Entities;

/* Ticks all entities. */
void Entities_Tick(struct ScheduledTask* task);
/* Renders all entities. */
void Entities_RenderModels(double delta, float t);
/* Renders the name tags of entities, depending on Entities.NamesMode. */
void Entities_RenderNames(void);
/* Renders hovered entity name tags. (these appears through blocks) */
void Entities_RenderHoveredNames(void);
/* Removes the given entity, raising EntityEvents.Removed event. */
void Entities_Remove(EntityID id);
/* Gets the ID of the closest entity to the given entity. */
EntityID Entities_GetClosest(struct Entity* src);
/* Draws shadows under entities, depending on Entities.ShadowsMode */
void Entities_DrawShadows(void);

#define TABLIST_MAX_NAMES 256
/* Data for all entries in tab list */
CC_VAR extern struct _TabListData {
	/* Buffer indices for player/list/group names. */
	/* Use TabList_UNSAFE_GetPlayer/List/Group to get these names. */
	/* NOTE: An Offset of 0 means the entry is unused. */
	cc_uint16 NameOffsets[TABLIST_MAX_NAMES];
	/* Position/Order of this entry within the group. */
	cc_uint8  GroupRanks[TABLIST_MAX_NAMES];
	struct StringsBuffer _buffer;
} TabList;

/* Removes the tab list entry with the given ID, raising TabListEvents.Removed event. */
CC_API void TabList_Remove(EntityID id);
/* Sets the data for the tab list entry with the given id. */
/* Raises TabListEvents.Changed if replacing, TabListEvents.Added if a new entry. */
CC_API void TabList_Set(EntityID id, const cc_string* player, const cc_string* list, const cc_string* group, cc_uint8 rank);

/* Raw unformatted name (for Tab name auto complete) */
#define TabList_UNSAFE_GetPlayer(id) StringsBuffer_UNSAFE_Get(&TabList._buffer, TabList.NameOffsets[id] - 3);
/* Formatted name for display in tab list. */
#define TabList_UNSAFE_GetList(id)   StringsBuffer_UNSAFE_Get(&TabList._buffer, TabList.NameOffsets[id] - 2);
/* Name of the group this entry is in (e.g. rank name, map name) */
#define TabList_UNSAFE_GetGroup(id)  StringsBuffer_UNSAFE_Get(&TabList._buffer, TabList.NameOffsets[id] - 1);

/* Represents another entity in multiplayer */
struct NetPlayer {
	struct Entity Base;
	struct NetInterpComp Interp;
};
CC_API void NetPlayer_Init(struct NetPlayer* player);
extern struct NetPlayer NetPlayers_List[ENTITIES_SELF_ID];

struct LocalPlayerInput;
struct LocalPlayerInput {
	void (*GetMovement)(float* xMoving, float* zMoving);
	struct LocalPlayerInput* next;
};

/* Represents the user/player's own entity. */
struct LocalPlayer {
	struct Entity Base;
	Vec3 Spawn, OldVelocity;
	float SpawnYaw, SpawnPitch, ReachDistance;
	struct HacksComp Hacks;
	struct TiltComp Tilt;
	struct InterpComp Interp;
	struct CollisionsComp Collisions;
	struct PhysicsComp Physics;
	cc_bool _warnedRespawn, _warnedFly, _warnedNoclip, _warnedZoom;
	struct LocalPlayerInput input;
};

extern struct LocalPlayer LocalPlayer_Instance;
/* Returns how high (in blocks) the player can jump. */
float LocalPlayer_JumpHeight(void);
/* Interpolates current position and orientation between Interp.Prev and Interp.Next */
void LocalPlayer_SetInterpPosition(float t);
void LocalPlayer_ResetJumpVelocity(void);
cc_bool LocalPlayer_CheckCanZoom(void);
/* Moves local player back to spawn point. */
void LocalPlayer_MoveToSpawn(void);

cc_bool LocalPlayer_HandleRespawn(void);
cc_bool LocalPlayer_HandleSetSpawn(void);
cc_bool LocalPlayer_HandleFly(void);
cc_bool LocalPlayer_HandleNoclip(void);
cc_bool LocalPlayer_HandleJump(void);
#endif
