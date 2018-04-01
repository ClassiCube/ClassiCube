// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Map;
using BlockRaw = System.Byte;

namespace ClassicalSharp.Singleplayer {

	public class OtherPhysics {
		Game game;
		World map;
		
		public OtherPhysics(Game game, PhysicsBase physics) {
			this.game = game;
			map = game.World;
			physics.OnPlace[Block.Slab] = HandleSlab;
			physics.OnPlace[Block.CobblestoneSlab] = HandleCobblestoneSlab;
		}
		
		void HandleSlab(int index, BlockRaw block) {
			if (index < map.OneY) return;
			if (map.blocks[index - map.OneY] != Block.Slab) return;
			
			int x = index % map.Width;
			int z = (index / map.Width) % map.Length;
			int y = (index / map.Width) / map.Length;
			game.UpdateBlock(x, y, z, Block.Air);
			game.UpdateBlock(x, y - 1, z, Block.DoubleSlab);
		}
		
		void HandleCobblestoneSlab(int index, BlockRaw block) {
			if (index < map.OneY) return;
			if (map.blocks[index - map.OneY] != Block.CobblestoneSlab) return;
			
			int x = index % map.Width;
			int z = (index / map.Width) % map.Length;
			int y = (index / map.Width) / map.Length;
			game.UpdateBlock(x, y, z, Block.Air);
			game.UpdateBlock(x, y - 1, z, Block.Cobblestone);
		}
	}
}