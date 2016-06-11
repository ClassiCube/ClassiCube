// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using ClassicalSharp.Map;

namespace ClassicalSharp.Singleplayer {

	public class LiquidPhysics {
		
		Game game;
		World map;
		Random rnd = new Random();
		BlockInfo info;
		int width, length, height, oneY;
		int maxX, maxY, maxZ, maxWaterX, maxWaterY, maxWaterZ;

		public LiquidPhysics( Game game, Physics physics ) {
			this.game = game;
			map = game.World;
			info = game.BlockInfo;
			
			physics.OnPlace[Block.Lava] =
				(index, b) => Lava.Enqueue( defLavaTick | (uint)index );
			physics.OnPlace[Block.Water] =
				(index, b) => Water.Enqueue( defWaterTick | (uint)index );
			physics.OnPlace[Block.Sponge] = PlaceSponge;
			physics.OnDelete[Block.Sponge] = DeleteSponge;
			
			physics.OnActivate[Block.Water] = ActivateWater;
			physics.OnActivate[Block.StillWater] = ActivateWater;
			physics.OnActivate[Block.Lava] = ActivateLava;
			physics.OnActivate[Block.StillLava] = ActivateLava;
		}
		
		public void Clear() { Lava.Clear(); Water.Clear(); }
		
		public void ResetMap() {
			Clear();
			width = map.Width; maxX = width - 1; maxWaterX = maxX - 2;
			height = map.Height; maxY = height - 1; maxWaterY = maxY - 2;
			length = map.Length; maxZ = length - 1; maxWaterZ = maxZ - 2;
			oneY = width * length;
		}

		
		Queue<uint> Lava = new Queue<uint>();
		const uint defLavaTick = 30u << Physics.tickShift;
		
		public void TickLava() {
			int count = Lava.Count;
			for( int i = 0; i < count; i++ ) {
				int index;
				if( Physics.CheckItem( Lava, out index ) ) {
					byte block = map.blocks[index];
					if( !(block == Block.Lava || block == Block.StillLava) ) continue;
					ActivateLava( index, block );
				}
			}
		}
		
		void ActivateLava( int index, byte block ) {
			int x = index % width;
			int y = index / oneY; // posIndex / (width * length)
			int z = (index / width) % length;
			
			if( x > 0 ) PropagateLava( index - 1, x - 1, y, z );
			if( x < width - 1 ) PropagateLava( index + 1, x + 1, y, z );
			if( z > 0 ) PropagateLava( index - width, x, y, z - 1 );
			if( z < length - 1 ) PropagateLava( index + width, x, y, z + 1 );
			if( y > 0 ) PropagateLava( index - oneY, x, y - 1, z );
		}
		
		void PropagateLava( int posIndex, int x, int y, int z ) {
			byte block = map.blocks[posIndex];
			if( block == Block.Water || block == Block.StillWater ) {
				game.UpdateBlock( x, y, z, Block.Stone );
			} else if( info.Collide[block] == CollideType.WalkThrough ) {
				Lava.Enqueue( defLavaTick | (uint)posIndex );
				game.UpdateBlock( x, y, z, Block.Lava );
			}
		}
		
		Queue<uint> Water = new Queue<uint>();
		const uint defWaterTick = 5u << Physics.tickShift;
		
		public void TickWater() {
			int count = Water.Count;
			for( int i = 0; i < count; i++ ) {
				int index;
				if( Physics.CheckItem( Water, out index ) ) {
					byte block = map.blocks[index];
					if( !(block == Block.Water || block == Block.StillWater) ) continue;
					ActivateWater( index, block );
				}
			}
		}
		
		void ActivateWater( int index, byte block ) {
			int x = index % width;
			int y = index / oneY; // posIndex / (width * length)
			int z = (index / width) % length;
			
			if( x > 0 ) PropagateWater( index - 1, x - 1, y, z );
			if( x < width - 1 ) PropagateWater( index + 1, x + 1, y, z );
			if( z > 0 ) PropagateWater( index - width, x, y, z - 1 );
			if( z < length - 1 ) PropagateWater( index + width, x, y, z + 1 );
			if( y > 0 ) PropagateWater( index - oneY, x, y - 1, z );
		}
		
		void PropagateWater( int posIndex, int x, int y, int z ) {
			byte block = map.blocks[posIndex];
			if( block == Block.Lava || block == Block.StillLava ) {
				game.UpdateBlock( x, y, z, Block.Stone );
			} else if( info.Collide[block] == CollideType.WalkThrough && block != Block.Rope ) {
				// Sponge check
				for( int yy = (y < 2 ? 0 : y - 2); yy <= (y > maxWaterY ? maxY : y + 2); yy++ )
					for( int zz = (z < 2 ? 0 : z - 2); zz <= (z > maxWaterZ ? maxZ : z + 2); zz++ )
						for( int xx = (x < 2 ? 0 : x - 2); xx <= (x > maxWaterX ? maxX : x + 2); xx++ )
				{
					block = map.blocks[(yy * length + zz) * width + xx];
					if( block == Block.Sponge ) return;
				}
				
				Water.Enqueue( defWaterTick | (uint)posIndex );
				game.UpdateBlock( x, y, z, Block.Water );
			}
		}

		
		void PlaceSponge( int index, byte block ) {
			int x = index % width;
			int y = index / oneY; // posIndex / (width * length)
			int z = (index / width) % length;
			
			for( int yy = y - 2; yy <= y + 2; yy++ )
				for( int zz = z - 2; zz <= z + 2; zz++ )
					for( int xx = x - 2; xx <= x + 2; xx++ )
			{
				block = map.SafeGetBlock( xx, yy, zz );
				if( block == Block.Water || block == Block.StillWater )
					game.UpdateBlock( xx, yy, zz, Block.Air );
			}
		}
		
		
		void DeleteSponge( int index, byte block ) {
			int x = index % width;
			int y = index / oneY; // posIndex / (width * length)
			int z = (index / width) % length;
			
			for( int yy = y - 3; yy <= y + 3; yy++ )
				for( int zz = z - 3; zz <= z + 3; zz++ )
					for( int xx = x - 3; xx <= x + 3; xx++ )
			{
				if( Math.Abs( yy - y ) == 3 || Math.Abs( zz - z ) == 3 || Math.Abs( xx - x ) == 3 ) {
					if( !map.IsValidPos( x, y, z ) ) continue;
					
					index = xx + width * (zz + yy * length);
					block = map.blocks[index];
					if( block == Block.Water || block == Block.StillWater )
						Water.Enqueue( (1u << Physics.tickShift) | (uint)index );
				}
			}
		}
	}
}