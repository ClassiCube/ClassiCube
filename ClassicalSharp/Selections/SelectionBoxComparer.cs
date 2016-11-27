// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using OpenTK;

namespace ClassicalSharp.Selections {
	
	internal class SelectionBoxComparer : IComparer<SelectionBox> {
		
		public int Compare(SelectionBox a, SelectionBox b) {
			// Reversed comparison order because we need to render back to front for alpha blending.
			return a.MinDist == b.MinDist ? b.MaxDist.CompareTo(a.MaxDist) 
				: b.MinDist.CompareTo(a.MinDist);
		}
		
		internal void Intersect(SelectionBox box, Vector3 origin) {
			Vector3I min = box.Min, max = box.Max;
			float closest = float.PositiveInfinity, furthest = float.NegativeInfinity;
			// Bottom corners
			UpdateDist(origin, min.X, min.Y, min.Z, ref closest, ref furthest);
			UpdateDist(origin, max.X, min.Y, min.Z, ref closest, ref furthest);
			UpdateDist(origin, max.X, min.Y, max.Z, ref closest, ref furthest);
			UpdateDist(origin, min.X, min.Y, max.Z, ref closest, ref furthest);
			// Top corners
			UpdateDist(origin, min.X, max.Y, min.Z, ref closest, ref furthest);
			UpdateDist(origin, max.X, max.Y, min.Z, ref closest, ref furthest);
			UpdateDist(origin, max.X, max.Y, max.Z, ref closest, ref furthest);
			UpdateDist(origin, min.X, max.Y, max.Z, ref closest, ref furthest);
			box.MinDist = closest; box.MaxDist = furthest;
		}
		
		static void UpdateDist(Vector3 p, float x2, float y2, float z2,
		                       ref float closest, ref float furthest) {
			float dist = Utils.DistanceSquared(p.X, p.Y, p.Z, x2, y2, z2);
			if (dist < closest) closest = dist;
			if (dist > furthest) furthest = dist;
		}
	}
}