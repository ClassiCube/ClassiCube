#if !USE_DX
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
using OpenTK;

namespace OpenTK.Graphics.OpenGL {
	
	[InteropPatch]
	public unsafe static class GL {
		
		public static void AlphaFunc(Compare func, float value) {
			Interop.Calli((int)func, value, AlphaFuncAddress);
		} static IntPtr AlphaFuncAddress;
		
		public static void Begin(BeginMode mode) {
			Interop.Calli((int)mode, BeginAddress);
		} static IntPtr BeginAddress;

		public static void BindBuffer(BufferTarget target, int buffer) {
			Interop.Calli((int)target, buffer, BindBufferAddress);
		} static IntPtr BindBufferAddress, BindBufferARBAddress;

		public static void BindTexture(TextureTarget target, int texture) {
			Interop.Calli((int)target, texture, BindTextureAddress);
		} static IntPtr BindTextureAddress;

		public static void BlendFunc(BlendingFactor sfactor, BlendingFactor dfactor) {
			Interop.Calli((int)sfactor, (int)dfactor, BlendFuncAddress);
		} static IntPtr BlendFuncAddress;
		
		public static void BufferData(BufferTarget target, IntPtr size, IntPtr data, BufferUsage usage) {
			Interop.Calli((int)target, size, data, (int)usage, BufferDataAddress);
		} static IntPtr BufferDataAddress, BufferDataARBAddress;
		
		public static void BufferSubData(BufferTarget target, IntPtr offset, IntPtr size, IntPtr data) {
			Interop.Calli((int)target, offset, size, data, BufferSubDataAddress);
		} static IntPtr BufferSubDataAddress, BufferSubDataARBAddress;
		
		public static void CallList(int list) {
			Interop.Calli(list, CallListAddress);
		} static IntPtr CallListAddress;

		public static void Clear(ClearBufferMask mask) {
			Interop.Calli((int)mask, ClearAddress);
		} static IntPtr ClearAddress;

		public static void ClearColor(float red, float green, float blue, float alpha) {
			Interop.Calli(red, green, blue, alpha, ClearColorAddress);
		} static IntPtr ClearColorAddress;
		
		public static void Color4ub(byte red, byte green, byte blue, byte alpha) {
			Interop.Calli(red, green, blue, alpha, Color4ubAddress);
		} static IntPtr Color4ubAddress;
		
		public static void ColorMask(bool red, bool green, bool blue, bool alpha) {
			Interop.Calli(red ? (byte)1 : (byte)0, green ? (byte)1 : (byte)0, blue ? (byte)1 : (byte)0, alpha ? (byte)1 : (byte)0, ColorMaskAddress);
		} static IntPtr ColorMaskAddress;

		public static void ColorPointer(int size, PointerType type, int stride, IntPtr pointer) {
			Interop.Calli(size, (int)type, stride, pointer, ColorPointerAddress);
		} static IntPtr ColorPointerAddress;

		public static void CullFace(CullFaceMode mode) {
			Interop.Calli((int)mode, CullFaceAddress);
		} static IntPtr CullFaceAddress;

		public static void DeleteBuffers(int n, int* buffers) {
			Interop.Calli(n, buffers, DeleteBuffersAddress);
		} static IntPtr DeleteBuffersAddress, DeleteBuffersARBAddress;

		public static void DeleteLists(int list, int n) {
			Interop.Calli(list, n, DeleteListsAddress);
		} static IntPtr DeleteListsAddress;
		
		public static void DeleteTextures(int n, int* textures) {
			Interop.Calli(n, textures, DeleteTexturesAddress);
		} static IntPtr DeleteTexturesAddress;
		
		public static void DepthFunc(Compare func) {
			Interop.Calli((int)func, DepthFuncAddress);
		} static IntPtr DepthFuncAddress;
		
		public static void DepthMask(bool flag) {
			Interop.Calli(flag ? (byte)1 : (byte)0, DepthMaskAddress);
		} static IntPtr DepthMaskAddress;

		public static void Disable(EnableCap cap) {
			Interop.Calli((int)cap, DisableAddress);
		} static IntPtr DisableAddress;
		
		public static void DisableClientState(ArrayCap cap) {
			Interop.Calli((int)cap, DisableClientStateAddress);
		} static IntPtr DisableClientStateAddress;
		
		public static void DrawArrays(BeginMode mode, int first, int count) {
			Interop.Calli((int)mode, first, count, DrawArraysAddress);
		} static IntPtr DrawArraysAddress;
		
		public static void DrawElements(BeginMode mode, int count, DrawElementsType type, IntPtr indices) {
			Interop.Calli((int)mode, count, (int)type, indices, DrawElementsAddress);
		} static IntPtr DrawElementsAddress;

		public static void Enable(EnableCap cap) {
			Interop.Calli((int)cap, EnableAddress);
		} static IntPtr EnableAddress;
		
		public static void EnableClientState(ArrayCap cap) {
			Interop.Calli((int)cap, EnableClientStateAddress);
		} static IntPtr EnableClientStateAddress;
		
		public static void End() {
			Interop.Calli(EndAddress);
		} static IntPtr EndAddress;
		
		public static void EndList() {
			Interop.Calli(EndListAddress);
		} static IntPtr EndListAddress;

		public static void Fogf(FogParameter pname, float param) {
			Interop.Calli((int)pname, param, FogfAddress);
		} static IntPtr FogfAddress;

		public static void Fogfv(FogParameter pname, float* param) {
			Interop.Calli((int)pname, param, FogfvAddress);
		} static IntPtr FogfvAddress;
		
		public static void Fogi(FogParameter pname, int param) {
			Interop.Calli((int)pname, param, FogiAddress);
		} static IntPtr FogiAddress;
		
		public static void GenBuffers(int n, int* buffers) {
			Interop.Calli(n, buffers, GenBuffersAddress);
		} static IntPtr GenBuffersAddress, GenBuffersARBAddress;
		
		public static int GenLists(int n) {
			return Interop.Calli_Int32(n, GenListsAddress);
		} static IntPtr GenListsAddress;
		
		public static void GenTextures(int n, int* textures) {
			Interop.Calli(n, textures, GenTexturesAddress);
		} static IntPtr GenTexturesAddress;
		
		public static ErrorCode GetError() {
			return (ErrorCode)Interop.Calli_Int32(GetErrorAddress);
		} static IntPtr GetErrorAddress;

		public static void GetIntegerv(GetPName pname, int* @params) {
			Interop.Calli((int)pname, @params, GetIntegervAddress);
		} static IntPtr GetIntegervAddress;
		
		public static IntPtr GetString(StringName name) {
			return Interop.Calli_IntPtr((int)name, GetStringAddress);
		} static IntPtr GetStringAddress;
		
		public static void GetTexImage(TextureTarget target, int level, PixelFormat format, PixelType type, IntPtr pixels) {
			Interop.Calli((int)target, level, (int)format, (int)type, pixels, GetTexImageAddress);
		} static IntPtr GetTexImageAddress;

		public static void Hint(HintTarget target, HintMode mode) {
			Interop.Calli((int)target, (int)mode, HintAddress);
		} static IntPtr HintAddress;
		
		public static void LoadIdentity() {
			Interop.Calli(LoadIdentityAddress);
		} static IntPtr LoadIdentityAddress;

		public static void LoadMatrixf(float* m) {
			Interop.Calli(m, LoadMatrixfAddress);
		} static IntPtr LoadMatrixfAddress;

		public static void MatrixMode(OpenGL.MatrixMode mode) {
			Interop.Calli((int)mode, MatrixModeAddress);
		} static IntPtr MatrixModeAddress;
		
		public static void MultMatrixf(float* m) {
			Interop.Calli(m, MultMatrixfAddress);
		} static IntPtr MultMatrixfAddress;
		
		public static void NewList(int list, int mode) {
			Interop.Calli(list, mode, NewListAddress);
		} static IntPtr NewListAddress;
		
		public static void ReadPixels(int x, int y, int width, int height, PixelFormat format, PixelType type, IntPtr pixels) {
			Interop.Calli(x, y, width, height, (int)format, (int)type, pixels, ReadPixelsAddress);
		} static IntPtr ReadPixelsAddress;
		
		public static void TexCoord2f(float u, float v) {
			Interop.Calli(u, v, TexCoord2fAddress);
		} static IntPtr TexCoord2fAddress;
		
		public static void TexCoordPointer(int size, PointerType type, int stride, IntPtr pointer) {
			Interop.Calli(size, (int)type, stride, pointer, TexCoordPointerAddress);
		} static IntPtr TexCoordPointerAddress;

		public static void TexImage2D(TextureTarget target, int level, PixelInternalFormat publicformat,
		                              int width, int height, PixelFormat format, PixelType type, IntPtr pixels) {
			Interop.Calli((int)target, level, (int)publicformat, width, height, 0, (int)format, (int)type, pixels, TexImage2DAddress);
		} static IntPtr TexImage2DAddress;
		
		public static void TexParameteri(TextureTarget target, TextureParameterName pname, int param) {
			Interop.Calli((int)target, (int)pname, param, TexParameteriAddress);
		} static IntPtr TexParameteriAddress;
		
		public static void TexSubImage2D(TextureTarget target, int level, int xoffset, int yoffset,
		                                 int width, int height, PixelFormat format, PixelType type, IntPtr pixels) {
			Interop.Calli((int)target, level, xoffset, yoffset, width, height, (int)format, (int)type, pixels, TexSubImage2DAddress);
		} static IntPtr TexSubImage2DAddress;

		public static void Vertex3f(float x, float y, float z) {
			Interop.Calli(x, y, z, Vertex3fAddress);
		} static IntPtr Vertex3fAddress;
		
		public static void VertexPointer(int size, PointerType type, int stride, IntPtr pointer) {
			Interop.Calli(size, (int)type, stride, pointer, VertexPointerAddress);
		} static IntPtr VertexPointerAddress;

		public static void Viewport(int x, int y, int width, int height) {
			Interop.Calli(x, y, width, height, ViewportAddress);
		} static IntPtr ViewportAddress;
		
		
		internal static void LoadEntryPoints(IGraphicsContext context) {
			Debug.Print("Loading OpenGL function pointers... ");
			AlphaFuncAddress = context.GetAddress("glAlphaFunc");
			
			BeginAddress = context.GetAddress("glBegin");
			BindBufferAddress = context.GetAddress("glBindBuffer");
			BindBufferARBAddress = context.GetAddress("glBindBufferARB");
			BindTextureAddress = context.GetAddress("glBindTexture");
			BlendFuncAddress = context.GetAddress("glBlendFunc");
			BufferDataAddress = context.GetAddress("glBufferData");
			BufferDataARBAddress = context.GetAddress("glBufferDataARB");
			BufferSubDataAddress = context.GetAddress("glBufferSubData");
			BufferSubDataARBAddress = context.GetAddress("glBufferSubDataARB");
			
			CallListAddress = context.GetAddress("glCallList");
			ClearAddress = context.GetAddress("glClear");
			ClearColorAddress = context.GetAddress("glClearColor");
			Color4ubAddress = context.GetAddress("glColor4ub");
			ColorMaskAddress = context.GetAddress("glColorMask");
			ColorPointerAddress = context.GetAddress("glColorPointer");
			CullFaceAddress = context.GetAddress("glCullFace");
			
			DeleteBuffersAddress = context.GetAddress("glDeleteBuffers");
			DeleteBuffersARBAddress = context.GetAddress("glDeleteBuffersARB");
			DeleteListsAddress = context.GetAddress("glDeleteLists");
			DeleteTexturesAddress = context.GetAddress("glDeleteTextures");
			DepthFuncAddress = context.GetAddress("glDepthFunc");
			DepthMaskAddress = context.GetAddress("glDepthMask");
			DisableAddress = context.GetAddress("glDisable");
			DisableClientStateAddress = context.GetAddress("glDisableClientState");
			DrawArraysAddress = context.GetAddress("glDrawArrays");
			DrawElementsAddress = context.GetAddress("glDrawElements");
			
			EnableAddress = context.GetAddress("glEnable");
			EnableClientStateAddress = context.GetAddress("glEnableClientState");
			EndAddress = context.GetAddress("glEnd");
			EndListAddress = context.GetAddress("glEndList");
			FogfAddress = context.GetAddress("glFogf");
			FogfvAddress = context.GetAddress("glFogfv");
			FogiAddress = context.GetAddress("glFogi");
			
			GenBuffersAddress = context.GetAddress("glGenBuffers");
			GenBuffersARBAddress = context.GetAddress("glGenBuffersARB");
			GenListsAddress = context.GetAddress("glGenLists");
			GenTexturesAddress = context.GetAddress("glGenTextures");
			GetErrorAddress = context.GetAddress("glGetError");
			GetIntegervAddress = context.GetAddress("glGetIntegerv");
			GetStringAddress = context.GetAddress("glGetString");
			GetTexImageAddress = context.GetAddress("glGetTexImage");
			
			HintAddress = context.GetAddress("glHint");
			LoadIdentityAddress = context.GetAddress("glLoadIdentity");
			LoadMatrixfAddress = context.GetAddress("glLoadMatrixf");
			MatrixModeAddress = context.GetAddress("glMatrixMode");
			MultMatrixfAddress = context.GetAddress("glMultMatrixf");
			NewListAddress = context.GetAddress("glNewList");
			ReadPixelsAddress = context.GetAddress("glReadPixels");
			
			TexCoord2fAddress = context.GetAddress("glTexCoord2f");
			TexCoordPointerAddress = context.GetAddress("glTexCoordPointer");
			TexImage2DAddress = context.GetAddress("glTexImage2D");
			TexParameteriAddress = context.GetAddress("glTexParameteri");
			TexSubImage2DAddress = context.GetAddress("glTexSubImage2D");
			Vertex3fAddress = context.GetAddress("glVertex3f");
			VertexPointerAddress = context.GetAddress("glVertexPointer");
			ViewportAddress = context.GetAddress("glViewport");
		}
		
		public static void UseArbVboAddresses() {
			BindBufferAddress = BindBufferARBAddress;
			BufferDataAddress = BufferDataARBAddress;
			BufferSubDataAddress = BufferSubDataARBAddress;
			DeleteBuffersAddress = DeleteBuffersARBAddress;
			GenBuffersAddress = GenBuffersARBAddress;
		}
	}

	public enum ArrayCap : int {
		VertexArray = 0x8074,
		ColorArray = 0x8076,
		TextureCoordArray = 0x8078,
	}

	public enum BeginMode : int {
		Lines = 0x0001,
		Triangles = 0x0004,
	}

	public enum BlendingFactor : int {
		Zero = 0,
		SrcAlpha = 0x0302,
		OneMinusSrcAlpha = 0x0303,
		DstAlpha = 0x0304,
		OneMinusDstAlpha = 0x0305,
		One = 1,
	}

	public enum BufferTarget : int {
		ArrayBuffer = 0x8892,
		ElementArrayBuffer = 0x8893,
	}

	public enum BufferUsage : int {
		StreamDraw = 0x88E0,
		StaticDraw = 0x88E4,
		DynamicDraw = 0x88E8,
	}

	[Flags]
	public enum ClearBufferMask : int {
		DepthBufferBit = 0x00000100,
		ColorBufferBit = 0x00004000,
	}

	public enum PointerType : int {
		UnsignedByte = 0x1401,
		Float = 0x1406,
	}

	public enum CullFaceMode : int {
		Front = 0x0404,
		Back = 0x0405,
		FrontAndBack = 0x0408,
	}

	public enum Compare : int {
		Never = 0x0200,
		Less = 0x0201,
		Equal = 0x0202,
		Lequal = 0x0203,
		Greater = 0x0204,
		Notequal = 0x0205,
		Gequal = 0x0206,
		Always = 0x0207,
	}

	public enum DrawElementsType : int {
		UnsignedShort = 0x1403,
	}

	public enum EnableCap : int {
		CullFace = 0x0B44,
		Fog = 0x0B60,
		DepthTest = 0x0B71,
		AlphaTest = 0x0BC0,
		Blend = 0x0BE2,
		Texture2D = 0x0DE1,

		VertexArray = 0x8074,
		ColorArray = 0x8076,
		TextureCoordArray = 0x8078,
	}

	public enum ErrorCode : int {
		NoError = 0,
		InvalidEnum = 0x0500,
		InvalidValue = 0x0501,
		InvalidOperation = 0x0502,
		StackOverflow = 0x0503,
		StackUnderflow = 0x0504,
		OutOfMemory = 0x0505,

		TextureTooLargeExt = 0x8065,
	}

	public enum FogMode : int {
		Exp = 0x0800,
		Exp2 = 0x0801,
		Linear = 0x2601,
	}

	public enum FogParameter : int {
		FogDensity = 0x0B62,
		FogEnd = 0x0B64,
		FogMode = 0x0B65,
		FogColor = 0x0B66,
	}

	public enum FrontFaceDirection : int {
		Cw = 0x0900,
		Ccw = 0x0901,
	}

	public enum GetPName : int {
		MaxTextureSize = 0x0D33,
		DepthBits = 0x0D56,
	}

	public enum HintMode : int {
		DontCare = 0x1100,
		Fastest = 0x1101,
		Nicest = 0x1102,
	}

	public enum HintTarget : int {
		PerspectiveCorrectionHint = 0x0C50,
		PointSmoothHint = 0x0C51,
		LineSmoothHint = 0x0C52,
		PolygonSmoothHint = 0x0C53,
		FogHint = 0x0C54,
		GenerateMipmapHint = 0x8192,
	}

	public enum MatrixMode : int {
		Modelview = 0x1700,
		Projection = 0x1701,
		Texture = 0x1702,
		Color = 0x1800,
	}

	public enum PixelFormat : int {
		Rgba = 0x1908,
		Bgra = 0x80E1,
	}

	public enum PixelInternalFormat : int {
		Rgba = 0x1908,
		Rgba8 = 0x8058,
	}

	public enum PixelType : int {
		UnsignedByte = 0x1401,
	}

	public enum StringName : int {
		Vendor = 0x1F00,
		Renderer = 0x1F01,
		Version = 0x1F02,
		Extensions = 0x1F03,
	}

	public enum TextureFilter : int {
		Nearest = 0x2600,
		Linear = 0x2601,
		NearestMipmapNearest = 0x2700,
		LinearMipmapNearest = 0x2701,
		NearestMipmapLinear = 0x2702,
		LinearMipmapLinear = 0x2703,
	}

	public enum TextureParameterName : int {
		MagFilter = 0x2800,
		MinFilter = 0x2801,
		TextureMaxLevel = 0x813D,
		GenerateMipmap = 0x8191,
	}

	public enum TextureTarget : int {
		Texture2D = 0x0DE1,
	}
}
#endif
