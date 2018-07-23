#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK Team.
 * This notice may not be removed from any source distribution.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;

namespace OpenTK.Graphics {
	
    /// <summary>Defines the format for graphics operations.</summary>
    public class GraphicsMode {
        static GraphicsMode defaultMode;       

        internal GraphicsMode(ColorFormat color, int depth, int stencil, int buffers) {
            ColorFormat = color;
            Depth = depth;
            Stencil = stencil;
            Buffers = buffers;
        }

        public ColorFormat ColorFormat;
        public int Depth, Stencil, Buffers;

        public static GraphicsMode Default {
            get {
        		if (defaultMode == null) {
        			Debug.Print("Creating default GraphicsMode ({0}, {1}, {2}, {3}).", 
        			            DisplayDevice.Default.BitsPerPixel, 24, 0, 2);
        			defaultMode = new GraphicsMode(DisplayDevice.Default.BitsPerPixel, 24, 0, 2);
        		}
        		return defaultMode;
            }
        }
    }

    public class GraphicsModeException : Exception {
        public GraphicsModeException() : base() { }
        public GraphicsModeException(string message) : base(message) { }
    }
}
