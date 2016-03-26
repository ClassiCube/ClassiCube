// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Renderers {

	public abstract class EnvRenderer : IDisposable {
		
		protected Map map;
		protected Game game;	
		protected IGraphicsApi graphics;
		
		public virtual void Init() {
			graphics = game.Graphics;
			game.MapEvents.OnNewMap += OnNewMap;
			game.MapEvents.OnNewMapLoaded += OnNewMapLoaded;
			game.MapEvents.EnvVariableChanged += EnvVariableChanged;
		}		
		
		public virtual void OnNewMap( object sender, EventArgs e ) {
		}
		
		public virtual void OnNewMapLoaded( object sender, EventArgs e ) {
		}
		
		public virtual void Dispose() {
			game.MapEvents.OnNewMap -= OnNewMap;
			game.MapEvents.OnNewMapLoaded -= OnNewMapLoaded;
			game.MapEvents.EnvVariableChanged -= EnvVariableChanged;
		}
		
		public abstract void Render( double deltaTime );
		
		protected abstract void EnvVariableChanged( object sender, EnvVarEventArgs e );
	}
}
