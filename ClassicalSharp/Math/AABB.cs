// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp {
	
	public struct AABB {
		
		public Vector3 Min;
		public Vector3 Max;
		
		public float Width { get { return Max.X - Min.X; } }
		public float Height { get { return Max.Y - Min.Y; } }
		public float Length { get { return Max.Z - Min.Z; } }
		
		public AABB( float x1, float y1, float z1, float x2, float y2, float z2 ) {
			Min = new Vector3( x1, y1, z1 );
			Max = new Vector3( x2, y2, z2 );
		}

		public AABB( Vector3 min, Vector3 max ) {
			Min = min;
			Max = max;
		}
		
		/// <summary> Returns a new bounding box, with the minimum and maximum coordinates 
		/// of the original bounding box translated by the given vector. </summary>
		public AABB Offset( Vector3 amount ) {
			return new AABB( Min + amount, Max + amount );
		}
		
		/// <summary> Returns a new bounding box, with the minimum and maximum coordinates 
		/// of the original bounding box expanded away from origin the given vector. </summary>
		public AABB Expand( Vector3 amount ) {
			return new AABB( Min - amount, Max + amount );
		}
		
		/// <summary> Returns a new bounding box, with the minimum and maximum coordinates 
		/// of the original bounding box scaled away from origin the given value. </summary>
		public AABB Scale( float scale ) {
			return new AABB( Min * scale, Max * scale );
		}

		/// <summary> Determines whether this bounding box intersects 
		/// the given bounding box on any axes. </summary>
		public bool Intersects( AABB other ) {
			if( Max.X >= other.Min.X && Min.X <= other.Max.X ) {
				if( Max.Y < other.Min.Y || Min.Y > other.Max.Y ) {
					return false;
				}
				return Max.Z >= other.Min.Z && Min.Z <= other.Max.Z;
			}
			return false;
		}
		
		/// <summary> Determines whether this bounding box entirely contains 
		/// the given bounding box on all axes. </summary>
		public bool Contains( AABB other ) {
			return other.Min.X >= Min.X && other.Min.Y >= Min.Y && other.Min.Z >= Min.Z &&
				other.Max.X <= Max.X && other.Max.Y <= Max.Y && other.Max.Z <= Max.Z;
		}
		
		/// <summary> Determines whether this bounding box entirely contains 
		/// the coordinates on all axes. </summary>
		public bool Contains( Vector3 P ) {
			return P.X >= Min.X && P.Y >= Min.Y && P.Z >= Min.Z &&
				P.X <= Max.X && P.Y <= Max.Y && P.Z <= Max.Z;
		}
		
		/// <summary> Determines whether this bounding box intersects 
		/// the given bounding box on the X axis. </summary>
		public bool XIntersects( AABB box ) {
			return Max.X >= box.Min.X && Min.X <= box.Max.X;
		}
		
		/// <summary> Determines whether this bounding box intersects 
		/// the given bounding box on the Y axis. </summary>
		public bool YIntersects( AABB box ) {
			return Max.Y >= box.Min.Y && Min.Y <= box.Max.Y;
		}
		
		/// <summary> Determines whether this bounding box intersects 
		/// the given bounding box on the Z axis. </summary>
		public bool ZIntersects( AABB box ) {
			return Max.Z >= box.Min.Z && Min.Z <= box.Max.Z;
		}
		
		public override string ToString() {
			return Min + " : " + Max;
		}
	}
}