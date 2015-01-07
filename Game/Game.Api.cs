using System;
using System.Drawing;
using OpenTK;

namespace ClassicalSharp {

	public abstract partial class Game {

		public abstract int Width { get; }
		
		public abstract int Height { get; }
		
		public abstract Rectangle Bounds { get; }
		
		public abstract string Title { get; set; }
		
		public abstract VSyncMode VSync { get; set; }
		
		public abstract bool Focused { get; }
		
		public abstract Size ClientSize { get; }
		
		public abstract WindowState WindowState { get; set; }
		
		protected abstract void SwapBuffers();
		
		public abstract void Run();
		
		public abstract void Exit();
		
		protected abstract void RegisterInputHandlers();
	}
}