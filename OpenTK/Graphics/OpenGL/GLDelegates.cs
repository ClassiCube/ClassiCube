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
		
		public static partial class Delegates {
			[SuppressUnmanagedCodeSecurity()]
			public delegate void AlphaFunc(AlphaFunction func, Single @ref);
			public static AlphaFunc glAlphaFunc;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void BindBuffer(BufferTarget target, Int32 buffer);
			public static BindBuffer glBindBuffer;
			public static BindBuffer glBindBufferARB;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void BindTexture(TextureTarget target, Int32 texture);
			public static BindTexture glBindTexture;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void BlendFunc(BlendingFactor sfactor, BlendingFactor dfactor);
			public static BlendFunc glBlendFunc;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void BufferData(BufferTarget target, IntPtr size, IntPtr data, BufferUsageHint usage);
			public static BufferData glBufferData;
			public static BufferData glBufferDataARB;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void BufferSubData(BufferTarget target, IntPtr offset, IntPtr size, IntPtr data);
			public static BufferSubData glBufferSubData;
			public static BufferSubData glBufferSubDataARB;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void Clear(ClearBufferMask mask);
			public static Clear glClear;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void ClearColor(Single red, Single green, Single blue, Single alpha);
			public static ClearColor glClearColor;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void ClearDepth(Double depth);
			public static ClearDepth glClearDepth;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void ColorMask(bool red, bool green, bool blue, bool alpha);
			public static ColorMask glColorMask;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void ColorPointer(Int32 size, PointerType type, Int32 stride, IntPtr pointer);
			public static ColorPointer glColorPointer;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void CullFace(CullFaceMode mode);
			public static CullFace glCullFace;

			[SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void DeleteBuffers(Int32 n, Int32* buffers);
			public static DeleteBuffers glDeleteBuffers;
			public static DeleteBuffers glDeleteBuffersARB;

			[SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void DeleteTextures(Int32 n, Int32* textures);
			public static DeleteTextures glDeleteTextures;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void DepthFunc(DepthFunction func);
			public static DepthFunc glDepthFunc;
			[SuppressUnmanagedCodeSecurity()]
			public delegate void DepthMask(bool flag);
			public static DepthMask glDepthMask;
			[SuppressUnmanagedCodeSecurity()]
			public delegate void DepthRange(Double near, Double far);
			public static DepthRange glDepthRange;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void Disable(EnableCap cap);
			public static Disable glDisable;
			[SuppressUnmanagedCodeSecurity()]
			public delegate void DisableClientState(ArrayCap array);
			public static DisableClientState glDisableClientState;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void DrawArrays(BeginMode mode, Int32 first, Int32 count);
			public static DrawArrays glDrawArrays;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void DrawElements(BeginMode mode, Int32 count, DrawElementsType type, IntPtr indices);
			public static DrawElements glDrawElements;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void Enable(EnableCap cap);
			public static Enable glEnable;
			[SuppressUnmanagedCodeSecurity()]
			public delegate void EnableClientState(ArrayCap array);
			public static EnableClientState glEnableClientState;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void Fogf(FogParameter pname, Single param);
			public static Fogf glFogf;

			[SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void Fogfv(FogParameter pname, Single* @params);
			public static Fogfv glFogfv;
			[SuppressUnmanagedCodeSecurity()]
			public delegate void Fogi(FogParameter pname, Int32 param);
			public static Fogi glFogi;
			
			[SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void GenBuffers(Int32 n, [OutAttribute] Int32* buffers);
			public static GenBuffers glGenBuffers;
			public static GenBuffers glGenBuffersARB;
			
			[SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void GenTextures(Int32 n, [OutAttribute] Int32* textures);
			public static GenTextures glGenTextures;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate ErrorCode GetError();
			public static GetError glGetError;

			[SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void GetFloatv(GetPName pname, [OutAttribute] Single* @params);
			public static GetFloatv glGetFloatv;

			[SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void GetIntegerv(GetPName pname, [OutAttribute] Int32* @params);
			public static GetIntegerv glGetIntegerv;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate System.IntPtr GetString(StringName name);
			public static GetString glGetString;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void Hint(HintTarget target, HintMode mode);
			public static Hint glHint;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate bool IsBuffer(Int32 buffer);
			public static IsBuffer glIsBuffer;
			public static IsBuffer glIsBufferARB;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate bool IsTexture(Int32 texture);
			public static IsTexture glIsTexture;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void LoadIdentity();
			public static LoadIdentity glLoadIdentity;

			[SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void LoadMatrixf(Single* m);
			public static LoadMatrixf glLoadMatrixf;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void MatrixMode(OpenGL.MatrixMode mode);
			public static MatrixMode glMatrixMode;
			
			[SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void MultMatrixf(Single* m);
			public static MultMatrixf glMultMatrixf;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void PopMatrix();
			public static PopMatrix glPopMatrix;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void PushMatrix();
			public static PushMatrix glPushMatrix;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void ReadPixels(Int32 x, Int32 y, Int32 width, Int32 height, PixelFormat format, PixelType type, [OutAttribute] IntPtr pixels);
			public static ReadPixels glReadPixels;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void ShadeModel(ShadingModel mode);
			public static ShadeModel glShadeModel;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void TexCoordPointer(Int32 size, PointerType type, Int32 stride, IntPtr pointer);
			public static TexCoordPointer glTexCoordPointer;

			[SuppressUnmanagedCodeSecurity()]
			public delegate void TexImage2D(TextureTarget target, Int32 level, PixelInternalFormat publicformat, Int32 width, Int32 height, Int32 border, PixelFormat format, PixelType type, IntPtr pixels);
			public static TexImage2D glTexImage2D;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void TexParameteri(TextureTarget target, TextureParameterName pname, Int32 param);
			public static TexParameteri glTexParameteri;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void TexSubImage2D(TextureTarget target, Int32 level, Int32 xoffset, Int32 yoffset, Int32 width, Int32 height, PixelFormat format, PixelType type, IntPtr pixels);
			public static TexSubImage2D glTexSubImage2D;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void VertexPointer(Int32 size, PointerType type, Int32 stride, IntPtr pointer);
			public static VertexPointer glVertexPointer;
			
			[SuppressUnmanagedCodeSecurity()]
			public delegate void Viewport(Int32 x, Int32 y, Int32 width, Int32 height);
			public static Viewport glViewport;
		}
	}
}
