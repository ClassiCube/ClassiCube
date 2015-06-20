using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public abstract class WeatherRenderer : IDisposable {
		
		public Game Window;
		protected Map map;
		protected OpenGLApi api;
		
		public WeatherRenderer( Game window ) {
			Window = window;
			map = Window.Map;
			api = window.Graphics;
		}
		
		public abstract void Render( double deltaTime );
		
		public abstract void Init();
		
		public abstract void Dispose();
	}
}
