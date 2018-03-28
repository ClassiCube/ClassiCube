// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Physics {
	
	public struct AABB {
		
		public Vector3 Min;
		public Vector3 Max;
		
		public float Width  { get { return Max.X - Min.X; } }
		public float Height { get { return Max.Y - Min.Y; } }
		public float Length { get { return Max.Z - Min.Z; } }
		
		public AABB(float x1, float y1, float z1, float x2, float y2, float z2) {
			Min.X = x1; Min.Y = y1; Min.Z = z1;
			Max.X = x2; Max.Y = y2; Max.Z = z2;
		}

		public AABB(Vector3 min, Vector3 max) {
			Min = min;
			Max = max;
		}
		
		public static AABB Make(Vector3 pos, Vector3 size) {
			return new AABB(pos.X - size.X / 2, pos.Y, pos.Z - size.Z / 2,
			                pos.X + size.X / 2, pos.Y + size.Y, pos.Z + size.Z / 2);
		}
		
		/// <summary> Returns a new bounding box, with the minimum and maximum coordinates
		/// of the original bounding box translated by the given vector. </summary>
		public AABB Offset(Vector3 amount) {
			return new AABB(Min + amount, Max + amount);
		}

		/// <summary> Determines whether this bounding box intersects
		/// the given bounding box on any axes. </summary>
		public bool Intersects(AABB other) {
			if (Max.X >= other.Min.X && Min.X <= other.Max.X) {
				if (Max.Y < other.Min.Y || Min.Y > other.Max.Y) {
					return false;
				}
				return Max.Z >= other.Min.Z && Min.Z <= other.Max.Z;
			}
			return false;
		}
		
		/// <summary> Determines whether this bounding box entirely contains
		/// the given bounding box on all axes. </summary>
		public bool Contains(AABB other) {
			return other.Min.X >= Min.X && other.Min.Y >= Min.Y && other.Min.Z >= Min.Z &&
				other.Max.X <= Max.X && other.Max.Y <= Max.Y && other.Max.Z <= Max.Z;
		}
		
		/// <summary> Determines whether this bounding box entirely contains
		/// the coordinates on all axes. </summary>
		public bool Contains(Vector3 P) {
			return P.X >= Min.X && P.Y >= Min.Y && P.Z >= Min.Z &&
				P.X <= Max.X && P.Y <= Max.Y && P.Z <= Max.Z;
		}
		
		public override string ToString() {
			return Min + " : " + Max;
		}
	}
}