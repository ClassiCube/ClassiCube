using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp {
	
	public abstract partial class Entity {
		
		public Vector3 Position;
		public Vector3 Velocity;
		public float YawDegrees, PitchDegrees;
		public IModel Model;
		
		public float YawRadians {
			get { return (float)Utils.DegreesToRadians( YawDegrees ); }
			set { YawDegrees = (float)Utils.RadiansToDegrees( value ); }
		}
		
		public float PitchRadians {
			get { return (float)Utils.DegreesToRadians( PitchDegrees ); }
			set { PitchDegrees = (float)Utils.RadiansToDegrees( value ); }
		}
		
		public Entity( Game window ) {
			map = window.Map;
			info = window.BlockInfo;
		}
		
		public abstract float StepSize { get; }
		public virtual Vector3 CollisionSize {
			get { return new Vector3( 8 / 16f, 30 / 16f, 8 / 16f );
				//Model.CollisionSize; TODO: for non humanoid models, we also need to offset eye position.
			}
		}
		
		public virtual BoundingBox CollisionBounds {
			get {
				Vector3 pos = Position;
				Vector3 size = Model.CollisionSize;
				return new BoundingBox( pos.X - size.X / 2, pos.Y, pos.Z - size.Z / 2,
				                       pos.X + size.X / 2, pos.Y + size.Y, pos.Z + size.Z / 2 );
			}
		}
		
		public virtual BoundingBox PickingBounds {
			get {
				BoundingBox bb = Model.PickingBounds;
				float angle = YawRadians;
				// TODO: This would be a lot simpler and more accurate if we just did ray-oobb intersection.
				Vector3 x1z1 = Utils.RotateY( bb.Min.X, 0, bb.Min.Z, angle );
				Vector3 x1z2 = Utils.RotateY( bb.Min.X, 0, bb.Max.Z, angle );
				Vector3 x2z1 = Utils.RotateY( bb.Max.X, 0, bb.Min.Z, angle );
				Vector3 x2z2 = Utils.RotateY( bb.Max.X, 0, bb.Max.Z, angle );
				float minX = Math.Min( x1z1.X, Math.Min( x1z2.X, Math.Min( x2z2.X, x2z1.X ) ) );
				float maxX = Math.Max( x1z1.X, Math.Max( x1z2.X, Math.Max( x2z2.X, x2z1.X ) ) );
				float minZ = Math.Min( x1z1.Z, Math.Min( x1z2.Z, Math.Min( x2z2.Z, x2z1.Z ) ) );
				float maxZ = Math.Max( x1z1.Z, Math.Max( x1z2.Z, Math.Max( x2z2.Z, x2z1.Z ) ) );
				return new BoundingBox( minX, bb.Min.Y, minZ, maxX, bb.Max.Y, maxZ ).Offset( Position );
			}
		}
		
		public abstract void Despawn();
		
		public abstract void Render( double deltaTime, float t );
		
		public bool TouchesAnyLava() {
			return TouchesAny( b => b == (byte)Block.Lava || b == (byte)Block.StillLava );
		}
		
		public bool TouchesAnyRope() {
			return TouchesAny( b => b == (byte)Block.Rope );
		}
		
		public bool TouchesAnyWater() {
			return TouchesAny( b => b == (byte)Block.Water || b == (byte)Block.StillWater );
		}

		public bool TouchesAnyOf( byte blockType ) {
			return TouchesAny( b => b == blockType );
		}
		
		public bool TouchesAny( Predicate<byte> condition ) {
			BoundingBox bounds = CollisionBounds;
			Vector3I bbMin = Vector3I.Floor( bounds.Min );
			Vector3I bbMax = Vector3I.Floor( bounds.Max );
			
			for( int x = bbMin.X; x <= bbMax.X; x++ ) {
				for( int y = bbMin.Y; y <= bbMax.Y; y++ ) {
					for( int z = bbMin.Z; z <= bbMax.Z; z++ ) {
						if( !map.IsValidPos( x, y, z ) ) continue;
						byte block = map.GetBlock( x, y, z );
						if( condition( block ) ) {
							float blockHeight = info.BlockHeight( block );
							Vector3 min = new Vector3( x, y, z );
							Vector3 max = new Vector3( x + 1, y + blockHeight, z + 1 );
							BoundingBox blockBB = new BoundingBox( min, max );
							if( blockBB.Intersects( bounds ) ) return true;
						}
					}
				}
			}
			return false;
		}
	}

	public struct LocationUpdate {
		
		public Vector3 Pos;
		public float Yaw, Pitch;
		
		public bool IncludesPosition;
		public bool RelativePosition;
		public bool IncludesOrientation;
		
		public LocationUpdate( float x, float y, float z, float yaw, float pitch,
		                      bool incPos, bool relPos, bool incOri ) {
			Pos = new Vector3( x, y, z );
			Yaw = yaw;
			Pitch = pitch;
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