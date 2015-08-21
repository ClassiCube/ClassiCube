#if USE_DX
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Threading;
using SharpDX;
using SharpDX.Direct3D9;
using D3D = SharpDX.Direct3D9;
using Matrix4 = OpenTK.Matrix4;
using WinWindowInfo = OpenTK.Platform.Windows.WinWindowInfo;

namespace ClassicalSharp.GraphicsAPI {

	// TODO: Should we use a native form wrapper instead of wrapping over OpenTK?
	public class Direct3D9Api : IGraphicsApi {

		Device device;
		Direct3D d3d;
		Capabilities caps;
		const int texBufferSize = 512, iBufferSize = 256, vBufferSize = 2048;
		
		D3D.Texture[] textures = new D3D.Texture[texBufferSize];
		DataBuffer[] vBuffers = new DataBuffer[vBufferSize];
		DataBuffer[] iBuffers = new DataBuffer[iBufferSize];
		MatrixStack viewStack, projStack, texStack;
		MatrixStack curStack;
		PrimitiveType[] modeMappings = {
			PrimitiveType.TriangleList, PrimitiveType.LineList,
			PrimitiveType.TriangleStrip,
		};
		static Format[] depthFormats = { Format.D32, Format.D24X8, Format.D24S8, Format.D24X4S4, Format.D16, Format.D15S1 };
		Format depthFormat, viewFormat = Format.X8R8G8B8;
		CreateFlags createFlags = CreateFlags.HardwareVertexProcessing;

		public Direct3D9Api( Game game ) {
			IntPtr windowHandle = ((WinWindowInfo)game.WindowInfo).WindowHandle;
			d3d = new Direct3D();
			int adapter = d3d.Adapters[0].Adapter;
			
			for( int i = 0; i < depthFormats.Length; i++ ) {
				depthFormat = depthFormats[i];
				if( d3d.CheckDepthStencilMatch( adapter, DeviceType.Hardware, viewFormat, viewFormat, depthFormat ) ) break;
				
				if( i == depthFormats.Length - 1 )
					throw new InvalidOperationException( "Unable to create a depth buffer with sufficient precision." );
			}
			
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
			memcpy64Bit = IntPtr.Size == 8;
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

		Compare[] compareFuncs = {
			Compare.Always, Compare.NotEqual, Compare.Never, Compare.Less,
			Compare.LessEqual, Compare.Equal, Compare.GreaterEqual, Compare.Greater,
		};
		Compare alphaTestFunc;
		int alphaTestRef;
		public override void AlphaTestFunc( CompareFunc func, float value ) {
			alphaTestFunc = compareFuncs[(int)func];
			device.SetRenderState( RenderState.AlphaFunc, (int)alphaTestFunc );
			alphaTestRef = (int)( value * 255 );
			device.SetRenderState( RenderState.AlphaRef, alphaTestRef );
		}

		Blend[] blendFuncs = {
			Blend.Zero, Blend.One,
			Blend.SourceAlpha, Blend.InverseSourceAlpha,
			Blend.DestinationAlpha, Blend.InverseDestinationAlpha,
		};
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

		FogMode[] modes = { FogMode.Linear, FogMode.Exponential, FogMode.ExponentialSquared };
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
			texture.SetData( scan0, width * height * 4, 0, LockFlags.None );
			return GetOrExpand( ref textures, texture, texBufferSize );
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
		
		public override int CreateDynamicVb( VertexFormat format, int maxVertices ) {
			return -1;
		}
		
		public override void DrawDynamicVb<T>( DrawMode mode, int vb, T[] vertices, VertexFormat format, int count ) {
			device.SetVertexFormat( formatMapping[(int)format] );
			device.DrawUserPrimitives( modeMappings[(int)mode], 0, NumPrimitives( count, mode ), vertices );
		}
		
		public override void DeleteDynamicVb( int id ) {
		}

		#region Vertex buffers

		D3D.VertexFormat[] formatMapping = {
			D3D.VertexFormat.Position | D3D.VertexFormat.Texture2,
			D3D.VertexFormat.Position | D3D.VertexFormat.Diffuse,
			D3D.VertexFormat.Position | D3D.VertexFormat.Texture2 | D3D.VertexFormat.Diffuse,
		};

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
		public override void BeginVbBatch( VertexFormat format ) {
			device.SetVertexFormat( formatMapping[(int)format] );
			batchStride = strideSizes[(int)format];
		}

		public override void DrawVb( DrawMode mode, int startVertex, int verticesCount ) {
			device.DrawPrimitives( modeMappings[(int)mode], startVertex, NumPrimitives( verticesCount, mode ) );
		}
		
		public override void BindVb( int vb ) {
			device.SetStreamSource( 0, vBuffers[vb], 0, batchStride );
		}
		
		public override void BindIb( int ib ) {
			device.SetIndices( iBuffers[ib] );
		}

		public override void DrawIndexedVb( DrawMode mode, int indicesCount, int startIndex ) {
			device.DrawIndexedPrimitives( modeMappings[(int)mode], 0, 0,
			                             indicesCount / 6 * 4, startIndex, NumPrimitives( indicesCount, mode ) );
		}
		
		public override void DrawIndexedVb_T2fC4b( DrawMode mode, int indicesCount, int startVertex, int startIndex ) {
			device.DrawIndexedPrimitives( modeMappings[(int)mode], startVertex, 0,
			                             indicesCount / 6 * 4, startIndex, NumPrimitives( indicesCount, mode ) );
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
			while( true ) {
				Thread.Sleep( 50 );
				code = device.TestCooperativeLevel();
				if( (uint)code == (uint)Direct3DError.DeviceNotReset ) {
					Utils.Log( "Retrieved Direct3D device again." );
					RecreateDevice( game );
					break;
				}
				game.Network.Tick( 1 / 20.0 );
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
			device.Reset( args );
			SetDefaultRenderStates();
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
		
		void SetDefaultRenderStates() {
			device.SetRenderState( RenderState.FillMode, (int)FillMode.Solid );
			FaceCulling = false;
			device.SetRenderState( RenderState.ColorVertex, false );
			device.SetRenderState( RenderState.Lighting, false );
			device.SetRenderState( RenderState.SpecularEnable, false );
			device.SetRenderState( RenderState.LocalViewer, false );
			device.SetRenderState( RenderState.DebugMonitorToken, false );
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
			if( mode == DrawMode.Triangles ) {
				return vertices / 3;
			} else if( mode == DrawMode.TriangleStrip ) {
				return vertices - 2;
			}
			return vertices / 2;
		}
		
		protected unsafe override void LoadOrthoMatrix( float width, float height ) {
			Matrix4 matrix = Matrix4.CreateOrthographicOffCenter( 0, width, height, 0, 0, 1 );
			matrix.M33 = -1;
			matrix.M43 = 0;
			curStack.SetTop( ref matrix );
		}
		
		public override void Dispose() {
			base.Dispose();
			device.Dispose();
			d3d.Dispose();
		}

		public override void PrintApiSpecificInfo() {
			Utils.Log( "D3D tex memory available: " + (uint)device.AvailableTextureMemory );
			Utils.Log( "D3D vertex processing: " + createFlags );
			Utils.Log( "D3D depth buffer format: " + depthFormat );
			Utils.Log( "D3D device caps: " + caps.DeviceCaps );
		}

		public unsafe override void TakeScreenshot( string output, Size size ) {
			using( Surface backbuffer = device.GetBackBuffer( 0, 0, BackBufferType.Mono ),
			      tempSurface = device.CreateOffscreenPlainSurface( size.Width, size.Height, Format.X8R8G8B8, Pool.SystemMemory ) ) {
				// For DX 8 use IDirect3DDevice8::CreateImageSurface
				device.GetRenderTargetData( backbuffer, tempSurface );
				LockedRectangle rect = tempSurface.LockRectangle( LockFlags.ReadOnly | LockFlags.NoDirtyUpdate );
				
				using( Bitmap bmp = new Bitmap( size.Width, size.Height, size.Width * 4,
				                               PixelFormat.Format32bppRgb, rect.DataPointer ) ) {
					bmp.Save( output, ImageFormat.Png );
				}
				tempSurface.UnlockRectangle();
			}
		}
	}
}
#endif