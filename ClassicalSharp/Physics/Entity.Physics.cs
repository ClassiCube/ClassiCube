using System;
using System.Collections.Generic;
using OpenTK;

namespace ClassicalSharp {
	
	public partial class Entity {
		
		protected bool onGround;
		protected Map map;
		protected BlockInfo info;
		
		protected byte GetPhysicsBlockId( int x, int y, int z ) {
			if( y >= map.Height ) return (byte)Block.Air;
			if( !map.IsValidPos( x, y, z ) ) return (byte)Block.Bedrock;
			return map.GetBlock( x, y, z );
		}
		
		bool GetBoundingBox( byte block, int x, int y, int z, out BoundingBox box ) {
			box = new BoundingBox( Vector3.Zero, Vector3.Zero );
			if( CanWalkThrough( block ) ) return false;
			Vector3 min = new Vector3( x, y, z );
			Vector3 max = new Vector3( x + 1, y + info.BlockHeight( block ), z + 1 );
			box = new BoundingBox( min, max );
			return true;
		}
		
		struct State {
			public BoundingBox BlockBB;
			public byte Block;
			public float tSquared;
			
			public State( BoundingBox bb, byte block, float tSquared ) {
				BlockBB = bb;
				Block = block;
				this.tSquared = tSquared;
			}
		}
		
		bool CanWalkThrough( byte block ) {
			return block == 0 || info.IsSprite( block ) || info.IsLiquid( block ) || block == (byte)Block.Snow;
		}
		
		bool IsFreeYForStep( Vector3 pos ) {
			// NOTE: if non whole blocks are added, use a proper AABB test.
			int x = Utils.Floor( pos.X );
			int y = Utils.Floor( pos.Y );
			int z = Utils.Floor( pos.Z );
			return CanWalkThrough( GetPhysicsBlockId( x, y + 1, z ) ) &&
				CanWalkThrough( GetPhysicsBlockId( x, y + 2, z ) );
		}
		
		// TODO: test for corner cases, and refactor this.
		static State[] stateCache = new State[0];
		class StateComparer : IComparer<State> {
			public int Compare( State x, State y ) {
				return x.tSquared.CompareTo( y.tSquared );
			}
		}
		static StateComparer comparer = new StateComparer();
		protected void MoveAndWallSlide() {
			if( Velocity == Vector3.Zero ) 
				return;		
			Vector3 size = CollisionSize;
			BoundingBox entityBB, entityExtentBB;
			int count = 0;
			FindReachableBlocks( ref count, ref size, out entityBB, out entityExtentBB );
			CollideWithReachableBlocks( count, ref size, ref entityBB, ref entityExtentBB );
		}
		
		void FindReachableBlocks( ref int count, ref Vector3 size, 
		                         out BoundingBox entityBB, out BoundingBox entityExtentBB ) {
			Vector3 vel = Velocity;
			Vector3 pos = Position;
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
			
			int elements = ( max.X + 1 - min.X ) * ( max.Y + 1 - min.Y ) * ( max.Z + 1 - min.Z );
			if( elements > stateCache.Length ) {
				stateCache = new State[elements];
			}
			
			for( int x = min.X; x <= max.X; x++ ) {
				for( int y = min.Y; y <= max.Y; y++ ) {
					for( int z = min.Z; z <= max.Z; z++ ) {
						byte blockId = GetPhysicsBlockId( x, y, z );
						BoundingBox blockBB;
						if( !GetBoundingBox( blockId, x, y, z, out blockBB ) ) continue;
						if( !entityExtentBB.Intersects( blockBB ) ) continue; // necessary for some non whole blocks. (slabs)
						
						float tx = 0, ty = 0, tz = 0;
						CalcTime( ref vel, ref entityBB, ref blockBB, out tx, out ty, out tz );
						if( tx > 1 || ty > 1 || tz > 1 ) continue;
						float tSquared = tx * tx + ty * ty + tz * tz;
						stateCache[count++] = new State( blockBB, blockId, tSquared );
					}
				}
			}
		}
		
		void CollideWithReachableBlocks( int count,  ref Vector3 size, 
		                                ref BoundingBox entityBB, ref BoundingBox entityExtentBB ) {
			bool wasOn = onGround;
			onGround = false;
			Array.Sort( stateCache, 0, count, comparer );

			for( int i = 0; i < count; i++ ) {
				State state = stateCache[i];
				BoundingBox blockBB = state.BlockBB;
				if( !entityExtentBB.Intersects( blockBB ) ) continue;
				
				float tx = 0, ty = 0, tz = 0;
				CalcTime( ref Velocity, ref entityBB, ref blockBB, out tx, out ty, out tz );
				if( tx > 1 || ty > 1 || tz > 1 )
					Utils.LogWarning( "t > 1 in physics calculation.. this shouldn't have happened." );
				BoundingBox finalBB = entityBB.Offset( Velocity * new Vector3( tx, ty, tz ) );
				
				// Find which axis we collide with.
				if( finalBB.Min.Y >= blockBB.Max.Y ) {
					Position.Y = blockBB.Max.Y + 0.001f;
					onGround = true;
					ClipY( ref size, ref entityBB, ref entityExtentBB );
				} else if( finalBB.Max.Y <= blockBB.Min.Y ) {
					Position.Y = blockBB.Min.Y - size.Y - 0.001f;
					ClipY( ref size, ref entityBB, ref entityExtentBB );
				} else {
					if( finalBB.Min.X >= blockBB.Max.X ) {
						if( !DidSlide( ref blockBB, ref size, wasOn, 1, 0, ref entityBB, ref entityExtentBB ) ) {
							Position.X = blockBB.Max.X + size.X / 2 + 0.001f;
							ClipX( ref size, ref entityBB, ref entityExtentBB );
						}
					} else if( finalBB.Max.X <= blockBB.Min.X ) {
						if( !DidSlide( ref blockBB, ref size, wasOn, -1, 0, ref entityBB, ref entityExtentBB ) ) {
							Position.X = blockBB.Min.X - size.X / 2 - 0.001f;
							ClipX( ref size, ref entityBB, ref entityExtentBB );
						}
					} else if( finalBB.Min.Z >= blockBB.Max.Z ) {
						if( !DidSlide( ref blockBB, ref size, wasOn, 0, 1, ref entityBB, ref entityExtentBB ) ) {
							Position.Z = blockBB.Max.Z + size.Z / 2 + 0.001f;
							ClipZ( ref size, ref entityBB, ref entityExtentBB );
						}
					} else if( finalBB.Max.Z <= blockBB.Min.Z ) {
						if( !DidSlide( ref blockBB, ref size, wasOn, 0, -1, ref entityBB, ref entityExtentBB ) ) {
							Position.Z = blockBB.Min.Z - size.Z / 2 - 0.001f;
							ClipZ( ref size, ref entityBB, ref entityExtentBB );
						}
					}
				}
			}
		}
		
		bool DidSlide( ref BoundingBox blockBB, ref Vector3 size, bool wasOnGround, float x, float z,
		              ref BoundingBox entityBB, ref BoundingBox entityExtentBB ) {
			float yDist = blockBB.Max.Y - entityBB.Min.Y;
			Vector3 blockOffsetMin = blockBB.Min + new Vector3( x, 0, z );
			if( yDist > 0 && yDist <= StepSize + 0.01f && wasOnGround &&
			   IsFreeYForStep( blockBB.Min ) && IsFreeYForStep(	blockOffsetMin ) ) {
				Position.Y = blockBB.Max.Y + 0.001f;
				onGround = true;
				ClipY( ref size, ref entityBB, ref entityExtentBB );
				return true;
			}
			return false;
		}
		
		void ClipX( ref Vector3 size, ref BoundingBox entityBB, ref BoundingBox entityExtentBB ) {
			Velocity.X = 0;
			entityBB.Min.X = entityExtentBB.Min.X = Position.X - size.X / 2;
			entityBB.Max.X = entityExtentBB.Max.X = Position.X + size.X / 2;
		}
		
		void ClipY( ref Vector3 size, ref BoundingBox entityBB, ref BoundingBox entityExtentBB ) {
			Velocity.Y = 0;
			entityBB.Min.Y = entityExtentBB.Min.Y = Position.Y;
			entityBB.Max.Y = entityExtentBB.Max.Y = Position.Y + size.Y;
		}		
		
		void ClipZ( ref Vector3 size, ref BoundingBox entityBB, ref BoundingBox entityExtentBB ) {
			Velocity.Z = 0;
			entityBB.Min.Z = entityExtentBB.Min.Z = Position.Z - size.Z / 2;
			entityBB.Max.Z = entityExtentBB.Max.Z = Position.Z + size.Z / 2;
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
	}
}