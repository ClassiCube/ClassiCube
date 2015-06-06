namespace OpenTK.Platform.Windows
{
	using System;
	using System.Runtime.InteropServices;
	#pragma warning disable 0649
    #pragma warning disable 3019
    #pragma warning disable 1591

	partial class Wgl
	{
		public static partial class Ext
		{
			public static Boolean SwapInterval(int interval) {
				return Delegates.wglSwapIntervalEXT((int)interval);
			}

			public static int GetSwapInterval() {
				return Delegates.wglGetSwapIntervalEXT();
			}
		}
		
		internal static partial class Delegates
        {
            [System.Security.SuppressUnmanagedCodeSecurity()]
            internal delegate IntPtr GetExtensionsStringARB(IntPtr hdc);
            internal static GetExtensionsStringARB wglGetExtensionsStringARB;

            [System.Security.SuppressUnmanagedCodeSecurity()]
            internal delegate IntPtr GetExtensionsStringEXT();
            internal static GetExtensionsStringEXT wglGetExtensionsStringEXT;
            [System.Security.SuppressUnmanagedCodeSecurity()]
            internal delegate Boolean SwapIntervalEXT(int interval);
            internal static SwapIntervalEXT wglSwapIntervalEXT;
            [System.Security.SuppressUnmanagedCodeSecurity()]
            internal delegate int GetSwapIntervalEXT();
            internal static GetSwapIntervalEXT wglGetSwapIntervalEXT;
        }
		
		internal static partial class Imports
        {
            [System.Security.SuppressUnmanagedCodeSecurity()]
            [System.Runtime.InteropServices.DllImport(Wgl.Library, EntryPoint = "wglCreateContext", ExactSpelling = true, SetLastError=true)]
            internal extern static IntPtr CreateContext(IntPtr hDc);
            [System.Security.SuppressUnmanagedCodeSecurity()]
            [System.Runtime.InteropServices.DllImport(Wgl.Library, EntryPoint = "wglDeleteContext", ExactSpelling = true, SetLastError = true)]
            internal extern static Boolean DeleteContext(IntPtr oldContext);
            [System.Security.SuppressUnmanagedCodeSecurity()]
            [System.Runtime.InteropServices.DllImport(Wgl.Library, EntryPoint = "wglGetCurrentContext", ExactSpelling = true, SetLastError=true)]
            internal extern static IntPtr GetCurrentContext();
            [System.Security.SuppressUnmanagedCodeSecurity()]
            [System.Runtime.InteropServices.DllImport(Wgl.Library, EntryPoint = "wglMakeCurrent", ExactSpelling = true, SetLastError=true)]
            internal extern static Boolean MakeCurrent(IntPtr hDc, IntPtr newContext);

            [System.Security.SuppressUnmanagedCodeSecurity()]
            [System.Runtime.InteropServices.DllImport(Wgl.Library, EntryPoint = "wglGetCurrentDC", ExactSpelling = true, SetLastError = true)]
            internal extern static IntPtr GetCurrentDC();

            [System.Security.SuppressUnmanagedCodeSecurity()]
            [System.Runtime.InteropServices.DllImport(Wgl.Library, EntryPoint = "wglGetProcAddress", ExactSpelling = true, SetLastError = true)]
            internal extern static IntPtr GetProcAddress(String lpszProc);
        }
	}
}
