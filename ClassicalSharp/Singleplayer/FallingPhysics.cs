// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using ClassicalSharp.Map;

namespace ClassicalSharp.Singleplayer {

	public class FallingPhysics {
		Game game;
		PhysicsBase physics;
		World map;
		BlockInfo info;
		int width, length, height, oneY;
		
		public FallingPhysics(Game game, PhysicsBase physics) {
			this.game = game;
			map = game.World;
			info = game.BlockInfo;
			this.physics = physics;
			
			physics.OnPlace[Block.Sand] = DoFalling;
			physics.OnPlace[Block.Gravel] = DoFalling;
			physics.OnActivate[Block.Sand] = DoFalling;
			physics.OnActivate[Block.Gravel] = DoFalling;
			physics.OnRandomTick[Block.Sand] = DoFalling;
			physics.OnRandomTick[Block.Gravel] = DoFalling;
		}
		
		public void ResetMap() {
			width = map.Width;
			height = map.Height;
			length = map.Length;
			oneY = width * length;
		}

		void DoFalling(int index, byte block) {
			int found = -1, start = index;
			// Find lowest air block
			while (index >= oneY) {
				index -= oneY;
				byte other = map.blocks[index];
				if (other == Block.Air || (other >= Block.Water && other <= Block.StillLava))
					found = index;
				else
					break;
			}
			if (found == -1) return;

			int x = found % width;
			int y = found / oneY; // posIndex / (width * length)
			int z = (found / width) % length;
			game.UpdateBlock(x, y, z, block);
			
			x = start % width;
			y = start / oneY; // posIndex / (width * length)
			z = (start / width) % length;
			game.UpdateBlock(x, y, z, Block.Air);
			physics.ActivateNeighbours(x, y, z, start);
		}
	}
}