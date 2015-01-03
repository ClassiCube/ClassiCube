using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp {
	
	public class NetPlayer : Player {
		
		int tickCount = 0;
		
		public NetPlayer( byte id, string displayName, string skinName, Game window ) : base( id, window ) {
			DisplayName = displayName;
			SkinName = Utils.StripColours( skinName );
			renderer = new PlayerRenderer( this, window );
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
				states.Clear();
				newState = oldState = new State( tickCount, serverPos, serverYaw, serverPitch );
			} else {
				// Smoother interpolation by also adding midpoint.
				Vector3 midPos = Vector3.Lerp( lastPos, serverPos, 0.5f );
				float midYaw = Utils.InterpAngle( lastYaw, serverYaw, 0.5f );
				float midPitch = Utils.InterpAngle( lastPitch, serverPitch, 0.5f );
				states.Add( new State( tickCount, midPos, midYaw, midPitch ) );
				states.Add( new State( tickCount, serverPos, serverYaw, serverPitch ) );
			}
		}
		
		public override void Despawn() {
			renderer.Dispose();
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
		
		List<State> states = new List<State>();
		State newState, oldState;
		public override void Tick( double delta ) {
			Bitmap bmp;
			Window.AsyncDownloader.TryGetImage( "skin_" + SkinName, out bmp );
			if( bmp != null ) {
				Window.Graphics.DeleteTexture( renderer.TextureId );
				renderer.TextureId = Window.Graphics.LoadTexture( bmp );
				SkinType = Utils.GetSkinType( bmp );
				bmp.Dispose();
			}
			tickCount++;
			UpdateCurrentState();
			UpdateAnimState( oldState.pos, newState.pos );
		}
		
		void UpdateCurrentState() {		
			oldState = newState;
			if( states.Count < 1 ) return;
			
			do {
				oldState = newState;
				State state = states[0];
				//if( state.tick > tickCount - 2 ) break; // 100 ms delay
				newState = state;
				states.RemoveAt( 0 );
			} while( states.Count >= 10 ); // Drop old states, otherwise we will never be able to catch up.
		}

		public override void Render( double deltaTime, float t ) {
			Position = Vector3.Lerp( oldState.pos, newState.pos, t );
			YawDegrees = Utils.InterpAngle( oldState.yaw, newState.yaw, t );
			PitchDegrees = Utils.InterpAngle( oldState.pitch, newState.pitch, t );
			
			SetCurrentAnimState( tickCount, t );
			renderer.Render( deltaTime );
		}
	}
}