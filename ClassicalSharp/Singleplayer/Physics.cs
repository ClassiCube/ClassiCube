using System;
using System.Collections.Generic;
using OpenTK;

namespace ClassicalSharp.Singleplayer {

	public class Physics {
		
		Game game;
		public Map map;
		BlockInfo info;
		int width, length, height, oneY;
		
		const uint tickMask = 0xF8000000;
		const uint posMask =  0x07FFFFFF;
		const int tickShift = 27;
		
		public Physics( Game game ) {
			this.game = game;
			map = game.Map;
			info = game.BlockInfo;
		}
		
		bool CheckItem( Queue<uint> queue, out int posIndex ) {
			uint packed = queue.Dequeue();
			int tickDelay = (int)((packed & tickMask) >> tickShift);
			posIndex = (int)(packed & posMask);

			if( tickDelay > 0 ) {
				tickDelay--;
				queue.Enqueue( (uint)posIndex | ((uint)tickDelay << tickShift) );
				return false;
			}
			return true;
		}
		
		bool CheckItem( Queue<uint> queue, int mask, out int posIndex, out int flags ) {
			uint packed = queue.Dequeue();
			flags = (int)((packed & tickMask) >> tickShift);
			posIndex = (int)(packed & posMask);
			int tickDelay = flags & mask;

			if( tickDelay > 0 ) {
				tickDelay--;
				flags &= ~mask; // zero old tick delay bits
				flags |= tickDelay; // then set them with new value
				queue.Enqueue( (uint)posIndex | ((uint)flags << tickShift) );
				return false;
			}
			return true;
		}
		
		public void Tick() {
			TickLava();
			TickWater();
			TickFalling();
		}
		
		public void OnBlockPlaced( int x, int y, int z, byte block ) {
			int index = ( y * length + z ) * width + x;
			if( block == (byte)Block.Lava ) {
				Lava.Enqueue( defLavaTick | (uint)index );
			} else if( block == (byte)Block.Water ) {
				Water.Enqueue( defWaterTick | (uint)index );
			} else if( block == (byte)Block.Sand || block == (byte)Block.Gravel ) {
				Falling.Enqueue( defFallingTick | (uint)index );
			} else if( block == (byte)Block.TNT ) {
				Explode( 4, x, y, z );
			}
		}
		
		public void ResetMap() {
			Lava.Clear();
			Water.Clear();
			width = map.Width;
			length = map.Length;
			height = map.Height;
			oneY = height * width;
		}
		
		#region Lava
		
		Queue<uint> Lava = new Queue<uint>();
		const uint defLavaTick = 30u << tickShift;
		
		void TickLava() {
			int count = Lava.Count;
			for( int i = 0; i < count; i++ ) {
				int posIndex;
				if( CheckItem( Lava, out posIndex ) ) {
					byte block = map.mapData[posIndex];
					if( !(block == (byte)Block.Lava || block == (byte)Block.StillLava) ) continue;
					
					int x = posIndex % width;
					int y = posIndex / oneY; // posIndex / (width * length)
					int z = (posIndex / width) % length;
					
					if( x > 0 ) PropagateLava( posIndex - 1, x - 1, y, z );
					if( x < width - 1 ) PropagateLava( posIndex + 1, x + 1, y, z );
					if( z > 0 ) PropagateLava( posIndex - width, x, y, z - 1 );
					if( z < length - 1 ) PropagateLava( posIndex + width, x, y, z + 1 );
					if( y > 0 ) PropagateLava( posIndex - oneY, x, y - 1, z );
				}
			}
		}
		
		void PropagateLava( int posIndex, int x, int y, int z ) {
			byte block = map.mapData[posIndex];
			if( block == (byte)Block.Water || block == (byte)Block.StillWater ) {
				game.UpdateBlock( x, y, z, (byte)Block.Stone );
			} else if( info.CollideType[block] == BlockCollideType.WalkThrough ) {
				Lava.Enqueue( defLavaTick | (uint)posIndex );
				game.UpdateBlock( x, y, z, (byte)Block.Lava );
			}
		}
		
		#endregion
		
		#region Water
		
		Queue<uint> Water = new Queue<uint>();
		const uint defWaterTick = 5u << tickShift;
		
		void TickWater() {
			int count = Water.Count;
			for( int i = 0; i < count; i++ ) {
				int posIndex;
				if( CheckItem( Water, out posIndex ) ) {
					byte block = map.mapData[posIndex];
					if( !(block == (byte)Block.Water || block == (byte)Block.StillWater) ) continue;
					
					int x = posIndex % width;
					int y = posIndex / oneY; // posIndex / (width * length)
					int z = (posIndex / width) % length;
					
					if( x > 0 ) PropagateWater( posIndex - 1, x - 1, y, z );
					if( x < width - 1 ) PropagateWater( posIndex + 1, x + 1, y, z );
					if( z > 0 ) PropagateWater( posIndex - width, x, y, z - 1 );
					if( z < length - 1 ) PropagateWater( posIndex + width, x, y, z + 1 );
					if( y > 0 ) PropagateWater( posIndex - oneY, x, y - 1, z );
				}
			}
		}
		
		void PropagateWater( int posIndex, int x, int y, int z ) {
			byte block = map.mapData[posIndex];
			if( block == (byte)Block.Lava || block == (byte)Block.StillLava ) {
				game.UpdateBlock( x, y, z, (byte)Block.Stone );
			} else if( info.CollideType[block] == BlockCollideType.WalkThrough ) {
				Water.Enqueue( defWaterTick | (uint)posIndex );
				game.UpdateBlock( x, y, z, (byte)Block.Water );
			}
		}
		
		#endregion
		
		#region TNT
		
		Vector3[] rayDirs;
		Random rnd = new Random();
		const float stepLen = 0.3f;
		float[] hardness;
		
		// Algorithm source: http://minecraft.gamepedia.com/Explosion
		void Explode( float power, int x, int y, int z ) {
			if( rayDirs == null )
				InitExplosionCache();
			
			game.UpdateBlock( x, y, z, 0 );
			Vector3 basePos = new Vector3( x, y, z );
			for( int i = 0; i < rayDirs.Length; i++ ) {
				Vector3 dir = rayDirs[i] * stepLen;
				Vector3 position = basePos;
				float intensity = (float)(0.7 + rnd.NextDouble() * 0.6) * power;
				while( intensity > 0 ) {
					position += dir;
					intensity -= stepLen * 0.75f;
					Vector3I blockPos = Vector3I.Floor( position );
					if( !map.IsValidPos( blockPos ) ) break;
					
					byte block = map.GetBlock( blockPos );
					intensity -= (hardness[block] / 5 + 0.3f) * stepLen; // TODO: proper block hardness table
					if( intensity > 0 && block != 0 ) {
						game.UpdateBlock( blockPos.X, blockPos.Y, blockPos.Z, 0 );
					}
				}
			}
		}
		
		void InitExplosionCache() {
			hardness = new float[] { 0, 30, 3, 2.5f, 30, 15, 0, 1.8E+07f, 500, 500, 500, 500, 2.5f, 
				3, 15, 15, 15, 10, 1, 3, 1.5f, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 
				0, 0, 30, 30, 30, 30, 30, 0, 7.5f, 30, 6000, 30, 0, 4, 0.5f, 0, 4, 4, 4, 4, 4, 2.5f,
				// Note that the 30, 500, 15, 15 are guesses (CeramicTile --> Crate)
				30, 500, 15, 15, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,				
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			rayDirs = new Vector3[1352];
			int index = 0;
			
			// Y bottom and top planes			
			for( float x = -1; x <= 1.001f; x += 2f/15) {
				for( float z = -1; z <= 1.001f; z += 2f/15) {
					rayDirs[index++] = Vector3.Normalize( x, -1, z );
					rayDirs[index++] = Vector3.Normalize( x, +1, z );
				}
			}
			// Z planes
			for( float y = -1 + 2f/15; y <= 1.001f - 2f/15; y += 2f/15) {
				for( float x = -1; x <= 1.001f; x += 2f/15) {
					rayDirs[index++] = Vector3.Normalize( x, y, -1 );
					rayDirs[index++] = Vector3.Normalize( x, y, +1 );
				}
			}
			// X planes
			for( float y = -1 + 2f/15; y <= 1.001f - 2f/15; y += 2f/15) {
				for( float z = -1 + 2f/15; z <= 1.001f- 2f/15; z += 2f/15) {
					rayDirs[index++] = Vector3.Normalize( -1, y, z );
					rayDirs[index++] = Vector3.Normalize( +1, y, z );
				}
			}
		}
		
		#endregion
		
		#region Sapling
		// TODO: tree growing
		#endregion
		
		#region Grass
		// TODO: grass growing
		#endregion
		
		#region Sand/Gravel
		
		Queue<uint> Falling = new Queue<uint>();
		const uint defFallingTick = 2u << tickShift;

		void TickFalling() {
			int count = Falling.Count;
			for( int i = 0; i < count; i++ ) {
				int posIndex, flags;
				if( CheckItem( Falling, 0x2, out posIndex, out flags ) ) {
					byte block = map.mapData[posIndex];
					if( !(block == (byte)Block.Sand || block == (byte)Block.Gravel ) ) continue;
					
					int x = posIndex % width;
					int y = posIndex / oneY; // posIndex / (width * length)
					int z = (posIndex / width) % length;
					if( y > 0 ) PropagateFalling( posIndex - oneY, x, y - 1, z, block, flags );
				}
			}
		}
		
		void PropagateFalling( int posIndex, int x, int y, int z, byte block, int flags ) {
			byte newBlock = map.mapData[posIndex];
			if( newBlock == 0 || info.IsLiquid[newBlock] ) {
				uint newFlags = MakeFallingFlags( newBlock ) << tickShift;
				Falling.Enqueue( newFlags | (uint)posIndex );
				game.UpdateBlock( x, y, z, oldBlock[flags >> 2] );
				game.UpdateBlock( x, y - 1, z, block );
			}
		}
		
		static byte[] oldBlock = new byte[] { (byte)Block.Air, (byte)Block.StillWater,
			(byte)Block.Water, (byte)Block.StillLava, (Byte)Block.Lava };
		static uint MakeFallingFlags( byte above ) {
			byte flags = 2;
			if( above == (byte)Block.StillWater ) flags |= 0x04; // 1
			else if( above == (byte)Block.Water ) flags |= 0x08; // 2
			else if( above == (byte)Block.StillLava ) flags |= 0x0C; // 3
			else if( above == (byte)Block.Lava ) flags |= 0x10; // 4
			return flags;
		}
		
		#endregion
	}
}