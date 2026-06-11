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
int     Survival_Arrows = SURVIVAL_START_ARROWS;
cc_bool Survival_InExplosion;

static int  sv_damageCooldown;
static int  sv_deathTimer;       /* unused -- no respawn, just for delayed HUD */
static int  sv_airCooldown;
static int  sv_lavaTick;
static int  sv_drownTick;
static cc_bool sv_prevOnGround;
static float   sv_highestY;
static int  sv_lastHealth;
static int  sv_lastAir;

/* Simple XorShift RNG for block drops */
static cc_uint32 sv_rng = 54321;
static int Sv_RandRange(int min, int max) {
	sv_rng ^= sv_rng << 13;
	sv_rng ^= sv_rng >> 17;
	sv_rng ^= sv_rng << 5;
	return min + (int)(sv_rng % (cc_uint32)(max - min + 1));
}

/* CP437 heart / bullet chars */
#define HEART_CHAR  "\x03"
#define BULLET_CHAR "\xf9"

/* --- Block drop table (c0.30-s) ---
   Logs drop 3-5 planks; leaves have 1/10 chance to drop a sapling;
   ore blocks drop the processed material, not the ore itself. */
static BlockID Survival_GetDrop(BlockID block, int* count) {
	*count = 1;
	switch (block) {
	case BLOCK_GRASS:    return BLOCK_DIRT;
	case BLOCK_STONE:    return BLOCK_COBBLE;
	case BLOCK_COAL_ORE: return BLOCK_SLAB;    /* coal ore -> slab in c0.30-s */
	case BLOCK_IRON_ORE: return BLOCK_IRON;
	case BLOCK_GOLD_ORE: return BLOCK_GOLD;
	case BLOCK_LOG:
		*count = Sv_RandRange(3, 5);           /* logs drop 3-5 planks */
		return BLOCK_WOOD;
	case BLOCK_LEAVES:
		if (Sv_RandRange(1, 10) == 1)          /* 1/10 chance */
			return BLOCK_SAPLING;
		return BLOCK_AIR;                      /* no drop */
	default:
		return block;
	}
}

static void OnBlockChanged(void* obj, IVec3 coords, BlockID oldBlock, BlockID newBlock) {
	BlockID drop;
	int count, i;
	(void)obj;

	if (!Survival_Active || !Server.IsSinglePlayer) return;
	/* Only care about block removals */
	if (newBlock != BLOCK_AIR || oldBlock == BLOCK_AIR) return;
	/* Explosion-cleared blocks give no items */
	if (Survival_InExplosion) return;

	/* Unbreakable / non-item blocks */
	if (oldBlock == BLOCK_BEDROCK)      return;
	if (oldBlock == BLOCK_WATER)        return;
	if (oldBlock == BLOCK_STILL_WATER)  return;
	if (oldBlock == BLOCK_LAVA)         return;
	if (oldBlock == BLOCK_STILL_LAVA)   return;

	/* Game_UpdateBlock in OnBlockPlaced fires a removal event for the mushroom;
	   skip it here -- mushroom healing is handled entirely in OnBlockPlaced */
	if (oldBlock == BLOCK_BROWN_SHROOM || oldBlock == BLOCK_RED_SHROOM) return;

	drop = Survival_GetDrop(oldBlock, &count);
	if (drop == BLOCK_AIR) return;
	for (i = 0; i < count; i++) Inventory_PickBlock(drop);
}

/* Intercept mushroom placement as consumption (brown = heal, red = poison) */
static void OnBlockPlaced(void* obj, IVec3 coords, BlockID oldBlock, BlockID newBlock) {
	(void)obj;
	if (!Survival_Active || !Server.IsSinglePlayer) return;
	if (oldBlock != BLOCK_AIR) return; /* only fresh placement */

	if (newBlock == BLOCK_BROWN_SHROOM) {
		/* Undo the placement and heal player instead */
		Game_UpdateBlock(coords.x, coords.y, coords.z, BLOCK_AIR);
		if (Survival_Dead) return;
		Survival_Health += 5;
		if (Survival_Health > SURVIVAL_MAX_HEALTH) Survival_Health = SURVIVAL_MAX_HEALTH;
		Chat_AddRaw("&aYou ate a mushroom (5 HP).");
		Survival_UpdateHUD();
	} else if (newBlock == BLOCK_RED_SHROOM) {
		Game_UpdateBlock(coords.x, coords.y, coords.z, BLOCK_AIR);
		if (Survival_Dead) return;
		Chat_AddRaw("&cYou ate a poisonous mushroom! (-3 HP)");
		Survival_Damage(3);
	}
}

void Survival_Damage(int amount) {
	if (!Survival_Active || Survival_Dead) return;
	if (sv_damageCooldown > 0) return;

	Survival_Health -= amount;
	sv_damageCooldown = 10; /* 0.5-second invincibility */

	if (Survival_Health <= 0) {
		Survival_Health = 0;
		Survival_Die();
		return;
	}
	Survival_UpdateHUD();
}

void Survival_Die(void) {
	cc_string msg; char buf[128];
	if (Survival_Dead) return;
	Survival_Dead = true;

	String_InitArray(msg, buf);
	String_Format1(&msg, "&4Game Over! &eYour score: %i", &Survival_Score);
	Chat_AddRaw("&4-----------------------------");
	Chat_Add(&msg);
	Chat_AddRaw("&4-----------------------------");
	Chat_AddRaw("&7Generate a new world to play again.");
	Survival_UpdateHUD();
}

void Survival_AddScore(int points) {
	cc_string msg; char buf[64];
	if (Survival_InExplosion) return; /* no score for indirect kills */
	Survival_Score += points;
	String_InitArray(msg, buf);
	String_Format1(&msg, "&eScore: %i", &Survival_Score);
	Chat_AddOf(&msg, MSG_TYPE_STATUS_3);
}

void Survival_UpdateHUD(void) {
	cc_string msg; char buf[192];
	int i;

	if (!Survival_Active) return;

	if (Survival_Dead) {
		/* Show Game Over on all status lines */
		String_InitArray(msg, buf);
		String_Format1(&msg, "&4GAME OVER  &eScore: %i", &Survival_Score);
		Chat_AddOf(&msg, MSG_TYPE_STATUS_1);
		Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_2);
		return;
	}

	/* Line 1: hearts */
	String_InitArray(msg, buf);
	String_AppendConst(&msg, "&c");
	for (i = 0; i < 10; i++) {
		if (i * 2 + 1 < Survival_Health) {
			String_AppendConst(&msg, HEART_CHAR);
		} else if (i * 2 < Survival_Health) {
			String_AppendConst(&msg, "&4" HEART_CHAR "&c");
		} else {
			String_AppendConst(&msg, "&8" HEART_CHAR "&c");
		}
	}

	/* Append air bubbles while drowning */
	if (Survival_Air < SURVIVAL_MAX_AIR) {
		int bubbles = (Survival_Air * 10) / SURVIVAL_MAX_AIR;
		String_AppendConst(&msg, " &9");
		for (i = 0; i < 10; i++) {
			if (i < bubbles)
				String_AppendConst(&msg, BULLET_CHAR);
			else
				String_AppendConst(&msg, "&8" BULLET_CHAR "&9");
		}
	}

	/* Flash hearts when critically low (<=4 HP = 2 hearts) */
	if (Survival_Health <= 4) {
		String_AppendConst(&msg, " &c!");
	}

	Chat_AddOf(&msg, MSG_TYPE_STATUS_1);

	/* Line 2: score */
	String_InitArray(msg, buf);
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

	if (Survival_Dead) {
		/* Freeze the player in place on death -- gravity still applies */
		if (Entities.CurPlayer) {
			Entities.CurPlayer->Base.Velocity.x = 0.0f;
			Entities.CurPlayer->Base.Velocity.z = 0.0f;
		}
		return true;
	}

	p = Entities.CurPlayer;
	e = &p->Base;

	curY     = e->next.pos.y;
	onGround = e->OnGround;

	if (sv_damageCooldown > 0) sv_damageCooldown--;

	/* ---- Fall damage: 1 HP per block beyond the safe 3 blocks ---- */
	if (onGround) {
		if (!sv_prevOnGround) {
			float fallDist = sv_highestY - curY;
			if (fallDist > 3.0f) {
				int damage = (int)(fallDist - 3.0f);
				if (damage < 1) damage = 1;
				Survival_Damage(damage);
			}
		}
		sv_highestY = curY;
	} else {
		if (curY > sv_highestY) sv_highestY = curY;
	}
	sv_prevOnGround = onGround;

	/* ---- Lava damage: 2 HP every 10 ticks (4 HP/s) ---- */
	if (Entity_TouchesAnyLava(e)) {
		sv_lavaTick++;
		if (sv_lavaTick >= 10) { sv_lavaTick = 0; Survival_Damage(2); }
	} else {
		sv_lavaTick = 0;
	}

	/* ---- Drowning ---- */
	if (Entity_TouchesAnyWater(e)) {
		if (sv_airCooldown > 0) {
			sv_airCooldown--;
		} else {
			sv_airCooldown = 15;
			if (Survival_Air > 0) {
				Survival_Air--;
				if (Survival_Air != sv_lastAir) {
					sv_lastAir = Survival_Air;
					Survival_UpdateHUD();
				}
			}
		}
		if (Survival_Air <= 0) {
			/* 2 HP per second = once every 20 ticks */
			sv_drownTick++;
			if (sv_drownTick >= 20) { sv_drownTick = 0; Survival_Damage(2); }
		}
	} else {
		if (Survival_Air < SURVIVAL_MAX_AIR) {
			Survival_Air += 5;
			if (Survival_Air > SURVIVAL_MAX_AIR) Survival_Air = SURVIVAL_MAX_AIR;
			sv_airCooldown = 0;
			sv_drownTick   = 0;
			if (Survival_Air != sv_lastAir) {
				sv_lastAir = Survival_Air;
				Survival_UpdateHUD();
			}
		}
	}

	if (Survival_Health != sv_lastHealth) {
		sv_lastHealth = Survival_Health;
		Survival_UpdateHUD();
	}

	return true;
}

static struct ScheduledTask2 sv_task;

static void Survival_Init(void) {
	cc_bool enabled = Options_GetBool(OPT_SURVIVAL_MODE, false);
	Survival_Active = enabled && Server.IsSinglePlayer;

	Survival_Health = SURVIVAL_MAX_HEALTH;
	Survival_Air    = SURVIVAL_MAX_AIR;
	Survival_Score  = 0;
	Survival_Dead   = false;
	Survival_Arrows = SURVIVAL_START_ARROWS;
	Survival_InExplosion = false;
	sv_damageCooldown = 0;
	sv_deathTimer     = 0;
	sv_airCooldown    = 0;
	sv_lavaTick       = 0;
	sv_drownTick      = 0;
	sv_prevOnGround   = true;
	sv_highestY       = 0.0f;
	sv_lastHealth     = SURVIVAL_MAX_HEALTH;
	sv_lastAir        = SURVIVAL_MAX_AIR;

	sv_task.interval = 0.05f; /* 20 Hz */
	sv_task.callback = Survival_Tick;
	ScheduledTask2_Add(&sv_task);

	Event_Register_(&UserEvents.BlockChanged, NULL, OnBlockChanged);
	Event_Register_(&UserEvents.BlockChanged, NULL, OnBlockPlaced);
}

static void Survival_Free(void) {
	Event_Unregister_(&UserEvents.BlockChanged, NULL, OnBlockChanged);
	Event_Unregister_(&UserEvents.BlockChanged, NULL, OnBlockPlaced);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_1);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_2);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_3);
}

static void Survival_Reset(void) {
	Survival_Health   = SURVIVAL_MAX_HEALTH;
	Survival_Air      = SURVIVAL_MAX_AIR;
	Survival_Score    = 0;
	Survival_Dead     = false;
	Survival_Arrows   = SURVIVAL_START_ARROWS;
	Survival_InExplosion = false;
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
	Survival_Active = Options_GetBool(OPT_SURVIVAL_MODE, false) && Server.IsSinglePlayer;

	Survival_Health   = SURVIVAL_MAX_HEALTH;
	Survival_Air      = SURVIVAL_MAX_AIR;
	Survival_Score    = 0;
	Survival_Dead     = false;
	Survival_Arrows   = SURVIVAL_START_ARROWS;
	Survival_InExplosion = false;
	sv_damageCooldown = 0;
	sv_deathTimer     = 0;
	sv_airCooldown    = 0;
	sv_lavaTick       = 0;
	sv_drownTick      = 0;
	sv_prevOnGround   = true;
	if (Entities.CurPlayer)
		sv_highestY = Entities.CurPlayer->Base.Position.y;
	sv_lastHealth = SURVIVAL_MAX_HEALTH;
	sv_lastAir    = SURVIVAL_MAX_AIR;

	if (Survival_Active) Survival_UpdateHUD();
}

struct IGameComponent Survival_Component = {
	Survival_Init,          /* Init           */
	Survival_Free,          /* Free           */
	Survival_Reset,         /* Reset          */
	NULL,                   /* OnNewMap       */
	Survival_OnNewMapLoaded /* OnNewMapLoaded */
};
