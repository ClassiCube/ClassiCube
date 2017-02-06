// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Entities {

	/// <summary> Stores data that describes either a relative position, 
	/// full position, or an orientation update for an entity. </summary>
	public struct LocationUpdate {
		
		public Vector3 Pos;
		public float RotX, RotY, RotZ, HeadX; // NaN if not included
		
		/// <summary> Whether this update includes an absolute or relative position. </summary>
		public bool IncludesPosition;
		
		/// <summary> Whether the positon is specified as absolute (world coordinates), 
		/// or relative to the last position that was received from the server. </summary>
		public bool RelativePosition;
		
		public LocationUpdate(float x, float y, float z, 
		                      float rotX, float rotY, float rotZ, float headX,
		                      bool incPos, bool relPos) {
			Pos = new Vector3(x, y, z);
			RotX = Clamp(rotX); RotY = Clamp(rotY); RotZ = Clamp(rotZ);
			HeadX = Clamp(headX);
			
			IncludesPosition = incPos;
			RelativePosition = relPos;
		}
		
		public static float Clamp(float degrees) {
			// Make sure angle is in [0, 360)
			degrees = degrees % 360;
			if (degrees < 0) degrees += 360;
			return degrees;
		}
		
		const float NaN = float.NaN;
		public static LocationUpdate Empty() {
			return new LocationUpdate(0, 0, 0, NaN, NaN, NaN, NaN, false, false);
		}
		
		public static LocationUpdate MakeOri(float rotY, float headX) {
			return new LocationUpdate(0, 0, 0, NaN, rotY, NaN, headX, false, false);
		}
		
		public static LocationUpdate MakePos(float x, float y, float z, bool rel) {
			return new LocationUpdate(x, y, z, NaN, NaN, NaN, NaN, true, rel);
		}
		
		public static LocationUpdate MakePos(Vector3 pos, bool rel) {
			return new LocationUpdate(pos.X, pos.Y, pos.Z, NaN, NaN, NaN, NaN, true, rel);
		}
		
		public static LocationUpdate MakePosAndOri(Vector3 v, float rotY, float headX, bool rel) {
			return new LocationUpdate(v.X, v.Y, v.Z, NaN, rotY, NaN, headX, true, rel);
		}
		
		public static LocationUpdate MakePosAndOri(float x, float y, float z, float rotY, float headX, bool rel) {
			return new LocationUpdate(x, y, z, NaN, rotY, NaN, headX, true, rel);
		}
	}
}