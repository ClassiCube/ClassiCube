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

        /// <summary>Constructs a new GraphicsMode with the specified parameters.</summary>
        /// <param name="index"> Platform specific context index. </param>
        /// <param name="color">The ColorFormat of the color buffer.</param>
        /// <param name="depth">The number of bits in the depth buffer.</param>
        /// <param name="stencil">The number of bits in the stencil buffer.</param>
        /// <param name="buffers">The number of render buffers. Typical values include one (single-), two (double-) or three (triple-buffering).</param>
        internal GraphicsMode(ColorFormat color, int depth, int stencil, int buffers) {
            if (depth < 0) throw new ArgumentOutOfRangeException("depth", "Must be greater than, or equal to zero.");
            if (stencil < 0) throw new ArgumentOutOfRangeException("stencil", "Must be greater than, or equal to zero.");
            if (buffers <= 0) throw new ArgumentOutOfRangeException("buffers", "Must be greater than zero.");

            ColorFormat = color;
            Depth = depth;
            Stencil = stencil;
            Buffers = buffers;
        }

        /// <summary> The OpenTK.Graphics.ColorFormat that describes the color format for this GraphicsFormat. </summary>
        public ColorFormat ColorFormat;

        /// <summary> The bits per pixel for the depth buffer for this GraphicsFormat. </summary>
        public int Depth;

        /// <summary> The bits per pixel for the stencil buffer of this GraphicsFormat. </summary>
        public int Stencil;

        /// <summary> The number of buffers associated with this DisplayMode. </summary>
        public int Buffers;

        /// <summary>Returns an OpenTK.GraphicsFormat compatible with the underlying platform.</summary>
        public static GraphicsMode Default {
            get {
        		if (defaultMode == null) {
        			Debug.Print( "Creating default GraphicsMode ({0}, {1}, {2}, {3}).", DisplayDevice.Primary.BitsPerPixel, 24, 0, 2 );
        			defaultMode = new GraphicsMode( DisplayDevice.Primary.BitsPerPixel, 24, 0, 2 );
        		}
        		return defaultMode;
            }
        }
    }
}
