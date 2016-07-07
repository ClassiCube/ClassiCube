// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using ClassicalSharp.Map;

namespace ClassicalSharp.Singleplayer {

	public class FallingPhysics {
		Game game;
		World map;
		BlockInfo info;
		int width, length, height, oneY;
		
		public FallingPhysics( Game game, Physics physics ) {
			this.game = game;
			map = game.World;
			info = game.BlockInfo;
			
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

		void DoFalling( int index, byte block ) {
			int found = -1, start = index;
			// Find lowest air block
			while( index >= oneY ) {
				index -= oneY;
				byte other = map.blocks[index];
				if( other == 0 || info.IsLiquid[other] )
					found = index;
				else
					break;
			}			
			if( found == -1 ) return;
			
			int x = start % width;
			int y = start / oneY; // posIndex / (width * length)
			int z = (start / width) % length;
			game.UpdateBlock( x, y, z, 0 );
			
			x = found % width;
			y = found / oneY; // posIndex / (width * length)
			z = (found / width) % length;
			game.UpdateBlock( x, y, z, block );
		}
	}
}