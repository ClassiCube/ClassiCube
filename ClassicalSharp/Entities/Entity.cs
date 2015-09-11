using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp {
	
	public abstract partial class Entity {
		
		public Entity( Game game ) {
			map = game.Map;
			info = game.BlockInfo;
		}
		
		public IModel Model;
		public string ModelName;
		public byte ID;
		
		public Vector3 Position;
		public Vector3 Velocity;
		public float YawDegrees, PitchDegrees;
		protected float StepSize;

		const float deg2Rad = (float)( Math.PI / 180 );
		const float rad2Deg = (float)( 180 / Math.PI );
		public float YawRadians {
			get { return YawDegrees * deg2Rad; }
			set { YawDegrees = value * rad2Deg; }
		}
		
		public float PitchRadians {
			get { return PitchDegrees * deg2Rad; }
			set { PitchDegrees = value * rad2Deg; }
		}
		
		public virtual Vector3 CollisionSize {
			get { return new Vector3( 8/16f, 28.5f/16f, 8/16f );
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
}