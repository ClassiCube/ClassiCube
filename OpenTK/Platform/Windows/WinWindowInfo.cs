#region License
//
// The Open Toolkit Library License
//
// Copyright (c) 2006 - 2009 the Open Toolkit library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
#endregion

using System;
using System.Runtime.InteropServices;

namespace OpenTK.Platform.Windows {
	
	/// \internal
	/// <summary>Describes a win32 window.</summary>
	public sealed class WinWindowInfo : IWindowInfo {
		internal IntPtr dc, handle;
		bool disposed;

		/// <summary> Constructs a new instance with the specified window handle and parent. </summary>
		/// <param name="handle">The window handle for this instance.</param>
		public WinWindowInfo(IntPtr windowHandle) {
			handle = windowHandle;
		}

		/// <summary> Gets the handle of the window. </summary>
		public IntPtr WindowHandle { get { return handle; } }

		/// <summary> Gets the device context for this window instance. </summary>
		public IntPtr DeviceContext {
			get {
				if (dc == IntPtr.Zero) dc = API.GetDC(this.handle);
				//dc = Functions.GetWindowDC(this.WindowHandle);
				return dc;
			}
		}

		/// <summary>Releases the unmanaged resources consumed by this instance.</summary>
		public void Dispose() {
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		void Dispose(bool manual) {
			if (!disposed) {
				if (dc != IntPtr.Zero && !API.ReleaseDC(handle, dc)) {
					Debug.Print("[Warning] Failed to release device context {0}. Windows error: {1}.", this.dc, Marshal.GetLastWin32Error());
				}

				disposed = true;
			}
		}

		~WinWindowInfo() { Dispose(false); }
		
		public IntPtr WinHandle { get { return handle; } }
	}
}
