// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Physics;
using OpenTK;
using BlockID = System.UInt16;

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs collision detection. </summary>
	public sealed class CollisionsComponent {
		
		Game game;
		Entity entity;
		public CollisionsComponent(Game game, Entity entity) {
			this.game = game;
			this.entity = entity;
		}
		
		internal bool hitXMin, hitYMin, hitZMin;
		internal bool hitXMax, hitYMax, hitZMax;
		
		/// <summary> Whether a collision occurred with any horizontal sides of any blocks. </summary>
		internal bool HorizontalCollision { 
			get { return hitXMin || hitXMax || hitZMin || hitZMax; } 
		}
		
		/// <summary> Constant offset used to avoid floating point roundoff errors. </summary>
		public const float Adjustment = 0.001f;	
		
		// TODO: test for corner cases, and refactor this.	
		internal void MoveAndWallSlide() {
			if (entity.Velocity == Vector3.Zero) return;
			
			AABB entityBB, entityExtentBB;			
			int count = Searcher.FindReachableBlocks(game, entity,
			                                         out entityBB, out entityExtentBB);
			CollideWithReachableBlocks(count, ref entityBB, ref entityExtentBB);
		}

		void CollideWithReachableBlocks(int count, ref AABB entityBB, ref AABB extentBB) {
			// Reset collision detection states
			bool wasOn = entity.onGround;
			entity.onGround = false;
			hitXMin = false; hitYMin = false; hitZMin = false;
			hitXMax = false; hitYMax = false; hitZMax = false;
			
			AABB blockBB;
			Vector3 bPos, size = entity.Size;
			for (int i = 0; i < count; i++) {
				// Unpack the block and coordinate data
				State state = Searcher.stateCache[i];
				#if !ONLY_8BIT
				bPos.X = state.X >> 3; bPos.Y = state.Y >> 4; bPos.Z = state.Z >> 3;
				int block = (state.X & 0x7) | (state.Y & 0xF) << 3 | (state.Z & 0x7) << 7;
				#else
				bPos.X = state.X >> 3; bPos.Y = state.Y >> 3; bPos.Z = state.Z >> 3;
				int block = (state.X & 0x7) | (state.Y & 0x7) << 3 | (state.Z & 0x7) << 6;
				#endif
				
				blockBB.Min = BlockInfo.MinBB[block];
				blockBB.Min.X += bPos.X; blockBB.Min.Y += bPos.Y; blockBB.Min.Z += bPos.Z;
				blockBB.Max = BlockInfo.MaxBB[block];
				blockBB.Max.X += bPos.X; blockBB.Max.Y += bPos.Y; blockBB.Max.Z += bPos.Z;
				if (!extentBB.Intersects(blockBB)) continue;
				
				// Recheck time to collide with block (as colliding with blocks modifies this)
				float tx, ty, tz;
				Searcher.CalcTime(ref entity.Velocity, ref entityBB, ref blockBB, out tx, out ty, out tz);
				if (tx > 1 || ty > 1 || tz > 1)
					Utils.LogDebug("t > 1 in physics calculation.. this shouldn't have happened.");
				
				// Calculate the location of the entity when it collides with this block
				Vector3 v = entity.Velocity;
				v.X *= tx; v.Y *= ty; v.Z *= tz;
				AABB finalBB = entityBB;
				finalBB.Min += v; finalBB.Max += v; // Inlined ABBB.Offset
				
				// if we have hit the bottom of a block, we need to change the axis we test first.
				if (!hitYMin) {
					if (finalBB.Min.Y + Adjustment >= blockBB.Max.Y) {
						ClipYMax(ref blockBB, ref entityBB, ref extentBB, ref size);
					} else if (finalBB.Max.Y - Adjustment <= blockBB.Min.Y) {
						ClipYMin(ref blockBB, ref entityBB, ref extentBB, ref size);
					} else if (finalBB.Min.X + Adjustment >= blockBB.Max.X) {
						ClipXMax(ref blockBB, ref entityBB, wasOn, ref finalBB, ref extentBB, ref size);
					} else if (finalBB.Max.X - Adjustment <= blockBB.Min.X) {
						ClipXMin(ref blockBB, ref entityBB, wasOn, ref finalBB, ref extentBB, ref size);
					} else if (finalBB.Min.Z + Adjustment >= blockBB.Max.Z) {
						ClipZMax(ref blockBB, ref entityBB, wasOn, ref finalBB, ref extentBB, ref size);
					} else if (finalBB.Max.Z - Adjustment <= blockBB.Min.Z) {
						ClipZMin(ref blockBB, ref entityBB, wasOn, ref finalBB, ref extentBB, ref size);
					}
					continue;
				}
				
				// if flying or falling, test the horizontal axes first.
				if (finalBB.Min.X + Adjustment >= blockBB.Max.X) {
					ClipXMax(ref blockBB, ref entityBB, wasOn, ref finalBB, ref extentBB, ref size);
				} else if (finalBB.Max.X - Adjustment <= blockBB.Min.X) {
					ClipXMin(ref blockBB, ref entityBB, wasOn, ref finalBB, ref extentBB, ref size);
				} else if (finalBB.Min.Z + Adjustment >= blockBB.Max.Z) {
					ClipZMax(ref blockBB, ref entityBB, wasOn, ref finalBB, ref extentBB, ref size);
				} else if (finalBB.Max.Z - Adjustment <= blockBB.Min.Z) {
					ClipZMin(ref blockBB, ref entityBB, wasOn, ref finalBB, ref extentBB, ref size);
				} else if (finalBB.Min.Y + Adjustment >= blockBB.Max.Y) {
					ClipYMax(ref blockBB, ref entityBB, ref extentBB, ref size);
				} else if (finalBB.Max.Y - Adjustment <= blockBB.Min.Y) {
					ClipYMin(ref blockBB, ref entityBB, ref extentBB, ref size);
				}
			}
		}
		
		void ClipXMin(ref AABB blockBB, ref AABB entityBB, bool wasOn, ref AABB finalBB, ref AABB extentBB, ref Vector3 size) {
			if (!wasOn || !DidSlide(ref blockBB, ref size, ref finalBB, ref entityBB, ref extentBB)) {
				entity.Position.X = blockBB.Min.X - size.X / 2 - Adjustment;
				ClipX(ref size, ref entityBB, ref extentBB);
				hitXMin = true;
			}
		}
		
		void ClipXMax(ref AABB blockBB, ref AABB entityBB, bool wasOn, ref AABB finalBB, ref AABB extentBB, ref Vector3 size) {
			if (!wasOn || !DidSlide(ref blockBB, ref size, ref finalBB, ref entityBB, ref extentBB)) {
				entity.Position.X = blockBB.Max.X + size.X / 2 + Adjustment;
				ClipX(ref size, ref entityBB, ref extentBB);
				hitXMax = true;
			}
		}
		
		void ClipZMax(ref AABB blockBB, ref AABB entityBB, bool wasOn, ref AABB finalBB, ref AABB extentBB, ref Vector3 size) {
			if (!wasOn || !DidSlide(ref blockBB, ref size, ref finalBB, ref entityBB, ref extentBB)) {
				entity.Position.Z = blockBB.Max.Z + size.Z / 2 + Adjustment;
				ClipZ(ref size, ref entityBB, ref extentBB);
				hitZMax = true;
			}
		}
		
		void ClipZMin(ref AABB blockBB, ref AABB entityBB, bool wasOn, ref AABB finalBB, ref AABB extentBB, ref Vector3 size) {
			if (!wasOn || !DidSlide(ref blockBB, ref size, ref finalBB, ref entityBB, ref extentBB)) {
				entity.Position.Z = blockBB.Min.Z - size.Z / 2 - Adjustment;
				ClipZ(ref size, ref entityBB, ref extentBB);
				hitZMin = true;
			}
		}
		
		void ClipYMin(ref AABB blockBB, ref AABB entityBB, ref AABB extentBB, ref Vector3 size) {
			entity.Position.Y = blockBB.Min.Y - size.Y - Adjustment;
			ClipY(ref size, ref entityBB, ref extentBB);
			hitYMin = true;
		}
		
		void ClipYMax(ref AABB blockBB, ref AABB entityBB, ref AABB extentBB, ref Vector3 size) {
			entity.Position.Y = blockBB.Max.Y + Adjustment;
			entity.onGround = true;
			ClipY(ref size, ref entityBB, ref extentBB);
			hitYMax = true;
		}
		
		bool DidSlide(ref AABB blockBB, ref Vector3 size, ref AABB finalBB, ref AABB entityBB, ref AABB extentBB) {
			float yDist = blockBB.Max.Y - entityBB.Min.Y;
			if (yDist > 0 && yDist <= entity.StepSize + 0.01f) {
				float blockBB_MinX = Math.Max(blockBB.Min.X, blockBB.Max.X - size.X / 2);
				float blockBB_MaxX = Math.Min(blockBB.Max.X, blockBB.Min.X + size.X / 2);
				float blockBB_MinZ = Math.Max(blockBB.Min.Z, blockBB.Max.Z - size.Z / 2);
				float blockBB_MaxZ = Math.Min(blockBB.Max.Z, blockBB.Min.Z + size.Z / 2);
				
				AABB adjBB;
				adjBB.Min.X = Math.Min(finalBB.Min.X, blockBB_MinX + Adjustment);
				adjBB.Max.X = Math.Max(finalBB.Max.X, blockBB_MaxX - Adjustment);
				adjBB.Min.Y = blockBB.Max.Y + Adjustment;
				adjBB.Max.Y = adjBB.Min.Y + size.Y;
				adjBB.Min.Z = Math.Min(finalBB.Min.Z, blockBB_MinZ + Adjustment);
				adjBB.Max.Z = Math.Max(finalBB.Max.Z, blockBB_MaxZ - Adjustment);
				
				if (!CanSlideThrough(ref adjBB)) return false;				
				entity.Position.Y = adjBB.Min.Y;
				entity.onGround = true;
				ClipY(ref size, ref entityBB, ref extentBB);
				return true;
			}
			return false;
		}
		
		bool CanSlideThrough(ref AABB adjFinalBB) {
			Vector3I bbMin = Vector3I.Floor(adjFinalBB.Min);
			Vector3I bbMax = Vector3I.Floor(adjFinalBB.Max);
			AABB blockBB;
			
			Vector3 pos;
			for (int y = bbMin.Y; y <= bbMax.Y; y++)
				for (int z = bbMin.Z; z <= bbMax.Z; z++)
					for (int x = bbMin.X; x <= bbMax.X; x++)
			{
				pos.X = x; pos.Y = y; pos.Z = z;
				BlockID block = game.World.GetPhysicsBlock(x, y, z);
				blockBB.Min = pos + BlockInfo.MinBB[block];
				blockBB.Max = pos + BlockInfo.MaxBB[block];
				
				if (!blockBB.Intersects(adjFinalBB)) continue;
				if (BlockInfo.Collide[block] == CollideType.Solid) return false;
			}
			return true;
		}
		
		void ClipX(ref Vector3 size, ref AABB entityBB, ref AABB extentBB) {
			entity.Velocity.X = 0;
			entityBB.Min.X = entity.Position.X - size.X / 2; extentBB.Min.X = entityBB.Min.X;
			entityBB.Max.X = entity.Position.X + size.X / 2; extentBB.Max.X = entityBB.Max.X;
		}
		
		void ClipY(ref Vector3 size, ref AABB entityBB, ref AABB extentBB) {
			entity.Velocity.Y = 0;
			entityBB.Min.Y = entity.Position.Y;              extentBB.Min.Y = entityBB.Min.Y;
			entityBB.Max.Y = entity.Position.Y + size.Y;     extentBB.Max.Y = entityBB.Max.Y;
		}
		
		void ClipZ(ref Vector3 size, ref AABB entityBB, ref AABB extentBB) {
			entity.Velocity.Z = 0;
			entityBB.Min.Z = entity.Position.Z - size.Z / 2; extentBB.Min.Z = entityBB.Min.Z;
			entityBB.Max.Z = entity.Position.Z + size.Z / 2; extentBB.Max.Z = entityBB.Max.Z;
		}
	}
}