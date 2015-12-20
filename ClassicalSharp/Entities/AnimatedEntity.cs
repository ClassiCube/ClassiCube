using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp {

	/// <summary> Entity that is animated depending on movement speed and time. </summary>
	public abstract class AnimatedEntity : PhysicsEntity {
		
		public AnimatedEntity( Game game ) : base( game ) {
		}
		public float legXRot, armXRot, armZRot;
		public float bobYOffset, tilt, walkTime, swing;
		protected float walkTimeO, walkTimeN, swingO, swingN;
		
		/// <summary> Calculates the next animation state based on old and new position. </summary>
		protected void UpdateAnimState( Vector3 oldPos, Vector3 newPos, double delta ) {
			walkTimeO = walkTimeN;
			swingO = swingN;
			float dx = newPos.X - oldPos.X;
			float dz = newPos.Z - oldPos.Z;
			double distance = Math.Sqrt( dx * dx + dz * dz );

			if( distance > 0.05 ) {
				float walkDelta = (float)distance * 2 * (float)(20 * delta);
				walkTimeN += walkDelta;
				swingN += (float)delta * 3;
			} else {
				swingN -= (float)delta * 3;
			}
			Utils.Clamp( ref swingN, 0, 1 );
			UpdateHumanState();
		}
		
		const float armMax = 60 * Utils.Deg2Rad;
		const float legMax = 80 * Utils.Deg2Rad;
		const float idleMax = 3 * Utils.Deg2Rad;
		const float idleXPeriod = (float)(2 * Math.PI / 5.0f);
		const float idleZPeriod = (float)(2 * Math.PI / 3.5f);
		
		/// <summary> Calculates the interpolated state between the last and next animation state. </summary>
		protected void GetCurrentAnimState( float t ) {
			swing = Utils.Lerp( swingO, swingN, t );
			walkTime = Utils.Lerp( walkTimeO, walkTimeN, t );
			float idleTime = (float)game.accumulator;
			float idleXRot = (float)(Math.Sin( idleTime * idleXPeriod ) * idleMax);
			float idleZRot = (float)(idleMax + Math.Cos(idleTime * idleZPeriod) * idleMax);
			
			armXRot = (float)(Math.Cos( walkTime ) * swing * armMax) - idleXRot;
			legXRot = -(float)(Math.Cos( walkTime ) * swing * legMax);
			armZRot = -idleZRot;
			
			bobYOffset = (float)(Math.Abs( Math.Cos( walkTime ) ) * swing * (2/16f));
			tilt = (float)Math.Cos( walkTime ) * swing * (0.15f * Utils.Deg2Rad);
			
			if( Model is PlayerModel )
				CalcHumanAnim( idleXRot, idleZRot );
		}
		
		internal float leftXRot, leftYRot, leftZRot;
		internal float rightXRot, rightYRot, rightZRot;
		ArmsAnim animMode = ArmsAnim.NoPerpendicular;
		int statesDone;
		static Random rnd = new Random();
		
		void UpdateHumanState() {
			if( game.SimpleArmsAnim ) {
				animMode = ArmsAnim.NoPerpendicular;
				return;
			}
			// crosses over body, finished an arm swing
			int oldState = Math.Sign( Math.Cos( walkTimeO ) );
			int newState = Math.Sign( Math.Cos( walkTimeN ) );
			if( oldState != newState )
				statesDone++;
			
			// should we switch animations?
			if( statesDone == 5 ) {
				statesDone = 0;
				animMode = (ArmsAnim)rnd.Next( 0, 4 );
			}
		}
		
		void CalcHumanAnim( float idleXRot, float idleZRot ) {
			switch( animMode ) {
				case ArmsAnim.NoPerpendicular:
					leftXRot = armXRot; leftYRot = 0; leftZRot = armZRot;
					rightXRot = -armXRot; rightYRot = 0; rightZRot = -armZRot;
					return;
				case ArmsAnim.LeftPerpendicular:
					PerpendicularAnim( out leftXRot, out leftYRot, out leftZRot );
					rightXRot = -armXRot; rightYRot = 0; rightZRot = -armZRot;
					return;
				case ArmsAnim.RightPerpendicular:
					leftXRot = armXRot; leftYRot = 0; leftZRot = armZRot;
					PerpendicularAnim( out rightXRot, out rightYRot, out rightZRot );
					rightXRot = -rightXRot; rightZRot = -rightZRot;
					return;
				case ArmsAnim.BothPerpendicular:
					PerpendicularAnim( out leftXRot, out leftYRot, out leftZRot );
					PerpendicularAnim( out rightXRot, out rightYRot, out rightZRot );
					rightXRot = -rightXRot; rightZRot = -rightZRot;
					break;
			}
		}
		
		const float maxAngle = 90 * Utils.Deg2Rad;
		void PerpendicularAnim( out float xRot, out float yRot, out float zRot ) {
			xRot = 0;
			yRot = 0;
			yRot = (float)(Math.Cos( walkTime ) * swing * armMax * 1.5f);
			float angle = (float)(1 + 0.3 * Math.Sin( walkTime ) );
			zRot = -angle * swing * maxAngle;
		}
		
		enum ArmsAnim {
			NoPerpendicular, // i.e. both parallel
			LeftPerpendicular,
			RightPerpendicular,
			BothPerpendicular,
		}
	}
}