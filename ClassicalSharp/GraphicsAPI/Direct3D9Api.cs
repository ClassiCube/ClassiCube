// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
#if USE_DX && !ANDROID
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Threading;
using OpenTK;
using SharpDX;
using SharpDX.Direct3D9;
using D3D = SharpDX.Direct3D9;
using WinWindowInfo = OpenTK.Platform.Windows.WinWindowInfo;

namespace ClassicalSharp.GraphicsAPI {

	/// <summary> Implements IGraphicsAPI using Direct3D 9. </summary>
	public class Direct3D9Api : IGraphicsApi {

		Device device;
		Direct3D d3d;
		Capabilities caps;
		const int texBufferSize = 512, iBufferSize = 32, vBufferSize = 2048;
		
		D3D.Texture[] textures = new D3D.Texture[texBufferSize];
		DataBuffer[] vBuffers = new DataBuffer[vBufferSize];
		DynamicDataBuffer[] dynamicvBuffers = new DynamicDataBuffer[iBufferSize];
		DataBuffer[] iBuffers = new DataBuffer[iBufferSize];
		
		MatrixStack viewStack, projStack, texStack, curStack;
		PrimitiveType[] modeMappings;
		Format[] depthFormats, viewFormats;
		Format depthFormat, viewFormat;
		CreateFlags createFlags = CreateFlags.HardwareVertexProcessing;

		public Direct3D9Api( Game game ) {
			MinZNear = 0.05f;
			IntPtr windowHandle = ((WinWindowInfo)game.window.WindowInfo).WindowHandle;
			d3d = new Direct3D();
			int adapter = d3d.Adapters[0].Adapter;
			InitFields();
			FindCompatibleFormat( adapter );
			
			PresentParameters args = GetPresentArgs( 640, 480 );
			try {
				device = d3d.CreateDevice( adapter, DeviceType.Hardware, windowHandle, createFlags, args );
			} catch( SharpDXException ) {
				createFlags = CreateFlags.MixedVertexProcessing;
				try {
					device = d3d.CreateDevice( adapter, DeviceType.Hardware, windowHandle, createFlags, args );
				} catch ( SharpDXException ) {
					createFlags = CreateFlags.SoftwareVertexProcessing;
					device = d3d.CreateDevice( adapter, DeviceType.Hardware, windowHandle, createFlags, args );
				}
			}
			
			caps = device.Capabilities;
			viewStack = new MatrixStack( 32, device, TransformState.View );
			projStack = new MatrixStack( 4, device, TransformState.Projection );
			texStack = new MatrixStack( 4, device, TransformState.Texture0 );
			SetDefaultRenderStates();
			InitDynamicBuffers();
		}
		
		void FindCompatibleFormat( int adapter ) {
			for( int i = 0; i < viewFormats.Length; i++ ) {
				viewFormat = viewFormats[i];
				if( d3d.CheckDeviceType( adapter, DeviceType.Hardware, viewFormat, viewFormat, true ) ) break;
				
				if( i == viewFormats.Length - 1 )
					throw new InvalidOperationException( "Unable to create a back buffer with sufficient precision." );
			}
			
			for( int i = 0; i < depthFormats.Length; i++ ) {
				depthFormat = depthFormats[i];
				if( d3d.CheckDepthStencilMatch( adapter, DeviceType.Hardware, viewFormat, viewFormat, depthFormat ) ) break;
				
				if( i == depthFormats.Length - 1 )
					throw new InvalidOperationException( "Unable to create a depth buffer with sufficient precision." );
			}
		}
		
		bool alphaTest, alphaBlend;
		public override bool AlphaTest {
			set { if( value == alphaTest ) return;
				alphaTest = value; device.SetRenderState( RenderState.AlphaTestEnable, value );
			}
		}

		public override bool AlphaBlending {
			set { if( value == alphaBlend ) return;
				alphaBlend = value; device.SetRenderState( RenderState.AlphaBlendEnable, value );
			}
		}

		Compare[] compareFuncs;
		Compare alphaTestFunc;
		int alphaTestRef;
		public override void AlphaTestFunc( CompareFunc func, float value ) {
			alphaTestFunc = compareFuncs[(int)func];
			device.SetRenderState( RenderState.AlphaFunc, (int)alphaTestFunc );
			alphaTestRef = (int)( value * 255 );
			device.SetRenderState( RenderState.AlphaRef, alphaTestRef );
		}

		Blend[] blendFuncs;
		Blend srcBlendFunc, dstBlendFunc;
		public override void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc dstFunc ) {
			srcBlendFunc = blendFuncs[(int)srcFunc];
			dstBlendFunc = blendFuncs[(int)dstFunc];
			device.SetRenderState( RenderState.SourceBlend, (int)srcBlendFunc );
			device.SetRenderState( RenderState.DestinationBlend, (int)dstBlendFunc );
		}

		bool fogEnable;
		public override bool Fog {
			set { if( value == fogEnable ) return;
				fogEnable = value; device.SetRenderState( RenderState.FogEnable, value );
			}
		}

		int fogCol;
		public override void SetFogColour( FastColour col ) {
			fogCol = col.ToArgb();
			device.SetRenderState( RenderState.FogColor, fogCol );
		}

		float fogDensity = -1, fogStart = -1, fogEnd = -1;
		public override void SetFogDensity( float value ) {
			if( value == fogDensity ) return;
			fogDensity = value;
			device.SetRenderState( RenderState.FogDensity, value );
		}
		
		public override void SetFogStart( float value ) {
			if( value == fogStart ) return;
			fogStart = value;
			device.SetRenderState( RenderState.FogStart, value );
		}

		public override void SetFogEnd( float value ) {
			if( value == fogEnd ) return;
			fogEnd = value;
			device.SetRenderState( RenderState.FogEnd, value );
		}

		FogMode[] modes;
		FogMode fogTableMode;
		public override void SetFogMode( Fog fogMode ) {
			FogMode newMode = modes[(int)fogMode];
			if( newMode == fogTableMode ) return;
			fogTableMode = newMode;
			device.SetRenderState( RenderState.FogTableMode, (int)fogTableMode );
		}
		
		public override bool FaceCulling {
			set {
				Cull mode = value ? Cull.Clockwise : Cull.None;
				device.SetRenderState( RenderState.CullMode, (int)mode );
			}
		}

		public override int MaxTextureDimensions {
			get { return Math.Min( caps.MaxTextureHeight, caps.MaxTextureWidth ); }
		}
		
		public override bool Texturing {
			set { if( !value ) device.SetTexture( 0, null ); }
		}

		public override int CreateTexture( int width, int height, IntPtr scan0 ) {
			D3D.Texture texture = device.CreateTexture( width, height, 0, Usage.None, Format.A8R8G8B8, Pool.Managed );
			texture.SetData( 0, LockFlags.None, scan0, width * height * 4 );
			return GetOrExpand( ref textures, texture, texBufferSize );
		}
		
		public override void UpdateTexturePart( int texId, int texX, int texY, FastBitmap part ) {
			D3D.Texture texture = textures[texId];
			texture.SetPartData( 0, LockFlags.None, part.Scan0, texX, texY, part.Width, part.Height );
		}

		public override void BindTexture( int texId ) {
			device.SetTexture( 0, textures[texId] );
		}

		public override void DeleteTexture( ref int texId ) {
			Delete( textures, texId );
			texId = -1;
		}

		int lastClearCol;
		public override void Clear() {
			device.Clear( ClearFlags.Target | ClearFlags.ZBuffer, lastClearCol, 1f, 0 );
		}

		public override void ClearColour( FastColour col ) {
			lastClearCol = col.ToArgb();
		}

		public override bool ColourWrite {
			set { device.SetRenderState( RenderState.ColorWriteEnable, value ? 0xF : 0x0 ); }
		}

		Compare depthTestFunc;
		public override void DepthTestFunc( CompareFunc func ) {
			depthTestFunc = compareFuncs[(int)func];
			device.SetRenderState( RenderState.ZFunc, (int)depthTestFunc );
		}

		bool depthTest, depthWrite;
		public override bool DepthTest {
			set { depthTest = value; device.SetRenderState( RenderState.ZEnable, value ); }
		}

		public override bool DepthWrite {
			set { depthWrite = value; device.SetRenderState( RenderState.ZWriteEnable, value ); }
		}
		
		public override bool AlphaArgBlend {
			set {
				TextureOp op = value ? TextureOp.Modulate : TextureOp.SelectArg1;
				device.SetTextureStageState( 0, TextureStage.AlphaOperation, (int)op ); 
			}
		}
		
		#region Vertex buffers
		
		public override int CreateDynamicVb( VertexFormat format, int maxVertices ) {
			int size = maxVertices * strideSizes[(int)format];
			DynamicDataBuffer buffer = device.CreateDynamicVertexBuffer( size, formatMapping[(int)format] );
			
			buffer.Format = formatMapping[(int)format];
			buffer.MaxSize = size;
			return GetOrExpand( ref dynamicvBuffers, buffer, iBufferSize );
		}
		
		public override void UpdateDynamicVb<T>( DrawMode mode, int vb, T[] vertices, int count ) {
			int size = count * batchStride;
			DataBuffer buffer = dynamicvBuffers[vb];
			buffer.SetData( vertices, size, LockFlags.Discard );
			
			device.SetStreamSource( 0, buffer, 0, batchStride );
			device.DrawPrimitives( modeMappings[(int)mode], 0, NumPrimitives( count, mode ) );
		}
		
		public override void UpdateDynamicIndexedVb<T>( DrawMode mode, int vb, T[] vertices, int vCount, int indicesCount ) {
			int size = vCount * batchStride;
			DataBuffer buffer = dynamicvBuffers[vb];
			buffer.SetData( vertices, size, LockFlags.Discard );
			
			device.SetStreamSource( 0, buffer, 0, batchStride );
			device.DrawIndexedPrimitives( modeMappings[(int)mode], 0, 0, indicesCount / 6 * 4, 0, NumPrimitives( indicesCount, mode ) );
		}
		
		public override void SetDynamicVbData<T>( DrawMode mode, int vb, T[] vertices, int count ) {
			int size = count * batchStride;
			DataBuffer buffer = dynamicvBuffers[vb];
			buffer.SetData( vertices, size, LockFlags.Discard );
			device.SetStreamSource( 0, buffer, 0, batchStride );
		}
		
		public override void DeleteDynamicVb( int id ) {
			Delete( dynamicvBuffers, id );
		}

		D3D.VertexFormat[] formatMapping;
		public override int CreateVb<T>( T[] vertices, VertexFormat format, int count ) {
			int size = count * strideSizes[(int)format];
			DataBuffer buffer = device.CreateVertexBuffer( size, Usage.None, formatMapping[(int)format], Pool.Managed );
			buffer.SetData( vertices, size, LockFlags.None );
			return GetOrExpand( ref vBuffers, buffer, vBufferSize );
		}
		
		public override int CreateVb( IntPtr vertices, VertexFormat format, int count ) {
			int size = count * strideSizes[(int)format];
			DataBuffer buffer = device.CreateVertexBuffer( size, Usage.None, formatMapping[(int)format], Pool.Managed );
			buffer.SetData( vertices, size, LockFlags.None );
			return GetOrExpand( ref vBuffers, buffer, vBufferSize );
		}
		
		public unsafe override int CreateIb( ushort[] indices, int indicesCount ) {
			int size = indicesCount * sizeof( ushort );
			DataBuffer buffer = device.CreateIndexBuffer( size, Usage.None, Format.Index16, Pool.Managed );
			buffer.SetData( indices, size, LockFlags.None );
			return GetOrExpand( ref iBuffers, buffer, iBufferSize );
		}
		
		public override int CreateIb( IntPtr indices, int indicesCount ) {
			int size = indicesCount * sizeof( ushort );
			DataBuffer buffer = device.CreateIndexBuffer( size, Usage.None, Format.Index16, Pool.Managed );
			buffer.SetData( indices, size, LockFlags.None );
			return GetOrExpand( ref iBuffers, buffer, iBufferSize );
		}

		public override void DeleteVb( int vb ) {
			Delete( vBuffers, vb );
		}
		
		public override void DeleteIb( int ib ) {
			Delete( iBuffers, ib );
		}

		int batchStride;
		public override void SetBatchFormat( VertexFormat format ) {
			device.SetVertexFormat( formatMapping[(int)format] );
			batchStride = strideSizes[(int)format];
		}
		
		public override void BindVb( int vb ) {
			device.SetStreamSource( 0, vBuffers[vb], 0, batchStride );
		}
		
		public override void BindIb( int ib ) {
			device.SetIndices( iBuffers[ib] );
		}

		public override void DrawVb( DrawMode mode, int startVertex, int verticesCount ) {
			device.DrawPrimitives( modeMappings[(int)mode], startVertex, NumPrimitives( verticesCount, mode ) );
		}

		public override void DrawIndexedVb( DrawMode mode, int indicesCount, int startIndex ) {
			device.DrawIndexedPrimitives( modeMappings[(int)mode], 0, startIndex / 6 * 4,
			                             indicesCount / 6 * 4, startIndex, NumPrimitives( indicesCount, mode ) );
		}
		
		internal override void DrawIndexedVb_TrisT2fC4b( int indicesCount, int startIndex ) {
			device.DrawIndexedPrimitives( PrimitiveType.TriangleList, 0, startIndex / 6 * 4,
			                             indicesCount / 6 * 4, startIndex, indicesCount / 3 );
		}
		
		internal override void DrawIndexedVb_TrisT2fC4b( int indicesCount, int startVertex, int startIndex ) {
			device.DrawIndexedPrimitives( PrimitiveType.TriangleList, startVertex, 0,
			                             indicesCount / 6 * 4, startIndex, indicesCount / 3 );
		}
		#endregion


		#region Matrix manipulation

		public override void SetMatrixMode( MatrixType mode ) {
			if( mode == MatrixType.Modelview ) {
				curStack = viewStack;
			} else if( mode == MatrixType.Projection ) {
				curStack = projStack;
			} else if( mode == MatrixType.Texture ) {
				curStack = texStack;
			}
		}

		public unsafe override void LoadMatrix( ref Matrix4 matrix ) {
			if( curStack == texStack ) {
				matrix.M31 = matrix.M41; // NOTE: this hack fixes the texture movements.
				device.SetTextureStageState( 0, TextureStage.TextureTransformFlags, (int)TextureTransform.Count2 );
			}
			curStack.SetTop( ref matrix );
		}

		Matrix4 identity = Matrix4.Identity;
		public override void LoadIdentityMatrix() {
			if( curStack == texStack ) {
				device.SetTextureStageState( 0, TextureStage.TextureTransformFlags, (int)TextureTransform.Disable );
			}
			curStack.SetTop( ref identity );
		}

		public override void PushMatrix() {
			curStack.Push();
		}

		public override void PopMatrix() {
			curStack.Pop();
		}

		public unsafe override void MultiplyMatrix( ref Matrix4 matrix ) {
			curStack.MultiplyTop( ref matrix );
		}

		class MatrixStack
		{
			Matrix4[] stack;
			int stackIndex;
			Device device;
			TransformState matrixType;

			public MatrixStack( int capacity, Device device, TransformState matrixType ) {
				stack = new Matrix4[capacity];
				stack[0] = Matrix4.Identity;
				this.device = device;
				this.matrixType = matrixType;
			}

			public void Push() {
				stack[stackIndex + 1] = stack[stackIndex]; // mimic GL behaviour
				stackIndex++; // exact same, we don't need to update DirectX state.
			}

			public void SetTop( ref Matrix4 matrix ) {
				stack[stackIndex] = matrix;
				device.SetTransform( matrixType, ref stack[stackIndex] );
			}

			public void MultiplyTop( ref Matrix4 matrix ) {
				stack[stackIndex] = matrix * stack[stackIndex];
				device.SetTransform( matrixType, ref stack[stackIndex] );
			}

			public void Pop() {
				stackIndex--;
				device.SetTransform( matrixType, ref stack[stackIndex] );
			}
		}

		#endregion
		
		public override void BeginFrame( Game game ) {
			device.BeginScene();
		}
		
		public override void EndFrame( Game game ) {
			device.EndScene();
			int code = device.Present();
			if( code >= 0 ) return;
			
			if( (uint)code != (uint)Direct3DError.DeviceLost )
				throw new SharpDXException( code );
			
			// TODO: Make sure this actually works on all graphics cards.
			Utils.LogDebug( "Lost Direct3D device." );
			LoopUntilRetrieved();
			RecreateDevice( game );
		}
		
		void LoopUntilRetrieved() {
			while( true ) {
				Thread.Sleep( 50 );
				uint code = (uint)device.TestCooperativeLevel();
				if( (uint)code == (uint)Direct3DError.DeviceNotReset ) {
					Utils.LogDebug( "Retrieved Direct3D device again." );
					return;
				}
				LostContextFunction( 1 / 20.0 );
			}
		}
		
		bool vsync = false;
		public override void SetVSync( Game game, bool value ) {
			vsync = value;
			game.VSync = value;
			RecreateDevice( game );
		}
		
		public override void OnWindowResize( Game game ) {
			RecreateDevice( game );
		}
		
		void RecreateDevice( Game game ) {
			PresentParameters args = GetPresentArgs( game.Width, game.Height );
			for( int i = 0; i < dynamicvBuffers.Length; i++ ) {
				DynamicDataBuffer buffer = dynamicvBuffers[i];
				if( buffer != null ) {
					buffer.Dispose();
				}
			}
			
			while( (uint)device.Reset( args ) == (uint)Direct3DError.DeviceLost )
				LoopUntilRetrieved();
			
			SetDefaultRenderStates();
			RestoreRenderStates();
			for( int i = 0; i < dynamicvBuffers.Length; i++ ) {
				DynamicDataBuffer buffer = dynamicvBuffers[i];
				if( buffer != null ) {
					dynamicvBuffers[i] = device.CreateDynamicVertexBuffer( buffer.MaxSize, buffer.Format );
					dynamicvBuffers[i].Format = buffer.Format;
					dynamicvBuffers[i].MaxSize = buffer.MaxSize;
					buffer = dynamicvBuffers[i];
				}
			}
		}
		
		void SetDefaultRenderStates() {
			device.SetRenderState( RenderState.FillMode, (int)FillMode.Solid );
			FaceCulling = false;
			device.SetRenderState( RenderState.ColorVertex, false );
			device.SetRenderState( RenderState.Lighting, false );
			device.SetRenderState( RenderState.SpecularEnable, false );
			device.SetRenderState( RenderState.LocalViewer, false );
			device.SetRenderState( RenderState.DebugMonitorToken, false );
		}
		
		void RestoreRenderStates() {
			device.SetRenderState( RenderState.AlphaTestEnable, alphaTest );
			device.SetRenderState( RenderState.AlphaBlendEnable, alphaBlend );
			device.SetRenderState( RenderState.AlphaFunc, (int)alphaTestFunc );
			device.SetRenderState( RenderState.AlphaRef, alphaTestRef );
			device.SetRenderState( RenderState.SourceBlend, (int)srcBlendFunc );
			device.SetRenderState( RenderState.DestinationBlend, (int)dstBlendFunc );
			device.SetRenderState( RenderState.FogEnable, fogEnable );
			device.SetRenderState( RenderState.FogColor, fogCol );
			device.SetRenderState( RenderState.FogDensity, fogDensity );
			device.SetRenderState( RenderState.FogStart, fogStart );
			device.SetRenderState( RenderState.FogEnd, fogEnd );
			device.SetRenderState( RenderState.FogTableMode, (int)fogTableMode );
			device.SetRenderState( RenderState.ZFunc, (int)depthTestFunc );
			device.SetRenderState( RenderState.ZEnable, depthTest );
			device.SetRenderState( RenderState.ZWriteEnable, depthWrite );
		}
		
		PresentParameters GetPresentArgs( int width, int height ) {
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

		static int GetOrExpand<T>( ref T[] array, T value, int expSize ) {
			// Find first free slot
			for( int i = 1; i < array.Length; i++ ) {
				if( array[i] == null ) {
					array[i] = value;
					return i;
				}
			}
			// Otherwise resize and add more elements
			int oldLength = array.Length;
			Array.Resize( ref array, array.Length + expSize );
			array[oldLength] = value;
			return oldLength;
		}
		
		static void Delete<T>( T[] array, int id ) where T : class, IDisposable {
			if( id <= 0 || id >= array.Length ) return;
			
			T value = array[id];
			if( value != null ) {
				value.Dispose();
			}
			array[id] = null;
		}
		
		static int NumPrimitives( int vertices, DrawMode mode ) {
			return mode == DrawMode.Triangles ? vertices / 3 : vertices / 2;
		}
		
		protected unsafe override void LoadOrthoMatrix( float width, float height ) {
			Matrix4 matrix = Matrix4.CreateOrthographicOffCenter( 0, width, height, 0, -10000, 10000 );
			const float zN = -10000, zF = 10000;
			matrix.M33 = 1 / (zN - zF);
			matrix.M43 = zN / (zN - zF);
			matrix.M44 = 1;
			curStack.SetTop( ref matrix );
		}
		
		public override void Dispose() {
			base.Dispose();
			device.Dispose();
			d3d.Dispose();
		}

		protected override void MakeApiInfo() {
			string adapter =  d3d.Adapters[0].Details.Description;
			float texMem = (uint)device.AvailableTextureMemory / 1024f / 1024f;
			
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

		public override void TakeScreenshot( string output, int width, int height ) {
			using( Surface backbuffer = device.GetBackBuffer( 0, 0, BackBufferType.Mono ),
			      tempSurface = device.CreateOffscreenPlainSurface( width, height, Format.X8R8G8B8, Pool.SystemMemory ) ) {
				// For DX 8 use IDirect3DDevice8::CreateImageSurface
				device.GetRenderTargetData( backbuffer, tempSurface );
				LockedRectangle rect = tempSurface.LockRectangle( LockFlags.ReadOnly | LockFlags.NoDirtyUpdate );
				
				using( Bitmap bmp = new Bitmap( width, height, width * sizeof(int),
				                               PixelFormat.Format32bppRgb, rect.DataPointer ) ) {
					bmp.Save( output, ImageFormat.Png );
				}
				tempSurface.UnlockRectangle();
			}
		}
		
		void InitFields() {
			// See comment in Game() constructor for why this is necessary.
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