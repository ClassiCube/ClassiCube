using System;
using OpenTK;

namespace ClassicalSharp {

	/// <summary> Stores data that describes either a relative position, 
	/// full position, or an orientation update for an entity. </summary>
	public struct LocationUpdate {
		
		public Vector3 Pos;
		public float Yaw, Pitch;
		
		public bool IncludesPosition;
		public bool RelativePosition;
		public bool IncludesOrientation;
		
		public LocationUpdate( float x, float y, float z, float yaw, float pitch,
		                      bool incPos, bool relPos, bool incOri ) {
			Pos = new Vector3( x, y, z );
			Yaw = yaw % 360;
			if( Yaw < 0 ) Yaw += 360;
			Pitch = pitch % 360;
			if( Pitch < 0 ) Pitch += 360;
			IncludesPosition = incPos;
			RelativePosition = relPos;
			IncludesOrientation = incOri;
		}
		
		public static LocationUpdate MakeOri( float yaw, float pitch ) {
			return new LocationUpdate( 0, 0, 0, yaw, pitch, false, false, true );
		}
		
		public static LocationUpdate MakePos( float x, float y, float z, bool rel ) {
			return new LocationUpdate( x, y, z, 0, 0, true, rel, false );
		}
		
		public static LocationUpdate MakePos( Vector3 pos, bool rel ) {
			return new LocationUpdate( pos.X, pos.Y, pos.Z, 0, 0, true, rel, false );
		}
		
		public static LocationUpdate MakePosAndOri( float x, float y, float z, float yaw, float pitch, bool rel ) {
			return new LocationUpdate( x, y, z, yaw, pitch, true, rel, true );
		}
	}
}