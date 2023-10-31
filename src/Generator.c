#include "Generator.h"
#include "BlockID.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Platform.h"
#include "World.h"
#include "Utils.h"
#include "Game.h"

volatile float Gen_CurrentProgress;
volatile const char* Gen_CurrentState;
volatile cc_bool Gen_Done;
int Gen_Seed;
cc_bool Gen_Vanilla;
BlockRaw* Gen_Blocks;

static void Gen_Init(void) {
	Gen_CurrentProgress = 0.0f;
	Gen_CurrentState    = "";
	Gen_Done   = false;
}


/*########################################################################################################################*
*-----------------------------------------------------Flatgrass gen-------------------------------------------------------*
*#########################################################################################################################*/
static void FlatgrassGen_MapSet(int yBeg, int yEnd, BlockRaw block) {
	cc_uint32 oneY = (cc_uint32)World.OneY;
	BlockRaw* ptr = Gen_Blocks;
	int y, yHeight;

	yBeg = max(yBeg, 0); yEnd = max(yEnd, 0);
	yHeight = (yEnd - yBeg) + 1;
	Gen_CurrentProgress = 0.0f;

	for (y = yBeg; y <= yEnd; y++) {
		Mem_Set(ptr + y * oneY, block, oneY);
		Gen_CurrentProgress = (float)(y - yBeg) / yHeight;
	}
}

void FlatgrassGen_Generate(void) {
	Gen_Init();

	Gen_CurrentState = "Setting air blocks";
	FlatgrassGen_MapSet(World.Height / 2, World.MaxY, BLOCK_AIR);

	Gen_CurrentState = "Setting dirt blocks";
	FlatgrassGen_MapSet(0, World.Height / 2 - 2, BLOCK_DIRT);

	Gen_CurrentState = "Setting grass blocks";
	FlatgrassGen_MapSet(World.Height / 2 - 1, World.Height / 2 - 1, BLOCK_GRASS);

	Gen_Done = true;
}


/*########################################################################################################################*
*---------------------------------------------------Noise generation------------------------------------------------------*
*#########################################################################################################################*/
#define NOISE_TABLE_SIZE 512
static void ImprovedNoise_Init(cc_uint8* p, RNGState* rnd) {
	cc_uint8 tmp;
	int i, j;
	for (i = 0; i < 256; i++) { p[i] = i; }

	/* shuffle randomly using fisher-yates */
	for (i = 0; i < 256; i++) {
		j   = Random_Range(rnd, i, 256);
		tmp = p[i]; p[i] = p[j]; p[j] = tmp;
	}

	for (i = 0; i < 256; i++) {
		p[i + 256] = p[i];
	}
}

static float ImprovedNoise_Calc(const cc_uint8* p, float x, float y) {
	int xFloor, yFloor, X, Y;
	float u, v;
	int A, B, hash;
	float g22, g12, c1;
	float g21, g11, c2;

	xFloor = x >= 0 ? (int)x : (int)x - 1;
	yFloor = y >= 0 ? (int)y : (int)y - 1;
	X = xFloor & 0xFF; Y = yFloor & 0xFF;
	x -= xFloor;       y -= yFloor;

	u = x * x * x * (x * (x * 6 - 15) + 10); /* Fade(x) */
	v = y * y * y * (y * (y * 6 - 15) + 10); /* Fade(y) */
	A = p[X] + Y; B = p[X + 1] + Y;

	/* Normally, calculating Grad involves a function call. However, we can directly pack this table
	(since each value indicates either -1, 0 1) into a set of bit flags. This way we avoid needing
	to call another function that performs branching */
#define xFlags 0x46552222
#define yFlags 0x2222550A

	hash = (p[p[A]] & 0xF) << 1;
	g22  = (((xFlags >> hash) & 3) - 1) * x       + (((yFlags >> hash) & 3) - 1) * y; /* Grad(p[p[A], x, y) */
	hash = (p[p[B]] & 0xF) << 1;
	g12  = (((xFlags >> hash) & 3) - 1) * (x - 1) + (((yFlags >> hash) & 3) - 1) * y; /* Grad(p[p[B], x - 1, y) */
	c1   = g22 + u * (g12 - g22);

	hash = (p[p[A + 1]] & 0xF) << 1;
	g21  = (((xFlags >> hash) & 3) - 1) * x       + (((yFlags >> hash) & 3) - 1) * (y - 1); /* Grad(p[p[A + 1], x, y - 1) */
	hash = (p[p[B + 1]] & 0xF) << 1;
	g11  = (((xFlags >> hash) & 3) - 1) * (x - 1) + (((yFlags >> hash) & 3) - 1) * (y - 1); /* Grad(p[p[B + 1], x - 1, y - 1) */
	c2   = g21 + u * (g11 - g21);

	return c1 + v * (c2 - c1);
}


struct OctaveNoise { cc_uint8 p[8][NOISE_TABLE_SIZE]; int octaves; };
static void OctaveNoise_Init(struct OctaveNoise* n, RNGState* rnd, int octaves) {
	int i;
	n->octaves = octaves;
	
	for (i = 0; i < octaves; i++) {
		ImprovedNoise_Init(n->p[i], rnd);
	}
}

static float OctaveNoise_Calc(const struct OctaveNoise* n, float x, float y) {
	float amplitude = 1, freq = 1;
	float sum = 0;
	int i;

	for (i = 0; i < n->octaves; i++) {
		sum += ImprovedNoise_Calc(n->p[i], x * freq, y * freq) * amplitude;
		amplitude *= 2.0f;
		freq *= 0.5f;
	}
	return sum;
}


struct CombinedNoise { struct OctaveNoise noise1, noise2; };
static void CombinedNoise_Init(struct CombinedNoise* n, RNGState* rnd, int octaves1, int octaves2) {
	OctaveNoise_Init(&n->noise1, rnd, octaves1);
	OctaveNoise_Init(&n->noise2, rnd, octaves2);
}

static float CombinedNoise_Calc(const struct CombinedNoise* n, float x, float y) {
	float offset = OctaveNoise_Calc(&n->noise2, x, y);
	return OctaveNoise_Calc(&n->noise1, x + offset, y);
}


/*########################################################################################################################*
*----------------------------------------------------Notchy map gen-------------------------------------------------------*
*#########################################################################################################################*/
static int waterLevel, minHeight;
static cc_int16* Heightmap;
static RNGState rnd;

static void NotchyGen_FillOblateSpheroid(int x, int y, int z, float radius, BlockRaw block) {
	int xBeg = Math_Floor(max(x - radius, 0));
	int xEnd = Math_Floor(min(x + radius, World.MaxX));
	int yBeg = Math_Floor(max(y - radius, 0));
	int yEnd = Math_Floor(min(y + radius, World.MaxY));
	int zBeg = Math_Floor(max(z - radius, 0));
	int zEnd = Math_Floor(min(z + radius, World.MaxZ));

	float radiusSq = radius * radius;
	int index;
	int xx, yy, zz, dx, dy, dz;

	for (yy = yBeg; yy <= yEnd; yy++) { dy = yy - y;
		for (zz = zBeg; zz <= zEnd; zz++) { dz = zz - z;
			for (xx = xBeg; xx <= xEnd; xx++) { dx = xx - x;

				if ((dx * dx + 2 * dy * dy + dz * dz) < radiusSq) {
					index = World_Pack(xx, yy, zz);
					if (Gen_Blocks[index] == BLOCK_STONE)
						Gen_Blocks[index] = block;
				}
			}
		}
	}
}

#define STACK_FAST 8192
static void NotchyGen_FloodFill(int index, BlockRaw block) {
	int* stack;
	int stack_default[STACK_FAST]; /* avoid allocating memory if possible */
	int count = 0, limit = STACK_FAST;
	int x, y, z;

	stack = stack_default;
	if (index < 0) return; /* y below map, don't bother starting */
	stack[count++] = index;

	while (count) {
		index = stack[--count];

		if (Gen_Blocks[index] != BLOCK_AIR) continue;
		Gen_Blocks[index] = block;

		x = index  % World.Width;
		y = index  / World.OneY;
		z = (index / World.Width) % World.Length;

		/* need to increase stack */
		if (count >= limit - FACE_COUNT) {
			Utils_Resize((void**)&stack, &limit, 4, STACK_FAST, STACK_FAST);
		}

		if (x > 0)          { stack[count++] = index - 1; }
		if (x < World.MaxX) { stack[count++] = index + 1; }
		if (z > 0)          { stack[count++] = index - World.Width; }
		if (z < World.MaxZ) { stack[count++] = index + World.Width; }
		if (y > 0)          { stack[count++] = index - World.OneY; }
	}
	if (limit > STACK_FAST) Mem_Free(stack);
}


static void NotchyGen_CreateHeightmap(void) {
	float hLow, hHigh, height;
	int hIndex = 0, adjHeight;
	int x, z;
	struct CombinedNoise n1, n2;
	struct OctaveNoise n3;

	CombinedNoise_Init(&n1, &rnd, 8, 8);
	CombinedNoise_Init(&n2, &rnd, 8, 8);	
	OctaveNoise_Init(&n3, &rnd, 6);

	Gen_CurrentState = "Building heightmap";
	for (z = 0; z < World.Length; z++) {
		Gen_CurrentProgress = (float)z / World.Length;

		for (x = 0; x < World.Width; x++) {
			hLow   = CombinedNoise_Calc(&n1, x * 1.3f, z * 1.3f) / 6 - 4;
			height = hLow;

			if (OctaveNoise_Calc(&n3, (float)x, (float)z) <= 0) {
				hHigh = CombinedNoise_Calc(&n2, x * 1.3f, z * 1.3f) / 5 + 6;
				height = max(hLow, hHigh);
			}

			height *= 0.5f;
			if (height < 0) height *= 0.8f;

			adjHeight = (int)(height + waterLevel);
			minHeight = min(adjHeight, minHeight);
			Heightmap[hIndex++] = adjHeight;
		}
	}
}

static int NotchyGen_CreateStrataFast(void) {
	cc_uint32 oneY = (cc_uint32)World.OneY;
	int stoneHeight, airHeight;
	int y;

	Gen_CurrentProgress = 0.0f;
	Gen_CurrentState    = "Filling map";
	/* Make lava layer at bottom */
	Mem_Set(Gen_Blocks, BLOCK_STILL_LAVA, oneY);

	/* Invariant: the lowest value dirtThickness can possible be is -14 */
	stoneHeight = minHeight - 14;
	/* We can quickly fill in bottom solid layers */
	for (y = 1; y <= stoneHeight; y++) {
		Mem_Set(Gen_Blocks + y * oneY, BLOCK_STONE, oneY);
		Gen_CurrentProgress = (float)y / World.Height;
	}

	/* Fill in rest of map wih air */
	airHeight = max(0, stoneHeight) + 1;
	for (y = airHeight; y < World.Height; y++) {
		Mem_Set(Gen_Blocks + y * oneY, BLOCK_AIR, oneY);
		Gen_CurrentProgress = (float)y / World.Height;
	}

	/* if stoneHeight is <= 0, then no layer is fully stone */
	return max(stoneHeight, 1);
}

static void NotchyGen_CreateStrata(void) {
	int dirtThickness, dirtHeight;
	int minStoneY, stoneHeight;
	int hIndex = 0, maxY = World.MaxY, index = 0;
	int x, y, z;
	struct OctaveNoise n;

	/* Try to bulk fill bottom of the map if possible */
	minStoneY = NotchyGen_CreateStrataFast();
	OctaveNoise_Init(&n, &rnd, 8);

	Gen_CurrentState = "Creating strata";
	for (z = 0; z < World.Length; z++) {
		Gen_CurrentProgress = (float)z / World.Length;

		for (x = 0; x < World.Width; x++) {
			dirtThickness = (int)(OctaveNoise_Calc(&n, (float)x, (float)z) / 24 - 4);
			dirtHeight    = Heightmap[hIndex++];
			stoneHeight   = dirtHeight + dirtThickness;

			stoneHeight = min(stoneHeight, maxY);
			dirtHeight  = min(dirtHeight,  maxY);

			index = World_Pack(x, minStoneY, z);
			for (y = minStoneY; y <= stoneHeight; y++) {
				Gen_Blocks[index] = BLOCK_STONE; index += World.OneY;
			}

			stoneHeight = max(stoneHeight, 0);
			index = World_Pack(x, (stoneHeight + 1), z);
			for (y = stoneHeight + 1; y <= dirtHeight; y++) {
				Gen_Blocks[index] = BLOCK_DIRT; index += World.OneY;
			}
		}
	}
}

static void NotchyGen_CarveCaves(void) {
	int cavesCount, caveLen;
	float caveX, caveY, caveZ;
	float theta, deltaTheta, phi, deltaPhi;
	float caveRadius, radius;
	int cenX, cenY, cenZ;
	int i, j;

	cavesCount       = World.Volume / 8192;
	Gen_CurrentState = "Carving caves";
	for (i = 0; i < cavesCount; i++) {
		Gen_CurrentProgress = (float)i / cavesCount;

		caveX = (float)Random_Next(&rnd, World.Width);
		caveY = (float)Random_Next(&rnd, World.Height);
		caveZ = (float)Random_Next(&rnd, World.Length);

		caveLen = (int)(Random_Float(&rnd) * Random_Float(&rnd) * 200.0f);
		theta   = Random_Float(&rnd) * 2.0f * MATH_PI; deltaTheta = 0.0f;
		phi     = Random_Float(&rnd) * 2.0f * MATH_PI; deltaPhi   = 0.0f;
		caveRadius = Random_Float(&rnd) * Random_Float(&rnd);

		for (j = 0; j < caveLen; j++) {
			caveX += Math_SinF(theta) * Math_CosF(phi);
			caveZ += Math_CosF(theta) * Math_CosF(phi);
			caveY += Math_SinF(phi);

			theta      = theta + deltaTheta * 0.2f;
			deltaTheta = deltaTheta * 0.9f + Random_Float(&rnd) - Random_Float(&rnd);
			phi        = phi * 0.5f + deltaPhi * 0.25f;
			deltaPhi   = deltaPhi  * 0.75f + Random_Float(&rnd) - Random_Float(&rnd);
			if (Random_Float(&rnd) < 0.25f) continue;

			cenX = (int)(caveX + (Random_Next(&rnd, 4) - 2) * 0.2f);
			cenY = (int)(caveY + (Random_Next(&rnd, 4) - 2) * 0.2f);
			cenZ = (int)(caveZ + (Random_Next(&rnd, 4) - 2) * 0.2f);

			radius = (World.Height - cenY) / (float)World.Height;
			radius = 1.2f + (radius * 3.5f + 1.0f) * caveRadius;
			radius = radius * Math_SinF(j * MATH_PI / caveLen);
			NotchyGen_FillOblateSpheroid(cenX, cenY, cenZ, radius, BLOCK_AIR);
		}
	}
}

static void NotchyGen_CarveOreVeins(float abundance, const char* state, BlockRaw block) {
	int numVeins, veinLen;
	float veinX, veinY, veinZ;
	float theta, deltaTheta, phi, deltaPhi;
	float radius;
	int i, j;

	numVeins         = (int)(World.Volume * abundance / 16384);
	Gen_CurrentState = state;
	for (i = 0; i < numVeins; i++) {
		Gen_CurrentProgress = (float)i / numVeins;

		veinX = (float)Random_Next(&rnd, World.Width);
		veinY = (float)Random_Next(&rnd, World.Height);
		veinZ = (float)Random_Next(&rnd, World.Length);

		veinLen = (int)(Random_Float(&rnd) * Random_Float(&rnd) * 75 * abundance);
		theta = Random_Float(&rnd) * 2.0f * MATH_PI; deltaTheta = 0.0f;
		phi   = Random_Float(&rnd) * 2.0f * MATH_PI; deltaPhi   = 0.0f;

		for (j = 0; j < veinLen; j++) {
			veinX += Math_SinF(theta) * Math_CosF(phi);
			veinZ += Math_CosF(theta) * Math_CosF(phi);
			veinY += Math_SinF(phi);

			theta      = deltaTheta * 0.2f;
			deltaTheta = deltaTheta * 0.9f + Random_Float(&rnd) - Random_Float(&rnd);
			phi        = phi * 0.5f + deltaPhi * 0.25f;
			deltaPhi   = deltaPhi   * 0.9f + Random_Float(&rnd) - Random_Float(&rnd);

			radius = abundance * Math_SinF(j * MATH_PI / veinLen) + 1.0f;
			NotchyGen_FillOblateSpheroid((int)veinX, (int)veinY, (int)veinZ, radius, block);
		}
	}
}

static void NotchyGen_FloodFillWaterBorders(void) {
	int waterY = waterLevel - 1;
	int index1, index2;
	int x, z;
	Gen_CurrentState = "Flooding edge water";

	index1 = World_Pack(0, waterY, 0);
	index2 = World_Pack(0, waterY, World.Length - 1);
	for (x = 0; x < World.Width; x++) {
		Gen_CurrentProgress = 0.0f + ((float)x / World.Width) * 0.5f;

		NotchyGen_FloodFill(index1, BLOCK_STILL_WATER);
		NotchyGen_FloodFill(index2, BLOCK_STILL_WATER);
		index1++; index2++;
	}

	index1 = World_Pack(0,             waterY, 0);
	index2 = World_Pack(World.Width - 1, waterY, 0);
	for (z = 0; z < World.Length; z++) {
		Gen_CurrentProgress = 0.5f + ((float)z / World.Length) * 0.5f;

		NotchyGen_FloodFill(index1, BLOCK_STILL_WATER);
		NotchyGen_FloodFill(index2, BLOCK_STILL_WATER);
		index1 += World.Width; index2 += World.Width;
	}
}

static void NotchyGen_FloodFillWater(void) {
	int numSources;
	int i, x, y, z;

	numSources       = World.Width * World.Length / 800;
	Gen_CurrentState = "Flooding water";
	for (i = 0; i < numSources; i++) {
		Gen_CurrentProgress = (float)i / numSources;

		x = Random_Next(&rnd, World.Width);
		z = Random_Next(&rnd, World.Length);
		y = waterLevel - Random_Range(&rnd, 1, 3);
		NotchyGen_FloodFill(World_Pack(x, y, z), BLOCK_STILL_WATER);
	}
}

static void NotchyGen_FloodFillLava(void) {
	int numSources;
	int i, x, y, z;

	numSources       = World.Width * World.Length / 20000;
	Gen_CurrentState = "Flooding lava";
	for (i = 0; i < numSources; i++) {
		Gen_CurrentProgress = (float)i / numSources;

		x = Random_Next(&rnd, World.Width);
		z = Random_Next(&rnd, World.Length);
		y = (int)((waterLevel - 3) * Random_Float(&rnd) * Random_Float(&rnd));
		NotchyGen_FloodFill(World_Pack(x, y, z), BLOCK_STILL_LAVA);
	}
}

static void NotchyGen_CreateSurfaceLayer(void) {	
	int hIndex = 0, index;
	BlockRaw above;
	int x, y, z;
	struct OctaveNoise n1, n2;

	OctaveNoise_Init(&n1, &rnd, 8);
	OctaveNoise_Init(&n2, &rnd, 8);

	Gen_CurrentState = "Creating surface";
	for (z = 0; z < World.Length; z++) {
		Gen_CurrentProgress = (float)z / World.Length;

		for (x = 0; x < World.Width; x++) {
			y = Heightmap[hIndex++];
			if (y < 0 || y >= World.Height) continue;

			index = World_Pack(x, y, z);
			above = y >= World.MaxY ? BLOCK_AIR : Gen_Blocks[index + World.OneY];

			/* TODO: update heightmap */
			if (above == BLOCK_STILL_WATER && (OctaveNoise_Calc(&n2, (float)x, (float)z) > 12)) {
				Gen_Blocks[index] = BLOCK_GRAVEL;
			} else if (above == BLOCK_AIR) {
				Gen_Blocks[index] = (y <= waterLevel && (OctaveNoise_Calc(&n1, (float)x, (float)z) > 8)) ? BLOCK_SAND : BLOCK_GRASS;
			}
		}
	}
}

static void NotchyGen_PlantFlowers(void) {
	int numPatches;
	BlockRaw block;
	int patchX,  patchZ;
	int flowerX, flowerY, flowerZ;
	int i, j, k, index;

	if (Game_Version.Version < VERSION_0023) return;
	numPatches       = World.Width * World.Length / 3000;
	Gen_CurrentState = "Planting flowers";

	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (float)i / numPatches;

		block  = (BlockRaw)(BLOCK_DANDELION + Random_Next(&rnd, 2));
		patchX = Random_Next(&rnd, World.Width);
		patchZ = Random_Next(&rnd, World.Length);

		for (j = 0; j < 10; j++) {
			flowerX = patchX; flowerZ = patchZ;
			for (k = 0; k < 5; k++) {
				flowerX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				flowerZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);

				if (!World_ContainsXZ(flowerX, flowerZ)) continue;
				flowerY = Heightmap[flowerZ * World.Width + flowerX] + 1;
				if (flowerY <= 0 || flowerY >= World.Height) continue;

				index = World_Pack(flowerX, flowerY, flowerZ);
				if (Gen_Blocks[index] == BLOCK_AIR && Gen_Blocks[index - World.OneY] == BLOCK_GRASS)
					Gen_Blocks[index] = block;
			}
		}
	}
}

static void NotchyGen_PlantMushrooms(void) {
	int numPatches, groundHeight;
	BlockRaw block;
	int patchX, patchY, patchZ;
	int mushX,  mushY,  mushZ;
	int i, j, k, index;

	if (Game_Version.Version < VERSION_0023) return;
	numPatches       = World.Volume / 2000;
	Gen_CurrentState = "Planting mushrooms";

	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (float)i / numPatches;

		block  = (BlockRaw)(BLOCK_BROWN_SHROOM + Random_Next(&rnd, 2));
		patchX = Random_Next(&rnd, World.Width);
		patchY = Random_Next(&rnd, World.Height);
		patchZ = Random_Next(&rnd, World.Length);

		for (j = 0; j < 20; j++) {
			mushX = patchX; mushY = patchY; mushZ = patchZ;
			for (k = 0; k < 5; k++) {
				mushX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				mushZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);

				if (!World_ContainsXZ(mushX, mushZ)) continue;
				groundHeight = Heightmap[mushZ * World.Width + mushX];
				if (mushY >= (groundHeight - 1)) continue;

				index = World_Pack(mushX, mushY, mushZ);
				if (Gen_Blocks[index] == BLOCK_AIR && Gen_Blocks[index - World.OneY] == BLOCK_STONE)
					Gen_Blocks[index] = block;
			}
		}
	}
}

static void NotchyGen_PlantTrees(void) {
	int numPatches;
	int patchX, patchZ;
	int treeX, treeY, treeZ;
	int treeHeight, index, count;
	BlockRaw under;
	int i, j, k, m;

	IVec3 coords[TREE_MAX_COUNT];
	BlockRaw blocks[TREE_MAX_COUNT];

	Tree_Blocks = Gen_Blocks;
	Tree_Rnd    = &rnd;

	numPatches       = World.Width * World.Length / 4000;
	Gen_CurrentState = "Planting trees";
	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (float)i / numPatches;

		patchX = Random_Next(&rnd, World.Width);
		patchZ = Random_Next(&rnd, World.Length);

		for (j = 0; j < 20; j++) {
			treeX = patchX; treeZ = patchZ;
			for (k = 0; k < 20; k++) {
				treeX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				treeZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);

				if (!World_ContainsXZ(treeX, treeZ) || Random_Float(&rnd) >= 0.25) continue;
				treeY = Heightmap[treeZ * World.Width + treeX] + 1;
				if (treeY >= World.Height) continue;
				treeHeight = 5 + Random_Next(&rnd, 3);

				index = World_Pack(treeX, treeY, treeZ);
				under = treeY > 0 ? Gen_Blocks[index - World.OneY] : BLOCK_AIR;

				if (under == BLOCK_GRASS && TreeGen_CanGrow(treeX, treeY, treeZ, treeHeight)) {
					count = TreeGen_Grow(treeX, treeY, treeZ, treeHeight, coords, blocks);

					for (m = 0; m < count; m++) {
						index = World_Pack(coords[m].X, coords[m].Y, coords[m].Z);
						Gen_Blocks[index] = blocks[m];
					}
				}
			}
		}
	}
}

void NotchyGen_Generate(void) {
	Gen_Init();
	Heightmap = (cc_int16*)Mem_Alloc(World.Width * World.Length, 2, "gen heightmap");

	Random_Seed(&rnd, Gen_Seed);
	waterLevel = World.Height / 2;	
	minHeight  = World.Height;

	NotchyGen_CreateHeightmap();
	NotchyGen_CreateStrata();
	NotchyGen_CarveCaves();
	NotchyGen_CarveOreVeins(0.9f, "Carving coal ore", BLOCK_COAL_ORE);
	NotchyGen_CarveOreVeins(0.7f, "Carving iron ore", BLOCK_IRON_ORE);
	NotchyGen_CarveOreVeins(0.5f, "Carving gold ore", BLOCK_GOLD_ORE);

	NotchyGen_FloodFillWaterBorders();
	NotchyGen_FloodFillWater();
	NotchyGen_FloodFillLava();

	NotchyGen_CreateSurfaceLayer();
	NotchyGen_PlantFlowers();
	NotchyGen_PlantMushrooms();
	NotchyGen_PlantTrees();

	Mem_Free(Heightmap);
	Heightmap = NULL;
	Gen_Done  = true;
}


/*########################################################################################################################*
*----------------------------------------------------Tree generation------------------------------------------------------*
*#########################################################################################################################*/
BlockRaw* Tree_Blocks;
RNGState* Tree_Rnd;

cc_bool TreeGen_CanGrow(int treeX, int treeY, int treeZ, int treeHeight) {
	int baseHeight = treeHeight - 4;
	int index;
	int x, y, z;

	/* check tree base */
	for (y = treeY; y < treeY + baseHeight; y++) {
		for (z = treeZ - 1; z <= treeZ + 1; z++) {
			for (x = treeX - 1; x <= treeX + 1; x++) {

				if (!World_Contains(x, y, z)) return false;
				index = World_Pack(x, y, z);
				if (Tree_Blocks[index] != BLOCK_AIR) return false;
			}
		}
	}

	/* and also check canopy */
	for (y = treeY + baseHeight; y < treeY + treeHeight; y++) {
		for (z = treeZ - 2; z <= treeZ + 2; z++) {
			for (x = treeX - 2; x <= treeX + 2; x++) {

				if (!World_Contains(x, y, z)) return false;
				index = World_Pack(x, y, z);
				if (Tree_Blocks[index] != BLOCK_AIR) return false;
			}
		}
	}
	return true;
}

#define TreeGen_Place(x, y, z, block)\
coords[count].X = (x); coords[count].Y = (y); coords[count].Z = (z);\
blocks[count] = block; count++;

int TreeGen_Grow(int treeX, int treeY, int treeZ, int height, IVec3* coords, BlockRaw* blocks) {
	int topStart = treeY + (height - 2);
	int count = 0;
	int xx, zz, x, y, z;

	/* leaves bottom layer */
	for (y = treeY + (height - 4); y < topStart; y++) {
		for (zz = -2; zz <= 2; zz++) {
			for (xx = -2; xx <= 2; xx++) {
				x = treeX + xx; z = treeZ + zz;

				if (Math_AbsI(xx) == 2 && Math_AbsI(zz) == 2) {
					if (Random_Float(Tree_Rnd) >= 0.5f) {
						TreeGen_Place(x, y, z, BLOCK_LEAVES);
					}
				} else {
					TreeGen_Place(x, y, z, BLOCK_LEAVES);
				}
			}
		}
	}

	/* leaves top layer */
	for (; y < treeY + height; y++) {
		for (zz = -1; zz <= 1; zz++) {
			for (xx = -1; xx <= 1; xx++) {
				x = xx + treeX; z = zz + treeZ;

				if (xx == 0 || zz == 0) {
					TreeGen_Place(x, y, z, BLOCK_LEAVES);
				} else if (y == topStart && Random_Float(Tree_Rnd) >= 0.5f) {
					TreeGen_Place(x, y, z, BLOCK_LEAVES);
				}
			}
		}
	}

	/* then place trunk */
	for (y = 0; y < height - 1; y++) {
		TreeGen_Place(treeX, treeY + y, treeZ, BLOCK_LOG);
	}
	return count;
}
