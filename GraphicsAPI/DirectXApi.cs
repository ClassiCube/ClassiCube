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
			PresentParameters args = new PresentParameters();
			args.Windowed = true;
			args.SwapEffect = SwapEffect.Discard;
			args.PresentationInterval = PresentInterval.Immediate;
			args.EnableAutoDepthStencil = true;
			args.AutoDepthStencilFormat = DepthFormat.D24X8; // D32 doesn't work
			
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
			texStack = new MatrixStack( 4, device, TransformType.Texture1 ); // TODO: Texture0?
			
			SetFillType( FillType.Solid );
			FaceCulling = false;
			device.SetRenderState( RenderStates.ColorVertex, false );
			device.SetRenderState( RenderStates.Lighting, false );
			device.SetRenderState( RenderStates.SpecularEnable, false );
			device.SetRenderState( RenderStates.DebugMonitorToken, false );
		}
		
		public override bool AlphaTest {
			set { device.SetRenderState( RenderStates.AlphaTestEnable, value ); }
		}

		public override bool AlphaBlending {
			set { device.SetRenderState( RenderStates.AlphaBlendEnable, value ); }
		}

		Compare[] compareFuncs = {
			Compare.Always, Compare.NotEqual, Compare.Never, Compare.Less,
			Compare.LessEqual, Compare.Equal, Compare.GreaterEqual, Compare.Greater,
		};
		public override void AlphaTestFunc( CompareFunc func, float value ) {
			device.SetRenderState( RenderStates.AlphaFunction, (int)compareFuncs[(int)func] );
			device.SetRenderState( RenderStates.ReferenceAlpha, (int)( value * 255 ) );
		}

		Blend[] blendFuncs = {
			Blend.Zero, Blend.One,
			Blend.SourceAlpha, Blend.InvSourceAlpha,
			Blend.DestinationAlpha, Blend.InvDestinationAlpha,
		};
		public override void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc dstFunc ) {
			device.SetRenderState( RenderStates.SourceBlend, (int)blendFuncs[(int)srcFunc] );
			device.SetRenderState( RenderStates.DestinationBlend, (int)blendFuncs[(int)dstFunc] );
		}

		public override bool Fog {
			set { device.SetRenderState( RenderStates.FogEnable, value ); }
		}

		public override void SetFogColour( FastColour col ) {
			device.SetRenderState( RenderStates.FogColor, col.ToColor().ToArgb() );
		}

		public override void SetFogDensity( float value ) {
			device.SetRenderState( RenderStates.FogDensity, value );
		}
		
		public override void SetFogStart( float value ) {
			device.SetRenderState( RenderStates.FogStart, value );
		}

		public override void SetFogEnd( float value ) {
			device.SetRenderState( RenderStates.FogEnd, value );
		}

		FogMode[] modes = { FogMode.Linear, FogMode.Exp, FogMode.Exp2 };
		public override void SetFogMode( Fog mode ) {
			device.SetRenderState( RenderStates.FogTableMode, (int)modes[(int)mode] );
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
			lastClearCol = col.ToColor().ToArgb();
		}

		public override bool ColourWrite {
			set { device.SetRenderState( RenderStates.ColorWriteEnable, value ? 0xF : 0x0 ); }
		}

		public override void DepthTestFunc( CompareFunc func ) {
			device.SetRenderState( RenderStates.ZBufferFunction, (int)compareFuncs[(int)func] );
		}

		public override bool DepthTest {
			set { device.SetRenderState( RenderStates.ZEnable, value ); }
		}

		public override bool DepthWrite {
			set { device.SetRenderState( RenderStates.ZBufferWriteEnable, value ); }
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

		FillMode[] fillModes = { FillMode.Point, FillMode.WireFrame, FillMode.Solid };
		public override void SetFillType( FillType type ) {
			device.SetRenderState( RenderStates.FillMode, (int)fillModes[(int)type] );
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
			GCHandle handle = GCHandle.Alloc( indices, GCHandleType.Pinned );
			IntPtr source = handle.AddrOfPinnedObject();
			IntPtr dest = vbData.InternalData;
			memcpy( source, dest, sizeInBytes );
			buffer.Unlock();
			handle.Free();
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

		public override void DrawVb( DrawMode mode, VertexFormat format, int id, int offset, int verticesCount ) {
			device.SetStreamSource( 0, vBuffers[id], 0, strideSizes[(int)format] );
			device.VertexFormat = formatMapping[(int)format];
			device.DrawPrimitives( modeMappings[(int)mode], offset, NumPrimitives( verticesCount, mode ) );
		}

		int batchStride;
		public override void BeginVbBatch( VertexFormat format ) {
			device.VertexFormat = formatMapping[(int)format];
			batchStride = strideSizes[(int)format];
		}

		public override void DrawVbBatch( DrawMode mode, int id, int offset, int verticesCount ) {
			device.SetStreamSource( 0, vBuffers[id], 0, batchStride );
			device.DrawPrimitives( modeMappings[(int)mode], offset, NumPrimitives( verticesCount, mode ) );
		}

		public override void EndVbBatch() {
		}
		
		public override void BeginIndexedVbBatch() {
			device.VertexFormat = formatMapping[(int)VertexFormat.Pos3fTex2fCol4b];
			batchStride = VertexPos3fTex2fCol4b.Size;
		}

		public override void DrawIndexedVbBatch( DrawMode mode, int vb, int ib, int indicesCount ) {
			device.Indices = iBuffers[ib];
			device.SetStreamSource( 0, vBuffers[vb], 0, batchStride );
			device.DrawIndexedPrimitives( modeMappings[(int)mode], 0, 0,
			                             indicesCount / 6 * 4, 0, NumPrimitives( indicesCount, mode ) );
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
			curStack.SetTop( ref dxMatrix );
		}

		Matrix identity = Matrix.Identity;
		public override void LoadIdentityMatrix() {
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

		public override void Translate( float x, float y, float z ) {
			Matrix matrix = Matrix.Translation( x, y, z );
			curStack.MultiplyTop( ref matrix );
		}

		public override void RotateX( float degrees ) {
			Matrix matrix = Matrix.RotationX( degrees * 0.01745329251f ); // PI / 180
			curStack.MultiplyTop( ref matrix );
		}

		public override void RotateY( float degrees ) {
			Matrix matrix = Matrix.RotationY( degrees * 0.01745329251f );
			curStack.MultiplyTop( ref matrix );
		}

		public override void RotateZ( float degrees ) {
			Matrix matrix = Matrix.RotationZ( degrees * 0.01745329251f );
			curStack.MultiplyTop( ref matrix );
		}

		public override void Scale( float x, float y, float z ) {
			Matrix matrix = Matrix.Scaling( x, y, z );
			curStack.MultiplyTop( ref matrix );
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
				stack[stackIndex] *= matrix;
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
		
		public override void OnWindowResize( int newWidth, int newHeight ) {
			//throw new NotSupportedException();
			// TODO: temp disabled to get Direct3D sort of working.
		}
		
		unsafe void memcpy( IntPtr sourcePtr, IntPtr destPtr, int bytes ) {
			byte* src = (byte*)sourcePtr;
			byte* dst = (byte*)destPtr;
			// TODO: check memcpy actually works and doesn't explode.

			while( bytes >= 4 ) {
				*( (int*)dst ) = *( (int*)src );
				dst += 4;
				src += 4;
				bytes -= 4;
			}
			// Handle non-aligned last 1-3 bytes.
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