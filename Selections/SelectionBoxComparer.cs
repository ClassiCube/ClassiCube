using System;
using System.Collections.Generic;
using OpenTK;

namespace ClassicalSharp.Selections {
	
	internal class SelectionBoxComparer : IComparer<SelectionBox> {
		
		public Vector3 pos;
		
		public int Compare( SelectionBox a, SelectionBox b ) {
			float minDistA = float.PositiveInfinity, minDistB = float.PositiveInfinity;
			float maxDistA = float.NegativeInfinity, maxDistB = float.NegativeInfinity;
			Intersect( a, pos, ref minDistA, ref maxDistA );
			Intersect( b, pos, ref minDistB, ref maxDistB );
			return maxDistA == maxDistB ? minDistA.CompareTo( minDistB ) : maxDistA.CompareTo( maxDistB );
		}
		
		void Intersect( SelectionBox box, Vector3 origin, ref float closest, ref float furthest ) {
			Vector3I min = box.Min;
			Vector3I max = box.Max;
			// Bottom corners
			UpdateDist( pos.X, pos.Y, pos.Z, min.X, min.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, max.X, min.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, max.X, min.Y, max.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, min.X, min.Y, max.Z, ref closest, ref furthest );
			// top corners
			UpdateDist( pos.X, pos.Y, pos.Z, min.X, max.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, max.X, max.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, max.X, max.Y, max.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, min.X, max.Y, max.Z, ref closest, ref furthest );
		}
		
		static void UpdateDist( float x1, float y1, float z1, float x2, float y2, float z2,
		                       ref float closest, ref float furthest ) {
			float dist = Utils.DistanceSquared( x1, y1, z1, x2, y2, z2 );
			if( dist < closest ) closest = dist;
			if( dist > furthest ) furthest = dist;
		}
	}
}