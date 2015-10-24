using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp {
	
	public abstract class Entity {
		
		public Entity( Game game ) {
			this.game = game;
			info = game.BlockInfo;
		}
		
		public IModel Model;
		public string ModelName;
		public byte ID;
		
		public Vector3 Position;
		public Vector3 Velocity;
		public float YawDegrees, PitchDegrees;
		protected float StepSize;
		protected Game game;
		protected BlockInfo info;
		
		public float YawRadians {
			get { return YawDegrees * Utils.Deg2Rad; }
			set { YawDegrees = value * Utils.Rad2Deg; }
		}
		
		public float PitchRadians {
			get { return PitchDegrees * Utils.Deg2Rad; }
			set { PitchDegrees = value * Utils.Rad2Deg; }
		}
		
		public virtual Vector3 CollisionSize {
			get { return new Vector3( 8/16f, 28.5f/16f, 8/16f );
				//Model.CollisionSize; TODO: for non humanoid models
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
		
		public bool TouchesAny( Predicate<byte> condition ) {
			return TouchesAny( CollisionBounds, condition );
		}
		
		public bool TouchesAny( BoundingBox bounds, Predicate<byte> condition ) {
			Vector3I bbMin = Vector3I.Floor( bounds.Min );
			Vector3I bbMax = Vector3I.Floor( bounds.Max );
			
			for( int x = bbMin.X; x <= bbMax.X; x++ ) {
				for( int y = bbMin.Y; y <= bbMax.Y; y++ ) {
					for( int z = bbMin.Z; z <= bbMax.Z; z++ ) {
						if( !game.Map.IsValidPos( x, y, z ) ) continue;
						byte block = game.Map.GetBlock( x, y, z );
						if( condition( block ) ) {
							float blockHeight = info.Height[block];
							Vector3 min = new Vector3( x, y, z ) + info.MinBB[block];
							Vector3 max = new Vector3( x, y, z ) + info.MaxBB[block];
							BoundingBox blockBB = new BoundingBox( min, max );
							if( blockBB.Intersects( bounds ) ) return true;
						}
					}
				}
			}
			return false;
		}
		
		public const float Adjustment = 0.001f;		
		protected bool TouchesAnyLava() {
			return TouchesAny( CollisionBounds, 
			                  b => b == (byte)Block.Lava || b == (byte)Block.StillLava );
		}
		
		protected bool TouchesAnyRope() {
			return TouchesAny( CollisionBounds, 
			                  b => b == (byte)Block.Rope );
		}
		
		protected bool TouchesAnyWater() {
			return TouchesAny( CollisionBounds, 
			                  b => b == (byte)Block.Water || b == (byte)Block.StillWater );
		}
	}
}