#include "Survival.h"
#include "SurvivalInv.h"
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
#include "Input.h"
#include "Mobs.h"
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
int     Survival_TNT    = SURVIVAL_START_TNT;
cc_bool Survival_InExplosion;

static int     sv_damageCooldown;
static int     sv_airCooldown;
static int     sv_lavaTick;
static int     sv_drownTick;
static cc_bool sv_prevOnGround;
static float   sv_highestY;
static int     sv_lastHealth;
static int     sv_lastAir;

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

/*########################################################################################################################*
*--------------------------------------------------Explosion--------------------------------------------------------------*
*#########################################################################################################################*/
void Survival_Explode(float cx, float cy, float cz, int radius, int maxDmg) {
	struct LocalPlayer* player;
	Vec3 diff;
	float dist;
	int damage;
	int x, y, z;
	int icx = (int)cx, icy = (int)cy, icz = (int)cz;

	Survival_InExplosion = true;

	/* Destroy non-resistant blocks in a sphere */
	for (x = icx - radius; x <= icx + radius; x++) {
		for (y = icy - radius; y <= icy + radius; y++) {
			for (z = icz - radius; z <= icz + radius; z++) {
				BlockID b;
				float dx = (float)(x - icx);
				float dy = (float)(y - icy);
				float dz = (float)(z - icz);
				if (!World_Contains(x, y, z)) continue;
				if (dx*dx + dy*dy + dz*dz > (float)(radius * radius)) continue;

				b = World_GetBlock(x, y, z);
				if (b == BLOCK_AIR)         continue;
				/* Stone, cobble, bedrock and liquids resist explosions */
				if (b == BLOCK_STONE || b == BLOCK_COBBLE || b == BLOCK_BEDROCK ||
					b == BLOCK_WATER || b == BLOCK_STILL_WATER ||
					b == BLOCK_LAVA  || b == BLOCK_STILL_LAVA)  continue;

				Game_ChangeBlock(x, y, z, BLOCK_AIR);
			}
		}
	}

	Survival_InExplosion = false;

	/* Damage player based on proximity to blast centre */
	player = Entities.CurPlayer;
	if (player && !Survival_Dead) {
		Vec3 centre;
		centre.x = cx; centre.y = cy; centre.z = cz;
		Vec3_Sub(&diff, &player->Base.Position, &centre);
		dist = Math_SqrtF(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
		if (dist < (float)radius) {
			float frac = 1.0f - dist / (float)radius;
			damage = (int)((float)maxDmg * frac);
			if (damage > 0) Survival_Damage(damage);
		}
	}
}

/*########################################################################################################################*
*--------------------------------------------------Block drop table------------------------------------------------------*
*#########################################################################################################################*/
/* Logs drop 3-5 planks; leaves have 1/10 chance to drop a sapling;
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

/*########################################################################################################################*
*--------------------------------------------------Block event handlers--------------------------------------------------*
*#########################################################################################################################*/
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

	/* Game_UpdateBlock in OnBlockPlaced fires a removal event for mushrooms;
	   skip it -- mushroom healing is handled entirely in OnBlockPlaced */
	if (oldBlock == BLOCK_BROWN_SHROOM || oldBlock == BLOCK_RED_SHROOM) return;

	/* TNT: explode instead of dropping */
	if (oldBlock == BLOCK_TNT) {
		float cx = (float)coords.x + 0.5f;
		float cy = (float)coords.y + 0.5f;
		float cz = (float)coords.z + 0.5f;
		Survival_Explode(cx, cy, cz, EXPLOSION_RADIUS, EXPLOSION_MAX_DMG);
		return;
	}

	drop = Survival_GetDrop(oldBlock, &count);
	if (drop == BLOCK_AIR) return;
	if (!SurvivalInv_Add(drop, count))
		Chat_AddRaw("&cInventory full!");
}

/* Intercept mushroom placement as consumption (brown = heal, red = poison).
   Intercept TNT placement to track supply count. */
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
		Chat_AddRaw("&aYou ate a mushroom (+5 HP).");
		Survival_UpdateHUD();
	} else if (newBlock == BLOCK_RED_SHROOM) {
		Game_UpdateBlock(coords.x, coords.y, coords.z, BLOCK_AIR);
		if (Survival_Dead) return;
		Chat_AddRaw("&cYou ate a poisonous mushroom! (-3 HP)");
		Survival_Damage(3);
	} else if (newBlock == BLOCK_TNT) {
		/* Remove one TNT from inventory when placed */
		SurvivalInv_RemoveBlock(BLOCK_TNT, 1);
		Survival_TNT = SurvivalInv_Count(BLOCK_TNT);
		Survival_UpdateHUD();
	}
}

/*########################################################################################################################*
*--------------------------------------------------Arrow firing (Tab key)------------------------------------------------*
*#########################################################################################################################*/
static void OnTabFire(void* obj, int key, cc_bool was, struct InputDevice* device) {
	struct LocalPlayer* player;
	Vec3 eyePos, lookDir, target;
	(void)obj; (void)device;
	if (was) return; /* fresh press only */
	if (key != CCKEY_TAB) return;
	if (!Survival_Active || !Server.IsSinglePlayer || Survival_Dead) return;

	if (Survival_Arrows <= 0) {
		Chat_AddRaw("&eNo arrows left!");
		return;
	}

	player = Entities.CurPlayer;
	if (!player) return;

	eyePos  = Entity_GetEyePosition(&player->Base);
	lookDir = Vec3_GetDirVector(player->Base.Yaw   * MATH_DEG2RAD,
	                            player->Base.Pitch * MATH_DEG2RAD);

	/* Target 50 blocks in the exact look direction -- no upward bias */
	target.x = eyePos.x + lookDir.x * 50.0f;
	target.y = eyePos.y + lookDir.y * 50.0f;
	target.z = eyePos.z + lookDir.z * 50.0f;

	MobArrow_Fire(eyePos, target, true);
	Survival_Arrows--;
	Survival_UpdateHUD();
}

/*########################################################################################################################*
*--------------------------------------------------Public API------------------------------------------------------------*
*#########################################################################################################################*/
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
	cc_string msg; char buf[256];
	int i;

	if (!Survival_Active) return;

	if (Survival_Dead) {
		String_InitArray(msg, buf);
		String_Format1(&msg, "&4GAME OVER  &eScore: %i", &Survival_Score);
		Chat_AddOf(&msg, MSG_TYPE_STATUS_1);
		Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_2);
		return;
	}

	/* Line 1: hearts (and air bubbles when underwater) */
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

	if (Survival_Health <= 4) String_AppendConst(&msg, " &c!");
	Chat_AddOf(&msg, MSG_TYPE_STATUS_1);

	/* Line 2: score, arrows, TNT */
	String_InitArray(msg, buf);
	String_Format2(&msg, "&eScore: %i  &bArrows: %i",
	               &Survival_Score, &Survival_Arrows);
	Survival_TNT = SurvivalInv_Count(BLOCK_TNT);
	if (Survival_TNT > 0) {
		String_AppendConst(&msg, "  &cTNT: ");
		String_AppendInt(&msg, Survival_TNT);
	}
	Chat_AddOf(&msg, MSG_TYPE_STATUS_2);
}

/*########################################################################################################################*
*--------------------------------------------------Survival tick---------------------------------------------------------*
*#########################################################################################################################*/
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

	/* ---- Drowning: 15-tick air drain; 2 HP/s when out of air ---- */
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

/*########################################################################################################################*
*----------------------------------------------------Component Interface-------------------------------------------------*
*#########################################################################################################################*/
static struct ScheduledTask2 sv_task;

static void Sv_ResetState(void) {
	Survival_Health      = SURVIVAL_MAX_HEALTH;
	Survival_Air         = SURVIVAL_MAX_AIR;
	Survival_Score       = 0;
	Survival_Dead        = false;
	Survival_Arrows      = SURVIVAL_START_ARROWS;
	Survival_TNT         = SURVIVAL_START_TNT;
	Survival_InExplosion = false;
	sv_damageCooldown    = 0;
	sv_airCooldown       = 0;
	sv_lavaTick          = 0;
	sv_drownTick         = 0;
	sv_prevOnGround      = true;
	sv_highestY          = 0.0f;
	sv_lastHealth        = SURVIVAL_MAX_HEALTH;
	sv_lastAir           = SURVIVAL_MAX_AIR;
}

static void Survival_Init(void) {
	Survival_Active = Options_GetBool(OPT_SURVIVAL_MODE, false) && Server.IsSinglePlayer;
	Sv_ResetState();

	sv_task.interval = 0.05f; /* 20 Hz */
	sv_task.callback = Survival_Tick;
	ScheduledTask2_Add(&sv_task);

	Event_Register_(&UserEvents.BlockChanged, NULL, OnBlockChanged);
	Event_Register_(&UserEvents.BlockChanged, NULL, OnBlockPlaced);
	Event_Register_(&InputEvents.Down2,       NULL, OnTabFire);
}

static void Survival_Free(void) {
	Event_Unregister_(&UserEvents.BlockChanged, NULL, OnBlockChanged);
	Event_Unregister_(&UserEvents.BlockChanged, NULL, OnBlockPlaced);
	Event_Unregister_(&InputEvents.Down2,       NULL, OnTabFire);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_1);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_2);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_3);
}

static void Survival_Reset(void) {
	Sv_ResetState();
	Survival_UpdateHUD();
}

static void Survival_OnNewMapLoaded(void) {
	Survival_Active = Options_GetBool(OPT_SURVIVAL_MODE, false) && Server.IsSinglePlayer;
	Sv_ResetState();

	if (Entities.CurPlayer)
		sv_highestY = Entities.CurPlayer->Base.Position.y;

	if (Survival_Active) {
		/* Give the player starting TNT supply via inventory */
		SurvivalInv_Add(BLOCK_TNT, SURVIVAL_START_TNT);
		Survival_UpdateHUD();
	}
}

struct IGameComponent Survival_Component = {
	Survival_Init,          /* Init           */
	Survival_Free,          /* Free           */
	Survival_Reset,         /* Reset          */
	NULL,                   /* OnNewMap       */
	Survival_OnNewMapLoaded /* OnNewMapLoaded */
};
