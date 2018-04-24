// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Entities {

	public static class LocationUpdateFlag {
		public const byte Pos   = 0x01;
		public const byte HeadX = 0x02;
		public const byte HeadY = 0x04;
		public const byte RotX  = 0x08;
		public const byte RotZ  = 0x10;
	}
	
	public struct LocationUpdate {
		public Vector3 Pos;
		public float HeadX, HeadY, RotX, RotZ;
		public byte Flags;
		/// <summary> True if position is relative to the last position received from server </summary>
		public bool RelativePos;
		
		public static float Clamp(float degrees) {
			// Make sure angle is in [0, 360)
			degrees = degrees % 360;
			if (degrees < 0) degrees += 360;
			return degrees;
		}
		
		const float NaN = float.NaN;
		
		/// <summary> Constructs a location update that does not have any position or orientation information. </summary>
		public static LocationUpdate Empty() {
			return default(LocationUpdate);
		}

		/// <summary> Constructs a location update that only consists of orientation information. </summary>		
		public static LocationUpdate MakeOri(float rotY, float headX) {
			LocationUpdate update = default(LocationUpdate);
			update.Flags = LocationUpdateFlag.HeadX | LocationUpdateFlag.HeadY;
			update.HeadX = Clamp(headX);
			update.HeadY  = Clamp(rotY);
			return update;
		}

		/// <summary> Constructs a location update that only consists of position information. </summary>		
		public static LocationUpdate MakePos(Vector3 pos, bool rel) {
			LocationUpdate update = default(LocationUpdate);
			update.Flags = LocationUpdateFlag.Pos;
			update.Pos   = pos;
			update.RelativePos = rel;
			return update;
		}

		/// <summary> Constructs a location update that consists of position and orientation information. </summary>	
		public static LocationUpdate MakePosAndOri(Vector3 pos, float rotY, float headX, bool rel) {
			LocationUpdate update = default(LocationUpdate);
			update.Flags = LocationUpdateFlag.Pos | LocationUpdateFlag.HeadX | LocationUpdateFlag.HeadY;
			update.HeadX = Clamp(headX);
			update.HeadY = Clamp(rotY);
			update.Pos   = pos;
			update.RelativePos = rel;
			return update;
		}
	}
}