#include "MapGenerator.h"
#include "BlockID.h"
#include "ErrorHandler.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Platform.h"

static int Gen_MaxX, Gen_MaxY, Gen_MaxZ, Gen_Volume, Gen_OneY;
#define Gen_Pack(x, y, z) (((y) * Gen_Length + (z)) * Gen_Width + (x))

static void Gen_Init(void) {
	Gen_MaxX = Gen_Width - 1; Gen_MaxY = Gen_Height - 1; Gen_MaxZ = Gen_Length - 1;
	Gen_Volume = Gen_Width * Gen_Length * Gen_Height;
	Gen_OneY   = Gen_Width * Gen_Length;

	Gen_CurrentProgress = 0.0f;
	Gen_CurrentState    = "";

	Gen_Blocks = Mem_Alloc(Gen_Volume, 1, "map blocks for gen");
	Gen_Done   = false;
}


/*########################################################################################################################*
*-----------------------------------------------------Flatgrass gen-------------------------------------------------------*
*#########################################################################################################################*/
static void FlatgrassGen_MapSet(int yBeg, int yEnd, BlockRaw block) {
	uint32_t oneY = (uint32_t)Gen_OneY;
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
	FlatgrassGen_MapSet(Gen_Height / 2, Gen_MaxY, BLOCK_AIR);

	Gen_CurrentState = "Setting dirt blocks";
	FlatgrassGen_MapSet(0, Gen_Height / 2 - 2, BLOCK_DIRT);

	Gen_CurrentState = "Setting grass blocks";
	FlatgrassGen_MapSet(Gen_Height / 2 - 1, Gen_Height / 2 - 1, BLOCK_GRASS);

	Gen_Done = true;
}


/*########################################################################################################################*
*----------------------------------------------------Notchy map gen-------------------------------------------------------*
*#########################################################################################################################*/
static int waterLevel, minHeight;
static int16_t* Heightmap;
static RNGState rnd;

static void NotchyGen_FillOblateSpheroid(int x, int y, int z, float radius, BlockRaw block) {
	int xBeg = Math_Floor(max(x - radius, 0));
	int xEnd = Math_Floor(min(x + radius, Gen_MaxX));
	int yBeg = Math_Floor(max(y - radius, 0));
	int yEnd = Math_Floor(min(y + radius, Gen_MaxY));
	int zBeg = Math_Floor(max(z - radius, 0));
	int zEnd = Math_Floor(min(z + radius, Gen_MaxZ));

	float radiusSq = radius * radius;
	int index;
	int xx, yy, zz, dx, dy, dz;

	for (yy = yBeg; yy <= yEnd; yy++) { dy = yy - y;
		for (zz = zBeg; zz <= zEnd; zz++) { dz = zz - z;
			for (xx = xBeg; xx <= xEnd; xx++) { dx = xx - x;

				if ((dx * dx + 2 * dy * dy + dz * dz) < radiusSq) {
					index = Gen_Pack(xx, yy, zz);
					if (Gen_Blocks[index] == BLOCK_STONE)
						Gen_Blocks[index] = block;
				}
			}
		}
	}
}

#define Stack_Push(index)\
stack[stack_size++] = index;\
if (stack_size == 32768) {\
	ErrorHandler_Fail("NotchyGen_FloodFail - stack limit hit");\
}

static void NotchyGen_FloodFill(int startIndex, BlockRaw block) {
	/* This is way larger size than I actually have seen used, but we play it safe here.*/
	int32_t stack[32768];
	int stack_size = 0, index;
	int x, y, z;

	if (startIndex < 0) return; /* y below map, immediately ignore */
	Stack_Push(startIndex);

	while (stack_size > 0) {
		stack_size--;
		index = stack[stack_size];

		if (Gen_Blocks[index] != BLOCK_AIR) continue;
		Gen_Blocks[index] = block;

		x = index  % Gen_Width;
		y = index  / Gen_OneY;
		z = (index / Gen_Width) % Gen_Length;

		if (x > 0)        { Stack_Push(index - 1); }
		if (x < Gen_MaxX) { Stack_Push(index + 1); }
		if (z > 0)        { Stack_Push(index - Gen_Width); }
		if (z < Gen_MaxZ) { Stack_Push(index + Gen_Width); }
		if (y > 0)        { Stack_Push(index - Gen_OneY); }
	}
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
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = (float)z / Gen_Length;

		for (x = 0; x < Gen_Width; x++) {
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
	uint32_t oneY = (uint32_t)Gen_OneY;
	int stoneHeight, airHeight;
	int y;

	Gen_CurrentProgress = 0.0f;
	Gen_CurrentState    = "Filling map";
	/* Make lava layer at bottom */
	Mem_Set(Gen_Blocks, BLOCK_LAVA, oneY);

	/* Invariant: the lowest value dirtThickness can possible be is -14 */
	stoneHeight = minHeight - 14;
	/* We can quickly fill in bottom solid layers */
	for (y = 1; y <= stoneHeight; y++) {
		Mem_Set(Gen_Blocks + y * oneY, BLOCK_STONE, oneY);
		Gen_CurrentProgress = (float)y / Gen_Height;
	}

	/* Fill in rest of map wih air */
	airHeight = max(0, stoneHeight) + 1;
	for (y = airHeight; y < Gen_Height; y++) {
		Mem_Set(Gen_Blocks + y * oneY, BLOCK_AIR, oneY);
		Gen_CurrentProgress = (float)y / Gen_Height;
	}

	/* if stoneHeight is <= 0, then no layer is fully stone */
	return max(stoneHeight, 1);
}

static void NotchyGen_CreateStrata(void) {
	int dirtThickness, dirtHeight;
	int minStoneY, stoneHeight;
	int hIndex = 0, maxY = Gen_MaxY, index = 0;
	int x, y, z;
	struct OctaveNoise n;

	/* Try to bulk fill bottom of the map if possible */
	minStoneY = NotchyGen_CreateStrataFast();
	OctaveNoise_Init(&n, &rnd, 8);

	Gen_CurrentState = "Creating strata";
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = (float)z / Gen_Length;

		for (x = 0; x < Gen_Width; x++) {
			dirtThickness = (int)(OctaveNoise_Calc(&n, (float)x, (float)z) / 24 - 4);
			dirtHeight    = Heightmap[hIndex++];
			stoneHeight   = dirtHeight + dirtThickness;

			stoneHeight = min(stoneHeight, maxY);
			dirtHeight  = min(dirtHeight,  maxY);

			index = Gen_Pack(x, minStoneY, z);
			for (y = minStoneY; y <= stoneHeight; y++) {
				Gen_Blocks[index] = BLOCK_STONE; index += Gen_OneY;
			}

			stoneHeight = max(stoneHeight, 0);
			index = Gen_Pack(x, (stoneHeight + 1), z);
			for (y = stoneHeight + 1; y <= dirtHeight; y++) {
				Gen_Blocks[index] = BLOCK_DIRT; index += Gen_OneY;
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

	cavesCount       = Gen_Volume / 8192;
	Gen_CurrentState = "Carving caves";
	for (i = 0; i < cavesCount; i++) {
		Gen_CurrentProgress = (float)i / cavesCount;

		caveX = (float)Random_Next(&rnd, Gen_Width);
		caveY = (float)Random_Next(&rnd, Gen_Height);
		caveZ = (float)Random_Next(&rnd, Gen_Length);

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

			radius = (Gen_Height - cenY) / (float)Gen_Height;
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

	numVeins         = (int)(Gen_Volume * abundance / 16384);
	Gen_CurrentState = state;
	for (i = 0; i < numVeins; i++) {
		Gen_CurrentProgress = (float)i / numVeins;

		veinX = (float)Random_Next(&rnd, Gen_Width);
		veinY = (float)Random_Next(&rnd, Gen_Height);
		veinZ = (float)Random_Next(&rnd, Gen_Length);

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

	index1 = Gen_Pack(0, waterY, 0);
	index2 = Gen_Pack(0, waterY, Gen_Length - 1);
	for (x = 0; x < Gen_Width; x++) {
		Gen_CurrentProgress = 0.0f + ((float)x / Gen_Width) * 0.5f;

		NotchyGen_FloodFill(index1, BLOCK_WATER);
		NotchyGen_FloodFill(index2, BLOCK_WATER);
		index1++; index2++;
	}

	index1 = Gen_Pack(0,             waterY, 0);
	index2 = Gen_Pack(Gen_Width - 1, waterY, 0);
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = 0.5f + ((float)z / Gen_Length) * 0.5f;

		NotchyGen_FloodFill(index1, BLOCK_WATER);
		NotchyGen_FloodFill(index2, BLOCK_WATER);
		index1 += Gen_Width; index2 += Gen_Width;
	}
}

static void NotchyGen_FloodFillWater(void) {
	int numSources;
	int i, x, y, z;

	numSources       = Gen_Width * Gen_Length / 800;
	Gen_CurrentState = "Flooding water";
	for (i = 0; i < numSources; i++) {
		Gen_CurrentProgress = (float)i / numSources;

		x = Random_Next(&rnd, Gen_Width);
		z = Random_Next(&rnd, Gen_Length);
		y = waterLevel - Random_Range(&rnd, 1, 3);
		NotchyGen_FloodFill(Gen_Pack(x, y, z), BLOCK_WATER);
	}
}

static void NotchyGen_FloodFillLava(void) {
	int numSources;
	int i, x, y, z;

	numSources       = Gen_Width * Gen_Length / 20000;
	Gen_CurrentState = "Flooding lava";
	for (i = 0; i < numSources; i++) {
		Gen_CurrentProgress = (float)i / numSources;

		x = Random_Next(&rnd, Gen_Width);
		z = Random_Next(&rnd, Gen_Length);
		y = (int)((waterLevel - 3) * Random_Float(&rnd) * Random_Float(&rnd));
		NotchyGen_FloodFill(Gen_Pack(x, y, z), BLOCK_LAVA);
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
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = (float)z / Gen_Length;

		for (x = 0; x < Gen_Width; x++) {
			y = Heightmap[hIndex++];
			if (y < 0 || y >= Gen_Height) continue;

			index = Gen_Pack(x, y, z);
			above = y >= Gen_MaxY ? BLOCK_AIR : Gen_Blocks[index + Gen_OneY];

			/* TODO: update heightmap */
			if (above == BLOCK_WATER && (OctaveNoise_Calc(&n2, (float)x, (float)z) > 12)) {
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

	numPatches       = Gen_Width * Gen_Length / 3000;
	Gen_CurrentState = "Planting flowers";
	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (float)i / numPatches;

		block  = (BlockRaw)(BLOCK_DANDELION + Random_Next(&rnd, 2));
		patchX = Random_Next(&rnd, Gen_Width);
		patchZ = Random_Next(&rnd, Gen_Length);

		for (j = 0; j < 10; j++) {
			flowerX = patchX; flowerZ = patchZ;
			for (k = 0; k < 5; k++) {
				flowerX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				flowerZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);

				if (flowerX < 0 || flowerZ < 0 || flowerX >= Gen_Width || flowerZ >= Gen_Length) continue;
				flowerY = Heightmap[flowerZ * Gen_Width + flowerX] + 1;
				if (flowerY <= 0 || flowerY >= Gen_Height) continue;

				index = Gen_Pack(flowerX, flowerY, flowerZ);
				if (Gen_Blocks[index] == BLOCK_AIR && Gen_Blocks[index - Gen_OneY] == BLOCK_GRASS)
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

	numPatches       = Gen_Volume / 2000;
	Gen_CurrentState = "Planting mushrooms";
	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (float)i / numPatches;

		block  = (BlockRaw)(BLOCK_BROWN_SHROOM + Random_Next(&rnd, 2));
		patchX = Random_Next(&rnd, Gen_Width);
		patchY = Random_Next(&rnd, Gen_Height);
		patchZ = Random_Next(&rnd, Gen_Length);

		for (j = 0; j < 20; j++) {
			mushX = patchX; mushY = patchY; mushZ = patchZ;
			for (k = 0; k < 5; k++) {
				mushX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				mushZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);

				if (mushX < 0 || mushZ < 0 || mushX >= Gen_Width || mushZ >= Gen_Length) continue;
				groundHeight = Heightmap[mushZ * Gen_Width + mushX];
				if (mushY >= (groundHeight - 1)) continue;

				index = Gen_Pack(mushX, mushY, mushZ);
				if (Gen_Blocks[index] == BLOCK_AIR && Gen_Blocks[index - Gen_OneY] == BLOCK_STONE)
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

	Vector3I coords[TREE_MAX_COUNT];
	BlockRaw blocks[TREE_MAX_COUNT];

	Tree_Width  = Gen_Width; Tree_Height = Gen_Height; Tree_Length = Gen_Length;
	Tree_Blocks = Gen_Blocks;
	Tree_Rnd    = &rnd;

	numPatches       = Gen_Width * Gen_Length / 4000;
	Gen_CurrentState = "Planting trees";
	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (float)i / numPatches;

		patchX = Random_Next(&rnd, Gen_Width);
		patchZ = Random_Next(&rnd, Gen_Length);

		for (j = 0; j < 20; j++) {
			treeX = patchX; treeZ = patchZ;
			for (k = 0; k < 20; k++) {
				treeX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				treeZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);

				if (treeX < 0 || treeZ < 0 || treeX >= Gen_Width ||
					treeZ >= Gen_Length || Random_Float(&rnd) >= 0.25) continue;

				treeY = Heightmap[treeZ * Gen_Width + treeX] + 1;
				if (treeY >= Gen_Height) continue;
				treeHeight = 5 + Random_Next(&rnd, 3);

				index = Gen_Pack(treeX, treeY, treeZ);
				under = treeY > 0 ? Gen_Blocks[index - Gen_OneY] : BLOCK_AIR;

				if (under == BLOCK_GRASS && TreeGen_CanGrow(treeX, treeY, treeZ, treeHeight)) {
					count = TreeGen_Grow(treeX, treeY, treeZ, treeHeight, coords, blocks);

					for (m = 0; m < count; m++) {
						index = Gen_Pack(coords[m].X, coords[m].Y, coords[m].Z);
						Gen_Blocks[index] = blocks[m];
					}
				}
			}
		}
	}
}

void NotchyGen_Generate(void) {
	Gen_Init();
	Heightmap = Mem_Alloc(Gen_Width * Gen_Length, 2, "gen heightmap");

	Random_Init(&rnd, Gen_Seed);
	waterLevel = Gen_Height / 2;	
	minHeight = Gen_Height;

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
*---------------------------------------------------Noise generation------------------------------------------------------*
*#########################################################################################################################*/
void ImprovedNoise_Init(uint8_t* p, RNGState* rnd) {
	uint8_t tmp;
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

float ImprovedNoise_Calc(uint8_t* p, float x, float y) {
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


void OctaveNoise_Init(struct OctaveNoise* n, RNGState* rnd, int octaves) {
	int i;
	n->octaves = octaves;
	
	for (i = 0; i < octaves; i++) {
		ImprovedNoise_Init(n->p[i], rnd);
	}
}

float OctaveNoise_Calc(struct OctaveNoise* n, float x, float y) {
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


void CombinedNoise_Init(struct CombinedNoise* n, RNGState* rnd, int octaves1, int octaves2) {
	OctaveNoise_Init(&n->noise1, rnd, octaves1);
	OctaveNoise_Init(&n->noise2, rnd, octaves2);
}

float CombinedNoise_Calc(struct CombinedNoise* n, float x, float y) {
	float offset = OctaveNoise_Calc(&n->noise2, x, y);
	return OctaveNoise_Calc(&n->noise1, x + offset, y);
}


/*########################################################################################################################*
*--------------------------------------------------Tree generation----------------------------------------------------*
*#########################################################################################################################*/
#define Tree_Pack(x, y, z) (((y) * Tree_Length + (z)) * Tree_Width + (x))

bool TreeGen_CanGrow(int treeX, int treeY, int treeZ, int treeHeight) {
	int baseHeight = treeHeight - 4;
	int index;
	int x, y, z;

	/* check tree base */
	for (y = treeY; y < treeY + baseHeight; y++) {
		for (z = treeZ - 1; z <= treeZ + 1; z++) {
			for (x = treeX - 1; x <= treeX + 1; x++) {
				if (x < 0 || y < 0 || z < 0 || x >= Tree_Width || y >= Tree_Height || z >= Tree_Length)
					return false;

				index = Tree_Pack(x, y, z);
				if (Tree_Blocks[index] != BLOCK_AIR) return false;
			}
		}
	}

	/* and also check canopy */
	for (y = treeY + baseHeight; y < treeY + treeHeight; y++) {
		for (z = treeZ - 2; z <= treeZ + 2; z++) {
			for (x = treeX - 2; x <= treeX + 2; x++) {
				if (x < 0 || y < 0 || z < 0 || x >= Tree_Width || y >= Tree_Height || z >= Tree_Length)
					return false;

				index = Tree_Pack(x, y, z);
				if (Tree_Blocks[index] != BLOCK_AIR) return false;
			}
		}
	}
	return true;
}

#define TreeGen_Place(x, y, z, block)\
coords[count].X = (x); coords[count].Y = (y); coords[count].Z = (z);\
blocks[count] = block; count++;

int TreeGen_Grow(int treeX, int treeY, int treeZ, int height, Vector3I* coords, BlockRaw* blocks) {
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
