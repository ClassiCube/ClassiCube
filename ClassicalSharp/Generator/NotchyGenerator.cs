// Based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
// Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
// I believe this process adheres to clean room reverse engineering.
using System;
using System.Collections.Generic;

namespace ClassicalSharp.Generator {
	
	public sealed partial class NotchyGenerator : IMapGenerator {
		
		int width, height, length;
		int waterLevel, oneY;
		byte[] blocks;
		short[] heightmap;
		Random rnd;
		
		public override string GeneratorName { get { return "Vanilla classic"; } }
		
		public override byte[] Generate( int width, int height, int length, int seed ) {
			this.width = width;
			this.height = height;
			this.length = length;
			oneY = width * length;
			waterLevel = height / 2;
			blocks = new byte[width * height * length];
			rnd = new Random( seed );
			
			CreateHeightmap();
			CreateStrata();
			CarveCaves();
			CarveOreVeins( 0.9f, "coal ore", (byte)Block.CoalOre );
			CarveOreVeins( 0.7f, "iron ore", (byte)Block.IronOre );
			CarveOreVeins( 0.5f, "gold ore", (byte)Block.GoldOre );
			
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
				new OctaveNoise( 8, rnd ), new OctaveNoise( 8, rnd ) );
			Noise n2 = new CombinedNoise(
				new OctaveNoise( 8, rnd ), new OctaveNoise( 8, rnd ) );
			Noise n3 = new OctaveNoise( 6, rnd );
			int index = 0;
			short[] hMap = new short[width * length];
			CurrentState = "Building heightmap";
			
			for( int z = 0; z < length; z++ ) {
				CurrentProgress = (float)z / length;
				for( int x = 0; x < width; x++ ) {
					double hLow = n1.Compute( x * 1.3f, z * 1.3f ) / 6 - 4;
					double hHigh = n2.Compute( x * 1.3f, z * 1.3f ) / 5 + 6;
					
					double height = n3.Compute( x, z ) > 0 ? hLow : Math.Max( hLow, hHigh );
					height *= 0.5;
					if( height < 0 ) height *= 0.8f;
					hMap[index++] = (short)(height + waterLevel);
				}
			}
			heightmap = hMap;
		}
		
		void CreateStrata() {
			Noise n = new OctaveNoise( 8, rnd );
			CurrentState = "Creating strata";
			
			int hMapIndex = 0;
			for( int z = 0; z < length; z++ ) {
				CurrentProgress = (float)z / length;
				for( int x = 0; x < width; x++ ) {
					int dirtThickness = (int)(n.Compute( x, z ) / 24 - 4);
					int dirtHeight = heightmap[hMapIndex++];

					int stoneHeight = dirtHeight + dirtThickness;
					int mapIndex = z * width + x;
					
					blocks[mapIndex] = (byte)Block.Lava;
					mapIndex += oneY;
					for( int y = 1; y < height; y++ ) {
						byte type = 0;
						if( y <= stoneHeight ) type = (byte)Block.Stone;
						else if( y <= dirtHeight ) type = (byte)Block.Dirt;
						
						blocks[mapIndex] = type;
						mapIndex += oneY;
					}
				}
			}
		}
		
		void CarveCaves() {
			int cavesCount = blocks.Length / 8192;
			CurrentState = "Carving caves";
			
			for( int i = 0; i < cavesCount; i++ ) {
				CurrentProgress = (float)i / cavesCount;
				double caveX = rnd.Next( width );
				double caveY = rnd.Next( height );
				double caveZ = rnd.Next( length );
				
				int caveLen = (int)(rnd.NextDouble() * rnd.NextDouble() * 200);
				double theta = rnd.NextDouble() * 2 * Math.PI, deltaTheta = 0;
				double phi = rnd.NextDouble() * 2 * Math.PI, deltaPhi = 0;
				double caveRadius = rnd.NextDouble() * rnd.NextDouble();
				
				for( int j = 0; j < caveLen; j++ ) {
					caveX += Math.Sin( theta ) * Math.Cos( phi );
					caveY += Math.Cos( theta ) * Math.Cos( phi );
					caveZ += Math.Sin( phi );
					
					theta = theta + deltaTheta * 0.2;
					deltaTheta = deltaTheta * 0.9 + rnd.NextDouble() - rnd.NextDouble();
					phi = phi / 2 + deltaPhi / 4;
					deltaPhi = deltaPhi * 0.75 + rnd.NextDouble() - rnd.NextDouble();
					if( rnd.NextDouble() < 0.25 ) continue;
					
					int cenX = (int)(caveX + (rnd.Next( 4 ) - 2) * 0.2);
					int cenY = (int)(caveY + (rnd.Next( 4 ) - 2) * 0.2);
					int cenZ = (int)(caveZ + (rnd.Next( 4 ) - 2) * 0.2);
					double radius = (height - cenY) / (double)height;
					radius = 1.2 + (radius * 3.5 + 1) * caveRadius;
					radius = radius + Math.Sin( j * Math.PI / caveLen );
					FillOblateSpheroid( cenX, cenY, cenZ, (float)radius, (byte)Block.Air );
				}
			}
		}
		
		void CarveOreVeins( float abundance, string blockName, byte block ) {
			int numVeins = (int)(blocks.Length * abundance / 16384);
			CurrentState = "Carving " + blockName;
			
			for( int i = 0; i < numVeins; i++ ) {
				CurrentProgress = (float)i / numVeins;
				double veinX = rnd.Next( width );
				double veinY = rnd.Next( height );
				double veinZ = rnd.Next( length );
				
				int veinLen = (int)(rnd.NextDouble() * rnd.NextDouble() * 75 * abundance);
				double theta = rnd.NextDouble() * 2 * Math.PI, deltaTheta = 0;
				double phi = rnd.NextDouble() * 2 * Math.PI, deltaPhi = 0;
				
				for( int j = 0; j < veinLen; j++ ) {
					veinX += Math.Sin( theta ) * Math.Cos( phi );
					veinY += Math.Cos( theta ) * Math.Cos( phi );
					veinZ += Math.Sin( phi );
					
					theta = deltaTheta * 0.2;
					deltaTheta = deltaTheta * 0.9 + rnd.NextDouble() - rnd.NextDouble();
					phi = phi / 2 + deltaPhi / 4;
					deltaPhi = deltaPhi * 0.9 + rnd.NextDouble() - rnd.NextDouble();
					
					float radius = abundance * (float)Math.Sin( j * Math.PI / veinLen ) + 1;
					FillOblateSpheroid( (int)veinX, (int)veinY, (int)veinZ, radius, block );
				}
			}
		}
		
		void FloodFillWaterBorders() {
			int waterY = waterLevel - 1;
			int index1 = (waterY * length + 0) * width + 0;
			int index2 = (waterY * length + (length - 1)) * width + 0;
			CurrentState = "Flooding edge water";
			
			for( int x = 0; x < width; x++ ) {
				CurrentProgress = 0 + ((float)x / width) * 0.5f;
				FloodFill( index1, (byte)Block.Water );
				FloodFill( index2, (byte)Block.Water );
				index1++; index2++;
			}
			
			index1 = (waterY * length + 0) * width + 0;
			index2 = (waterY * length + 0) * width + (width - 1);
			for( int z = 0; z < length; z++ ) {
				CurrentProgress = 0.5f + ((float)z / length) * 0.5f;
				FloodFill( index1, (byte)Block.Water );
				FloodFill( index2, (byte)Block.Water );
				index1 += width; index2 += width;
			}
		}
		
		void FloodFillWater() {
			int numSources = width * length / 800;
			CurrentState = "Flooding water";
			
			for( int i = 0; i < numSources; i++ ) {
				CurrentProgress = (float)i / numSources;
				int x = rnd.Next( width ), z = rnd.Next( length );
				int y = waterLevel - rnd.Next( 1, 3 );
				FloodFill( (y * length + z) * width + x, (byte)Block.Water );
			}
		}
		
		void FloodFillLava() {
			int numSources = width * length / 20000;
			CurrentState = "Flooding lava";
			
			for( int i = 0; i < numSources; i++ ) {
				CurrentProgress = (float)i / numSources;
				int x = rnd.Next( width ), z = rnd.Next( length );
				int y = (int)((waterLevel - 3) * rnd.NextDouble() * rnd.NextDouble());
				FloodFill( (y * length + z) * width + x, (byte)Block.Lava );
			}
		}
		
		void CreateSurfaceLayer() {
			Noise n1 = new OctaveNoise( 8, rnd ), n2 = new OctaveNoise( 8, rnd );
			CurrentState = "Creating surface";
			// TODO: update heightmap
			
			int hMapIndex = 0;
			for( int z = 0; z < length; z++ ) {
				CurrentProgress = (float)z / length;
				for( int x = 0; x < width; x++ ) {
					bool sand = n1.Compute( x, z ) > 8;
					bool gravel = n2.Compute( x, z ) > 12;
					int y = heightmap[hMapIndex++];
					if( y >= height ) continue;
					
					int index = (y * length + z) * width + x;
					byte blockAbove = y >= (height - 1) ? (byte)0 : blocks[index + oneY];
					if( blockAbove == (byte)Block.Water && gravel ) {
						blocks[index] = (byte)Block.Gravel;
					} else if( blockAbove == 0 ) {
						blocks[index] = (y <= waterLevel && sand) ?
							(byte)Block.Sand : (byte)Block.Grass;
					}
				}
			}
		}
		
		void PlantFlowers() {
			int numPatches = width * length / 3000;
			CurrentState = "Planting flowers";
			
			for( int i = 0; i < numPatches; i++ ) {
				CurrentProgress = (float)i / numPatches;
				byte type = (byte)((byte)Block.Dandelion + rnd.Next( 0, 2 ) );
				int patchX = rnd.Next( width ), patchZ = rnd.Next( length );
				for( int j = 0; j < 10; j++ ) {
					int flowerX = patchX, flowerZ = patchZ;
					for( int k = 0; k < 5; k++ ) {
						flowerX += rnd.Next( 6 ) - rnd.Next( 6 );
						flowerZ += rnd.Next( 6 ) - rnd.Next( 6 );
						if( flowerX < 0 || flowerZ < 0 || flowerX >= width || flowerZ >= length )
							continue;
						
						int flowerY = heightmap[flowerZ * width + flowerX] + 1;
						int index = (flowerY * length + flowerZ) * width + flowerX;
						if( blocks[index] == 0 && blocks[index - oneY] == (byte)Block.Grass )
							blocks[index] = type;
					}
				}
			}
		}
		
		void PlantMushrooms() {
			int numPatches = width * length / 2000;
			CurrentState = "Planting mushrooms";
			
			for( int i = 0; i < numPatches; i++ ) {
				CurrentProgress = (float)i / numPatches;
				byte type = (byte)((byte)Block.BrownMushroom + rnd.Next( 0, 2 ) );
				int patchX = rnd.Next( width );
				int patchY = rnd.Next( height );
				int patchZ = rnd.Next( length );
				
				for( int j = 0; j < 20; j++ ) {
					int mushX = patchX, mushY = patchY, mushZ = patchZ;
					for( int k = 0; k < 5; k++ ) {
						mushX += rnd.Next( 6 ) - rnd.Next( 6 );
						mushZ += rnd.Next( 6 ) - rnd.Next( 6 );
						if( mushX < 0 || mushZ < 0 || mushX >= width || mushZ >= length)
							continue;
						int solidHeight = heightmap[mushZ * width + mushX];
						if( mushY >= (solidHeight - 1) )
							continue;
						
						int index = (mushY * length + mushZ) * width + mushX;
						if( blocks[index] == 0 && blocks[index - oneY] == (byte)Block.Stone )
							blocks[index] = type;
					}
				}
			}
		}
		
		void PlantTrees() {
			int numPatches = width * length / 4000;
			CurrentState = "Planting tress";
			
			for( int i = 0; i < numPatches; i++ ) {
				CurrentProgress = (float)i / numPatches;
				int patchX = rnd.Next( width ), patchZ = rnd.Next( length );
				
				for( int j = 0; j < 20; j++ ) {
					int treeX = patchX, treeZ = patchZ;
					for( int k = 0; k < 20; k++ ) {
						treeX += rnd.Next( 6 ) - rnd.Next( 6 );
						treeZ += rnd.Next( 6 ) - rnd.Next( 6 );
						if( treeX < 0 || treeZ < 0 || treeX >= width ||
						   treeZ >= length || rnd.NextDouble() >= 0.25 )
							continue;
						
						int treeY = heightmap[treeZ * width + treeX] + 1;
						int treeHeight = 5 + rnd.Next( 3 );
						if( CanGrowTree( treeX, treeY, treeZ, treeHeight ) ) {
							GrowTree( treeX, treeY, treeZ, treeHeight );
						}
					}
				}
			}
		}
		
		bool CanGrowTree( int treeX, int treeY, int treeZ, int treeHeight ) {
			// check tree base
			int baseHeight = treeHeight - 4;
			for( int y = treeY; y < treeY + baseHeight; y++ )
				for( int z = treeZ - 1; z <= treeZ + 1; z++ )
					for( int x = treeX - 1; x <= treeX + 1; x++ )
			{
				if( x < 0 || y < 0 || z < 0 || x >= width || y >= height || z >= length )
					return false;
				int index = (y * length + z) * width + x;
				if( blocks[index] != 0 ) return false;
			}
			
			// and also check canopy
			for( int y = treeY + baseHeight; y < treeY + treeHeight; y++ )
				for( int z = treeZ - 2; z <= treeZ + 2; z++ )
					for( int x = treeX - 2; x <= treeX + 2; x++ )
			{
				if( x < 0 || y < 0 || z < 0 || x >= width || y >= height || z >= length )
					return false;
				int index = (y * length + z) * width + x;
				if( blocks[index] != 0 ) return false;
			}
			return true;
		}
		
		void GrowTree( int treeX, int treeY, int treeZ, int height ) {
			int baseHeight = height - 4;
			int index = 0;
			
			// leaves bottom layer
			for( int y = treeY + baseHeight; y < treeY + baseHeight + 2; y++ )
				for( int zz = -2; zz <= 2; zz++ )
					for( int xx = -2; xx <= 2; xx++ )
			{
				int x = xx + treeX, z = zz + treeZ;
				index = (y * length + z) * width + x;
				
				if( Math.Abs( xx ) == 2 && Math.Abs( zz ) == 2 ) {
					if( rnd.NextDouble() >= 0.5 )
						blocks[index] = (byte)Block.Leaves;
				} else {
					blocks[index] = (byte)Block.Leaves;
				}
			}
			
			// leaves top layer
			int bottomY = treeY + baseHeight + 2;
			for( int y = treeY + baseHeight + 2; y < treeY + height; y++ )
				for( int zz = -1; zz <= 1; zz++ )
					for( int xx = -1; xx <= 1; xx++ )
			{
				int x = xx + treeX, z = zz + treeZ;
				index = (y * length + z) * width + x;

				if( xx == 0 || zz == 0 ) {
					blocks[index] = (byte)Block.Leaves;
				} else if( y == bottomY && rnd.NextDouble() >= 0.5 ) {
					blocks[index] = (byte)Block.Leaves;
				}
			}
			
			// then place trunk
			index = (treeY * length + treeZ ) * width + treeX;
			for( int y = 0; y < height - 1; y++ ) {
				blocks[index] = (byte)Block.Wood;
				index += oneY;
			}
		}
	}
}