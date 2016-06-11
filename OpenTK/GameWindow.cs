#region License
//
// The Open Toolkit Library License
//
// Copyright (c) 2006 - 2009 the Open Toolkit library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
#endregion

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Threading;
using OpenTK.Graphics;
using OpenTK.Input;
using OpenTK.Platform;

namespace OpenTK
{
	/// <summary>
	/// The GameWindow class contains cross-platform methods to create and render on an OpenGL
	/// window, handle input and load resources.
	/// </summary>
	/// <remarks>
	/// GameWindow contains several events you can hook or override to add your custom logic:
	/// <list>
	/// <item>
	/// OnLoad: Occurs after creating the OpenGL context, but before entering the main loop.
	/// Override to load resources.
	/// </item>
	/// <item>
	/// OnUnload: Occurs after exiting the main loop, but before deleting the OpenGL context.
	/// Override to unload resources.
	/// </item>
	/// <item>
	/// OnResize: Occurs whenever GameWindow is resized. You should update the OpenGL Viewport
	/// and Projection Matrix here.
	/// </item>
	/// <item>
	/// OnUpdateFrame: Occurs at the specified logic update rate. Override to add your game
	/// logic.
	/// </item>
	/// <item>
	/// OnRenderFrame: Occurs at the specified frame render rate. Override to add your
	/// rendering code.
	/// </item>
	/// </list>
	/// Call the Run() method to start the application's main loop. Run(double, double) takes two
	/// parameters that
	/// specify the logic update rate, and the render update rate.
	/// </remarks>
	public class GameWindow : NativeWindow, IGameWindow, IDisposable {
		
		IGraphicsContext glContext;
		bool isExiting = false;
		double render_period;
		// TODO: Implement these:
		double render_time;
		bool vsync;

		Stopwatch render_watch = new Stopwatch();
		double next_render = 0.0;
		FrameEventArgs render_args = new FrameEventArgs();
		
		/// <summary>Constructs a new GameWindow with the specified attributes.</summary>
		/// <param name="width">The width of the GameWindow in pixels.</param>
		/// <param name="height">The height of the GameWindow in pixels.</param>
		/// <param name="mode">The OpenTK.Graphics.GraphicsMode of the GameWindow.</param>
		/// <param name="title">The title of the GameWindow.</param>
		/// <param name="options">GameWindow options regarding window appearance and behavior.</param>
		/// <param name="device">The OpenTK.Graphics.DisplayDevice to construct the GameWindow in.</param>
		public GameWindow(int width, int height, GraphicsMode mode, string title, bool nullContext,
		                  GameWindowFlags options, DisplayDevice device)
			: base(width, height, title, options, mode, device) {
			try {
				glContext = nullContext ? new NullContext() :
					Factory.Default.CreateGLContext(mode, WindowInfo);
				glContext.MakeCurrent(WindowInfo);
				glContext.LoadAll();
				VSync = true;
			} catch (Exception e) {
				Debug.Print(e.ToString());
				base.Dispose();
				throw;
			}
		}
		
		/// <summary> Disposes of the GameWindow, releasing all resources consumed by it. </summary>
		public override void Dispose() {
			try {
				Dispose(true);
			} finally {
				try {
					if (glContext != null) {
						glContext.Dispose();
						glContext = null;
					}
				} finally {
					base.Dispose();
				}
			}
			GC.SuppressFinalize(this);
		}

		/// <summary> Closes the GameWindow. Equivalent to <see cref="NativeWindow.Close"/> method. </summary>
		/// <remarks> <para>Override if you are not using <see cref="GameWindow.Run()"/>.</para>
		/// <para>If you override this method, place a call to base.Exit(), to ensure proper OpenTK shutdown.</para> </remarks>
		public virtual void Exit() {
			Close();
		}
		
		/// <summary> Makes the GraphicsContext current on the calling thread. </summary>
		public void MakeCurrent() {
			EnsureUndisposed();
			Context.MakeCurrent(WindowInfo);
		}
		
		/// <summary> Called when the NativeWindow is about to close. </summary>
		/// <param name="e">
		/// The <see cref="System.ComponentModel.CancelEventArgs" /> for this event.
		/// Set e.Cancel to true in order to stop the GameWindow from closing.</param>
		protected override void OnClosing(object sender, System.ComponentModel.CancelEventArgs e) {
			base.OnClosing(sender, e);
			if (!e.Cancel) {
				isExiting = true;
				OnUnload(EventArgs.Empty);
			}
		}
		
		/// <summary> Called after an OpenGL context has been established, but before entering the main loop. </summary>
		/// <param name="e">Not used.</param>
		protected virtual void OnLoad(EventArgs e) {
			if (Load != null) Load(this, e);
		}
		
		/// <summary> Called after GameWindow.Exit was called, but before destroying the OpenGL context. </summary>
		/// <param name="e">Not used.</param>
		protected virtual void OnUnload(EventArgs e) {
			if (Unload != null) Unload(this, e);
		}
		
		/// <summary> Enters the game loop of the GameWindow updating and rendering at maximum frequency. </summary>
		/// <remarks> When overriding the default game loop you should call ProcessEvents()
		/// to ensure that your GameWindow responds to operating system events.
		/// <para> Once ProcessEvents() returns, it is time to call update and render the next frame. </para> </remarks>
		/// <param name="frames_per_second">The frequency of RenderFrame events.</param>
		public void Run() {
			EnsureUndisposed();
			try {
				Visible = true;   // Make sure the GameWindow is visible.
				OnLoad(EventArgs.Empty);
				OnResize(null, EventArgs.Empty);
				Debug.Print("Entering main loop.");
				render_watch.Start();
				
				while (true) {
					ProcessEvents();
					if (Exists && !IsExiting)
						RaiseRenderFrame(render_watch, ref next_render, render_args);
					else
						return;
				}
			} finally {
				if (Exists) {
					// TODO: Should similar behaviour be retained, possibly on native window level?
					//while (this.Exists)
					//    ProcessEvents(false);
				}
			}
		}

		void RaiseRenderFrame(Stopwatch render_watch, ref double next_render, FrameEventArgs render_args)  {
			// Cap the maximum time drift to 1 second (e.g. when the process is suspended).
			double time = render_watch.Elapsed.TotalSeconds;
			if (time > 1.0)
				time = 1.0;
			if (time <= 0)
				return;
			double time_left = next_render - time;

			if (time_left <= 0.0) {
				// Schedule next render event. The 1 second cap ensures the process does not appear to hang.
				next_render = time_left;
				if (next_render < -1.0)
					next_render = -1.0;

				render_watch.Reset();
				render_watch.Start();

				if (time > 0) {
					render_period = render_args.Time = time;
					OnRenderFrameInternal(render_args);
					render_time = render_watch.Elapsed.TotalSeconds;
				}
			}
		}

		/// <summary> Swaps the front and back buffer, presenting the rendered scene to the user. </summary>
		public void SwapBuffers() {
			EnsureUndisposed();
			this.Context.SwapBuffers();
		}
		
		/// <summary> Returns the opengl IGraphicsContext associated with the current GameWindow. </summary>
		public IGraphicsContext Context {
			get { EnsureUndisposed(); return glContext; }
		}

		/// <summary>
		/// Gets a value indicating whether the shutdown sequence has been initiated
		/// for this window, by calling GameWindow.Exit() or hitting the 'close' button.
		/// If this property is true, it is no longer safe to use any OpenTK.Input or
		/// OpenTK.Graphics.OpenGL functions or properties.
		/// </summary>
		public bool IsExiting {
			get { EnsureUndisposed(); return isExiting; }
		}
		
		/// <summary> Gets a double representing the actual frequency of RenderFrame events, in hertz (i.e. fps or frames per second). </summary>
		public double RenderFrequency {
			get {
				EnsureUndisposed();
				if (render_period == 0.0)
					return 1.0;
				return 1.0 / render_period;
			}
		}
		
		/// <summary> Gets a double representing the period of RenderFrame events, in seconds. </summary>
		public double RenderPeriod {
			get { EnsureUndisposed(); return render_period; }
		}
		
		/// <summary> Gets a double representing the time spent in the RenderFrame function, in seconds. </summary>
		public double RenderTime {
			get { EnsureUndisposed(); return render_time; }
			protected set { EnsureUndisposed(); render_time = value; }
		}
		
		/// <summary> Gets or sets the VSyncMode. </summary>
		public bool VSync {
			get { EnsureUndisposed(); return vsync; }
			set { EnsureUndisposed(); Context.VSync = (vsync = value); }
		}
		
		/// <summary> Gets or states the state of the NativeWindow. </summary>
		public override WindowState WindowState {
			get { return base.WindowState; }
			set {
				base.WindowState = value;
				Debug.Print("Updating Context after setting WindowState to {0}", value);

				if (Context != null)
					Context.Update(WindowInfo);
			}
		}
		
		/// <summary> Occurs before the window is displayed for the first time. </summary>
		public event EventHandler<EventArgs> Load;

		/// <summary> Occurs when it is time to render a frame. </summary>
		public event EventHandler<FrameEventArgs> RenderFrame;

		/// <summary> Occurs before the window is destroyed. </summary>
		public event EventHandler<EventArgs> Unload;
		
		/// <summary> Override to add custom cleanup logic. </summary>
		/// <param name="manual">True, if this method was called by the application; false if this was called by the finalizer thread.</param>
		protected virtual void Dispose(bool manual) { }
		
		/// <summary> Called when the frame is rendered. </summary>
		/// <param name="e">Contains information necessary for frame rendering.</param>
		/// <remarks> Subscribe to the <see cref="RenderFrame"/> event instead of overriding this method. </remarks>
		protected virtual void OnRenderFrame(FrameEventArgs e) {
			if (RenderFrame != null) RenderFrame(this, e);
		}
		
		protected override void OnResize(object sender, EventArgs e) {
			base.OnResize(sender, e);
			glContext.Update(base.WindowInfo);
		}
		
		private void OnRenderFrameInternal(FrameEventArgs e) { if (Exists && !isExiting) OnRenderFrame(e); }
	}
}