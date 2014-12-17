using System;
using System.Collections.Generic;
using System.Drawing;
using OpenTK.Input;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public abstract class Screen : IDisposable {
		
		public Game Window;
		public IGraphicsApi GraphicsApi;
		
		public Screen( Game window ) {
			Window = window;
			GraphicsApi = Window.Graphics;
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
		
		public virtual bool HandlesMouseMove( int mouseX, int mouseY ) {
			return false;
		}
		
		public virtual bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
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
