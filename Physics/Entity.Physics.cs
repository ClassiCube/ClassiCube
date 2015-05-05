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
		
		bool IsFreeYForStep( BoundingBox blockBB ) {
			// NOTE: if non whole blocks are added, use a proper AABB test.
			int x = (int)Utils.Floor( blockBB.Min.X );
			int y = (int)Utils.Floor( blockBB.Min.Y );
			int z = (int)Utils.Floor( blockBB.Min.Z );
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
			Vector3 vel = Velocity;
			if( vel == Vector3.Zero ) return;
			Vector3 size = Size;
			Vector3 pos = Position;
			BoundingBox entityBB = new BoundingBox(
				pos.X - size.X / 2, pos.Y, pos.Z - size.Z / 2,
				pos.X + size.X / 2, pos.Y + size.Y, pos.Z + size.Z / 2
			);
			
			// Exact maximum extent the entity can reach.
			BoundingBox entityExtentBB = new BoundingBox(
				vel.X < 0 ? entityBB.Min.X + vel.X : entityBB.Min.X,
				vel.Y < 0 ? entityBB.Min.Y + vel.Y : entityBB.Min.Y,
				vel.Z < 0 ? entityBB.Min.Z + vel.Z : entityBB.Min.Z,
				vel.X > 0 ? entityBB.Max.X + vel.X : entityBB.Max.X,
				vel.Y > 0 ? entityBB.Max.Y + vel.Y : entityBB.Max.Y,
				vel.Z > 0 ? entityBB.Max.Z + vel.Z : entityBB.Max.Z
			);
			// Find the equivalent map coordinates of the maximum extent.
			int minX = Utils.Floor( entityExtentBB.Min.X );
			int minY = Utils.Floor( entityExtentBB.Min.Y );
			int minZ = Utils.Floor( entityExtentBB.Min.Z );
			int maxX = Utils.Floor( entityExtentBB.Max.X );
			int maxY = Utils.Floor( entityExtentBB.Max.Y );
			int maxZ = Utils.Floor( entityExtentBB.Max.Z );
			
			int elements = ( maxX + 1 - minX ) * ( maxY + 1 - minY ) * ( maxZ + 1 - minZ );
			if( elements > stateCache.Length ) {
				stateCache = new State[elements];
			}
			int count = 0;
			
			for( int x = minX; x <= maxX; x++ ) {
				for( int y = minY; y <= maxY; y++ ) {
					for( int z = minZ; z <= maxZ; z++ ) {
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
			
			bool wasOnGround = onGround;
			onGround = false;
			Array.Sort( stateCache, 0, count, comparer );

			for( int i = 0; i < count; i++ ) {
				State state = stateCache[i];
				BoundingBox blockBB = state.BlockBB;
				if( !entityExtentBB.Intersects( blockBB ) ) continue;
				
				float tx = 0, ty = 0, tz = 0;
				CalcTime( ref Velocity, ref entityBB, ref blockBB, out tx, out ty, out tz );
				float tMin = Math.Min( tx, Math.Min( ty, tz ) );
				if( tMin > 1 ) Utils.LogWarning( "tmin > 1 in physics calculation.. this shouldn't have happened." );
				BoundingBox finalBB = entityBB.Offset( Velocity * tMin );
				
				// Find which axis we collide with.
				if( finalBB.Min.Y >= blockBB.Max.Y ) {
					Position.Y = blockBB.Max.Y + 0.001f;
					onGround = true;
					Velocity.Y = 0;
					entityBB.Min.Y = entityExtentBB.Min.Y = Position.Y;
					entityBB.Max.Y = entityExtentBB.Max.Y = Position.Y + Size.Y;
				} else if( finalBB.Max.Y <= blockBB.Min.Y ) {
					Position.Y = blockBB.Min.Y - size.Y - 0.001f;
					Velocity.Y = 0;
					entityBB.Min.Y = entityExtentBB.Min.Y = Position.Y;
					entityBB.Max.Y = entityExtentBB.Max.Y = Position.Y + Size.Y;
				} else {
					float yDist = blockBB.Max.Y - entityBB.Min.Y;
					if( yDist > 0 && yDist <= StepSize + 0.01f && wasOnGround && IsFreeYForStep( blockBB ) ) {
						// Slide up steps.
						Position.Y = blockBB.Max.Y + 0.001f;
						Velocity.Y = 0;
						entityBB.Min.Y = entityExtentBB.Min.Y = Position.Y;
						entityBB.Max.Y = entityExtentBB.Max.Y = Position.Y + Size.Y;
						onGround = true;
					} else if( finalBB.Min.X >= blockBB.Max.X ) {
						Position.X = blockBB.Max.X + size.X / 2 + 0.001f;
						Velocity.X = 0;
						entityBB.Min.X = entityExtentBB.Min.X = pos.X - size.X / 2;
						entityBB.Max.X = entityExtentBB.Max.X = pos.X + size.X / 2;
					} else if( finalBB.Max.X <= blockBB.Min.X ) {
						Position.X = blockBB.Min.X - size.X / 2 - 0.001f;
						Velocity.X = 0;
						entityBB.Min.X = entityExtentBB.Min.X = pos.X - size.X / 2;
						entityBB.Max.X = entityExtentBB.Max.X = pos.X + size.X / 2;
					} else if( finalBB.Min.Z >= blockBB.Max.Z ) {
						Position.Z = blockBB.Max.Z + size.Z / 2 + 0.001f;
						Velocity.Z = 0;
						entityBB.Min.Z = entityExtentBB.Min.Z = Position.Z - size.Z / 2;
						entityBB.Max.Z = entityExtentBB.Max.Z = Position.Z + size.Z / 2;
					} else if( finalBB.Max.Z <= blockBB.Min.Z ) {
						Position.Z = blockBB.Min.Z - size.Z / 2 - 0.001f;
						Velocity.Z = 0;
						entityBB.Min.Z = entityExtentBB.Min.Z = Position.Z - size.Z / 2;
						entityBB.Max.Z = entityExtentBB.Max.Z = Position.Z + size.Z / 2;
					}
				}
			}
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