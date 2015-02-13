using System;
using OpenTK;

namespace ClassicalSharp {

	public abstract class LivingEntity : Entity {
		
		public LivingEntity( Game window ) : base( window ) {
		}
		
		protected float walkTimeO, walkTimeN, swingO, swingN;
		
		protected void UpdateAnimState( Vector3 oldPos, Vector3 newPos, double delta ) {
			walkTimeO = walkTimeN;
			swingO = swingN;
			float dx = newPos.X - oldPos.X;
			float dz = newPos.Z - oldPos.Z;
			double distance = Math.Sqrt( dx * dx + dz * dz );
			bool moves = distance > 0.05;
			
			if( moves ) {
				walkTimeN += (float)distance * 2 * (float)( 20 * delta );
				swingN += (float)delta * 3;
				if( swingN > 1 ) swingN = 1;
			} else {
				swingN -= (float)delta * 3;
				if( swingN < 0 ) swingN = 0;
			}
		}
		
		const float armMax = (float)( 90 * Math.PI / 180.0 );
		const float legMax = (float)( 80 * Math.PI / 180.0 );
		const float idleMax = (float)( 3 * Math.PI / 180.0 );
		const float idleXPeriod = (float)( 2 * Math.PI / 5f );
		const float idleZPeriod = (float)( 2 * Math.PI / 3.5f );		
		protected void SetCurrentAnimState( float t ) {
			float swing = Utils.Lerp( swingO, swingN, t );
			float walkTime = Utils.Lerp( walkTimeO, walkTimeN, t );
			float idleTime = (float)( game.accumulator );
			float idleXRot = (float)( Math.Sin( idleTime * idleXPeriod ) * idleMax );
			float idleZRot = (float)( idleMax + Math.Cos( idleTime * idleZPeriod ) * idleMax );		
			
			leftArmXRot = (float)( Math.Cos( walkTime ) * swing * armMax ) - idleXRot;
			rightArmXRot = -leftArmXRot;
			rightLegXRot = (float)( Math.Cos( walkTime ) * swing * legMax );
			leftLegXRot = -rightLegXRot;
			rightArmZRot = idleZRot;
			leftArmZRot = -idleZRot;
		}
	}
}