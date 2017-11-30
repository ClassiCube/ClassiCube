// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using ClassicalSharp.Map;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Singleplayer {

	public class LiquidPhysics {
		
		Game game;
		World map;
		Random rnd = new Random();
		int width, length, height, oneY;
		int maxX, maxY, maxZ, maxWaterX, maxWaterY, maxWaterZ;		
		bool isLavaDown = true;
		bool isWaterDown = true;
		Random lavarand = new Random();
				
		const uint tickMask = 0xF8000000;
		const uint posMask =  0x07FFFFFF;
		const int tickShift = 27;

		public LiquidPhysics(Game game, PhysicsBase physics) {
			this.game = game;
			map = game.World;
			
			physics.OnPlace[Block.Lava] = OnPlaceLava;
			physics.OnPlace[Block.Water] = OnPlaceWater;
			physics.OnPlace[Block.Sponge] = PlaceSponge;
			physics.OnDelete[Block.Sponge] = DeleteSponge;
			
			physics.OnActivate[Block.Water] = physics.OnPlace[Block.Water];
			physics.OnActivate[Block.StillWater] = physics.OnPlace[Block.Water];
			physics.OnActivate[Block.Lava] = physics.OnPlace[Block.Lava];
			physics.OnActivate[Block.StillLava] = physics.OnPlace[Block.Lava];
			
			physics.OnRandomTick[Block.Water] = RandomTickWater;
			physics.OnRandomTick[Block.StillWater] = RandomTickWater;
			physics.OnRandomTick[Block.Lava] = RandomTickLava;
			physics.OnRandomTick[Block.StillLava] = RandomTickLava;
		}
		
		void RandomTickLava(int index, BlockID b) { ActivateLava(index, b, 0, true); }
		void RandomTickWater(int index, BlockID b) { ActivateWater(index, b, false); }
		void OnPlaceLava(int index, BlockID b) { Lava.Enqueue(defLavaTick | (uint)index); }
		void OnPlaceWater(int index, BlockID b) { Water.Enqueue(defWaterTick | (uint)index); }
		
		public void Clear() { Lava.Clear(); Water.Clear(); }
		
		public void ResetMap() {
			Clear();
			width = map.Width;   maxX = width  - 1; maxWaterX = maxX - 2;
			height = map.Height; maxY = height - 1; maxWaterY = maxY - 2;
			length = map.Length; maxZ = length - 1; maxWaterZ = maxZ - 2;
			oneY = width * length;
		}		
				
		static bool CheckItem(Queue<uint> queue, out int posIndex) {
			uint packed = queue.Dequeue();
			int tickDelay = (int)((packed & tickMask) >> tickShift);
			posIndex = (int)(packed & posMask);

			if (tickDelay > 0) {
				tickDelay--;
				queue.Enqueue((uint)posIndex | ((uint)tickDelay << tickShift));
				return false;
			}
			return true;
		}

		
		Queue<uint> Lava = new Queue<uint>();
		const uint defLavaTick = 6u << tickShift;
		const uint defQuickLavaTick = 0u << tickShift;
		
		public void TickLava() {
			int count = Lava.Count;
			//isLavaDown = true;
			for (int i = 0; i < count; i++) {
				int index;
				if (CheckItem(Lava, out index)) {
					BlockID block = map.blocks[index];
					if (!(block == Block.Lava || block == Block.StillLava)) continue;
					ActivateLava(index, block, defLavaTick, false);
				}
			}
		}
		
		void ActivateLava(int index, BlockID block, uint delay, bool skipEnq) {
			int x = index % width;
			int y = index / oneY; // posIndex / (width * length)
			int z = (index / width) % length;
			int randvar = lavarand.Next(1, 100);
			
			if (y > 0 && map.blocks[index - oneY] != Block.Air || y == 0) {
				if (x > 0) PropagateLava(index - 1, x - 1, y, z, delay);
				if (x < width - 1) PropagateLava(index + 1, x + 1, y, z, delay);
				if (z > 0) PropagateLava(index - width, x, y, z - 1, delay);
				if (z < length - 1) PropagateLava(index + width, x, y, z + 1, delay);
				if (y > 1) {
					if (map.blocks[index - oneY] != Block.Water &&
						map.blocks[index - oneY] != Block.StillWater) {
						if (skipEnq) return;
						Lava.Enqueue(defLavaTick | (uint)(index - oneY));
					} else {
						PropagateLavaY(index - oneY, x, y - 1, z, delay);
					}
				}
			} else {
				if (y > 0) {
					PropagateLavaY(index - oneY, x, y - 1, z, delay);
					if (randvar <= 20) {
						if (y < height - 1 && map.blocks[index + oneY] == Block.Lava ||
						    y < height - 1 && map.blocks[index + oneY] == Block.StillLava) {
							Lava.Enqueue(0 | (uint)(index - oneY));
						}
					}
				}
			}
		}
		
		void PropagateLava(int posIndex, int x, int y, int z, uint delay) {
			BlockID block = map.blocks[posIndex];
			BlockID block2 = block;
			if (y > 1) {
				block2 = map.blocks[posIndex - oneY];
			}
			
			if (block == Block.Water || block == Block.StillWater) {
				game.UpdateBlock(x, y, z, Block.Stone);
			} else if (BlockInfo.Collide[block] == CollideType.Gas) {
				Lava.Enqueue(defLavaTick | (uint)posIndex);
				game.UpdateBlock(x, y, z, Block.Lava);
			}
			
			if (block2 == Block.Water || block2 == Block.StillWater) {
				game.UpdateBlock(x, y - 1, z, Block.Stone);
			}
		}
		
		void PropagateLavaY(int posIndex, int x, int y, int z, uint delay) {
			BlockID block = map.blocks[posIndex];
			BlockID block2 = block;
			if (y > 1) {
				block2 = map.blocks[posIndex - oneY];
			}
			
			if (block == Block.Water || block == Block.StillWater) {
				game.UpdateBlock(x, y, z, Block.Stone);
			} else if (BlockInfo.Collide[block] == CollideType.Gas) {
				Lava.Enqueue(defLavaTick | (uint)(posIndex + oneY));
				game.UpdateBlock(x, y, z, Block.Lava);
			}
			
			if (block2 == Block.Water || block2 == Block.StillWater) {
				game.UpdateBlock(x, y - 1, z, Block.Stone);
			}
		}
		
		Queue<uint> Water = new Queue<uint>();
		const uint defWaterTick = 0u << tickShift;
		
		public void TickWater() {
			int count = Water.Count;
			for (int i = 0; i < count; i++) {
				int index;
				if (CheckItem(Water, out index)) {
					BlockID block = map.blocks[index];
					if (!(block == Block.Water || block == Block.StillWater)) continue;
					ActivateWater(index, block, false);
				}
			}
		}
		
		void ActivateWater(int index, BlockID block, bool noVer) {
			int x = index % width;
			int y = index / oneY; // posIndex / (width * length)
			int z = (index / width) % length;
			if (index > oneY && map.blocks[index - oneY] != Block.Air || y == 0) {
				if (x > 0) PropagateWater(index - 1, x - 1, y, z);
				if (x < width - 1) PropagateWater(index + 1, x + 1, y, z);
				if (z > 0) PropagateWater(index - width, x, y, z - 1);
				if (z < length - 1) PropagateWater(index + width, x, y, z + 1);
			} else {
				Water.Enqueue(0 |(uint)index);
			}
			if (!noVer) {
				if (y > 0) PropagateWaterY(index - oneY, x, y - 1, z);
			}
		}
		
		void PropagateWater(int posIndex, int x, int y, int z) {
			BlockID block = map.blocks[posIndex];
			if (block == Block.Lava || block == Block.StillLava) {
				game.UpdateBlock(x, y, z, Block.Stone);
			} else if (BlockInfo.Collide[block] == CollideType.Gas && block != Block.Rope) {
				// Sponge check
				for (int yy = (y < 2 ? 0 : y - 2); yy <= (y > maxWaterY ? maxY : y + 2); yy++)
					for (int zz = (z < 2 ? 0 : z - 2); zz <= (z > maxWaterZ ? maxZ : z + 2); zz++)
						for (int xx = (x < 2 ? 0 : x - 2); xx <= (x > maxWaterX ? maxX : x + 2); xx++)
				{
					block = map.blocks[(yy * length + zz) * width + xx];
					if (block == Block.Sponge) return;
				}
				
				Water.Enqueue(0 | (uint)posIndex);
				game.UpdateBlock(x, y, z, Block.Water);
			}
		}
		
		void PropagateWaterY(int posIndex, int x, int blkY, int z) {
			BlockID block = map.blocks[posIndex];
			bool sponge = false;
			for (int y = blkY; y >= 0; y--) {
				if (block == Block.Lava || block == Block.StillLava) {
					game.UpdateBlock(x, y, z, Block.Stone);
					break;
				} else if (BlockInfo.Collide[block] == CollideType.Gas && block != Block.Rope) {
					// Sponge check
					for (int yy = (y < 2 ? 0 : y - 2); yy <= (y > maxWaterY ? maxY : y + 2); yy++)
						for (int zz = (z < 2 ? 0 : z - 2); zz <= (z > maxWaterZ ? maxZ : z + 2); zz++)
							for (int xx = (x < 2 ? 0 : x - 2); xx <= (x > maxWaterX ? maxX : x + 2); xx++)
					{
						block = map.blocks[(yy * length + zz) * width + xx];
						if (block == Block.Sponge) sponge = true;
					}
					if (sponge) break;
					
					Water.Enqueue(0 | (uint)posIndex);
					game.UpdateBlock(x, y, z, Block.Water);
					if (posIndex > oneY) {
						posIndex -= oneY;
						block = map.blocks[posIndex];
					} else {
						posIndex += oneY;
						block = map.blocks[posIndex];
						break;
					}
				} else {
					posIndex += oneY;
					block = map.blocks[posIndex];
					break;
				}
			}
			
			ActivateWater(posIndex, block, true);
			
		}

		
		void PlaceSponge(int index, BlockID block) {
			int x = index % width;
			int y = index / oneY; // posIndex / (width * length)
			int z = (index / width) % length;
			
			for (int yy = y - 2; yy <= y + 2; yy++)
				for (int zz = z - 2; zz <= z + 2; zz++)
					for (int xx = x - 2; xx <= x + 2; xx++)
			{
				block = map.SafeGetBlock(xx, yy, zz);
				if (block == Block.Water || block == Block.StillWater)
					game.UpdateBlock(xx, yy, zz, Block.Air);
			}
		}
		
		
		void DeleteSponge(int index, BlockID block) {
			int x = index % width;
			int y = index / oneY; // posIndex / (width * length)
			int z = (index / width) % length;
			
			for (int yy = y - 3; yy <= y + 3; yy++)
				for (int zz = z - 3; zz <= z + 3; zz++)
					for (int xx = x - 3; xx <= x + 3; xx++)
			{
				if (Math.Abs(yy - y) == 3 || Math.Abs(zz - z) == 3 || Math.Abs(xx - x) == 3) {
					if (!map.IsValidPos(xx, yy, zz)) continue;
					
					index = xx + width * (zz + yy * length);
					block = map.blocks[index];
					if (block == Block.Water || block == Block.StillWater)
						Water.Enqueue(0 | (uint)index);
				}
			}
		}
	}
}
