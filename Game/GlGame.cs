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
	
	public class GlGame : Game {
		
		GameWindow window;
		
		public GlGame() {
			window = new GameWindow();
			window.Load += WindowLoad;
			window.RenderFrame += WindowRenderFrame;
			window.Resize += WindowResize;
		}

		void WindowResize(object sender, EventArgs e) {
			OnResize();
		}

		void WindowRenderFrame( object sender, FrameEventArgs e) {
			OnRenderFrame( e.Time );
		}

		void WindowLoad( object sender, EventArgs e ) {
			Load();
		}
		
		public override Rectangle Bounds {
			get { return window.Bounds; }
		}
		
		public override Size ClientSize {
			get { return window.ClientSize; }
		}
		
		public override bool Focused {
			get { return window.Focused; }
		}
		
		public override int Width {
			get { return window.Width; }
		}
		
		public override int Height {
			get { return window.Height; }
		}
		
		public override string Title {
			get { return window.Title; }
			set { window.Title = value; }
		}
		
		public override WindowState WindowState {
			get { return window.WindowState; }
			set { window.WindowState = value; }
		}

		public override VSyncMode VSync {
			get { return window.VSync; }
			set { window.VSync = value; }
		}
		
		public override bool IsKeyDown( Key key ) {
			return window.Keyboard[key];
		}
		
		public override bool IsMousePressed( MouseButton button ) {
			return window.Mouse[button];
		}
		
		public override void Exit() {
			window.Exit();
		}
		
		protected override void RegisterInputHandlers() {
			window.Keyboard.KeyDown += KeyDownHandler;
			window.Keyboard.KeyUp += KeyUpHandler;
			window.KeyPress += KeyPressHandler;
			window.Mouse.WheelChanged += MouseWheelChanged;
			window.Mouse.Move += MouseMove;
			window.Mouse.ButtonDown += MouseButtonDown;
			window.Mouse.ButtonUp += MouseButtonUp;
		}
		
		public override void Run() {
			window.Run();
		}
		
		protected override void SwapBuffers() {
			window.SwapBuffers();
		}
	}
}
