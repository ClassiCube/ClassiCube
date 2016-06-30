// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Map;

namespace ClassicalSharp.Singleplayer {

	public class OtherPhysics {
		Game game;
		World map;
		
		public OtherPhysics( Game game, Physics physics ) {
			this.game = game;
			map = game.World;
			physics.OnPlace[Block.Slab] = HandleSlab;
			physics.OnPlace[Block.CobblestoneSlab] = HandleCobblestoneSlab;
		}
		
		void HandleSlab( int index, byte block ) {
			if( index < map.Width * map.Length ) return; // y < 1
			if( map.blocks[index - map.Width * map.Length] != Block.Slab ) return;
			
			int x = index % map.Width;
			int z = (index / map.Width) % map.Length;
			int y = (index / map.Width) / map.Length;
			game.UpdateBlock( x, y, z, Block.Air );
			game.UpdateBlock( x, y - 1, z, Block.DoubleSlab );
		}
		
		void HandleCobblestoneSlab( int index, byte block ) {
			if( index < map.Width * map.Length ) return; // y < 1
			if( map.blocks[index - map.Width * map.Length] != Block.CobblestoneSlab ) return;
			
			int x = index % map.Width;
			int z = (index / map.Width) % map.Length;
			int y = (index / map.Width) / map.Length;
			game.UpdateBlock( x, y, z, Block.Air );
			game.UpdateBlock( x, y - 1, z, Block.Cobblestone );
		}
	}
}