#include "Mobs.h"
#include "Survival.h"
#include "Game.h"
#include "World.h"
#include "Block.h"
#include "BlockID.h"
#include "Entity.h"
#include "EntityComponents.h"
#include "Event.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Chat.h"
#include "Graphics.h"
#include "Bitmap.h"
#include "Lighting.h"
#include "Model.h"
#include "Inventory.h"
#include "Platform.h"
#include "Server.h"
#include "String_.h"

struct MobEntity MobEntities[MAX_MOBS];

/* Shared HacksComp for all mobs (no flying, no noclip, etc.) */
static struct HacksComp mob_hacks;

/* Mob skin textures (solid colored) */
static GfxResourceID mob_skins[MOB_TYPE_COUNT];

/* Simple RNG */
static cc_uint32 mob_rng_state = 12345;
static cc_uint32 MobRng_Next(void) {
    mob_rng_state ^= mob_rng_state << 13;
    mob_rng_state ^= mob_rng_state >> 17;
    mob_rng_state ^= mob_rng_state << 5;
    return mob_rng_state;
}
static float MobRng_Float(void) { return (float)(MobRng_Next() & 0x7FFF) / 32767.0f; }
static float MobRng_Range(float min, float max) { return min + MobRng_Float() * (max - min); }

/*########################################################################################################################*
*--------------------------------------------------Mob Texture Creation----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID CreateSolidColorTex(int r, int g, int b) {
    struct Bitmap bmp;
    BitmapCol data[64]; /* 8x8 texture */
    BitmapCol col = BitmapCol_Make(r, g, b, 255);
    int i;
    for (i = 0; i < 64; i++) data[i] = col;
    Bitmap_Init(bmp, 8, 8, data);
    return Gfx_CreateTexture(&bmp, TEXTURE_FLAG_MANAGED, false);
}

static void CreateMobTextures(void) {
    mob_skins[MOB_TYPE_PIG]      = CreateSolidColorTex(0xF0, 0x84, 0x7A); /* pig pink */
    mob_skins[MOB_TYPE_SHEEP]    = CreateSolidColorTex(0xE0, 0xE0, 0xE0); /* sheep white */
    mob_skins[MOB_TYPE_ZOMBIE]   = CreateSolidColorTex(0x00, 0x99, 0x00); /* zombie green */
    mob_skins[MOB_TYPE_SKELETON] = CreateSolidColorTex(0xD0, 0xD0, 0xD0); /* skeleton gray */
    mob_skins[MOB_TYPE_SPIDER]   = CreateSolidColorTex(0x40, 0x20, 0x10); /* spider brown */
    mob_skins[MOB_TYPE_CREEPER]  = CreateSolidColorTex(0x30, 0xA0, 0x30); /* creeper green */
}

static void DestroyMobTextures(void) {
    int i;
    for (i = 0; i < MOB_TYPE_COUNT; i++) {
        Gfx_DeleteTexture(&mob_skins[i]);
    }
}

/*########################################################################################################################*
*------------------------------------------------------Mob Properties------------------------------------------------------*
*#########################################################################################################################*/
static int MobMaxHealth(int type) {
    switch (type) {
    case MOB_TYPE_PIG:      return MOB_HP_PIG;
    case MOB_TYPE_SHEEP:    return MOB_HP_SHEEP;
    case MOB_TYPE_ZOMBIE:   return MOB_HP_ZOMBIE;
    case MOB_TYPE_SKELETON: return MOB_HP_SKELETON;
    case MOB_TYPE_SPIDER:   return MOB_HP_SPIDER;
    case MOB_TYPE_CREEPER:  return MOB_HP_CREEPER;
    default: return 10;
    }
}

static int MobScore(int type) {
    switch (type) {
    case MOB_TYPE_PIG:      return MOB_SCORE_PIG;
    case MOB_TYPE_SHEEP:    return MOB_SCORE_SHEEP;
    case MOB_TYPE_ZOMBIE:   return MOB_SCORE_ZOMBIE;
    case MOB_TYPE_SKELETON: return MOB_SCORE_SKELETON;
    case MOB_TYPE_SPIDER:   return MOB_SCORE_SPIDER;
    case MOB_TYPE_CREEPER:  return MOB_SCORE_CREEPER;
    default: return 10;
    }
}

/* Walk speed multiplier per mob type */
static float MobSpeed(int type) {
    switch (type) {
    case MOB_TYPE_SPIDER:   return 1.0f;  /* same as player */
    case MOB_TYPE_ZOMBIE:   return 0.5f;
    case MOB_TYPE_SKELETON: return 0.5f;
    case MOB_TYPE_CREEPER:  return 0.5f;
    default: return 0.25f; /* passive mobs wander slowly */
    }
}

/* Attack damage per mob type (in HP) */
static int MobAttackDamage(int type) {
    switch (type) {
    case MOB_TYPE_ZOMBIE:   return 3; /* 1.5 hearts */
    case MOB_TYPE_SKELETON: return 3;
    case MOB_TYPE_SPIDER:   return 2; /* 1 heart */
    case MOB_TYPE_CREEPER:  return 4; /* 2 hearts on melee */
    default: return 0;
    }
}

/* Model scale per mob type */
static void SetMobModelScale(struct Entity* e, int type) {
    switch (type) {
    case MOB_TYPE_SPIDER:
        Vec3_Set(e->ModelScale, 1.4f, 0.6f, 1.4f);
        break;
    case MOB_TYPE_PIG:
    case MOB_TYPE_SHEEP:
        Vec3_Set(e->ModelScale, 0.8f, 0.8f, 0.8f);
        break;
    case MOB_TYPE_CREEPER:
        Vec3_Set(e->ModelScale, 0.55f, 1.1f, 0.55f);
        break;
    default:
        Vec3_Set(e->ModelScale, 1.0f, 1.0f, 1.0f);
        break;
    }
}

/*########################################################################################################################*
*--------------------------------------------------------Mob VTABLE--------------------------------------------------------*
*#########################################################################################################################*/
static void Mob_Despawn(struct Entity* e) {
    struct MobEntity* mob = (struct MobEntity*)e;
    mob->IsAlive = false;
    Entities.List[mob->EntityID] = NULL;
}

static void Mob_SetLocation(struct Entity* e, struct LocationUpdate* update) {
    (void)e; (void)update;
}

static PackedCol Mob_GetCol(struct Entity* e) {
    Vec3 eyePos = Entity_GetEyePosition(e);
    IVec3 pos; IVec3_Floor(&pos, &eyePos);
    return Lighting.Color(pos.x, pos.y, pos.z);
}

static void Mob_RenderModel(struct Entity* e, float delta, float t) {
    AnimatedComp_GetCurrent(e, t);
    if (Model_ShouldRender(e)) {
        Model_Render(e->Model, e);
    }
}

static cc_bool Mob_ShouldRenderName(struct Entity* e) { return false; }

static void Mob_Tick(struct Entity* e, float delta);

static const struct EntityVTABLE mob_vtable = {
    Mob_Tick, Mob_Despawn, Mob_SetLocation, Mob_GetCol,
    Mob_RenderModel, Mob_ShouldRenderName
};

/*########################################################################################################################*
*---------------------------------------------------------Mob AI-----------------------------------------------------------*
*#########################################################################################################################*/
static void Mob_UpdateAI(struct MobEntity* mob, float delta) {
    struct Entity* e = &mob->Base;
    struct LocalPlayer* player;
    Vec3 toPlayer;
    float dist2, invLen, len;
    cc_bool hostile = mob->IsHostile;

    if (!Entities.CurPlayer) return;
    player = Entities.CurPlayer;

    Vec3_Sub(&toPlayer, &player->Base.Position, &e->Position);
    toPlayer.y = 0.0f; /* ignore vertical for horizontal movement */

    dist2 = toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z;

    if (hostile) {
        /* Hostile: always move toward player if within 32 blocks */
        if (dist2 > 0.001f && dist2 < 32.0f * 32.0f) {
            invLen = 1.0f / Math_SqrtF(dist2);
            mob->WalkX = toPlayer.x * invLen;
            mob->WalkZ = toPlayer.z * invLen;

            /* Face player */
            e->Yaw = (float)(Math_Atan2f(toPlayer.x, toPlayer.z) * MATH_RAD2DEG);

            /* Attack if close enough (melee range ~2 blocks) */
            if (dist2 < 2.25f) { /* 1.5^2 */
                if (mob->AttackTimer <= 0.0f) {
                    Survival_Damage(MobAttackDamage(mob->MobType));
                    mob->AttackTimer = 1.0f; /* 1 second between attacks */
                }
            }
        } else if (dist2 >= 32.0f * 32.0f) {
            /* Too far - wander randomly */
            mob->AITimer -= delta;
            if (mob->AITimer <= 0.0f) {
                mob->AITimer = MobRng_Range(2.0f, 5.0f);
                mob->WalkX = MobRng_Range(-1.0f, 1.0f);
                mob->WalkZ = MobRng_Range(-1.0f, 1.0f);
                len = Math_SqrtF(mob->WalkX * mob->WalkX + mob->WalkZ * mob->WalkZ);
                if (len > 0.001f) { mob->WalkX /= len; mob->WalkZ /= len; }
                else              { mob->WalkX = 0.0f; mob->WalkZ = 0.0f; }
            }
        }
    } else {
        /* Passive: wander randomly, flee from player if very close */
        if (dist2 < 4.0f) {
            /* Flee! */
            invLen = 1.0f / Math_SqrtF(dist2 + 0.001f);
            mob->WalkX = -toPlayer.x * invLen;
            mob->WalkZ = -toPlayer.z * invLen;
        } else {
            mob->AITimer -= delta;
            if (mob->AITimer <= 0.0f) {
                mob->AITimer = MobRng_Range(2.0f, 6.0f);
                /* 50% chance to stop, 50% chance to walk in random direction */
                if (MobRng_Next() & 1) {
                    mob->WalkX = 0.0f; mob->WalkZ = 0.0f;
                } else {
                    mob->WalkX = MobRng_Range(-1.0f, 1.0f);
                    mob->WalkZ = MobRng_Range(-1.0f, 1.0f);
                    len = Math_SqrtF(mob->WalkX*mob->WalkX + mob->WalkZ*mob->WalkZ);
                    if (len > 0.001f) { mob->WalkX /= len; mob->WalkZ /= len; }
                }
            }
        }
    }

    /* Update attack timer */
    if (mob->AttackTimer > 0.0f) mob->AttackTimer -= delta;
}

static void Mob_Tick(struct Entity* e, float delta) {
    struct MobEntity* mob = (struct MobEntity*)e;
    Vec3 heading;

    if (!mob->IsAlive || !World.Loaded) return;

    /* Advance position interpolation */
    e->prev = e->next;
    e->Position = e->prev.pos;

    /* Update AI */
    if (Survival_Active) {
        Mob_UpdateAI(mob, delta);
    }

    /* Jump over obstacles */
    if (Collisions_HitHorizontal(&mob->Collisions) && e->OnGround) {
        mob->Physics.Jumping = true;
    }

    /* Apply physics */
    Vec3_Set(heading, mob->WalkX * MobSpeed(mob->MobType),
                      0.0f,
                      mob->WalkZ * MobSpeed(mob->MobType));

    PhysicsComp_UpdateVelocityState(&mob->Physics);
    PhysicsComp_PhysicsTick(&mob->Physics, heading);
    mob->Physics.Jumping = false; /* reset each tick */

    /* Update position for next frame */
    e->next.pos = e->Position;
    e->Position = e->prev.pos;
    AnimatedComp_Update(e, e->prev.pos, e->next.pos, delta);

    /* Face walking direction when moving */
    if ((mob->WalkX != 0.0f || mob->WalkZ != 0.0f) && !mob->IsHostile) {
        e->Yaw = (float)(Math_Atan2f(mob->WalkX, mob->WalkZ) * MATH_RAD2DEG);
    }
}

/*########################################################################################################################*
*------------------------------------------------------Mob Lifecycle-------------------------------------------------------*
*#########################################################################################################################*/
void Mob_Damage(struct MobEntity* mob, int amount) {
    if (!mob->IsAlive) return;
    mob->Health -= amount;
    if (mob->Health <= 0) Mob_Kill(mob);
}

void Mob_Kill(struct MobEntity* mob) {
    if (!mob->IsAlive) return;
    mob->IsAlive = false;

    /* Give score */
    Survival_AddScore(mob->Score);

    /* Drop items: mushrooms from spider/pig/sheep */
    if (mob->MobType == MOB_TYPE_SPIDER ||
        mob->MobType == MOB_TYPE_PIG    ||
        mob->MobType == MOB_TYPE_SHEEP) {
        Inventory_PickBlock(BLOCK_BROWN_SHROOM);
    }
    /* Skeleton drops arrows - represented as sapling (closest Classic equivalent) */
    if (mob->MobType == MOB_TYPE_SKELETON) {
        Inventory_PickBlock(BLOCK_SAPLING);
    }

    Entities_Remove(mob->EntityID);
}

void Mob_Spawn(int mobType, float x, float y, float z) {
    static const cc_string humanoid = String_FromConst("humanoid");
    struct MobEntity* mob;
    struct Entity* e;
    int i;

    if (!Server.IsSinglePlayer) return;

    /* Find a free slot */
    mob = NULL;
    for (i = 0; i < MAX_MOBS; i++) {
        if (!MobEntities[i].IsAlive) { mob = &MobEntities[i]; break; }
    }
    if (!mob) return; /* No free slots */

    /* Find a free entity ID */
    i = mob - MobEntities; /* use same index as mob array */
    if (Entities.List[i]) return; /* Slot occupied */

    e = &mob->Base;
    Mem_Set(mob, 0, sizeof(*mob));

    /* Initialize entity */
    Entity_Init(e);
    e->VTABLE = &mob_vtable;

    /* Set model */
    Entity_SetModel(e, &humanoid);
    SetMobModelScale(e, mobType);
    Entity_UpdateModelBounds(e);

    /* Set solid color texture for this mob type */
    e->TextureId = mob_skins[mobType];
    e->uScale    = 1.0f;
    e->vScale    = 1.0f;
    e->SkinType  = 0; /* SKIN_64x32 */
    e->NonHumanSkin = true; /* use entity TextureId directly */

    /* Position */
    Vec3_Set(e->Position, x, y, z);
    e->prev.pos = e->Position;
    e->next.pos = e->Position;
    e->Yaw      = MobRng_Range(0.0f, 360.0f);

    /* Physics */
    PhysicsComp_Init(&mob->Physics, e);
    mob->Physics.Hacks      = &mob_hacks;
    mob->Physics.Collisions = &mob->Collisions;
    mob->Collisions.Entity  = e;

    /* Mob properties */
    mob->MobType   = mobType;
    mob->Health    = MobMaxHealth(mobType);
    mob->MaxHealth = mob->Health;
    mob->Score     = MobScore(mobType);
    mob->IsAlive   = true;
    mob->IsHostile = (mobType == MOB_TYPE_ZOMBIE || mobType == MOB_TYPE_SKELETON ||
                      mobType == MOB_TYPE_SPIDER  || mobType == MOB_TYPE_CREEPER);
    mob->EntityID  = i;
    mob->AITimer   = MobRng_Range(0.5f, 2.0f);

    /* Register in entity list */
    Entities.List[i] = e;
}

void Mobs_ClearAll(void) {
    int i;
    for (i = 0; i < MAX_MOBS; i++) {
        if (MobEntities[i].IsAlive) {
            MobEntities[i].IsAlive = false;
            if (Entities.List[i] == &MobEntities[i].Base) {
                Entities.List[i] = NULL;
            }
        }
    }
}

/*########################################################################################################################*
*------------------------------------------------------Mob Spawning-------------------------------------------------------*
*#########################################################################################################################*/
static int CountAliveMobs(void) {
    int count = 0, i;
    for (i = 0; i < MAX_MOBS; i++) {
        if (MobEntities[i].IsAlive) count++;
    }
    return count;
}

static cc_bool IsValidSpawnPos(float x, float y, float z) {
    BlockID below, at, above;
    int bx = (int)x, by = (int)y, bz = (int)z;

    if (!World_Contains(bx, by, bz)) return false;
    if (!World_Contains(bx, by - 1, bz)) return false;
    if (!World_Contains(bx, by + 1, bz)) return false;

    below = World_GetBlock(bx, by - 1, bz);
    at    = World_GetBlock(bx, by, bz);
    above = World_GetBlock(bx, by + 1, bz);

    /* Need solid block below, air at spawn and above */
    return Blocks.Collide[below] == COLLIDE_SOLID &&
           Blocks.Draw[at]   == DRAW_GAS &&
           Blocks.Draw[above] == DRAW_GAS;
}

static struct ScheduledTask2 mob_spawn_task;

static cc_bool Mobs_SpawnTick(struct ScheduledTask2* task) {
    struct LocalPlayer* player;
    float px, py, pz;
    float spawnX, spawnY, spawnZ;
    float angle, dist;
    int mobType, attempts, i, roll;
    (void)task;

    if (!Survival_Active || !World.Loaded || Survival_Dead) return true;
    if (!Server.IsSinglePlayer) return true;

    player = Entities.CurPlayer;
    if (!player) return true;

    /* Only spawn if fewer than MAX_MOBS/2 mobs currently alive */
    if (CountAliveMobs() >= MAX_MOBS / 2) return true;

    px = player->Base.Position.x;
    py = player->Base.Position.y;
    pz = player->Base.Position.z;

    /* Try to spawn a mob at a random position 10-32 blocks from player */
    for (attempts = 0; attempts < 10; attempts++) {
        angle = MobRng_Range(0.0f, MATH_PI * 2.0f);
        dist  = MobRng_Range(10.0f, 28.0f);
        spawnX = px + Math_CosF(angle) * dist;
        spawnZ = pz + Math_SinF(angle) * dist;

        /* Find surface Y at this position */
        spawnY = py;
        for (i = World.MaxY; i >= 0; i--) {
            if (IsValidSpawnPos(spawnX, (float)i, spawnZ)) {
                spawnY = (float)i;
                break;
            }
        }

        if (!IsValidSpawnPos(spawnX, spawnY, spawnZ)) continue;

        /* Pick mob type: 70% hostile, 30% passive */
        if (MobRng_Next() % 10 < 7) {
            /* Hostile: random between zombie/skeleton/spider/creeper */
            roll = MobRng_Next() % 4;
            if      (roll == 0) mobType = MOB_TYPE_ZOMBIE;
            else if (roll == 1) mobType = MOB_TYPE_SKELETON;
            else if (roll == 2) mobType = MOB_TYPE_SPIDER;
            else                mobType = MOB_TYPE_CREEPER;
        } else {
            /* Passive: pig or sheep */
            mobType = (MobRng_Next() & 1) ? MOB_TYPE_PIG : MOB_TYPE_SHEEP;
        }

        Mob_Spawn(mobType, spawnX, spawnY, spawnZ);
        break;
    }
    return true;
}

/*########################################################################################################################*
*---------------------------------------------------------Context----------------------------------------------------------*
*#########################################################################################################################*/
static void Mobs_ContextLost(void* obj) {
    int i;
    (void)obj;
    for (i = 0; i < MOB_TYPE_COUNT; i++) {
        Gfx_DeleteTexture(&mob_skins[i]);
    }
    Mem_Set(mob_skins, 0, sizeof(mob_skins));
}

static void Mobs_ContextRecreated(void* obj) {
    int i;
    (void)obj;
    CreateMobTextures();
    /* Reapply textures to living mobs */
    for (i = 0; i < MAX_MOBS; i++) {
        if (MobEntities[i].IsAlive) {
            MobEntities[i].Base.TextureId = mob_skins[MobEntities[i].MobType];
        }
    }
}

/*########################################################################################################################*
*----------------------------------------------------Component Interface---------------------------------------------------*
*#########################################################################################################################*/
static void Mobs_Init(void) {
    Mem_Set(MobEntities, 0, sizeof(MobEntities));
    Mem_Set(&mob_hacks,  0, sizeof(mob_hacks));

    /* Setup shared hacks: no special abilities, standard speed */
    mob_hacks.Enabled      = false;
    mob_hacks.BaseHorSpeed = 1.0f;
    mob_hacks.MaxHorSpeed  = 4.0f;
    mob_hacks.MaxJumps     = 1;

    CreateMobTextures();

    mob_spawn_task.interval = 5.0f; /* Try to spawn a mob every 5 seconds */
    mob_spawn_task.callback = Mobs_SpawnTick;
    ScheduledTask2_Add(&mob_spawn_task);

    Event_Register_(&GfxEvents.ContextLost,       NULL, Mobs_ContextLost);
    Event_Register_(&GfxEvents.ContextRecreated,   NULL, Mobs_ContextRecreated);
}

static void Mobs_Free(void) {
    Mobs_ClearAll();
    DestroyMobTextures();
    Event_Unregister_(&GfxEvents.ContextLost,      NULL, Mobs_ContextLost);
    Event_Unregister_(&GfxEvents.ContextRecreated,  NULL, Mobs_ContextRecreated);
}

static void Mobs_Reset(void) {
    Mobs_ClearAll();
}

static void Mobs_OnNewMap(void) {
    Mobs_ClearAll();
}

struct IGameComponent Mobs_Component = {
    Mobs_Init,    /* Init          */
    Mobs_Free,    /* Free          */
    Mobs_Reset,   /* Reset         */
    Mobs_OnNewMap, /* OnNewMap     */
    NULL          /* OnNewMapLoaded */
};
