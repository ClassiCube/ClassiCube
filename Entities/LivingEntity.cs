using System;
using OpenTK;

namespace ClassicalSharp {

	public abstract class LivingEntity : Entity {
		
		public LivingEntity( Game window ) : base( window ) {
		}
		
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