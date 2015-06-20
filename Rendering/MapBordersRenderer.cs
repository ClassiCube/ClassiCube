using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public abstract class MapBordersRenderer : IDisposable {
		
		protected Map map;
		public Game Window;
		protected OpenGLApi api;
		
		public MapBordersRenderer( Game window ) {
			Window = window;
			map = Window.Map;
			api = Window.Graphics;
		}
		
		public virtual void Init() {
			Window.OnNewMap += OnNewMap;
			Window.OnNewMapLoaded += OnNewMapLoaded;
			Window.EnvVariableChanged += EnvVariableChanged;
		}

		public abstract void RenderMapSides( double deltaTime );
		
		public abstract void RenderMapEdges( double deltaTime );
		
		public virtual void Dispose() {
			Window.OnNewMap -= OnNewMap;
			Window.OnNewMapLoaded -= OnNewMapLoaded;
			Window.EnvVariableChanged -= EnvVariableChanged;
		}
		
		protected abstract void OnNewMap( object sender, EventArgs e );
		
		protected abstract void OnNewMapLoaded( object sender, EventArgs e );
		
		protected abstract void EnvVariableChanged( object sender, EnvVariableEventArgs e );
		
		// |-----------|
		// |     2     |
		// |---*****---|
		// | 3 *Map* 4 |
		// |   *   *   |
		// |---*****---|
		// |     1     |
		// |-----------|
		protected IEnumerable<Rectangle> OutsideMap( int extent ) {
			yield return new Rectangle( -extent, -extent, extent + map.Width + extent, extent );
			yield return new Rectangle( -extent, map.Length, extent +map.Width + extent, extent );
			yield return new Rectangle( -extent, 0, extent, map.Length );
			yield return new Rectangle( map.Width, 0, extent, map.Length );
		}
	}
}