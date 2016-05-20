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
		
		public const uint tickMask = 0xF8000000;
		public const uint posMask =  0x07FFFFFF;
		public const int tickShift = 27;
		FallingPhysics falling;
		TNTPhysics tnt;
		FoilagePhysics foilage;
		
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
			falling = new FallingPhysics( game );
			tnt = new TNTPhysics( game );
			foilage = new FoilagePhysics( game );
		}
		
		internal static bool CheckItem( Queue<uint> queue, out int posIndex ) {
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
		
		internal static bool CheckItem( Queue<uint> queue, int mask, out int posIndex, out int flags ) {
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
			falling.Tick();
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
				falling.OnBlockPlace( index, block );
			} else if( block == (byte)Block.TNT ) {
				tnt.Explode( 4, x, y, z );
			} else if( block == (byte)Block.Sapling ) {
				foilage.GrowTree( x, y, z );
			} else if( block == (byte)Block.Sponge ) {
				PlaceSponge( x, y, z );
			}
		}
		
		void ResetMap( object sender, EventArgs e ) {
			falling.ResetMap();
			width = map.Width; maxX = width - 1; maxWaterX = maxX - 2;
			height = map.Height; maxY = height - 1; maxWaterY = maxY - 2;
			length = map.Length; maxZ = length - 1; maxWaterZ = maxZ - 2;			
			oneY = width * length;
		}
		
		void ClearQueuedEvents() {
			Lava.Clear();
			Water.Clear();
			falling.Clear();
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
			byte block = map.blocks[posIndex];
			
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
					foilage.GrowTree( x, y, z );
					break;
					
				case (byte)Block.Sand:
				case (byte)Block.Gravel:
					falling.OnRandomTick( posIndex, block );
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
					byte block = map.blocks[posIndex];
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
			byte block = map.blocks[posIndex];
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
					byte block = map.blocks[posIndex];
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
			byte block = map.blocks[posIndex];
			if( block == (byte)Block.Lava || block == (byte)Block.StillLava ) {
				game.UpdateBlock( x, y, z, (byte)Block.Stone );
			} else if( info.Collide[block] == CollideType.WalkThrough && block != (byte)Block.Rope ) {
				if( CheckIfSponge( x, y, z ) ) return;
				Water.Enqueue( defWaterTick | (uint)posIndex );
				game.UpdateBlock( x, y, z, (byte)Block.Water );
			}
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
					game.UpdateBlock( xx, yy, zz, (byte)Block.Air );
			}
		}
		
		bool CheckIfSponge( int x, int y, int z ) {
			for( int yy = (y < 2 ? 0 : y - 2); yy <= (y > maxWaterY ? maxY : y + 2); yy++ )
				for( int zz = (z < 2 ? 0 : z - 2); zz <= (z > maxWaterZ ? maxZ : z + 2); zz++ )
					for( int xx = (x < 2 ? 0 : x - 2); xx <= (x > maxWaterX ? maxX : x + 2); xx++ )
			{
				byte block = map.blocks[(yy * length + zz) * width + xx];
				if( block == (byte)Block.Sponge ) return true;
			}
			
			return false;
		}
		#endregion
	}
}