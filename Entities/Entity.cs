using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Window;
using OpenTK;

namespace ClassicalSharp {
	
	public abstract partial class Entity {
		
		public Vector3 Position;
		public Vector3 Velocity;
		public float YawDegrees, PitchDegrees;
		
		public int EntityId;
		protected Game game;
		
		public abstract void Tick( double delta );
		
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
			this.game = window;
		}
		
		public abstract float StepSize { get; }
		public abstract Vector3 Size { get; }
		
		public virtual BoundingBox Bounds {
			get {
				Vector3 pos = Position;
				Vector3 size = Size;
				return new BoundingBox( pos.X - size.X / 2, pos.Y, pos.Z - size.Z / 2,
				                       pos.X + size.X / 2, pos.Y + size.Y, pos.Z + size.Z / 2 );
			}
		}
		
		public virtual void SetEquipmentSlot( short slotId, Slot slot ) {
		}
		
		public virtual void SetStatus( byte status ) {
		}
		
		public virtual void SetMetadata( EntityMetadata meta ) {			
		}
		
		public virtual void DoAnimation( byte anim ) {
		}
		
		public abstract void SetLocation( LocationUpdate update, bool interpolate );
		
		public abstract void Despawn();
		
		public abstract void Render( double deltaTime, float t );
		
		public bool TouchesAnyLava() {
			return TouchesAny( b => b == (byte)Block.Lava || b == (byte)Block.StillLava );
		}
		
		public bool TouchesAnyRope() {
			//return TouchesAny( b => b == (byte)Block.Rope );
			return false;
		}
		
		public bool TouchesAnyWater() {
			return TouchesAny( b => b == (byte)Block.Water || b == (byte)Block.StillWater );
		}

		public bool TouchesAnyOf( byte blockType ) {			
			return TouchesAny( b => b == blockType );
		}
		
		public bool TouchesAny( Predicate<byte> condition ) {			
			BoundingBox bounds = Bounds;
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