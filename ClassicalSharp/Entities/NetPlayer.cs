using System;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp {
	
	public class NetPlayer : Player {
		
		int tickCount;		
		public NetPlayer( string displayName, string skinName, Game window ) : base( window ) {
			DisplayName = displayName;
			SkinName = Utils.StripColours( skinName );
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
				serverYaw = update.Yaw;
				serverPitch = update.Pitch;
			}
			
			if( !interpolate ) {
				stateCount = 0;
				newState = oldState = new State( tickCount, serverPos, serverYaw, serverPitch );
			} else {
				// Smoother interpolation by also adding midpoint.
				Vector3 midPos = Vector3.Lerp( lastPos, serverPos, 0.5f );
				float midYaw = Utils.InterpAngle( lastYaw, serverYaw, 0.5f );
				float midPitch = Utils.InterpAngle( lastPitch, serverPitch, 0.5f );
				AddState( new State( tickCount, midPos, midYaw, midPitch ) );
				AddState( new State( tickCount, serverPos, serverYaw, serverPitch ) );
			}
		}
		
		struct State {
			public int tick;
			public Vector3 pos;
			public float yaw, pitch;
			
			public State( int tick, Vector3 pos, float yaw, float pitch ) {
				this.tick = tick;
				this.pos = pos;
				this.yaw = yaw;
				this.pitch = pitch;
			}
		}
		
		State[] states = new State[10];
		State newState, oldState;
		int stateCount;
		public override void Tick( double delta ) {
			CheckSkin();
			tickCount++;
			UpdateCurrentState();
			UpdateAnimState( oldState.pos, newState.pos, delta );
		}
		
		void AddState( State state ) {
		   if( stateCount == states.Length )
		     RemoveOldestState();
		   states[stateCount++] = state;
		}
		
		void RemoveOldestState() {
		   for( int i = 0; i < states.Length - 1; i++ ) {
		      states[i] = states[i + 1];
		   }
		   stateCount--;
		}
		
		void UpdateCurrentState() {		
			oldState = newState;
			if( stateCount == 0 ) return;
			
			//if( states[0].tick > tickCount - 2 ) return; // 100 ms delay
			newState = states[0];
			RemoveOldestState();
		}

		public override void Render( double deltaTime, float t ) {
			Position = Vector3.Lerp( oldState.pos, newState.pos, t );
			YawDegrees = Utils.InterpAngle( oldState.yaw, newState.yaw, t );
			PitchDegrees = Utils.InterpAngle( oldState.pitch, newState.pitch, t );
			
			SetCurrentAnimState( t );
			RenderModel( deltaTime );
		}
	}
}