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
		LiquidPhysics liquid;
		
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
			liquid = new LiquidPhysics( game );
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
			liquid.TickLava();
			liquid.TickWater();
			falling.Tick();
			//}
			tickCount++;
			TickRandomBlocks();
		}
		
		public void OnBlockPlaced( int x, int y, int z, byte block ) {
			if( !Enabled ) return;
			
			int index = (y * length + z) * width + x;
			if( block == (byte)Block.Lava ) {
				liquid.PlaceLava( index );
			} else if( block == (byte)Block.Water ) {
				liquid.PlaceWater( index );
			} else if( block == (byte)Block.Sand || block == (byte)Block.Gravel ) {
				falling.OnBlockPlace( index, block );
			} else if( block == (byte)Block.TNT ) {
				tnt.Explode( 4, x, y, z );
			} else if( block == (byte)Block.Sapling ) {
				foilage.GrowTree( x, y, z );
			} else if( block == (byte)Block.Sponge ) {
				liquid.PlaceSponge( index );
			}
		}
		
		void ResetMap( object sender, EventArgs e ) {
			falling.ResetMap();
			liquid.ResetMap();
			width = map.Width; maxX = width - 1; maxWaterX = maxX - 2;
			height = map.Height; maxY = height - 1; maxWaterY = maxY - 2;
			length = map.Length; maxZ = length - 1; maxWaterZ = maxZ - 2;			
			oneY = width * length;
		}
		
		void ClearQueuedEvents() {
			liquid.Clear();
			falling.Clear();
		}
		
		public void Dispose() {
			game.WorldEvents.OnNewMapLoaded -= ResetMap;
		}

		
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
					if( y <= map.GetLightHeight( x, z ) )
						game.UpdateBlock( x, y, z, (byte)Block.Dirt );
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
					liquid.HandleLava( posIndex );
					break;
					
				case (byte)Block.Water:
					liquid.HandleWater( posIndex );
					break;
			}
		}
	}
}