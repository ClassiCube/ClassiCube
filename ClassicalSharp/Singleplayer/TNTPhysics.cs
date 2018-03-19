// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Map;
using OpenTK;
using BlockRaw = System.Byte;

namespace ClassicalSharp.Singleplayer {

	public class TNTPhysics {
		Game game;
		World map;
		PhysicsBase physics;
		Random rnd = new Random();
		
		public TNTPhysics(Game game, PhysicsBase physics) {
			this.game = game;
			map = game.World;
			this.physics = physics;
			physics.OnPlace[Block.TNT] = HandleTnt;
		}
		
		bool[] blocksTnt;
		
		void HandleTnt(int index, BlockRaw block) {
			int x = index % map.Width;
			int z = (index / map.Width) % map.Length;
			int y = (index / map.Width) / map.Length;
			Explode(4, x, y, z);
		}
		
		public void Explode(int power, int x, int y, int z) {
			if (blocksTnt == null)
				InitExplosionCache();
			
			game.UpdateBlock(x, y, z, Block.Air);
			int index = (y * map.Length + z) * map.Width + x;
			physics.ActivateNeighbours(x, y, z, index);
			
			int powerSquared = power * power;
			for (int dy = -power; dy <= power; dy++)
				for (int dz = -power; dz <= power; dz++)
					for (int dx = -power; dx <= power; dx++)
			{
				if (dx * dx + dy * dy + dz * dz > powerSquared) continue;
				
				int xx = x + dx, yy = y + dy, zz = z + dz;
				if (!map.IsValidPos(xx, yy, zz)) continue;
				index = (yy * map.Length + zz) * map.Width + xx;
				
				BlockRaw block = map.blocks1[index];
				if (block < Block.CpeCount && blocksTnt[block]) continue;		
				
				game.UpdateBlock(xx, yy, zz, Block.Air);				
				physics.ActivateNeighbours(xx, yy, zz, index);
			}
		}
		
		void InitExplosionCache() {
			blocksTnt = new bool[Block.CpeCount];
			blocksTnt[Block.Stone] = true;
			blocksTnt[Block.Cobblestone] = true;
			blocksTnt[Block.Bedrock] = true;
			for (int i = Block.Water; i <= Block.StillLava; i++)
				blocksTnt[i] = true;
			
			blocksTnt[Block.GoldOre] = true;
			blocksTnt[Block.IronOre] = true;
			blocksTnt[Block.CoalOre] = true;
			blocksTnt[Block.Gold] = true;
			blocksTnt[Block.Iron] = true;
			blocksTnt[Block.DoubleSlab] = true;
			blocksTnt[Block.Slab] = true;
			blocksTnt[Block.Brick] = true;
			blocksTnt[Block.MossyRocks] = true;
			blocksTnt[Block.Obsidian] = true;
			// CPE guesses
			blocksTnt[Block.CobblestoneSlab] = true;
			blocksTnt[Block.Sandstone] = true;
			blocksTnt[Block.CeramicTile] = true;
			blocksTnt[Block.Magma] = true;
			blocksTnt[Block.Pillar] = true;
			blocksTnt[Block.Crate] = true;
			blocksTnt[Block.StoneBrick] = true;
		}
	}
}