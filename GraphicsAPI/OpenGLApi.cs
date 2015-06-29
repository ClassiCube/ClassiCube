using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using OpenTK;
using OpenTK.Graphics.OpenGL;
using BmpPixelFormat = System.Drawing.Imaging.PixelFormat;
using GlPixelFormat = OpenTK.Graphics.OpenGL.PixelFormat;
using Gl = OpenTK.Graphics.OpenGL.GL.Delegates;

namespace ClassicalSharp.GraphicsAPI {

	public class OpenGLApi : IGraphicsApi {
		
		int textureDimensions;
		BeginMode[] modeMappings = { BeginMode.Triangles, BeginMode.Lines, BeginMode.TriangleStrip };
		
		public unsafe OpenGLApi() {
			int texDims;
			Gl.glGetIntegerv( GetPName.MaxTextureSize, &texDims );
			textureDimensions = texDims;
			string extensions = new String( (sbyte*)Gl.glGetString( StringName.Extensions ) );
			
			if( !extensions.Contains( "GL_ARB_vertex_buffer_object" ) ) {
				Utils.LogError( "ClassicalSharp post 0.6 version requires OpenGL VBOs." );
				Utils.LogWarning( "You may need to install and/or update your video card drivers." );
				Utils.LogWarning( "Alternatively, you can download the 'DLCompatibility build." );
				throw new InvalidOperationException( "Cannot use OpenGL vbos." );
			}
			base.InitDynamicBuffers();
			
			drawBatchFuncCol4b = DrawVbPos3fCol4b;
			drawBatchFuncTex2f = DrawVbPos3fTex2f;
			drawBatchFuncTex2fCol4b = DrawVbPos3fTex2fCol4b;
			Gl.glEnableClientState( ArrayCap.VertexArray );
		}

		public override int MaxTextureDimensions {
			get { return textureDimensions; }
		}
		
		public override bool AlphaTest {
			set { ToggleCap( EnableCap.AlphaTest, value ); }
		}
		
		public override bool AlphaBlending {
			set { ToggleCap( EnableCap.Blend, value ); }
		}
		
		AlphaFunction[] alphaFuncs = {
			AlphaFunction.Always, AlphaFunction.Notequal,
			AlphaFunction.Never, AlphaFunction.Less,
			AlphaFunction.Lequal, AlphaFunction.Equal,
			AlphaFunction.Gequal, AlphaFunction.Greater,
		};
		public override void AlphaTestFunc( CompareFunc func, float value ) {
			Gl.glAlphaFunc( alphaFuncs[(int)func], value );
		}
		
		BlendingFactor[] blendFuncs = {
			BlendingFactor.Zero, BlendingFactor.One,
			BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha,
			BlendingFactor.DstAlpha, BlendingFactor.OneMinusDstAlpha,
		};
		public override void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc destFunc ) {
			Gl.glBlendFunc( blendFuncs[(int)srcFunc], blendFuncs[(int)destFunc] );
		}
		
		public override bool Fog {
			set { ToggleCap( EnableCap.Fog, value ); }
		}
		
		FastColour lastFogCol = FastColour.Black;
		public unsafe override void SetFogColour( FastColour col ) {			
			if( col != lastFogCol ) {
				Vector4 colRGBA = new Vector4( col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f );
				Gl.glFogfv( FogParameter.FogColor, &colRGBA.X );
				lastFogCol = col;
			}
		}
		
		float lastFogStart = -1, lastFogEnd = -1, lastFogDensity = -1;
		public override void SetFogDensity( float value ) {
			FogParam( FogParameter.FogDensity, value, ref lastFogDensity );
		}
		
		public override void SetFogStart( float value ) {
			FogParam( FogParameter.FogStart, value, ref lastFogStart );
		}
		
		public override void SetFogEnd( float value ) {
			FogParam( FogParameter.FogEnd, value, ref lastFogEnd );
		}
		
		static void FogParam( FogParameter param, float value, ref float last ) {
			if( value != last ) {
				Gl.glFogf( param, value );
				last = value;
			}
		}
		
		Fog lastFogMode = (Fog)999;
		FogMode[] fogModes = { FogMode.Linear, FogMode.Exp, FogMode.Exp2 };
		public override void SetFogMode( Fog mode ) {
			if( mode != lastFogMode ) {
				Gl.glFogi( FogParameter.FogMode, (int)fogModes[(int)mode] );
				lastFogMode = mode;
			}
		}
		
		public override bool FaceCulling {
			set { ToggleCap( EnableCap.CullFace, value ); }
		}
		

		public unsafe override int LoadTexture( int width, int height, IntPtr scan0 ) {
			if( !Utils.IsPowerOf2( width ) || !Utils.IsPowerOf2( height ) )
				Utils.LogWarning( "Creating a non power of two texture." );
			
			int texId = 0;
			Gl.glGenTextures( 1, &texId );
			Gl.glEnable( EnableCap.Texture2D );
			Gl.glBindTexture( TextureTarget.Texture2D, texId );
			Gl.glTexParameteri( TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest );
			Gl.glTexParameteri( TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest );

			Gl.glTexImage2D( TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, width, height, 0,
			              GlPixelFormat.Bgra, PixelType.UnsignedByte, scan0 );
			Gl.glDisable( EnableCap.Texture2D );
			return texId;
		}
		
		public override void Bind2DTexture( int texture ) {
			Gl.glBindTexture( TextureTarget.Texture2D, texture );
		}
		
		public unsafe override void DeleteTexture( ref int texId ) {
			if( texId <= 0 ) return;
			int id = texId;
			Gl.glDeleteTextures( 1, &id );
			texId = -1;
		}
		
		public override bool IsValidTexture( int texId ) {
			return Gl.glIsTexture( texId );
		}
		
		public override bool Texturing {
			set { ToggleCap( EnableCap.Texture2D, value ); }
		}
		
		public override void Clear() {
			Gl.glClear( ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit );
		}
		
		FastColour lastClearCol;
		public override void ClearColour( FastColour col ) {
			if( col != lastClearCol ) {
				Gl.glClearColor( col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f );
				lastClearCol = col;
			}
		}
		
		public override bool ColourWrite {
			set { Gl.glColorMask( value, value, value, value ); }
		}
		
		DepthFunction[] depthFuncs = {
			DepthFunction.Always, DepthFunction.Notequal,
			DepthFunction.Never, DepthFunction.Less,
			DepthFunction.Lequal, DepthFunction.Equal,
			DepthFunction.Gequal, DepthFunction.Greater,
		};
		public override void DepthTestFunc( CompareFunc func ) {
			Gl.glDepthFunc( depthFuncs[(int)func] );
		}
		
		public override bool DepthTest {
			set { ToggleCap( EnableCap.DepthTest, value ); }
		}
		
		public override bool DepthWrite {
			set { Gl.glDepthMask( value ); }
		}
		
		#region Vertex buffers
		
		Action<DrawMode, int, int, int> drawBatchFunc;
		Action<DrawMode, int, int, int> drawBatchFuncTex2f;
		Action<DrawMode, int, int, int> drawBatchFuncCol4b;
		Action<DrawMode, int, int, int> drawBatchFuncTex2fCol4b;		
		
		public unsafe override int CreateDynamicVb( VertexFormat format, int maxVertices ) {
			int id = 0;
			Gl.glGenBuffersARB( 1, &id );
			int sizeInBytes = maxVertices * strideSizes[(int)format];
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, id );
			Gl.glBufferDataARB( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), IntPtr.Zero, BufferUsageArb.DynamicDraw );
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, 0 );
			return id;
		}
		
		public unsafe override int InitVb<T>( T[] vertices, VertexFormat format, int count ) {
			int id = 0;
			Gl.glGenBuffersARB( 1, &id );
			int sizeInBytes = count * strideSizes[(int)format];
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, id );
			GCHandle handle = GCHandle.Alloc( vertices, GCHandleType.Pinned );
			Gl.glBufferDataARB( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), handle.AddrOfPinnedObject(), BufferUsageArb.StaticDraw );
			handle.Free();
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, 0 );
			return id;
		}
		
		public unsafe override int InitIb( ushort[] indices, int indicesCount ) {
			int id = 0;
			Gl.glGenBuffersARB( 1, &id );
			int sizeInBytes = indicesCount * sizeof( ushort );
			Gl.glBindBufferARB( BufferTargetArb.ElementArrayBuffer, id );
			fixed( ushort* ptr = indices ) {
				Gl.glBufferDataARB( BufferTargetArb.ElementArrayBuffer, new IntPtr( sizeInBytes ), (IntPtr)ptr, BufferUsageArb.StaticDraw );
			}
			Gl.glBindBufferARB( BufferTargetArb.ElementArrayBuffer, 0 );
			return id;
		}
		
		public override void DrawDynamicVb<T>( DrawMode mode, int vb, T[] vertices, VertexFormat format, int count ) {
			int sizeInBytes = count * strideSizes[(int)format];
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, vb );
			GCHandle handle = GCHandle.Alloc( vertices, GCHandleType.Pinned );
			Gl.glBufferSubDataARB( BufferTargetArb.ArrayBuffer, IntPtr.Zero, new IntPtr( sizeInBytes ), handle.AddrOfPinnedObject() );
			handle.Free();
			
			BeginVbBatch( format );
			DrawVbBatch( mode, vb, 0, count );
			EndVbBatch();
		}
		
		public unsafe override void DeleteDynamicVb( int id ) {
			if( id <= 0 ) return;
			Gl.glDeleteBuffersARB( 1, &id );
		}
		
		public unsafe override void DeleteVb( int id ) {
			if( id <= 0 ) return;
			Gl.glDeleteBuffersARB( 1, &id );
		}
		
		public unsafe override void DeleteIb( int id ) {
			if( id <= 0 ) return;
			Gl.glDeleteBuffersARB( 1, &id );
		}
		
		public override bool IsValidVb( int vb ) {
			return Gl.glIsBufferARB( vb );
		}
		
		public override bool IsValidIb( int ib ) {
			return Gl.glIsBufferARB( ib );
		}
		
		public override void DrawVb( DrawMode mode, VertexFormat format, int id, int startVertex, int verticesCount ) {
			BeginVbBatch( format );
			DrawVbBatch( mode, id, startVertex, verticesCount );
			EndVbBatch();
		}
		
		VertexFormat batchFormat = (VertexFormat)999;
		public override void BeginVbBatch( VertexFormat format ) {
			if( format == batchFormat ) return;
			
			if( batchFormat == VertexFormat.Pos3fTex2fCol4b ) {
				Gl.glDisableClientState( ArrayCap.ColorArray );
				Gl.glDisableClientState( ArrayCap.TextureCoordArray );
			} else if( batchFormat == VertexFormat.Pos3fTex2f ) {
				Gl.glDisableClientState( ArrayCap.TextureCoordArray );
			} else if( batchFormat == VertexFormat.Pos3fCol4b ) {
				Gl.glDisableClientState( ArrayCap.ColorArray );
			}
			
			batchFormat = format;
			if( format == VertexFormat.Pos3fTex2fCol4b ) {
				Gl.glEnableClientState( ArrayCap.ColorArray );
				Gl.glEnableClientState( ArrayCap.TextureCoordArray );
				drawBatchFunc = drawBatchFuncTex2fCol4b;
			} else if( format == VertexFormat.Pos3fTex2f ) {
				Gl.glEnableClientState( ArrayCap.TextureCoordArray );
				drawBatchFunc = drawBatchFuncTex2f;
			} else if( format == VertexFormat.Pos3fCol4b ) {
				Gl.glEnableClientState( ArrayCap.ColorArray );
				drawBatchFunc = drawBatchFuncCol4b;
			}
		}
		
		public override void BeginIndexedVbBatch() {
			BeginVbBatch( VertexFormat.Pos3fTex2fCol4b );
		}
		
		public override void DrawVbBatch( DrawMode mode, int id, int startVertex, int verticesCount ) {
			drawBatchFunc( mode, id, startVertex, verticesCount );
		}
		
		const DrawElementsType indexType = DrawElementsType.UnsignedShort;
		public override void DrawIndexedVbBatch( DrawMode mode, int vb, int ib, int indicesCount,
		                                        int startVertex, int startIndex ) {
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, vb );
			Gl.glBindBufferARB( BufferTargetArb.ElementArrayBuffer, ib );
			
			int offset = startVertex * VertexPos3fTex2fCol4b.Size;
			Gl.glVertexPointer( 3, VertexPointerType.Float, 24, new IntPtr( offset ) );
			Gl.glColorPointer( 4, ColorPointerType.UnsignedByte, 24, new IntPtr( offset + 12 ) );
			Gl.glTexCoordPointer( 2, TexCoordPointerType.Float, 24, new IntPtr( offset + 16 ) );
			Gl.glDrawElements( modeMappings[(int)mode], indicesCount, indexType, new IntPtr( startIndex * 2 ) );
		}
		
		public override void EndVbBatch() {
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, 0 );
		}
		
		public override void EndIndexedVbBatch() {
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, 0 );
			Gl.glBindBufferARB( BufferTargetArb.ElementArrayBuffer, 0 );
		}
		
		void DrawVbPos3fTex2f( DrawMode mode, int id, int offset, int verticesCount ) {
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, id );
			Gl.glVertexPointer( 3, VertexPointerType.Float, 20, new IntPtr( 0 ) );
			Gl.glTexCoordPointer( 2, TexCoordPointerType.Float, 20, new IntPtr( 12 ) );
			Gl.glDrawArrays( modeMappings[(int)mode], offset, verticesCount );
		}
		
		void DrawVbPos3fCol4b( DrawMode mode, int id, int offset, int verticesCount ) {
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, id );
			Gl.glVertexPointer( 3, VertexPointerType.Float, 16, new IntPtr( 0 ) );
			Gl.glColorPointer( 4, ColorPointerType.UnsignedByte, 16, new IntPtr( 12 ) );
			Gl.glDrawArrays( modeMappings[(int)mode], offset, verticesCount );
		}
		
		void DrawVbPos3fTex2fCol4b( DrawMode mode, int id, int offset, int verticesCount ) {
			Gl.glBindBufferARB( BufferTargetArb.ArrayBuffer, id );
			Gl.glVertexPointer( 3, VertexPointerType.Float, 24, new IntPtr( 0 ) );
			Gl.glColorPointer( 4, ColorPointerType.UnsignedByte, 24, new IntPtr( 12 ) );
			Gl.glTexCoordPointer( 2, TexCoordPointerType.Float, 24, new IntPtr( 16 ) );
			Gl.glDrawArrays( modeMappings[(int)mode], offset, verticesCount );
		}
		#endregion
		
		
		#region Matrix manipulation

		MatrixMode lastMode = 0;
		MatrixMode[] matrixModes = { MatrixMode.Projection, MatrixMode.Modelview, MatrixMode.Texture };
		public override void SetMatrixMode( MatrixType mode ) {
			MatrixMode glMode = matrixModes[(int)mode];
			if( glMode != lastMode ) {
				Gl.glMatrixMode( glMode );
				lastMode = glMode;
			}
		}
		
		public unsafe override void LoadMatrix( ref Matrix4 matrix ) {
			fixed( Single* ptr = &matrix.Row0.X )
				Gl.glLoadMatrixf( ptr );
		}
		
		public override void LoadIdentityMatrix() {
			Gl.glLoadIdentity();
		}
		
		public override void PushMatrix() {
			Gl.glPushMatrix();
		}
		
		public override void PopMatrix() {
			Gl.glPopMatrix();
		}
		
		public unsafe override void MultiplyMatrix( ref Matrix4 matrix ) {
			fixed( Single* ptr = &matrix.Row0.X )
				Gl.glMultMatrixf( ptr );
		}
		
		#endregion
		
		public override void BeginFrame( Game game ) {
		}
		
		public override void EndFrame( Game game ) {
			game.SwapBuffers();
		}
		
		public unsafe override void PrintApiSpecificInfo() {
			Console.WriteLine( "OpenGL vendor: " + new String( (sbyte*)Gl.glGetString( StringName.Vendor ) ) );
			Console.WriteLine( "OpenGL renderer: " + new String( (sbyte*)Gl.glGetString( StringName.Renderer ) ) );
			Console.WriteLine( "OpenGL version: " + new String( (sbyte*)Gl.glGetString( StringName.Version ) ) );
			int depthBits = 0;
			Gl.glGetIntegerv( GetPName.DepthBits, &depthBits );
			Console.WriteLine( "Depth buffer bits: " + depthBits );
			if( depthBits < 24 ) {
				Utils.LogWarning( "Depth buffer is less than 24 bits, you may see some issues " +
				                 "with disappearing and/or 'white' graphics." );
				Utils.LogWarning( "If this bothers you, type \"/client rendertype legacy\" (without quotes) " +
				                 "after you have loaded the first map." );
			}
		}
		
		// Based on http://www.opentk.com/doc/graphics/save-opengl-rendering-to-disk
		public override void TakeScreenshot( string output, Size size ) {
			using( Bitmap bmp = new Bitmap( size.Width, size.Height, BmpPixelFormat.Format32bppRgb ) ) { // ignore alpha component
				using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
					Gl.glReadPixels( 0, 0, size.Width, size.Height, GlPixelFormat.Bgra, PixelType.UnsignedByte, fastBmp.Scan0 );
				}
				bmp.RotateFlip( RotateFlipType.RotateNoneFlipY );
				bmp.Save( output, ImageFormat.Png );
			}
		}
		
		public override void OnWindowResize( int newWidth, int newHeight ) {
			Gl.glViewport( 0, 0, newWidth, newHeight );
		}
		
		static void ToggleCap( EnableCap cap, bool value ) {
			if( value ) Gl.glEnable( cap );
			else Gl.glDisable( cap );
		}

		public void UpdateTexturePart( int texId, int x, int y, FastBitmap part ) {
			Gl.glEnable( EnableCap.Texture2D );
			Gl.glBindTexture( TextureTarget.Texture2D, texId );
			Gl.glTexSubImage2D( TextureTarget.Texture2D, 0, x, y, part.Width, part.Height,
			                 GlPixelFormat.Bgra, PixelType.UnsignedByte, part.Scan0 );
			Gl.glDisable( EnableCap.Texture2D );
		}
	}
}