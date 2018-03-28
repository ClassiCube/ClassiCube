// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Map;
using ClassicalSharp.Physics;
using OpenTK;
using BlockID = System.UInt16;

namespace ClassicalSharp {
	public static class Picking {
		
		static RayTracer t = new RayTracer();
		
		/// <summary> Determines the picked block based on the given origin and direction vector.<br/>
		/// Marks pickedPos as invalid if a block could not be found due to going outside map boundaries
		/// or not being able to find a suitable candiate within the given reach distance. </summary>
		public static void CalculatePickedBlock(Game game, Vector3 origin, Vector3 dir,
		                                        float reach, PickedPos pos) {
			if (!RayTrace(game, origin, dir, reach, pos, false)) {
				pos.SetAsInvalid();
			}
		}
		
		static bool PickBlock(Game game, PickedPos pos) {
			if (!game.CanPick(t.Block)) return false;
			
			// This cell falls on the path of the ray. Now perform an additional bounding box test,
			// since some blocks do not occupy a whole cell.
			float t0, t1;
			if (!Intersection.RayIntersectsBox(t.Origin, t.Dir, t.Min, t.Max, out t0, out t1))
				return false;
			Vector3 I = t.Origin + t.Dir * t0;
			
			// Only pick the block if the block is precisely within reach distance.
			float lenSq = (I - t.Origin).LengthSquared;
			float reach = game.LocalPlayer.ReachDistance;
			if (lenSq <= reach * reach) {
				pos.SetAsValid(t.X, t.Y, t.Z, t.Min, t.Max, t.Block, I);
			} else {
				pos.SetAsInvalid();
			}
			return true;
		}
		
		public static void ClipCameraPos(Game game, Vector3 origin, Vector3 dir,
		                                 float reach, PickedPos pos) {
			bool noClip = !game.CameraClipping || game.LocalPlayer.Hacks.Noclip;
			if (noClip || !RayTrace(game, origin, dir, reach, pos, true)) {
				pos.SetAsInvalid();
				pos.Intersect = origin + dir * reach;
			}
		}
		
		static bool CameraClip(Game game, PickedPos pos) {
			if (BlockInfo.Draw[t.Block] == DrawType.Gas || BlockInfo.Collide[t.Block] != CollideType.Solid)
				return false;
			
			float t0, t1;
			const float adjust = 0.1f;
			if (!Intersection.RayIntersectsBox(t.Origin, t.Dir, t.Min, t.Max, out t0, out t1))
				return false;
			Vector3 I = t.Origin + t.Dir * t0;
			pos.SetAsValid(t.X, t.Y, t.Z, t.Min, t.Max, t.Block, I);
			
			switch (pos.Face) {
					case BlockFace.XMin: pos.Intersect.X -= adjust; break;
					case BlockFace.XMax: pos.Intersect.X += adjust; break;
					case BlockFace.YMin: pos.Intersect.Y -= adjust; break;
					case BlockFace.YMax: pos.Intersect.Y += adjust; break;
					case BlockFace.ZMin: pos.Intersect.Z -= adjust; break;
					case BlockFace.ZMax: pos.Intersect.Z += adjust; break;
			}
			return true;
		}


		static bool RayTrace(Game game, Vector3 origin, Vector3 dir, float reach,
		                     PickedPos pos, bool clipMode) {
			t.SetVectors(origin, dir);
			float reachSq = reach * reach;
			Vector3I pOrigin = Vector3I.Floor(origin);
			bool insideMap = game.World.IsValidPos(pOrigin);

			Vector3 coords;
			for (int i = 0; i < 10000; i++) {
				int x = t.X, y = t.Y, z = t.Z;
				coords.X = x; coords.Y = y; coords.Z = z;
				t.Block = insideMap ?
					InsideGetBlock(game.World, x, y, z) : OutsideGetBlock(game.World, x, y, z, pOrigin);
				
				Vector3 min = coords + BlockInfo.RenderMinBB[t.Block];
				Vector3 max = coords + BlockInfo.RenderMaxBB[t.Block];
				
				float dx = Math.Min(Math.Abs(origin.X - min.X), Math.Abs(origin.X - max.X));
				float dy = Math.Min(Math.Abs(origin.Y - min.Y), Math.Abs(origin.Y - max.Y));
				float dz = Math.Min(Math.Abs(origin.Z - min.Z), Math.Abs(origin.Z - max.Z));
				if (dx * dx + dy * dy + dz * dz > reachSq) return false;
				
				t.Min = min; t.Max = max;
				bool intersect = clipMode ? CameraClip(game, pos) : PickBlock(game, pos);
				if (intersect) return true;
				t.Step();
			}
			
			throw new InvalidOperationException("did over 10000 iterations in CalculatePickedBlock(). " +
			                                    "Something has gone wrong. (dir: " + dir + ")");
		}

		const BlockID border = Block.Bedrock;
		static BlockID InsideGetBlock(World map, int x, int y, int z) {
			if (x >= 0 && z >= 0 && x < map.Width && z < map.Length) {
				if (y >= map.Height) return Block.Air;
				if (y >= 0) return map.GetBlock(x, y, z);
			}
			
			// bedrock on bottom or outside map
			bool sides = map.Env.SidesBlock != Block.Air;
			int height = map.Env.SidesHeight; if (height < 1) height = 1;
			return sides && y < height ? border : Block.Air;
		}
		
		static BlockID OutsideGetBlock(World map, int x, int y, int z, Vector3I origin) {
			if (x < 0 || z < 0 || x >= map.Width || z >= map.Length) return Block.Air;
			bool sides = map.Env.SidesBlock != Block.Air;
			// handling of blocks inside the map, above, and on borders
			
			if (y >= map.Height) return Block.Air;
			if (sides && y == -1 && origin.Y > 0) return border;
			if (sides && y == 0  && origin.Y < 0) return border;
			int height = map.Env.SidesHeight; if (height < 1) height = 1;
			
			if (sides && x == 0        && y >= 0 && y < height && origin.X < 0)           return border;
			if (sides && z == 0        && y >= 0 && y < height && origin.Z < 0)           return border;
			if (sides && x == map.MaxX && y >= 0 && y < height && origin.X >= map.Width)  return border;
			if (sides && z == map.MaxZ && y >= 0 && y < height && origin.Z >= map.Length) return border;
			
			if (y >= 0) return map.GetBlock(x, y, z);
			return Block.Air;
		}
	}
}