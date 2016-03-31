// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp {

	// http://www.xnawiki.com/index.php/Voxel_traversal
	// https://web.archive.org/web/20120113051728/http://www.xnawiki.com/index.php?title=Voxel_traversal
	
	// Implementation based on: "A Fast Voxel Traversal Algorithm for Ray Tracing"
	// John Amanatides, Andrew Woo
	// http://www.cse.yorku.ca/~amana/research/grid.pdf
	// http://www.devmaster.net/articles/raytracing_series/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf
	public sealed class RayTracer {
		
		public int X, Y, Z;
		Vector3I step, cellBoundary;
		Vector3 tMax, tDelta;
		
		public void SetRayData( Vector3 origin, Vector3 dir ) {
			Vector3I start = Vector3I.Floor( origin ); // Rounds the position's X, Y and Z down to the nearest integer values.
			// The cell in which the ray starts.
			X = start.X; Y = start.Y; Z = start.Z;
			
			// Determine which way we go.
			step.X = Math.Sign( dir.X ); step.Y = Math.Sign( dir.Y ); step.Z = Math.Sign( dir.Z );
			// Calculate cell boundaries. When the step (i.e. direction sign) is positive,
			// the next boundary is AFTER our current position, meaning that we have to add 1.
			// Otherwise, it is BEFORE our current position, in which case we add nothing.
			cellBoundary = new Vector3I(
				start.X + (step.X > 0 ? 1 : 0),
				start.Y + (step.Y > 0 ? 1 : 0),
				start.Z + (step.Z > 0 ? 1 : 0) );
			
			// NOTE: we want it so if dir.x = 0, tmax.x = positive infinity
			// Determine how far we can travel along the ray before we hit a voxel boundary.
			tMax = new Vector3(
				(cellBoundary.X - origin.X) / dir.X,    // Boundary is a plane on the YZ axis.
				(cellBoundary.Y - origin.Y) / dir.Y,    // Boundary is a plane on the XZ axis.
				(cellBoundary.Z - origin.Z) / dir.Z );  // Boundary is a plane on the XY axis.
			if( Single.IsNaN( tMax.X ) || Single.IsInfinity( tMax.X ) ) tMax.X = Single.PositiveInfinity;
			if( Single.IsNaN( tMax.Y ) || Single.IsInfinity( tMax.Y ) ) tMax.Y = Single.PositiveInfinity;
			if( Single.IsNaN( tMax.Z ) || Single.IsInfinity( tMax.Z ) ) tMax.Z = Single.PositiveInfinity;

			// Determine how far we must travel along the ray before we have crossed a gridcell.
			tDelta = new Vector3( step.X / dir.X, step.Y / dir.Y, step.Z / dir.Z );
			if( Single.IsNaN( tDelta.X ) ) tDelta.X = Single.PositiveInfinity;
			if( Single.IsNaN( tDelta.Y ) ) tDelta.Y = Single.PositiveInfinity;
			if( Single.IsNaN( tDelta.Z ) ) tDelta.Z = Single.PositiveInfinity;
		}
		
		public void Step() {
			// For each step, determine which distance to the next voxel boundary is lowest (i.e.
			// which voxel boundary is nearest) and walk that way.
			if( tMax.X < tMax.Y && tMax.X < tMax.Z ) {
				// tMax.X is the lowest, an YZ cell boundary plane is nearest.
				X += step.X;
				tMax.X += tDelta.X;
			} else if( tMax.Y < tMax.Z ) {
				// tMax.Y is the lowest, an XZ cell boundary plane is nearest.
				Y += step.Y;
				tMax.Y += tDelta.Y;
			} else {
				// tMax.Z is the lowest, an XY cell boundary plane is nearest.
				Z += step.Z;
				tMax.Z += tDelta.Z;
			}
		}
	}
}