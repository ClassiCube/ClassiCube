using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {
	
	public abstract class MapRenderer : IDisposable {
		
		public Game Window;
		public OpenGLApi api;
		
		public MapRenderer( Game window ) {
			Window = window;
			api = window.Graphics;
		}
		
		public virtual void Init() {
			Window.OnNewMap += OnNewMap;
			Window.OnNewMapLoaded += OnNewMapLoaded;
		}
		
		public virtual void Dispose() {
			Window.OnNewMap -= OnNewMap;
			Window.OnNewMapLoaded -= OnNewMapLoaded;
		}
		
		public abstract void Refresh();
		
		protected abstract void OnNewMap( object sender, EventArgs e );
		
		protected abstract void OnNewMapLoaded( object sender, EventArgs e );
		
		public abstract void RedrawBlock( int x, int y, int z, byte block, int oldHeight, int newHeight );
		
		public abstract void Render( double deltaTime );
	}
}