#region --- License ---
/* Copyright (c) 2006-2008 the OpenTK team.
 * See license.txt for license info
 * 
 * Contributions by Andy Gill.
 */
#endregion

#region --- Using Directives ---

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Diagnostics;
using System.Reflection.Emit;


#endregion

namespace OpenTK.Graphics.OpenGL
{
    /// <summary>
    /// OpenGL bindings for .NET, implementing the full OpenGL API, including extensions.
    /// </summary>
    /// <remarks>
    /// <para>
    /// This class contains all OpenGL enums and functions defined in the latest OpenGL specification.
    /// The official .spec files can be found at: http://opengl.org/registry/.
    /// </para>
    /// <para> A valid OpenGL context must be created before calling any OpenGL function.</para>
    /// <para>
    /// Use the GL.Load and GL.LoadAll methods to prepare function entry points prior to use. To maintain
    /// cross-platform compatibility, this must be done for both core and extension functions. The GameWindow
    /// and the GLControl class will take care of this automatically.
    /// </para>
    /// <para>
    /// You can use the GL.SupportsExtension method to check whether any given category of extension functions
    /// exists in the current OpenGL context. Keep in mind that different OpenGL contexts may support different
    /// extensions, and under different entry points. Always check if all required extensions are still supported
    /// when changing visuals or pixel formats.
    /// </para>
    /// <para>
    /// You may retrieve the entry point for an OpenGL function using the GL.GetDelegate method.
    /// </para>
    /// </remarks>
    /// <see href="http://opengl.org/registry/"/>
    public sealed partial class GL : GraphicsBindingsBase
    {
        #region --- Fields ---

        internal const string Library = "opengl32.dll";

        static readonly object sync_root = new object();

        #endregion

        #region --- Constructor ---

        static GL() { }
        
        public GL() : base( typeof( Delegates ), typeof( Core ), 2 ) {
        }

        #endregion

        #region --- Public Members ---

        /// <summary>
        /// Loads all OpenGL entry points (core and extension).
        /// This method is provided for compatibility purposes with older OpenTK versions.
        /// </summary>
        [Obsolete("If you are using a context constructed outside of OpenTK, create a new GraphicsContext and pass your context handle to it. Otherwise, there is no need to call this method.")]
        public static void LoadAll()
        {
            new GL().LoadEntryPoints();
        }

        #endregion

        #region --- Protected Members ---

        /// <summary>
        /// Returns a synchronization token unique for the GL class.
        /// </summary>
        protected override object SyncRoot
        {
            get { return sync_root; }
        }

        #endregion

        #region --- GL Overloads ---

#pragma warning disable 3019
#pragma warning disable 1591
#pragma warning disable 1572
#pragma warning disable 1573

        // Note: Mono 1.9.1 truncates StringBuilder results (for 'out string' parameters).
        // We work around this issue by doubling the StringBuilder capacity.

        public static void Rotate(Single angle, Vector3 axis)
        {
            GL.Rotate(angle, axis.X, axis.Y, axis.Z);
        }

        public static void Scale(Vector3 scale)
        {
            GL.Scale(scale.X, scale.Y, scale.Z);
        }

        public static void Translate(Vector3 trans)
        {
            GL.Translate(trans.X, trans.Y, trans.Z);
        }

        public unsafe static void MultMatrix(ref Matrix4 mat)
        {
        	fixed (Single* m_ptr = &mat.Row0.X)
        		GL.MultMatrix(m_ptr);
        }

        public unsafe static void LoadMatrix(ref Matrix4 mat)
        {
        	fixed (Single* m_ptr = &mat.Row0.X)
        		GL.LoadMatrix(m_ptr);
        }

        public static void VertexPointer(int size, VertexPointerType type, int stride, int offset)
        {
            VertexPointer(size, type, stride, (IntPtr)offset);
        }

        public static void ColorPointer(int size, ColorPointerType type, int stride, int offset)
        {
            ColorPointer(size, type, stride, (IntPtr)offset);
        }

        public static void TexCoordPointer(int size, TexCoordPointerType type, int stride, int offset)
        {
            TexCoordPointer(size, type, stride, (IntPtr)offset);
        }

        public static void DrawElements(BeginMode mode, int count, DrawElementsType type, int offset)
        {
            DrawElements(mode, count, type, new IntPtr(offset));
        }

        public unsafe static void GetFloat(GetPName pname, out Vector2 vector)
        {
        	fixed (Vector2* ptr = &vector)
        		GetFloat(pname, (float*)ptr);
        }

        public unsafe static void GetFloat(GetPName pname, out Vector3 vector)
        {
            fixed (Vector3* ptr = &vector)
        		GetFloat(pname, (float*)ptr);
        }

        public unsafe static void GetFloat(GetPName pname, out Vector4 vector)
        {
            fixed (Vector4* ptr = &vector)
        		GetFloat(pname, (float*)ptr);
        }

        public unsafe static void GetFloat(GetPName pname, out Matrix4 matrix)
        {
        	fixed (Matrix4* ptr = &matrix)
        		GetFloat(pname, (float*)ptr);
        }

#pragma warning restore 3019
#pragma warning restore 1591
#pragma warning restore 1572
#pragma warning restore 1573

        #endregion
    }
}
