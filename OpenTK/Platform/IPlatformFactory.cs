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
using OpenTK.Graphics;

namespace OpenTK.Platform {
	
	interface IPlatformFactory {
		INativeWindow CreateNativeWindow(int x, int y, int width, int height, string title, GraphicsMode mode, GameWindowFlags options, DisplayDevice device);

		IDisplayDeviceDriver CreateDisplayDeviceDriver();

		IGraphicsContext CreateGLContext(GraphicsMode mode, IWindowInfo window);
	}
	
	internal static class Factory {
		public static readonly IPlatformFactory Default;

		static Factory() {
			if (Configuration.RunningOnWindows) Default = new Windows.WinFactory();
			else if (Configuration.RunningOnMacOS) Default = new MacOS.MacOSFactory();
			else if (Configuration.RunningOnX11) Default = new X11.X11Factory();
			else throw new NotSupportedException( "Running on an unsupported platform, please refer to http://www.opentk.com for more information." );
		}
	}
}

namespace OpenTK.Platform.MacOS {
	class MacOSFactory : IPlatformFactory {
		
		public INativeWindow CreateNativeWindow(int x, int y, int width, int height, string title, GraphicsMode mode, GameWindowFlags options, DisplayDevice device) {
			return new CarbonGLNative(x, y, width, height, title, options, device);
		}

		public IDisplayDeviceDriver CreateDisplayDeviceDriver() {
			return new QuartzDisplayDeviceDriver();
		}

		public IGraphicsContext CreateGLContext(GraphicsMode mode, IWindowInfo window) {
			return new AglContext(mode, window);
		}
	}
}

namespace OpenTK.Platform.Windows {
	class WinFactory : IPlatformFactory {
		
		public INativeWindow CreateNativeWindow(int x, int y, int width, int height, string title, GraphicsMode mode, GameWindowFlags options, DisplayDevice device) {
			return new WinGLNative(x, y, width, height, title, options, device);
		}

		public IDisplayDeviceDriver CreateDisplayDeviceDriver() {
			return new WinDisplayDeviceDriver();
		}

		public IGraphicsContext CreateGLContext(GraphicsMode mode, IWindowInfo window) {
			return new WinGLContext(mode, (WinWindowInfo)window);
		}
	}
}

namespace OpenTK.Platform.X11 {
	class X11Factory : IPlatformFactory {

		public INativeWindow CreateNativeWindow(int x, int y, int width, int height, string title, GraphicsMode mode, GameWindowFlags options, DisplayDevice device) {
			return new X11GLNative(x, y, width, height, title, mode, options, device);
		}

		public IDisplayDeviceDriver CreateDisplayDeviceDriver() {
			return new X11DisplayDevice();
		}

		public IGraphicsContext CreateGLContext(GraphicsMode mode, IWindowInfo window) {
			return new X11GLContext(mode, window);
		}
	}
}

