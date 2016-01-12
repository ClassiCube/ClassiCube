using System;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp {
	
	public class NetPlayer : Player {
		
		int tickCount;
		public NetPlayer( string displayName, string skinName, Game game, byte id ) : base( game ) {
			DisplayName = displayName;
			SkinName = skinName;
			SkinIdentifier = "skin_" + id;
			InitRenderingData();
		}
		
		// Last known position and orientation sent by the server.
		Vector3 serverPos;
		float serverYaw, serverPitch;
		
		public override void SetLocation( LocationUpdate update, bool interpolate ) {
			Vector3 lastPos = serverPos;
			float lastYaw = serverYaw, lastPitch = serverPitch;
			if( update.IncludesPosition ) {
				serverPos = update.RelativePosition ? serverPos + update.Pos : update.Pos;
			}
			if( update.IncludesOrientation ) {
				serverYaw = update.Yaw; serverPitch = update.Pitch;
			}
			
			if( !interpolate ) {
				stateCount = 0;
				newState = oldState = new State( tickCount, serverPos, serverYaw, serverPitch );
				yawStateCount = 0;
				newYaw = oldYaw = serverYaw;
			} else {
				// Smoother interpolation by also adding midpoint.
				Vector3 midPos = Vector3.Lerp( lastPos, serverPos, 0.5f );
				float midYaw = Utils.LerpAngle( lastYaw, serverYaw, 0.5f );
				float midPitch = Utils.LerpAngle( lastPitch, serverPitch, 0.5f );
				AddState( new State( tickCount, midPos, midYaw, midPitch ) );
				AddState( new State( tickCount, serverPos, serverYaw, serverPitch ) );
				for( int i = 0; i < 3; i++ )
					AddYaw( Utils.LerpAngle( lastYaw, serverYaw, (i + 1) / 3f ) );
			}
		}
		
		struct State {
			public int tick;
			public Vector3 pos;
			public float headYaw, pitch;
			
			public State( int tick, Vector3 pos, float headYaw, float pitch ) {
				this.tick = tick;
				this.pos = pos;
				this.headYaw = headYaw;
				this.pitch = pitch;
			}
		}
		
		State[] states = new State[10];
		float[] yawStates = new float[15];
		State newState, oldState;
		float newYaw, oldYaw;
		int stateCount, yawStateCount;
		
		public override void Tick( double delta ) {
			CheckSkin();
			tickCount++;
			UpdateCurrentState();
			UpdateAnimState( oldState.pos, newState.pos, delta );
		}
		
		void AddState( State state ) {
			if( stateCount == states.Length )
				RemoveOldest( states, ref stateCount );
			states[stateCount++] = state;
		}
		
		void AddYaw( float state ) {
			if( yawStateCount == yawStates.Length )
				RemoveOldest( yawStates, ref yawStateCount );
			yawStates[yawStateCount++] = state;
		}
		
		void UpdateCurrentState() {
			oldState = newState;
			oldYaw = newYaw;
			if( stateCount > 0 ) {
				//if( states[0].tick > tickCount - 2 ) return; // 100 ms delay
				newState = states[0];
				RemoveOldest( states, ref stateCount );
			}
			if( yawStateCount > 0 ) {
				newYaw = yawStates[0];
				RemoveOldest( yawStates, ref yawStateCount );
			}
		}

		public override void RenderModel( double deltaTime, float t ) {
			Position = Vector3.Lerp( oldState.pos, newState.pos, t );
			HeadYawDegrees = Utils.LerpAngle( oldState.headYaw, newState.headYaw, t );
			YawDegrees = Utils.LerpAngle( oldYaw, newYaw, t );
			PitchDegrees = Utils.LerpAngle( oldState.pitch, newState.pitch, t );
			
			GetCurrentAnimState( t );
			Model.RenderModel( this );
		}
		
		public override void RenderName() { DrawName(); }
	}
}