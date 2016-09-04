// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs collision detection. </summary>
	public sealed class CollisionsComponent {
		
		Game game;
		Entity entity;
		BlockInfo info;
		public CollisionsComponent( Game game, Entity entity ) {
			this.game = game;
			this.entity = entity;
			info = game.BlockInfo;
		}
		
		internal bool hitXMin, hitYMin, hitZMin;
		internal bool hitXMax, hitYMax, hitZMax;
		internal bool HorCollision { 
			get { return hitXMin || hitXMax || hitZMin || hitZMax; } 
		}
		
		/// <summary> Constant offset used to avoid floating point roundoff errors. </summary>
		public const float Adjustment = 0.001f;
		
		public byte GetPhysicsBlockId( int x, int y, int z ) {
			if( x < 0 || x >= game.World.Width || z < 0 ||
			   z >= game.World.Length || y < 0 ) return Block.Bedrock;
			
			if( y >= game.World.Height ) return Block.Air;
			return game.World.GetBlock( x, y, z );
		}		
		
		// TODO: test for corner cases, and refactor this.	
		internal void MoveAndWallSlide() {
			if( entity.Velocity == Vector3.Zero ) return;
			Vector3 size = entity.Size;
			AABB entityBB, entityExtentBB;
			int count = 0;
			FindReachableBlocks( ref count, out entityBB, out entityExtentBB );
			CollideWithReachableBlocks( count, ref size, ref entityBB, ref entityExtentBB );
		}
		
		void FindReachableBlocks( ref int count, out AABB entityBB, 
		                         out AABB entityExtentBB ) {
			Vector3 vel = entity.Velocity;
			entityBB = entity.Bounds;
			
			// Exact maximum extent the entity can reach, and the equivalent map coordinates.
			entityExtentBB = new AABB(
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
			
			AABB blockBB = default( AABB );
			// Order loops so that we minimise cache misses
			for( int y = min.Y; y <= max.Y; y++ )
				for( int z = min.Z; z <= max.Z; z++ )
					for( int x = min.X; x <= max.X; x++ )
			{
				byte block = GetPhysicsBlockId( x, y, z );
				if( info.Collide[block] != CollideType.Solid ) continue;
				
				blockBB.Min = info.MinBB[block];
				blockBB.Min.X += x; blockBB.Min.Y += y; blockBB.Min.Z += z;
				blockBB.Max = info.MaxBB[block];
				blockBB.Max.X += x; blockBB.Max.Y += y; blockBB.Max.Z += z;
				
				if( !entityExtentBB.Intersects( blockBB ) ) continue; // necessary for non whole blocks. (slabs)
				
				float tx = 0, ty = 0, tz = 0;
				CalcTime( ref vel, ref entityBB, ref blockBB, out tx, out ty, out tz );
				if( tx > 1 || ty > 1 || tz > 1 ) continue;
				float tSquared = tx * tx + ty * ty + tz * tz;
				stateCache[count++] = new State( x, y, z, block, tSquared );
			}
		}
		
		void CollideWithReachableBlocks( int count, ref Vector3 size,
		                                ref AABB entityBB, ref AABB entityExtentBB ) {
			bool wasOn = entity.onGround;
			entity.onGround = false;
			if( count > 0 )
				QuickSort( stateCache, 0, count - 1 );
			hitXMin = false; hitYMin = false; hitZMin = false;
			hitXMax = false; hitYMax = false; hitZMax = false;
			AABB blockBB = default(AABB);

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
				
				Vector3 v = entity.Velocity;
				v.X *= tx; v.Y *= ty; v.Z *= tz;
				AABB finalBB = entityBB.Offset( v );
				
				// if we have hit the bottom of a block, we need to change the axis we test first.
				if( !hitYMin ) {
					if( finalBB.Min.Y + Adjustment >= blockBB.Max.Y ) {
						ClipYMax( ref blockBB, ref entityBB, ref entityExtentBB, ref size );
					} else if( finalBB.Max.Y - Adjustment <= blockBB.Min.Y ) {
						ClipYMin( ref blockBB, ref entityBB, ref entityExtentBB, ref size );
					} else if( finalBB.Min.X + Adjustment >= blockBB.Max.X ) {
						ClipXMax( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
					} else if( finalBB.Max.X - Adjustment <= blockBB.Min.X ) {
						ClipXMin( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
					} else if( finalBB.Min.Z + Adjustment >= blockBB.Max.Z ) {
						ClipZMax( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
					} else if( finalBB.Max.Z - Adjustment <= blockBB.Min.Z ) {
						ClipZMin( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
					}
					continue;
				}
				
				// if flying or falling, test the horizontal axes first.
				if( finalBB.Min.X + Adjustment >= blockBB.Max.X ) {
					ClipXMax( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
				} else if( finalBB.Max.X - Adjustment <= blockBB.Min.X ) {
					ClipXMin( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
				} else if( finalBB.Min.Z + Adjustment >= blockBB.Max.Z ) {
					ClipZMax( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
				} else if( finalBB.Max.Z - Adjustment <= blockBB.Min.Z ) {
					ClipZMin( ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size );
				} else if( finalBB.Min.Y + Adjustment >= blockBB.Max.Y ) {
					ClipYMax( ref blockBB, ref entityBB, ref entityExtentBB, ref size );
				} else if( finalBB.Max.Y - Adjustment <= blockBB.Min.Y ) {
					ClipYMin( ref blockBB, ref entityBB, ref entityExtentBB, ref size );
				}
			}
		}
		
		void ClipXMin( ref AABB blockBB, ref AABB entityBB, bool wasOn,
		              AABB finalBB, ref AABB entityExtentBB, ref Vector3 size ) {
			if( !wasOn || !DidSlide( blockBB, ref size, finalBB, ref entityBB, ref entityExtentBB ) ) {
				entity.Position.X = blockBB.Min.X - size.X / 2 - Adjustment;
				ClipX( ref size, ref entityBB, ref entityExtentBB );
				hitXMin = true;
			}
		}
		
		void ClipXMax( ref AABB blockBB, ref AABB entityBB, bool wasOn,
		              AABB finalBB, ref AABB entityExtentBB, ref Vector3 size ) {
			if( !wasOn || !DidSlide( blockBB, ref size, finalBB, ref entityBB, ref entityExtentBB ) ) {
				entity.Position.X = blockBB.Max.X + size.X / 2 + Adjustment;
				ClipX( ref size, ref entityBB, ref entityExtentBB );
				hitXMax = true;
			}
		}
		
		void ClipZMax( ref AABB blockBB, ref AABB entityBB, bool wasOn,
		              AABB finalBB, ref AABB entityExtentBB, ref Vector3 size ) {
			if( !wasOn || !DidSlide( blockBB, ref size, finalBB, ref entityBB, ref entityExtentBB ) ) {
				entity.Position.Z = blockBB.Max.Z + size.Z / 2 + Adjustment;
				ClipZ( ref size, ref entityBB, ref entityExtentBB );
				hitZMax = true;
			}
		}
		
		void ClipZMin( ref AABB blockBB, ref AABB entityBB, bool wasOn,
		              AABB finalBB, ref AABB extentBB, ref Vector3 size ) {
			if( !wasOn || !DidSlide( blockBB, ref size, finalBB, ref entityBB, ref extentBB ) ) {
				entity.Position.Z = blockBB.Min.Z - size.Z / 2 - Adjustment;
				ClipZ( ref size, ref entityBB, ref extentBB );
				hitZMin = true;
			}
		}
		
		void ClipYMin( ref AABB blockBB, ref AABB entityBB,
		              ref AABB extentBB, ref Vector3 size ) {
			entity.Position.Y = blockBB.Min.Y - size.Y - Adjustment;
			ClipY( ref size, ref entityBB, ref extentBB );
			hitYMin = true;
		}
		
		void ClipYMax( ref AABB blockBB, ref AABB entityBB,
		              ref AABB extentBB, ref Vector3 size ) {
			entity.Position.Y = blockBB.Max.Y + Adjustment;
			entity.onGround = true;
			ClipY( ref size, ref entityBB, ref extentBB );
			hitYMax = true;
		}
		
		bool DidSlide( AABB blockBB, ref Vector3 size, AABB finalBB,
		              ref AABB entityBB, ref AABB entityExtentBB ) {
			float yDist = blockBB.Max.Y - entityBB.Min.Y;
			if( yDist > 0 && yDist <= entity.StepSize + 0.01f ) {
				float blockXMin = blockBB.Min.X, blockZMin = blockBB.Min.Z;
				blockBB.Min.X = Math.Max( blockBB.Min.X, blockBB.Max.X - size.X / 2 );
				blockBB.Max.X = Math.Min( blockBB.Max.X, blockXMin + size.X / 2 );
				blockBB.Min.Z = Math.Max( blockBB.Min.Z, blockBB.Max.Z - size.Z / 2 );
				blockBB.Max.Z = Math.Min( blockBB.Max.Z, blockZMin + size.Z / 2 );
				
				AABB adjBB = finalBB;
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
		
		bool CanSlideThrough( ref AABB adjFinalBB ) {
			Vector3I bbMin = Vector3I.Floor( adjFinalBB.Min );
			Vector3I bbMax = Vector3I.Floor( adjFinalBB.Max );
			
			for( int y = bbMin.Y; y <= bbMax.Y; y++ )
				for( int z = bbMin.Z; z <= bbMax.Z; z++ )
					for( int x = bbMin.X; x <= bbMax.X; x++ )
			{
				byte block = GetPhysicsBlockId( x, y, z );
				Vector3 min = new Vector3( x, y, z ) + info.MinBB[block];
				Vector3 max = new Vector3( x, y, z ) + info.MaxBB[block];
				
				AABB blockBB = new AABB( min, max );
				if( !blockBB.Intersects( adjFinalBB ) )
					continue;
				if( info.Collide[GetPhysicsBlockId( x, y, z )] == CollideType.Solid )
					return false;
			}
			return true;
		}
		
		void ClipX( ref Vector3 size, ref AABB entityBB, ref AABB entityExtentBB ) {
			entity.Velocity.X = 0;
			entityBB.Min.X = entityExtentBB.Min.X = entity.Position.X - size.X / 2;
			entityBB.Max.X = entityExtentBB.Max.X = entity.Position.X + size.X / 2;
		}
		
		void ClipY( ref Vector3 size, ref AABB entityBB, ref AABB entityExtentBB ) {
			entity.Velocity.Y = 0;
			entityBB.Min.Y = entityExtentBB.Min.Y = entity.Position.Y;
			entityBB.Max.Y = entityExtentBB.Max.Y = entity.Position.Y + size.Y;
		}
		
		void ClipZ( ref Vector3 size, ref AABB entityBB, ref AABB entityExtentBB ) {
			entity.Velocity.Z = 0;
			entityBB.Min.Z = entityExtentBB.Min.Z = entity.Position.Z - size.Z / 2;
			entityBB.Max.Z = entityExtentBB.Max.Z = entity.Position.Z + size.Z / 2;
		}
		
		static void CalcTime( ref Vector3 vel, ref AABB entityBB, ref AABB blockBB,
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