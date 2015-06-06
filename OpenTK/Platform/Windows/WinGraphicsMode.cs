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
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Runtime.InteropServices;

using OpenTK.Graphics;
using ColorDepth = OpenTK.Graphics.ColorFormat;

namespace OpenTK.Platform.Windows
{
    internal class WinGraphicsMode : IGraphicsMode
    {
        #region --- IGraphicsMode Members ---

        public GraphicsMode SelectGraphicsMode(ColorDepth color, int depth, int stencil, int samples,
                                               int buffers, bool stereo)
        {
            using (INativeWindow native_window = new NativeWindow())
            {
                WinWindowInfo window = (WinWindowInfo)native_window.WindowInfo;
                IntPtr deviceContext = window.DeviceContext;
                Debug.WriteLine(String.Format("Device context: {0}", deviceContext));

                Debug.Write("Selecting pixel format PFD... ");
                PixelFormatDescriptor pixelFormat = new PixelFormatDescriptor();
                pixelFormat.Size = API.PixelFormatDescriptorSize;
                pixelFormat.Version = API.PixelFormatDescriptorVersion;
                pixelFormat.Flags =
                    PixelFormatDescriptorFlags.SUPPORT_OPENGL |
                    PixelFormatDescriptorFlags.DRAW_TO_WINDOW;
                pixelFormat.ColorBits = (byte)(color.Red + color.Green + color.Blue);

                pixelFormat.PixelType = color.IsIndexed ? PixelType.INDEXED : PixelType.RGBA;
                pixelFormat.RedBits = (byte)color.Red;
                pixelFormat.GreenBits = (byte)color.Green;
                pixelFormat.BlueBits = (byte)color.Blue;
                pixelFormat.AlphaBits = (byte)color.Alpha;

                pixelFormat.DepthBits = (byte)depth;
                pixelFormat.StencilBits = (byte)stencil;

                if (depth <= 0) pixelFormat.Flags |= PixelFormatDescriptorFlags.DEPTH_DONTCARE;
                if (stereo) pixelFormat.Flags |= PixelFormatDescriptorFlags.STEREO;
                if (buffers > 1) pixelFormat.Flags |= PixelFormatDescriptorFlags.DOUBLEBUFFER;

                int pixel = Functions.ChoosePixelFormat(deviceContext, ref pixelFormat);
                if (pixel == 0)
                    throw new GraphicsModeException("The requested GraphicsMode is not available.");

                // Find out what we really got as a format:
                PixelFormatDescriptor pfd = new PixelFormatDescriptor();
                pixelFormat.Size = API.PixelFormatDescriptorSize;
                pixelFormat.Version = API.PixelFormatDescriptorVersion;
                Functions.DescribePixelFormat(deviceContext, pixel, API.PixelFormatDescriptorSize, ref pfd);
                return new GraphicsMode((IntPtr)pixel,
                    new ColorDepth(pfd.RedBits, pfd.GreenBits, pfd.BlueBits, pfd.AlphaBits),
                    pfd.DepthBits,
                    pfd.StencilBits,
                    0,
                    (pfd.Flags & PixelFormatDescriptorFlags.DOUBLEBUFFER) != 0 ? 2 : 1,
                    (pfd.Flags & PixelFormatDescriptorFlags.STEREO) != 0);
            }
        }

        #endregion
    }
}

