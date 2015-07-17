// necessary dll references: (Managed DirectX)
// Microsoft.DirectX
// Microsoft.DirectX.Direct3D
// Microsoft.DirectX.Direct3DX
#if USE_DX
using System;
using System.Drawing;
using System.Runtime.InteropServices;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using D3D = Microsoft.DirectX.Direct3D;
using Matrix4 = OpenTK.Matrix4;
using System.Reflection;

namespace ClassicalSharp.GraphicsAPI {

	// TODO: Should we use a native form wrapper instead of wrapping over OpenTK?
	public class DirectXApi : IGraphicsApi {

		public Device device;
		Caps caps;
		const int texBufferSize = 512;
		const int iBufferSize = 1024;
		const int vBufferSize = 2048;
		
		D3D.Texture[] textures = new D3D.Texture[texBufferSize];
		VertexBuffer[] vBuffers = new VertexBuffer[vBufferSize];
		IndexBuffer[] iBuffers = new IndexBuffer[iBufferSize];
		MatrixStack viewStack, projStack, texStack;
		MatrixStack curStack;
		PrimitiveType[] modeMappings = {
			PrimitiveType.TriangleList, PrimitiveType.LineList,
			PrimitiveType.TriangleStrip,
		};

		public DirectXApi( Game game ) {
			PresentParameters args = GetPresentArgs( 640, 480 );
			// Code to get window handle from WinWindowInfo class
			OpenTK.Platform.IWindowInfo info = game.WindowInfo;
			Type type = info.GetType();
			FieldInfo getWindowHandle = type.GetField( "handle", BindingFlags.NonPublic | BindingFlags.Instance );
			IntPtr windowHandle = (IntPtr)getWindowHandle.GetValue( info );
			
			int adapter = Manager.Adapters.Default.Adapter;
			CreateFlags flags = CreateFlags.HardwareVertexProcessing;
			device = new Device( adapter, DeviceType.Hardware, windowHandle, flags, args );
			
			caps = device.DeviceCaps;
			viewStack = new MatrixStack( 32, device, TransformType.View );
			projStack = new MatrixStack( 4, device, TransformType.Projection );
			texStack = new MatrixStack( 4, device, TransformType.Texture0 );
			SetDefaultRenderStates();
		}
		
		bool alphaTest, alphaBlend;
		public override bool AlphaTest {
			set { alphaTest = value; device.SetRenderState( RenderStates.AlphaTestEnable, value ); }
		}

		public override bool AlphaBlending {
			set { alphaBlend = value; device.SetRenderState( RenderStates.AlphaBlendEnable, value ); }
		}

		Compare[] compareFuncs = {
			Compare.Always, Compare.NotEqual, Compare.Never, Compare.Less,
			Compare.LessEqual, Compare.Equal, Compare.GreaterEqual, Compare.Greater,
		};
		Compare alphaTestFunc;
		int alphaTestRef;
		public override void AlphaTestFunc( CompareFunc func, float value ) {
			alphaTestFunc = compareFuncs[(int)func];
			device.SetRenderState( RenderStates.AlphaFunction, (int)alphaTestFunc );
			alphaTestRef = (int)( value * 255 );
			device.SetRenderState( RenderStates.ReferenceAlpha, alphaTestRef );
		}

		Blend[] blendFuncs = {
			Blend.Zero, Blend.One,
			Blend.SourceAlpha, Blend.InvSourceAlpha,
			Blend.DestinationAlpha, Blend.InvDestinationAlpha,
		};
		Blend srcFunc, dstFunc;
		public override void AlphaBlendFunc( BlendFunc srcBlendFunc, BlendFunc dstBlendFunc ) {
			srcFunc = blendFuncs[(int)srcBlendFunc];
			dstFunc = blendFuncs[(int)dstBlendFunc];
			device.SetRenderState( RenderStates.SourceBlend, (int)srcFunc );
			device.SetRenderState( RenderStates.DestinationBlend, (int)dstFunc );
		}

		bool fogEnable;
		public override bool Fog {
			set { fogEnable = value; device.SetRenderState( RenderStates.FogEnable, value ); }
		}

		int fogCol;
		public override void SetFogColour( FastColour col ) {
			fogCol = col.ToArgb();
			device.SetRenderState( RenderStates.FogColor, fogCol );
		}

		float fogDensity, fogStart, fogEnd;
		public override void SetFogDensity( float value ) {
			fogDensity = value;
			device.SetRenderState( RenderStates.FogDensity, value );
		}
		
		public override void SetFogStart( float value ) {
			fogStart = value;
			device.SetRenderState( RenderStates.FogStart, value );
		}

		public override void SetFogEnd( float value ) {
			fogEnd = value;
			device.SetRenderState( RenderStates.FogEnd, value );
		}

		FogMode[] modes = { FogMode.Linear, FogMode.Exp, FogMode.Exp2 };
		FogMode fogMode;
		public override void SetFogMode( Fog mode ) {
			fogMode = modes[(int)mode];
			device.SetRenderState( RenderStates.FogTableMode, (int)fogMode );
		}
		
		public override bool FaceCulling {
			set {
				Cull mode = value ? Cull.Clockwise : Cull.None;
				device.SetRenderState( RenderStates.CullMode, (int)mode );
			}
		}

		public override int MaxTextureDimensions {
			get { return Math.Min( caps.MaxTextureHeight, caps.MaxTextureWidth ); }
		}

		public override int LoadTexture( int width, int height, IntPtr scan0 ) {
			D3D.Texture texture = new D3D.Texture( device, width, height, 0, Usage.None, Format.A8R8G8B8, Pool.Managed );
			GraphicsStream vbData = texture.LockRectangle( 0, LockFlags.None );
			IntPtr dest = vbData.InternalData;
			memcpy( scan0, dest, width * height * 4 );
			texture.UnlockRectangle( 0 );
			return GetOrExpand( ref textures, texture, texBufferSize );
		}

		public override void Bind2DTexture( int texId ) {
			device.SetTexture( 0, textures[texId] );
		}

		public override bool Texturing {
			set {
				if( !value ) device.SetTexture( 0, null );
			}
		}

		public override void DeleteTexture( ref int texId ) {
			Delete( textures, texId );
			texId = -1;
		}
		
		public override bool IsValidTexture( int texId ) {
			return texId < textures.Length && textures[texId] != null;
		}

		int lastClearCol = 0;
		public override void Clear() {
			device.Clear( ClearFlags.Target | ClearFlags.ZBuffer, lastClearCol, 1f, 0 );
		}

		public override void ClearColour( FastColour col ) {
			lastClearCol = col.ToArgb();
		}

		public override bool ColourWrite {
			set { device.SetRenderState( RenderStates.ColorWriteEnable, value ? 0xF : 0x0 ); }
		}

		Compare depthTestFunc;
		public override void DepthTestFunc( CompareFunc func ) {
			depthTestFunc = compareFuncs[(int)func];
			device.SetRenderState( RenderStates.ZBufferFunction, (int)depthTestFunc );
		}

		bool depthTest, depthWrite;
		public override bool DepthTest {
			set { depthTest = value; device.SetRenderState( RenderStates.ZEnable, value ); }
		}

		public override bool DepthWrite {
			set { depthWrite = value; device.SetRenderState( RenderStates.ZBufferWriteEnable, value ); }
		}
		
		public override int CreateDynamicVb( VertexFormat format, int maxVertices ) {
			return -1;
		}
		
		public override void DrawDynamicVb<T>( DrawMode mode, int vb, T[] vertices, VertexFormat format, int count ) {
			device.VertexFormat = formatMapping[(int)format];
			device.DrawUserPrimitives( modeMappings[(int)mode], NumPrimitives( count, mode ), vertices );
		}
		
		public override void DeleteDynamicVb( int id ) {
		}

		#region Vertex buffers

		VertexFormats[] formatMapping = {
			VertexFormats.Position | VertexFormats.Texture1,
			VertexFormats.Position | VertexFormats.Diffuse,
			VertexFormats.Position | VertexFormats.Texture1 | VertexFormats.Diffuse,
		};

		public override int InitVb<T>( T[] vertices, VertexFormat format, int count ) {
			VertexBuffer buffer = CreateVb( vertices, count, format );
			return GetOrExpand( ref vBuffers, buffer, vBufferSize );
		}
		
		public override int InitIb( ushort[] indices, int count ) {
			IndexBuffer buffer = CreateIb( indices, count );
			return GetOrExpand( ref iBuffers, buffer, iBufferSize );
		}

		unsafe VertexBuffer CreateVb<T>( T[] vertices, int count, VertexFormat format ) {
			int sizeInBytes = count * strideSizes[(int)format];
			VertexFormats d3dFormat = formatMapping[(int)format];
			VertexBuffer buffer = new VertexBuffer( device, sizeInBytes, Usage.None, d3dFormat, Pool.Managed );
			
			GraphicsStream vbData = buffer.Lock( 0, sizeInBytes, LockFlags.None );
			GCHandle handle = GCHandle.Alloc( vertices, GCHandleType.Pinned );
			IntPtr source = handle.AddrOfPinnedObject();
			IntPtr dest = vbData.InternalData;
			memcpy( source, dest, sizeInBytes );
			buffer.Unlock();
			handle.Free();
			return buffer;
		}
		
		unsafe IndexBuffer CreateIb( ushort[] indices, int count ) {
			int sizeInBytes = count * 2;
			IndexBuffer buffer = new IndexBuffer( device, sizeInBytes, Usage.None, Pool.Managed, true );
			
			GraphicsStream vbData = buffer.Lock( 0, sizeInBytes, LockFlags.None );
			fixed( ushort* src = indices ) {
				memcpy( (IntPtr)src, vbData.InternalData, sizeInBytes );
			}
			buffer.Unlock();
			return buffer;
		}

		public override void DeleteVb( int vb ) {
			Delete( vBuffers, vb );
		}
		
		public override void DeleteIb( int ib ) {
			Delete( iBuffers, ib );
		}
		
		public override bool IsValidVb( int vb ) {
			return IsValid( vBuffers, vb );
		}
		
		public override bool IsValidIb( int ib ) {
			return IsValid( iBuffers, ib );
		}

		public override void DrawVb( DrawMode mode, VertexFormat format, int id, int startVertex, int verticesCount ) {
			device.SetStreamSource( 0, vBuffers[id], 0, strideSizes[(int)format] );
			device.VertexFormat = formatMapping[(int)format];
			device.DrawPrimitives( modeMappings[(int)mode], startVertex, NumPrimitives( verticesCount, mode ) );
		}

		int batchStride;
		public override void BeginVbBatch( VertexFormat format ) {
			device.VertexFormat = formatMapping[(int)format];
			batchStride = strideSizes[(int)format];
		}

		public override void DrawVbBatch( DrawMode mode, int id, int startVertex, int verticesCount ) {
			device.SetStreamSource( 0, vBuffers[id], 0, batchStride );
			device.DrawPrimitives( modeMappings[(int)mode], startVertex, NumPrimitives( verticesCount, mode ) );
		}
		
		public override void BeginIndexedVbBatch() {
			device.VertexFormat = formatMapping[(int)VertexFormat.Pos3fTex2fCol4b];
			batchStride = VertexPos3fTex2fCol4b.Size;
		}

		public override void DrawIndexedVbBatch( DrawMode mode, int vb, int ib, int indicesCount,
		                                        int startVertex, int startIndex ) {
			device.Indices = iBuffers[ib];
			device.SetStreamSource( 0, vBuffers[vb], 0, batchStride );
			device.DrawIndexedPrimitives( modeMappings[(int)mode], startVertex, startVertex,
			                             indicesCount / 6 * 4, startIndex, NumPrimitives( indicesCount, mode ) );
		}

		public override void EndIndexedVbBatch() {
			device.Indices = null;
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
			Matrix4 transposed = matrix;
			Matrix dxMatrix = *(Matrix*)&transposed;
			if( curStack == texStack ) {
				dxMatrix.M31 = dxMatrix.M41; // NOTE: this hack fixes the texture movements.
				device.SetTextureStageState( 0, TextureStageStates.TextureTransform, (int)TextureTransform.Count2 );
			}
			curStack.SetTop( ref dxMatrix );
		}

		Matrix identity = Matrix.Identity;
		public override void LoadIdentityMatrix() {
			if( curStack == texStack ) {
				device.SetTextureStageState( 0, TextureStageStates.TextureTransform, (int)TextureTransform.Disable );
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
			Matrix4 transposed = matrix;
			Matrix dxMatrix = *(Matrix*)&transposed;
			curStack.MultiplyTop( ref dxMatrix );
		}

		class MatrixStack
		{
			Matrix[] stack;
			int stackIndex;
			Device device;
			TransformType matrixType;

			public MatrixStack( int capacity, Device device, TransformType matrixType ) {
				stack = new Matrix[capacity];
				stack[0] = Matrix.Identity;
				this.device = device;
				this.matrixType = matrixType;
			}

			public void Push() {
				stack[stackIndex + 1] = stack[stackIndex]; // mimic GL behaviour
				stackIndex++; // exact same, we don't need to update DirectX state.
			}

			public void SetTop( ref Matrix matrix ) {
				stack[stackIndex] = matrix;
				device.SetTransform( matrixType, stack[stackIndex] );
			}

			public void MultiplyTop( ref Matrix matrix ) {
				stack[stackIndex] = matrix * stack[stackIndex];
				device.SetTransform( matrixType, stack[stackIndex] );
			}

			public Matrix GetTop() {
				return stack[stackIndex];
			}

			public void Pop() {
				stackIndex--;
				device.SetTransform( matrixType, stack[stackIndex] );
			}
		}

		#endregion
		
		public override void BeginFrame( Game game ) {
			device.BeginScene();
		}
		
		public override void EndFrame( Game game ) {
			device.EndScene();
			device.Present();
		}
		
		bool vsync = false;
		public override void SetVSync( Game game, bool value ) {
			vsync = value;
			game.VSync = value ? OpenTK.VSyncMode.On : OpenTK.VSyncMode.Off;
			RecreateDevice( game );
		}
		
		public override void OnWindowResize( Game game ) {
			RecreateDevice( game );
		}
		
		void RecreateDevice( Game game ) {
			PresentParameters args = GetPresentArgs( game.Width, game.Height );
			device.Reset( args );
			SetDefaultRenderStates();
			device.SetRenderState( RenderStates.AlphaTestEnable, alphaTest );
			device.SetRenderState( RenderStates.AlphaBlendEnable, alphaBlend );
			device.SetRenderState( RenderStates.AlphaFunction, (int)alphaTestFunc );
			device.SetRenderState( RenderStates.ReferenceAlpha, alphaTestRef );
			device.SetRenderState( RenderStates.SourceBlend, (int)srcFunc );
			device.SetRenderState( RenderStates.DestinationBlend, (int)dstFunc );
			device.SetRenderState( RenderStates.FogEnable, fogEnable );
			device.SetRenderState( RenderStates.FogColor, fogCol );
			device.SetRenderState( RenderStates.FogDensity, fogDensity );
			device.SetRenderState( RenderStates.FogStart, fogStart );
			device.SetRenderState( RenderStates.FogEnd, fogEnd );
			device.SetRenderState( RenderStates.FogTableMode, (int)fogMode );
			device.SetRenderState( RenderStates.ZBufferFunction, (int)depthTestFunc );
			device.SetRenderState( RenderStates.ZEnable, depthTest );
			device.SetRenderState( RenderStates.ZBufferWriteEnable, depthWrite );
		}
		
		void SetDefaultRenderStates() {
			device.SetRenderState( RenderStates.FillMode, (int)FillMode.Solid );
			FaceCulling = false;
			device.SetRenderState( RenderStates.ColorVertex, false );
			device.SetRenderState( RenderStates.Lighting, false );
			device.SetRenderState( RenderStates.SpecularEnable, false );
			device.SetRenderState( RenderStates.DebugMonitorToken, false );
		}
		
		PresentParameters GetPresentArgs( int width, int height ) {
			PresentParameters args = new PresentParameters();
			args.AutoDepthStencilFormat = DepthFormat.D24X8; // D32 doesn't work
			args.BackBufferWidth = width;
			args.BackBufferHeight = height;
			args.EnableAutoDepthStencil = true;
			args.PresentationInterval = vsync ? PresentInterval.One : PresentInterval.Immediate;
			args.SwapEffect = SwapEffect.Discard;
			args.Windowed = true;
			return args;
		}
		
		unsafe void memcpy( IntPtr sourcePtr, IntPtr destPtr, int bytes ) {
			byte* src = (byte*)sourcePtr;
			byte* dst = (byte*)destPtr;
			int* srcInt = (int*)src;
			int* dstInt = (int*)dst;

			while( bytes >= 4 ) {
				*dstInt++ = *srcInt++;
				dst += 4;
				src += 4;
				bytes -= 4;
			}
			// Handle non-aligned last few bytes.
			for( int i = 0; i < bytes; i++ ) {
				*dst++ = *src++;
			}
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
		
		static bool IsValid<T>( T[] array, int id ) {
			return id > 0 && id < array.Length && array[id] != null;
		}
		
		protected override void LoadOrthoMatrix( float width, float height ) {
			Matrix dxMatrix = Matrix.OrthoOffCenterRH( 0, width, height, 0, 0, 1 );
			curStack.SetTop( ref dxMatrix );
		}
		
		public override void Dispose() {
			base.Dispose();
			device.Dispose();
		}

		public override void PrintApiSpecificInfo() {
			Console.WriteLine( "D3D tex memory available: " + (uint)device.AvailableTextureMemory );
			Console.WriteLine( "D3D software vertex processing: " + device.SoftwareVertexProcessing );
			Console.WriteLine( "D3D hardware rasterization: " + caps.DeviceCaps.SupportsHardwareRasterization );
			Console.WriteLine( "D3D hardware T & L: " + caps.DeviceCaps.SupportsHardwareTransformAndLight );
			Console.WriteLine( "D3D hardware pure: " + caps.DeviceCaps.SupportsPureDevice );
		}

		public override void TakeScreenshot( string output, Size size ) {
			using( Surface backbuffer = device.GetBackBuffer( 0, 0, BackBufferType.Mono ) ) {
				SurfaceLoader.Save( output, ImageFileFormat.Png, backbuffer );
				// D3DX SurfaceLoader is the easiest way.. I tried to save manually and failed, as according to MSDN:
				// "This method fails on render targets unless they were created as lockable
				// (or, in the case of back buffers, with LockableBackBuffer of a PresentFlag)."
			}
		}
	}
}
#endif