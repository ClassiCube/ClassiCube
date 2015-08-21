#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK Team.
 * This notice may not be removed from any source distribution.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;
using System.Diagnostics;

namespace OpenTK.Graphics {
	
    /// <summary>Defines the format for graphics operations.</summary>
    public class GraphicsMode : IEquatable<GraphicsMode> {
    	
        ColorFormat color_format;
        int depth, stencil, buffers;
        IntPtr? index = null;  // The id of the pixel format or visual.

        static GraphicsMode defaultMode;
        static IGraphicsMode implementation;
        static readonly object SyncRoot = new object();
        
        static GraphicsMode() {
            lock (SyncRoot) {
                implementation = Platform.Factory.Default.CreateGraphicsMode();
            }
        }

        internal GraphicsMode(IntPtr? index, ColorFormat color, int depth, int stencil, int buffers) {
            if (depth < 0) throw new ArgumentOutOfRangeException("depth", "Must be greater than, or equal to zero.");
            if (stencil < 0) throw new ArgumentOutOfRangeException("stencil", "Must be greater than, or equal to zero.");
            if (buffers <= 0) throw new ArgumentOutOfRangeException("buffers", "Must be greater than zero.");

            this.Index = index;
            this.ColorFormat = color;
            this.Depth = depth;
            this.Stencil = stencil;
            this.Buffers = buffers;
        }

        /// <summary>Constructs a new GraphicsMode with the specified parameters.</summary>
        /// <param name="color">The ColorFormat of the color buffer.</param>
        /// <param name="depth">The number of bits in the depth buffer.</param>
        /// <param name="stencil">The number of bits in the stencil buffer.</param>
        /// <param name="buffers">The number of render buffers. Typical values include one (single-), two (double-) or three (triple-buffering).</param>
        public GraphicsMode(ColorFormat color, int depth, int stencil, int buffers)
            : this(null, color, depth, stencil, buffers) { }

        /// <summary> Gets a nullable <see cref="System.IntPtr"/> value, indicating the platform-specific index for this GraphicsMode. </summary>
        public IntPtr? Index {
            get { LazySelectGraphicsMode(); return index; }
            set { index = value; }
        }

        /// <summary> Gets an OpenTK.Graphics.ColorFormat that describes the color format for this GraphicsFormat. </summary>
        public ColorFormat ColorFormat {
            get { LazySelectGraphicsMode(); return color_format; }
            private set { color_format = value; }
        }

        /// <summary> Gets a System.Int32 that contains the bits per pixel for the depth buffer for this GraphicsFormat. </summary>
        public int Depth {
            get { LazySelectGraphicsMode(); return depth; }
            private set { depth = value; }
        }

        /// <summary> Gets a System.Int32 that contains the bits per pixel for the stencil buffer of this GraphicsFormat. </summary>
        public int Stencil {
            get { LazySelectGraphicsMode(); return stencil; }
            private set { stencil = value; }
        }

        /// <summary> Gets a System.Int32 containing the number of buffers associated with this DisplayMode. </summary>
        public int Buffers {
            get { LazySelectGraphicsMode(); return buffers; }
            private set { buffers = value; }
        }

        /// <summary>Returns an OpenTK.GraphicsFormat compatible with the underlying platform.</summary>
        public static GraphicsMode Default {
            get {
                lock (SyncRoot) {
                    if (defaultMode == null) {
                        Debug.Print("Creating default GraphicsMode ({0}, {1}, {2}, {3}).", DisplayDevice.Default.BitsPerPixel,
                                    16, 0, 2);
                        defaultMode = new GraphicsMode(DisplayDevice.Default.BitsPerPixel, 16, 0, 2);
                    }
                    return defaultMode;
                }
            }
        }

        // Queries the implementation for the actual graphics mode if this hasn't been done already.
        // This method allows for lazy evaluation of the actual GraphicsMode and should be called
        // by all GraphicsMode properties.
        void LazySelectGraphicsMode() {
            if (index == null) {
                GraphicsMode mode = implementation.SelectGraphicsMode(color_format, depth, stencil, buffers);

                Index = mode.Index;
                ColorFormat = mode.ColorFormat;
                Depth = mode.Depth;
                Stencil = mode.Stencil;
                Buffers = mode.Buffers;
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
