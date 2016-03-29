// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs collision detection. </summary>
	public sealed class PhysicsComponent {
		
		Game game;
		Entity entity;
		BlockInfo info;
		public PhysicsComponent( Game game, Entity entity ) {
			this.game = game;
			this.entity = entity;
			info = game.BlockInfo;
		}
		
		internal bool hitYMax, collideX, collideY, collideZ;
		
		/// <summary> Constant offset used to avoid floating point roundoff errors. </summary>
		public const float Adjustment = 0.001f;
		
		public byte GetPhysicsBlockId( int x, int y, int z ) {
			if( x < 0 || x >= game.World.Width || z < 0 ||
			   z >= game.World.Length || y < 0 ) return (byte)Block.Bedrock;
			
			if( y >= game.World.Height ) return (byte)Block.Air;
			return game.World.GetBlock( x, y, z );
		}
		
		bool GetBoundingBox( byte block, int x, int y, int z, ref BoundingBox box ) {
			if( info.Collide[block] != CollideType.Solid ) return false;
			Add( x, y, z, ref info.MinBB[block], ref box.Min );
			Add( x, y, z, ref info.MaxBB[block], ref box.Max );
			return true;
		}
		
		static void Add( int x, int y, int z, ref Vector3 offset, ref Vector3 target ) {
			target.X = x + offset.X;
			target.Y = y + offset.Y;
			target.Z = z + offset.Z;
		}
		
		
		// TODO: test for corner cases, and refactor this.	
		internal void MoveAndWallSlide() {
			if( entity.Velocity == Vector3.Zero ) return;
			Vector3 size = entity.CollisionSize;
			BoundingBox entityBB, entityExtentBB;
			int count = 0;
			FindReachableBlocks( ref count, ref size, out entityBB, out entityExtentBB );
			CollideWithReachableBlocks( count, ref size, ref entityBB, ref entityExtentBB );
		}
		
		void FindReachableBlocks( ref int count, ref Vector3 size,
		                         out BoundingBox entityBB, out BoundingBox entityExtentBB ) {
			Vector3 vel = entity.Velocity;
			Vector3 pos = entity.Position;
			entityBB = new BoundingBox(
				pos.X - size.X / 2, pos.Y, pos.Z - size.Z / 2,
				pos.X + size.X / 2, pos.Y + size.Y, pos.Z + size.Z / 2
			);
			
			// Exact maximum extent the entity can reach, and the equivalent map coordinates.
			entityExtentBB = new BoundingBox(
				vel.X < 0 ? entityBB.Min.X + vel.X : entityBB.Min.X,
				vel.Y < 0 ? entityBB.Min.Y + vel.Y : entityBB.Min.Y,
				vel.Z < 0 ? entityBB.Min.Z + vel.Z : entityBB.Min.Z,
				vel.X > 0 ? entityBB.Max.X + vel.X : entityBB.Max.X,
				vel.Y > 0 ? entityBB.Max.Y + vel.Y : entityBB.Max.Y,
				vel.Z > 0 ? entityBB.Max.Z + vel.Z : entityBB.Max.Z
			);
			Vector3I min = Vector3I.Floor( entityExtentBB.Min );
			Vector3I max = Vector3I.Floor( entityExtentBB.Max );
			
			int elements = (max.X + 1 - min.X) * (max.Y + 1 - min.Y) * (max.Z + 1 - min.Z);
			if( elements > stateCache.Length ) {
				stateCache = new State[elements];
			}
			
			BoundingBox blockBB = default( BoundingBox );
			// Order loops so that we minimise cache misses
			for( int y = min.Y; y <= max.Y; y++ )
				for( int z = min.Z; z <= max.Z; z++ )
					for( int x = min.X; x <= max.X; x++ )
			{
				byte blockId = GetPhysicsBlockId( x, y, z );
				if( !GetBoundingBox( blockId, x, y, z, ref blockBB ) ) continue;
				if( !entityExtentBB.Intersects( blockBB ) ) continue; // necessary for non whole blocks. (slabs)
				
				float tx = 0, ty = 0, tz = 0;
				CalcTime( ref vel, ref entityBB, ref blockBB, out tx, out ty, out tz );
				if( tx > 1 || ty > 1 || tz > 1 ) continue;
				float tSquared = tx * tx + ty * ty + tz * tz;
				stateCache[count++] = new State( x, y, z, blockId, tSquared );
			}
		}
		
		void CollideWithReachableBlocks( int count, ref Vector3 size,
		                                ref BoundingBox entityBB, ref BoundingBox entityExtentBB ) {
			bool wasOn = entity.onGround;
			entity.onGround = false;
			if( count > 0 )
				QuickSort( stateCache, 0, count - 1 );
			collideX = false; collideY = false; collideZ = false;
			BoundingBox blockBB = default(BoundingBox);

			for( int i = 0; i < count; i++ ) {
				State state = stateCache[i];
				Vector3 blockPos = new Vector3( state.X >> 3, state.Y >> 3, state.Z >> 3 );
				int block = (state.X & 0x7) | (state.Y & 0x7) << 3 | (state.Z & 0x7) << 6;
				blockBB.Min = blockPos + info.MinBB[block];
				blockBB.Max = blockPos + info.MaxBB[block];
				if( !entityExtentBB.Intersects( blockBB ) ) continue;
				
				float tx = 0, ty = 0, tz = 0;
				CalcTime( ref entity.Velocity, ref entityBB, ref blockBB, out tx, out ty, out tz );
				if( tx > 1 || ty > 1 || tz > 1 )
					Utils.LogDebug( "t > 1 in physics calculation.. this shouldn't have happened." );
				BoundingBox finalBB = entityBB.Offset( entity.Velocity * new Vector3( tx, ty, tz ) );
				
				// if we have hit the bottom of a block, we need to change the axis we test first.
				if( hitYMax ) {
					if( finalBB.Min.Y + Adjustment >= blockBB.Max.Y )
						ClipYMax( ref blockBB, ref entityBB, ref entityExtentBB, ref size );
					else if( finalBB.Max.Y - Adjustment <= blockBB.Min.Y )
						ClipYMin( ref blockBB, ref entityBB, ref entityExtentBB, ref size );
					else if( finalBB.Min.X + Adjustment >= blockBB.Max.X )
						ClipXMax( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
					else if( finalBB.Max.X - Adjustment <= blockBB.Min.X )
						ClipXMin( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
					else if( finalBB.Min.Z + Adjustment >= blockBB.Max.Z )
						ClipZMax( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
					else if( finalBB.Max.Z - Adjustment <= blockBB.Min.Z )
						ClipZMin( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
					continue;
				}
				
				// if flying or falling, test the horizontal axes first.
				if( finalBB.Min.X + Adjustment >= blockBB.Max.X )
					ClipXMax( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
				else if( finalBB.Max.X - Adjustment <= blockBB.Min.X )
					ClipXMin( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
				else if( finalBB.Min.Z + Adjustment >= blockBB.Max.Z )
					ClipZMax( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
				else if( finalBB.Max.Z - Adjustment <= blockBB.Min.Z )
					ClipZMin( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
				else if( finalBB.Min.Y + Adjustment >= blockBB.Max.Y )
					ClipYMax( ref blockBB, ref entityBB, ref entityExtentBB, ref size );
				else if( finalBB.Max.Y - Adjustment <= blockBB.Min.Y )
					ClipYMin( ref blockBB, ref entityBB, ref entityExtentBB, ref size );
			}
		}
		
		void ClipXMin( ref BoundingBox blockBB, ref BoundingBox entityBB, bool wasOn,
		              BoundingBox finalBB, ref BoundingBox entityExtentBB, ref Vector3 size ) {
			if( !wasOn || !DidSlide( blockBB, ref size, finalBB, ref entityBB, ref entityExtentBB ) ) {
				entity.Position.X = blockBB.Min.X - size.X / 2 - Adjustment;
				ClipX( ref size, ref entityBB, ref entityExtentBB );
			}
		}
		
		void ClipXMax( ref BoundingBox blockBB, ref BoundingBox entityBB, bool wasOn,
		              BoundingBox finalBB, ref BoundingBox entityExtentBB, ref Vector3 size ) {
			if( !wasOn || !DidSlide( blockBB, ref size, finalBB, ref entityBB, ref entityExtentBB ) ) {
				entity.Position.X = blockBB.Max.X + size.X / 2 + Adjustment;
				ClipX( ref size, ref entityBB, ref entityExtentBB );
			}
		}
		
		void ClipZMax( ref BoundingBox blockBB, ref BoundingBox entityBB, bool wasOn,
		              BoundingBox finalBB, ref BoundingBox entityExtentBB, ref Vector3 size ) {
			if( !wasOn || !DidSlide( blockBB, ref size, finalBB, ref entityBB, ref entityExtentBB ) ) {
				entity.Position.Z = blockBB.Max.Z + size.Z / 2 + Adjustment;
				ClipZ( ref size, ref entityBB, ref entityExtentBB );
			}
		}
		
		void ClipZMin( ref BoundingBox blockBB, ref BoundingBox entityBB, bool wasOn,
		              BoundingBox finalBB, ref BoundingBox extentBB, ref Vector3 size ) {
			if( !wasOn || !DidSlide( blockBB, ref size, finalBB, ref entityBB, ref extentBB ) ) {
				entity.Position.Z = blockBB.Min.Z - size.Z / 2 - Adjustment;
				ClipZ( ref size, ref entityBB, ref extentBB );
			}
		}
		
		void ClipYMin( ref BoundingBox blockBB, ref BoundingBox entityBB,
		              ref BoundingBox extentBB, ref Vector3 size ) {
			entity.Position.Y = blockBB.Min.Y - size.Y - Adjustment;
			ClipY( ref size, ref entityBB, ref extentBB );
			hitYMax = false;
		}
		
		void ClipYMax( ref BoundingBox blockBB, ref BoundingBox entityBB,
		              ref BoundingBox extentBB, ref Vector3 size ) {
			entity.Position.Y = blockBB.Max.Y + Adjustment;
			entity.onGround = true;
			ClipY( ref size, ref entityBB, ref extentBB );
			hitYMax = true;
		}
		
		bool DidSlide( BoundingBox blockBB, ref Vector3 size, BoundingBox finalBB,
		              ref BoundingBox entityBB, ref BoundingBox entityExtentBB ) {
			float yDist = blockBB.Max.Y - entityBB.Min.Y;
			if( yDist > 0 && yDist <= entity.StepSize + 0.01f ) {
				float blockXMin = blockBB.Min.X, blockZMin = blockBB.Min.Z;
				blockBB.Min.X = Math.Max( blockBB.Min.X, blockBB.Max.X - size.X / 2 );
				blockBB.Max.X = Math.Min( blockBB.Max.X, blockXMin + size.X / 2 );
				blockBB.Min.Z = Math.Max( blockBB.Min.Z, blockBB.Max.Z - size.Z / 2 );
				blockBB.Max.Z = Math.Min( blockBB.Max.Z, blockZMin + size.Z / 2 );
				
				BoundingBox adjBB = finalBB;
				adjBB.Min.X = Math.Min( finalBB.Min.X, blockBB.Min.X + Adjustment );
				adjBB.Max.X = Math.Max( finalBB.Max.X, blockBB.Max.X - Adjustment );
				adjBB.Min.Y = blockBB.Max.Y + Adjustment;
				adjBB.Max.Y = adjBB.Min.Y + size.Y;
				adjBB.Min.Z = Math.Min( finalBB.Min.Z, blockBB.Min.Z + Adjustment );
				adjBB.Max.Z = Math.Max( finalBB.Max.Z, blockBB.Max.Z - Adjustment );
				
				if( !CanSlideThrough( ref adjBB ) )
					return false;
				entity.Position.Y = blockBB.Max.Y + Adjustment;
				entity.onGround = true;
				ClipY( ref size, ref entityBB, ref entityExtentBB );
				return true;
			}
			return false;
		}
		
		bool CanSlideThrough( ref BoundingBox adjFinalBB ) {
			Vector3I bbMin = Vector3I.Floor( adjFinalBB.Min );
			Vector3I bbMax = Vector3I.Floor( adjFinalBB.Max );
			
			for( int y = bbMin.Y; y <= bbMax.Y; y++ )
				for( int z = bbMin.Z; z <= bbMax.Z; z++ )
					for( int x = bbMin.X; x <= bbMax.X; x++ )
			{
				byte block = GetPhysicsBlockId( x, y, z );
				Vector3 min = new Vector3( x, y, z ) + info.MinBB[block];
				Vector3 max = new Vector3( x, y, z ) + info.MaxBB[block];
				
				BoundingBox blockBB = new BoundingBox( min, max );
				if( !blockBB.Intersects( adjFinalBB ) )
					continue;
				if( info.Collide[GetPhysicsBlockId( x, y, z )] == CollideType.Solid )
					return false;
			}
			return true;
		}
		
		void ClipX( ref Vector3 size, ref BoundingBox entityBB, ref BoundingBox entityExtentBB ) {
			entity.Velocity.X = 0;
			entityBB.Min.X = entityExtentBB.Min.X = entity.Position.X - size.X / 2;
			entityBB.Max.X = entityExtentBB.Max.X = entity.Position.X + size.X / 2;
			collideX = true;
		}
		
		void ClipY( ref Vector3 size, ref BoundingBox entityBB, ref BoundingBox entityExtentBB ) {
			entity.Velocity.Y = 0;
			entityBB.Min.Y = entityExtentBB.Min.Y = entity.Position.Y;
			entityBB.Max.Y = entityExtentBB.Max.Y = entity.Position.Y + size.Y;
			collideY = true;
		}
		
		void ClipZ( ref Vector3 size, ref BoundingBox entityBB, ref BoundingBox entityExtentBB ) {
			entity.Velocity.Z = 0;
			entityBB.Min.Z = entityExtentBB.Min.Z = entity.Position.Z - size.Z / 2;
			entityBB.Max.Z = entityExtentBB.Max.Z = entity.Position.Z + size.Z / 2;
			collideZ = true;
		}
		
		static void CalcTime( ref Vector3 vel, ref BoundingBox entityBB, ref BoundingBox blockBB,
		                     out float tx, out float ty, out float tz ) {
			float dx = vel.X > 0 ? blockBB.Min.X - entityBB.Max.X : entityBB.Min.X - blockBB.Max.X;
			float dy = vel.Y > 0 ? blockBB.Min.Y - entityBB.Max.Y : entityBB.Min.Y - blockBB.Max.Y;
			float dz = vel.Z > 0 ? blockBB.Min.Z - entityBB.Max.Z : entityBB.Min.Z - blockBB.Max.Z;
			
			tx = vel.X == 0 ? float.PositiveInfinity : Math.Abs( dx / vel.X );
			ty = vel.Y == 0 ? float.PositiveInfinity : Math.Abs( dy / vel.Y );
			tz = vel.Z == 0 ? float.PositiveInfinity : Math.Abs( dz / vel.Z );
			
			if( entityBB.XIntersects( blockBB ) ) tx = 0;
			if( entityBB.YIntersects( blockBB ) ) ty = 0;
			if( entityBB.ZIntersects( blockBB ) ) tz = 0;
		}
		
		
		struct State {
			public int X, Y, Z;
			public float tSquared;
			
			public State( int x, int y, int z, byte block, float tSquared ) {
				X = x << 3; Y = y << 3; Z = z << 3;
				X |= (block & 0x07);
				Y |= (block & 0x38) >> 3;
				Z |= (block & 0xC0) >> 6;
				this.tSquared = tSquared;
			}
		}
		static State[] stateCache = new State[0];
		
		static void QuickSort( State[] keys, int left, int right ) {
			while( left < right ) {
				int i = left, j = right;		
				float pivot = keys[(i + j) / 2].tSquared;
				// partition the list
				while( i <= j ) {
					while( pivot > keys[i].tSquared ) i++;
					while( pivot < keys[j].tSquared ) j--;
					
					if( i <= j ) {
						State key = keys[i]; keys[i] = keys[j]; keys[j] = key;
						i++; j--;
					}					
				}
				
				// recurse into the smaller subset
				if( j - left <= right - i ) {
					if( left < j )
						QuickSort( keys, left, j );
					left = i;
				} else {
					if( i < right )
						QuickSort( keys, i, right );
					right = j;
				}
			}
		}
	}
}