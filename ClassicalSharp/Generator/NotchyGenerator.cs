// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
// Based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
// Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
// I believe this process adheres to clean room reverse engineering.
using System;
using System.Collections.Generic;

namespace ClassicalSharp.Generator {
	
	public sealed partial class NotchyGenerator : IMapGenerator {
		
		Game game; int width, height, length; bool winter;
		int waterLevel, oneY;
		byte[] blocks;
		short[] heightmap;
		bool winterMode;
		JavaRandom rnd;

		
		public override string GeneratorName { get { return "Vanilla classic"; } }
		
		public override byte[] Generate(Game game, int width, int height, int length, int seed, bool winter) {
			this.game = game;
			this.width = width;
			this.height = height;
			this.length = length;
			oneY = width * length;
			waterLevel = height / 2;
			blocks = new byte[width * height * length];
			rnd = new JavaRandom(seed);
			winterMode = winter; // TODO: add an option when creating the world to enable this.
			
			CreateHeightmap();			
			CreateStrata();
			CarveCaves();
			CarveOreVeins(0.9f, "coal ore", Block.CoalOre);
			CarveOreVeins(0.7f, "iron ore", Block.IronOre);
			CarveOreVeins(0.5f, "gold ore", Block.GoldOre);
			
			FloodFillWaterBorders();
			FloodFillWater();
			FloodFillLava();

			CreateSurfaceLayer();
			PlantFlowers();
			PlantMushrooms();
			PlantTrees();
			if (winterMode) {
				FreezeWorld();
			}
			return blocks;
		}
		
		void CreateHeightmap() {
			Noise n1 = new CombinedNoise(
				new OctaveNoise(8, rnd), new OctaveNoise(8, rnd));
			Noise n2 = new CombinedNoise(
				new OctaveNoise(8, rnd), new OctaveNoise(8, rnd));
			Noise n3 = new OctaveNoise(6, rnd);
			int index = 0;
			short[] hMap = new short[width * length];
			CurrentState = "Building heightmap";
			
			for (int z = 0; z < length; z++) {
				CurrentProgress = (float)z / length;
				for (int x = 0; x < width; x++) {
					double hLow = n1.Compute(x * 1.3f, z * 1.3f) / 6 - 4;
					double hHigh = n2.Compute(x * 1.3f, z * 1.3f) / 5 + 6;
					
					double height = n3.Compute(x, z) > 0 ? hLow : Math.Max(hLow, hHigh);
					height *= 0.5;
					if (height < 0) height *= 0.8f;
					hMap[index++] = (short)(height + waterLevel);
				}
			}
			heightmap = hMap;
		}
		
		void CreateStrata() {
			Noise n = new OctaveNoise(8, rnd);
			CurrentState = "Creating strata";
			
			int hMapIndex = 0;
			for (int z = 0; z < length; z++) {
				CurrentProgress = (float)z / length;
				for (int x = 0; x < width; x++) {
					int dirtThickness = (int)(n.Compute(x, z) / 24 - 4);
					int dirtHeight = heightmap[hMapIndex++];

					int stoneHeight = dirtHeight + dirtThickness;
					int mapIndex = z * width + x;
					
					blocks[mapIndex] = Block.Lava;
					mapIndex += oneY;
					for (int y = 1; y < height; y++) {
						byte block = 0;
						if (y <= stoneHeight) block = Block.Stone;
						else if (y <= dirtHeight) block = Block.Dirt;
						
						blocks[mapIndex] = block;
						mapIndex += oneY;
					}
				}
			}
		}
		
		void CarveCaves() {
			int cavesCount = blocks.Length / 8192;
			CurrentState = "Carving caves";
			
			for (int i = 0; i < cavesCount; i++) {
				CurrentProgress = (float)i / cavesCount;
				double caveX = rnd.Next(width);
				double caveY = rnd.Next(height);
				double caveZ = rnd.Next(length);
				
				int caveLen = (int)(rnd.NextFloat() * rnd.NextFloat() * 200);
				double theta = rnd.NextFloat() * 2 * Math.PI, deltaTheta = 0;
				double phi = rnd.NextFloat() * 2 * Math.PI, deltaPhi = 0;
				double caveRadius = rnd.NextFloat() * rnd.NextFloat();
				
				for (int j = 0; j < caveLen; j++) {
					caveX += Math.Sin(theta) * Math.Cos(phi);
					caveY += Math.Cos(theta) * Math.Cos(phi);
					caveZ += Math.Sin(phi);
					
					theta = theta + deltaTheta * 0.2;
					deltaTheta = deltaTheta * 0.9 + rnd.NextFloat() - rnd.NextFloat();
					phi = phi / 2 + deltaPhi / 4;
					deltaPhi = deltaPhi * 0.75 + rnd.NextFloat() - rnd.NextFloat();
					if (rnd.NextFloat() < 0.25) continue;
					
					int cenX = (int)(caveX + (rnd.Next(4) - 2) * 0.2);
					int cenY = (int)(caveY + (rnd.Next(4) - 2) * 0.2);
					int cenZ = (int)(caveZ + (rnd.Next(4) - 2) * 0.2);
					double radius = (height - cenY) / (double)height;
					radius = 1.2 + (radius * 3.5 + 1) * caveRadius;
					radius = radius + Math.Sin(j * Math.PI / caveLen); // TODO: this should be * according to the spec. doesn't seem right though.
					FillOblateSpheroid(cenX, cenY, cenZ, (float)radius, Block.Air);
				}
			}
		}
		
		void CarveOreVeins(float abundance, string blockName, byte block) {
			int numVeins = (int)(blocks.Length * abundance / 16384);
			CurrentState = "Carving " + blockName;
			
			for (int i = 0; i < numVeins; i++) {
				CurrentProgress = (float)i / numVeins;
				double veinX = rnd.Next(width);
				double veinY = rnd.Next(height);
				double veinZ = rnd.Next(length);
				
				int veinLen = (int)(rnd.NextFloat() * rnd.NextFloat() * 75 * abundance);
				double theta = rnd.NextFloat() * 2 * Math.PI, deltaTheta = 0;
				double phi = rnd.NextFloat() * 2 * Math.PI, deltaPhi = 0;
				
				for (int j = 0; j < veinLen; j++) {
					veinX += Math.Sin(theta) * Math.Cos(phi);
					veinY += Math.Cos(theta) * Math.Cos(phi);
					veinZ += Math.Sin(phi);
					
					theta = deltaTheta * 0.2;
					deltaTheta = deltaTheta * 0.9 + rnd.NextFloat() - rnd.NextFloat();
					phi = phi / 2 + deltaPhi / 4;
					deltaPhi = deltaPhi * 0.9 + rnd.NextFloat() - rnd.NextFloat();
					
					float radius = abundance * (float)Math.Sin(j * Math.PI / veinLen) + 1;
					FillOblateSpheroid((int)veinX, (int)veinY, (int)veinZ, radius, block);
				}
			}
		}
		
		void FloodFillWaterBorders() {
			int waterY = waterLevel - 1;			
			int index1 = (waterY * length + 0) * width + 0;
			int index2 = (waterY * length + (length - 1)) * width + 0;
			CurrentState = "Flooding edge water";
			
			for (int x = 0; x < width; x++) {
				CurrentProgress = 0 + ((float)x / width) * 0.5f;
				FloodFill(index1, Block.Water);
				FloodFill(index2, Block.Water);
				index1++; index2++;
			}
			
			index1 = (waterY * length + 0) * width + 0;
			index2 = (waterY * length + 0) * width + (width - 1);
			for (int z = 0; z < length; z++) {
				CurrentProgress = 0.5f + ((float)z / length) * 0.5f;
				FloodFill(index1, Block.Water);
				FloodFill(index2, Block.Water);
				index1 += width; index2 += width;
			}
		}
		
		void FloodFillWater() {
			int numSources = width * length / 800;
			CurrentState = "Flooding water";
			
			for (int i = 0; i < numSources; i++) {
				CurrentProgress = (float)i / numSources;
				int x = rnd.Next(width), z = rnd.Next(length);
				int y = waterLevel - rnd.Next(1, 3);
				FloodFill((y * length + z) * width + x, Block.Water);
			}
		}
		
		void FloodFillLava() {
			int numSources = width * length / 20000;
			CurrentState = "Flooding lava";
			
			for (int i = 0; i < numSources; i++) {
				CurrentProgress = (float)i / numSources;
				int x = rnd.Next(width), z = rnd.Next(length);
				int y = (int)((waterLevel - 3) * rnd.NextFloat() * rnd.NextFloat());
				FloodFill((y * length + z) * width + x, Block.Lava);
			}
		}
		
		void CreateSurfaceLayer() {
			Noise n1 = new OctaveNoise(8, rnd), n2 = new OctaveNoise(8, rnd);
			CurrentState = "Creating surface";
			// TODO: update heightmap
			
			int hMapIndex = 0;
			for (int z = 0; z < length; z++) {
				CurrentProgress = (float)z / length;
				for (int x = 0; x < width; x++) {
					bool sand = n1.Compute(x, z) > 8;
					bool gravel = n2.Compute(x, z) > 12;
					int y = heightmap[hMapIndex++];
					if (y < 0 || y >= height) continue;
					
					int index = (y * length + z) * width + x;
					byte blockAbove = y >= (height - 1) ? Block.Air : blocks[index + oneY];
					if (blockAbove == Block.Water && gravel) {
						blocks[index] = Block.Gravel;
					} else if (blockAbove == Block.Air) {
						blocks[index] = (y <= waterLevel && sand) ? Block.Sand : Block.Grass;
					}
				}
			}
		}
		
		void PlantFlowers() {
			int numPatches = width * length / 3000;
			CurrentState = "Planting flowers";
			
			for (int i = 0; i < numPatches; i++) {
				CurrentProgress = (float)i / numPatches;
				byte type = (byte)(Block.Dandelion + rnd.Next(2));
				int patchX = rnd.Next(width), patchZ = rnd.Next(length);
				for (int j = 0; j < 10; j++) {
					int flowerX = patchX, flowerZ = patchZ;
					for (int k = 0; k < 5; k++) {
						flowerX += rnd.Next(6) - rnd.Next(6);
						flowerZ += rnd.Next(6) - rnd.Next(6);
						if (flowerX < 0 || flowerZ < 0 || flowerX >= width || flowerZ >= length)
							continue;
						
						int flowerY = heightmap[flowerZ * width + flowerX] + 1;
						if (flowerY <= 0 || flowerY >= height) continue;
						
						int index = (flowerY * length + flowerZ) * width + flowerX;
						if (blocks[index] == Block.Air && blocks[index - oneY] == Block.Grass)
							blocks[index] = type;
					}
				}
			}
		}
		
		void PlantMushrooms() {
			int numPatches = blocks.Length / 2000;
			CurrentState = "Planting mushrooms";
			
			for (int i = 0; i < numPatches; i++) {
				CurrentProgress = (float)i / numPatches;
				byte type = (byte)(Block.BrownMushroom + rnd.Next(2));
				int patchX = rnd.Next(width);
				int patchY = rnd.Next(height);
				int patchZ = rnd.Next(length);
				
				for (int j = 0; j < 20; j++) {
					int mushX = patchX, mushY = patchY, mushZ = patchZ;
					for (int k = 0; k < 5; k++) {
						mushX += rnd.Next(6) - rnd.Next(6);
						mushZ += rnd.Next(6) - rnd.Next(6);
						if (mushX < 0 || mushZ < 0 || mushX >= width || mushZ >= length)
							continue;
						int solidHeight = heightmap[mushZ * width + mushX];
						if (mushY >= (solidHeight - 1))
							continue;
						
						int index = (mushY * length + mushZ) * width + mushX;
						if (blocks[index] == Block.Air && blocks[index - oneY] == Block.Stone)
							blocks[index] = type;
					}
				}
			}
		}
		
		void PlantTrees() {
			int numPatches = width * length / 4000;
			CurrentState = "Planting trees";
			
			for (int i = 0; i < numPatches; i++) {
				CurrentProgress = (float)i / numPatches;
				int patchX = rnd.Next(width), patchZ = rnd.Next(length);
				
				for (int j = 0; j < 20; j++) {
					int treeX = patchX, treeZ = patchZ;
					for (int k = 0; k < 20; k++) {
						treeX += rnd.Next(6) - rnd.Next(6);
						treeZ += rnd.Next(6) - rnd.Next(6);
						if (treeX < 0 || treeZ < 0 || treeX >= width ||
						   treeZ >= length || rnd.NextFloat() >= 0.25)
							continue;
						
						int treeY = heightmap[treeZ * width + treeX] + 1;
						int treeHeight = 5 + rnd.Next(3);
						if (CanGrowTree(treeX, treeY, treeZ, treeHeight)) {
							GrowTree(treeX, treeY, treeZ, treeHeight);
						}
					}
				}
			}
		}
		
		bool CanGrowTree(int treeX, int treeY, int treeZ, int treeHeight) {
			// check tree base
			int baseHeight = treeHeight - 4;
			for (int y = treeY; y < treeY + baseHeight; y++)
				for (int z = treeZ - 1; z <= treeZ + 1; z++)
					for (int x = treeX - 1; x <= treeX + 1; x++)
			{
				if (x < 0 || y < 0 || z < 0 || x >= width || y >= height || z >= length)
					return false;
				int index = (y * length + z) * width + x;
				if (blocks[index] != 0) return false;
			}
			
			// and also check canopy
			for (int y = treeY + baseHeight; y < treeY + treeHeight; y++)
				for (int z = treeZ - 2; z <= treeZ + 2; z++)
					for (int x = treeX - 2; x <= treeX + 2; x++)
			{
				if (x < 0 || y < 0 || z < 0 || x >= width || y >= height || z >= length)
					return false;
				int index = (y * length + z) * width + x;
				if (blocks[index] != 0) return false;
			}
			return true;
		}
		
		void GrowTree(int treeX, int treeY, int treeZ, int height) {
			int baseHeight = height - 4;
			int index = 0;
			
			// leaves bottom layer
			for (int y = treeY + baseHeight; y < treeY + baseHeight + 2; y++)
				for (int zz = -2; zz <= 2; zz++)
					for (int xx = -2; xx <= 2; xx++)
			{
				int x = xx + treeX, z = zz + treeZ;
				index = (y * length + z) * width + x;
				
				if (Math.Abs(xx) == 2 && Math.Abs(zz) == 2) {
					if (rnd.NextFloat() >= 0.5)
						blocks[index] = Block.Leaves;
				} else {
					blocks[index] = Block.Leaves;
				}
			}
			
			// leaves top layer
			int bottomY = treeY + baseHeight + 2;
			for (int y = treeY + baseHeight + 2; y < treeY + height; y++)
				for (int zz = -1; zz <= 1; zz++)
					for (int xx = -1; xx <= 1; xx++)
			{
				int x = xx + treeX, z = zz + treeZ;
				index = (y * length + z) * width + x;

				if (xx == 0 || zz == 0) {
					blocks[index] = Block.Leaves;
				} else if (y == bottomY && rnd.NextFloat() >= 0.5) {
					blocks[index] = Block.Leaves;
				}
			}
			
			// then place trunk
			index = (treeY * length + treeZ) * width + treeX;
			for (int y = 0; y < height - 1; y++) {
				blocks[index] = Block.Log;
				index += oneY;
			}
		}

		void FreezeWorld() {
			int index = 0;
			int indexUp = 0;
			int indexLand = 0;
			CurrentState = "Freezing world";
			for (int x = 0; x < width; x++)
				for (int z = 0; z < length; z++)
			{
				int y = CalcHeightAt(x, z);
				int yUp = y + 1;
				if (y >= height - 1) continue;
				
				indexUp = (yUp * length + z) * width + x;
				index = (y * length + z) * width + x;
				if (blocks[index] == Block.Lava || blocks[index] == Block.StillLava) continue;
				
				if (blocks[index] == Block.Leaves) {
					int yLand = CalcLandAt(x, z);
					indexLand = (yLand * length + z) * width + x;
					blocks[indexLand] = (blocks[indexLand] == Block.Water) ? Block.Ice : blocks[indexLand];
					blocks[indexLand] = (blocks[indexLand] == Block.StillWater) ? Block.Ice : blocks[indexLand];
					blocks[indexUp] = Block.Snow;
				}
				else if (blocks[index] == Block.Water || blocks[index] == Block.StillWater) {
					blocks[index] = Block.Ice;
				}else if (blocks[indexUp] == Block.Air)
					if (blocks[index] != Block.Rose)
						if (blocks[index] != Block.Dandelion) {
					blocks[indexUp] = Block.Snow;
				}
				float xProg = (float)x * length; //for each x increment, there are length blocks finished.
				float zProg = (float)z;
				CurrentProgress = (xProg + zProg) / (width * length);
			}
			game.World.Env.SetEdgeBlock(Block.Ice); // TODO: Set the weather to snowy.
			game.BlockInfo.BlocksLight[Block.Snow] = false;
		}
		
		int CalcHeightAt(int x, int z) {
			int index = ((height - 1) * length + z) * width + x;
			
			for (int y = height - 1; y >= 0; y--) {
				if (blocks[index] != Block.Air) return y;
				index -= oneY;
			}
			return -1;
		}
		int CalcLandAt(int x, int z) {
			int index = ((height - 1) * length + z) * width + x;
			int Minimum = 0;
			Minimum = waterLevel - 20;
			
			for (int y = height - 1; y >= Minimum; y--) {
				if (blocks[index] == Block.Water || blocks[index] == Block.StillWater || blocks[index] == Block.Grass || blocks[index] == Block.Dirt || blocks[index] == Block.Stone || blocks[index] == Block.Sand) return y;
				index -= oneY;
			}
			return -1;
		}
	}
}
