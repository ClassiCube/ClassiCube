#if !USE_DX
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
    	
        public abstract void SwapBuffers();

        public abstract void MakeCurrent(INativeWindow window);

        public abstract bool IsCurrent { get; }

        public abstract bool VSync { get; set; }

        public virtual void Update(INativeWindow window) { }

        public abstract void LoadAll();
        
        public GraphicsMode Mode;
        public IntPtr ContextHandle;

        public abstract IntPtr GetAddress(string function);
        
        public void Dispose() {
        	Dispose(true);
        	GC.SuppressFinalize(this);
        }
        
        protected abstract void Dispose(bool calledManually);
        
        ~IGraphicsContext() { Dispose(false); }
    }
    
    public class GraphicsContextException : Exception {
        public GraphicsContextException() : base() { }
        public GraphicsContextException(string message) : base(message) { }
    }
}
#endif
