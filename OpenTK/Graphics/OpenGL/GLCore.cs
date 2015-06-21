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
	#pragma warning disable 3019
	#pragma warning disable 1591

	partial class GL
	{

		internal static partial class Core
		{
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glActiveTexture", ExactSpelling = true)]
			internal extern static void ActiveTexture(TextureUnit texture);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glAttachShader", ExactSpelling = true)]
			internal extern static void AttachShader(UInt32 program, UInt32 shader);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glBindBuffer", ExactSpelling = true)]
			internal extern static void BindBuffer(BufferTarget target, UInt32 buffer);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glBindFramebuffer", ExactSpelling = true)]
			internal extern static void BindFramebuffer(FramebufferTarget target, UInt32 framebuffer);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glBindTexture", ExactSpelling = true)]
			internal extern static void BindTexture(TextureTarget target, UInt32 texture);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glBlendFunc", ExactSpelling = true)]
			internal extern static void BlendFunc(BlendingFactor sfactor, BlendingFactor dfactor);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glBlitFramebuffer", ExactSpelling = true)]
			internal extern static void BlitFramebuffer(Int32 srcX0, Int32 srcY0, Int32 srcX1, Int32 srcY1, Int32 dstX0, Int32 dstY0, Int32 dstX1, Int32 dstY1, ClearBufferMask mask, BlitFramebufferFilter filter);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glBufferData", ExactSpelling = true)]
			internal extern static void BufferData(BufferTarget target, IntPtr size, IntPtr data, BufferUsageHint usage);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glBufferSubData", ExactSpelling = true)]
			internal extern static void BufferSubData(BufferTarget target, IntPtr offset, IntPtr size, IntPtr data);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glCheckFramebufferStatus", ExactSpelling = true)]
			internal extern static FramebufferErrorCode CheckFramebufferStatus(FramebufferTarget target);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glClear", ExactSpelling = true)]
			internal extern static void Clear(ClearBufferMask mask);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glClearColor", ExactSpelling = true)]
			internal extern static void ClearColor(Single red, Single green, Single blue, Single alpha);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glClearDepth", ExactSpelling = true)]
			internal extern static void ClearDepth(Double depth);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glColorMask", ExactSpelling = true)]
			internal extern static void ColorMask(bool red, bool green, bool blue, bool alpha);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glCompileShader", ExactSpelling = true)]
			internal extern static void CompileShader(UInt32 shader);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glCreateProgram", ExactSpelling = true)]
			internal extern static Int32 CreateProgram();

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glCreateShader", ExactSpelling = true)]
			internal extern static Int32 CreateShader(ShaderType type);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glCullFace", ExactSpelling = true)]
			internal extern static void CullFace(CullFaceMode mode);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDeleteBuffers", ExactSpelling = true)]
			internal extern static unsafe void DeleteBuffers(Int32 n, UInt32* buffers);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDeleteFramebuffers", ExactSpelling = true)]
			internal extern static unsafe void DeleteFramebuffers(Int32 n, UInt32* framebuffers);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDeleteProgram", ExactSpelling = true)]
			internal extern static void DeleteProgram(UInt32 program);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDeleteShader", ExactSpelling = true)]
			internal extern static void DeleteShader(UInt32 shader);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDeleteTextures", ExactSpelling = true)]
			internal extern static unsafe void DeleteTextures(Int32 n, UInt32* textures);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDepthFunc", ExactSpelling = true)]
			internal extern static void DepthFunc(DepthFunction func);
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDepthMask", ExactSpelling = true)]
			internal extern static void DepthMask(bool flag);
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDepthRange", ExactSpelling = true)]
			internal extern static void DepthRange(Double near, Double far);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDetachShader", ExactSpelling = true)]
			internal extern static void DetachShader(UInt32 program, UInt32 shader);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDisable", ExactSpelling = true)]
			internal extern static void Disable(EnableCap cap);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDisableVertexAttribArray", ExactSpelling = true)]
			internal extern static void DisableVertexAttribArray(UInt32 index);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDrawArrays", ExactSpelling = true)]
			internal extern static void DrawArrays(BeginMode mode, Int32 first, Int32 count);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDrawBuffer", ExactSpelling = true)]
			internal extern static void DrawBuffer(DrawBufferMode mode);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glDrawElements", ExactSpelling = true)]
			internal extern static void DrawElements(BeginMode mode, Int32 count, DrawElementsType type, IntPtr indices);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glEnable", ExactSpelling = true)]
			internal extern static void Enable(EnableCap cap);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glEnableVertexAttribArray", ExactSpelling = true)]
			internal extern static void EnableVertexAttribArray(UInt32 index);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glFinish", ExactSpelling = true)]
			internal extern static void Finish();

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glFlush", ExactSpelling = true)]
			internal extern static void Flush();
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glFramebufferTexture2D", ExactSpelling = true)]
			internal extern static void FramebufferTexture2D(FramebufferTarget target, FramebufferAttachment attachment, TextureTarget textarget, UInt32 texture, Int32 level);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glFrontFace", ExactSpelling = true)]
			internal extern static void FrontFace(FrontFaceDirection mode);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGenBuffers", ExactSpelling = true)]
			internal extern static unsafe void GenBuffers(Int32 n, [OutAttribute] UInt32* buffers);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGenFramebuffers", ExactSpelling = true)]
			internal extern static unsafe void GenFramebuffers(Int32 n, [OutAttribute] UInt32* framebuffers);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGenTextures", ExactSpelling = true)]
			internal extern static unsafe void GenTextures(Int32 n, [OutAttribute] UInt32* textures);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetActiveAttrib", ExactSpelling = true)]
			internal extern static unsafe void GetActiveAttrib(UInt32 program, UInt32 index, Int32 bufSize, [OutAttribute] Int32* length, [OutAttribute] Int32* size, [OutAttribute] ActiveAttribType* type, [OutAttribute] StringBuilder name);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetActiveUniform", ExactSpelling = true)]
			internal extern static unsafe void GetActiveUniform(UInt32 program, UInt32 index, Int32 bufSize, [OutAttribute] Int32* length, [OutAttribute] Int32* size, [OutAttribute] ActiveUniformType* type, [OutAttribute] StringBuilder name);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetAttribLocation", ExactSpelling = true)]
			internal extern static Int32 GetAttribLocation(UInt32 program, String name);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetBooleanv", ExactSpelling = true)]
			internal extern static unsafe void GetBooleanv(GetPName pname, [OutAttribute] bool* @params);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetError", ExactSpelling = true)]
			internal extern static ErrorCode GetError();
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetFloatv", ExactSpelling = true)]
			internal extern static unsafe void GetFloatv(GetPName pname, [OutAttribute] Single* @params);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetIntegerv", ExactSpelling = true)]
			internal extern static unsafe void GetIntegerv(GetPName pname, [OutAttribute] Int32* @params);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetProgramInfoLog", ExactSpelling = true)]
			internal extern static unsafe void GetProgramInfoLog(UInt32 program, Int32 bufSize, [OutAttribute] Int32* length, [OutAttribute] StringBuilder infoLog);
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetProgramiv", ExactSpelling = true)]
			internal extern static unsafe void GetProgramiv(UInt32 program, ProgramParameter pname, [OutAttribute] Int32* @params);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetShaderInfoLog", ExactSpelling = true)]
			internal extern static unsafe void GetShaderInfoLog(UInt32 shader, Int32 bufSize, [OutAttribute] Int32* length, [OutAttribute] StringBuilder infoLog);
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetShaderiv", ExactSpelling = true)]
			internal extern static unsafe void GetShaderiv(UInt32 shader, ShaderParameter pname, [OutAttribute] Int32* @params);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetString", ExactSpelling = true)]
			internal extern static System.IntPtr GetString(StringName name);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glGetUniformLocation", ExactSpelling = true)]
			internal extern static Int32 GetUniformLocation(UInt32 program, String name);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glHint", ExactSpelling = true)]
			internal extern static void Hint(HintTarget target, HintMode mode);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glIsBuffer", ExactSpelling = true)]
			internal extern static bool IsBuffer(UInt32 buffer);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glIsFramebuffer", ExactSpelling = true)]
			internal extern static bool IsFramebuffer(UInt32 framebuffer);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glIsProgram", ExactSpelling = true)]
			internal extern static bool IsProgram(UInt32 program);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glIsShader", ExactSpelling = true)]
			internal extern static bool IsShader(UInt32 shader);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glIsTexture", ExactSpelling = true)]
			internal extern static bool IsTexture(UInt32 texture);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glLinkProgram", ExactSpelling = true)]
			internal extern static void LinkProgram(UInt32 program);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glLogicOp", ExactSpelling = true)]
			internal extern static void LogicOp(LogicOp opcode);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glPixelStoref", ExactSpelling = true)]
			internal extern static void PixelStoref(PixelStoreParameter pname, Single param);
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glPixelStorei", ExactSpelling = true)]
			internal extern static void PixelStorei(PixelStoreParameter pname, Int32 param);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glPolygonOffset", ExactSpelling = true)]
			internal extern static void PolygonOffset(Single factor, Single units);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glReadBuffer", ExactSpelling = true)]
			internal extern static void ReadBuffer(ReadBufferMode mode);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glReadPixels", ExactSpelling = true)]
			internal extern static void ReadPixels(Int32 x, Int32 y, Int32 width, Int32 height, PixelFormat format, PixelType type, [OutAttribute] IntPtr pixels);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glShaderSource", ExactSpelling = true)]
			internal extern static unsafe void ShaderSource(UInt32 shader, Int32 count, String[] @string, Int32* length);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glTexImage2D", ExactSpelling = true)]
			internal extern static void TexImage2D(TextureTarget target, Int32 level, PixelInternalFormat internalformat, Int32 width, Int32 height, Int32 border, PixelFormat format, PixelType type, IntPtr pixels);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glTexParameterf", ExactSpelling = true)]
			internal extern static void TexParameterf(TextureTarget target, TextureParameterName pname, Single param);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glTexParameteri", ExactSpelling = true)]
			internal extern static void TexParameteri(TextureTarget target, TextureParameterName pname, Int32 param);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glTexSubImage2D", ExactSpelling = true)]
			internal extern static void TexSubImage2D(TextureTarget target, Int32 level, Int32 xoffset, Int32 yoffset, Int32 width, Int32 height, PixelFormat format, PixelType type, IntPtr pixels);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform1f", ExactSpelling = true)]
			internal extern static void Uniform1f(Int32 location, Single v0);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform1fv", ExactSpelling = true)]
			internal extern static unsafe void Uniform1fv(Int32 location, Int32 count, Single* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform1i", ExactSpelling = true)]
			internal extern static void Uniform1i(Int32 location, Int32 v0);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform1iv", ExactSpelling = true)]
			internal extern static unsafe void Uniform1iv(Int32 location, Int32 count, Int32* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform1ui", ExactSpelling = true)]
			internal extern static void Uniform1ui(Int32 location, UInt32 v0);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform1uiv", ExactSpelling = true)]
			internal extern static unsafe void Uniform1uiv(Int32 location, Int32 count, UInt32* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform2f", ExactSpelling = true)]
			internal extern static void Uniform2f(Int32 location, Single v0, Single v1);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform2fv", ExactSpelling = true)]
			internal extern static unsafe void Uniform2fv(Int32 location, Int32 count, Single* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform2i", ExactSpelling = true)]
			internal extern static void Uniform2i(Int32 location, Int32 v0, Int32 v1);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform2iv", ExactSpelling = true)]
			internal extern static unsafe void Uniform2iv(Int32 location, Int32 count, Int32* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform2ui", ExactSpelling = true)]
			internal extern static void Uniform2ui(Int32 location, UInt32 v0, UInt32 v1);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform2uiv", ExactSpelling = true)]
			internal extern static unsafe void Uniform2uiv(Int32 location, Int32 count, UInt32* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform3f", ExactSpelling = true)]
			internal extern static void Uniform3f(Int32 location, Single v0, Single v1, Single v2);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform3fv", ExactSpelling = true)]
			internal extern static unsafe void Uniform3fv(Int32 location, Int32 count, Single* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform3i", ExactSpelling = true)]
			internal extern static void Uniform3i(Int32 location, Int32 v0, Int32 v1, Int32 v2);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform3iv", ExactSpelling = true)]
			internal extern static unsafe void Uniform3iv(Int32 location, Int32 count, Int32* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform3ui", ExactSpelling = true)]
			internal extern static void Uniform3ui(Int32 location, UInt32 v0, UInt32 v1, UInt32 v2);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform3uiv", ExactSpelling = true)]
			internal extern static unsafe void Uniform3uiv(Int32 location, Int32 count, UInt32* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform4f", ExactSpelling = true)]
			internal extern static void Uniform4f(Int32 location, Single v0, Single v1, Single v2, Single v3);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform4fv", ExactSpelling = true)]
			internal extern static unsafe void Uniform4fv(Int32 location, Int32 count, Single* value);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform4i", ExactSpelling = true)]
			internal extern static void Uniform4i(Int32 location, Int32 v0, Int32 v1, Int32 v2, Int32 v3);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform4iv", ExactSpelling = true)]
			internal extern static unsafe void Uniform4iv(Int32 location, Int32 count, Int32* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform4ui", ExactSpelling = true)]
			internal extern static void Uniform4ui(Int32 location, UInt32 v0, UInt32 v1, UInt32 v2, UInt32 v3);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniform4uiv", ExactSpelling = true)]
			internal extern static unsafe void Uniform4uiv(Int32 location, Int32 count, UInt32* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUniformMatrix4fv", ExactSpelling = true)]
			internal extern static unsafe void UniformMatrix4fv(Int32 location, Int32 count, bool transpose, Single* value);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glUseProgram", ExactSpelling = true)]
			internal extern static void UseProgram(UInt32 program);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glValidateProgram", ExactSpelling = true)]
			internal extern static void ValidateProgram(UInt32 program);
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glVertexAttribIPointer", ExactSpelling = true)]
			internal extern static void VertexAttribIPointer(UInt32 index, Int32 size, VertexAttribIPointerType type, Int32 stride, IntPtr pointer);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glVertexAttribPointer", ExactSpelling = true)]
			internal extern static void VertexAttribPointer(UInt32 index, Int32 size, VertexAttribPointerType type, bool normalized, Int32 stride, IntPtr pointer);

			[System.Security.SuppressUnmanagedCodeSecurity()]
			[System.Runtime.InteropServices.DllImport(GL.Library, EntryPoint = "glViewport", ExactSpelling = true)]
			internal extern static void Viewport(Int32 x, Int32 y, Int32 width, Int32 height);
		}
	}
}
