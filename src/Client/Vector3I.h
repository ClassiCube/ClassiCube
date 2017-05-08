#ifndef CS_VECTOR3I_H
#define CS_VECTOR3I_H
#include "Typedefs.h"
/* Represents 3 dimensional integer vector.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct Vector3I { Int32 X, Y, Z; } Vector3I;

/* Constructs a 3D vector with X, Y and Z set to given value. */
Vector3I Vector3I_Create1(Int32 value);

/* Constructs a 3D vector from the given coordinates. */
Vector3I Vector3I_Create3(Int32 x, Int32 y, Int32 z);

/* Returns the length of the given vector. */
Real32 Vector3I_Length(Vector3I* v);

/* Returns the squared length of the given vector. */
Int32 Vector3I_LengthSquared(Vector3I* v);


#define Vector3I_UnitX Vector3I_Create3(1, 0, 0)
#define Vector3I_UnitY Vector3I_Create3(0, 1, 0)
#define Vector3I_UnitZ Vector3I_Create3(0, 0, 1)
#define Vector3I_Zero Vector3I_Create3(0, 0, 0)
#define Vector3I_One Vector3I_Create3(1, 1, 1)
#define Vector3I_MinusOne Vector3I_Create3(-1, -1, -1)




			public Vector3 Normalise() {
			float len = Length;
			return new Vector3(X / len, Y / len, Z / len);
		}

		public static Vector3I operator * (Vector3I left, int right) {
			return new Vector3I(left.X * right, left.Y * right, left.Z * right);
		}

		public static Vector3I operator + (Vector3I left, Vector3I right) {
			return new Vector3I(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
		}

		public static Vector3I operator - (Vector3I left, Vector3I right) {
			return new Vector3I(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
		}

		public static Vector3I operator - (Vector3I left) {
			return new Vector3I(-left.X, -left.Y, -left.Z);
		}

		public static explicit operator Vector3I(Vector3 value) {
			return Truncate(value);
		}

		public static explicit operator Vector3(Vector3I value) {
			return new Vector3(value.X, value.Y, value.Z);
		}

		public bool Equals(Vector3I other) {
			return X == other.X && Y == other.Y && Z == other.Z;
		}

		public static Vector3I Truncate(Vector3 vector) {
			int x = (int)vector.X;
			int y = (int)vector.Y;
			int z = (int)vector.Z;
			return new Vector3I(x, y, z);
		}

		public static Vector3I Floor(Vector3 value) {
			return new Vector3I(Utils.Floor(value.X), Utils.Floor(value.Y), Utils.Floor(value.Z));
		}

		public static Vector3I Floor(float x, float y, float z) {
			return new Vector3I(Utils.Floor(x), Utils.Floor(y), Utils.Floor(z));
		}

		public static Vector3I Min(Vector3I p1, Vector3I p2) {
			return new Vector3I(Math.Min(p1.X, p2.X), Math.Min(p1.Y, p2.Y), Math.Min(p1.Z, p2.Z));
		}

		public static Vector3I Min(int x1, int y1, int z1, int x2, int y2, int z2) {
			return new Vector3I(Math.Min(x1, x2), Math.Min(y1, y2), Math.Min(z1, z2));
		}

		public static Vector3I Max(Vector3I p1, Vector3I p2) {
			return new Vector3I(Math.Max(p1.X, p2.X), Math.Max(p1.Y, p2.Y), Math.Max(p1.Z, p2.Z));
		}

		public static Vector3I Max(int x1, int y1, int z1, int x2, int y2, int z2) {
			return new Vector3I(Math.Max(x1, x2), Math.Max(y1, y2), Math.Max(z1, z2));
		}
	}
#endif