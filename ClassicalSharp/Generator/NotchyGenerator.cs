// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
// Based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
// Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
// I believe this process adheres to clean room reverse engineering.
using System;
using System.Collections.Generic;

namespace ClassicalSharp.Generator {
	
	public sealed partial class NotchyGenerator : IMapGenerator {
		
		int waterLevel, oneY;
		byte[] blocks;
		short[] heightmap;
		JavaRandom rnd;
		
		public override string GeneratorName { get { return "Vanilla classic"; } }
		
		public override byte[] Generate(int seed) {
			oneY = Width * Length;
			waterLevel = Height / 2;
			blocks = new byte[Width * Height * Length];
			rnd = new JavaRandom(seed);
			
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
			return blocks;
		}
		
		void CreateHeightmap() {
			Noise n1 = new CombinedNoise(
				new OctaveNoise(8, rnd), new OctaveNoise(8, rnd));
			Noise n2 = new CombinedNoise(
				new OctaveNoise(8, rnd), new OctaveNoise(8, rnd));
			Noise n3 = new OctaveNoise(6, rnd);
			int index = 0;
			short[] hMap = new short[Width * Length];
			CurrentState = "Building heightmap";
			
			for (int z = 0; z < Length; z++) {
				CurrentProgress = (float)z / Length;
				for (int x = 0; x < Width; x++) {
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
			for (int z = 0; z < Length; z++) {
				CurrentProgress = (float)z / Length;
				for (int x = 0; x < Width; x++) {
					int dirtThickness = (int)(n.Compute(x, z) / 24 - 4);
					int dirtHeight = heightmap[hMapIndex++];

					int stoneHeight = dirtHeight + dirtThickness;
					int mapIndex = z * Width + x;
					
					blocks[mapIndex] = Block.Lava;
					mapIndex += oneY;
					for (int y = 1; y < Height; y++) {
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
				double caveX = rnd.Next(Width);
				double caveY = rnd.Next(Height);
				double caveZ = rnd.Next(Length);
				
				int caveLen = (int)(rnd.NextFloat() * rnd.NextFloat() * 200);
				double theta = rnd.NextFloat() * 2 * Math.PI, deltaTheta = 0;
				double phi = rnd.NextFloat() * 2 * Math.PI, deltaPhi = 0;
				double caveRadius = rnd.NextFloat() * rnd.NextFloat();
				
				for (int j = 0; j < caveLen; j++) {
					caveX += Math.Sin(theta) * Math.Cos(phi);
					caveZ += Math.Cos(theta) * Math.Cos(phi);
					caveY += Math.Sin(phi);
					
					theta = theta + deltaTheta * 0.2;
					deltaTheta = deltaTheta * 0.9 + rnd.NextFloat() - rnd.NextFloat();
					phi = phi / 2 + deltaPhi / 4;
					deltaPhi = deltaPhi * 0.75 + rnd.NextFloat() - rnd.NextFloat();
					if (rnd.NextFloat() < 0.25) continue;
					
					int cenX = (int)(caveX + (rnd.Next(4) - 2) * 0.2);
					int cenY = (int)(caveY + (rnd.Next(4) - 2) * 0.2);
					int cenZ = (int)(caveZ + (rnd.Next(4) - 2) * 0.2);
					
					double radius = (Height - cenY) / (double)Height;
					radius = 1.2 + (radius * 3.5 + 1) * caveRadius;
					radius = radius * Math.Sin(j * Math.PI / caveLen);
					FillOblateSpheroid(cenX, cenY, cenZ, (float)radius, Block.Air);
				}
			}
		}
		
		void CarveOreVeins(float abundance, string blockName, byte block) {
			int numVeins = (int)(blocks.Length * abundance / 16384);
			CurrentState = "Carving " + blockName;
			
			for (int i = 0; i < numVeins; i++) {
				CurrentProgress = (float)i / numVeins;
				double veinX = rnd.Next(Width);
				double veinY = rnd.Next(Height);
				double veinZ = rnd.Next(Length);
				
				int veinLen = (int)(rnd.NextFloat() * rnd.NextFloat() * 75 * abundance);
				double theta = rnd.NextFloat() * 2 * Math.PI, deltaTheta = 0;
				double phi = rnd.NextFloat() * 2 * Math.PI, deltaPhi = 0;
				
				for (int j = 0; j < veinLen; j++) {
					veinX += Math.Sin(theta) * Math.Cos(phi);
					veinZ += Math.Cos(theta) * Math.Cos(phi);
					veinY += Math.Sin(phi);
					
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
			int index1 = (waterY * Length + 0) * Width + 0;
			int index2 = (waterY * Length + (Length - 1)) * Width + 0;
			CurrentState = "Flooding edge water";
			
			for (int x = 0; x < Width; x++) {
				CurrentProgress = 0 + ((float)x / Width) * 0.5f;
				FloodFill(index1, Block.Water);
				FloodFill(index2, Block.Water);
				index1++; index2++;
			}
			
			index1 = (waterY * Length + 0) * Width + 0;
			index2 = (waterY * Length + 0) * Width + (Width - 1);
			for (int z = 0; z < Length; z++) {
				CurrentProgress = 0.5f + ((float)z / Length) * 0.5f;
				FloodFill(index1, Block.Water);
				FloodFill(index2, Block.Water);
				index1 += Width; index2 += Width;
			}
		}
		
		void FloodFillWater() {
			int numSources = Width * Length / 800;
			CurrentState = "Flooding water";
			
			for (int i = 0; i < numSources; i++) {
				CurrentProgress = (float)i / numSources;
				int x = rnd.Next(Width), z = rnd.Next(Length);
				int y = waterLevel - rnd.Next(1, 3);
				FloodFill((y * Length + z) * Width + x, Block.Water);
			}
		}
		
		void FloodFillLava() {
			int numSources = Width * Length / 20000;
			CurrentState = "Flooding lava";
			
			for (int i = 0; i < numSources; i++) {
				CurrentProgress = (float)i / numSources;
				int x = rnd.Next(Width), z = rnd.Next(Length);
				int y = (int)((waterLevel - 3) * rnd.NextFloat() * rnd.NextFloat());
				FloodFill((y * Length + z) * Width + x, Block.Lava);
			}
		}
		
		void CreateSurfaceLayer() {
			Noise n1 = new OctaveNoise(8, rnd), n2 = new OctaveNoise(8, rnd);
			CurrentState = "Creating surface";
			// TODO: update heightmap
			
			int hMapIndex = 0;
			for (int z = 0; z < Length; z++) {
				CurrentProgress = (float)z / Length;
				for (int x = 0; x < Width; x++) {
					bool sand = n1.Compute(x, z) > 8;
					bool gravel = n2.Compute(x, z) > 12;
					int y = heightmap[hMapIndex++];
					if (y < 0 || y >= Height) continue;
					
					int index = (y * Length + z) * Width + x;
					byte blockAbove = y >= (Height - 1) ? Block.Air : blocks[index + oneY];
					if (blockAbove == Block.Water && gravel) {
						blocks[index] = Block.Gravel;
					} else if (blockAbove == Block.Air) {
						blocks[index] = (y <= waterLevel && sand) ? Block.Sand : Block.Grass;
					}
				}
			}
		}
		
		void PlantFlowers() {
			int numPatches = Width * Length / 3000;
			CurrentState = "Planting flowers";
			
			for (int i = 0; i < numPatches; i++) {
				CurrentProgress = (float)i / numPatches;
				byte type = (byte)(Block.Dandelion + rnd.Next(2));
				int patchX = rnd.Next(Width), patchZ = rnd.Next(Length);
				for (int j = 0; j < 10; j++) {
					int flowerX = patchX, flowerZ = patchZ;
					for (int k = 0; k < 5; k++) {
						flowerX += rnd.Next(6) - rnd.Next(6);
						flowerZ += rnd.Next(6) - rnd.Next(6);
						if (flowerX < 0 || flowerZ < 0 || flowerX >= Width || flowerZ >= Length)
							continue;
						
						int flowerY = heightmap[flowerZ * Width + flowerX] + 1;
						if (flowerY <= 0 || flowerY >= Height) continue;
						
						int index = (flowerY * Length + flowerZ) * Width + flowerX;
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
				int patchX = rnd.Next(Width);
				int patchY = rnd.Next(Height);
				int patchZ = rnd.Next(Length);
				
				for (int j = 0; j < 20; j++) {
					int mushX = patchX, mushY = patchY, mushZ = patchZ;
					for (int k = 0; k < 5; k++) {
						mushX += rnd.Next(6) - rnd.Next(6);
						mushZ += rnd.Next(6) - rnd.Next(6);
						if (mushX < 0 || mushZ < 0 || mushX >= Width || mushZ >= Length)
							continue;
						int solidHeight = heightmap[mushZ * Width + mushX];
						if (mushY >= (solidHeight - 1))
							continue;
						
						int index = (mushY * Length + mushZ) * Width + mushX;
						if (blocks[index] == Block.Air && blocks[index - oneY] == Block.Stone)
							blocks[index] = type;
					}
				}
			}
		}
		
		void PlantTrees() {
			int numPatches = Width * Length / 4000;
			CurrentState = "Planting trees";
			
			for (int i = 0; i < numPatches; i++) {
				CurrentProgress = (float)i / numPatches;
				int patchX = rnd.Next(Width), patchZ = rnd.Next(Length);
				
				for (int j = 0; j < 20; j++) {
					int treeX = patchX, treeZ = patchZ;
					for (int k = 0; k < 20; k++) {
						treeX += rnd.Next(6) - rnd.Next(6);
						treeZ += rnd.Next(6) - rnd.Next(6);
						if (treeX < 0 || treeZ < 0 || treeX >= Width ||
						   treeZ >= Length || rnd.NextFloat() >= 0.25)
							continue;
						
						int treeY = heightmap[treeZ * Width + treeX] + 1;
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
				if (x < 0 || y < 0 || z < 0 || x >= Width || y >= Height || z >= Length)
					return false;
				int index = (y * Length + z) * Width + x;
				if (blocks[index] != 0) return false;
			}
			
			// and also check canopy
			for (int y = treeY + baseHeight; y < treeY + treeHeight; y++)
				for (int z = treeZ - 2; z <= treeZ + 2; z++)
					for (int x = treeX - 2; x <= treeX + 2; x++)
			{
				if (x < 0 || y < 0 || z < 0 || x >= Width || y >= Height || z >= Length)
					return false;
				int index = (y * Length + z) * Width + x;
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
				index = (y * Length + z) * Width + x;
				
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
				index = (y * Length + z) * Width + x;

				if (xx == 0 || zz == 0) {
					blocks[index] = Block.Leaves;
				} else if (y == bottomY && rnd.NextFloat() >= 0.5) {
					blocks[index] = Block.Leaves;
				}
			}
			
			// then place trunk
			index = (treeY * Length + treeZ) * Width + treeX;
			for (int y = 0; y < height - 1; y++) {
				blocks[index] = Block.Log;
				index += oneY;
			}
		}
	}
}
