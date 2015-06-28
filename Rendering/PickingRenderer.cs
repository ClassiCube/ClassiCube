using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public abstract class PickingRenderer {
		
		protected Game window;
		protected OpenGLApi graphics;
		
		public PickingRenderer( Game window ) {
			this.window = window;
			graphics = window.Graphics;
		}
		
		public abstract void Init();
		
		public abstract void Render( double delta, PickedPos pos );
		
		public abstract void Dispose();
	}
}
