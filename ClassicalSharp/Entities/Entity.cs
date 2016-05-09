// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	/// <summary> Contains a model, along with position, velocity, and rotation.
	/// May also contain other fields and properties. </summary>
	public abstract partial class Entity {
		
		public Entity( Game game ) {
			this.game = game;
			info = game.BlockInfo;
		}
		
		public IModel Model;
		public string ModelName;
		public float ModelScale = 1;
		public byte ID;
		
		public Vector3 Position;
		public Vector3 Velocity;
		public float HeadYawDegrees, YawDegrees, PitchDegrees;
		
		protected Game game;
		protected BlockInfo info;
		protected internal bool onGround;
		internal float StepSize;
		
		/// <summary> Rotation of the entity's head horizontally (i.e. looking north or east) </summary>
		public float HeadYawRadians {
			get { return HeadYawDegrees * Utils.Deg2Rad; }
			set { HeadYawDegrees = value * Utils.Rad2Deg; }
		}
		
		/// <summary> Rotation of the entity's body horizontally (i.e. looking north or east) </summary>
		public float YawRadians {
			get { return YawDegrees * Utils.Deg2Rad; }
			set { YawDegrees = value * Utils.Rad2Deg; }
		}
		
		/// <summary> Rotation of the entity vertically. (i.e. looking up or down) </summary>
		public float PitchRadians {
			get { return PitchDegrees * Utils.Deg2Rad; }
			set { PitchDegrees = value * Utils.Rad2Deg; }
		}
		
		/// <summary> Returns the size of the model that is used for collision detection. </summary>
		public Vector3 CollisionSize {
			get { UpdateModel(); return Model.CollisionSize * ModelScale; }
		}
		
		void UpdateModel() {
			BlockModel model = Model as BlockModel;
			if( model != null )
				model.CalcState( Utils.FastByte( ModelName ) );
		}
		
		public abstract void Tick( double delta );
		
		public abstract void SetLocation( LocationUpdate update, bool interpolate );
		
		public abstract void Despawn();
		
		/// <summary> Renders the entity's model, interpolating between the previous and next state. </summary>
		public abstract void RenderModel( double deltaTime, float t );
		
		/// <summary> Renders the entity's name over the top of its model. </summary>
		/// <remarks> Assumes that RenderModel was previously called this frame. </remarks>
		public abstract void RenderName();
		
		/// <summary> Gets the position of the player's eye in the world. </summary>
		public Vector3 EyePosition {
			get { return new Vector3( Position.X, 
			                         Position.Y + Model.GetEyeY( this ) * ModelScale, Position.Z ); }
		}

		/// <summary> Gets the block just underneath the player's feet position. </summary>
		public Block BlockUnderFeet {
			get { return GetBlock( new Vector3( Position.X, Position.Y - 0.01f, Position.Z ) ); }
		}
		
		/// <summary> Gets the block at player's eye position. </summary>
		public Block BlockAtHead {
			get { return GetBlock( EyePosition ); }
		}
		
		protected Block GetBlock( Vector3 coords ) {
			return (Block)game.World.SafeGetBlock( Vector3I.Floor( coords ) );
		}
	}
}