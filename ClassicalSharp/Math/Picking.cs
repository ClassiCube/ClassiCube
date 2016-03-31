// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp {

	public static class Picking {
		
		static RayTracer tracer = new RayTracer();
		
		/// <summary> Determines the picked block based on the given origin and direction vector.<br/>
		/// Marks pickedPos as invalid if a block could not be found due to going outside map boundaries
		/// or not being able to find a suitable candiate within the given reach distance. </summary>
		public static void CalculatePickedBlock( Game game, Vector3 origin, Vector3 dir, float reach, PickedPos pickedPos ) {
			tracer.SetRayData( origin, dir );			
			World map = game.World;
			BlockInfo info = game.BlockInfo;
			float reachSquared = reach * reach;
			int iterations = 0;

			while( iterations < 10000 ) {
				int x = tracer.X, y = tracer.Y, z = tracer.Z;
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
				tracer.Step();
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
			tracer.SetRayData( origin, dir );			
			World map = game.World;
			BlockInfo info = game.BlockInfo;
			float reachSquared = reach * reach;
			int iterations = 0;

			while( iterations < 10000 ) {
				int x = tracer.X, y = tracer.Y, z = tracer.Z;
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
				
				if( info.Collide[block] == CollideType.Solid && !info.IsAir[block] ) {
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
				tracer.Step();
				iterations++;
			}
			throw new InvalidOperationException( "did over 10000 iterations in ClipCameraPos(). " +
			                                    "Something has gone wrong. (dir: " + dir + ")" );
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