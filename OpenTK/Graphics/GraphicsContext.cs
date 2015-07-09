#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK Team.
 * This notice may not be removed from any source distribution.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

using OpenTK.Platform;

namespace OpenTK.Graphics
{
	/// <summary>
	/// Represents and provides methods to manipulate an OpenGL render context.
	/// </summary>
	public sealed class GraphicsContext : IGraphicsContext, IGraphicsContextInternal
	{
		#region --- Fields ---

		IGraphicsContext implementation;  // The actual render context implementation for the underlying platform.
		bool disposed;

		#endregion

		#region --- Constructors ---

		/// <summary>
		/// Constructs a new GraphicsContext with the specified GraphicsMode, version and flags,  and attaches it to the specified window.
		/// </summary>
		/// <param name="mode">The OpenTK.Graphics.GraphicsMode of the GraphicsContext.</param>
		/// <param name="window">The OpenTK.Platform.IWindowInfo to attach the GraphicsContext to.</param>
		public GraphicsContext(GraphicsMode mode, IWindowInfo window)
		{
			if (mode == null) throw new ArgumentNullException("mode", "Must be a valid GraphicsMode.");
			if (window == null) throw new ArgumentNullException("window", "Must point to a valid window.");

			Debug.Print("Creating GraphicsContext.");
			try {
				Debug.Indent();
				Debug.Print("GraphicsMode: {0}", mode);
				Debug.Print("IWindowInfo: {0}", window);
					
				IPlatformFactory factory = Factory.Default;
				implementation = factory.CreateGLContext(mode, window);
			} finally {
				Debug.Unindent();
			}
		}
		#endregion

		#region --- IGraphicsContext Members ---

		/// <summary>
		/// Swaps buffers on a context. This presents the rendered scene to the user.
		/// </summary>
		public void SwapBuffers()
		{
			implementation.SwapBuffers();
		}

		/// <summary>
		/// Makes the GraphicsContext the current rendering target.
		/// </summary>
		/// <param name="window">A valid <see cref="OpenTK.Platform.IWindowInfo" /> structure.</param>
		/// <remarks>
		/// You can use this method to bind the GraphicsContext to a different window than the one it was created from.
		/// </remarks>
		public void MakeCurrent(IWindowInfo window)
		{
			implementation.MakeCurrent(window);
		}

		/// <summary>
		/// Gets a <see cref="System.Boolean"/> indicating whether this instance is current in the calling thread.
		/// </summary>
		public bool IsCurrent
		{
			get { return implementation.IsCurrent; }
		}

		/// <summary>
		/// Gets a <see cref="System.Boolean"/> indicating whether this instance has been disposed.
		/// It is an error to access any instance methods if this property returns true.
		/// </summary>
		public bool IsDisposed
		{
			get { return disposed && implementation.IsDisposed; }
			private set { disposed = value; }
		}

		/// <summary>
		/// Gets or sets a value indicating whether VSync is enabled.
		/// </summary>
		public bool VSync
		{
			get { return implementation.VSync; }
			set { implementation.VSync = value;  }
		}

		/// <summary>
		/// Updates the graphics context.  This must be called when the render target
		/// is resized for proper behavior on Mac OS X.
		/// </summary>
		/// <param name="window"></param>
		public void Update(IWindowInfo window)
		{
			implementation.Update(window);
		}

		/// <summary>
		/// Loads all OpenGL entry points.
		/// </summary>
		/// <exception cref="OpenTK.Graphics.GraphicsContextException">
		/// Occurs when this instance is not current on the calling thread.
		/// </exception>
		public void LoadAll()
		{
			implementation.LoadAll();
		}
		
		#endregion

		#region --- IGraphicsContextInternal Members ---

		/// <summary>
		/// Gets a handle to the OpenGL rendering context.
		/// </summary>
		IntPtr IGraphicsContextInternal.Context
		{
			get { return ((IGraphicsContextInternal)implementation).Context; }
		}

		/// <summary>
		/// Gets the GraphicsMode of the context.
		/// </summary>
		public GraphicsMode GraphicsMode
		{
			get { return implementation.GraphicsMode; }
		}

		/// <summary>
		/// Gets the address of an OpenGL extension function.
		/// </summary>
		/// <param name="function">The name of the OpenGL function (e.g. "glGetString")</param>
		/// <returns>
		/// A pointer to the specified function or IntPtr.Zero if the function isn't
		/// available in the current opengl context.
		/// </returns>
		IntPtr IGraphicsContextInternal.GetAddress(string function)
		{
			return (implementation as IGraphicsContextInternal).GetAddress(function);
		}

		#endregion

		#region --- IDisposable Members ---

		/// <summary>
		/// Disposes of the GraphicsContext.
		/// </summary>
		public void Dispose()
		{
			this.Dispose(true);
			GC.SuppressFinalize(this);
		}

		void Dispose(bool manual)
		{
			if (!IsDisposed)
			{
				Debug.Print("Disposing context {0}.", (this as IGraphicsContextInternal).Context.ToString());

				if (manual) {
					if (implementation != null)
						implementation.Dispose();
				}
				IsDisposed = true;
			}
		}

		#endregion
	}
}
