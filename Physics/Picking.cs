using System;
using System.Collections.Generic;
using OpenTK;

namespace ClassicalSharp {

	public static class Picking {
		
		// http://www.xnawiki.com/index.php/Voxel_traversal
		public static void GetPickedBlockPos( Game window, Vector3 origin, Vector3 dir, float reach, PickedPos pickedPos ) {
			// Implementation is based on:
			// "A Fast Voxel Traversal Algorithm for Ray Tracing"
			// John Amanatides, Andrew Woo
			// http://www.cse.yorku.ca/~amana/research/grid.pdf
			// http://www.devmaster.net/articles/raytracing_series/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf

			// NOTES:
			// * This code assumes that the ray's position and direction are in 'cell coordinates', which means
			//   that one unit equals one cell in all directions.
			// * When the ray doesn't start within the voxel grid, calculate the first position at which the
			//   ray could enter the grid. If it never enters the grid, there is nothing more to do here.
			// * Also, it is important to test when the ray exits the voxel grid when the grid isn't infinite.
			// * The Point3D structure is a simple structure having three integer fields (X, Y and Z).

			// The cell in which the ray starts.
			Vector3I start = Vector3I.Floor( origin ); // Rounds the position's X, Y and Z down to the nearest integer values.
			int x = start.X;
			int y = start.Y;
			int z = start.Z;

			// Determine which way we go.
			int stepX = Math.Sign( dir.X );
			int stepY = Math.Sign( dir.Y );
			int stepZ = Math.Sign( dir.Z );

			// Calculate cell boundaries. When the step (i.e. direction sign) is positive,
			// the next boundary is AFTER our current position, meaning that we have to add 1.
			// Otherwise, it is BEFORE our current position, in which case we add nothing.
			Vector3I cellBoundary = new Vector3I(
				x + ( stepX > 0 ? 1 : 0 ),
				y + ( stepY > 0 ? 1 : 0 ),
				z + ( stepZ > 0 ? 1 : 0 ) );

			// NOTE: For the following calculations, the result will be Single.PositiveInfinity
			// when ray.Direction.X, Y or Z equals zero, which is OK. However, when the left-hand
			// value of the division also equals zero, the result is Single.NaN, which is not OK.

			// Determine how far we can travel along the ray before we hit a voxel boundary.
			Vector3 tMax = new Vector3(
				( cellBoundary.X - origin.X ) / dir.X,    // Boundary is a plane on the YZ axis.
				( cellBoundary.Y - origin.Y ) / dir.Y,    // Boundary is a plane on the XZ axis.
				( cellBoundary.Z - origin.Z ) / dir.Z );  // Boundary is a plane on the XY axis.
			if( Single.IsNaN( tMax.X ) || Single.IsInfinity( tMax.X ) ) tMax.X = Single.PositiveInfinity;
			if( Single.IsNaN( tMax.Y ) || Single.IsInfinity( tMax.Y ) ) tMax.Y = Single.PositiveInfinity;
			if( Single.IsNaN( tMax.Z ) || Single.IsInfinity( tMax.Z ) ) tMax.Z = Single.PositiveInfinity;

			// Determine how far we must travel along the ray before we have crossed a gridcell.
			Vector3 tDelta = new Vector3(
				stepX / dir.X,     // Crossing the width of a cell.
				stepY / dir.Y,     // Crossing the height of a cell.
				stepZ / dir.Z );   // Crossing the depth of a cell.
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
				byte block = map.IsValidPos( x, y, z ) ? map.GetBlock( x, y, z ) : (byte)0;
				Vector3 min = new Vector3( x, y, z );
				Vector3 max = min + new Vector3( 1, block == 0 ? 1 : info.BlockHeight( block ), 1 );
				
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
						pickedPos.SetAsValid( min, max, intersect );
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
	}

	public class PickedPos {
		
		public Vector3 Min, Max;
		public Vector3I BlockPos;
		public Vector3I TranslatedPos;
		public bool Valid = true;
		public CpeBlockFace BlockFace;
		
		public void SetAsValid( Vector3 min, Vector3 max, Vector3 intersect ) {
			Min = min;
			Max = max;
			BlockPos = Vector3I.Truncate( Min );
			Valid = true;
			
			Vector3I normal = Vector3I.Zero;
			float dist = float.PositiveInfinity;
			TestAxis( intersect.X - Min.X, ref dist, -Vector3I.UnitX, ref normal, CpeBlockFace.XMin );
			TestAxis( intersect.X - Max.X, ref dist, Vector3I.UnitX, ref normal, CpeBlockFace.XMax );
			TestAxis( intersect.Y - Min.Y, ref dist, -Vector3I.UnitY, ref normal, CpeBlockFace.YMin );
			TestAxis( intersect.Y - Max.Y, ref dist, Vector3I.UnitY, ref normal, CpeBlockFace.YMax );
			TestAxis( intersect.Z - Min.Z, ref dist, -Vector3I.UnitZ, ref normal, CpeBlockFace.ZMin );
			TestAxis( intersect.Z - Max.Z, ref dist, Vector3I.UnitZ, ref normal, CpeBlockFace.ZMax );
			TranslatedPos = BlockPos + normal;
		}
		
		public void SetAsInvalid() {
			Valid = false;
			BlockPos = TranslatedPos = Vector3I.MinusOne;
			BlockFace = (CpeBlockFace)255;
		}
		
		void TestAxis( float dAxis, ref float dist, Vector3I nAxis, ref Vector3I normal, 
		                     CpeBlockFace fAxis) {
			dAxis = Math.Abs( dAxis );
			if( dAxis < dist ) {
				dist = dAxis;
				normal = nAxis;
				BlockFace = fAxis;
			}
		}
	}
}