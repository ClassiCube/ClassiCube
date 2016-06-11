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
    public class GraphicsMode : IEquatable<GraphicsMode> {

        static GraphicsMode defaultMode;       

        /// <summary>Constructs a new GraphicsMode with the specified parameters.</summary>
        /// <param name="index"> Platform specific context index. </param>
        /// <param name="color">The ColorFormat of the color buffer.</param>
        /// <param name="depth">The number of bits in the depth buffer.</param>
        /// <param name="stencil">The number of bits in the stencil buffer.</param>
        /// <param name="buffers">The number of render buffers. Typical values include one (single-), two (double-) or three (triple-buffering).</param>
        internal GraphicsMode(IntPtr? index, ColorFormat color, int depth, int stencil, int buffers) {
            if (depth < 0) throw new ArgumentOutOfRangeException("depth", "Must be greater than, or equal to zero.");
            if (stencil < 0) throw new ArgumentOutOfRangeException("stencil", "Must be greater than, or equal to zero.");
            if (buffers <= 0) throw new ArgumentOutOfRangeException("buffers", "Must be greater than zero.");

            Index = index;
            ColorFormat = color;
            Depth = depth;
            Stencil = stencil;
            Buffers = buffers;
        }

        /// <summary> Gets a nullable <see cref="System.IntPtr"/> value, indicating the platform-specific index for this GraphicsMode. </summary>
        public IntPtr? Index; // The id of the pixel format or visual.

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
        			Debug.Print( "Creating default GraphicsMode ({0}, {1}, {2}, {3}).", DisplayDevice.Default.BitsPerPixel, 24, 0, 2 );
        			defaultMode = new GraphicsMode( null, DisplayDevice.Default.BitsPerPixel, 24, 0, 2 );
        		}
        		return defaultMode;
            }
        }

        /// <summary>Returns a System.String describing the current GraphicsFormat.</summary>
        /// <returns> System.String describing the current GraphicsFormat.</returns>
        public override string ToString() {
            return String.Format("Index: {0}, Color: {1}, Depth: {2}, Stencil: {3}, Buffers: {4}",
                Index, ColorFormat, Depth, Stencil, Buffers);
        }

        /// <summary> Returns the hashcode for this instance. </summary>
        /// <returns>A <see cref="System.Int32"/> hashcode for this instance.</returns>
        public override int GetHashCode() {
            return Index.GetHashCode();
        }

        /// <summary> Indicates whether obj is equal to this instance. </summary>
        /// <param name="obj">An object instance to compare for equality.</param>
        /// <returns>True, if obj equals this instance; false otherwise.</returns>
        public override bool Equals(object obj) {
        	return (obj is GraphicsMode) && Equals((GraphicsMode)obj);
        }

        /// <summary> Indicates whether other represents the same mode as this instance. </summary>
        /// <param name="other">The GraphicsMode to compare to.</param>
        /// <returns>True, if other is equal to this instance; false otherwise.</returns>
        public bool Equals(GraphicsMode other) {
            return Index.HasValue && Index == other.Index;
        }
    }
}
