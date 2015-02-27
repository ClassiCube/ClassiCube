using System;
using System.Collections.Generic;
using OpenTK;

namespace ClassicalSharp {
	
	public static class IntersectionUtils {
		
		//http://www.cs.utah.edu/~awilliam/box/box.pdf
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
		public static bool RayTriangleIntersect( Vector3 orig, Vector3 dir, Vector3 p0, Vector3 p1, Vector3 p2, out Vector3 I ) {
			Vector3 edge1 = p1 - p0;
			Vector3 edge2 = p2 - p0;
			I = Vector3.Zero;

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
			I = orig + t * dir;
			return t > 0.000001f;
		}
	}
}
