using System;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp.Commands;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Network;
using ClassicalSharp.Particles;
using ClassicalSharp.Renderers;
using ClassicalSharp.Selections;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	// TODO: Rewrite this so it isn't tied to GameWindow. (so we can use DirectX as backend)
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