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
#include "Camera.h"
#include "Input.h"
#include "Lighting.h"
#include "Model.h"
#include "Inventory.h"
#include "Platform.h"
#include "Server.h"
#include "String_.h"

struct MobEntity MobEntities[MAX_MOBS];
struct MobArrow  MobArrows[MAX_ARROWS];

/* Shared HacksComp for all mobs */
static struct HacksComp mob_hacks;

/* Mob skin textures (solid coloured) */
static GfxResourceID mob_skins[MOB_TYPE_COUNT];

/* Player melee cooldown (1-second = 20 ticks between swings) */
static int sv_playerAttackCooldown;

/*########################################################################################################################*
*--------------------------------------------------Simple RNG------------------------------------------------------------*
*#########################################################################################################################*/
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
*--------------------------------------------------Mob Texture Creation--------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID CreateSolidColorTex(int r, int g, int b) {
    struct Bitmap bmp;
    BitmapCol data[64];
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
    mob_skins[MOB_TYPE_SKELETON] = CreateSolidColorTex(0xD0, 0xD0, 0xD0); /* skeleton pale */
    mob_skins[MOB_TYPE_SPIDER]   = CreateSolidColorTex(0x40, 0x20, 0x10); /* spider dark brown */
    mob_skins[MOB_TYPE_CREEPER]  = CreateSolidColorTex(0x30, 0xA0, 0x30); /* creeper green */
}

static void DestroyMobTextures(void) {
    int i;
    for (i = 0; i < MOB_TYPE_COUNT; i++) Gfx_DeleteTexture(&mob_skins[i]);
}

/*########################################################################################################################*
*------------------------------------------------------Mob Properties----------------------------------------------------*
*#########################################################################################################################*/
static int MobMaxHealth(int type) {
    switch (type) {
    case MOB_TYPE_PIG:      return MOB_HP_PIG;
    case MOB_TYPE_SHEEP:    return MOB_HP_SHEEP;
    case MOB_TYPE_ZOMBIE:   return MOB_HP_ZOMBIE;
    case MOB_TYPE_SKELETON: return MOB_HP_SKELETON;
    case MOB_TYPE_SPIDER:   return MOB_HP_SPIDER;
    case MOB_TYPE_CREEPER:  return MOB_HP_CREEPER;
    default: return 20;
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

/* Walk speed multiplier per mob type.
   Spider matches player speed; undead are slower; passive mobs wander gently. */
static float MobSpeed(int type) {
    switch (type) {
    case MOB_TYPE_SPIDER:   return 1.0f;   /* same speed as player */
    case MOB_TYPE_ZOMBIE:   return 0.5f;
    case MOB_TYPE_SKELETON: return 0.5f;
    case MOB_TYPE_CREEPER:  return 0.5f;
    default:                return 0.25f;
    }
}

/* Melee damage range (random 2-6 HP for hostile, 0 for passive).
   Skeleton uses ranged attack, so its melee value is unused. */
static int MobMeleeDamage(int type) {
    switch (type) {
    case MOB_TYPE_ZOMBIE:
    case MOB_TYPE_SPIDER:
    case MOB_TYPE_CREEPER:
        return 2 + (int)(MobRng_Next() % 5); /* 2–6 HP */
    default:
        return 0;
    }
}

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
*------------------------------------------------------Creeper Explosion-------------------------------------------------*
*#########################################################################################################################*/
static void MobCreeper_Explode(struct MobEntity* mob) {
    Vec3 center;
    struct LocalPlayer* player;
    Vec3 diff;
    float dist;
    int damage;
    int x, y, z;
    int cx, cy, cz;
    int r = CREEPER_BLAST_RADIUS;

    center = mob->Base.Position;
    cx = (int)center.x;
    cy = (int)center.y;
    cz = (int)center.z;

    Survival_InExplosion = true;

    /* Destroy non-stone blocks in a sphere of radius r */
    for (x = cx - r; x <= cx + r; x++) {
        for (y = cy - r; y <= cy + r; y++) {
            for (z = cz - r; z <= cz + r; z++) {
                BlockID b;
                float dx, dy, dz;
                if (!World_Contains(x, y, z)) continue;

                dx = (float)(x - cx);
                dy = (float)(y - cy);
                dz = (float)(z - cz);
                if (dx*dx + dy*dy + dz*dz > (float)(r * r)) continue;

                b = World_GetBlock(x, y, z);
                /* Stone and bedrock resist explosions */
                if (b == BLOCK_STONE    || b == BLOCK_BEDROCK ||
                    b == BLOCK_COBBLE   || b == BLOCK_WATER   ||
                    b == BLOCK_STILL_WATER || b == BLOCK_LAVA ||
                    b == BLOCK_STILL_LAVA) continue;
                if (b == BLOCK_AIR) continue;

                Game_ChangeBlock(x, y, z, BLOCK_AIR);
            }
        }
    }

    Survival_InExplosion = false;

    /* Damage player based on distance from blast center */
    player = Entities.CurPlayer;
    if (player && !Survival_Dead) {
        Vec3_Sub(&diff, &player->Base.Position, &center);
        dist = Math_SqrtF(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
        if (dist < (float)r) {
            float frac = 1.0f - dist / (float)r;
            damage = (int)((float)CREEPER_BLAST_MAX_DMG * frac);
            if (damage > 0) Survival_Damage(damage);
        }
    }
}

/*########################################################################################################################*
*------------------------------------------------------Arrow System------------------------------------------------------*
*#########################################################################################################################*/
void MobArrow_Fire(Vec3 origin, Vec3 target, cc_bool fromPlayer) {
    Vec3 dir;
    float len;
    int i;

    for (i = 0; i < MAX_ARROWS; i++) {
        if (!MobArrows[i].active) break;
    }
    if (i >= MAX_ARROWS) return;

    dir.x = target.x - origin.x;
    dir.y = target.y - origin.y + 0.5f; /* aim slightly above centre */
    dir.z = target.z - origin.z;
    len   = Math_SqrtF(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (len < 0.001f) return;

    /* Arrow speed: 20 blocks/second */
    MobArrows[i].vel.x = dir.x / len * 20.0f;
    MobArrows[i].vel.y = dir.y / len * 20.0f;
    MobArrows[i].vel.z = dir.z / len * 20.0f;
    MobArrows[i].pos   = origin;
    MobArrows[i].age   = 0;
    MobArrows[i].active     = true;
    MobArrows[i].fromPlayer = fromPlayer;
}

static void MobArrows_Tick(float delta) {
    int i, j;

    for (i = 0; i < MAX_ARROWS; i++) {
        Vec3 newPos;
        IVec3 blockPos;
        BlockID b;

        if (!MobArrows[i].active) continue;

        MobArrows[i].age++;

        /* Move arrow */
        newPos.x = MobArrows[i].pos.x + MobArrows[i].vel.x * delta;
        newPos.y = MobArrows[i].pos.y + MobArrows[i].vel.y * delta;
        newPos.z = MobArrows[i].pos.z + MobArrows[i].vel.z * delta;

        /* Check for block collision */
        IVec3_Floor(&blockPos, &newPos);
        if (World_Contains(blockPos.x, blockPos.y, blockPos.z)) {
            b = World_GetBlock(blockPos.x, blockPos.y, blockPos.z);
            if (Blocks.Collide[b] == COLLIDE_SOLID) {
                MobArrows[i].active = false;
                continue;
            }
        }

        MobArrows[i].pos = newPos;

        /* Despawn after 3 seconds */
        if (MobArrows[i].age > 60) {
            MobArrows[i].active = false;
            continue;
        }

        /* Only deal damage in first 20 ticks (1 second of flight) */
        if (MobArrows[i].age > 20) continue;

        if (MobArrows[i].fromPlayer) {
            /* Check mob hits */
            for (j = 0; j < MAX_MOBS; j++) {
                Vec3 diff;
                float dist2;
                if (!MobEntities[j].IsAlive) continue;
                Vec3_Sub(&diff, &MobEntities[j].Base.Position, &newPos);
                diff.y -= 0.9f; /* aim at mob body centre */
                dist2 = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
                if (dist2 < 0.6f * 0.6f) {
                    Mob_Damage(&MobEntities[j], ARROW_DAMAGE);
                    MobArrows[i].active = false;
                    break;
                }
            }
        } else {
            /* Skeleton arrow — check player hit */
            if (Entities.CurPlayer && !Survival_Dead) {
                Vec3 diff;
                float dist2;
                Vec3_Sub(&diff, &Entities.CurPlayer->Base.Position, &newPos);
                dist2 = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
                if (dist2 < 0.6f * 0.6f) {
                    Survival_Damage(ARROW_DAMAGE);
                    MobArrows[i].active = false;
                }
            }
        }
    }
}

/*########################################################################################################################*
*--------------------------------------------------------Mob VTABLE------------------------------------------------------*
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
    struct MobEntity* mob = (struct MobEntity*)e;
    Vec3 eyePos;
    IVec3 pos;

    /* White flash when the mob has just been hit */
    if (mob->HitFlashTimer > 0) return PACKEDCOL_WHITE;

    eyePos = Entity_GetEyePosition(e);
    IVec3_Floor(&pos, &eyePos);
    return Lighting.Color(pos.x, pos.y, pos.z);
}

static void Mob_RenderModel(struct Entity* e, float delta, float t) {
    AnimatedComp_GetCurrent(e, t);
    if (Model_ShouldRender(e)) Model_Render(e->Model, e);
}

static cc_bool Mob_ShouldRenderName(struct Entity* e) { (void)e; return false; }

static void Mob_Tick(struct Entity* e, float delta);

static const struct EntityVTABLE mob_vtable = {
    Mob_Tick, Mob_Despawn, Mob_SetLocation, Mob_GetCol,
    Mob_RenderModel, Mob_ShouldRenderName
};

/*########################################################################################################################*
*---------------------------------------------------------Mob AI---------------------------------------------------------*
*#########################################################################################################################*/
static void Mob_UpdateAI(struct MobEntity* mob, float delta) {
    struct Entity* e = &mob->Base;
    struct LocalPlayer* player;
    Vec3 toPlayer;
    float dist2, invLen, len;

    if (!Entities.CurPlayer) return;
    player = Entities.CurPlayer;

    Vec3_Sub(&toPlayer, &player->Base.Position, &e->Position);
    toPlayer.y = 0.0f;
    dist2 = toPlayer.x*toPlayer.x + toPlayer.z*toPlayer.z;

    /* Decrement flash timer */
    if (mob->HitFlashTimer > 0) mob->HitFlashTimer--;

    if (mob->IsHostile) {
        if (dist2 > 0.001f && dist2 < 32.0f * 32.0f) {
            invLen = 1.0f / Math_SqrtF(dist2);
            mob->WalkX = toPlayer.x * invLen;
            mob->WalkZ = toPlayer.z * invLen;
            e->Yaw = (float)(Math_Atan2f(toPlayer.x, toPlayer.z) * MATH_RAD2DEG);

            if (mob->MobType == MOB_TYPE_SKELETON) {
                /* Skeleton: ranged attack — random chance per tick (~10%).
                   Makes skeletons very dangerous as in the original. */
                mob->WalkX = 0.0f; mob->WalkZ = 0.0f; /* skeleton keeps distance */
                if (dist2 < 24.0f * 24.0f) {
                    if ((MobRng_Next() % 10) == 0) {
                        Vec3 origin;
                        origin = e->Position;
                        origin.y += 1.6f; /* fire from eye level */
                        MobArrow_Fire(origin, player->Base.Position, false);
                    }
                    /* Skeleton backs up slightly if player is too close */
                    if (dist2 < 4.0f) {
                        mob->WalkX = -toPlayer.x * invLen;
                        mob->WalkZ = -toPlayer.z * invLen;
                    }
                }
            } else {
                /* Melee: attack when within range */
                if (dist2 < 2.25f) { /* 1.5 block range */
                    if (mob->AttackTimer <= 0.0f) {
                        Survival_Damage(MobMeleeDamage(mob->MobType));
                        mob->AttackTimer = 1.0f;
                    }
                }
            }
        } else if (dist2 >= 32.0f * 32.0f) {
            /* Wander when player is out of range */
            mob->AITimer -= delta;
            if (mob->AITimer <= 0.0f) {
                mob->AITimer = MobRng_Range(2.0f, 5.0f);
                mob->WalkX = MobRng_Range(-1.0f, 1.0f);
                mob->WalkZ = MobRng_Range(-1.0f, 1.0f);
                len = Math_SqrtF(mob->WalkX*mob->WalkX + mob->WalkZ*mob->WalkZ);
                if (len > 0.001f) { mob->WalkX /= len; mob->WalkZ /= len; }
                else              { mob->WalkX = 0.0f; mob->WalkZ = 0.0f; }
            }
        }
    } else {
        /* Passive: wander randomly, flee if player is too close */
        if (dist2 < 4.0f) {
            invLen = 1.0f / Math_SqrtF(dist2 + 0.001f);
            mob->WalkX = -toPlayer.x * invLen;
            mob->WalkZ = -toPlayer.z * invLen;
        } else {
            mob->AITimer -= delta;
            if (mob->AITimer <= 0.0f) {
                mob->AITimer = MobRng_Range(2.0f, 6.0f);
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

    if (mob->AttackTimer > 0.0f) mob->AttackTimer -= delta;
}

static void Mob_Tick(struct Entity* e, float delta) {
    struct MobEntity* mob = (struct MobEntity*)e;
    Vec3 heading;

    if (!mob->IsAlive || !World.Loaded) return;

    e->prev = e->next;
    e->Position = e->prev.pos;

    if (Survival_Active) Mob_UpdateAI(mob, delta);

    /* Jump over obstacles */
    if (Collisions_HitHorizontal(&mob->Collisions) && e->OnGround)
        mob->Physics.Jumping = true;

    Vec3_Set(heading,
        mob->WalkX * MobSpeed(mob->MobType),
        0.0f,
        mob->WalkZ * MobSpeed(mob->MobType));

    PhysicsComp_UpdateVelocityState(&mob->Physics);
    PhysicsComp_PhysicsTick(&mob->Physics, heading);
    mob->Physics.Jumping = false;

    e->next.pos = e->Position;
    e->Position = e->prev.pos;
    AnimatedComp_Update(e, e->prev.pos, e->next.pos, delta);

    if ((mob->WalkX != 0.0f || mob->WalkZ != 0.0f) && !mob->IsHostile)
        e->Yaw = (float)(Math_Atan2f(mob->WalkX, mob->WalkZ) * MATH_RAD2DEG);
}

/*########################################################################################################################*
*------------------------------------------------------Mob Lifecycle-----------------------------------------------------*
*#########################################################################################################################*/
void Mob_Damage(struct MobEntity* mob, int amount) {
    if (!mob->IsAlive) return;
    mob->Health -= amount;
    mob->HitFlashTimer = 4; /* brief white flash */
    if (mob->Health <= 0) Mob_Kill(mob);
}

void Mob_Kill(struct MobEntity* mob) {
    if (!mob->IsAlive) return;
    mob->IsAlive = false;

    Survival_AddScore(mob->Score); /* no score if Survival_InExplosion */

    /* Item drops: passive mobs and spider drop mushrooms */
    if (mob->MobType == MOB_TYPE_SPIDER ||
        mob->MobType == MOB_TYPE_PIG    ||
        mob->MobType == MOB_TYPE_SHEEP) {
        Inventory_PickBlock(BLOCK_BROWN_SHROOM);
    }
    /* Skeleton drops 4–9 arrows (use sapling as arrow proxy) */
    if (mob->MobType == MOB_TYPE_SKELETON) {
        int n = 4 + (int)(MobRng_Next() % 6); /* 4–9 */
        int i;
        for (i = 0; i < n; i++) Inventory_PickBlock(BLOCK_SAPLING);
    }

    /* Creeper explodes on death */
    if (mob->MobType == MOB_TYPE_CREEPER) {
        MobCreeper_Explode(mob);
    }

    Entities_Remove(mob->EntityID);
}

void Mob_Spawn(int mobType, float x, float y, float z) {
    static const cc_string humanoid = String_FromConst("humanoid");
    struct MobEntity* mob;
    struct Entity* e;
    int i;

    if (!Server.IsSinglePlayer) return;

    mob = NULL;
    for (i = 0; i < MAX_MOBS; i++) {
        if (!MobEntities[i].IsAlive) { mob = &MobEntities[i]; break; }
    }
    if (!mob) return;

    i = (int)(mob - MobEntities);
    if (Entities.List[i]) return;

    e = &mob->Base;
    Mem_Set(mob, 0, sizeof(*mob));

    Entity_Init(e);
    e->VTABLE = &mob_vtable;

    Entity_SetModel(e, &humanoid);
    SetMobModelScale(e, mobType);
    Entity_UpdateModelBounds(e);

    e->TextureId    = mob_skins[mobType];
    e->uScale       = 1.0f;
    e->vScale       = 1.0f;
    e->SkinType     = 0;
    e->NonHumanSkin = true;

    Vec3_Set(e->Position, x, y, z);
    e->prev.pos = e->Position;
    e->next.pos = e->Position;
    e->Yaw      = MobRng_Range(0.0f, 360.0f);

    PhysicsComp_Init(&mob->Physics, e);
    mob->Physics.Hacks      = &mob_hacks;
    mob->Physics.Collisions = &mob->Collisions;
    mob->Collisions.Entity  = e;

    mob->MobType   = mobType;
    mob->Health    = MobMaxHealth(mobType);
    mob->MaxHealth = mob->Health;
    mob->Score     = MobScore(mobType);
    mob->IsAlive   = true;
    mob->IsHostile = (mobType == MOB_TYPE_ZOMBIE || mobType == MOB_TYPE_SKELETON ||
                      mobType == MOB_TYPE_SPIDER  || mobType == MOB_TYPE_CREEPER);
    mob->EntityID  = i;
    mob->AITimer   = MobRng_Range(0.5f, 2.0f);

    Entities.List[i] = e;
}

void Mobs_ClearAll(void) {
    int i;
    for (i = 0; i < MAX_MOBS; i++) {
        if (MobEntities[i].IsAlive) {
            MobEntities[i].IsAlive = false;
            if (Entities.List[i] == &MobEntities[i].Base)
                Entities.List[i] = NULL;
        }
    }
    for (i = 0; i < MAX_ARROWS; i++) MobArrows[i].active = false;
}

/*########################################################################################################################*
*------------------------------------------------------Player Attack-----------------------------------------------------*
*#########################################################################################################################*/
void Mobs_PlayerMeleeAttack(void) {
    struct LocalPlayer* player;
    Vec3 eyePos, lookDir;
    int i;
    struct MobEntity* best;
    float bestDist2;

    if (!Survival_Active || !Server.IsSinglePlayer) return;
    if (Survival_Dead || sv_playerAttackCooldown > 0) return;

    player = Entities.CurPlayer;
    if (!player) return;

    eyePos  = Entity_GetEyePosition(&player->Base);
    lookDir = Vec3_GetDirVector(player->Base.Yaw   * MATH_DEG2RAD,
                                player->Base.Pitch * MATH_DEG2RAD);

    best      = NULL;
    bestDist2 = 4.0f * 4.0f; /* 4-block reach */

    for (i = 0; i < MAX_MOBS; i++) {
        Vec3 diff;
        float dist2, dot;

        if (!MobEntities[i].IsAlive) continue;
        Vec3_Sub(&diff, &MobEntities[i].Base.Position, &eyePos);
        dist2 = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
        if (dist2 > bestDist2) continue;

        /* Must be roughly in front of player */
        dot = diff.x*lookDir.x + diff.y*lookDir.y + diff.z*lookDir.z;
        if (dot < 0.3f) continue;

        best      = &MobEntities[i];
        bestDist2 = dist2;
    }

    if (!best) return;

    Mob_Damage(best, PLAYER_FIST_DAMAGE);
    sv_playerAttackCooldown = 20; /* 1-second swing cooldown */
}

static void OnPlayerClick(void* obj, int key, cc_bool was, struct InputDevice* device) {
    (void)obj;
    if (was) return; /* only on fresh press */
    if (!InputBind_Claims(BIND_DELETE_BLOCK, key, device)) return;
    Mobs_PlayerMeleeAttack();
}

/*########################################################################################################################*
*------------------------------------------------------Mob Spawning------------------------------------------------------*
*#########################################################################################################################*/
static int CountAliveMobs(void) {
    int count = 0, i;
    for (i = 0; i < MAX_MOBS; i++) {
        if (MobEntities[i].IsAlive) count++;
    }
    return count;
}

static cc_bool IsValidSpawnPos(float x, float y, float z) {
    int bx = (int)x, by = (int)y, bz = (int)z;
    BlockID below, at, above;

    if (!World_Contains(bx, by, bz))        return false;
    if (!World_Contains(bx, by - 1, bz))    return false;
    if (!World_Contains(bx, by + 1, bz))    return false;

    below = World_GetBlock(bx, by - 1, bz);
    at    = World_GetBlock(bx, by,     bz);
    above = World_GetBlock(bx, by + 1, bz);

    return Blocks.Collide[below] == COLLIDE_SOLID &&
           Blocks.Draw[at]       == DRAW_GAS      &&
           Blocks.Draw[above]    == DRAW_GAS;
}

static struct ScheduledTask2 mob_spawn_task;
static struct ScheduledTask2 mob_tick_task;

static cc_bool Mobs_UpdateTick(struct ScheduledTask2* task) {
    (void)task;
    if (!World.Loaded) return true;

    /* Update player attack cooldown */
    if (sv_playerAttackCooldown > 0) sv_playerAttackCooldown--;

    /* Update arrows (entity tick doesn't cover these) */
    MobArrows_Tick(task->interval);

    /* NOTE: Mob_Tick for each mob is called automatically by the entity
       system (Entities_Tick -> VTABLE->Tick for all entries in Entities.List).
       Do NOT call Mob_Tick here to avoid double-ticking. */
    return true;
}

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

    if (CountAliveMobs() >= MAX_MOBS / 2) return true;

    px = player->Base.Position.x;
    py = player->Base.Position.y;
    pz = player->Base.Position.z;

    for (attempts = 0; attempts < 10; attempts++) {
        angle  = MobRng_Range(0.0f, MATH_PI * 2.0f);
        dist   = MobRng_Range(10.0f, 28.0f);
        spawnX = px + Math_CosF(angle) * dist;
        spawnZ = pz + Math_SinF(angle) * dist;
        spawnY = py;

        for (i = World.MaxY; i >= 0; i--) {
            if (IsValidSpawnPos(spawnX, (float)i, spawnZ)) {
                spawnY = (float)i;
                break;
            }
        }

        if (!IsValidSpawnPos(spawnX, spawnY, spawnZ)) continue;

        if (MobRng_Next() % 10 < 7) {
            roll = MobRng_Next() % 4;
            if      (roll == 0) mobType = MOB_TYPE_ZOMBIE;
            else if (roll == 1) mobType = MOB_TYPE_SKELETON;
            else if (roll == 2) mobType = MOB_TYPE_SPIDER;
            else                mobType = MOB_TYPE_CREEPER;
        } else {
            mobType = (MobRng_Next() & 1) ? MOB_TYPE_PIG : MOB_TYPE_SHEEP;
        }

        Mob_Spawn(mobType, spawnX, spawnY, spawnZ);
        break;
    }
    return true;
}

/*########################################################################################################################*
*---------------------------------------------------------Context--------------------------------------------------------*
*#########################################################################################################################*/
static void Mobs_ContextLost(void* obj) {
    int i;
    (void)obj;
    for (i = 0; i < MOB_TYPE_COUNT; i++) Gfx_DeleteTexture(&mob_skins[i]);
    Mem_Set(mob_skins, 0, sizeof(mob_skins));
}

static void Mobs_ContextRecreated(void* obj) {
    int i;
    (void)obj;
    CreateMobTextures();
    for (i = 0; i < MAX_MOBS; i++) {
        if (MobEntities[i].IsAlive)
            MobEntities[i].Base.TextureId = mob_skins[MobEntities[i].MobType];
    }
}

/*########################################################################################################################*
*----------------------------------------------------Component Interface-------------------------------------------------*
*#########################################################################################################################*/
static void Mobs_Init(void) {
    Mem_Set(MobEntities, 0, sizeof(MobEntities));
    Mem_Set(MobArrows,   0, sizeof(MobArrows));
    Mem_Set(&mob_hacks,  0, sizeof(mob_hacks));

    mob_hacks.Enabled      = false;
    mob_hacks.BaseHorSpeed = 1.0f;
    mob_hacks.MaxHorSpeed  = 4.0f;
    mob_hacks.MaxJumps     = 1;

    sv_playerAttackCooldown = 0;

    CreateMobTextures();

    /* Mob physics/AI runs at 20 Hz (same as entity tick) */
    mob_tick_task.interval  = 0.05f;
    mob_tick_task.callback  = Mobs_UpdateTick;
    ScheduledTask2_Add(&mob_tick_task);

    mob_spawn_task.interval = 5.0f;
    mob_spawn_task.callback = Mobs_SpawnTick;
    ScheduledTask2_Add(&mob_spawn_task);

    Event_Register_(&GfxEvents.ContextLost,      NULL, Mobs_ContextLost);
    Event_Register_(&GfxEvents.ContextRecreated,  NULL, Mobs_ContextRecreated);
    Event_Register_(&InputEvents.Down2,           NULL, OnPlayerClick);
}

static void Mobs_Free(void) {
    Mobs_ClearAll();
    DestroyMobTextures();
    Event_Unregister_(&GfxEvents.ContextLost,      NULL, Mobs_ContextLost);
    Event_Unregister_(&GfxEvents.ContextRecreated,  NULL, Mobs_ContextRecreated);
    Event_Unregister_(&InputEvents.Down2,           NULL, OnPlayerClick);
}

static void Mobs_Reset(void) {
    Mobs_ClearAll();
    sv_playerAttackCooldown = 0;
}

static void Mobs_OnNewMap(void) { Mobs_ClearAll(); }

struct IGameComponent Mobs_Component = {
    Mobs_Init,     /* Init           */
    Mobs_Free,     /* Free           */
    Mobs_Reset,    /* Reset          */
    Mobs_OnNewMap, /* OnNewMap       */
    NULL           /* OnNewMapLoaded */
};
