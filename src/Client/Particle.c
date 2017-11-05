#include "Particle.h"
#include "Block.h"
#include "World.h"
#include "ExtMath.h"
#include "Lighting.h"
#include "Matrix.h"
#include "Entity.h"
#include "Random.h"
#include "TerrainAtlas.h"
#include "GraphicsAPI.h"
#include "GraphicsCommon.h"
#include "Funcs.h"

void Particle_DoRender(Vector2* size, Vector3* pos, TextureRec* rec, PackedCol col, VertexP3fT2fC4b** vertices) {
	Real32 sX = size->X * 0.5f, sY = size->Y * 0.5f;
	Vector3 centre = *pos; centre.Y += sY;
	VertexP3fT2fC4b* ptr = *vertices;

	Matrix* view = &Game_View;
	Real32 aX, aY, aZ, bX, bY, bZ;
	aX = view->Row0.X * sX; aY = view->Row1.X * sX; aZ = view->Row2.X * sX; /* right * size.X * 0.5f */
	bX = view->Row0.Y * sY; bY = view->Row1.Y * sY; bZ = view->Row2.Y * sY; /* up    * size.Y * 0.5f */
	VertexP3fT2fC4b v; v.Col = col;

	v.X = centre.X - aX - bX; v.Y = centre.Y - aY - bY; v.Z = centre.Z - aZ - bZ;
	v.U = rec->U1; v.V = rec->V2; ptr[0] = v;

	v.X = centre.X - aX + bX; v.Y = centre.Y - aY + bY; v.Z = centre.Z - aZ + bZ;
				   v.V = rec->V1; ptr[1] = v;

	v.X = centre.X + aX + bX; v.Y = centre.Y + aY + bY; v.Z = centre.Z + aZ + bZ;
	v.U = rec->U2;                ptr[2] = v;

	v.X = centre.X + aX - bX; v.Y = centre.Y + aY - bY; v.Z = centre.Z + aZ - bZ;
				   v.V = rec->V2; ptr[3] = v;
}

void Particle_Reset(Particle* p, Vector3 pos, Vector3 velocity, Real32 lifetime) {
	p->LastPos = pos; p->NextPos = pos;
	p->Velocity = velocity;
	p->Lifetime = lifetime;
	p->HitTerrain = false;
}

bool Particle_CanPass(BlockID block, bool throughLiquids) {
	DrawType draw = Block_Draw[block];
	return draw == DrawType_Gas || draw == DrawType_Sprite || (throughLiquids && Block_IsLiquid(block));
}

bool Particle_CollideHor(Vector3* nextPos, BlockID block) {
	Vector3 horPos = Vector3_Create3((Real32)Math_Floor(nextPos->X), 0.0f, (Real32)Math_Floor(nextPos->Z));
	Vector3 min, max;
	Vector3_Add(&min, &Block_MinBB[block], &horPos);
	Vector3_Add(&max, &Block_MaxBB[block], &horPos);
	return nextPos->X >= min.X && nextPos->Z >= min.Z && nextPos->X < max.X && nextPos->Z < max.Z;
}

BlockID Particle_GetBlock(Int32 x, Int32 y, Int32 z) {
	if (World_IsValidPos(x, y, z)) { return World_GetBlock(x, y, z); }

	if (y >= WorldEnv_EdgeHeight) return BlockID_Air;
	if (y >= WorldEnv_SidesHeight) return WorldEnv_EdgeBlock;
	return WorldEnv_SidesBlock;
}

bool Particle_TestY(Particle* p, Int32 y, bool topFace, bool throughLiquids) {
	if (y < 0) {
		p->NextPos.Y = ENTITY_ADJUSTMENT; p->LastPos.Y = ENTITY_ADJUSTMENT;
		p->Velocity = Vector3_Zero;
		p->HitTerrain = true;
		return false;
	}

	BlockID block = Particle_GetBlock((Int32)p->NextPos.X, y, (Int32)p->NextPos.Z);
	if (Particle_CanPass(block, throughLiquids)) return true;
	Vector3 minBB = Block_MinBB[block];
	Vector3 maxBB = Block_MaxBB[block];
	Real32 collideY = y + (topFace ? maxBB.Y : minBB.Y);
	bool collideVer = topFace ? (p->NextPos.Y < collideY) : (p->NextPos.Y > collideY);

	if (collideVer && Particle_CollideHor(&p->NextPos, block)) {
		Real32 adjust = topFace ? ENTITY_ADJUSTMENT : -ENTITY_ADJUSTMENT;
		p->LastPos.Y = collideY + adjust;
		p->NextPos.Y = p->LastPos.Y;
		p->Velocity = Vector3_Zero;
		p->HitTerrain = true;
		return false;
	}
	return true;
}

bool Particle_PhysicsTick(Particle* p, Real32 gravity, bool throughLiquids, Real64 delta) {
	p->HitTerrain = false;
	p->LastPos = p->NextPos;

	BlockID cur = Particle_GetBlock((Int32)p->NextPos.X, (Int32)p->NextPos.Y, (Int32)p->NextPos.Z);
	Real32 minY = Math_Floor(p->NextPos.Y) + Block_MinBB[cur].Y;
	Real32 maxY = Math_Floor(p->NextPos.Y) + Block_MaxBB[cur].Y;
	if (!Particle_CanPass(cur, throughLiquids) && p->NextPos.Y >= minY
		&& p->NextPos.Y < maxY && Particle_CollideHor(&p->NextPos, cur)) {
		return true;
	}

	p->Velocity.Y -= gravity * (Real32)delta;
	Int32 startY = Math_Floor(p->NextPos.Y);
	Vector3 velocity;
	Vector3_Multiply1(&velocity, &p->Velocity, (Real32)delta * 3.0f);
	Vector3_Add(&p->NextPos, &p->NextPos, &velocity);
	Int32 endY = Math_Floor(p->NextPos.Y);

	Int32 y;
	if (p->Velocity.Y > 0.0f) {
		/* don't test block we are already in */
		for (y = startY + 1; y <= endY && Particle_TestY(p, y, false, throughLiquids); y++) {}
	} else {
		for (y = startY; y >= endY && Particle_TestY(p, y, true, throughLiquids); y--) {}
	}

	p->Lifetime -= (Real32)delta;
	return p->Lifetime < 0.0f;
}

bool RainParticle_Tick(RainParticle* p, Real64 delta) {
	bool dies = Particle_PhysicsTick(&p->Base, 3.5f, false, delta);
	return p->Base.HitTerrain || dies;
}

Vector2 Rain_Big = { 1.00f / 16.0f, 1.00f / 16.0f };
Vector2 Rain_Small = { 0.75f / 16.0f, 0.75f / 16.0f };
Vector2 Rain_Tiny = { 0.50f / 16.0f, 0.50f / 16.0f };
TextureRec Rain_Rec = { 2.0f / 128.0f, 14.0f / 128.0f, 3.0f / 128.0f, 2.0f / 128.0f };
void RainParticle_Render(RainParticle* p, Real32 t, VertexP3fT2fC4b** vertices) {
	Vector3 pos;
	Vector3_Lerp(&pos, &p->Base.LastPos, &p->Base.NextPos, t);
	Vector2* size = p->Big ? &Rain_Big : (p->Tiny ? &Rain_Tiny : &Rain_Small);

	Int32 x = Math_Floor(pos.X), y = Math_Floor(pos.Y), z = Math_Floor(pos.Z);
	PackedCol col = World_IsValidPos(x, y, z) ? Lighting_Col(x, y, z) : Lighting_Outside;
	Particle_DoRender(size, &pos, &Rain_Rec, col, vertices);
}

bool TerrainParticle_Tick(TerrainParticle* p, Real64 delta) {
	return Particle_PhysicsTick(&p->Base, 5.4f, true, delta);
}

Vector2 Terrain_Size = { 1.0f / 8.0f, 1.0f / 8.0f };
void TerrainParticle_Render(TerrainParticle* p, Real32 t, VertexP3fT2fC4b** vertices) {
	Vector3 pos;
	Vector3_Lerp(&pos, &p->Base.LastPos, &p->Base.NextPos, t);
	PackedCol col = PACKEDCOL_WHITE;
	if (!Block_FullBright[p->Block]) {
		Int32 x = Math_Floor(pos.X), y = Math_Floor(pos.Y), z = Math_Floor(pos.Z);
		col = World_IsValidPos(x, y, z) ? Lighting_Col_XSide(x, y, z) : Lighting_OutsideXSide;
	}

	if (Block_Tinted[p->Block]) {
		PackedCol tintCol = Block_FogColour[p->Block];
		col.R = (UInt8)(col.R * tintCol.R / 255);
		col.G = (UInt8)(col.G * tintCol.G / 255);
		col.B = (UInt8)(col.B * tintCol.B / 255);
	}
	Particle_DoRender(&Terrain_Size, &pos, &p->Rec, col, vertices);
}


GfxResourceID Particles_TexId, Particles_VB;
#define PARTICLES_MAX 600
TerrainParticle Terrain_Particles[PARTICLES_MAX];
RainParticle Rain_Particles[PARTICLES_MAX];
Int32 Terrain_Count, Rain_Count;
UInt16 Terrain_1DCount[ATLAS1D_MAX_ATLASES_COUNT];
UInt16 Terrain_1DIndices[ATLAS1D_MAX_ATLASES_COUNT];
Random rnd;

void Particles_FileChanged(Stream* stream) {
	String particlesPng = String_FromConst("particles.png");
	if (String_Equals(&stream->Name, &particlesPng)) {
		//Game_UpdateTexture(&Particles_TexId, stream, false);
	}
}

void Particles_ContextLost(void) { Gfx_DeleteVb(&Particles_VB); }
void Particles_ContextRecreated(void) {
	Particles_VB = Gfx_CreateDynamicVb(VertexFormat_P3fT2fC4b, PARTICLES_MAX * 4);
}

void Particles_Init(void) {
	Random_InitFromCurrentTime(&rnd);
	Event_RegisterBlock(&UserEvents_BlockChanged, Particles_BreakBlockEffect);
	Event_RegisterStream(&TextureEvents_FileChanged, Particles_FileChanged);

	Particles_ContextRecreated();
	Event_RegisterVoid(&GfxEvents_ContextLost, Particles_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, Particles_ContextRecreated);
}

void Particles_Reset(void) { Rain_Count = 0; Terrain_Count = 0; }

void Particles_Free(void) {
	Gfx_DeleteTexture(&Particles_TexId);
	Event_UnregisterBlock(&UserEvents_BlockChanged, Particles_BreakBlockEffect);
	Event_UnregisterStream(&TextureEvents_FileChanged, Particles_FileChanged);

	Particles_ContextLost();
	Event_UnregisterVoid(&GfxEvents_ContextLost, Particles_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, Particles_ContextRecreated);
}


IGameComponent Particles_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = Particles_Init;
	comp.Reset = Particles_Reset;
	comp.OnNewMap = Particles_Reset;
	comp.Free = Particles_Free;
	return comp;
}

void Terrain_Update1DCounts(void) {
	Int32 i;
	for (i = 0; i < Atlas1D_Count; i++) {
		Terrain_1DCount[i] = 0;
		Terrain_1DIndices[i] = 0;
	}
	for (i = 0; i < Terrain_Count; i++) {
		Int32 index = Atlas1D_Index(Terrain_Particles[i].TexLoc);
		Terrain_1DCount[index] += 4;
	}
	for (i = 1; i < Atlas1D_Count; i++) {
		Terrain_1DIndices[i] = Terrain_1DIndices[i - 1] + Terrain_1DCount[i - 1];
	}
}

void Terrain_Render(Real32 t) {
	if (Terrain_Count == 0) return;
	VertexP3fT2fC4b vertices[PARTICLES_MAX * 4];
	Terrain_Update1DCounts();
	Int32 i;
	for (i = 0; i < Terrain_Count; i++) {
		Int32 index = Atlas1D_Index(Terrain_Particles[i].TexLoc);
		VertexP3fT2fC4b* ptr = &vertices[Terrain_1DIndices[index]];
		TerrainParticle_Render(&Terrain_Particles[i], t, &ptr);
		Terrain_1DIndices[index] += 4;
	}

	Gfx_SetDynamicVbData(Particles_VB, vertices, Terrain_Count * 4);
	Int32 offset = 0;
	for (i = 0; i < Atlas1D_Count; i++) {
		UInt16 partCount = Terrain_1DCount[i];
		if (partCount == 0) continue;

		Gfx_BindTexture(Atlas1D_TexIds[i]);
		Gfx_DrawVb_IndexedTris_Range(partCount, offset);
		offset += partCount;
	}
}

void Rain_Render(Real32 t) {
	if (Rain_Count == 0) return;
	VertexP3fT2fC4b vertices[PARTICLES_MAX * 4];
	Int32 i;
	VertexP3fT2fC4b* ptr = vertices;
	for (i = 0; i < Rain_Count; i++) {
		RainParticle_Render(&Rain_Particles[i], t, &ptr);
	}

	Gfx_BindTexture(Particles_TexId);
	GfxCommon_UpdateDynamicVb_IndexedTris(Particles_VB, vertices, Rain_Count * 4);
}

void Particles_Render(Real64 delta, Real32 t) {
	if (Terrain_Count == 0 && Rain_Count == 0) return;
	if (Gfx_LostContext) return;

	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);

	Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);
	Terrain_Render(t);
	Rain_Render(t);

	Gfx_SetAlphaTest(false);
	Gfx_SetTexturing(false);
}

void Terrain_RemoveAt(Int32 index) {
	TerrainParticle removed = Terrain_Particles[index];
	Int32 i;
	for (i = index; i < Terrain_Count - 1; i++) {
		Terrain_Particles[i] = Terrain_Particles[i + 1];
	}
	Terrain_Particles[Terrain_Count - 1] = removed;
	Terrain_Count--;
}

void Terrain_Tick(Real64 delta) {
	Int32 i;
	for (i = 0; i < Terrain_Count; i++) {
		if (TerrainParticle_Tick(&Terrain_Particles[i], delta)) {
			Terrain_RemoveAt(i); i--;
		}
	}
}

void Rain_RemoveAt(Int32 index) {
	RainParticle removed = Rain_Particles[index];
	Int32 i;
	for (i = index; i < Rain_Count - 1; i++) {
		Rain_Particles[i] = Rain_Particles[i + 1];
	}
	Rain_Particles[Rain_Count - 1] = removed;
	Rain_Count--;
}

void Rain_Tick(Real64 delta) {
	Int32 i;
	for (i = 0; i < Rain_Count; i++) {
		if (RainParticle_Tick(&Rain_Particles[i], delta)) {
			Rain_RemoveAt(i); i--;
		}
	}
}

void Particles_Tick(ScheduledTask* task) {
	Terrain_Tick(task->Interval);
	Rain_Tick(task->Interval);
}

void Particles_BreakBlockEffect(Vector3I coords, BlockID oldBlock, BlockID block) {
	if (block != BlockID_Air) return;
	block = oldBlock;

	Vector3 worldPos;
	Vector3I_ToVector3(&worldPos, &coords);
	TextureLoc texLoc = Block_GetTexLoc(block, Face_XMin);
	Int32 texIndex;
	TextureRec baseRec = Atlas1D_TexRec(texLoc, 1, &texIndex);
	Real32 uScale = (1.0f / 16.0f), vScale = (1.0f / 16.0f) * Atlas1D_InvElementSize;

	Vector3 minBB = Block_MinBB[block];
	Vector3 maxBB = Block_MaxBB[block];
	Int32 minX = (Int32)(minBB.X * 16), minZ = (Int32)(minBB.Z * 16);
	Int32 maxX = (Int32)(maxBB.X * 16), maxZ = (Int32)(maxBB.Z * 16);

	Int32 minU = min(minX, minZ), maxU = min(maxX, maxZ);
	Int32 minV = (Int32)(16 - maxBB.Y * 16), maxV = (Int32)(16 - minBB.Y * 16);
	Int32 maxUsedU = maxU, maxUsedV = maxV;
	/* This way we can avoid creating particles which outside the bounds and need to be clamped */
	if (minU < 12 && maxU > 12) maxUsedU = 12;
	if (minV < 12 && maxV > 12) maxUsedV = 12;

	#define GRID_SIZE 4
	/* gridOffset gives the centre of the cell on a grid */
	#define CELL_CENTRE ((1.0f / GRID_SIZE) * 0.5f)

	Int32 x, y, z;
	Real32 maxU2 = baseRec.U1 + maxU * uScale;
	Real32 maxV2 = baseRec.V1 + maxV * vScale;
	for (x = 0; x < GRID_SIZE; x++)
		for (y = 0; y < GRID_SIZE; y++)
			for (z = 0; z < GRID_SIZE; z++) {
				Real32 cellX = (Real32)x / GRID_SIZE, cellY = (Real32)y / GRID_SIZE, cellZ = (Real32)z / GRID_SIZE;
				Vector3 cell = Vector3_Create3(CELL_CENTRE + cellX, CELL_CENTRE / 2 + cellY, CELL_CENTRE + cellZ);
				if (cell.X < minBB.X || cell.X > maxBB.X || cell.Y < minBB.Y
					|| cell.Y > maxBB.Y || cell.Z < minBB.Z || cell.Z > maxBB.Z) continue;

				Vector3 velocity; /* centre random offset around [-0.2, 0.2] */
				velocity.X = CELL_CENTRE + (cellX - 0.5f) + (Random_Float(&rnd) * 0.4f - 0.2f); 
				velocity.Y = CELL_CENTRE + (cellY - 0.0f) + (Random_Float(&rnd) * 0.4f - 0.2f);
				velocity.Z = CELL_CENTRE + (cellZ - 0.5f) + (Random_Float(&rnd) * 0.4f - 0.2f);

				TextureRec rec = baseRec;
				rec.U1 = baseRec.U1 + Random_Range(&rnd, minU, maxUsedU) * uScale;
				rec.V1 = baseRec.V1 + Random_Range(&rnd, minV, maxUsedV) * vScale;
				rec.U2 = rec.U1 + 4 * uScale; 
				rec.V2 = rec.V1 + 4 * vScale;
				rec.U2 = min(rec.U2, maxU2) - 0.01f * uScale;
				rec.V2 = min(rec.V2, maxV2) - 0.01f * vScale;

				if (Terrain_Count == PARTICLES_MAX) Terrain_RemoveAt(0);
				TerrainParticle* p = &Terrain_Particles[Terrain_Count - 1];
				Terrain_Count++; 
				Real32 life = 0.3f + Random_Float(&rnd) * 1.2f;

				Vector3 pos;
				Vector3_Add(&pos, &worldPos, &cell);
				Particle_Reset(&p->Base, pos, velocity, life);
				p->Rec = rec;
				p->TexLoc = (TextureLoc)texLoc;
				p->Block = block;
			}
}

void Particles_RainSnowEffect(Vector3 pos) {
	Vector3 startPos = pos;
	Int32 i;
	for (i = 0; i < 2; i++) {
		Real32 velX = Random_Float(&rnd) * 0.8f - 0.4f; /* [-0.4, 0.4] */
		Real32 velZ = Random_Float(&rnd) * 0.8f - 0.4f;
		Real32 velY = Random_Float(&rnd) + 0.4f;
		Vector3 velocity = Vector3_Create3(velX, velY, velZ);

		Vector3 offset;
		offset.X = Random_Float(&rnd);  /* [0.0, 1.0] */
		offset.Y = Random_Float(&rnd) * 0.1f + 0.01f;
		offset.Z = Random_Float(&rnd);

		if (Rain_Count == PARTICLES_MAX) Rain_RemoveAt(0);
		RainParticle* p = &Rain_Particles[Rain_Count - 1];
		Rain_Count++;

		Vector3_Add(&pos, &startPos, &offset);
		Particle_Reset(&p->Base, pos, velocity, 40.0f);		
		p->Big  = Random_Range(&rnd, 0, 20) >= 18;
		p->Tiny = Random_Range(&rnd, 0, 30) >= 28;
	}
}