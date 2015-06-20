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
		internal static partial class Delegates
		{
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void ActiveTexture(OpenTK.Graphics.OpenGL.TextureUnit texture);
			internal static ActiveTexture glActiveTexture;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void AttachShader(UInt32 program, UInt32 shader);
			internal static AttachShader glAttachShader;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void BindBuffer(OpenTK.Graphics.OpenGL.BufferTarget target, UInt32 buffer);
			internal static BindBuffer glBindBuffer;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void BindTexture(OpenTK.Graphics.OpenGL.TextureTarget target, UInt32 texture);
			internal static BindTexture glBindTexture;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void BlendFunc(OpenTK.Graphics.OpenGL.BlendingFactor sfactor, OpenTK.Graphics.OpenGL.BlendingFactor dfactor);
			internal static BlendFunc glBlendFunc;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void BufferData(OpenTK.Graphics.OpenGL.BufferTarget target, IntPtr size, IntPtr data, OpenTK.Graphics.OpenGL.BufferUsageHint usage);
			internal static BufferData glBufferData;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void BufferSubData(OpenTK.Graphics.OpenGL.BufferTarget target, IntPtr offset, IntPtr size, IntPtr data);
			internal static BufferSubData glBufferSubData;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Clear(OpenTK.Graphics.OpenGL.ClearBufferMask mask);
			internal static Clear glClear;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void ClearColor(Single red, Single green, Single blue, Single alpha);
			internal static ClearColor glClearColor;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void ClearDepth(Double depth);
			internal static ClearDepth glClearDepth;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void ClientActiveTexture(OpenTK.Graphics.OpenGL.TextureUnit texture);
			internal static ClientActiveTexture glClientActiveTexture;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void ColorMask(bool red, bool green, bool blue, bool alpha);
			internal static ColorMask glColorMask;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void CompileShader(UInt32 shader);
			internal static CompileShader glCompileShader;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate Int32 CreateProgram();
			internal static CreateProgram glCreateProgram;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate Int32 CreateShader(OpenTK.Graphics.OpenGL.ShaderType type);
			internal static CreateShader glCreateShader;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void CullFace(OpenTK.Graphics.OpenGL.CullFaceMode mode);
			internal static CullFace glCullFace;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void DeleteBuffers(Int32 n, UInt32* buffers);
			internal unsafe static DeleteBuffers glDeleteBuffers;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void DeleteProgram(UInt32 program);
			internal static DeleteProgram glDeleteProgram;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void DeleteShader(UInt32 shader);
			internal static DeleteShader glDeleteShader;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void DeleteTextures(Int32 n, UInt32* textures);
			internal unsafe static DeleteTextures glDeleteTextures;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void DepthFunc(OpenTK.Graphics.OpenGL.DepthFunction func);
			internal static DepthFunc glDepthFunc;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void DepthMask(bool flag);
			internal static DepthMask glDepthMask;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void DepthRange(Double near, Double far);
			internal static DepthRange glDepthRange;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void DetachShader(UInt32 program, UInt32 shader);
			internal static DetachShader glDetachShader;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Disable(OpenTK.Graphics.OpenGL.EnableCap cap);
			internal static Disable glDisable;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void DisableVertexAttribArray(UInt32 index);
			internal static DisableVertexAttribArray glDisableVertexAttribArray;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void DrawArrays(OpenTK.Graphics.OpenGL.BeginMode mode, Int32 first, Int32 count);
			internal static DrawArrays glDrawArrays;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void DrawElements(OpenTK.Graphics.OpenGL.BeginMode mode, Int32 count, OpenTK.Graphics.OpenGL.DrawElementsType type, IntPtr indices);
			internal static DrawElements glDrawElements;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Enable(OpenTK.Graphics.OpenGL.EnableCap cap);
			internal static Enable glEnable;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
            internal delegate void EnableVertexAttribArray(UInt32 index);
            internal static EnableVertexAttribArray glEnableVertexAttribArray;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Finish();
			internal static Finish glFinish;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Flush();
			internal static Flush glFlush;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void FrontFace(OpenTK.Graphics.OpenGL.FrontFaceDirection mode);
			internal static FrontFace glFrontFace;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GenBuffers(Int32 n, [OutAttribute] UInt32* buffers);
			internal unsafe static GenBuffers glGenBuffers;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GenTextures(Int32 n, [OutAttribute] UInt32* textures);
			internal unsafe static GenTextures glGenTextures;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GetActiveAttrib(UInt32 program, UInt32 index, Int32 bufSize, [OutAttribute] Int32* length, [OutAttribute] Int32* size, [OutAttribute] OpenTK.Graphics.OpenGL.ActiveAttribType* type, [OutAttribute] StringBuilder name);
			internal unsafe static GetActiveAttrib glGetActiveAttrib;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GetActiveUniform(UInt32 program, UInt32 index, Int32 bufSize, [OutAttribute] Int32* length, [OutAttribute] Int32* size, [OutAttribute] OpenTK.Graphics.OpenGL.ActiveUniformType* type, [OutAttribute] StringBuilder name);
			internal unsafe static GetActiveUniform glGetActiveUniform;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate Int32 GetAttribLocation(UInt32 program, String name);
			internal static GetAttribLocation glGetAttribLocation;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GetBooleanv(OpenTK.Graphics.OpenGL.GetPName pname, [OutAttribute] bool* @params);
			internal unsafe static GetBooleanv glGetBooleanv;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate OpenTK.Graphics.OpenGL.ErrorCode GetError();
			internal static GetError glGetError;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GetFloatv(OpenTK.Graphics.OpenGL.GetPName pname, [OutAttribute] Single* @params);
			internal unsafe static GetFloatv glGetFloatv;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GetIntegerv(OpenTK.Graphics.OpenGL.GetPName pname, [OutAttribute] Int32* @params);
			internal unsafe static GetIntegerv glGetIntegerv;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GetProgramInfoLog(UInt32 program, Int32 bufSize, [OutAttribute] Int32* length, [OutAttribute] StringBuilder infoLog);
			internal unsafe static GetProgramInfoLog glGetProgramInfoLog;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GetProgramiv(UInt32 program, OpenTK.Graphics.OpenGL.ProgramParameter pname, [OutAttribute] Int32* @params);
			internal unsafe static GetProgramiv glGetProgramiv;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GetShaderInfoLog(UInt32 shader, Int32 bufSize, [OutAttribute] Int32* length, [OutAttribute] StringBuilder infoLog);
			internal unsafe static GetShaderInfoLog glGetShaderInfoLog;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void GetShaderiv(UInt32 shader, OpenTK.Graphics.OpenGL.ShaderParameter pname, [OutAttribute] Int32* @params);
			internal unsafe static GetShaderiv glGetShaderiv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate System.IntPtr GetString(OpenTK.Graphics.OpenGL.StringName name);
			internal static GetString glGetString;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate Int32 GetUniformLocation(UInt32 program, String name);
			internal static GetUniformLocation glGetUniformLocation;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Hint(OpenTK.Graphics.OpenGL.HintTarget target, OpenTK.Graphics.OpenGL.HintMode mode);
			internal static Hint glHint;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate bool IsBuffer(UInt32 buffer);
			internal static IsBuffer glIsBuffer;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate bool IsProgram(UInt32 program);
			internal static IsProgram glIsProgram;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate bool IsShader(UInt32 shader);
			internal static IsShader glIsShader;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate bool IsTexture(UInt32 texture);
			internal static IsTexture glIsTexture;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void LinkProgram(UInt32 program);
			internal static LinkProgram glLinkProgram;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void LogicOp(OpenTK.Graphics.OpenGL.LogicOp opcode);
			internal static LogicOp glLogicOp;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void PixelStoref(OpenTK.Graphics.OpenGL.PixelStoreParameter pname, Single param);
			internal static PixelStoref glPixelStoref;
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void PixelStorei(OpenTK.Graphics.OpenGL.PixelStoreParameter pname, Int32 param);
			internal static PixelStorei glPixelStorei;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void PolygonOffset(Single factor, Single units);
			internal static PolygonOffset glPolygonOffset;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void ReadPixels(Int32 x, Int32 y, Int32 width, Int32 height, OpenTK.Graphics.OpenGL.PixelFormat format, OpenTK.Graphics.OpenGL.PixelType type, [OutAttribute] IntPtr pixels);
			internal static ReadPixels glReadPixels;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void ShaderSource(UInt32 shader, Int32 count, String[] @string, Int32* length);
			internal unsafe static ShaderSource glShaderSource;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void TexImage2D(OpenTK.Graphics.OpenGL.TextureTarget target, Int32 level, OpenTK.Graphics.OpenGL.PixelInternalFormat internalformat, Int32 width, Int32 height, Int32 border, OpenTK.Graphics.OpenGL.PixelFormat format, OpenTK.Graphics.OpenGL.PixelType type, IntPtr pixels);
			internal static TexImage2D glTexImage2D;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void TexParameterf(OpenTK.Graphics.OpenGL.TextureTarget target, OpenTK.Graphics.OpenGL.TextureParameterName pname, Single param);
			internal static TexParameterf glTexParameterf;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void TexParameteri(OpenTK.Graphics.OpenGL.TextureTarget target, OpenTK.Graphics.OpenGL.TextureParameterName pname, Int32 param);
			internal static TexParameteri glTexParameteri;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void TexSubImage2D(OpenTK.Graphics.OpenGL.TextureTarget target, Int32 level, Int32 xoffset, Int32 yoffset, Int32 width, Int32 height, OpenTK.Graphics.OpenGL.PixelFormat format, OpenTK.Graphics.OpenGL.PixelType type, IntPtr pixels);
			internal static TexSubImage2D glTexSubImage2D;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform1f(Int32 location, Single v0);
			internal static Uniform1f glUniform1f;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform1fv(Int32 location, Int32 count, Single* value);
			internal unsafe static Uniform1fv glUniform1fv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform1i(Int32 location, Int32 v0);
			internal static Uniform1i glUniform1i;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform1iv(Int32 location, Int32 count, Int32* value);
			internal unsafe static Uniform1iv glUniform1iv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform1ui(Int32 location, UInt32 v0);
			internal static Uniform1ui glUniform1ui;
			
			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform1uiv(Int32 location, Int32 count, UInt32* value);
			internal unsafe static Uniform1uiv glUniform1uiv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform2f(Int32 location, Single v0, Single v1);
			internal static Uniform2f glUniform2f;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform2fv(Int32 location, Int32 count, Single* value);
			internal unsafe static Uniform2fv glUniform2fv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform2i(Int32 location, Int32 v0, Int32 v1);
			internal static Uniform2i glUniform2i;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform2iv(Int32 location, Int32 count, Int32* value);
			internal unsafe static Uniform2iv glUniform2iv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform2ui(Int32 location, UInt32 v0, UInt32 v1);
			internal static Uniform2ui glUniform2ui;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform2uiv(Int32 location, Int32 count, UInt32* value);
			internal unsafe static Uniform2uiv glUniform2uiv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform3f(Int32 location, Single v0, Single v1, Single v2);
			internal static Uniform3f glUniform3f;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform3fv(Int32 location, Int32 count, Single* value);
			internal unsafe static Uniform3fv glUniform3fv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform3i(Int32 location, Int32 v0, Int32 v1, Int32 v2);
			internal static Uniform3i glUniform3i;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform3iv(Int32 location, Int32 count, Int32* value);
			internal unsafe static Uniform3iv glUniform3iv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform3ui(Int32 location, UInt32 v0, UInt32 v1, UInt32 v2);
			internal static Uniform3ui glUniform3ui;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform3uiv(Int32 location, Int32 count, UInt32* value);
			internal unsafe static Uniform3uiv glUniform3uiv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform4f(Int32 location, Single v0, Single v1, Single v2, Single v3);
			internal static Uniform4f glUniform4f;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform4fv(Int32 location, Int32 count, Single* value);
			internal unsafe static Uniform4fv glUniform4fv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform4i(Int32 location, Int32 v0, Int32 v1, Int32 v2, Int32 v3);
			internal static Uniform4i glUniform4i;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform4iv(Int32 location, Int32 count, Int32* value);
			internal unsafe static Uniform4iv glUniform4iv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Uniform4ui(Int32 location, UInt32 v0, UInt32 v1, UInt32 v2, UInt32 v3);
			internal static Uniform4ui glUniform4ui;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void Uniform4uiv(Int32 location, Int32 count, UInt32* value);
			internal unsafe static Uniform4uiv glUniform4uiv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal unsafe delegate void UniformMatrix4fv(Int32 location, Int32 count, bool transpose, Single* value);
			internal unsafe static UniformMatrix4fv glUniformMatrix4fv;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void UseProgram(UInt32 program);
			internal static UseProgram glUseProgram;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void ValidateProgram(UInt32 program);
			internal static ValidateProgram glValidateProgram;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void VertexAttribIPointer(UInt32 index, Int32 size, OpenTK.Graphics.OpenGL.VertexAttribIPointerType type, Int32 stride, IntPtr pointer);
			internal static VertexAttribIPointer glVertexAttribIPointer;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void VertexAttribPointer(UInt32 index, Int32 size, OpenTK.Graphics.OpenGL.VertexAttribPointerType type, bool normalized, Int32 stride, IntPtr pointer);
			internal static VertexAttribPointer glVertexAttribPointer;

			[System.Security.SuppressUnmanagedCodeSecurity()]
			internal delegate void Viewport(Int32 x, Int32 y, Int32 width, Int32 height);
			internal static Viewport glViewport;
		}
	}
}
