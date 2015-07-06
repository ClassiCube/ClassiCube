#region --- License ---
/* Copyright (c) 2006-2008 the OpenTK team.
 * See license.txt for license info
 * 
 * Contributions by Andy Gill.
 */
#endregion

using System;
using System.Diagnostics;
using System.Reflection;

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
    /// </remarks>
    /// <see href="http://opengl.org/registry/"/>
    public sealed partial class GL : BindingsBase
    {

        internal const string Library = "opengl32.dll";

        static readonly object sync_root = new object();

        static GL() { }
        
        public GL() : base( typeof( Core ) ) {
        }
        
        protected override IntPtr GetAddress( string funcname ) {
            return (GraphicsContext.CurrentContext as IGraphicsContextInternal).GetAddress( funcname );
        }
        
        internal void LoadEntryPoints() {
            // Using reflection is more than 3 times faster than directly loading delegates on the first
            // run, probably due to code generation overhead. Subsequent runs are faster with direct loading
            // than with reflection, but the first time is more significant.

            int supported = 0; 
            FieldInfo[] delegates = typeof( Delegates ).GetFields( BindingFlags.Static | BindingFlags.Public );

            Debug.Write("Loading OpenGL function pointers... ");
            Stopwatch time = Stopwatch.StartNew();

            foreach (FieldInfo f in delegates) {
                Delegate d = LoadDelegate( f.Name, f.FieldType );
                if (d != null) supported++;

                lock (sync_root) {
                    f.SetValue(null, d);
                }
            }

            time.Stop();
            Debug.Print("{0} extensions loaded in {1} ms.", supported, time.Elapsed.TotalMilliseconds);
            time.Reset();
        }
    }
}
