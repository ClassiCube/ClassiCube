// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp {

	public static class Picking {
		
		// http://www.xnawiki.com/index.php/Voxel_traversal
		// https://web.archive.org/web/20120113051728/http://www.xnawiki.com/index.php?title=Voxel_traversal
		/// <summary> Determines the picked block based on the given origin and direction vector.<br/>
		/// Marks pickedPos as invalid if a block could not be found due to going outside map boundaries
		/// or not being able to find a suitable candiate within the given reach distance. </summary>
		public static void CalculatePickedBlock( Game game, Vector3 origin, Vector3 dir, float reach, PickedPos pickedPos ) {
			// Implementation based on: "A Fast Voxel Traversal Algorithm for Ray Tracing"
			// John Amanatides, Andrew Woo
			// http://www.cse.yorku.ca/~amana/research/grid.pdf
			// http://www.devmaster.net/articles/raytracing_series/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf

			// The cell in which the ray starts.
			Vector3I start = Vector3I.Floor( origin ); // Rounds the position's X, Y and Z down to the nearest integer values.
			int x = start.X, y = start.Y, z = start.Z;
			Vector3I step, cellBoundary;
			Vector3 tMax, tDelta;
			CalcVectors( origin, dir, out step, out cellBoundary, out tMax, out tDelta );
			
			World map = game.World;
			BlockInfo info = game.BlockInfo;
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
					pickedPos.SetAsInvalid(); return;
				}
				
				if( game.CanPick( block ) ) {
					// This cell falls on the path of the ray. Now perform an additional bounding box test,
					// since some blocks do not occupy a whole cell.
					float t0, t1;
					if( Intersection.RayIntersectsBox( origin, dir, min, max, out t0, out t1 ) ) {
						Vector3 intersect = origin + dir * t0;
						pickedPos.SetAsValid( x, y, z, min, max, block, intersect );
						return;
					}
				}
				Step( ref tMax, ref tDelta, ref step, ref x, ref y, ref z );
				iterations++;
			}
			throw new InvalidOperationException( "did over 10000 iterations in CalculatePickedBlock(). " +
			                                    "Something has gone wrong. (dir: " + dir + ")" );
		}
		
		public static void ClipCameraPos( Game game, Vector3 origin, Vector3 dir, float reach, PickedPos pickedPos ) {
			if( !game.CameraClipping ) {
				pickedPos.SetAsInvalid();
				pickedPos.IntersectPoint = origin + dir * reach;
				return;
			}
			Vector3I start = Vector3I.Floor( origin );
			int x = start.X, y = start.Y, z = start.Z;
			Vector3I step, cellBoundary;
			Vector3 tMax, tDelta;
			CalcVectors( origin, dir, out step, out cellBoundary, out tMax, out tDelta );
			
			World map = game.World;
			BlockInfo info = game.BlockInfo;
			float reachSquared = reach * reach;
			int iterations = 0;

			while( iterations < 10000 ) {
				byte block = GetBlock( map, x, y, z, origin );
				Vector3 min = new Vector3( x, y, z ) + info.MinBB[block];
				Vector3 max = new Vector3( x, y, z ) + info.MaxBB[block];
				
				float dx = Math.Min( Math.Abs( origin.X - min.X ), Math.Abs( origin.X - max.X ) );
				float dy = Math.Min( Math.Abs( origin.Y - min.Y ), Math.Abs( origin.Y - max.Y ) );
				float dz = Math.Min( Math.Abs( origin.Z - min.Z ), Math.Abs( origin.Z - max.Z ) );
				
				if( dx * dx + dy * dy + dz * dz > reachSquared ) {
					pickedPos.SetAsInvalid();
					pickedPos.IntersectPoint = origin + dir * reach;
					return;
				}
				
				if( info.CollideType[block] == BlockCollideType.Solid && !info.IsAir[block] ) {
					float t0, t1;
					const float adjust = 0.1f;
					if( Intersection.RayIntersectsBox( origin, dir, min, max, out t0, out t1 ) ) {
						Vector3 intersect = origin + dir * t0;
						pickedPos.SetAsValid( x, y, z, min, max, block, intersect );
						
						switch( pickedPos.BlockFace) {
							case CpeBlockFace.XMin:
								pickedPos.IntersectPoint.X -= adjust; break;
							case CpeBlockFace.XMax:
								pickedPos.IntersectPoint.X += adjust; break;
							case CpeBlockFace.YMin:
								pickedPos.IntersectPoint.Y -= adjust; break;
							case CpeBlockFace.YMax:
								pickedPos.IntersectPoint.Y += adjust; break;
							case CpeBlockFace.ZMin:
								pickedPos.IntersectPoint.Z -= adjust; break;
							case CpeBlockFace.ZMax:
								pickedPos.IntersectPoint.Z += adjust; break;
						}
						return;
					}
				}
				Step( ref tMax, ref tDelta, ref step, ref x, ref y, ref z );
				iterations++;
			}
			throw new InvalidOperationException( "did over 10000 iterations in ClipCameraPos(). " +
			                                    "Something has gone wrong. (dir: " + dir + ")" );
		}
		
		static void CalcVectors( Vector3 origin, Vector3 dir, out Vector3I step,
		                        out Vector3I cellBoundary, out Vector3 tMax, out Vector3 tDelta ) {
			Vector3I start = Vector3I.Floor( origin );
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
		
		static void Step( ref Vector3 tMax, ref Vector3 tDelta, ref Vector3I step, ref int x, ref int y, ref int z ) {
			if( tMax.X < tMax.Y && tMax.X < tMax.Z ) {
				// tMax.X is the lowest, an YZ cell boundary plane is nearest.
				x += step.X;
				tMax.X += tDelta.X;
			} else if( tMax.Y < tMax.Z ) {
				// tMax.Y is the lowest, an XZ cell boundary plane is nearest.
				y += step.Y;
				tMax.Y += tDelta.Y;
			} else {
				// tMax.Z is the lowest, an XY cell boundary plane is nearest.
				z += step.Z;
				tMax.Z += tDelta.Z;
			}
		}
		
		const byte border = (byte)Block.Bedrock;
		static byte GetBlock( World map, int x, int y, int z, Vector3 origin ) {
			bool sides = map.SidesBlock != Block.Air;
			int height = Math.Max( 1, map.SidesHeight );
			bool insideMap = map.IsValidPos( Vector3I.Floor( origin ) );
			
			// handling of blocks inside the map, above, and on borders
			if( x >= 0 && z >= 0 && x < map.Width && z < map.Length ) {
				if( y >= map.Height ) return 0;				
				if( sides && y == -1 && insideMap ) return border;
				if( sides && y == 0 && origin.Y < 0 ) return border;
				
				if( sides && x == 0 && y >= 0 && y < height && origin.X < 0 ) return border;
				if( sides && z == 0 && y >= 0 && y < height && origin.Z < 0 ) return border;
				if( sides && x == (map.Width - 1) && y >= 0 && y < height && origin.X >= map.Width ) 
					return border;
				if( sides && z == (map.Length - 1) && y >= 0 && y < height && origin.Z >= map.Length ) 
					return border;
				if( y >= 0 ) return map.GetBlock( x, y, z );
			}
			
			// pick blocks on the map boundaries (when inside the map)
			if( !sides || !insideMap ) return 0;
			if( y == 0 && origin.Y < 0 ) return border;	
			bool validX = (x == -1 || x == map.Width) && (z >= 0 && z < map.Length);
			bool validZ = (z == -1 || z == map.Length) && (x >= 0 && x < map.Width);
			if( y >= 0 && y < height && (validX || validZ) ) return border;
			return 0;
		}
	}
}