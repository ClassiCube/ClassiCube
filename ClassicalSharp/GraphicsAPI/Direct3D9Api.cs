// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if USE_DX && !ANDROID
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using OpenTK;
using SharpDX;
using SharpDX.Direct3D9;
using D3D = SharpDX.Direct3D9;

namespace ClassicalSharp.GraphicsAPI {

	/// <summary> Implements IGraphicsAPI using Direct3D 9. </summary>
	public class Direct3D9Api : IGraphicsApi {

		IntPtr device, d3d;
		Capabilities caps;
		const int texBufferSize = 512, iBufferSize = 4, vBufferSize = 2048;
		
		IntPtr[] textures = new IntPtr[texBufferSize];
		IntPtr[] vBuffers = new IntPtr[vBufferSize];
		IntPtr[] iBuffers = new IntPtr[iBufferSize];
		
		TransformState curMatrix;
		PrimitiveType[] modeMappings;
		Format[] depthFormats, viewFormats;
		Format depthFormat, viewFormat;
		CreateFlags createFlags = CreateFlags.HardwareVertexProcessing;

		public Direct3D9Api(Game game) {
			MinZNear = 0.05f;
			IntPtr winHandle = game.window.WinHandle;
			d3d = Direct3D.Direct3DCreate9(Direct3D.SdkVersion);
			const int adapter = 0; // default adapter
			InitFields();
			FindCompatibleFormat(adapter);
			
			PresentParameters args = GetPresentArgs(640, 480);
			try {
				device = Direct3D.CreateDevice(d3d, adapter, DeviceType.Hardware, winHandle, createFlags, args);
			} catch (SharpDXException) {
				createFlags = CreateFlags.MixedVertexProcessing;
				try {
					device = Direct3D.CreateDevice(d3d, adapter, DeviceType.Hardware, winHandle, createFlags, args);
				} catch (SharpDXException) {
					createFlags = CreateFlags.SoftwareVertexProcessing;
					device = Direct3D.CreateDevice(d3d, adapter, DeviceType.Hardware, winHandle, createFlags, args);
				}
			}
			
			CustomMipmapsLevels = true;
			caps = Device.GetCapabilities(device);
			SetDefaultRenderStates();
			InitCommon();
		}
		
		void FindCompatibleFormat(int adapter) {
			for (int i = 0; i < viewFormats.Length; i++) {
				viewFormat = viewFormats[i];
				if (Direct3D.CheckDeviceType(d3d, adapter, DeviceType.Hardware, viewFormat, viewFormat, true)) break;
				
				if (i == viewFormats.Length - 1)
					throw new InvalidOperationException("Unable to create a back buffer with sufficient precision.");
			}
			
			for (int i = 0; i < depthFormats.Length; i++) {
				depthFormat = depthFormats[i];
				if (Direct3D.CheckDepthStencilMatch(d3d, adapter, DeviceType.Hardware, viewFormat, viewFormat, depthFormat)) break;
				
				if (i == depthFormats.Length - 1)
					throw new InvalidOperationException("Unable to create a depth buffer with sufficient precision.");
			}
		}
		
		bool alphaTest, alphaBlend;
		public override bool AlphaTest {
			set { if (value == alphaTest) return;
				alphaTest = value; Device.SetRenderState(device, RenderState.AlphaTestEnable, value);
			}
		}

		public override bool AlphaBlending {
			set { if (value == alphaBlend) return;
				alphaBlend = value; Device.SetRenderState(device, RenderState.AlphaBlendEnable, value);
			}
		}

		Compare[] compareFuncs;
		Compare alphaTestFunc = Compare.Always;
		int alphaTestRef;
		public override void AlphaTestFunc(CompareFunc func, float value) {
			alphaTestFunc = compareFuncs[(int)func];
			Device.SetRenderState(device, RenderState.AlphaFunc, (int)alphaTestFunc);
			alphaTestRef = (int)(value * 255);
			Device.SetRenderState(device, RenderState.AlphaRef, alphaTestRef);
		}

		Blend[] blendFuncs;
		Blend srcBlendFunc = Blend.One;
		Blend dstBlendFunc = Blend.Zero;
		public override void AlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc) {
			srcBlendFunc = blendFuncs[(int)srcFunc];
			dstBlendFunc = blendFuncs[(int)dstFunc];
			Device.SetRenderState(device, RenderState.SourceBlend, (int)srcBlendFunc);
			Device.SetRenderState(device, RenderState.DestinationBlend, (int)dstBlendFunc);
		}

		bool fogEnable;
		public override bool Fog {
			get { return fogEnable; }
			set {
				if (value == fogEnable) return;
				fogEnable = value;
				if (LostContext) return;
				Device.SetRenderState(device, RenderState.FogEnable, value);
			}
		}

		int fogCol, lastFogCol = FastColour.BlackPacked;
		public override void SetFogColour(FastColour col) {
			fogCol = col.Pack();
			if (fogCol == lastFogCol) return;
			lastFogCol = fogCol;
			if (LostContext) return;
			Device.SetRenderState(device, RenderState.FogColor, fogCol);
		}

		float fogDensity = -1, fogStart = -1, fogEnd = -1;
		public override void SetFogDensity(float value) {
			if (value == fogDensity) return;
			fogDensity = value;
			if (LostContext) return;
			Device.SetRenderState(device, RenderState.FogDensity, value);
		}
		
		public override void SetFogStart(float value) {
			fogStart = value;
			if (LostContext) return;
			Device.SetRenderState(device, RenderState.FogStart, value);
		}

		public override void SetFogEnd(float value) {
			if (value == fogEnd) return;
			fogEnd = value;
			if (LostContext) return;
			Device.SetRenderState(device, RenderState.FogEnd, value);
		}

		FogMode[] modes;
		FogMode fogTableMode;
		public override void SetFogMode(Fog fogMode) {
			FogMode newMode = modes[(int)fogMode];
			if (newMode == fogTableMode) return;
			fogTableMode = newMode;
			if (LostContext) return;
			Device.SetRenderState(device, RenderState.FogTableMode, (int)fogTableMode);
		}
		
		public override bool FaceCulling {
			set {
				Cull mode = value ? Cull.Clockwise : Cull.None;
				Device.SetRenderState(device, RenderState.CullMode, (int)mode);
			}
		}

		public override int MaxTextureDimensions {
			get { return Math.Min(caps.MaxTextureHeight, caps.MaxTextureWidth); }
		}
		
		public override bool Texturing {
			set { if (!value) Device.SetTexture(device, 0, IntPtr.Zero); }
		}

		protected override int CreateTexture(int width, int height, IntPtr scan0, bool managedPool, bool mipmaps) {
			IntPtr tex = IntPtr.Zero;
			int levels = 1 + (mipmaps ? MipmapsLevels(width, height) : 0);
			
			if (managedPool) {
				tex = Device.CreateTexture(device, width, height, levels, Usage.None, Format.A8R8G8B8, Pool.Managed);
				D3D.Texture.SetData(tex, 0, LockFlags.None, scan0, width * height * 4);
				if (mipmaps) DoMipmaps(tex, 0, 0, width, height, scan0, false);
			} else {
				IntPtr sys = Device.CreateTexture(device, width, height, levels, Usage.None, Format.A8R8G8B8, Pool.SystemMemory);
				D3D.Texture.SetData(sys, 0, LockFlags.None, scan0, width * height * 4);
				if (mipmaps) DoMipmaps(sys, 0, 0, width, height, scan0, false);
				
				tex = Device.CreateTexture(device, width, height, levels, Usage.None, Format.A8R8G8B8, Pool.Default);
				Device.UpdateTexture(device, sys, tex);
				Delete(ref sys);
			}
			return GetOrExpand(ref textures, tex, texBufferSize);
		}
		
		unsafe void DoMipmaps(IntPtr tex, int x, int y, int width,
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
					D3D.Texture.SetPartData(tex, lvl, LockFlags.None, cur, x, y, width, height);
				} else {
					D3D.Texture.SetData(tex, lvl, LockFlags.None, cur, size);
				}
				
				if (prev != scan0) Marshal.FreeHGlobal(prev);
				prev = cur;
			}
			if (prev != scan0) Marshal.FreeHGlobal(prev);
		}
		
		public override void UpdateTexturePart(int texId, int x, int y, FastBitmap part, bool mipmaps) {
			IntPtr tex = textures[texId];
			D3D.Texture.SetPartData(tex, 0, LockFlags.None, part.Scan0, x, y, part.Width, part.Height);
			if (Mipmaps) DoMipmaps(tex, x, y, part.Width, part.Height, part.Scan0, true);
		}

		public override void BindTexture(int texId) {
			Device.SetTexture(device, 0, textures[texId]);
		}

		public override void DeleteTexture(ref int texId) {
			Delete(textures, ref texId);
		}
		
		public override void EnableMipmaps() {
			if (Mipmaps) {
				Device.SetSamplerState(device, 0, SamplerState.MipFilter, (int)TextureFilter.Linear);
			}
		}
		
		public override void DisableMipmaps() {
			if (Mipmaps) {
				Device.SetSamplerState(device, 0, SamplerState.MipFilter, (int)TextureFilter.None);
			}
		}
		

		int lastClearCol;
		public override void Clear() {
			Device.Clear(device, ClearFlags.Target | ClearFlags.ZBuffer, lastClearCol, 1f, 0);
		}

		public override void ClearColour(FastColour col) {
			lastClearCol = col.Pack();
		}

		public override void ColourWriteMask(bool r, bool g, bool b, bool a) {
			int flags = (r ? 1 : 0) | (g ? 2 : 0) | (b ? 4 : 0) | (a ? 8 : 0);
			Device.SetRenderState(device, RenderState.ColorWriteEnable, flags);
		}

		Compare depthTestFunc = Compare.LessEqual;
		public override void DepthTestFunc(CompareFunc func) {
			depthTestFunc = compareFuncs[(int)func];
			Device.SetRenderState(device, RenderState.ZFunc, (int)depthTestFunc);
		}

		bool depthTest, depthWrite;
		public override bool DepthTest {
			set { depthTest = value; Device.SetRenderState(device, RenderState.ZEnable, value); }
		}

		public override bool DepthWrite {
			set { depthWrite = value; Device.SetRenderState(device, RenderState.ZWriteEnable, value); }
		}
		
		public override bool AlphaArgBlend {
			set {
				TextureOp op = value ? TextureOp.Modulate : TextureOp.SelectArg1;
				Device.SetTextureStageState(device, 0, TextureStage.AlphaOperation, (int)op);
			}
		}
		
		#region Vertex buffers
		
		public override int CreateDynamicVb(VertexFormat format, int maxVertices) {
			int size = maxVertices * strideSizes[(int)format];
			IntPtr buffer = Device.CreateVertexBuffer(device, size, Usage.Dynamic | Usage.WriteOnly,
			                                          formatMapping[(int)format], Pool.Default);
			return GetOrExpand(ref vBuffers, buffer, iBufferSize);
		}
		
		public override void SetDynamicVbData(int vb, IntPtr vertices, int count) {
			int size = count * batchStride;
			IntPtr buffer = vBuffers[vb];
			DataBuffser.SetData(buffer, vertices, size, LockFlags.Discard);
			Device.SetStreamSource(device, 0, buffer, 0, batchStride);
		}

		D3D.VertexFormat[] formatMapping;
		
		public override int CreateVb(IntPtr vertices, VertexFormat format, int count) {
			int size = count * strideSizes[(int)format];
			IntPtr buffer = Device.CreateVertexBuffer(device, size, Usage.WriteOnly,
			                                          formatMapping[(int)format], Pool.Default);
			DataBuffser.SetData(buffer, vertices, size, LockFlags.None);
			return GetOrExpand(ref vBuffers, buffer, vBufferSize);
		}
		
		public override int CreateIb(IntPtr indices, int indicesCount) {
			int size = indicesCount * sizeof(ushort);
			IntPtr buffer = Device.CreateIndexBuffer(device, size, Usage.WriteOnly,
			                                         Format.Index16, Pool.Default);
			DataBuffser.SetData(buffer, indices, size, LockFlags.None);
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
			
			Device.SetVertexFormat(device, formatMapping[(int)format]);
			batchStride = strideSizes[(int)format];
		}
		
		public override void BindVb(int vb) {
			Device.SetStreamSource(device, 0, vBuffers[vb], 0, batchStride);
		}
		
		public override void BindIb(int ib) {
			Device.SetIndices(device, iBuffers[ib]);
		}

		public override void DrawVb_Lines(int verticesCount) {
			Device.DrawPrimitives(device, PrimitiveType.LineList, 0, verticesCount >> 1);
		}

		public override void DrawVb_IndexedTris(int verticesCount, int startVertex) {
			Device.DrawIndexedPrimitives(device, PrimitiveType.TriangleList, startVertex, 0,
			                             verticesCount, 0, verticesCount >> 1);
		}

		public override void DrawVb_IndexedTris(int verticesCount) {
			Device.DrawIndexedPrimitives(device, PrimitiveType.TriangleList, 0, 0,
			                             verticesCount, 0, verticesCount >> 1);
		}
		
		internal override void DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex) {
			Device.DrawIndexedPrimitives(device, PrimitiveType.TriangleList, startVertex, 0,
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
				Device.SetTextureStageState(device, 0, TextureStage.TextureTransformFlags, (int)TextureTransform.Count2);
			}
			
			if (LostContext) return;
			Device.SetTransform(device , curMatrix, ref matrix);
		}

		public override void LoadIdentityMatrix() {
			if (curMatrix == TransformState.Texture0) {
				Device.SetTextureStageState(device, 0, TextureStage.TextureTransformFlags, (int)TextureTransform.Disable);
			}
			
			if (LostContext) return;
			Device.SetTransform(device, curMatrix, ref Matrix4.Identity);
		}
		
		public override void CalcOrthoMatrix(float width, float height, out Matrix4 matrix) {
			Matrix4.CreateOrthographicOffCenter(0, width, height, 0, -10000, 10000, out matrix);
			const float zN = -10000, zF = 10000;
			matrix.Row2.Z = 1 / (zN - zF);
			matrix.Row3.Z = zN / (zN - zF);
		}

		#endregion
		
		public override void BeginFrame(Game game) {
			Device.BeginScene(device);
		}
		
		public override void EndFrame(Game game) {
			Device.EndScene(device);
			int code = Device.Present(device);
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
				uint code = (uint)Device.TestCooperativeLevel(device);
				if ((uint)code == (uint)Direct3DError.DeviceNotReset) return;
				
				task.Callback(task);
			}
		}
		
		
		bool vsync = false;
		public override void SetVSync(Game game, bool value) {
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
			
			while ((uint)Device.Reset(device, args) == (uint)Direct3DError.DeviceLost)
				LoopUntilRetrieved();
			
			SetDefaultRenderStates();
			RestoreRenderStates();
			RecreateContext();
		}
		
		void SetDefaultRenderStates() {
			FaceCulling = false;
			batchFormat = (VertexFormat)999;
			Device.SetRenderState(device, RenderState.ColorVertex, false);
			Device.SetRenderState(device, RenderState.Lighting, false);
			Device.SetRenderState(device, RenderState.SpecularEnable, false);
			Device.SetRenderState(device, RenderState.LocalViewer, false);
			Device.SetRenderState(device, RenderState.DebugMonitorToken, false);
		}
		
		void RestoreRenderStates() {
			Device.SetRenderState(device, RenderState.AlphaTestEnable, alphaTest);
			Device.SetRenderState(device, RenderState.AlphaBlendEnable, alphaBlend);
			Device.SetRenderState(device, RenderState.AlphaFunc, (int)alphaTestFunc);
			Device.SetRenderState(device, RenderState.AlphaRef, alphaTestRef);
			Device.SetRenderState(device, RenderState.SourceBlend, (int)srcBlendFunc);
			Device.SetRenderState(device, RenderState.DestinationBlend, (int)dstBlendFunc);
			Device.SetRenderState(device, RenderState.FogEnable, fogEnable);
			Device.SetRenderState(device, RenderState.FogColor, fogCol);
			Device.SetRenderState(device, RenderState.FogDensity, fogDensity);
			Device.SetRenderState(device, RenderState.FogStart, fogStart);
			Device.SetRenderState(device, RenderState.FogEnd, fogEnd);
			Device.SetRenderState(device, RenderState.FogTableMode, (int)fogTableMode);
			Device.SetRenderState(device, RenderState.ZFunc, (int)depthTestFunc);
			Device.SetRenderState(device, RenderState.ZEnable, depthTest);
			Device.SetRenderState(device, RenderState.ZWriteEnable, depthWrite);
		}
		
		PresentParameters GetPresentArgs(int width, int height) {
			PresentParameters args = new PresentParameters();
			args.AutoDepthStencilFormat = depthFormat;
			args.BackBufferWidth = width;
			args.BackBufferHeight = height;
			args.BackBufferFormat = viewFormat;
			args.BackBufferCount = 1;
			args.EnableAutoDepthStencil = 1;
			args.PresentationInterval = vsync ? PresentInterval.One : PresentInterval.Immediate;
			args.SwapEffect = SwapEffect.Discard;
			args.Windowed = 1;
			return args;
		}

		static int GetOrExpand(ref IntPtr[] array, IntPtr value, int expSize) {
			// Find first free slot
			for (int i = 1; i < array.Length; i++) {
				if (array[i] == IntPtr.Zero) {
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
		
		static void Delete(ref IntPtr ptr) {
			if (ptr == IntPtr.Zero) return;
			
			int refCount = Marshal.Release(ptr);
			if (refCount > 0) {
				string text = String.Format("Warning: ComObject [0x{0:X}] is still referenced, potential memory leak: ({1})",
				                            ptr.ToInt64(), refCount);
			}
			ptr = IntPtr.Zero;
		}
		
		static void Delete(IntPtr[] array, ref int id) {
			if (id == 0 || id >= array.Length) return;
			
			Delete(ref array[id]);
			id = 0;
		}

		public override void Dispose() {
			base.Dispose();
			Delete(ref device);
			Delete(ref d3d);
		}

		internal override void MakeApiInfo() {
			AdapterDetails details = Direct3D.GetAdapterIdentifier(d3d, 0);
			string adapter = details.Description;
			float texMem = Device.GetAvailableTextureMemory(device) / 1024f / 1024f;
			
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

		public override void TakeScreenshot(Stream output, int width, int height) {
			IntPtr backbuffer = Device.GetBackBuffer(device, 0, 0, BackBufferType.Mono);
			IntPtr temp = Device.CreateOffscreenPlainSurface(device, width, height, Format.X8R8G8B8, Pool.SystemMemory);
			// For DX 8 use IDirect3DDevice8::CreateImageSurface
			
			Device.GetRenderTargetData(device, backbuffer, temp);
			LockedRectangle rect = Surface.LockRectangle(temp, LockFlags.ReadOnly | LockFlags.NoDirtyUpdate);
			
			using (Bitmap bmp = new Bitmap(width, height, width * sizeof(int), PixelFormat.Format32bppRgb, rect.DataPointer)) {
				Platform.WriteBmp(bmp, output);
			}
			Surface.UnlockRectangle(temp);
			
			Delete(ref backbuffer);
			Delete(ref temp);
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