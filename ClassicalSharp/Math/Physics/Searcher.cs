// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using ClassicalSharp.Entities;
using ClassicalSharp.Map;
using System;
using OpenTK;

namespace ClassicalSharp.Physics {
	
	public struct State {
		public int X, Y, Z;
		public float tSquared;
		
		public State(int x, int y, int z, byte block, float tSquared) {
			X = x << 3; Y = y << 3; Z = z << 3;
			X |= (block & 0x07);
			Y |= (block & 0x38) >> 3;
			Z |= (block & 0xC0) >> 6;
			this.tSquared = tSquared;
		}
	}
	
	/// <summary> Calculates all possible blocks that a moving entity can intersect with. </summary>
	public sealed class Searcher {
		
		public static State[] stateCache = new State[0];
		
		public static int FindReachableBlocks(Game game, Entity entity,
		                                      out AABB entityBB, out AABB entityExtentBB) {
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
			Vector3I min = Vector3I.Floor(entityExtentBB.Min);
			Vector3I max = Vector3I.Floor(entityExtentBB.Max);
			
			int elements = (max.X + 1 - min.X) * (max.Y + 1 - min.Y) * (max.Z + 1 - min.Z);
			if (elements > stateCache.Length) {
				stateCache = new State[elements];
			}
			
			
			AABB blockBB = default(AABB);
			BlockInfo info = game.BlockInfo;
			int count = 0;
			// Order loops so that we minimise cache misses
			for (int y = min.Y; y <= max.Y; y++)
				for (int z = min.Z; z <= max.Z; z++)
					for (int x = min.X; x <= max.X; x++)
			{
				byte block = game.World.GetPhysicsBlock(x, y, z);
				if (info.Collide[block] != CollideType.Solid) continue;
				
				blockBB.Min = info.MinBB[block];
				blockBB.Min.X += x; blockBB.Min.Y += y; blockBB.Min.Z += z;
				blockBB.Max = info.MaxBB[block];
				blockBB.Max.X += x; blockBB.Max.Y += y; blockBB.Max.Z += z;
				
				if (!entityExtentBB.Intersects(blockBB)) continue; // necessary for non whole blocks. (slabs)
				
				float tx = 0, ty = 0, tz = 0;
				CalcTime(ref vel, ref entityBB, ref blockBB, out tx, out ty, out tz);
				if (tx > 1 || ty > 1 || tz > 1) continue;
				float tSquared = tx * tx + ty * ty + tz * tz;
				stateCache[count++] = new State(x, y, z, block, tSquared);
			}
			
			if (count > 0)
				QuickSort(stateCache, 0, count - 1);
			return count;
		}

		
		public static void CalcTime(ref Vector3 vel, ref AABB entityBB, ref AABB blockBB,
		                            out float tx, out float ty, out float tz) {
			float dx = vel.X > 0 ? blockBB.Min.X - entityBB.Max.X : entityBB.Min.X - blockBB.Max.X;
			float dy = vel.Y > 0 ? blockBB.Min.Y - entityBB.Max.Y : entityBB.Min.Y - blockBB.Max.Y;
			float dz = vel.Z > 0 ? blockBB.Min.Z - entityBB.Max.Z : entityBB.Min.Z - blockBB.Max.Z;
			
			tx = vel.X == 0 ? float.PositiveInfinity : Math.Abs(dx / vel.X);
			ty = vel.Y == 0 ? float.PositiveInfinity : Math.Abs(dy / vel.Y);
			tz = vel.Z == 0 ? float.PositiveInfinity : Math.Abs(dz / vel.Z);
			
			if (entityBB.Max.X >= blockBB.Min.X && entityBB.Min.X <= blockBB.Max.X) tx = 0; // Inlined XIntersects
			if (entityBB.Max.Y >= blockBB.Min.Y && entityBB.Min.Y <= blockBB.Max.Y) ty = 0; // Inlined YIntersects
			if (entityBB.Max.Z >= blockBB.Min.Z && entityBB.Min.Z <= blockBB.Max.Z) tz = 0; // Inlined ZIntersects
		}
		
		static void QuickSort(State[] keys, int left, int right) {
			while (left < right) {
				int i = left, j = right;
				float pivot = keys[(i + j) / 2].tSquared;
				// partition the list
				while (i <= j) {
					while (pivot > keys[i].tSquared) i++;
					while (pivot < keys[j].tSquared) j--;
					
					if (i <= j) {
						State key = keys[i]; keys[i] = keys[j]; keys[j] = key;
						i++; j--;
					}
				}
				
				// recurse into the smaller subset
				if (j - left <= right - i) {
					if (left < j)
						QuickSort(keys, left, j);
					left = i;
				} else {
					if (i < right)
						QuickSort(keys, i, right);
					right = j;
				}
			}
		}
	}
}