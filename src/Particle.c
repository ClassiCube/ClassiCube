#include "Particle.h"
#include "Block.h"
#include "World.h"
#include "ExtMath.h"
#include "Lighting.h"
#include "Entity.h"
#include "TexturePack.h"
#include "Graphics.h"
#include "Funcs.h"
#include "Game.h"
#include "Event.h"


/*########################################################################################################################*
*------------------------------------------------------Particle base------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Particles_TexId, Particles_VB;
#define PARTICLES_MAX 600
static RNGState rnd;
static cc_bool hitTerrain;
typedef cc_bool (*CanPassThroughFunc)(BlockID b);

void Particle_DoRender(const Vec2* size, const Vec3* pos, const TextureRec* rec, PackedCol col, struct VertexTextured* v) {
	struct Matrix* view;
	float sX, sY;
	Vec3 centre;
	float aX, aY, aZ, bX, bY, bZ;

	sX = size->X * 0.5f; sY = size->Y * 0.5f;
	centre = *pos; centre.Y += sY;
	view   = &Gfx.View;
	
	aX = view->row1.X * sX; aY = view->row2.X * sX; aZ = view->row3.X * sX; /* right * size.X * 0.5f */
	bX = view->row1.Y * sY; bY = view->row2.Y * sY; bZ = view->row3.Y * sY; /* up    * size.Y * 0.5f */

	v->X = centre.X - aX - bX; v->Y = centre.Y - aY - bY; v->Z = centre.Z - aZ - bZ; v->Col = col; v->U = rec->U1; v->V = rec->V2; v++;
	v->X = centre.X - aX + bX; v->Y = centre.Y - aY + bY; v->Z = centre.Z - aZ + bZ; v->Col = col; v->U = rec->U1; v->V = rec->V1; v++;
	v->X = centre.X + aX + bX; v->Y = centre.Y + aY + bY; v->Z = centre.Z + aZ + bZ; v->Col = col; v->U = rec->U2; v->V = rec->V1; v++;
	v->X = centre.X + aX - bX; v->Y = centre.Y + aY - bY; v->Z = centre.Z + aZ - bZ; v->Col = col; v->U = rec->U2; v->V = rec->V2; v++;
}

static cc_bool CollidesHor(Vec3* nextPos, BlockID block) {
	Vec3 horPos = Vec3_Create3((float)Math_Floor(nextPos->X), 0.0f, (float)Math_Floor(nextPos->Z));
	Vec3 min, max;
	Vec3_Add(&min, &Blocks.MinBB[block], &horPos);
	Vec3_Add(&max, &Blocks.MaxBB[block], &horPos);
	return nextPos->X >= min.X && nextPos->Z >= min.Z && nextPos->X < max.X && nextPos->Z < max.Z;
}

static BlockID GetBlock(int x, int y, int z) {
	if (World_Contains(x, y, z)) return World_GetBlock(x, y, z);

	if (y >= Env.EdgeHeight)  return BLOCK_AIR;
	if (y >= Env_SidesHeight) return Env.EdgeBlock;
	return Env.SidesBlock;
}

static cc_bool ClipY(struct Particle* p, int y, cc_bool topFace, CanPassThroughFunc canPassThrough) {
	BlockID block;
	Vec3 minBB, maxBB;
	float collideY;
	cc_bool collideVer;

	if (y < 0) {
		p->nextPos.Y = ENTITY_ADJUSTMENT; 
		p->lastPos.Y = ENTITY_ADJUSTMENT;

		Vec3_Set(p->velocity, 0,0,0);
		hitTerrain = true;
		return false;
	}

	block = GetBlock((int)p->nextPos.X, y, (int)p->nextPos.Z);
	if (canPassThrough(block)) return true;
	minBB = Blocks.MinBB[block]; maxBB = Blocks.MaxBB[block];

	collideY   = y + (topFace ? maxBB.Y : minBB.Y);
	collideVer = topFace ? (p->nextPos.Y < collideY) : (p->nextPos.Y > collideY);

	if (collideVer && CollidesHor(&p->nextPos, block)) {
		float adjust = topFace ? ENTITY_ADJUSTMENT : -ENTITY_ADJUSTMENT;
		p->lastPos.Y = collideY + adjust;
		p->nextPos.Y = p->lastPos.Y;

		Vec3_Set(p->velocity, 0,0,0);
		hitTerrain = true;
		return false;
	}
	return true;
}

static cc_bool IntersectsBlock(struct Particle* p, CanPassThroughFunc canPassThrough) {
	BlockID cur = GetBlock((int)p->nextPos.X, (int)p->nextPos.Y, (int)p->nextPos.Z);
	float minY  = Math_Floor(p->nextPos.Y) + Blocks.MinBB[cur].Y;
	float maxY  = Math_Floor(p->nextPos.Y) + Blocks.MaxBB[cur].Y;

	return !canPassThrough(cur) && p->nextPos.Y >= minY && p->nextPos.Y < maxY && CollidesHor(&p->nextPos, cur);
}

static cc_bool PhysicsTick(struct Particle* p, float gravity, CanPassThroughFunc canPassThrough, double delta) {
	Vec3 velocity;
	int y, begY, endY;

	p->lastPos = p->nextPos;
	if (IntersectsBlock(p, canPassThrough)) return true;

	p->velocity.Y -= gravity * (float)delta;
	begY = Math_Floor(p->nextPos.Y);
	
	Vec3_Mul1(&velocity, &p->velocity, (float)delta * 3.0f);
	Vec3_Add(&p->nextPos, &p->nextPos, &velocity);
	endY = Math_Floor(p->nextPos.Y);

	if (p->velocity.Y > 0.0f) {
		/* don't test block we are already in */
		for (y = begY + 1; y <= endY && ClipY(p, y, false, canPassThrough); y++) {}
	} else {
		for (y = begY; y >= endY && ClipY(p, y, true, canPassThrough); y--) {}
	}

	p->lifetime -= (float)delta;
	return p->lifetime < 0.0f;
}


/*########################################################################################################################*
*-------------------------------------------------------Rain particle-----------------------------------------------------*
*#########################################################################################################################*/
static struct Particle rain_Particles[PARTICLES_MAX];
static int rain_count;
static TextureRec rain_rec = { 2.0f/128.0f, 14.0f/128.0f, 5.0f/128.0f, 16.0f/128.0f };

static cc_bool RainParticle_CanPass(BlockID block) {
	cc_uint8 draw = Blocks.Draw[block];
	return draw == DRAW_GAS || draw == DRAW_SPRITE;
}

static cc_bool RainParticle_Tick(struct Particle* p, double delta) {
	hitTerrain = false;
	return PhysicsTick(p, 3.5f, RainParticle_CanPass, delta) || hitTerrain;
}

static void RainParticle_Render(struct Particle* p, float t, struct VertexTextured* vertices) {
	Vec3 pos;
	Vec2 size;
	PackedCol col;
	int x, y, z;

	Vec3_Lerp(&pos, &p->lastPos, &p->nextPos, t);
	size.X = p->size * 0.015625f; size.Y = size.X;

	x = Math_Floor(pos.X); y = Math_Floor(pos.Y); z = Math_Floor(pos.Z);
	col = Lighting.Color(x, y, z);
	Particle_DoRender(&size, &pos, &rain_rec, col, vertices);
}

static void Rain_Render(float t) {
	struct VertexTextured* data;
	int i;
	if (!rain_count) return;
	
	data = (struct VertexTextured*)Gfx_LockDynamicVb(Particles_VB, 
										VERTEX_FORMAT_TEXTURED, rain_count * 4);
	for (i = 0; i < rain_count; i++) {
		RainParticle_Render(&rain_Particles[i], t, data);
		data += 4;
	}

	Gfx_BindTexture(Particles_TexId);
	Gfx_UnlockDynamicVb(Particles_VB);
	Gfx_DrawVb_IndexedTris(rain_count * 4);
}

static void Rain_RemoveAt(int i) {
	for (; i < rain_count - 1; i++) {
		rain_Particles[i] = rain_Particles[i + 1];
	}
	rain_count--;
}

static void Rain_Tick(double delta) {
	int i;
	for (i = 0; i < rain_count; i++) {
		if (RainParticle_Tick(&rain_Particles[i], delta)) {
			Rain_RemoveAt(i); i--;
		}
	}
}


/*########################################################################################################################*
*------------------------------------------------------Terrain particle---------------------------------------------------*
*#########################################################################################################################*/
struct TerrainParticle {
	struct Particle base;
	TextureRec rec;
	TextureLoc texLoc;
	BlockID block;
};

static struct TerrainParticle terrain_particles[PARTICLES_MAX];
static int terrain_count;
static cc_uint16 terrain_1DCount[ATLAS1D_MAX_ATLASES];
static cc_uint16 terrain_1DIndices[ATLAS1D_MAX_ATLASES];

static cc_bool TerrainParticle_CanPass(BlockID block) {
	cc_uint8 draw = Blocks.Draw[block];
	return draw == DRAW_GAS || draw == DRAW_SPRITE || Blocks.IsLiquid[block];
}

static cc_bool TerrainParticle_Tick(struct TerrainParticle* p, double delta) {
	return PhysicsTick(&p->base, Blocks.ParticleGravity[p->block], TerrainParticle_CanPass, delta);
}

static void TerrainParticle_Render(struct TerrainParticle* p, float t, struct VertexTextured* vertices) {
	PackedCol col = PACKEDCOL_WHITE;
	Vec3 pos;
	Vec2 size;
	int x, y, z;

	Vec3_Lerp(&pos, &p->base.lastPos, &p->base.nextPos, t);
	size.X = p->base.size * 0.015625f; size.Y = size.X;
	
	if (!Blocks.FullBright[p->block]) {
		x = Math_Floor(pos.X); y = Math_Floor(pos.Y); z = Math_Floor(pos.Z);
		col = Lighting.Color_XSide(x, y, z);
	}

	Block_Tint(col, p->block);
	Particle_DoRender(&size, &pos, &p->rec, col, vertices);
}

static void Terrain_Update1DCounts(void) {
	int i, index;

	for (i = 0; i < ATLAS1D_MAX_ATLASES; i++) {
		terrain_1DCount[i]   = 0;
		terrain_1DIndices[i] = 0;
	}
	for (i = 0; i < terrain_count; i++) {
		index = Atlas1D_Index(terrain_particles[i].texLoc);
		terrain_1DCount[index] += 4;
	}
	for (i = 1; i < Atlas1D.Count; i++) {
		terrain_1DIndices[i] = terrain_1DIndices[i - 1] + terrain_1DCount[i - 1];
	}
}

static void Terrain_Render(float t) {
	struct VertexTextured* data;
	struct VertexTextured* ptr;
	int offset = 0;
	int i, index;
	if (!terrain_count) return;

	data = (struct VertexTextured*)Gfx_LockDynamicVb(Particles_VB, 
										VERTEX_FORMAT_TEXTURED, terrain_count * 4);
	Terrain_Update1DCounts();
	for (i = 0; i < terrain_count; i++) {
		index = Atlas1D_Index(terrain_particles[i].texLoc);
		ptr   = data + terrain_1DIndices[index];

		TerrainParticle_Render(&terrain_particles[i], t, ptr);
		terrain_1DIndices[index] += 4;
	}

	Gfx_UnlockDynamicVb(Particles_VB);
	for (i = 0; i < Atlas1D.Count; i++) {
		int partCount = terrain_1DCount[i];
		if (!partCount) continue;

		Gfx_BindTexture(Atlas1D.TexIds[i]);
		Gfx_DrawVb_IndexedTris_Range(partCount, offset);
		offset += partCount;
	}
}

static void Terrain_RemoveAt(int i) {
	for (; i < terrain_count - 1; i++) {
		terrain_particles[i] = terrain_particles[i + 1];
	}
	terrain_count--;
}

static void Terrain_Tick(double delta) {
	int i;
	for (i = 0; i < terrain_count; i++) {
		if (TerrainParticle_Tick(&terrain_particles[i], delta)) {
			Terrain_RemoveAt(i); i--;
		}
	}
}

/*########################################################################################################################*
*-------------------------------------------------------Custom particle---------------------------------------------------*
*#########################################################################################################################*/
struct CustomParticle {
	struct Particle base;
	int effectId;
	float totalLifespan;
};

struct CustomParticleEffect Particles_CustomEffects[256];
static struct CustomParticle custom_particles[PARTICLES_MAX];
static int custom_count;
static cc_uint8 collideFlags;
#define EXPIRES_UPON_TOUCHING_GROUND (1 << 0)
#define SOLID_COLLIDES  (1 << 1)
#define LIQUID_COLLIDES (1 << 2)
#define LEAF_COLLIDES   (1 << 3)

static cc_bool CustomParticle_CanPass(BlockID block) {
	cc_uint8 draw, collide;
	
	draw = Blocks.Draw[block];
	if (draw == DRAW_TRANSPARENT_THICK && !(collideFlags & LEAF_COLLIDES)) return true;

	collide = Blocks.Collide[block];
	if (collide == COLLIDE_SOLID  && (collideFlags & SOLID_COLLIDES))  return false;
	if (collide == COLLIDE_LIQUID && (collideFlags & LIQUID_COLLIDES)) return false;
	return true;
}

static cc_bool CustomParticle_Tick(struct CustomParticle* p, double delta) {
	struct CustomParticleEffect* e = &Particles_CustomEffects[p->effectId];
	hitTerrain   = false;
	collideFlags = e->collideFlags;

	return PhysicsTick(&p->base, e->gravity, CustomParticle_CanPass, delta)
		|| (hitTerrain && (e->collideFlags & EXPIRES_UPON_TOUCHING_GROUND));
}

static void CustomParticle_Render(struct CustomParticle* p, float t, struct VertexTextured* vertices) {
	struct CustomParticleEffect* e = &Particles_CustomEffects[p->effectId];
	Vec3 pos;
	Vec2 size;
	PackedCol col;
	TextureRec rec = e->rec;
	int x, y, z;

	float time_lived = p->totalLifespan - p->base.lifetime;
	int curFrame = Math_Floor(e->frameCount * (time_lived / p->totalLifespan));
	float shiftU = curFrame * (rec.U2 - rec.U1);

	rec.U1 += shiftU;/* * 0.0078125f; */
	rec.U2 += shiftU;/* * 0.0078125f; */

	Vec3_Lerp(&pos, &p->base.lastPos, &p->base.nextPos, t);
	size.X = p->base.size; size.Y = size.X;

	x = Math_Floor(pos.X); y = Math_Floor(pos.Y); z = Math_Floor(pos.Z);
	col = e->fullBright ? PACKEDCOL_WHITE : Lighting.Color(x, y, z);
	col = PackedCol_Tint(col, e->tintCol);

	Particle_DoRender(&size, &pos, &rec, col, vertices);
}

static void Custom_Render(float t) {
	struct VertexTextured* data;
	int i;
	if (!custom_count) return;

	data = (struct VertexTextured*)Gfx_LockDynamicVb(Particles_VB, 
										VERTEX_FORMAT_TEXTURED, custom_count * 4);
	for (i = 0; i < custom_count; i++) {
		CustomParticle_Render(&custom_particles[i], t, data);
		data += 4;
	}

	Gfx_BindTexture(Particles_TexId);
	Gfx_UnlockDynamicVb(Particles_VB);
	Gfx_DrawVb_IndexedTris(custom_count * 4);
}

static void Custom_RemoveAt(int i) {
	for (; i < custom_count - 1; i++) {
		custom_particles[i] = custom_particles[i + 1];
	}
	custom_count--;
}

static void Custom_Tick(double delta) {
	int i;
	for (i = 0; i < custom_count; i++) {
		if (CustomParticle_Tick(&custom_particles[i], delta)) {
			Custom_RemoveAt(i); i--;
		}
	}
}


/*########################################################################################################################*
*--------------------------------------------------------Particles--------------------------------------------------------*
*#########################################################################################################################*/
void Particles_Render(float t) {
	if (!terrain_count && !rain_count && !custom_count) return;
	if (Gfx.LostContext) return;

	Gfx_SetAlphaTest(true);

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Terrain_Render(t);
	Rain_Render(t);
	Custom_Render(t);

	Gfx_SetAlphaTest(false);
}

static void Particles_Tick(struct ScheduledTask* task) {
	double delta = task->interval;
	Terrain_Tick(delta);
	Rain_Tick(delta);
	Custom_Tick(delta);
}

void Particles_BreakBlockEffect(IVec3 coords, BlockID old, BlockID now) {
	struct TerrainParticle* p;
	TextureLoc loc;
	int texIndex;
	TextureRec baseRec, rec;
	Vec3 origin, minBB, maxBB;

	/* texture UV variables */
	float uScale, vScale, maxU2, maxV2;
	int minX, minZ, maxX, maxZ;
	int minU, minV, maxU, maxV;
	int maxUsedU, maxUsedV;
	
	/* per-particle variables */
	float cellX, cellY, cellZ;
	Vec3 cell;
	int x, y, z, type;

	if (now != BLOCK_AIR || Blocks.Draw[old] == DRAW_GAS) return;
	IVec3_ToVec3(&origin, &coords);
	loc = Block_Tex(old, FACE_XMIN);
	
	baseRec = Atlas1D_TexRec(loc, 1, &texIndex);
	uScale  = (1.0f/16.0f); vScale = (1.0f/16.0f) * Atlas1D.InvTileSize;

	minBB = Blocks.MinBB[old];    maxBB = Blocks.MaxBB[old];
	minX  = (int)(minBB.X * 16); maxX  = (int)(maxBB.X * 16);
	minZ  = (int)(minBB.Z * 16); maxZ  = (int)(maxBB.Z * 16);

	minU = min(minX, minZ); minV = (int)(16 - maxBB.Y * 16);
	maxU = min(maxX, maxZ); maxV = (int)(16 - minBB.Y * 16);
	/* This way we can avoid creating particles which outside the bounds and need to be clamped */
	maxUsedU = maxU; maxUsedV = maxV;
	if (minU < 12 && maxU > 12) maxUsedU = 12;
	if (minV < 12 && maxV > 12) maxUsedV = 12;

	#define GRID_SIZE 4
	/* gridOffset gives the centre of the cell on a grid */
	#define CELL_CENTRE ((1.0f / GRID_SIZE) * 0.5f)

	maxU2 = baseRec.U1 + maxU * uScale;
	maxV2 = baseRec.V1 + maxV * vScale;
	for (x = 0; x < GRID_SIZE; x++) {
		for (y = 0; y < GRID_SIZE; y++) {
			for (z = 0; z < GRID_SIZE; z++) {

				cellX = (float)x / GRID_SIZE; cellY = (float)y / GRID_SIZE; cellZ = (float)z / GRID_SIZE;
				cell  = Vec3_Create3(CELL_CENTRE + cellX, CELL_CENTRE / 2 + cellY, CELL_CENTRE + cellZ);
				if (cell.X < minBB.X || cell.X > maxBB.X || cell.Y < minBB.Y
					|| cell.Y > maxBB.Y || cell.Z < minBB.Z || cell.Z > maxBB.Z) continue;

				if (terrain_count == PARTICLES_MAX) Terrain_RemoveAt(0);
				p = &terrain_particles[terrain_count++];

				/* centre random offset around [-0.2, 0.2] */
				p->base.velocity.X = CELL_CENTRE + (cellX - 0.5f) + (Random_Float(&rnd) * 0.4f - 0.2f);
				p->base.velocity.Y = CELL_CENTRE + (cellY - 0.0f) + (Random_Float(&rnd) * 0.4f - 0.2f);
				p->base.velocity.Z = CELL_CENTRE + (cellZ - 0.5f) + (Random_Float(&rnd) * 0.4f - 0.2f);

				rec = baseRec;
				rec.U1 = baseRec.U1 + Random_Range(&rnd, minU, maxUsedU) * uScale;
				rec.V1 = baseRec.V1 + Random_Range(&rnd, minV, maxUsedV) * vScale;
				rec.U2 = rec.U1 + 4 * uScale;
				rec.V2 = rec.V1 + 4 * vScale;
				rec.U2 = min(rec.U2, maxU2) - 0.01f * uScale;
				rec.V2 = min(rec.V2, maxV2) - 0.01f * vScale;
		
				Vec3_Add(&p->base.lastPos, &origin, &cell);
				p->base.nextPos  = p->base.lastPos;
				p->base.lifetime = 0.3f + Random_Float(&rnd) * 1.2f;

				p->rec    = rec;
				p->texLoc = loc;
				p->block  = old;
				type = Random_Next(&rnd, 30);
				p->base.size = type >= 28 ? 12 : (type >= 25 ? 10 : 8);
			}
		}
	}
}

void Particles_RainSnowEffect(float x, float y, float z) {
	struct Particle* p;
	int i, type;

	for (i = 0; i < 2; i++) {
		if (rain_count == PARTICLES_MAX) Rain_RemoveAt(0);
		p = &rain_Particles[rain_count++];

		p->velocity.X = Random_Float(&rnd) * 0.8f - 0.4f; /* [-0.4, 0.4] */
		p->velocity.Z = Random_Float(&rnd) * 0.8f - 0.4f;
		p->velocity.Y = Random_Float(&rnd) + 0.4f;

		p->lastPos.X = x + Random_Float(&rnd); /* [0.0, 1.0] */
		p->lastPos.Y = y + Random_Float(&rnd) * 0.1f + 0.01f;
		p->lastPos.Z = z + Random_Float(&rnd);

		p->nextPos  = p->lastPos;
		p->lifetime = 40.0f;

		type = Random_Next(&rnd, 30);
		p->size = type >= 28 ? 2 : (type >= 25 ? 4 : 3);
	}
}

void Particles_CustomEffect(int effectID, float x, float y, float z, float originX, float originY, float originZ) {
	struct CustomParticle* p;
	struct CustomParticleEffect* e = &Particles_CustomEffects[effectID];
	int i, count = e->particleCount;
	Vec3 offset, delta;
	float d;

	for (i = 0; i < count; i++) {
		if (custom_count == PARTICLES_MAX) Custom_RemoveAt(0);
		p = &custom_particles[custom_count++];
		p->effectId = effectID;

		offset.X = Random_Float(&rnd) - 0.5f;
		offset.Y = Random_Float(&rnd) - 0.5f;
		offset.Z = Random_Float(&rnd) - 0.5f;
		Vec3_Normalise(&offset);

		/* See https://karthikkaranth.me/blog/generating-random-points-in-a-sphere/ */
		/* 'Using normally distributed random numbers' */
		d  = Random_Float(&rnd);
		d  = Math_Exp(Math_Log(d) / 3.0); /* d^1/3 for better distribution */
		d *= e->spread;

		p->base.lastPos.X = x + offset.X * d;
		p->base.lastPos.Y = y + offset.Y * d;
		p->base.lastPos.Z = z + offset.Z * d;
		
		Vec3 origin = { originX, originY, originZ };
		Vec3_Sub(&delta, &p->base.lastPos, &origin);
		Vec3_Normalise(&delta);

		p->base.velocity.X = delta.X * e->speed;
		p->base.velocity.Y = delta.Y * e->speed;
		p->base.velocity.Z = delta.Z * e->speed;

		p->base.nextPos  = p->base.lastPos;
		p->base.lifetime = e->baseLifetime + (e->baseLifetime * e->lifetimeVariation) * ((Random_Float(&rnd) - 0.5f) * 2);
		p->totalLifespan = p->base.lifetime;

		p->base.size = e->size + (e->size * e->sizeVariation) * ((Random_Float(&rnd) - 0.5f) * 2);

		/* Don't spawn custom particle inside a block (otherwise it appears */
		/*   for a few frames, then disappears in first PhysicsTick call)*/
		collideFlags = e->collideFlags;
		if (IntersectsBlock(&p->base, CustomParticle_CanPass)) custom_count--;
	}
}


/*########################################################################################################################*
*---------------------------------------------------Particles component---------------------------------------------------*
*#########################################################################################################################*/
static void ParticlesPngProcess(struct Stream* stream, const cc_string* name) {
	Game_UpdateTexture(&Particles_TexId, stream, name, NULL);
}
static struct TextureEntry particles_entry = { "particles.png", ParticlesPngProcess };


static void OnContextLost(void* obj) {
	Gfx_DeleteDynamicVb(&Particles_VB); 

	if (Gfx.ManagedTextures) return;
	Gfx_DeleteTexture(&Particles_TexId);
}
static void OnContextRecreated(void* obj) {
	Gfx_RecreateDynamicVb(&Particles_VB, VERTEX_FORMAT_TEXTURED, PARTICLES_MAX * 4);
}
static void OnBreakBlockEffect_Handler(void* obj, IVec3 coords, BlockID old, BlockID now) {
	Particles_BreakBlockEffect(coords, old, now);
}

static void OnInit(void) {
	ScheduledTask_Add(GAME_DEF_TICKS, Particles_Tick);
	Random_SeedFromCurrentTime(&rnd);
	OnContextRecreated(NULL);
	TextureEntry_Register(&particles_entry);

	Event_Register_(&UserEvents.BlockChanged,    NULL, OnBreakBlockEffect_Handler);
	Event_Register_(&GfxEvents.ContextLost,      NULL, OnContextLost);
	Event_Register_(&GfxEvents.ContextRecreated, NULL, OnContextRecreated);
}

static void OnFree(void) { OnContextLost(NULL); }

static void OnReset(void) { rain_count = 0; terrain_count = 0; custom_count = 0; }

struct IGameComponent Particles_Component = {
	OnInit,  /* Init  */
	OnFree,  /* Free  */
	OnReset, /* Reset */
	OnReset  /* OnNewMap */
};
