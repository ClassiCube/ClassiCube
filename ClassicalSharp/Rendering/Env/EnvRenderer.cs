// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Renderers {
	public abstract class EnvRenderer : IGameComponent {
		
		protected World map;
		protected Game game;
		protected IGraphicsApi graphics;
		
		public virtual void Init( Game game ) {
			this.game = game;
			map = game.World;
			graphics = game.Graphics;
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
		}
		
		public virtual void UseLegacyMode( bool legacy ) { }
		
		public void Ready( Game game ) { }
		public virtual void Reset( Game game ) { OnNewMap( game ); }
		
		public abstract void OnNewMap( Game game );
		
		public abstract void OnNewMapLoaded( Game game );
		
		public virtual void Dispose() {
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
		}
		
		public abstract void Render( double deltaTime );
		
		protected abstract void EnvVariableChanged( object sender, EnvVarEventArgs e );
		
		
		protected byte BlockOn( out float fogDensity, out FastColour fogCol ) {
			BlockInfo info = game.BlockInfo;
			Vector3 pos = game.CurrentCameraPos;
			Vector3I coords = Vector3I.Floor( pos );
			
			byte block = game.World.SafeGetBlock( coords );
			AABB blockBB = new AABB(
				(Vector3)coords + info.MinBB[block],
				(Vector3)coords + info.MaxBB[block] );
			
			if( blockBB.Contains( pos ) && info.FogDensity[block] != 0 ) {
				fogDensity = info.FogDensity[block];
				fogCol = info.FogColour[block];
			} else {
				fogDensity = 0;
				// Blend fog and sky together
				float blend = (float)BlendFactor( game.ViewDistance );
				fogCol = FastColour.Lerp( map.Env.FogCol, map.Env.SkyCol, blend );
			}
			return block;
		}
		
		double BlendFactor( float x ) {
			//return -0.05 + 0.22 * Math.Log( Math.Pow( x, 0.25 ) );
			double blend = -0.13 + 0.28 * Math.Log( Math.Pow( x, 0.25 ) );
			if( blend < 0 ) blend = 0;
			if( blend > 1 ) blend = 1;
			return blend;
		}
	}
}
