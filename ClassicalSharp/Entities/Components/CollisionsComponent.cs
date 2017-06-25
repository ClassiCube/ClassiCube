// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Physics;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs collision detection. </summary>
	public sealed class CollisionsComponent {
		
		Game game;
		Entity entity;
		BlockInfo info;
		public CollisionsComponent(Game game, Entity entity) {
			this.game = game;
			this.entity = entity;
			info = game.BlockInfo;
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
			Vector3 size = entity.Size;
			AABB entityBB, entityExtentBB;
			
			int count = Searcher.FindReachableBlocks(game, entity,
			                                         out entityBB, out entityExtentBB);
			CollideWithReachableBlocks(count, ref size, ref entityBB, ref entityExtentBB);
		}

		void CollideWithReachableBlocks(int count, ref Vector3 size,
		                                ref AABB entityBB, ref AABB entityExtentBB) {
			// Reset collision detection states
			bool wasOn = entity.onGround;
			entity.onGround = false;
			hitXMin = false; hitYMin = false; hitZMin = false;
			hitXMax = false; hitYMax = false; hitZMax = false;
			AABB blockBB = default(AABB);
			Vector3 bPos;

			for (int i = 0; i < count; i++) {
				// Unpack the block and coordinate data
				State state = Searcher.stateCache[i];
				bPos.X = state.X >> 3; bPos.Y = state.Y >> 3; bPos.Z = state.Z >> 3;				
				int block = (state.X & 0x7) | (state.Y & 0x7) << 3 | (state.Z & 0x7) << 6;
				blockBB.Min = bPos + info.MinBB[block];
				blockBB.Max = bPos + info.MaxBB[block];
				if (!entityExtentBB.Intersects(blockBB)) continue;
				
				// Recheck time to collide with block (as colliding with blocks modifies this)
				float tx = 0, ty = 0, tz = 0;
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
						ClipYMax(ref blockBB, ref entityBB, ref entityExtentBB, ref size);
					} else if (finalBB.Max.Y - Adjustment <= blockBB.Min.Y) {
						ClipYMin(ref blockBB, ref entityBB, ref entityExtentBB, ref size);
					} else if (finalBB.Min.X + Adjustment >= blockBB.Max.X) {
						ClipXMax(ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size);
					} else if (finalBB.Max.X - Adjustment <= blockBB.Min.X) {
						ClipXMin(ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size);
					} else if (finalBB.Min.Z + Adjustment >= blockBB.Max.Z) {
						ClipZMax(ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size);
					} else if (finalBB.Max.Z - Adjustment <= blockBB.Min.Z) {
						ClipZMin(ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size);
					}
					continue;
				}
				
				// if flying or falling, test the horizontal axes first.
				if (finalBB.Min.X + Adjustment >= blockBB.Max.X) {
					ClipXMax(ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size);
				} else if (finalBB.Max.X - Adjustment <= blockBB.Min.X) {
					ClipXMin(ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size);
				} else if (finalBB.Min.Z + Adjustment >= blockBB.Max.Z) {
					ClipZMax(ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size);
				} else if (finalBB.Max.Z - Adjustment <= blockBB.Min.Z) {
					ClipZMin(ref blockBB, ref entityBB, wasOn, finalBB, ref entityExtentBB, ref size);
				} else if (finalBB.Min.Y + Adjustment >= blockBB.Max.Y) {
					ClipYMax(ref blockBB, ref entityBB, ref entityExtentBB, ref size);
				} else if (finalBB.Max.Y - Adjustment <= blockBB.Min.Y) {
					ClipYMin(ref blockBB, ref entityBB, ref entityExtentBB, ref size);
				}
			}
		}
		
		void ClipXMin(ref AABB blockBB, ref AABB entityBB, bool wasOn,
		              AABB finalBB, ref AABB entityExtentBB, ref Vector3 size) {
			if (!wasOn || !DidSlide(blockBB, ref size, finalBB, ref entityBB, ref entityExtentBB)) {
				entity.Position.X = blockBB.Min.X - size.X / 2 - Adjustment;
				ClipX(ref size, ref entityBB, ref entityExtentBB);
				hitXMin = true;
			}
		}
		
		void ClipXMax(ref AABB blockBB, ref AABB entityBB, bool wasOn,
		              AABB finalBB, ref AABB entityExtentBB, ref Vector3 size) {
			if (!wasOn || !DidSlide(blockBB, ref size, finalBB, ref entityBB, ref entityExtentBB)) {
				entity.Position.X = blockBB.Max.X + size.X / 2 + Adjustment;
				ClipX(ref size, ref entityBB, ref entityExtentBB);
				hitXMax = true;
			}
		}
		
		void ClipZMax(ref AABB blockBB, ref AABB entityBB, bool wasOn,
		              AABB finalBB, ref AABB entityExtentBB, ref Vector3 size) {
			if (!wasOn || !DidSlide(blockBB, ref size, finalBB, ref entityBB, ref entityExtentBB)) {
				entity.Position.Z = blockBB.Max.Z + size.Z / 2 + Adjustment;
				ClipZ(ref size, ref entityBB, ref entityExtentBB);
				hitZMax = true;
			}
		}
		
		void ClipZMin(ref AABB blockBB, ref AABB entityBB, bool wasOn,
		              AABB finalBB, ref AABB extentBB, ref Vector3 size) {
			if (!wasOn || !DidSlide(blockBB, ref size, finalBB, ref entityBB, ref extentBB)) {
				entity.Position.Z = blockBB.Min.Z - size.Z / 2 - Adjustment;
				ClipZ(ref size, ref entityBB, ref extentBB);
				hitZMin = true;
			}
		}
		
		void ClipYMin(ref AABB blockBB, ref AABB entityBB,
		              ref AABB extentBB, ref Vector3 size) {
			entity.Position.Y = blockBB.Min.Y - size.Y - Adjustment;
			ClipY(ref size, ref entityBB, ref extentBB);
			hitYMin = true;
		}
		
		void ClipYMax(ref AABB blockBB, ref AABB entityBB,
		              ref AABB extentBB, ref Vector3 size) {
			entity.Position.Y = blockBB.Max.Y + Adjustment;
			entity.onGround = true;
			ClipY(ref size, ref entityBB, ref extentBB);
			hitYMax = true;
		}
		
		bool DidSlide(AABB blockBB, ref Vector3 size, AABB finalBB,
		              ref AABB entityBB, ref AABB entityExtentBB) {
			float yDist = blockBB.Max.Y - entityBB.Min.Y;
			if (yDist > 0 && yDist <= entity.StepSize + 0.01f) {
				float blockXMin = blockBB.Min.X, blockZMin = blockBB.Min.Z;
				blockBB.Min.X = Math.Max(blockBB.Min.X, blockBB.Max.X - size.X / 2);
				blockBB.Max.X = Math.Min(blockBB.Max.X, blockXMin + size.X / 2);
				blockBB.Min.Z = Math.Max(blockBB.Min.Z, blockBB.Max.Z - size.Z / 2);
				blockBB.Max.Z = Math.Min(blockBB.Max.Z, blockZMin + size.Z / 2);
				
				AABB adjBB = finalBB;
				adjBB.Min.X = Math.Min(finalBB.Min.X, blockBB.Min.X + Adjustment);
				adjBB.Max.X = Math.Max(finalBB.Max.X, blockBB.Max.X - Adjustment);
				adjBB.Min.Y = blockBB.Max.Y + Adjustment;
				adjBB.Max.Y = adjBB.Min.Y + size.Y;
				adjBB.Min.Z = Math.Min(finalBB.Min.Z, blockBB.Min.Z + Adjustment);
				adjBB.Max.Z = Math.Max(finalBB.Max.Z, blockBB.Max.Z - Adjustment);
				
				if (!CanSlideThrough(ref adjBB))
					return false;
				entity.Position.Y = blockBB.Max.Y + Adjustment;
				entity.onGround = true;
				ClipY(ref size, ref entityBB, ref entityExtentBB);
				return true;
			}
			return false;
		}
		
		bool CanSlideThrough(ref AABB adjFinalBB) {
			Vector3I bbMin = Vector3I.Floor(adjFinalBB.Min);
			Vector3I bbMax = Vector3I.Floor(adjFinalBB.Max);
			
			for (int y = bbMin.Y; y <= bbMax.Y; y++)
				for (int z = bbMin.Z; z <= bbMax.Z; z++)
					for (int x = bbMin.X; x <= bbMax.X; x++)
			{
				BlockID block = game.World.GetPhysicsBlock(x, y, z);
				Vector3 min = new Vector3(x, y, z) + info.MinBB[block];
				Vector3 max = new Vector3(x, y, z) + info.MaxBB[block];
				
				AABB blockBB = new AABB(min, max);
				if (!blockBB.Intersects(adjFinalBB))
					continue;
				if (info.Collide[block] == CollideType.Solid)
					return false;
			}
			return true;
		}
		
		void ClipX(ref Vector3 size, ref AABB entityBB, ref AABB entityExtentBB) {
			entity.Velocity.X = 0;
			entityBB.Min.X = entity.Position.X - size.X / 2; entityExtentBB.Min.X = entityBB.Min.X;
			entityBB.Max.X = entity.Position.X + size.X / 2; entityExtentBB.Max.X = entityBB.Max.X;
		}
		
		void ClipY(ref Vector3 size, ref AABB entityBB, ref AABB entityExtentBB) {
			entity.Velocity.Y = 0;
			entityBB.Min.Y = entity.Position.Y;              entityExtentBB.Min.Y = entityBB.Min.Y;
			entityBB.Max.Y = entity.Position.Y + size.Y;     entityExtentBB.Max.Y = entityBB.Max.Y;
		}
		
		void ClipZ(ref Vector3 size, ref AABB entityBB, ref AABB entityExtentBB) {
			entity.Velocity.Z = 0;
			entityBB.Min.Z = entity.Position.Z - size.Z / 2; entityExtentBB.Min.Z = entityBB.Min.Z;
			entityBB.Max.Z = entity.Position.Z + size.Z / 2; entityExtentBB.Max.Z = entityBB.Max.Z;
		}
	}
}