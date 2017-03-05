// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Model;
using ClassicalSharp.Physics;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Entities {
	
	/// <summary> Contains a model, along with position, velocity, and rotation.
	/// May also contain other fields and properties. </summary>
	public abstract partial class Entity {
		
		AABB modelAABB;
		
		/// <summary> Returns the bounding box that contains the model, without any rotations applied. </summary>
		public AABB PickingBounds { get { return modelAABB.Offset(Position); } }
		
		/// <summary> Bounding box of the model that collision detection is performed with, in world coordinates. </summary>
		public AABB Bounds { get { return AABB.Make(Position, Size); } }
		
		/// <summary> Determines whether any of the blocks that intersect the
		/// bounding box of this entity satisfy the given condition. </summary>
		public bool TouchesAny(Predicate<BlockID> condition) {
			return TouchesAny(Bounds, condition);
		}
		
		/// <summary> Determines whether any of the blocks that intersect the
		/// given bounding box satisfy the given condition. </summary>
		public bool TouchesAny(AABB bounds, Predicate<BlockID> condition) {
			Vector3I bbMin = Vector3I.Floor(bounds.Min);
			Vector3I bbMax = Vector3I.Floor(bounds.Max);
			
			BlockInfo info = game.BlockInfo;
			AABB blockBB = default(AABB);
			
			// Order loops so that we minimise cache misses
			for (int y = bbMin.Y; y <= bbMax.Y; y++)
				for (int z = bbMin.Z; z <= bbMax.Z; z++)
					for (int x = bbMin.X; x <= bbMax.X; x++)
			{
				if (!game.World.IsValidPos(x, y, z)) continue;
				BlockID block = game.World.GetBlock(x, y, z);
				blockBB.Min = new Vector3(x, y, z) + info.MinBB[block];
				blockBB.Max = new Vector3(x, y, z) + info.MaxBB[block];
				
				if (!blockBB.Intersects(bounds)) continue;
				if (condition(block)) return true;
			}
			return false;
		}
		
		/// <summary> Determines whether any of the blocks that intersect the
		/// bounding box of this entity are rope. </summary>
		public bool TouchesAnyRope() {
			AABB bounds = Bounds;
			bounds.Max.Y += 0.5f/16f;
			return TouchesAny(bounds, b => b == Block.Rope);
		}
		
		/// <summary> Constant offset used to avoid floating point roundoff errors. </summary>
		public const float Adjustment = 0.001f;
		
		
		static readonly Vector3 liqExpand = new Vector3(0.25f/16f, 0, 0.25f/16f);
		bool TouchesAnyLiquid(AABB bounds, BlockID block1, BlockID block2) {
			Vector3I bbMin = Vector3I.Floor(bounds.Min);
			Vector3I bbMax = Vector3I.Floor(bounds.Max);
			
			int height = game.World.Height;
			BlockInfo info = game.BlockInfo;
			AABB blockBB = default(AABB);
			
			// Order loops so that we minimise cache misses
			for (int y = bbMin.Y; y <= bbMax.Y; y++)
				for (int z = bbMin.Z; z <= bbMax.Z; z++)
					for (int x = bbMin.X; x <= bbMax.X; x++)
			{
				if (!game.World.IsValidPos(x, y, z)) continue;
				BlockID block = game.World.GetBlock(x, y, z);
				blockBB.Min = new Vector3(x, y, z) + info.MinBB[block];
				blockBB.Max = new Vector3(x, y, z) + info.MaxBB[block];
				
				if (!blockBB.Intersects(bounds)) continue;
				if (block == block1 || block == block2) return true;
			}
			return false;
		}
		
		/// <summary> Determines whether any of the blocks that intersect the
		/// bounding box of this entity are lava or still lava. </summary>
		public bool TouchesAnyLava() {
			AABB bounds = Bounds.Offset(liqExpand); bounds.Min.Y += 6/16f; bounds.Max.Y -= 6/16f;
			return TouchesAnyLiquid(bounds, Block.Lava, Block.StillLava);
		}

		/// <summary> Determines whether any of the blocks that intersect the
		/// bounding box of this entity are water or still water. </summary>
		public bool TouchesAnyWater() {
			AABB bounds = Bounds.Offset(liqExpand); bounds.Min.Y += 6/16f; bounds.Max.Y -= 6/16f;
			return TouchesAnyLiquid(bounds, Block.Water, Block.StillWater);
		}
	}
}