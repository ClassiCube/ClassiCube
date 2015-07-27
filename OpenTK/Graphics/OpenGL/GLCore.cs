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
using System.Security;

namespace OpenTK.Graphics.OpenGL {
	
    partial class GL {

        internal static class Core {           
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glAlphaFunc(AlphaFunction func, Single @ref);
           
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glBindBuffer(BufferTarget target, Int32 buffer);
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glBindBufferARB(BufferTarget target, Int32 buffer);
           
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glBindTexture(TextureTarget target, Int32 texture);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glBlendFunc(BlendingFactor sfactor, BlendingFactor dfactor);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glBufferData(BufferTarget target, IntPtr size, IntPtr data, BufferUsageHint usage);
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glBufferDataARB(BufferTarget target, IntPtr size, IntPtr data, BufferUsageHint usage);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glBufferSubData(BufferTarget target, IntPtr offset, IntPtr size, IntPtr data);
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glBufferSubDataARB(BufferTarget target, IntPtr offset, IntPtr size, IntPtr data);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glClear(ClearBufferMask mask);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glClearColor(Single red, Single green, Single blue, Single alpha);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glClearDepth(Double depth);          
           
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glColorMask(bool red, bool green, bool blue, bool alpha);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glColorPointer(Int32 size, PointerType type, Int32 stride, IntPtr pointer);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glCullFace(CullFaceMode mode);
           
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glDeleteBuffers(Int32 n, Int32* buffers);
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glDeleteBuffersARB(Int32 n, Int32* buffers);
            
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glDeleteTextures(Int32 n, Int32* textures);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glDepthFunc(DepthFunction func);
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glDepthMask(bool flag);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glDisable(EnableCap cap);
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glDisableClientState(ArrayCap array);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glDrawArrays(BeginMode mode, Int32 first, Int32 count);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glDrawElements(BeginMode mode, Int32 count, DrawElementsType type, IntPtr indices);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glEnable(EnableCap cap);
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glEnableClientState(ArrayCap array);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glFogf(FogParameter pname, Single param);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glFogfv(FogParameter pname, Single* @params);
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glFogi(FogParameter pname, Int32 param);
           
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glGenBuffers(Int32 n, [OutAttribute] Int32* buffers);
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glGenBuffersARB(Int32 n, [OutAttribute] Int32* buffers);
           
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glGenTextures(Int32 n, [OutAttribute] Int32* textures);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static ErrorCode glGetError();
            
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glGetFloatv(GetPName pname, [OutAttribute] Single* @params);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glGetIntegerv(GetPName pname, [OutAttribute] Int32* @params);
          
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static System.IntPtr glGetString(StringName name);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glHint(HintTarget target, HintMode mode);
           
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static bool glIsBuffer(Int32 buffer);
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static bool glIsBufferARB(Int32 buffer);
            
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static bool glIsTexture(Int32 texture);
            
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glLoadIdentity();

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glLoadMatrixf(Single* m);
            
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glMatrixMode(OpenGL.MatrixMode mode);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static unsafe void glMultMatrixf(Single* m);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glPopMatrix();

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glPushMatrix();

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glReadPixels(Int32 x, Int32 y, Int32 width, Int32 height, PixelFormat format, PixelType type, [OutAttribute] IntPtr pixels);      
            
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glShadeModel(ShadingModel mode);     

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glTexCoordPointer(Int32 size, PointerType type, Int32 stride, IntPtr pointer);
            
           	[SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glTexImage2D(TextureTarget target, Int32 level, PixelInternalFormat internalformat, Int32 width, Int32 height, Int32 border, PixelFormat format, PixelType type, IntPtr pixels);
           
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glTexParameterf(TextureTarget target, TextureParameterName pname, Single param);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glTexParameteri(TextureTarget target, TextureParameterName pname, Int32 param);
           
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glTexSubImage2D(TextureTarget target, Int32 level, Int32 xoffset, Int32 yoffset, Int32 width, Int32 height, PixelFormat format, PixelType type, IntPtr pixels);
            
            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glVertexPointer(Int32 size, PointerType type, Int32 stride, IntPtr pointer);

            [SuppressUnmanagedCodeSecurity, DllImport(Library)]
            internal extern static void glViewport(Int32 x, Int32 y, Int32 width, Int32 height);
        }
    }
}
