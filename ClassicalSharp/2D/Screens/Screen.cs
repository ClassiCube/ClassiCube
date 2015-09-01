using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class Screen : IDisposable {
		
		protected internal Game game;
		protected IGraphicsApi graphicsApi;
		
		public Screen( Game game ) {
			this.game = game;
			graphicsApi = game.Graphics;
		}
		
		public virtual bool HandlesAllInput { get; protected set; }
		
		public abstract void Init();
		
		public abstract void Render( double delta );
		
		public abstract void Dispose();
		
		public virtual void OnResize( int oldWidth, int oldHeight, int width, int height ) {
		}
		
		public virtual bool HandlesKeyDown( Key key ) {
			return false;
		}
		
		public virtual bool HandlesKeyPress( char key ) {
			return false;
		}
		
		public virtual bool HandlesKeyUp( Key key ) {
			return false;
		}
		
		public virtual bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			return false;
		}
		
		public virtual bool HandlesMouseMove( int mouseX, int mouseY ) {
			return false;
		}
		
		public virtual bool HandlesMouseScroll( int delta ) {
			return false;
		}
		
		public virtual bool HandlesMouseUp( int mouseX, int mouseY, MouseButton button ) {
			return false;
		}
		
		/// <summary> Whether the screen completely covers the world behind it. </summary>
		public virtual bool BlocksWorld {
			get { return false; }
		}
	}
}
