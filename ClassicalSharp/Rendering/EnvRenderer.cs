// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;

namespace ClassicalSharp.Renderers {

	public abstract class EnvRenderer : IDisposable {
		
		protected World map;
		protected Game game;	
		protected IGraphicsApi graphics;
		
		public virtual void Init() {
			graphics = game.Graphics;
			game.WorldEvents.OnNewMap += OnNewMap;
			game.WorldEvents.OnNewMapLoaded += OnNewMapLoaded;
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
		}		
		
		public virtual void OnNewMap( object sender, EventArgs e ) {
		}
		
		public virtual void OnNewMapLoaded( object sender, EventArgs e ) {
		}
		
		public virtual void Dispose() {
			game.WorldEvents.OnNewMap -= OnNewMap;
			game.WorldEvents.OnNewMapLoaded -= OnNewMapLoaded;
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
		}
		
		public abstract void Render( double deltaTime );
		
		protected abstract void EnvVariableChanged( object sender, EnvVarEventArgs e );
	}
}
