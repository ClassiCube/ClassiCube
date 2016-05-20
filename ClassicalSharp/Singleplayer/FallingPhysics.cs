// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Singleplayer {

	public class FallingPhysics {
		
		Game game;
		World map;
		BlockInfo info;
		int width, length, height, oneY;
		
		public FallingPhysics( Game game ) {
			this.game = game;
			map = game.World;
			info = game.BlockInfo;
		}
		
		public void ResetMap() {
			Falling.Clear();
			width = map.Width;
			height = map.Height;
			length = map.Length;	
			oneY = width * length;
		}
		
		public void Clear() { Falling.Clear(); }
		
		public void OnBlockPlace( int index, byte block ) {
			Falling.Enqueue( defFallingTick | (uint)index );
		}
		
		public void OnRandomTick( int index, byte block ) {
			if( index >= oneY ) PropagateFalling( index, block, 0 );
		}
		
		Queue<uint> Falling = new Queue<uint>();
		const uint defFallingTick = 2u << Physics.tickShift;

		public void Tick() {
			int count = Falling.Count;
			for( int i = 0; i < count; i++ ) {
				int index, flags;
				if( Physics.CheckItem( Falling, 0x2, out index, out flags ) ) {
					byte block = map.blocks[index];
					if( !(block == (byte)Block.Sand || block == (byte)Block.Gravel ) ) continue;
					if( index >= oneY ) PropagateFalling( index, block, flags );
				}
			}
		}
		
		void PropagateFalling( int index, byte block, int flags ) {
			byte newBlock = map.blocks[index - oneY];
			if( newBlock == 0 || info.IsLiquid[newBlock] ) {
				int x = index % width;
				int y = index / oneY; // posIndex / (width * length)
				int z = (index / width) % length;
					
				uint newFlags = MakeFallingFlags( newBlock ) << Physics.tickShift;
				Falling.Enqueue( newFlags | (uint)(index - oneY) );
				
				game.UpdateBlock( x, y, z, oldBlock[flags >> 2] );
				game.UpdateBlock( x, y - 1, z, block );
			}
		}
		
		static byte[] oldBlock = new byte[] { (byte)Block.Air, (byte)Block.StillWater,
			(byte)Block.Water, (byte)Block.StillLava, (byte)Block.Lava };
		static uint MakeFallingFlags( byte above ) {
			byte flags = 2;
			if( above == (byte)Block.StillWater ) flags |= 0x04; // 1
			else if( above == (byte)Block.Water ) flags |= 0x08; // 2
			else if( above == (byte)Block.StillLava ) flags |= 0x0C; // 3
			else if( above == (byte)Block.Lava ) flags |= 0x10; // 4
			return flags;
		}
	}
}