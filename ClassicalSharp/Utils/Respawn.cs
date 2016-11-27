// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Physics;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Shared helper class for respawning an entity. </summary>
	public static class Respawn {

		public static float HighestFreeY(Game game, ref AABB bb) {
			int minX = Utils.Floor(bb.Min.X), maxX = Utils.Floor(bb.Max.X);
			int minY = Utils.Floor(bb.Min.Y), maxY = Utils.Floor(bb.Max.Y);
			int minZ = Utils.Floor(bb.Min.Z), maxZ = Utils.Floor(bb.Max.Z);
			
			BlockInfo info = game.BlockInfo;
			float spawnY = float.NegativeInfinity;
			AABB blockBB = default(AABB);
			
			for (int y = minY; y <= maxY; y++)
				for (int z = minZ; z <= maxZ; z++)
					for (int x = minX; x <= maxX; x++)
			{
				byte block = game.World.GetPhysicsBlock(x, y, z);
				blockBB.Min = new Vector3(x, y, z) + info.MinBB[block];
				blockBB.Max = new Vector3(x, y, z) + info.MaxBB[block];
				
				if (info.Collide[block] != CollideType.Solid) continue;
				if (!bb.Intersects(blockBB)) continue;				
				spawnY = Math.Max(spawnY, blockBB.Max.Y);
			}
			return spawnY;
		}
	}
}