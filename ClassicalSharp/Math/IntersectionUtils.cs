// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Contains various methods for intersecting geometry. </summary>
	public static class Intersection {
		
		/// <summary> Calculates the intersection points of a ray and a rotated bounding box.  </summary>
		internal static bool RayIntersectsRotatedBox( Vector3 origin, Vector3 dir, Entity target, out float tMin, out float tMax ) {
			// This is the rotated AABB of the model we want to test for intersection
			//        *
			//       / \     we then perform a counter       *---*   and we can then do
			// ====>* x *    rotation to the rotated AABB    | x |   a standard AABB test  
			//       \ /     and ray origin and direction    *---*   with the rotated ray
			//        *                                     /
			//                                             /
			Vector3 rotatedOrigin = target.Position + Utils.RotateY( origin - target.Position, -target.HeadYawRadians );
			Vector3 rotatedDir = Utils.RotateY( dir, -target.HeadYawRadians );
			AABB bb = target.PickingBounds;
			return RayIntersectsBox( rotatedOrigin, rotatedDir, bb.Min, bb.Max, out tMin, out tMax );
		}
		
		//http://www.cs.utah.edu/~awilliam/box/box.pdf
		/// <summary> Calculates the intersection points of a ray and a bounding box.  </summary>
		public static bool RayIntersectsBox( Vector3 origin, Vector3 dir, Vector3 min, Vector3 max,
		                                    out float t0, out float t1 ) {
			t0 = t1 = 0;
			float tmin, tmax, tymin, tymax, tzmin, tzmax;
			float invDirX = 1 / dir.X;
			if( invDirX >= 0 ) {
				tmin = ( min.X - origin.X ) * invDirX;
				tmax = ( max.X - origin.X ) * invDirX;
			} else {
				tmin = ( max.X - origin.X ) * invDirX;
				tmax = ( min.X - origin.X ) * invDirX;
			}
			
			float invDirY = 1 / dir.Y;
			if( invDirY >= 0 ) {
				tymin = ( min.Y - origin.Y ) * invDirY;
				tymax = ( max.Y - origin.Y ) * invDirY;
			} else {
				tymin = ( max.Y - origin.Y ) * invDirY;
				tymax = ( min.Y - origin.Y ) * invDirY;
			}
			if( tmin > tymax || tymin > tmax )
				return false;
			if( tymin > tmin )
				tmin = tymin;
			if( tymax < tmax )
				tmax = tymax;
			
			float invDirZ = 1 / dir.Z;
			if( invDirZ >= 0 ) {
				tzmin = ( min.Z - origin.Z ) * invDirZ;
				tzmax = ( max.Z - origin.Z ) * invDirZ;
			} else {
				tzmin = ( max.Z - origin.Z ) * invDirZ;
				tzmax = ( min.Z - origin.Z ) * invDirZ;
			}
			if( tmin > tzmax || tzmin > tmax )
				return false;
			if( tzmin > tmin )
				tmin = tzmin;
			if( tzmax < tmax )
				tmax = tzmax;
			t0 = tmin;
			t1 = tmax;
			return true;
		}
		
		//http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/raytri/raytri.c
		/// <summary> Calculates the intersection point of a ray and a triangle. </summary>
		public static bool RayTriangleIntersect( Vector3 orig, Vector3 dir, Vector3 p0, Vector3 p1, Vector3 p2, out Vector3 intersect ) {
			Vector3 edge1 = p1 - p0;
			Vector3 edge2 = p2 - p0;
			intersect = Vector3.Zero;

			Vector3 p = Vector3.Cross( dir, edge2 );
			float det = Vector3.Dot( edge1, p );
			if( det > -0.000001f && det < 0.000001f ) return false;
			
			float invDet = 1 / det;
			Vector3 s = orig - p0;
			
			float u = Vector3.Dot( s, p ) * invDet;
			if( u < 0 || u > 1 ) return false;
			
			Vector3 q = Vector3.Cross( s, edge1 );
			float v = Vector3.Dot( dir, q ) * invDet;
			if( v < 0 || u + v > 1 ) return false;

			float t = Vector3.Dot( edge2, q ) * invDet;
			intersect = orig + dir * t;
			return t > 0.000001f;
		}
	}
}
