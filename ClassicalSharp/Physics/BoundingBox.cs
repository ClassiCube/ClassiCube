using System;
using OpenTK;

namespace ClassicalSharp {
	
	public struct BoundingBox {
		
		public Vector3 Min;
		public Vector3 Max;
		
		public BoundingBox( float x1, float y1, float z1, float x2, float y2, float z2 ) {
			Min = new Vector3( x1, y1, z1 );
			Max = new Vector3( x2, y2, z2 );
		}

		public BoundingBox( Vector3 min, Vector3 max ) {
			Min = min;
			Max = max;
		}
		
		public BoundingBox Offset( Vector3 amount ) {
			return new BoundingBox( Min + amount, Max + amount );
		}

		public bool Intersects( BoundingBox other ) {
			if( Max.X >= other.Min.X && Min.X <= other.Max.X ) {
				if( Max.Y < other.Min.Y || Min.Y > other.Max.Y ) {
					return false;
				}
				return Max.Z >= other.Min.Z && Min.Z <= other.Max.Z;
			}
			return false;
		}
		
		public bool Contains( BoundingBox other ) {
			return other.Min.X >= Min.X && other.Min.Y >= Min.Y && other.Min.Z >= Min.Z &&
				other.Max.X <= Max.X && other.Max.Y <= Max.Y && other.Max.Z <= Max.Z;
		}
		
		public bool XIntersects( BoundingBox box ) {
			return Max.X >= box.Min.X && Min.X <= box.Max.X;
		}
		
		public bool YIntersects( BoundingBox box ) {
			return Max.Y >= box.Min.Y && Min.Y <= box.Max.Y;
		}
		
		public bool ZIntersects( BoundingBox box ) {
			return Max.Z >= box.Min.Z && Min.Z <= box.Max.Z;
		}
		
		public override string ToString() {
			return Min + " : " + Max;
		}
	}
}