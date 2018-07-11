#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK Team.
 * This notice may not be removed from any source distribution.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;
using OpenTK.Platform;

namespace OpenTK.Graphics {
	
    /// <summary> Provides methods for creating and interacting with an OpenGL context. </summary>
    public abstract class IGraphicsContext : IDisposable {
    	
        /// <summary> Swaps buffers, presenting the rendered scene to the user. </summary>
        public abstract void SwapBuffers();

        /// <summary> Makes the GraphicsContext current in the calling thread. </summary>
        /// <param name="window">An OpenTK.Platform.IWindowInfo structure that points to a valid window.</param>
        /// <remarks>
        /// <para>OpenGL commands in one thread, affect the GraphicsContext which is current in that thread.</para>
        /// <para>It is an error to issue an OpenGL command in a thread without a current GraphicsContext.</para>
        /// </remarks>
        public abstract void MakeCurrent(IWindowInfo window);

        /// <summary> Gets a <see cref="System.Boolean"/> indicating whether this instance is current in the calling thread. </summary>
        public abstract bool IsCurrent { get; }

        /// <summary> Gets or sets a value indicating whether VSyncing is enabled. </summary>
        public abstract bool VSync { get; set; }

        /// <summary> Updates the graphics context.  This must be called when the region the graphics context
        /// is drawn to is resized. </summary>
        public virtual void Update(IWindowInfo window) { }

        /// <summary> Gets the GraphicsMode of this instance. </summary>
        public GraphicsMode Mode;

        /// <summary> Loads all OpenGL entry points. Requires this instance to be current on the calling thread. </summary>
        public abstract void LoadAll();

        /// <summary> Handle to the OpenGL rendering context. </summary>
        public IntPtr ContextHandle;

        /// <summary> Gets the address of an OpenGL extension function. </summary>
        /// <param name="function">The name of the OpenGL function (e.g. "glGetString")</param>
        /// <returns> A pointer to the specified function or IntPtr.Zero if the function isn't
        /// available in the current opengl context. </returns>
        public abstract IntPtr GetAddress(string function);
        
        public void Dispose() {
        	Dispose(true);
        	GC.SuppressFinalize( this );
        }
        
        protected abstract void Dispose(bool calledManually);
        
        ~IGraphicsContext() { Dispose(false); }
    }
    
    public class GraphicsContextException : Exception {
        public GraphicsContextException() : base() { }
        public GraphicsContextException(string message) : base(message) { }
    }
}
