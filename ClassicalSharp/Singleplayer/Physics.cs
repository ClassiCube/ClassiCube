// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Singleplayer {

	public class Physics {
		
		Game game;
		World map;
		Random rnd = new Random();
		BlockInfo info;
		int width, length, height, oneY;
		int maxX, maxY, maxZ, maxWaterX, maxWaterY, maxWaterZ;
		
		const uint tickMask = 0xF8000000;
		const uint posMask =  0x07FFFFFF;
		const int tickShift = 27;
		
		bool enabled = true;
		public bool Enabled {
			get { return enabled; }
			set { enabled = value; ClearQueuedEvents(); }
		}
		
		public Physics( Game game ) {
			this.game = game;
			map = game.World;
			info = game.BlockInfo;
			game.WorldEvents.OnNewMapLoaded += ResetMap;
			enabled = Options.GetBool( OptionsKey.SingleplayerPhysics, true );
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
		
		int tickCount = 0;
		public void Tick() {
			if( !Enabled || game.World.IsNotLoaded ) return;
			
			//if( (tickCount % 5) == 0 ) {
			TickLava();
			TickWater();
			TickFalling();
			//}
			tickCount++;
			TickRandomBlocks();
		}
		
		public void OnBlockPlaced( int x, int y, int z, byte block ) {
			if( !Enabled ) return;
			
			int index = (y * length + z) * width + x;
			if( block == (byte)Block.Lava ) {
				Lava.Enqueue( defLavaTick | (uint)index );
			} else if( block == (byte)Block.Water ) {
				Water.Enqueue( defWaterTick | (uint)index );
			} else if( block == (byte)Block.Sand || block == (byte)Block.Gravel ) {
				Falling.Enqueue( defFallingTick | (uint)index );
			} else if( block == (byte)Block.TNT ) {
				Explode( 4, x, y, z );
			} else if( block == (byte)Block.Sapling ) {
				GrowTree( x, y, z );
			} else if( block == (byte)Block.Sponge ) {
				PlaceSponge( x, y, z );
			}
		}
		
		void ResetMap( object sender, EventArgs e ) {
			ClearQueuedEvents();
			width = map.Width; maxX = width - 1; maxWaterX = maxX - 2;
			height = map.Height; maxY = height - 1; maxWaterY = maxY - 2;
			length = map.Length; maxZ = length - 1; maxWaterZ = maxZ - 2;			
			oneY = width * length;
		}
		
		void ClearQueuedEvents() {
			Lava.Clear();
			Water.Clear();
			Falling.Clear();
		}
		
		public void Dispose() {
			game.WorldEvents.OnNewMapLoaded -= ResetMap;
		}
		
		#region General
		
		void TickRandomBlocks() {
			int xMax = width - 1, yMax = height - 1, zMax = length - 1;
			for( int y = 0; y < height; y += 16 )
				for( int z = 0; z < length; z += 16 )
					for( int x = 0; x < width; x += 16 )
			{
				int lo = (y * length + z) * width + x;
				int hi = (Math.Min( yMax, y + 15 ) * length + Math.Min( zMax, z + 15 ))
					* width + Math.Min( xMax, x + 15 );
				
				HandleBlock( rnd.Next( lo, hi ) );
				HandleBlock( rnd.Next( lo, hi ) );
				HandleBlock( rnd.Next( lo, hi ) );
			}
		}
		
		void HandleBlock( int posIndex ) {
			int x = posIndex % width;
			int y = posIndex / oneY; // posIndex / (width * length)
			int z = (posIndex / width) % length;
			byte block = map.mapData[posIndex];
			
			switch( block ) {
				case (byte)Block.Dirt:
					if( y > map.GetLightHeight( x, z ) )
						game.UpdateBlock( x, y, z, (byte)Block.Grass );
					break;
					
				case (byte)Block.Grass:
					if( y <= map.GetLightHeight( x, z ) ) {
						game.UpdateBlock( x, y, z, (byte)Block.Dirt );
					}
					break;
					
				case (byte)Block.Dandelion:
				case (byte)Block.Rose:
				case (byte)Block.BrownMushroom:
				case (byte)Block.RedMushroom:
					if( y <= map.GetLightHeight( x, z ) )
						game.UpdateBlock( x, y, z, (byte)Block.Air );
					break;
					
				case (byte)Block.Sapling:
					GrowTree( x, y, z );
					break;
					
				case (byte)Block.Sand:
				case (byte)Block.Gravel:
					if( y > 0 ) PropagateFalling( posIndex, x, y, z, block, 0 );
					break;
					
				case (byte)Block.Lava:
					HandleLava( posIndex, x, y, z );
					break;
					
				case (byte)Block.Water:
					HandleWater( posIndex, x, y, z );
					break;
			}
		}
		#endregion
		
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
					HandleLava( posIndex, x, y, z );
				}
			}
		}
		
		void HandleLava( int posIndex, int x, int y, int z ) {
			if( x > 0 ) PropagateLava( posIndex - 1, x - 1, y, z );
			if( x < width - 1 ) PropagateLava( posIndex + 1, x + 1, y, z );
			if( z > 0 ) PropagateLava( posIndex - width, x, y, z - 1 );
			if( z < length - 1 ) PropagateLava( posIndex + width, x, y, z + 1 );
			if( y > 0 ) PropagateLava( posIndex - oneY, x, y - 1, z );
		}
		
		void PropagateLava( int posIndex, int x, int y, int z ) {
			byte block = map.mapData[posIndex];
			if( block == (byte)Block.Water || block == (byte)Block.StillWater ) {
				game.UpdateBlock( x, y, z, (byte)Block.Stone );
			} else if( info.Collide[block] == CollideType.WalkThrough ) {
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
					HandleWater( posIndex, x, y, z );
				}
			}
		}
		
		void HandleWater( int posIndex, int x, int y, int z ) {
			if( x > 0 ) PropagateWater( posIndex - 1, x - 1, y, z );
			if( x < width - 1 ) PropagateWater( posIndex + 1, x + 1, y, z );
			if( z > 0 ) PropagateWater( posIndex - width, x, y, z - 1 );
			if( z < length - 1 ) PropagateWater( posIndex + width, x, y, z + 1 );
			if( y > 0 ) PropagateWater( posIndex - oneY, x, y - 1, z );
		}
		
		void PropagateWater( int posIndex, int x, int y, int z ) {
			byte block = map.mapData[posIndex];
			if( block == (byte)Block.Lava || block == (byte)Block.StillLava ) {
				game.UpdateBlock( x, y, z, (byte)Block.Stone );
			} else if( info.Collide[block] == CollideType.WalkThrough && block != (byte)Block.Rope ) {
				if( CheckIfSponge( x, y, z ) ) return;
				Water.Enqueue( defWaterTick | (uint)posIndex );
				game.UpdateBlock( x, y, z, (byte)Block.Water );
			}
		}
		
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
					if( y > 0 ) PropagateFalling( posIndex, x, y, z, block, flags );
				}
			}
		}
		
		void PropagateFalling( int posIndex, int x, int y, int z, byte block, int flags ) {
			byte newBlock = map.mapData[posIndex - oneY];
			if( newBlock == 0 || info.IsLiquid[newBlock] ) {
				uint newFlags = MakeFallingFlags( newBlock ) << tickShift;
				Falling.Enqueue( newFlags | (uint)(posIndex - oneY) );
				
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
		
		#endregion
		
		#region TNT
		
		Vector3[] rayDirs;
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
					intensity -= (hardness[block] / 5 + 0.3f) * stepLen;
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
		
		// Algorithm source: Looking at the trees generated by the default classic server.
		// Hence, the random thresholds may be slightly off.
		void GrowTree( int x, int y, int z ) {
			int trunkH = rnd.Next( 1, 4 );
			game.UpdateBlock( x, y, z, 0 );
			
			// Can the new tree grow?
			if( !CheckBounds( x, x, y, y + trunkH - 1, z, z ) ||
			   !CheckBounds( x - 2, x + 2, y + trunkH, y + trunkH + 1, z - 2, z + 2 ) ||
			   !CheckBounds( x - 1, x + 1, y + trunkH + 2, y + trunkH + 3, z - 1, z + 1 ) ) {
				game.UpdateBlock( x, y, z, 0 );
				return;
			}
			
			// Leaves bottom layer
			y += trunkH;
			for( int zz = -2; zz <= 2; zz++ ) {
				for( int xx = -2; xx <= 2; xx++ ) {
					if( Math.Abs( xx ) == 2 && Math.Abs( zz ) == 2 ) {
						if( rnd.Next( 0, 5 ) < 4 ) game.UpdateBlock( x + xx, y, z + zz, (byte)Block.Leaves );
						if( rnd.Next( 0, 5 ) < 2 ) game.UpdateBlock( x + xx, y + 1, z + zz, (byte)Block.Leaves );
					} else {
						game.UpdateBlock( x + xx, y, z + zz, (byte)Block.Leaves );
						game.UpdateBlock( x + xx, y + 1, z + zz, (byte)Block.Leaves );
					}
				}
			}
			
			// Leaves top layer
			y += 2;
			for( int zz = -1; zz <= 1; zz++ ) {
				for( int xx = -1; xx <= 1; xx++ ) {
					if( xx == 0 || zz == 0 ) {
						game.UpdateBlock( x + xx, y, z + zz, (byte)Block.Leaves );
						game.UpdateBlock( x + xx, y + 1, z + zz, (byte)Block.Leaves );
					} else if( rnd.Next( 0, 5 ) == 0 ) {
						game.UpdateBlock( x + xx, y, z + zz, (byte)Block.Leaves );
					}
				}
			}
			
			// Base trunk
			y -= 2 + trunkH;
			for( int yy = 0; yy < trunkH + 3; yy++ )
				game.UpdateBlock( x, y + yy, z, (byte)Block.Log );
		}
		
		bool CheckBounds( int x1, int x2, int y1, int y2, int z1, int z2 ) {
			for( int y = y1; y <= y2; y++ ) {
				for( int z = z1; z <= z2; z++ ) {
					for( int x = x1; x <= x2; x++ ) {
						if( !map.IsValidPos( x, y, z ) ) return false;
						
						byte block = map.GetBlock( x, y, z );
						if( !(block == 0 || block == (byte)Block.Leaves ) ) return false;
					}
				}
			}
			return true;
		}
		
		#endregion
		
		#region Sponge
		
		void PlaceSponge( int x, int y, int z ) {
			for( int yy = y - 2; yy <= y + 2; yy++ )
				for( int zz = z - 2; zz <= z + 2; zz++ )
					for( int xx = x - 2; xx <= x + 2; xx++ )
			{
				byte block = map.SafeGetBlock( xx, yy, zz );
				if( block == (byte)Block.Water || block == (byte)Block.StillWater )
					map.SetBlock( xx, yy, zz, (byte)Block.Air );
			}
		}
		
		bool CheckIfSponge( int x, int y, int z ) {
			for( int yy = (y < 2 ? 0 : y - 2); yy <= (y > maxWaterY ? maxY : y + 2); yy++ )
				for( int zz = (z < 2 ? 0 : z - 2); zz <= (z > maxWaterZ ? maxZ : z + 2); zz++ )
					for( int xx = (x < 2 ? 0 : x - 2); xx <= (x > maxWaterX ? maxX : x + 2); xx++ )
			{
				byte block = map.mapData[(yy * length + zz) * width + xx];
				if( block == (byte)Block.Sponge ) return true;
			}
			
			return false;
		}
		#endregion
	}
}