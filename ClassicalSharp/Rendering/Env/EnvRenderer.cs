// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;

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
		
		public void Ready( Game game ) { }			
		public virtual void Reset( Game game ) { OnNewMap( game ); }
		
		public abstract void OnNewMap( Game game );
		
		public abstract void OnNewMapLoaded( Game game );
		
		public virtual void Dispose() {
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
		}
		
		public abstract void Render( double deltaTime );
		
		protected abstract void EnvVariableChanged( object sender, EnvVarEventArgs e );
	}
}
