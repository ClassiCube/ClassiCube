// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Represents a 3D integer vector. </summary>
	public struct Vector3I {
		public static Vector3I MinusOne = new Vector3I(-1, -1, -1);
		
		public int X, Y, Z;
		
		public Vector3I(int x, int y, int z) {
			X = x; Y = y; Z = z;
		}
		
		public Vector3I(int value) {
			X = value; Y = value; Z = value;
		}
		
		public static explicit operator Vector3(Vector3I value) {
			return new Vector3(value.X, value.Y, value.Z);
		}
		
		public override bool Equals(object obj) {
			return obj is Vector3I && Equals((Vector3I)obj);
		}
		
		public bool Equals(Vector3I other) {
			return X == other.X && Y == other.Y && Z == other.Z;
		}
		
		public override int GetHashCode() {
			int hashCode = 1000000007 * X;
			hashCode += 1000000009 * Y;
			hashCode += 1000000021 * Z;
			return hashCode;
		}
		
		public static bool operator == (Vector3I lhs, Vector3I rhs) {
			return lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z;
		}
		
		public static bool operator != (Vector3I lhs, Vector3I rhs) {
			return !(lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z);
		}
		
		public static Vector3I Floor(Vector3 value) {
			return new Vector3I(Utils.Floor(value.X), Utils.Floor(value.Y), Utils.Floor(value.Z));
		}
		
		public static Vector3I Min(Vector3I p1, Vector3I p2) {
			return new Vector3I(Math.Min(p1.X, p2.X), Math.Min(p1.Y, p2.Y), Math.Min(p1.Z, p2.Z));
		}
		
		public static Vector3I Max(Vector3I p1, Vector3I p2) {
			return new Vector3I(Math.Max(p1.X, p2.X), Math.Max(p1.Y, p2.Y), Math.Max(p1.Z, p2.Z));
		}
		
		public override string ToString() {
			return X + "," + Y + "," + Z;
		}
	}
}