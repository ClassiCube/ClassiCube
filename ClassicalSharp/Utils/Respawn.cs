// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Physics;
using OpenTK;
using BlockID = System.UInt16;

namespace ClassicalSharp {
	
	/// <summary> Shared helper class for respawning an entity. </summary>
	public static class Respawn {

		public static float HighestFreeY(Game game, ref AABB bb) {
			int minX = Utils.Floor(bb.Min.X), maxX = Utils.Floor(bb.Max.X);
			int minY = Utils.Floor(bb.Min.Y), maxY = Utils.Floor(bb.Max.Y);
			int minZ = Utils.Floor(bb.Min.Z), maxZ = Utils.Floor(bb.Max.Z);
			
			float spawnY = float.NegativeInfinity;
			AABB blockBB = default(AABB);
			
			for (int y = minY; y <= maxY; y++)
				for (int z = minZ; z <= maxZ; z++)
					for (int x = minX; x <= maxX; x++)
			{
				BlockID block = game.World.GetPhysicsBlock(x, y, z);
				blockBB.Min = new Vector3(x, y, z) + BlockInfo.MinBB[block];
				blockBB.Max = new Vector3(x, y, z) + BlockInfo.MaxBB[block];
				
				if (BlockInfo.Collide[block] != CollideType.Solid) continue;
				if (!bb.Intersects(blockBB)) continue;				
				spawnY = Math.Max(spawnY, blockBB.Max.Y);
			}
			return spawnY;
		}
		
		public static Vector3 FindSpawnPosition(Game game, float x, float z, Vector3 modelSize) {
			Vector3 spawn = new Vector3(x, 0, z);
			spawn.Y = game.World.Height + Entity.Adjustment;			
			AABB bb = AABB.Make(spawn, modelSize);
			spawn.Y = 0;
			
			for (int y = game.World.Height; y >= 0; y--) {
				float highestY = HighestFreeY(game, ref bb);
				if (highestY != float.NegativeInfinity) {
					spawn.Y = highestY; break;
				}
				bb.Min.Y -= 1; bb.Max.Y -= 1;
			}
			return spawn;
		}
	}
}