#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 *
 * Date: 12/8/2007
 * Time: 6:43 ��
 */
#endregion

using System;
using System.Runtime.InteropServices;
using System.Reflection;

namespace OpenTK.Platform.Windows
{
	internal partial class Wgl : BindingsBase
	{
		#region --- Constructors ---

		static Wgl() {
			// Ensure core entry points are ready prior to accessing any method.
			// Resolves bug [#993]: "Possible bug in GraphicsContext.CreateDummyContext()"
			//LoadAll();
		}

		#endregion

		#region --- Fields ---

		internal const string Library = "OPENGL32.DLL";
		static readonly object sync_root = new object();

		#endregion

		public Wgl() : base( typeof( Delegates ), typeof( Imports ), 3 ) {
		}

		protected override object SyncRoot {
			get { return sync_root; }
		}

		protected override IntPtr GetAddress( string funcname ) {
			return Wgl.Imports.GetProcAddress( funcname );
		}

		public static void LoadAll() {
			new Wgl().LoadEntryPoints();
		}

		public static partial class Arb {
			
			public unsafe static bool SupportsExtension(WinGLContext context, string ext) {
				if (Wgl.Delegates.wglGetExtensionsStringARB != null) {					
					IntPtr ansiPtr = Delegates.wglGetExtensionsStringARB(context.DeviceContext);
					string extensions = new string((sbyte*)ansiPtr);
					if( String.IsNullOrEmpty( extensions ) ) 
						return false;

					return extensions.Contains( ext );
				}
				return false;
			}
		}

		public static partial class Ext {
			
			public unsafe static bool SupportsExtension(string ext) {
				if (Wgl.Delegates.wglGetExtensionsStringEXT != null) {				
					IntPtr ansiPtr = Delegates.wglGetExtensionsStringEXT();
					string extensions = new string((sbyte*)ansiPtr);
					if( String.IsNullOrEmpty( extensions ) ) 
						return false;

					return extensions.Contains( ext );
				}
				return false;
			}
		}
	}
}
