using System;
using OpenTK;

namespace ClassicalSharp {

	public static class Picking {
		
		// http://www.xnawiki.com/index.php/Voxel_traversal
		//https://web.archive.org/web/20120113051728/http://www.xnawiki.com/index.php?title=Voxel_traversal
		/// <summary> Determines the picked block based on the given origin and direction vector.<br/>
		/// Marks pickedPos as invalid if a block could not be found due to going outside map boundaries 
		/// or not being able to find a suitable candiate within the given reach distance. </summary>
		public static void CalculatePickedBlock( Game window, Vector3 origin, Vector3 dir, float reach, PickedPos pickedPos ) {
			// Implementation based on: "A Fast Voxel Traversal Algorithm for Ray Tracing"
			// John Amanatides, Andrew Woo
			// http://www.cse.yorku.ca/~amana/research/grid.pdf
			// http://www.devmaster.net/articles/raytracing_series/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf

			// The cell in which the ray starts.
			Vector3I start = Vector3I.Floor( origin ); // Rounds the position's X, Y and Z down to the nearest integer values.
			int x = start.X, y = start.Y, z = start.Z;
			// Determine which way we go.
			int stepX = Math.Sign( dir.X ), stepY = Math.Sign( dir.Y ), stepZ = Math.Sign( dir.Z );

			// Calculate cell boundaries. When the step (i.e. direction sign) is positive,
			// the next boundary is AFTER our current position, meaning that we have to add 1.
			// Otherwise, it is BEFORE our current position, in which case we add nothing.
			Vector3I cellBoundary = new Vector3I(
				x + (stepX > 0 ? 1 : 0),
				y + (stepY > 0 ? 1 : 0),
				z + (stepZ > 0 ? 1 : 0) );

			// NOTE: we want it so if dir.x = 0, tmax.x = positive infinity
			// Determine how far we can travel along the ray before we hit a voxel boundary.
			Vector3 tMax = new Vector3(
				(cellBoundary.X - origin.X) / dir.X,    // Boundary is a plane on the YZ axis.
				(cellBoundary.Y - origin.Y) / dir.Y,    // Boundary is a plane on the XZ axis.
				(cellBoundary.Z - origin.Z) / dir.Z );  // Boundary is a plane on the XY axis.
			if( Single.IsNaN( tMax.X ) || Single.IsInfinity( tMax.X ) ) tMax.X = Single.PositiveInfinity;
			if( Single.IsNaN( tMax.Y ) || Single.IsInfinity( tMax.Y ) ) tMax.Y = Single.PositiveInfinity;
			if( Single.IsNaN( tMax.Z ) || Single.IsInfinity( tMax.Z ) ) tMax.Z = Single.PositiveInfinity;

			// Determine how far we must travel along the ray before we have crossed a gridcell.
			Vector3 tDelta = new Vector3( stepX / dir.X, stepY / dir.Y, stepZ / dir.Z );
			if( Single.IsNaN( tDelta.X ) ) tDelta.X = Single.PositiveInfinity;
			if( Single.IsNaN( tDelta.Y ) ) tDelta.Y = Single.PositiveInfinity;
			if( Single.IsNaN( tDelta.Z ) ) tDelta.Z = Single.PositiveInfinity;
			
			Map map = window.Map;
			BlockInfo info = window.BlockInfo;
			float reachSquared = reach * reach;
			int iterations = 0;

			// For each step, determine which distance to the next voxel boundary is lowest (i.e.
			// which voxel boundary is nearest) and walk that way.
			while( iterations < 10000 ) {
				byte block = GetBlock( map, x, y, z, origin );
				Vector3 min = new Vector3( x, y, z ) + info.MinBB[block];
				Vector3 max = new Vector3( x, y, z ) + info.MaxBB[block];
				
				float dx = Math.Min( Math.Abs( origin.X - min.X ), Math.Abs( origin.X - max.X ) );
				float dy = Math.Min( Math.Abs( origin.Y - min.Y ), Math.Abs( origin.Y - max.Y ) );
				float dz = Math.Min( Math.Abs( origin.Z - min.Z ), Math.Abs( origin.Z - max.Z ) );
				
				if( dx * dx + dy * dy + dz * dz > reachSquared ) {
					pickedPos.SetAsInvalid();
					return;
				}
				
				if( window.CanPick( block ) ) {
					// This cell falls on the path of the ray. Now perform an additional bounding box test,
					// since some blocks do not occupy a whole cell.				
					float t0, t1;
					if( Intersection.RayIntersectsBox( origin, dir, min, max, out t0, out t1 ) ) {
						Vector3 intersect = origin + dir * t0;
						pickedPos.SetAsValid( x, y, z, min, max, block, intersect );
						return;
					}
				}
				
				if( tMax.X < tMax.Y && tMax.X < tMax.Z ) {
					// tMax.X is the lowest, an YZ cell boundary plane is nearest.
					x += stepX;
					tMax.X += tDelta.X;
				} else if( tMax.Y < tMax.Z ) {
					// tMax.Y is the lowest, an XZ cell boundary plane is nearest.
					y += stepY;
					tMax.Y += tDelta.Y;
				} else {
					// tMax.Z is the lowest, an XY cell boundary plane is nearest.
					z += stepZ;
					tMax.Z += tDelta.Z;
				}
				iterations++;
			}
			throw new InvalidOperationException( "did over 10000 iterations in GetPickedBlockPos(). " +
			                                    "Something has gone wrong. (dir: " + dir + ")" );
		}
		
		static byte GetBlock( Map map, int x, int y, int z, Vector3 origin ) {
			if( x >= 0 && z >= 0 && x < map.Width && z < map.Length ) {
				if( y >= map.Height ) return 0;
				if( y >= 0 ) return map.GetBlock( x, y, z );
				
				// special case: we want to be able to pick bedrock when we're standing on top of it
				if( origin.Y >= 0 && y == -1 )
					return (byte)Block.Bedrock;
			}
			return 0;
		}
	}
}