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

	public class PlatformException : Exception {
		public PlatformException(string s) : base(s) { }
	}
	
	public interface IPlatformFactory {
		INativeWindow CreateWindow(int x, int y, int width, int height, string title, GraphicsMode mode, DisplayDevice device);

		void InitDisplayDeviceDriver();

		IGraphicsContext CreateGLContext(GraphicsMode mode, IWindowInfo window);
	}
	
	public static class Factory {
		public static readonly IPlatformFactory Default;
		
		public static INativeWindow CreateWindow(int width, int height, string title, GraphicsMode mode, DisplayDevice device) {
			int x = device.Bounds.Left + (device.Bounds.Width  - width)  / 2;
			int y = device.Bounds.Top  + (device.Bounds.Height - height) / 2;
			return Default.CreateWindow(x, y, width, height, title, mode, device);
		}

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
		
		public INativeWindow CreateWindow(int x, int y, int width, int height, string title, GraphicsMode mode, DisplayDevice device) {
			return new CarbonWindow(x, y, width, height, title, device);
		}

		public void InitDisplayDeviceDriver() {
			QuartzDisplayDevice.Init();
		}

		public IGraphicsContext CreateGLContext(GraphicsMode mode, IWindowInfo window) {
			return new AglContext(mode, window);
		}
	}
}

namespace OpenTK.Platform.Windows {
	class WinFactory : IPlatformFactory {
		
		public INativeWindow CreateWindow(int x, int y, int width, int height, string title, GraphicsMode mode, DisplayDevice device) {
			return new WinWindow(x, y, width, height, title, device);
		}

		public void InitDisplayDeviceDriver() {
			WinDisplayDevice.Init();
		}

		public IGraphicsContext CreateGLContext(GraphicsMode mode, IWindowInfo window) {
			return new WinGLContext(mode, (WinWindowInfo)window);
		}
	}
}

namespace OpenTK.Platform.X11 {
	class X11Factory : IPlatformFactory {

		public INativeWindow CreateWindow(int x, int y, int width, int height, string title, GraphicsMode mode, DisplayDevice device) {
			return new X11Window(x, y, width, height, title, mode, device);
		}

		public void InitDisplayDeviceDriver() {
			X11DisplayDevice.Init();
		}

		public IGraphicsContext CreateGLContext(GraphicsMode mode, IWindowInfo window) {
			return new X11GLContext(mode, window);
		}
	}
}

