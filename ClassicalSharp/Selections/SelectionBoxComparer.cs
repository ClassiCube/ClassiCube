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
			// Reversed comparison order because we need to render back to front for alpha blending.
			return minDistA == minDistB ? maxDistB.CompareTo( maxDistA ) : minDistB.CompareTo( minDistA );
		}
		
		void Intersect( SelectionBox box, Vector3 origin, ref float closest, ref float furthest ) {
			Vector3I min = box.Min;
			Vector3I max = box.Max;
			// Bottom corners
			UpdateDist( pos, min.X, min.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos, max.X, min.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos, max.X, min.Y, max.Z, ref closest, ref furthest );
			UpdateDist( pos, min.X, min.Y, max.Z, ref closest, ref furthest );
			// top corners
			UpdateDist( pos, min.X, max.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos, max.X, max.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos, max.X, max.Y, max.Z, ref closest, ref furthest );
			UpdateDist( pos, min.X, max.Y, max.Z, ref closest, ref furthest );
		}
		
		static void UpdateDist( Vector3 p, float x2, float y2, float z2,
		                       ref float closest, ref float furthest ) {
			float dist = Utils.DistanceSquared( p.X, p.Y, p.Z, x2, y2, z2 );
			if( dist < closest ) closest = dist;
			if( dist > furthest ) furthest = dist;
		}
	}
}