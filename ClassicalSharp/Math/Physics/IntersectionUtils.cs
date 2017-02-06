// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using OpenTK;

namespace ClassicalSharp.Physics {
	
	/// <summary> Contains various methods for intersecting geometry. </summary>
	public static class Intersection {
		
		/// <summary> Calculates the intersection points of a ray and a rotated bounding box.  </summary>
		public static bool RayIntersectsRotatedBox(Vector3 origin, Vector3 dir, Entity target, out float tMin, out float tMax) {
			// This is the rotated AABB of the model we want to test for intersection
			//        *
			//       / \     we then perform a counter       *---*   and we can then do
			// ====>* x *    rotation to the rotated AABB    | x |   a standard AABB test  
			//       \ /     and ray origin and direction    *---*   with the rotated ray
			//        *                                     /
			//                                             /
			origin = InverseRotate(origin - target.Position, target) + target.Position;
			dir =    InverseRotate(dir, target);
			AABB bb = target.PickingBounds;
			return RayIntersectsBox(origin, dir, bb.Min, bb.Max, out tMin, out tMax);
		}
		
		static Vector3 InverseRotate(Vector3 pos, Entity target) {
			pos = Utils.RotateY(pos, -target.RotY * Utils.Deg2Rad);
			pos = Utils.RotateZ(pos, -target.RotZ * Utils.Deg2Rad);
			pos = Utils.RotateX(pos, -target.RotX * Utils.Deg2Rad);
			return pos;
		}
		
		//http://www.cs.utah.edu/~awilliam/box/box.pdf
		/// <summary> Calculates the intersection points of a ray and a bounding box.  </summary>
		public static bool RayIntersectsBox(Vector3 origin, Vector3 dir, Vector3 min, Vector3 max,
		                                    out float t0, out float t1) {
			t0 = t1 = 0;
			float tmin, tmax, tymin, tymax, tzmin, tzmax;
			float invDirX = 1 / dir.X;
			if (invDirX >= 0) {
				tmin = (min.X - origin.X) * invDirX;
				tmax = (max.X - origin.X) * invDirX;
			} else {
				tmin = (max.X - origin.X) * invDirX;
				tmax = (min.X - origin.X) * invDirX;
			}
			
			float invDirY = 1 / dir.Y;
			if (invDirY >= 0) {
				tymin = (min.Y - origin.Y) * invDirY;
				tymax = (max.Y - origin.Y) * invDirY;
			} else {
				tymin = (max.Y - origin.Y) * invDirY;
				tymax = (min.Y - origin.Y) * invDirY;
			}
			if (tmin > tymax || tymin > tmax) return false;
			if (tymin > tmin) tmin = tymin;
			if (tymax < tmax) tmax = tymax;
			
			float invDirZ = 1 / dir.Z;
			if (invDirZ >= 0) {
				tzmin = (min.Z - origin.Z) * invDirZ;
				tzmax = (max.Z - origin.Z) * invDirZ;
			} else {
				tzmin = (max.Z - origin.Z) * invDirZ;
				tzmax = (min.Z - origin.Z) * invDirZ;
			}
			
			if (tmin > tzmax || tzmin > tmax) return false;
			if (tzmin > tmin) tmin = tzmin;
			if (tzmax < tmax) tmax = tzmax;
			
			t0 = tmin;
			t1 = tmax;
			return t0 >= 0;
		}
	}
}
