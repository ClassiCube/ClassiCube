#include "Survival.h"
#include "Game.h"
#include "Entity.h"
#include "EntityComponents.h"
#include "World.h"
#include "Block.h"
#include "BlockID.h"
#include "Chat.h"
#include "Event.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Options.h"
#include "Server.h"
#include "Inventory.h"
#include "String_.h"

cc_bool Survival_Active;
int     Survival_Health = SURVIVAL_MAX_HEALTH;
int     Survival_Air    = SURVIVAL_MAX_AIR;
int     Survival_Score;
cc_bool Survival_Dead;

static int  sv_damageCooldown; /* ticks of invincibility after hit */
static int  sv_regenTimer;     /* unused - no regen in c0.30-s */
static int  sv_deathTimer;     /* ticks since death, for auto-respawn */
static int  sv_airCooldown;    /* ticks between air decrements */
static int  sv_lavaTick;       /* ticks for lava damage rate */
static int  sv_drownTick;      /* ticks for drowning damage rate */
static cc_bool sv_prevOnGround;
static float   sv_highestY;   /* highest Y since last landing */
static int  sv_lastHealth;    /* track changes to update HUD */
static int  sv_lastAir;

/* CP437 heart char for health display */
#define HEART_CHAR  "\x03"
#define BULLET_CHAR "\xf9"

/* --- Block drop table for c0.30-s --- */
static BlockID Survival_GetDrop(BlockID block) {
    switch (block) {
    case BLOCK_GRASS:    return BLOCK_DIRT;
    case BLOCK_STONE:    return BLOCK_COBBLE;
    case BLOCK_COAL_ORE: return BLOCK_SLAB;
    case BLOCK_IRON_ORE: return BLOCK_IRON;
    case BLOCK_GOLD_ORE: return BLOCK_GOLD;
    case BLOCK_LOG:      return BLOCK_WOOD;
    case BLOCK_LEAVES:   return BLOCK_SAPLING;
    default:             return block;
    }
}

static void OnBlockChanged(void* obj, IVec3 coords, BlockID oldBlock, BlockID newBlock) {
    BlockID drop;
    (void)obj; (void)coords;
    if (!Survival_Active || !Server.IsSinglePlayer) return;
    /* Only care about block removals (break events) */
    if (newBlock != BLOCK_AIR || oldBlock == BLOCK_AIR) return;
    /* Bedrock, water, lava are unbreakable in survival */
    if (oldBlock == BLOCK_BEDROCK)    return;
    if (oldBlock == BLOCK_WATER)      return;
    if (oldBlock == BLOCK_STILL_WATER) return;
    if (oldBlock == BLOCK_LAVA)       return;
    if (oldBlock == BLOCK_STILL_LAVA) return;

    drop = Survival_GetDrop(oldBlock);
    /* Auto-pick the dropped block into the hotbar */
    Inventory_PickBlock(drop);
}

void Survival_Damage(int amount) {
    if (!Survival_Active || Survival_Dead) return;
    if (sv_damageCooldown > 0) return;

    Survival_Health -= amount;
    sv_damageCooldown = 10; /* half second invincibility */

    if (Survival_Health <= 0) {
        Survival_Health = 0;
        Survival_Die();
    }
    Survival_UpdateHUD();
}

void Survival_Die(void) {
    if (Survival_Dead) return;
    Survival_Dead   = true;
    sv_deathTimer   = 0;
    Chat_AddRaw("&cYou died! Respawning in 3 seconds...");
    Chat_AddRaw("&eGame Over!");
    Survival_UpdateHUD();
}

void Survival_Respawn(void) {
    struct LocationUpdate update;
    Survival_Health     = SURVIVAL_MAX_HEALTH;
    Survival_Air        = SURVIVAL_MAX_AIR;
    Survival_Dead       = false;
    sv_damageCooldown   = 0;
    sv_deathTimer       = 0;
    sv_prevOnGround     = true;
    sv_highestY         = 0.0f;

    LocalPlayer_CalcDefaultSpawn(Entities.CurPlayer, &update);
    LocalPlayers_MoveToSpawn(&update);
    Survival_UpdateHUD();
    Chat_AddRaw("&aRespawned!");
}

void Survival_AddScore(int points) {
    cc_string msg; char msgBuffer[64];
    Survival_Score += points;
    String_InitArray(msg, msgBuffer);
    String_Format1(&msg, "&eScore: %i", &Survival_Score);
    Chat_AddOf(&msg, MSG_TYPE_STATUS_3);
}

void Survival_UpdateHUD(void) {
    cc_string msg; char msgBuffer[128];
    int hearts, i;

    if (!Survival_Active) return;

    /* Build health string: e.g. "&c[*][*][*]&8[.][.]" */
    String_InitArray(msg, msgBuffer);
    hearts = Survival_Health / 2; /* full hearts */

    /* Health bar above hotbar shown in BOTTOMRIGHT_1 */
    String_AppendConst(&msg, "&c");
    for (i = 0; i < 10; i++) {
        if (i * 2 + 1 < Survival_Health) {
            /* Full heart */
            String_AppendConst(&msg, HEART_CHAR);
        } else if (i * 2 < Survival_Health) {
            /* Half heart - show slightly different */
            String_AppendConst(&msg, "&4" HEART_CHAR "&c");
        } else {
            /* Empty heart */
            String_AppendConst(&msg, "&8" HEART_CHAR "&c");
        }
    }

    /* Air bubbles when drowning */
    if (Survival_Air < SURVIVAL_MAX_AIR) {
        int bubbles = (Survival_Air * 10) / SURVIVAL_MAX_AIR;
        String_AppendConst(&msg, " &9");
        for (i = 0; i < 10; i++) {
            if (i < bubbles) {
                String_AppendConst(&msg, BULLET_CHAR);
            } else {
                String_AppendConst(&msg, "&8" BULLET_CHAR "&9");
            }
        }
    }

    if (Survival_Dead) {
        Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_1);
        Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_2);
        return;
    }

    Chat_AddOf(&msg, MSG_TYPE_STATUS_1);

    /* Score on line 2 */
    String_InitArray(msg, msgBuffer);
    String_Format1(&msg, "&eScore: %i", &Survival_Score);
    Chat_AddOf(&msg, MSG_TYPE_STATUS_2);
}

static cc_bool Survival_Tick(struct ScheduledTask2* task) {
    struct LocalPlayer* p;
    struct Entity* e;
    cc_bool onGround;
    float   curY;
    (void)task;

    if (!Survival_Active || !World.Loaded) return true;

    /* Auto-respawn after 60 ticks (3 seconds) */
    if (Survival_Dead) {
        sv_deathTimer++;
        if (sv_deathTimer >= 60) Survival_Respawn();
        return true;
    }

    p = Entities.CurPlayer;
    e = &p->Base;

    /* Physical position is e->next.pos after entity tick */
    curY     = e->next.pos.y;
    onGround = e->OnGround;

    /* Decrement invincibility frames */
    if (sv_damageCooldown > 0) sv_damageCooldown--;

    /* ---- Fall damage ---- */
    if (onGround) {
        if (!sv_prevOnGround) {
            /* Just landed - compute fall distance */
            float fallDist = sv_highestY - curY;
            if (fallDist > 3.0f) {
                int damage = (int)(fallDist - 3.0f) * 2;
                if (damage < 1) damage = 1;
                Survival_Damage(damage);
            }
        }
        /* Reset highest Y while on ground */
        sv_highestY = curY;
    } else {
        /* Track highest point while in air */
        if (curY > sv_highestY) sv_highestY = curY;
    }
    sv_prevOnGround = onGround;

    /* ---- Lava damage (2 hearts/sec = 2 HP per tick is too fast, use 1/sec) ---- */
    if (Entity_TouchesAnyLava(e)) {
        /* Lava: 1 heart (2 HP) per second = every 10 ticks */
        sv_lavaTick++;
        if (sv_lavaTick >= 10) { sv_lavaTick = 0; Survival_Damage(2); }
    }

    /* ---- Drowning ---- */
    if (Entity_TouchesAnyWater(e)) {
        if (sv_airCooldown > 0) {
            sv_airCooldown--;
        } else {
            sv_airCooldown = 15; /* lose 1 air every 15 ticks (~0.75s) */
            if (Survival_Air > 0) {
                Survival_Air--;
                if (Survival_Air != sv_lastAir) {
                    sv_lastAir = Survival_Air;
                    Survival_UpdateHUD();
                }
            }
        }
        if (Survival_Air <= 0) {
            /* 1 heart per second when out of air */
            sv_drownTick++;
            if (sv_drownTick >= 20) { sv_drownTick = 0; Survival_Damage(2); }
        }
    } else {
        /* Recover air quickly out of water */
        if (Survival_Air < SURVIVAL_MAX_AIR) {
            Survival_Air += 5;
            if (Survival_Air > SURVIVAL_MAX_AIR) Survival_Air = SURVIVAL_MAX_AIR;
            sv_airCooldown = 0;
            if (Survival_Air != sv_lastAir) {
                sv_lastAir = Survival_Air;
                Survival_UpdateHUD();
            }
        }
    }

    /* Update HUD if health changed */
    if (Survival_Health != sv_lastHealth) {
        sv_lastHealth = Survival_Health;
        Survival_UpdateHUD();
    }

    return true;
}

static struct ScheduledTask2 sv_task;

static void Survival_Init(void) {
    cc_bool enabled = Options_GetBool("survival-mode", false);
    Survival_Active = enabled && true; /* Always allow enabling */

    Survival_Health = SURVIVAL_MAX_HEALTH;
    Survival_Air    = SURVIVAL_MAX_AIR;
    Survival_Score  = 0;
    Survival_Dead   = false;
    sv_damageCooldown = 0;
    sv_deathTimer     = 0;
    sv_airCooldown    = 0;
    sv_prevOnGround   = true;
    sv_highestY       = 0.0f;
    sv_lastHealth     = SURVIVAL_MAX_HEALTH;
    sv_lastAir        = SURVIVAL_MAX_AIR;

    sv_task.interval = 0.05f; /* 20 ticks per second */
    sv_task.callback = Survival_Tick;
    ScheduledTask2_Add(&sv_task);

    Event_Register_(&UserEvents.BlockChanged, NULL, OnBlockChanged);
}

static void Survival_Free(void) {
    Event_Unregister_(&UserEvents.BlockChanged, NULL, OnBlockChanged);
    /* Clear survival status from HUD */
    Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_1);
    Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_2);
    Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_3);
}

static void Survival_Reset(void) {
    Survival_Health   = SURVIVAL_MAX_HEALTH;
    Survival_Air      = SURVIVAL_MAX_AIR;
    Survival_Dead     = false;
    sv_damageCooldown = 0;
    sv_deathTimer     = 0;
    sv_airCooldown    = 0;
    sv_lavaTick       = 0;
    sv_drownTick      = 0;
    sv_prevOnGround   = true;
    sv_highestY       = 0.0f;
    sv_lastHealth     = SURVIVAL_MAX_HEALTH;
    sv_lastAir        = SURVIVAL_MAX_AIR;
    Survival_UpdateHUD();
}

static void Survival_OnNewMapLoaded(void) {
    /* Reset state for new map */
    Survival_Health   = SURVIVAL_MAX_HEALTH;
    Survival_Air      = SURVIVAL_MAX_AIR;
    Survival_Dead     = false;
    sv_damageCooldown = 0;
    sv_deathTimer     = 0;
    sv_airCooldown    = 0;
    sv_lavaTick       = 0;
    sv_drownTick      = 0;
    sv_prevOnGround   = true;
    /* Grab current player Y as starting high point */
    if (Entities.CurPlayer) {
        sv_highestY = Entities.CurPlayer->Base.Position.y;
    }
    sv_lastHealth = SURVIVAL_MAX_HEALTH;
    sv_lastAir    = SURVIVAL_MAX_AIR;

    /* Force a check: is survival enabled in options? */
    Survival_Active = Options_GetBool("survival-mode", false);
    if (Survival_Active && Server.IsSinglePlayer) {
        Survival_UpdateHUD();
    }
}

struct IGameComponent Survival_Component = {
    Survival_Init,         /* Init          */
    Survival_Free,         /* Free          */
    Survival_Reset,        /* Reset         */
    NULL,                  /* OnNewMap      */
    Survival_OnNewMapLoaded /* OnNewMapLoaded */
};
