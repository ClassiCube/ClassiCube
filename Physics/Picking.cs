using System;
using System.Collections.Generic;
using OpenTK;

namespace ClassicalSharp {

	public static class Picking {
		
		public static PickedPos GetPickedBlockPos( Game window, Vector3 origin, Vector3 dir, float reach ) {
			Map map = window.Map;
			BlockInfo info = window.BlockInfo;
			float reachSquared = reach * reach;
			
			int iterations = 0;
			foreach( Vector3I cell in GetCellsOnRay( origin, dir ) ) {
				Vector3 pos = new Vector3( cell.X, cell.Y, cell.Z );
				if( Utils.DistanceSquared( pos, origin ) >= reachSquared ) return null;
				iterations++;

				if( iterations > 10000 ) {
					throw new InvalidOperationException(
						"did over 10000 iterations in Picking.GetPickedBlockPos(). " +
						"Something has gone wrong. (dir: " + dir + ")" );
				}
				
				int x = cell.X, y = cell.Y, z = cell.Z;
				byte block;
				if( map.IsValidPos( x, y, z ) && info.CanPick( block = map.GetBlock( x, y, z ) ) ) {
					window.Title = "Looking at type: " + block + "," + window.MapRenderer.sectionsCacheCount;
					float height = 1;
					height = window.BlockInfo.BlockHeight( block );
					Vector3 min = new Vector3( x, y, z );
					Vector3 max = new Vector3( x + 1, y + height, z + 1 );
					if( IntersectionUtils.RayIntersectsBox( origin, dir, min, max ) ) {
						return new PickedPos( min, max, origin, dir );
					}
				}
			}
			return null;
		}
		
		// http://www.xnawiki.com/index.php/Voxel_traversal
		public static IEnumerable<Vector3I> GetCellsOnRay( Vector3 origin, Vector3 dir ) {
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

			// For each step, determine which distance to the next voxel boundary is lowest (i.e.
			// which voxel boundary is nearest) and walk that way.
			while( true ) {
				yield return new Vector3I( x, y, z );
				
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
			}
		}
	}
	
	public class PickedPos {
		
		public Vector3 Min, Max;
		public Vector3I BlockPos;
		public Vector3I? TranslatedPos;
		public byte Direction = 0xFF;
		
		struct QuadIntersection {
			public Quad Quad;
			public Vector3 Intersection;
			
			public QuadIntersection( Quad quad, Vector3 intersection ) {
				Quad = quad;
				Intersection = intersection;
			}
		}
		
		public PickedPos( Vector3 p1, Vector3 p2, Vector3 origin, Vector3 dir ) {
			Min = Vector3.Min( p1, p2 );
			Max = Vector3.Max( p1, p2 );
			BlockPos = Vector3I.Truncate( Min );
			
			Quad? closestQuad = null;
			Vector3 closest = new Vector3( float.MaxValue, float.MaxValue, float.MaxValue );
			IEnumerable<Quad> faces = IntersectionUtils.GetFaces( Min, Max );
			foreach( QuadIntersection result in FindIntersectingTriangles( origin, dir, faces ) ) {
				Vector3 I = result.Intersection;
				if( ( origin - I ).LengthSquared < ( origin - closest ).LengthSquared ) {
					closest = I;
					closestQuad = result.Quad;
				}
			}
			if( closestQuad != null ) {
				TranslatedPos = Vector3I.Truncate( Min + closestQuad.Value.Normal );
				Vector3 intersect = origin + dir * ( origin - closest ).Length;
				Direction = GetDirection( closestQuad.Value.Normal );
			}
		}
		
		byte GetDirection( Vector3 norm ) {
			if( norm == -Vector3.UnitY ) return 0;
			if( norm == Vector3.UnitY ) return 1;
			if( norm == -Vector3.UnitZ ) return 2;
			if( norm == Vector3.UnitZ ) return 3;
			if( norm == -Vector3.UnitX ) return 4;
			if( norm == Vector3.UnitX ) return 5;
			return 0xFF;
		}
		
		
		static IEnumerable<QuadIntersection> FindIntersectingTriangles( Vector3 origin, Vector3 dir, IEnumerable<Quad> quads ) {
			foreach( Quad quad in quads ) {
				Vector3 p0 = quad.Pos1; // assumed to be min point
				Vector3 p2 = quad.Pos3; // assumed to be max point
				Vector3 I;
				
				Vector3 p1 = quad.Pos2; // triangle 1
				if( IntersectionUtils.RayTriangleIntersect( origin, dir, p0, p1, p2, out I ) ) {
					yield return new QuadIntersection( quad, I );
				}

				p1 = quad.Pos4; // triangle 2
				if( IntersectionUtils.RayTriangleIntersect( origin, dir, p0, p1, p2, out I ) ) {
					yield return new QuadIntersection( quad, I );
				}
			}
		}
	}
}