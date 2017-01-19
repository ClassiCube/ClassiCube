// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp {
	
	public static partial class Utils {
		
		/// <summary> Creates a vector with all components at 1E25. </summary>
		public static Vector3 MaxPos() { return new Vector3(1E25f, 1E25f, 1E25f); }
		
		/// <summary> Clamps that specified value such that min ≤ value ≤ max </summary>
		public static void Clamp(ref float value, float min, float max) {
			if (value < min) value = min;
			if (value > max) value = max;
		}
		
		/// <summary> Clamps that specified value such that min ≤ value ≤ max </summary>
		public static void Clamp(ref int value, int min, int max) {
			if (value < min) value = min;
			if (value > max) value = max;
		}
		
		public static Vector3 Mul(Vector3 a, Vector3 scale) {
			a.X *= scale.X; a.Y *= scale.Y; a.Z *= scale.Z;
			return a;
		}
		
		/// <summary> Returns the next highest power of 2 that is ≥ to the given value. </summary>
		public static int NextPowerOf2(int value) {
			int next = 1;
			while (value > next)
				next <<= 1;
			return next;
		}
		
		/// <summary> Returns whether the given value is a power of 2. </summary>
		public static bool IsPowerOf2(int value) {
			return value != 0 && (value & (value - 1)) == 0;
		}
		
		/// <summary> Multiply a value in degrees by this to get its value in radians. </summary>
		public const float Deg2Rad = (float)(Math.PI / 180);
		/// <summary> Multiply a value in radians by this to get its value in degrees. </summary>
		public const float Rad2Deg = (float)(180 / Math.PI);
		
		public static int DegreesToPacked(double degrees, int period) {
			return (int)(degrees * period / 360.0) % period;
		}
		
		public static int DegreesToPacked(double degrees) {
			return (int)(degrees * 256 / 360.0) & 0xFF;
		}
		
		public static double PackedToDegrees(byte packed) {
			return packed * 360.0 / 256.0;
		}
		
		/// <summary> Rotates the given 3D coordinates around the y axis. </summary>
		public static Vector3 RotateY(Vector3 v, float angle) {
			float cosA = (float)Math.Cos(angle);
			float sinA = (float)Math.Sin(angle);
			return new Vector3(cosA * v.X - sinA * v.Z, v.Y, sinA * v.X + cosA * v.Z);
		}
		
		/// <summary> Rotates the given 3D coordinates around the y axis. </summary>
		public static Vector3 RotateY(float x, float y, float z, float angle) {
			float cosA = (float)Math.Cos(angle);
			float sinA = (float)Math.Sin(angle);
			return new Vector3(cosA * x - sinA * z, y, sinA * x + cosA * z);
		}
		
		/// <summary> Rotates the given 3D coordinates around the x axis. </summary>
		public static void RotateX(ref float y, ref float z, float cosA, float sinA) {
			float y2 = cosA * y + sinA * z; z = -sinA * y + cosA * z; y = y2;
		}
		
		/// <summary> Rotates the given 3D coordinates around the y axis. </summary>
		public static void RotateY(ref float x, ref float z, float cosA, float sinA) {
			float x2 = cosA * x - sinA * z; z = sinA * x + cosA * z; x = x2;
		}
		
		/// <summary> Rotates the given 3D coordinates around the z axis. </summary>
		public static void RotateZ(ref float x, ref float y, float cosA, float sinA) {
			float x2 = cosA * x + sinA * y; y = -sinA * x + cosA * y; x = x2;
		}
		
		/// <summary> Returns the square of the euclidean distance between two points. </summary>
		public static float DistanceSquared(Vector3 p1, Vector3 p2) {
			float dx = p2.X - p1.X, dy = p2.Y - p1.Y, dz = p2.Z - p1.Z;
			return dx * dx + dy * dy + dz * dz;
		}
		
		/// <summary> Returns the square of the euclidean distance between two points. </summary>
		public static float DistanceSquared(float x1, float y1, float z1, float x2, float y2, float z2) {
			float dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
			return dx * dx + dy * dy + dz * dz;
		}
		
		/// <summary> Returns the square of the euclidean distance between two points. </summary>
		public static int DistanceSquared(int x1, int y1, int z1, int x2, int y2, int z2) {
			int dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
			return dx * dx + dy * dy + dz * dz;
		}
		
		/// <summary> Returns a normalised vector that faces in the direction
		/// described by the given yaw and pitch. </summary>
		public static Vector3 GetDirVector(double yawRad, double pitchRad) {
			double x = -Math.Cos(pitchRad) * -Math.Sin(yawRad);
			double y = -Math.Sin(pitchRad);
			double z = -Math.Cos(pitchRad) * Math.Cos(yawRad);
			return new Vector3((float)x, (float)y, (float)z);
		}
		
		public static void GetHeading(Vector3 dir, out double yawRad, out double pitchRad) {
			pitchRad = Math.Asin(-dir.Y);
			yawRad = Math.Atan2(dir.Z, dir.X);
		}
		
		public static int Floor(float value) {
			int valueI = (int)value;
			return value < valueI ? valueI - 1 : valueI;
		}
		
		/// <summary> Performs rounding upwards integer division. </summary>
		public static int CeilDiv(int a, int b) {
			return a / b + (a % b != 0 ? 1 : 0);
		}
		
		/// <summary> Performs linear interpolation between two values. </summary>
		public static float Lerp(float a, float b, float t) {
			return a + (b - a) * t;
		}
		
		// http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/
		public static void CalcBillboardPoints(Vector2 size, Vector3 position, ref Matrix4 view, out Vector3 p111,
		                                       out Vector3 p121, out Vector3 p212, out Vector3 p222) {
			Vector3 centre = position; centre.Y += size.Y / 2;
			Vector3 a = new Vector3(view.Row0.X * size.X, view.Row1.X * size.X, view.Row2.X * size.X); // right * size.X
			Vector3 b = new Vector3(view.Row0.Y * size.Y, view.Row1.Y * size.Y, view.Row2.Y * size.Y); // up * size.Y
			
			p111 = centre + a * -0.5f + b * -0.5f;
			p121 = centre + a * -0.5f + b *  0.5f;
			p212 = centre + a *  0.5f + b * -0.5f;
			p222 = centre + a *  0.5f + b *  0.5f;
		}	
		
		/// <summary> Linearly interpolates between a given angle range, adjusting if necessary. </summary>
		public static float LerpAngle(float leftAngle, float rightAngle, float t) {
			// we have to cheat a bit for angles here.
			// Consider 350* --> 0*, we only want to travel 10*,
			// but without adjusting for this case, we would interpolate back the whole 350* degrees.
			bool invertLeft = leftAngle > 270 && rightAngle < 90;
			bool invertRight = rightAngle > 270 && leftAngle < 90;
			if (invertLeft) leftAngle = leftAngle - 360;
			if (invertRight) rightAngle = rightAngle - 360;
			
			return Lerp(leftAngle, rightAngle, t);
		}
	}
}