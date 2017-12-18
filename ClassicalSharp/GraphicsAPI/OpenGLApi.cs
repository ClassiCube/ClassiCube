// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if !USE_DX && !ANDROID
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using OpenTK;
using OpenTK.Graphics.OpenGL;
using BmpPixelFormat = System.Drawing.Imaging.PixelFormat;
using GlPixelFormat = OpenTK.Graphics.OpenGL.PixelFormat;

namespace ClassicalSharp.GraphicsAPI {

	/// <summary> Implements IGraphicsAPI using OpenGL 1.5,
	/// or 1.2 with the GL_ARB_vertex_buffer_object extension. </summary>
	public unsafe class OpenGLApi : IGraphicsApi {
		
		bool glLists = false;
		int activeList = -1;
		const int dynamicListId = 1234567891;
		IntPtr dynamicListData;
		
		public OpenGLApi() {
			InitFields();
			int texDims;
			GL.GetIntegerv(GetPName.MaxTextureSize, &texDims);
			texDimensions = texDims;
			
			glLists = Options.GetBool(OptionsKey.ForceOldOpenGL, false);
			CustomMipmapsLevels = !glLists;
			CheckVboSupport();
			base.InitDynamicBuffers();
			
			setupBatchFuncCol4b = SetupVbPos3fCol4b;
			setupBatchFuncTex2fCol4b = SetupVbPos3fTex2fCol4b;
			setupBatchFuncCol4b_Range = SetupVbPos3fCol4b_Range;
			setupBatchFuncTex2fCol4b_Range = SetupVbPos3fTex2fCol4b_Range;
			
			GL.EnableClientState(ArrayCap.VertexArray);
			GL.EnableClientState(ArrayCap.ColorArray);
		}
		
		void CheckVboSupport() {
			string extensions = new String((sbyte*)GL.GetString(StringName.Extensions));
			string version = new String((sbyte*)GL.GetString(StringName.Version));
			int major = (int)(version[0] - '0'); // x.y. (and so forth)
			int minor = (int)(version[2] - '0');
			
			if ((major > 1) || (major == 1 && minor >= 5)) return; // Supported in core since 1.5
			
			Utils.LogDebug("Using ARB vertex buffer objects");
			if (extensions.Contains("GL_ARB_vertex_buffer_object")) {
				GL.UseArbVboAddresses();
			} else {
				glLists = true;
				CustomMipmapsLevels = false;
			}
		}

		public override bool AlphaTest {
			set { if (value) GL.Enable(EnableCap.AlphaTest);
				else GL.Disable(EnableCap.AlphaTest); }
		}
		
		public override bool AlphaBlending {
			set { if (value) GL.Enable(EnableCap.Blend);
				else GL.Disable(EnableCap.Blend); }
		}
		
		Compare[] compareFuncs;
		public override void AlphaTestFunc(CompareFunc func, float value) {
			GL.AlphaFunc(compareFuncs[(int)func], value);
		}
		
		BlendingFactor[] blendFuncs;
		public override void AlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc) {
			GL.BlendFunc(blendFuncs[(int)srcFunc], blendFuncs[(int)dstFunc]);
		}
		
		bool fogEnable;
		public override bool Fog {
			get { return fogEnable; }
			set { fogEnable = value;
				if (value) GL.Enable(EnableCap.Fog);
				else GL.Disable(EnableCap.Fog); }
		}
		
		FastColour lastFogCol = FastColour.Black;
		public override void SetFogColour(FastColour col) {
			if (col == lastFogCol) return;
			Vector4 colRGBA = new Vector4(col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f);
			GL.Fogfv(FogParameter.FogColor, &colRGBA.X);
			lastFogCol = col;
		}
		
		float lastFogEnd = -1, lastFogDensity = -1;
		public override void SetFogDensity(float value) {
			FogParam(FogParameter.FogDensity, value, ref lastFogDensity);
		}
		
		public override void SetFogStart(float value) {
			GL.Fogf(FogParameter.FogStart, value);
		}
		
		public override void SetFogEnd(float value) {
			FogParam(FogParameter.FogEnd, value, ref lastFogEnd);
		}
		
		static void FogParam(FogParameter param, float value, ref float last) {
			if (value == last) return;
			GL.Fogf(param, value);
			last = value;
		}
		
		Fog lastFogMode = (Fog)999;
		FogMode[] fogModes;
		public override void SetFogMode(Fog mode) {
			if (mode != lastFogMode) {
				GL.Fogi(FogParameter.FogMode, (int)fogModes[(int)mode]);
				lastFogMode = mode;
			}
		}
		
		public override bool FaceCulling {
			set { if (value) GL.Enable(EnableCap.CullFace);
				else GL.Disable(EnableCap.CullFace); }
		}
		
		public override void Clear() {
			GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);
		}
		
		FastColour lastClearCol;
		public override void ClearColour(FastColour col) {
			if (col != lastClearCol) {
				GL.ClearColor(col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f);
				lastClearCol = col;
			}
		}
		
		public override bool ColourWrite { set { GL.ColorMask(value, value, value, value); } }
		
		public override void DepthTestFunc(CompareFunc func) {
			GL.DepthFunc(compareFuncs[(int)func]);
		}
		
		public override bool DepthTest  {
			set { if (value) GL.Enable(EnableCap.DepthTest);
				else GL.Disable(EnableCap.DepthTest); }
		}
		
		public override bool DepthWrite { set { GL.DepthMask(value); } }
		
		public override bool AlphaArgBlend { set { } }
		
		#region Texturing
		
		int texDimensions;
		public override int MaxTextureDimensions { get { return texDimensions; } }
		
		public override bool Texturing {
			set { if (value) GL.Enable(EnableCap.Texture2D);
				else GL.Disable(EnableCap.Texture2D); }
		}
		
		protected override int CreateTexture(int width, int height, IntPtr scan0, bool managedPool, bool mipmaps) {
			int texId = 0;
			GL.GenTextures(1, &texId);
			GL.BindTexture(TextureTarget.Texture2D, texId);
			GL.TexParameteri(TextureTarget.Texture2D, TextureParameterName.MagFilter, (int)TextureFilter.Nearest);
			
			if (mipmaps) {
				GL.TexParameteri(TextureTarget.Texture2D, TextureParameterName.MinFilter, (int)TextureFilter.NearestMipmapLinear);
				if (CustomMipmapsLevels) {
					GL.TexParameteri(TextureTarget.Texture2D, TextureParameterName.TextureMaxLevel, MipmapsLevels(width, height));
				}
			} else {
				GL.TexParameteri(TextureTarget.Texture2D, TextureParameterName.MinFilter, (int)TextureFilter.Nearest);
			}

			GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, width, height,
			              GlPixelFormat.Bgra, PixelType.UnsignedByte, scan0);
			
			if (mipmaps) DoMipmaps(texId, 0, 0, width, height, scan0, false);
			return texId;
		}
		
		unsafe void DoMipmaps(int texId, int x, int y, int width,
		                      int height, IntPtr scan0, bool partial) {
			IntPtr prev = scan0;
			int lvls = MipmapsLevels(width, height);
			
			for (int lvl = 1; lvl <= lvls; lvl++) {
				x /= 2; y /= 2;
				if (width > 1)   width /= 2;
				if (height > 1) height /= 2;
				int size = width * height * 4;
				
				IntPtr cur = Marshal.AllocHGlobal(size);
				GenMipmaps(width, height, cur, prev);
				
				if (partial) {
					GL.TexSubImage2D(TextureTarget.Texture2D, lvl, x, y, width, height,
					                 GlPixelFormat.Bgra, PixelType.UnsignedByte, cur);
				} else {
					GL.TexImage2D(TextureTarget.Texture2D, lvl, PixelInternalFormat.Rgba, width, height,
					              GlPixelFormat.Bgra, PixelType.UnsignedByte, cur);
				}
				
				if (prev != scan0) Marshal.FreeHGlobal(prev);
				prev = cur;
			}
			if (prev != scan0) Marshal.FreeHGlobal(prev);
		}
		
		public override void BindTexture(int texture) {
			GL.BindTexture(TextureTarget.Texture2D, texture);
		}
		
		public override void UpdateTexturePart(int texId, int x, int y, FastBitmap part, bool mipmaps) {
			GL.BindTexture(TextureTarget.Texture2D, texId);
			GL.TexSubImage2D(TextureTarget.Texture2D, 0, x, y, part.Width, part.Height,
			                 GlPixelFormat.Bgra, PixelType.UnsignedByte, part.Scan0);
			if (mipmaps) DoMipmaps(texId, x, y, part.Width, part.Height, part.Scan0, true);
		}
		
		public override void DeleteTexture(ref int texId) {
			if (texId <= 0) return;
			int id = texId; GL.DeleteTextures(1, &id);
			texId = -1;
		}
		
		public override void EnableMipmaps() { }
		
		public override void DisableMipmaps() { }
		#endregion
		
		#region Vertex/index buffers
		Action setupBatchFunc, setupBatchFuncCol4b, setupBatchFuncTex2fCol4b;
		Action<int> setupBatchFunc_Range, setupBatchFuncCol4b_Range, setupBatchFuncTex2fCol4b_Range;
		
		public override int CreateDynamicVb(VertexFormat format, int maxVertices) {
			if (glLists) return dynamicListId;
			int id = GenAndBind(BufferTarget.ArrayBuffer);
			int sizeInBytes = maxVertices * strideSizes[(int)format];
			GL.BufferData(BufferTarget.ArrayBuffer, new IntPtr(sizeInBytes), IntPtr.Zero, BufferUsage.DynamicDraw);
			return id;
		}
		
		public override int CreateVb(IntPtr vertices, VertexFormat format, int count) {
			if (glLists) {
				// We need to setup client state properly when building the list
				VertexFormat curFormat = batchFormat;
				SetBatchFormat(format);
				int list = GL.GenLists(1);
				GL.NewList(list, 0x1300);
				count &= ~0x01; // Need to get rid of the 1 extra element, see comment in chunk mesh builder for why
				
				const int maxIndices = 65536 / 4 * 6;
				ushort* indicesPtr = stackalloc ushort[maxIndices];
				MakeIndices(indicesPtr, maxIndices);
				
				int stride = format == VertexFormat.P3fT2fC4b ? VertexP3fT2fC4b.Size : VertexP3fC4b.Size;
				GL.VertexPointer(3, PointerType.Float, stride, vertices);
				GL.ColorPointer(4, PointerType.UnsignedByte, stride, (IntPtr)((byte*)vertices + 12));
				if (format == VertexFormat.P3fT2fC4b) {
					GL.TexCoordPointer(2, PointerType.Float, stride, (IntPtr)((byte*)vertices + 16));
				}
				
				GL.DrawElements(BeginMode.Triangles, (count >> 2) * 6, DrawElementsType.UnsignedShort, (IntPtr)indicesPtr);
				GL.EndList();
				SetBatchFormat(curFormat);
				return list;
			}
			
			int id = GenAndBind(BufferTarget.ArrayBuffer);
			int sizeInBytes = count * strideSizes[(int)format];
			GL.BufferData(BufferTarget.ArrayBuffer, new IntPtr(sizeInBytes), vertices, BufferUsage.StaticDraw);
			return id;
		}
		
		public override int CreateIb(IntPtr indices, int indicesCount) {
			if (glLists) return 0;
			int id = GenAndBind(BufferTarget.ElementArrayBuffer);
			int sizeInBytes = indicesCount * sizeof(ushort);
			GL.BufferData(BufferTarget.ElementArrayBuffer, new IntPtr(sizeInBytes), indices, BufferUsage.StaticDraw);
			return id;
		}
		
		static int GenAndBind(BufferTarget target) {
			int id = 0;
			GL.GenBuffers(1, &id);
			GL.BindBuffer(target, id);
			return id;
		}
		
		int batchStride;
		public override void SetDynamicVbData(int id, IntPtr vertices, int count) {
			if (glLists) {
				activeList = dynamicListId;
				dynamicListData = vertices;
				return;
			}
			
			GL.BindBuffer(BufferTarget.ArrayBuffer, id);
			GL.BufferSubData(BufferTarget.ArrayBuffer, IntPtr.Zero,
			                 new IntPtr(count * batchStride), vertices);
		}
		
		static void V(VertexP3fC4b v) {
			FastColour AAA = FastColour.Unpack(v.Colour);
			GL.Color4ub(AAA.R, AAA.G, AAA.B, AAA.A);
			GL.Vertex3f(v.X, v.Y, v.Z);
		}
		
		static void V(VertexP3fT2fC4b v) {
			FastColour AAA = FastColour.Unpack(v.Colour);
			GL.Color4ub(AAA.R, AAA.G, AAA.B, AAA.A);
			GL.TexCoord2f(v.U, v.V);
			GL.Vertex3f(v.X, v.Y, v.Z);
		}
		
		public override void DeleteVb(ref int vb) {
			if (vb <= 0) return;
			int id = vb; vb = -1;
			
			if (glLists) { if (id != dynamicListId) GL.DeleteLists(id, 1); }
			else { GL.DeleteBuffers(1, &id); }
		}
		
		public override void DeleteIb(ref int ib) {
			if (glLists || ib <= 0) return;
			int id = ib; ib = -1;
			GL.DeleteBuffers(1, &id);
		}
		
		VertexFormat batchFormat = (VertexFormat)999;
		public override void SetBatchFormat(VertexFormat format) {
			if (format == batchFormat) return;
			
			if (batchFormat == VertexFormat.P3fT2fC4b) {
				GL.DisableClientState(ArrayCap.TextureCoordArray);
			}
			
			batchFormat = format;
			if (format == VertexFormat.P3fT2fC4b) {
				GL.EnableClientState(ArrayCap.TextureCoordArray);
				setupBatchFunc = setupBatchFuncTex2fCol4b;
				setupBatchFunc_Range = setupBatchFuncTex2fCol4b_Range;
				batchStride = VertexP3fT2fC4b.Size;
			} else {
				setupBatchFunc = setupBatchFuncCol4b;
				setupBatchFunc_Range = setupBatchFuncCol4b_Range;
				batchStride = VertexP3fC4b.Size;
			}
		}
		
		public override void BindVb(int vb) {
			if (glLists) { activeList = vb; }
			else { GL.BindBuffer(BufferTarget.ArrayBuffer, vb); }
		}
		
		public override void BindIb(int ib) {
			if (glLists) return;
			GL.BindBuffer(BufferTarget.ElementArrayBuffer, ib);
		}
		
		const DrawElementsType indexType = DrawElementsType.UnsignedShort;
		public override void DrawVb_Lines(int verticesCount) {
			if (glLists) { DrawDynamicLines(verticesCount); return; }
			
			setupBatchFunc();
			GL.DrawArrays(BeginMode.Lines, 0, verticesCount);
		}
		
		public override void DrawVb_IndexedTris(int verticesCount, int startVertex) {
			if (glLists) {
				if (activeList != dynamicListId) { GL.CallList(activeList); }
				else { DrawDynamicTriangles(verticesCount, startVertex); }
				return;
			}
			
			setupBatchFunc_Range(startVertex);
			GL.DrawElements(BeginMode.Triangles, (verticesCount >> 2) * 6, indexType, IntPtr.Zero);
		}
		
		public override void DrawVb_IndexedTris(int verticesCount) {
			if (glLists) {
				if (activeList != dynamicListId) { GL.CallList(activeList); }
				else { DrawDynamicTriangles(verticesCount, 0); }
				return;
			}
			
			setupBatchFunc();
			GL.DrawElements(BeginMode.Triangles, (verticesCount >> 2) * 6, indexType, IntPtr.Zero);
		}
		
		unsafe void DrawDynamicLines(int verticesCount) {
			GL.Begin(BeginMode.Lines);
			if (batchFormat == VertexFormat.P3fT2fC4b) {
				VertexP3fT2fC4b* ptr = (VertexP3fT2fC4b*)dynamicListData;
				for (int i = 0; i < verticesCount; i += 2) {
					V(ptr[i + 0]); V(ptr[i + 1]);
				}
			} else {
				VertexP3fC4b* ptr = (VertexP3fC4b*)dynamicListData;
				for (int i = 0; i < verticesCount; i += 2) {
					V(ptr[i + 0]); V(ptr[i + 1]);
				}
			}
			GL.End();
		}
		
		unsafe void DrawDynamicTriangles(int verticesCount, int startVertex) {
			GL.Begin(BeginMode.Triangles);
			if (batchFormat == VertexFormat.P3fT2fC4b) {
				VertexP3fT2fC4b* ptr = (VertexP3fT2fC4b*)dynamicListData;
				for (int i = startVertex; i < startVertex + verticesCount; i += 4) {
					V(ptr[i + 0]); V(ptr[i + 1]); V(ptr[i + 2]);
					V(ptr[i + 2]); V(ptr[i + 3]); V(ptr[i + 0]);
				}
			} else {
				VertexP3fC4b* ptr = (VertexP3fC4b*)dynamicListData;
				for (int i = startVertex; i < startVertex + verticesCount; i += 4) {
					V(ptr[i + 0]); V(ptr[i + 1]); V(ptr[i + 2]);
					V(ptr[i + 2]); V(ptr[i + 3]); V(ptr[i + 0]);
				}
			}
			GL.End();
		}
		
		int lastPartialList = -1;
		internal override void DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex) {
			// TODO: This renders the whole map, bad performance!! FIX FIX
			if (glLists) {
				if (activeList != lastPartialList) {
					GL.CallList(activeList); lastPartialList = activeList;
				}
				return;
			}
			
			int offset = startVertex * VertexP3fT2fC4b.Size;
			GL.VertexPointer(3, PointerType.Float, VertexP3fT2fC4b.Size, new IntPtr(offset));
			GL.ColorPointer(4, PointerType.UnsignedByte, VertexP3fT2fC4b.Size, new IntPtr(offset + 12));
			GL.TexCoordPointer(2, PointerType.Float, VertexP3fT2fC4b.Size, new IntPtr(offset + 16));
			GL.DrawElements(BeginMode.Triangles, (verticesCount >> 2) * 6, indexType, IntPtr.Zero);
		}
		
		IntPtr zero = new IntPtr(0), twelve = new IntPtr(12), sixteen = new IntPtr(16);
		
		void SetupVbPos3fCol4b() {
			GL.VertexPointer(3, PointerType.Float, VertexP3fC4b.Size, zero);
			GL.ColorPointer(4, PointerType.UnsignedByte, VertexP3fC4b.Size, twelve);
		}
		
		void SetupVbPos3fTex2fCol4b() {
			GL.VertexPointer(3, PointerType.Float, VertexP3fT2fC4b.Size, zero);
			GL.ColorPointer(4, PointerType.UnsignedByte, VertexP3fT2fC4b.Size, twelve);
			GL.TexCoordPointer(2, PointerType.Float, VertexP3fT2fC4b.Size, sixteen);
		}
		
		void SetupVbPos3fCol4b_Range(int startVertex) {
			int offset = startVertex * VertexP3fC4b.Size;
			GL.VertexPointer(3, PointerType.Float, VertexP3fC4b.Size, new IntPtr(offset));
			GL.ColorPointer(4, PointerType.UnsignedByte, VertexP3fC4b.Size, new IntPtr(offset + 12));
		}
		
		void SetupVbPos3fTex2fCol4b_Range(int startVertex) {
			int offset = startVertex * VertexP3fT2fC4b.Size;
			GL.VertexPointer(3, PointerType.Float, VertexP3fT2fC4b.Size, new IntPtr(offset));
			GL.ColorPointer(4, PointerType.UnsignedByte, VertexP3fT2fC4b.Size, new IntPtr(offset + 12));
			GL.TexCoordPointer(2, PointerType.Float, VertexP3fT2fC4b.Size, new IntPtr(offset + 16));
		}
		#endregion
		
		#region Matrix manipulation
		MatrixMode lastMode = 0;
		MatrixMode[] matrixModes;
		public override void SetMatrixMode(MatrixType mode) {
			MatrixMode glMode = matrixModes[(int)mode];
			if (glMode != lastMode) {
				GL.MatrixMode(glMode);
				lastMode = glMode;
			}
		}
		
		public override void LoadMatrix(ref Matrix4 matrix) {
			fixed(Single* ptr = &matrix.Row0.X)
				GL.LoadMatrixf(ptr);
		}
		
		public override void LoadIdentityMatrix() {
			GL.LoadIdentity();
		}

		public override void CalcOrthoMatrix(float width, float height, out Matrix4 matrix) {
			Matrix4.CreateOrthographicOffCenter(0, width, height, 0, -10000, 10000, out matrix);
		}
		
		#endregion
		
		public override void BeginFrame(Game game) {
		}
		
		public override void EndFrame(Game game) {
			game.window.SwapBuffers();
			activeList = -1;
		}
		
		public override void SetVSync(Game game, bool value) {
			game.VSync = value;
		}
		
		bool isIntelRenderer;
		internal override void MakeApiInfo() {
			string vendor = new String((sbyte*)GL.GetString(StringName.Vendor));
			string renderer = new String((sbyte*)GL.GetString(StringName.Renderer));
			string version = new String((sbyte*)GL.GetString(StringName.Version));
			int depthBits = 0;
			GL.GetIntegerv(GetPName.DepthBits, &depthBits);
			
			ApiInfo = new string[] {
				"--Using OpenGL api--",
				"Vendor: " + vendor,
				"Renderer: " + renderer,
				"GL version: " + version,
				"Max 2D texture dimensions: " + MaxTextureDimensions,
				"Depth buffer bits: " + depthBits,
			};
			isIntelRenderer = renderer.Contains("Intel");
		}
		
		public override bool WarnIfNecessary(Chat chat) {
			if (glLists) {
				chat.Add("&cYou are using the very outdated OpenGL backend.");
				chat.Add("&cAs such you may experience poor performance.");
				chat.Add("&cIt is likely you need to install video card drivers.");
			}
			
			if (!isIntelRenderer) return false;
			
			chat.Add("&cIntel graphics cards are known to have issues with the OpenGL build.");
			chat.Add("&cVSync may not work, and you may see disappearing clouds and map edges.");
			chat.Add("&cFor Windows, try downloading the Direct3D 9 build instead.");
			return true;
		}
		
		// Based on http://www.opentk.com/doc/graphics/save-opengl-rendering-to-disk
		public override void TakeScreenshot(string output, int width, int height) {
			using (Bitmap bmp = new Bitmap(width, height, BmpPixelFormat.Format32bppRgb)) { // ignore alpha component
				using (FastBitmap fastBmp = new FastBitmap(bmp, true, false))
					GL.ReadPixels(0, 0, width, height, GlPixelFormat.Bgra, PixelType.UnsignedByte, fastBmp.Scan0);
				bmp.RotateFlip(RotateFlipType.RotateNoneFlipY);
				bmp.Save(output, ImageFormat.Png);
			}
		}
		
		public override void OnWindowResize(Game game) {
			GL.Viewport(0, 0, game.Width, game.Height);
		}
		
		void InitFields() {
			// See comment in KeyMap() constructor for why this is necessary.
			blendFuncs = new BlendingFactor[6];
			blendFuncs[0] = BlendingFactor.Zero; blendFuncs[1] = BlendingFactor.One;
			blendFuncs[2] = BlendingFactor.SrcAlpha; blendFuncs[3] = BlendingFactor.OneMinusSrcAlpha;
			blendFuncs[4] = BlendingFactor.DstAlpha; blendFuncs[5] = BlendingFactor.OneMinusDstAlpha;
			compareFuncs = new Compare[8];
			compareFuncs[0] = Compare.Always; compareFuncs[1] = Compare.Notequal;
			compareFuncs[2] = Compare.Never; compareFuncs[3] = Compare.Less;
			compareFuncs[4] = Compare.Lequal; compareFuncs[5] = Compare.Equal;
			compareFuncs[6] = Compare.Gequal; compareFuncs[7] = Compare.Greater;

			fogModes = new FogMode[3];
			fogModes[0] = FogMode.Linear; fogModes[1] = FogMode.Exp;
			fogModes[2] = FogMode.Exp2;
			matrixModes = new MatrixMode[3];
			matrixModes[0] = MatrixMode.Projection; matrixModes[1] = MatrixMode.Modelview;
			matrixModes[2] = MatrixMode.Texture;
		}
	}
}
#endif