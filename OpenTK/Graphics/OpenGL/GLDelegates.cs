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

namespace OpenTK.Graphics.OpenGL
{
	using System;
	using System.Text;
	using System.Runtime.InteropServices;
	#pragma warning disable 0649
	#pragma warning disable 3019
	#pragma warning disable 1591

	partial class GL
	{
		public static partial class Delegates
		{
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void AlphaFunc(AlphaFunction func, Single @ref);
			public static AlphaFunc glAlphaFunc;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void BindBuffer(BufferTarget target, Int32 buffer);
			public static BindBuffer glBindBuffer;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void BindBufferARB(BufferTargetArb target, Int32 buffer);
			public static BindBufferARB glBindBufferARB;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void BindTexture(TextureTarget target, Int32 texture);
			public static BindTexture glBindTexture;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void BlendFunc(BlendingFactor sfactor, BlendingFactor dfactor);
			public static BlendFunc glBlendFunc;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void BufferData(BufferTarget target, IntPtr size, IntPtr data, BufferUsageHint usage);
			public static BufferData glBufferData;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void BufferDataARB(BufferTargetArb target, IntPtr size, IntPtr data, BufferUsageArb usage);
			public static BufferDataARB glBufferDataARB;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void BufferSubData(BufferTarget target, IntPtr offset, IntPtr size, IntPtr data);
			public static BufferSubData glBufferSubData;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void BufferSubDataARB(BufferTargetArb target, IntPtr offset, IntPtr size, IntPtr data);
			public static BufferSubDataARB glBufferSubDataARB;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void Clear(ClearBufferMask mask);
			public static Clear glClear;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void ClearColor(Single red, Single green, Single blue, Single alpha);
			public static ClearColor glClearColor;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void ClearDepth(Double depth);
			public static ClearDepth glClearDepth;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void ColorMask(bool red, bool green, bool blue, bool alpha);
			public static ColorMask glColorMask;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void ColorPointer(Int32 size, ColorPointerType type, Int32 stride, IntPtr pointer);
			public static ColorPointer glColorPointer;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void CullFace(CullFaceMode mode);
			public static CullFace glCullFace;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void DeleteBuffers(Int32 n, Int32* buffers);
			public unsafe static DeleteBuffers glDeleteBuffers;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void DeleteBuffersARB(Int32 n, Int32* buffers);
			public unsafe static DeleteBuffersARB glDeleteBuffersARB;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void DeleteTextures(Int32 n, Int32* textures);
			public unsafe static DeleteTextures glDeleteTextures;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void DepthFunc(DepthFunction func);
			public static DepthFunc glDepthFunc;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void DepthMask(bool flag);
			public static DepthMask glDepthMask;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void DepthRange(Double near, Double far);
			public static DepthRange glDepthRange;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void Disable(EnableCap cap);
			public static Disable glDisable;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void DisableClientState(ArrayCap array);
			public static DisableClientState glDisableClientState;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void DrawArrays(BeginMode mode, Int32 first, Int32 count);
			public static DrawArrays glDrawArrays;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void DrawElements(BeginMode mode, Int32 count, DrawElementsType type, IntPtr indices);
			public static DrawElements glDrawElements;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void Enable(EnableCap cap);
			public static Enable glEnable;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void EnableClientState(ArrayCap array);
			public static EnableClientState glEnableClientState;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void Finish();
			public static Finish glFinish;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void Flush();
			public static Flush glFlush;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void Fogf(FogParameter pname, Single param);
			public static Fogf glFogf;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void Fogfv(FogParameter pname, Single* @params);
			public unsafe static Fogfv glFogfv;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void Fogi(FogParameter pname, Int32 param);
			public static Fogi glFogi;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void Fogiv(FogParameter pname, Int32* @params);
			public unsafe static Fogiv glFogiv;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void FrontFace(FrontFaceDirection mode);
			public static FrontFace glFrontFace;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void GenBuffers(Int32 n, [OutAttribute] Int32* buffers);
			public unsafe static GenBuffers glGenBuffers;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void GenBuffersARB(Int32 n, [OutAttribute] Int32* buffers);
			public unsafe static GenBuffersARB glGenBuffersARB;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void GenTextures(Int32 n, [OutAttribute] Int32* textures);
			public unsafe static GenTextures glGenTextures;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate ErrorCode GetError();
			public static GetError glGetError;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void GetFloatv(GetPName pname, [OutAttribute] Single* @params);
			public unsafe static GetFloatv glGetFloatv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void GetIntegerv(GetPName pname, [OutAttribute] Int32* @params);
			public unsafe static GetIntegerv glGetIntegerv;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate System.IntPtr GetString(StringName name);
			public static GetString glGetString;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void Hint(HintTarget target, HintMode mode);
			public static Hint glHint;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate bool IsBuffer(Int32 buffer);
			public static IsBuffer glIsBuffer;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate bool IsBufferARB(Int32 buffer);
			public static IsBufferARB glIsBufferARB;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate bool IsTexture(Int32 texture);
			public static IsTexture glIsTexture;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void LoadIdentity();
			public static LoadIdentity glLoadIdentity;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void LoadMatrixf(Single* m);
			public unsafe static LoadMatrixf glLoadMatrixf;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void MatrixMode(OpenTK.Graphics.OpenGL.MatrixMode mode);
			public static MatrixMode glMatrixMode;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public unsafe delegate void MultMatrixf(Single* m);
			public unsafe static MultMatrixf glMultMatrixf;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void PolygonMode(MaterialFace face, OpenTK.Graphics.OpenGL.PolygonMode mode);
			public static PolygonMode glPolygonMode;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void PopMatrix();
			public static PopMatrix glPopMatrix;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void PushMatrix();
			public static PushMatrix glPushMatrix;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void ReadPixels(Int32 x, Int32 y, Int32 width, Int32 height, PixelFormat format, PixelType type, [OutAttribute] IntPtr pixels);
			public static ReadPixels glReadPixels;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void ShadeModel(ShadingModel mode);
			public static ShadeModel glShadeModel;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void TexCoordPointer(Int32 size, TexCoordPointerType type, Int32 stride, IntPtr pointer);
			public static TexCoordPointer glTexCoordPointer;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void TexImage2D(TextureTarget target, Int32 level, PixelInternalFormat publicformat, Int32 width, Int32 height, Int32 border, PixelFormat format, PixelType type, IntPtr pixels);
			public static TexImage2D glTexImage2D;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void TexParameterf(TextureTarget target, TextureParameterName pname, Single param);
			public static TexParameterf glTexParameterf;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void TexParameteri(TextureTarget target, TextureParameterName pname, Int32 param);
			public static TexParameteri glTexParameteri;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void TexSubImage2D(TextureTarget target, Int32 level, Int32 xoffset, Int32 yoffset, Int32 width, Int32 height, PixelFormat format, PixelType type, IntPtr pixels);
			public static TexSubImage2D glTexSubImage2D;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void VertexPointer(Int32 size, VertexPointerType type, Int32 stride, IntPtr pointer);
			public static VertexPointer glVertexPointer;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			public delegate void Viewport(Int32 x, Int32 y, Int32 width, Int32 height);
			public static Viewport glViewport;
		}
	}
}
