// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if USE_DX && !ANDROID
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Threading;
using OpenTK;
using SharpDX;
using SharpDX.Direct3D9;
using D3D = SharpDX.Direct3D9;

namespace ClassicalSharp.GraphicsAPI {

	/// <summary> Implements IGraphicsAPI using Direct3D 9. </summary>
	public class Direct3D9Api : IGraphicsApi {

		Device device;
		Direct3D d3d;
		Capabilities caps;
		const int texBufferSize = 512, iBufferSize = 32, vBufferSize = 2048;
		
		D3D.Texture[] textures = new D3D.Texture[texBufferSize];
		DataBuffer[] vBuffers = new DataBuffer[vBufferSize];
		DataBuffer[] iBuffers = new DataBuffer[iBufferSize];
		
		TransformState curMatrix;
		PrimitiveType[] modeMappings;
		Format[] depthFormats, viewFormats;
		Format depthFormat, viewFormat;
		CreateFlags createFlags = CreateFlags.HardwareVertexProcessing;

		public Direct3D9Api(Game game) {
			MinZNear = 0.05f;
			IntPtr windowHandle = game.window.WindowInfo.WinHandle;
			d3d = new Direct3D();
			int adapter = d3d.Adapters[0].Adapter;
			InitFields();
			FindCompatibleFormat(adapter);
			
			PresentParameters args = GetPresentArgs(640, 480);
			try {
				device = d3d.CreateDevice(adapter, DeviceType.Hardware, windowHandle, createFlags, args);
			} catch (SharpDXException) {
				createFlags = CreateFlags.MixedVertexProcessing;
				try {
					device = d3d.CreateDevice(adapter, DeviceType.Hardware, windowHandle, createFlags, args);
				} catch (SharpDXException) {
					createFlags = CreateFlags.SoftwareVertexProcessing;
					device = d3d.CreateDevice(adapter, DeviceType.Hardware, windowHandle, createFlags, args);
				}
			}
			
			CustomMipmapsLevels = true;
			caps = device.Capabilities;
			SetDefaultRenderStates();
			InitDynamicBuffers();
		}
		
		void FindCompatibleFormat(int adapter) {
			for (int i = 0; i < viewFormats.Length; i++) {
				viewFormat = viewFormats[i];
				if (d3d.CheckDeviceType(adapter, DeviceType.Hardware, viewFormat, viewFormat, true)) break;
				
				if (i == viewFormats.Length - 1)
					throw new InvalidOperationException("Unable to create a back buffer with sufficient precision.");
			}
			
			for (int i = 0; i < depthFormats.Length; i++) {
				depthFormat = depthFormats[i];
				if (d3d.CheckDepthStencilMatch(adapter, DeviceType.Hardware, viewFormat, viewFormat, depthFormat)) break;
				
				if (i == depthFormats.Length - 1)
					throw new InvalidOperationException("Unable to create a depth buffer with sufficient precision.");
			}
		}
		
		bool alphaTest, alphaBlend;
		public override bool AlphaTest {
			set { if (value == alphaTest) return;
				alphaTest = value; device.SetRenderState(RenderState.AlphaTestEnable, value);
			}
		}

		public override bool AlphaBlending {
			set { if (value == alphaBlend) return;
				alphaBlend = value; device.SetRenderState(RenderState.AlphaBlendEnable, value);
			}
		}

		Compare[] compareFuncs;
		Compare alphaTestFunc;
		int alphaTestRef;
		public override void AlphaTestFunc(CompareFunc func, float value) {
			alphaTestFunc = compareFuncs[(int)func];
			device.SetRenderState(RenderState.AlphaFunc, (int)alphaTestFunc);
			alphaTestRef = (int)(value * 255);
			device.SetRenderState(RenderState.AlphaRef, alphaTestRef);
		}

		Blend[] blendFuncs;
		Blend srcBlendFunc, dstBlendFunc;
		public override void AlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc) {
			srcBlendFunc = blendFuncs[(int)srcFunc];
			dstBlendFunc = blendFuncs[(int)dstFunc];
			device.SetRenderState(RenderState.SourceBlend, (int)srcBlendFunc);
			device.SetRenderState(RenderState.DestinationBlend, (int)dstBlendFunc);
		}

		bool fogEnable;
		public override bool Fog {
			get { return fogEnable; }
			set { if (value == fogEnable) return;
				fogEnable = value; device.SetRenderState(RenderState.FogEnable, value);
			}
		}

		int fogCol, lastFogCol = FastColour.BlackPacked;
		public override void SetFogColour(FastColour col) {
			fogCol = col.Pack();
			if (fogCol == lastFogCol) return;
			
			device.SetRenderState(RenderState.FogColor, fogCol);
			lastFogCol = fogCol;
		}

		float fogDensity = -1, fogStart = -1, fogEnd = -1;
		public override void SetFogDensity(float value) {
			if (value == fogDensity) return;
			fogDensity = value;
			device.SetRenderState(RenderState.FogDensity, value);
		}
		
		public override void SetFogStart(float value) {
			device.SetRenderState(RenderState.FogStart, value);
		}

		public override void SetFogEnd(float value) {
			if (value == fogEnd) return;
			fogEnd = value;
			device.SetRenderState(RenderState.FogEnd, value);
		}

		FogMode[] modes;
		FogMode fogTableMode;
		public override void SetFogMode(Fog fogMode) {
			FogMode newMode = modes[(int)fogMode];
			if (newMode == fogTableMode) return;
			fogTableMode = newMode;
			device.SetRenderState(RenderState.FogTableMode, (int)fogTableMode);
		}
		
		public override bool FaceCulling {
			set {
				Cull mode = value ? Cull.Clockwise : Cull.None;
				device.SetRenderState(RenderState.CullMode, (int)mode);
			}
		}

		public override int MaxTextureDimensions {
			get { return Math.Min(caps.MaxTextureHeight, caps.MaxTextureWidth); }
		}
		
		public override bool Texturing {
			set { if (!value) device.SetTexture(0, null); }
		}

		protected override int CreateTexture(int width, int height, IntPtr scan0, bool managedPool, bool mipmaps) {
			D3D.Texture texture = null;
			int levels = 1 + (mipmaps ? MipmapsLevels(width, height) : 0);
			
			if (managedPool) {
				texture = device.CreateTexture(width, height, levels, Usage.None, Format.A8R8G8B8, Pool.Managed);
				texture.SetData(0, LockFlags.None, scan0, width * height * 4);
				
				if (mipmaps) DoMipmaps(texture, 0, 0, width, height, scan0, false);
			} else {
				D3D.Texture sys = device.CreateTexture(width, height, levels, Usage.None, Format.A8R8G8B8, Pool.SystemMemory);
				sys.SetData(0, LockFlags.None, scan0, width * height * 4);
				
				texture = device.CreateTexture(width, height, levels, Usage.None, Format.A8R8G8B8, Pool.Default);
				device.UpdateTexture(sys, texture);				
				sys.Dispose();
			}
			return GetOrExpand(ref textures, texture, texBufferSize);
		}
			
		unsafe void DoMipmaps(D3D.Texture texture, int x, int y, int width, 
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
					texture.SetPartData(lvl, LockFlags.None, cur, x, y, width, height);
				} else {
					texture.SetData(lvl, LockFlags.None, cur, size);
				}
				
				if (prev != scan0) Marshal.FreeHGlobal(prev);
				prev = cur;
			}
			if (prev != scan0) Marshal.FreeHGlobal(prev);
		}
		
		public override void UpdateTexturePart(int texId, int x, int y, FastBitmap part, bool mipmaps) {
			D3D.Texture texture = textures[texId];
			texture.SetPartData(0, LockFlags.None, part.Scan0, x, y, part.Width, part.Height);
			if (Mipmaps) DoMipmaps(texture, x, y, part.Width, part.Height, part.Scan0, true);
		}

		public override void BindTexture(int texId) {
			device.SetTexture(0, textures[texId]);
		}

		public override void DeleteTexture(ref int texId) {
			Delete(textures, ref texId);
		}
		
		public override void EnableMipmaps() {
			if (Mipmaps) {
				device.SetSamplerState(0, SamplerState.MipFilter, (int)TextureFilter.Linear);
			}
		}
		
		public override void DisableMipmaps() {
			if (Mipmaps) {
				device.SetSamplerState(0, SamplerState.MipFilter, (int)TextureFilter.None);
			}
		}
		

		int lastClearCol;
		public override void Clear() {
			device.Clear(ClearFlags.Target | ClearFlags.ZBuffer, lastClearCol, 1f, 0);
		}

		public override void ClearColour(FastColour col) {
			lastClearCol = col.Pack();
		}

		public override bool ColourWrite {
			set { device.SetRenderState(RenderState.ColorWriteEnable, value ? 0xF : 0x0); }
		}

		Compare depthTestFunc;
		public override void DepthTestFunc(CompareFunc func) {
			depthTestFunc = compareFuncs[(int)func];
			device.SetRenderState(RenderState.ZFunc, (int)depthTestFunc);
		}

		bool depthTest, depthWrite;
		public override bool DepthTest {
			set { depthTest = value; device.SetRenderState(RenderState.ZEnable, value); }
		}

		public override bool DepthWrite {
			set { depthWrite = value; device.SetRenderState(RenderState.ZWriteEnable, value); }
		}
		
		public override bool AlphaArgBlend {
			set {
				TextureOp op = value ? TextureOp.Modulate : TextureOp.SelectArg1;
				device.SetTextureStageState(0, TextureStage.AlphaOperation, (int)op);
			}
		}
		
		#region Vertex buffers
		
		public override int CreateDynamicVb(VertexFormat format, int maxVertices) {
			int size = maxVertices * strideSizes[(int)format];
			DataBuffer buffer = device.CreateVertexBuffer(size, Usage.Dynamic | Usage.WriteOnly,
			                                              formatMapping[(int)format], Pool.Default);
			return GetOrExpand(ref vBuffers, buffer, iBufferSize);
		}
		
		public override void SetDynamicVbData(int vb, IntPtr vertices, int count) {
			int size = count * batchStride;
			DataBuffer buffer = vBuffers[vb];
			buffer.SetData(vertices, size, LockFlags.Discard);
			device.SetStreamSource(0, buffer, 0, batchStride);
		}

		D3D.VertexFormat[] formatMapping;
		
		public override int CreateVb(IntPtr vertices, VertexFormat format, int count) {
			int size = count * strideSizes[(int)format];
			DataBuffer buffer = device.CreateVertexBuffer(size, Usage.WriteOnly, 
			                                              formatMapping[(int)format], Pool.Default);
			buffer.SetData(vertices, size, LockFlags.None);
			return GetOrExpand(ref vBuffers, buffer, vBufferSize);
		}
		
		public override int CreateIb(IntPtr indices, int indicesCount) {
			int size = indicesCount * sizeof(ushort);
			DataBuffer buffer = device.CreateIndexBuffer(size, Usage.WriteOnly,
			                                             Format.Index16, Pool.Managed);
			buffer.SetData(indices, size, LockFlags.None);
			return GetOrExpand(ref iBuffers, buffer, iBufferSize);
		}

		public override void DeleteVb(ref int vb) {
			Delete(vBuffers, ref vb);
		}
		
		public override void DeleteIb(ref int ib) {
			Delete(iBuffers, ref ib);
		}

		int batchStride;
		VertexFormat batchFormat = (VertexFormat)999;
		public override void SetBatchFormat(VertexFormat format) {
			if (format == batchFormat) return;
			batchFormat = format;
			
			device.SetVertexFormat(formatMapping[(int)format]);
			batchStride = strideSizes[(int)format];
		}
		
		public override void BindVb(int vb) {
			device.SetStreamSource(0, vBuffers[vb], 0, batchStride);
		}
		
		public override void BindIb(int ib) {
			device.SetIndices(iBuffers[ib]);
		}

		public override void DrawVb_Lines(int verticesCount) {
			device.DrawPrimitives(PrimitiveType.LineList, 0, verticesCount >> 1);
		}

		public override void DrawVb_IndexedTris(int verticesCount, int startVertex) {
			device.DrawIndexedPrimitives(PrimitiveType.TriangleList, startVertex, 0,
			                             verticesCount, 0, verticesCount >> 1);
		}

		public override void DrawVb_IndexedTris(int verticesCount) {
			device.DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0,
			                             verticesCount, 0, verticesCount >> 1);
		}
		
		internal override void DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex) {
			device.DrawIndexedPrimitives(PrimitiveType.TriangleList, startVertex, 0,
			                             verticesCount, 0, verticesCount >> 1);
		}
		#endregion


		#region Matrix manipulation

		public override void SetMatrixMode(MatrixType mode) {
			if (mode == MatrixType.Modelview) {
				curMatrix = TransformState.View;
			} else if (mode == MatrixType.Projection) {
				curMatrix = TransformState.Projection;
			} else if (mode == MatrixType.Texture) {
				curMatrix = TransformState.Texture0;
			}
		}

		public unsafe override void LoadMatrix(ref Matrix4 matrix) {
			if (curMatrix == TransformState.Texture0) {
				matrix.Row2.X = matrix.Row3.X; // NOTE: this hack fixes the texture movements.
				device.SetTextureStageState(0, TextureStage.TextureTransformFlags, (int)TextureTransform.Count2);
			}
			
			if (LostContext) return;
			device.SetTransform(curMatrix, ref matrix);
		}

		public override void LoadIdentityMatrix() {
			if (curMatrix == TransformState.Texture0) {
				device.SetTextureStageState(0, TextureStage.TextureTransformFlags, (int)TextureTransform.Disable);
			}
			
			if (LostContext) return;
			device.SetTransform(curMatrix, ref Matrix4.Identity);
		}
		
		public override void CalcOrthoMatrix(float width, float height, out Matrix4 matrix) {
			Matrix4.CreateOrthographicOffCenter(0, width, height, 0, -10000, 10000, out matrix);
			const float zN = -10000, zF = 10000;
			matrix.Row2.Z = 1 / (zN - zF);
			matrix.Row3.Z = zN / (zN - zF);
		}

		#endregion
		
		public override void BeginFrame(Game game) {
			device.BeginScene();
		}
		
		public override void EndFrame(Game game) {
			device.EndScene();
			int code = device.Present();
			if (code >= 0) return;
			
			if ((uint)code != (uint)Direct3DError.DeviceLost)
				throw new SharpDXException(code);
			
			// TODO: Make sure this actually works on all graphics cards.
			
			LoseContext(" (Direct3D9 device lost)");
			LoopUntilRetrieved();
			RecreateDevice(game);
		}
		
		void LoopUntilRetrieved() {
			ScheduledTask task = new ScheduledTask();
			task.Interval = 1.0 / 60;
			task.Callback = LostContextFunction;
			
			while (true) {
				Thread.Sleep(16);
				uint code = (uint)device.TestCooperativeLevel();
				if ((uint)code == (uint)Direct3DError.DeviceNotReset) return;
				
				task.Callback(task);
			}
		}
		
		
		bool vsync = false;
		public override void SetVSync(Game game, bool value) {
			game.VSync = value;
			if (vsync == value) return;
			vsync = value;
			
			LoseContext(" (toggling VSync)");
			RecreateDevice(game);
		}
		
		public override void OnWindowResize(Game game) {
			LoseContext(" (resizing window)");
			RecreateDevice(game);
		}
		
		void RecreateDevice(Game game) {
			PresentParameters args = GetPresentArgs(game.Width, game.Height);
			
			while ((uint)device.Reset(args) == (uint)Direct3DError.DeviceLost)
				LoopUntilRetrieved();
			
			SetDefaultRenderStates();
			RestoreRenderStates();
			RecreateContext();
		}
		
		void SetDefaultRenderStates() {
			FaceCulling = false;
			batchFormat = (VertexFormat)999;
			device.SetRenderState(RenderState.ColorVertex, false);
			device.SetRenderState(RenderState.Lighting, false);
			device.SetRenderState(RenderState.SpecularEnable, false);
			device.SetRenderState(RenderState.LocalViewer, false);
			device.SetRenderState(RenderState.DebugMonitorToken, false);
		}
		
		void RestoreRenderStates() {
			device.SetRenderState(RenderState.AlphaTestEnable, alphaTest);
			device.SetRenderState(RenderState.AlphaBlendEnable, alphaBlend);
			device.SetRenderState(RenderState.AlphaFunc, (int)alphaTestFunc);
			device.SetRenderState(RenderState.AlphaRef, alphaTestRef);
			device.SetRenderState(RenderState.SourceBlend, (int)srcBlendFunc);
			device.SetRenderState(RenderState.DestinationBlend, (int)dstBlendFunc);
			device.SetRenderState(RenderState.FogEnable, fogEnable);
			device.SetRenderState(RenderState.FogColor, fogCol);
			device.SetRenderState(RenderState.FogDensity, fogDensity);
			device.SetRenderState(RenderState.FogStart, fogStart);
			device.SetRenderState(RenderState.FogEnd, fogEnd);
			device.SetRenderState(RenderState.FogTableMode, (int)fogTableMode);
			device.SetRenderState(RenderState.ZFunc, (int)depthTestFunc);
			device.SetRenderState(RenderState.ZEnable, depthTest);
			device.SetRenderState(RenderState.ZWriteEnable, depthWrite);
		}
		
		PresentParameters GetPresentArgs(int width, int height) {
			PresentParameters args = new PresentParameters();
			args.AutoDepthStencilFormat = depthFormat;
			args.BackBufferWidth = width;
			args.BackBufferHeight = height;
			args.BackBufferFormat = viewFormat;
			args.BackBufferCount = 1;
			args.EnableAutoDepthStencil = true;
			args.PresentationInterval = vsync ? PresentInterval.One : PresentInterval.Immediate;
			args.SwapEffect = SwapEffect.Discard;
			args.Windowed = true;
			return args;
		}

		static int GetOrExpand<T>(ref T[] array, T value, int expSize) {
			// Find first free slot
			for (int i = 1; i < array.Length; i++) {
				if (array[i] == null) {
					array[i] = value;
					return i;
				}
			}
			
			// Otherwise resize and add more elements
			int oldLength = array.Length;
			Array.Resize(ref array, array.Length + expSize);
			array[oldLength] = value;
			return oldLength;
		}
		
		static void Delete<T>(T[] array, ref int id) where T : class, IDisposable {
			if (id <= 0 || id >= array.Length) return;
			
			T value = array[id];
			if (value != null) {
				value.Dispose();
			}
			array[id] = null;
			id = -1;
		}

		public override void Dispose() {
			base.Dispose();
			device.Dispose();
			d3d.Dispose();
		}

		internal override void MakeApiInfo() {
			string adapter = d3d.Adapters[0].Details.Description;
			float texMem = device.AvailableTextureMemory / 1024f / 1024f;
			
			ApiInfo = new string[] {
				"-- Using Direct3D9 api --",
				"Adapter: " + adapter,
				"Mode: " + createFlags,
				"Texture memory: " + texMem + " MB",
				"Max 2D texture dimensions: " + MaxTextureDimensions,
				"Depth buffer format: " + depthFormat,
				"Back buffer format: " + viewFormat,
			};
		}

		public override void TakeScreenshot(string output, int width, int height) {
			using (Surface backbuffer = device.GetBackBuffer(0, 0, BackBufferType.Mono),
			       tempSurface = device.CreateOffscreenPlainSurface(width, height, Format.X8R8G8B8, Pool.SystemMemory)) {
				// For DX 8 use IDirect3DDevice8::CreateImageSurface
				device.GetRenderTargetData(backbuffer, tempSurface);
				LockedRectangle rect = tempSurface.LockRectangle(LockFlags.ReadOnly | LockFlags.NoDirtyUpdate);
				
				using (Bitmap bmp = new Bitmap(width, height, width * sizeof(int),
				                               PixelFormat.Format32bppRgb, rect.DataPointer)) {
					bmp.Save(output, ImageFormat.Png);
				}
				tempSurface.UnlockRectangle();
			}
		}
		
		void InitFields() {
			// See comment in KeyMap() constructor for why this is necessary.
			modeMappings = new PrimitiveType[2];
			modeMappings[0] = PrimitiveType.TriangleList; modeMappings[1] = PrimitiveType.LineList;
			depthFormats = new Format[6];
			depthFormats[0] = Format.D32; depthFormats[1] = Format.D24X8; depthFormats[2] = Format.D24S8;
			depthFormats[3] = Format.D24X4S4; depthFormats[4] = Format.D16; depthFormats[5] = Format.D15S1;
			viewFormats = new Format[4];
			viewFormats[0] = Format.X8R8G8B8; viewFormats[1] = Format.R8G8B8;
			viewFormats[2] = Format.R5G6B5; viewFormats[3] = Format.X1R5G5B5;
			
			blendFuncs = new Blend[6];
			blendFuncs[0] = Blend.Zero; blendFuncs[1] = Blend.One; blendFuncs[2] = Blend.SourceAlpha;
			blendFuncs[3] = Blend.InverseSourceAlpha; blendFuncs[4] = Blend.DestinationAlpha;
			blendFuncs[5] = Blend.InverseDestinationAlpha;
			compareFuncs = new Compare[8];
			compareFuncs[0] = Compare.Always; compareFuncs[1] = Compare.NotEqual; compareFuncs[2] = Compare.Never;
			compareFuncs[3] = Compare.Less; compareFuncs[4] = Compare.LessEqual; compareFuncs[5] = Compare.Equal;
			compareFuncs[6] = Compare.GreaterEqual; compareFuncs[7] = Compare.Greater;
			
			formatMapping = new D3D.VertexFormat[2];
			formatMapping[0] = D3D.VertexFormat.Position | D3D.VertexFormat.Diffuse;
			formatMapping[1] = D3D.VertexFormat.Position | D3D.VertexFormat.Texture2 | D3D.VertexFormat.Diffuse;
			modes = new FogMode[3];
			modes[0] = FogMode.Linear; modes[1] = FogMode.Exponential; modes[2] = FogMode.ExponentialSquared;
		}
	}
}
#endif