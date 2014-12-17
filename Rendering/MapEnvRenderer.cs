using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public abstract class MapEnvRenderer : IDisposable {
		
		public Map Map;
		public Game Window;
		public IGraphicsApi Graphics;
		protected Block sideBlock, edgeBlock;
		
		public virtual void OnNewMap( object sender, EventArgs e ) {
		}
		
		public virtual void OnNewMapLoaded( object sender, EventArgs e ) {
		}
		
		public virtual void Init() {
			Graphics = Window.Graphics;
			Window.OnNewMap += OnNewMap;
			Window.OnNewMapLoaded += OnNewMapLoaded;
			Window.EnvVariableChanged += EnvVariableChanged;
		}
		
		public virtual void Dispose() {
			Window.OnNewMap -= OnNewMap;
			Window.OnNewMapLoaded -= OnNewMapLoaded;
			Window.EnvVariableChanged -= EnvVariableChanged;
		}
		
		void EnvVariableChanged( object sender, EnvVariableEventArgs e ) {
			if( e.Variable == EnvVariable.EdgeBlock ) {
				EdgeBlockChanged();
			} else if( e.Variable == EnvVariable.SidesBlock ) {
				SidesBlockChanged();
			} else if( e.Variable == EnvVariable.WaterLevel ) {
				WaterLevelChanged();
			}
		}
		
		public abstract void RenderMapSides( double deltaTime );
		
		public abstract void RenderMapEdges( double deltaTime );
		
		protected abstract void EdgeBlockChanged();
		
		protected abstract void SidesBlockChanged();
		
		protected abstract void WaterLevelChanged();
		
		// |-----------|
		// |     2     |
		// |---*****---|
		// | 3 *Map* 4 |
		// |   *   *   |
		// |---*****---|
		// |     1     |
		// |-----------|
		protected IEnumerable<Rectangle> OutsideMap( int extent ) {
			yield return new Rectangle( -extent, -extent, extent + Map.Width + extent, extent );
			yield return new Rectangle( -extent, Map.Length, extent + Map.Width + extent, extent );
			yield return new Rectangle( -extent, 0, extent, Map.Length );
			yield return new Rectangle( Map.Width, 0, extent, Map.Length );
		}
	}
}