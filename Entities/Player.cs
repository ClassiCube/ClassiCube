using System;
using OpenTK;
using ClassicalSharp.Renderers;

namespace ClassicalSharp {

	public abstract class Player : Entity {
		
		public const float Width = 0.6f;
		public const float EyeHeight = 1.625f;
		public const float Height = 1.8f;
		public const float Depth = 0.6f;		

		public override Vector3 Size {
			get { return new Vector3( Width, Height, Depth ); }
		}
		
		/// <summary> Gets the position of the player's eye in the world. </summary>
		public Vector3 EyePosition {
			get { return new Vector3( Position.X, Position.Y + EyeHeight, Position.Z ); }
		}
		
		public override float StepSize {
			get { return 0.5f; }
		}
		
		public Game Window;
		public byte ID;
		public string DisplayName, SkinName;
		public string ModelName = "humanoid";
		protected PlayerRenderer renderer;
		public SkinType SkinType;
		
		public Player( byte id, Game window ) : base( window ) {
			ID = id;
			Window = window;
			SkinType = Window.DefaultPlayerSkinType;
		}
		
		/// <summary> Gets the block just underneath the player's feet position. </summary>
		public Block BlockUnderFeet {
			get {
				if( map == null || map.IsNotLoaded ) return Block.Air;
				Vector3I blockCoords = Vector3I.Floor( Position.X, Position.Y - 0.01f, Position.Z );
				if( !map.IsValidPos( blockCoords ) ) return Block.Air;
				return (Block)map.GetBlock( blockCoords );
			}
		}
		
		/// <summary> Gets the block at player's eye position. </summary>
		public Block BlockAtHead {
			get {
				if( map == null || map.IsNotLoaded ) return Block.Air;
				Vector3I blockCoords = Vector3I.Floor( EyePosition );
				if( !map.IsValidPos( blockCoords ) ) return Block.Air;
				return (Block)map.GetBlock( blockCoords );
			}
		}
		
		public abstract void Tick( double delta );
		
		public abstract void SetLocation( LocationUpdate update, bool interpolate );
		
		public float leftLegXRot, leftArmXRot, leftArmZRot;
		public float rightLegXRot, rightArmXRot, rightArmZRot;
		protected float animStepO, animStepN, runO, runN;
		
		protected void UpdateAnimState( Vector3 oldPos, Vector3 newPos ) {
			animStepO = animStepN;
			runO = runN;
			float dx = newPos.X - oldPos.X;
			float dz = newPos.Z - oldPos.Z;
			double distance = Math.Sqrt( dx * dx + dz * dz );
			float animSpeed = distance > 0.05 ? (float)distance * 3 : 0;
			float runDist = distance > 0.05 ? 1 : 0;
			runN += ( runDist - runN ) * 0.3f;
			animStepN += animSpeed;
		}
		
		protected void SetCurrentAnimState( int tickCount, float t ) {
			float run = Utils.Lerp( runO, runN, t );
			float anim = Utils.Lerp( animStepO, animStepN, t );
			float time = tickCount + t;
			
			rightArmXRot = (float)( Math.Cos( anim * 0.6662f + Math.PI ) * 1.5f * run );
			leftArmXRot = (float)( Math.Cos( anim * 0.6662f ) * 1.5f * run );
			rightLegXRot = (float)( Math.Cos( anim * 0.6662f ) * 1.4f * run );
			leftLegXRot = (float)( Math.Cos( anim * 0.6662f + Math.PI ) * 1.4f * run );

			float idleZRot = (float)( Math.Cos( time * 0.09f ) * 0.05f + 0.05f );
			float idleXRot = (float)( Math.Sin( time * 0.067f ) * 0.05f );
			rightArmZRot = idleZRot;
			leftArmZRot = -idleZRot;
			rightArmXRot += idleXRot;
			leftArmXRot -= idleXRot;
		}
	}
}