// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if ANDROID
using System;
using System.IO;
using OpenTK;
using OpenTK.Graphics.ES11;
using OpenTK.Platform.Android;
using Android.Graphics;

namespace ClassicalSharp.GraphicsAPI {

	/// <summary> Implements IGraphicsAPI using OpenGL ES 1.1 </summary>
	public unsafe class OpenGLESApi : IGraphicsApi {
		
		public OpenGLESApi() {
			InitFields();
			int texDims;
			GL.GetInteger(All.MaxTextureSize, &texDims);
			textureDims = texDims;
			base.InitDynamicBuffers();
			// TODO: Support mipmaps
			
			setupBatchFuncCol4b = SetupVbPos3fCol4b;
			setupBatchFuncTex2fCol4b = SetupVbPos3fTex2fCol4b;
			GL.EnableClientState(All.VertexArray);
			GL.EnableClientState(All.ColorArray);
		}

		public override bool AlphaTest { set { Toggle(All.AlphaTest, value); } }
		
		public override bool AlphaBlending { set { Toggle(All.Blend, value); } }
		
		All[] compareFuncs;
		public override void AlphaTestFunc(CompareFunc func, float value) {
			GL.AlphaFunc(compareFuncs[(int)func], value);
		}
		
		All[] blendFuncs;
		public override void AlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc) {
			GL.BlendFunc(blendFuncs[(int)srcFunc], blendFuncs[(int)dstFunc]);
		}
		
		bool fogEnable;
		public override bool Fog {
			get { return fogEnable; }
			set { fogEnable = value; Toggle(All.Fog, value); } 
		}
		
		FastColour lastFogCol = FastColour.Black;
		public override void SetFogColour(FastColour col) {
			if (col != lastFogCol) {
				Vector4 colRGBA = new Vector4(col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f);
				GL.Fog(All.FogColor, &colRGBA.X);
				lastFogCol = col;
			}
		}
		
		float lastFogStart = -1, lastFogEnd = -1, lastFogDensity = -1;
		public override void SetFogDensity(float value) {
			FogParam(All.FogDensity, value, ref lastFogDensity);
		}
		
		public override void SetFogStart(float value) {
			FogParam(All.FogStart, value, ref lastFogStart);
		}
		
		public override void SetFogEnd(float value) {
			FogParam(All.FogEnd, value, ref lastFogEnd);
		}
		
		static void FogParam(All param, float value, ref float last) {
			if (value != last) {
				GL.Fog(param, value);
				last = value;
			}
		}
		
		Fog lastFogMode = (Fog)999;
		All[] fogModes;
		public override void SetFogMode(Fog mode) {
			if (mode != lastFogMode) {
				GL.Fog(All.FogMode, (int)fogModes[(int)mode]);
				lastFogMode = mode;
			}
		}
		
		public override bool FaceCulling {
			set {
				if (value) GL.Enable(All.CullFace);
				else GL.Disable(All.CullFace);
			}
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
		
		public override bool DepthTest { set { Toggle(All.DepthTest, value); } }
		
		public override bool DepthWrite { set { GL.DepthMask(value); } }
		
		public override bool AlphaArgBlend { set { } }
		
		#region Texturing
		
		int textureDims;
		public override int MaxTextureDimensions { get { return textureDims; } }
		
		public override bool Texturing { set { Toggle(All.Texture2D, value); } }
		
		protected override int CreateTexture(int width, int height, IntPtr scan0, bool managedPool) {
			int texId = 0;
			GL.GenTextures(1, &texId);
			GL.BindTexture(All.Texture2D, texId);
			GL.TexParameter(All.Texture2D, All.TextureMinFilter, (int)All.Nearest);
			GL.TexParameter(All.Texture2D, All.TextureMagFilter, (int)All.Nearest);

			GL.TexImage2D(All.Texture2D, 0, (int)All.Rgba, width, height,
				0, All.BgraExt, All.UnsignedByte, scan0);
			return texId;
		}
		
		public override void BindTexture(int texture) {
			GL.BindTexture(All.Texture2D, texture);
		}
		
		public override void UpdateTexturePart(int texId, int x, int y, FastBitmap part) {
			GL.BindTexture(All.Texture2D, texId);
			GL.TexSubImage2D(All.Texture2D, 0, x, y, part.Width, part.Height,
				All.BgraExt, All.UnsignedByte, part.Scan0);
		}
		
		public override void DeleteTexture(ref int texId) {
			if (texId <= 0) return;
			int id = texId;
			GL.DeleteTextures(1, &id);
			texId = -1;
		}
		#endregion
		
		#region Vertex/index buffers
		Action setupBatchFunc, setupBatchFuncCol4b, setupBatchFuncTex2fCol4b;
		
		public override int CreateDynamicVb(VertexFormat format, int maxVertices) {
			int id = GenAndBind(All.ArrayBuffer);
			int sizeInBytes = maxVertices * strideSizes[(int)format];
			GL.BufferData(All.ArrayBuffer, new IntPtr(sizeInBytes), IntPtr.Zero, All.DynamicDraw);
			return id;
		}
		
		public override int CreateVb(IntPtr vertices, VertexFormat format, int count) {
			int id = GenAndBind(All.ArrayBuffer);
			int sizeInBytes = count * strideSizes[(int)format];
			GL.BufferData(All.ArrayBuffer, new IntPtr(sizeInBytes), vertices, All.StaticDraw);
			return id;
		}
		
		public override int CreateIb(IntPtr indices, int indicesCount) {
			int id = GenAndBind(All.ElementArrayBuffer);
			int sizeInBytes = indicesCount * sizeof(ushort);
			GL.BufferData(All.ElementArrayBuffer, new IntPtr(sizeInBytes), indices, All.StaticDraw);
			return id;
		}
		
		static int GenAndBind(All target) {
			int id = 0;
			GL.GenBuffers(1, &id);
			GL.BindBuffer(target, id);
			return id;
		}
		
		int batchStride;		
		public override void SetDynamicVbData(int id, IntPtr vertices, int count) {
			GL.BindBuffer(All.ArrayBuffer, id);
			GL.BufferSubData(All.ArrayBuffer, IntPtr.Zero, 
			                 new IntPtr(count * batchStride), vertices);
		}
		
		public override void DeleteVb(ref int vb) {
			if (vb <= 0) return;
			int id = vb; GL.DeleteBuffers(1, &id);
			vb = -1;
		}
		
		public override void DeleteIb(ref int ib) {
			if (ib <= 0) return;
			int id = ib; GL.DeleteBuffers(1, &id);
			ib = -1;
		}
		
		VertexFormat batchFormat = (VertexFormat)999;
		public override void SetBatchFormat(VertexFormat format) {
			if (format == batchFormat) return;
			
			if (batchFormat == VertexFormat.P3fT2fC4b) {
				GL.DisableClientState(All.TextureCoordArray);
			}
			
			batchFormat = format;
			if (format == VertexFormat.P3fT2fC4b) {
				GL.EnableClientState(All.TextureCoordArray);
				setupBatchFunc = setupBatchFuncTex2fCol4b;
				batchStride = VertexP3fT2fC4b.Size;
			} else {
				setupBatchFunc = setupBatchFuncCol4b;
				batchStride = VertexP3fC4b.Size;
			}
		}
		
		public override void BindVb(int vb) {
			GL.BindBuffer(All.ArrayBuffer, vb);
		}

		public override void BindIb(int ib) {
			GL.BindBuffer(All.ElementArrayBuffer, ib);
		}
		
		const All indexType = All.UnsignedShort;
		public override void DrawVb_Lines(int verticesCount) {
			setupBatchFunc();
			GL.DrawArrays(All.Lines, 0, verticesCount);
		}		
		
		public override void DrawVb_IndexedTris(int indicesCount, int startIndex) {
			setupBatchFunc();
			GL.DrawElements(All.Triangles, indicesCount, indexType, new IntPtr(startIndex * 2));
		}
		
		public override void DrawVb_IndexedTris(int indicesCount) {
			setupBatchFunc();
			GL.DrawElements(All.Triangles, indicesCount, indexType, IntPtr.Zero);
		}
		
		internal override void DrawIndexedVb_TrisT2fC4b(int indicesCount, int startIndex) {
			GL.VertexPointer(3, All.Float, 24, zero);
			GL.ColorPointer(4, All.UnsignedByte, 24, twelve);
			GL.TexCoordPointer(2, All.Float, 24, sixteen);
			GL.DrawElements(All.Triangles, indicesCount, indexType, new IntPtr(startIndex * 2));
		}
		
		internal override void DrawIndexedVb_TrisT2fC4b(int indicesCount, int startVertex, int startIndex) {
			int offset = startVertex * VertexP3fT2fC4b.Size;
			GL.VertexPointer(3, All.Float, 24, new IntPtr(offset));
			GL.ColorPointer(4, All.UnsignedByte, 24, new IntPtr(offset + 12));
			GL.TexCoordPointer(2, All.Float, 24, new IntPtr(offset + 16));
			GL.DrawElements(All.Triangles, indicesCount, indexType, new IntPtr(startIndex * 2));
		}
		
		IntPtr zero = new IntPtr(0), twelve = new IntPtr(12), sixteen = new IntPtr(16);
		
		void SetupVbPos3fCol4b() {
			GL.VertexPointer(3, All.Float, VertexP3fC4b.Size, zero);
			GL.ColorPointer(4, All.UnsignedByte, VertexP3fC4b.Size, twelve);
		}
		
		void SetupVbPos3fTex2fCol4b() {
			GL.VertexPointer(3, All.Float, VertexP3fT2fC4b.Size, zero);
			GL.ColorPointer(4, All.UnsignedByte, VertexP3fT2fC4b.Size, twelve);
			GL.TexCoordPointer(2, All.Float, VertexP3fT2fC4b.Size, sixteen);
		}
		#endregion
		
		#region Matrix manipulation
		All lastMode = 0;
		All[] matrixModes;
		public override void SetMatrixMode(MatrixType mode) {
			All glMode = matrixModes[(int)mode];
			if (glMode != lastMode) {
				GL.MatrixMode(glMode);
				lastMode = glMode;
			}
		}
		
		public override void LoadMatrix(ref Matrix4 matrix) {
			fixed(Single* ptr = &matrix.Row0.X)
				GL.LoadMatrix(ptr);
		}
		
		public override void LoadIdentityMatrix() {
			GL.LoadIdentity();
		}
		
		public override void PushMatrix() {
			GL.PushMatrix();
		}
		
		public override void PopMatrix() {
			GL.PopMatrix();
		}
		
		public override void MultiplyMatrix(ref Matrix4 matrix) {
			fixed(Single* ptr = &matrix.Row0.X)
				GL.MultMatrix(ptr);
		}
		
		#endregion
		
		public override void BeginFrame(Game game) {
		}
		
		public override void EndFrame(Game game) {
			game.window.SwapBuffers();
		}
		
		public override void SetVSync(Game game, bool value) {
			//game.VSync = value; TODO: vsync
		}
		
		internal override void MakeApiInfo() {
			string vendor = GL.GetString(All.Vendor);
			string renderer = GL.GetString(All.Renderer);
			string version = GL.GetString(All.Version);
			int depthBits = 0;
			GL.GetInteger(All.DepthBits, &depthBits);
			
			ApiInfo = new string[] {
				"--Using OpenGL api--",
				"Vendor: " + vendor,
				"Renderer: " + renderer,
				"GL version: " + version,
				"Max 2D texture dimensions: " + MaxTextureDimensions,
				"Depth buffer bits: " + depthBits,
			};
		}
		
		public override bool WarnIfNecessary(Chat chat) {
			return false;
		}
		
		// Based on http://www.opentk.com/doc/graphics/save-opengl-rendering-to-disk
		public override void TakeScreenshot(string output, int width, int height) {
			using (Bitmap bmp = Bitmap.CreateBitmap(width, height, Bitmap.Config.Argb8888)) { // ignore alpha component
				using (FastBitmap fastBmp = new FastBitmap(bmp, true)) {
					GL.ReadPixels(0, 0, width, height, All.BgraExt, All.UnsignedByte, fastBmp.Scan0);
					// flip vertically around y
					for (int y = 0; y < height / 2; y++) {
						int* src = fastBmp.GetRowPtr(y);
						int* dst = fastBmp.GetRowPtr(height - y - 1);
						for (int x = 0; x < fastBmp.Width; x++) {
							int temp = dst[x]; dst[x] = src[x]; src[x] = temp;
						}
					}
				}
				using (FileStream fs = File.Create(output))
					Platform.WriteBmp(bmp, fs);
			}
		}
		
		public override void OnWindowResize(Game game) {
			GL.Viewport(0, 0, game.Width, game.Height);
		}
		
		static void Toggle(All cap, bool value) {
			if (value) GL.Enable(cap);
			else GL.Disable(cap);
		}
		
		void InitFields() {
			blendFuncs = new All[6];
			blendFuncs[0] = All.Zero; blendFuncs[1] = All.One;
			blendFuncs[2] = All.SrcAlpha; blendFuncs[3] = All.OneMinusSrcAlpha;
			blendFuncs[4] = All.DstAlpha; blendFuncs[5] = All.OneMinusDstAlpha;
			compareFuncs = new All[8];
			compareFuncs[0] = All.Always; compareFuncs[1] = All.Notequal;
			compareFuncs[2] = All.Never; compareFuncs[3] = All.Less;
			compareFuncs[4] = All.Lequal; compareFuncs[5] = All.Equal;
			compareFuncs[6] = All.Gequal; compareFuncs[7] = All.Greater;
			fogModes = new All[3];
			fogModes[0] = All.Linear; fogModes[1] = All.Exp;
			fogModes[2] = All.Exp2;
			matrixModes = new All[3];
			matrixModes[0] = All.Projection; matrixModes[1] = All.Modelview;
			matrixModes[2] = All.Texture;
		}
	}
}
#endif