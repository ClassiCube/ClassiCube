#if !USE_DX
using System;
using System.Drawing;
using System.Drawing.Imaging;
using OpenTK;
using OpenTK.Graphics.OpenGL;
using BmpPixelFormat = System.Drawing.Imaging.PixelFormat;
using GlPixelFormat = OpenTK.Graphics.OpenGL.PixelFormat;

namespace ClassicalSharp.GraphicsAPI {

	public class OpenGLApi : IGraphicsApi {
		
		BeginMode[] modeMappings = { BeginMode.Triangles, BeginMode.Lines, BeginMode.TriangleStrip };		
		public unsafe OpenGLApi() {
			int texDims;
			GL.GetIntegerv( GetPName.MaxTextureSize, &texDims );
			textureDimensions = texDims;
			string extensions = new String( (sbyte*)GL.GetString( StringName.Extensions ) );
			
			if( !extensions.Contains( "GL_ARB_vertex_buffer_object" ) ) {
				Utils.LogError( "ClassicalSharp post 0.6 version requires OpenGL VBOs." );
				Utils.LogWarning( "You may need to install and/or update your video card drivers." );
				Utils.LogWarning( "Alternatively, you can download the 'DLCompatibility build." );
				throw new InvalidOperationException( "Cannot use OpenGL vbos." );
			}
			base.InitDynamicBuffers();
			
			setupBatchFuncCol4b = SetupVbPos3fCol4b;
			setupBatchFuncTex2f = SetupVbPos3fTex2f;
			setupBatchFuncTex2fCol4b = SetupVbPos3fTex2fCol4b;
			GL.EnableClientState( ArrayCap.VertexArray );
		}
		
		public override bool AlphaTest {
			set { ToggleCap( EnableCap.AlphaTest, value ); }
		}
		
		public override bool AlphaBlending {
			set { ToggleCap( EnableCap.Blend, value ); }
		}
		
		Compare[] compareFuncs = {
			Compare.Always, Compare.Notequal, Compare.Never, Compare.Less,
			Compare.Lequal, Compare.Equal, Compare.Gequal, Compare.Greater,
		};
		public override void AlphaTestFunc( CompareFunc func, float value ) {
			GL.AlphaFunc( compareFuncs[(int)func], value );
		}
		
		BlendingFactor[] blendFuncs = {
			BlendingFactor.Zero, BlendingFactor.One,
			BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha,
			BlendingFactor.DstAlpha, BlendingFactor.OneMinusDstAlpha,
		};
		public override void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc dstFunc ) {
			GL.BlendFunc( blendFuncs[(int)srcFunc], blendFuncs[(int)dstFunc] );
		}
		
		public override bool Fog {
			set { ToggleCap( EnableCap.Fog, value ); }
		}
		
		FastColour lastFogCol = FastColour.Black;
		public unsafe override void SetFogColour( FastColour col ) {			
			if( col != lastFogCol ) {
				Vector4 colRGBA = new Vector4( col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f );
				GL.Fogfv( FogParameter.FogColor, &colRGBA.X );
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
				GL.Fogf( param, value );
				last = value;
			}
		}
		
		Fog lastFogMode = (Fog)999;
		FogMode[] fogModes = { FogMode.Linear, FogMode.Exp, FogMode.Exp2 };
		public override void SetFogMode( Fog mode ) {
			if( mode != lastFogMode ) {
				GL.Fogi( FogParameter.FogMode, (int)fogModes[(int)mode] );
				lastFogMode = mode;
			}
		}
		
		public override bool FaceCulling {
			set { ToggleCap( EnableCap.CullFace, value ); }
		}
		
		public override void Clear() {
			GL.Clear( ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit );
		}
		
		FastColour lastClearCol;
		public override void ClearColour( FastColour col ) {
			if( col != lastClearCol ) {
				GL.ClearColor( col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f );
				lastClearCol = col;
			}
		}
		
		public override bool ColourWrite {
			set { GL.ColorMask( value, value, value, value ); }
		}
		
		public override void DepthTestFunc( CompareFunc func ) {
			GL.DepthFunc( compareFuncs[(int)func] );
		}
		
		public override bool DepthTest {
			set { ToggleCap( EnableCap.DepthTest, value ); }
		}
		
		public override bool DepthWrite {
			set { GL.DepthMask( value ); }
		}
		
		#region Texturing
		int textureDimensions;
		public override int MaxTextureDimensions {
			get { return textureDimensions; }
		}
		
		public override bool Texturing {
			set { ToggleCap( EnableCap.Texture2D, value ); }
		}
		
		public unsafe override int CreateTexture( int width, int height, IntPtr scan0 ) {
			if( !Utils.IsPowerOf2( width ) || !Utils.IsPowerOf2( height ) )
				Utils.LogWarning( "Creating a non power of two texture." );
			
			int texId = 0;
			GL.GenTextures( 1, &texId );
			GL.BindTexture( TextureTarget.Texture2D, texId );
			GL.TexParameteri( TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureFilter.Nearest );
			GL.TexParameteri( TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureFilter.Nearest );

			GL.TexImage2D( TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, width, height,
			              GlPixelFormat.Bgra, PixelType.UnsignedByte, scan0 );
			return texId;
		}
		
		public override void BindTexture( int texture ) {
			GL.BindTexture( TextureTarget.Texture2D, texture );
		}
		
		public unsafe override void DeleteTexture( ref int texId ) {
			if( texId <= 0 ) return;
			int id = texId;
			GL.DeleteTextures( 1, &id );
			texId = -1;
		}				
		#endregion
		
		#region Vertex/index buffers		
		Action setupBatchFunc;
		Action setupBatchFuncTex2f, setupBatchFuncCol4b, setupBatchFuncTex2fCol4b;
		
		public override int CreateDynamicVb( VertexFormat format, int maxVertices ) {
			int id = GenAndBind( BufferTarget.ArrayBuffer );
			int sizeInBytes = maxVertices * strideSizes[(int)format];
			GL.BufferDataARB( BufferTarget.ArrayBuffer, new IntPtr( sizeInBytes ), IntPtr.Zero, BufferUsage.DynamicDraw );
			return id;
		}
		
		public override int CreateVb<T>( T[] vertices, VertexFormat format, int count ) {
			int id = GenAndBind( BufferTarget.ArrayBuffer );
			int sizeInBytes = count * strideSizes[(int)format];
			GL.BufferDataARB( BufferTarget.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsage.StaticDraw );
			return id;
		}
		
		public override int CreateVb( IntPtr vertices, VertexFormat format, int count ) {
			int id = GenAndBind( BufferTarget.ArrayBuffer );
			int sizeInBytes = count * strideSizes[(int)format];
			GL.BufferDataARB( BufferTarget.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsage.StaticDraw );
			return id;
		}
		
		public override int CreateIb( ushort[] indices, int indicesCount ) {
			int id = GenAndBind( BufferTarget.ElementArrayBuffer );
			int sizeInBytes = indicesCount * sizeof( ushort );
			GL.BufferDataARB( BufferTarget.ElementArrayBuffer, new IntPtr( sizeInBytes ), indices, BufferUsage.StaticDraw );
			return id;
		}
		
		public override int CreateIb( IntPtr indices, int indicesCount ) {
			int id = GenAndBind( BufferTarget.ElementArrayBuffer );
			int sizeInBytes = indicesCount * sizeof( ushort );		
			GL.BufferDataARB( BufferTarget.ElementArrayBuffer, new IntPtr( sizeInBytes ), indices, BufferUsage.StaticDraw );
			return id;
		}
		
		unsafe static int GenAndBind( BufferTarget target ) {
			int id = 0;
			GL.GenBuffersARB( 1, &id );
			GL.BindBufferARB( target, id );
			return id;
		}
		
		public override void DrawDynamicVb<T>( DrawMode mode, int id, T[] vertices, VertexFormat format, int count ) {
			int sizeInBytes = count * strideSizes[(int)format];
			GL.BindBufferARB( BufferTarget.ArrayBuffer, id );
			GL.BufferSubDataARB( BufferTarget.ArrayBuffer, IntPtr.Zero, new IntPtr( sizeInBytes ), vertices );
			
			BeginVbBatch( format );
			setupBatchFunc();
			GL.DrawArrays( modeMappings[(int)mode], 0, count );
		}
		
		public unsafe override void DeleteDynamicVb( int id ) {
			if( id <= 0 ) return;
			GL.DeleteBuffersARB( 1, &id );
		}
		
		public unsafe override void DeleteVb( int vb ) {
			if( vb <= 0 ) return;
			GL.DeleteBuffersARB( 1, &vb );
		}
		
		public unsafe override void DeleteIb( int ib ) {
			if( ib <= 0 ) return;
			GL.DeleteBuffersARB( 1, &ib );
		}
		
		VertexFormat batchFormat = (VertexFormat)999;
		public override void BeginVbBatch( VertexFormat format ) {
			if( format == batchFormat ) return;
			
			if( batchFormat == VertexFormat.Pos3fTex2fCol4b ) {
				GL.DisableClientState( ArrayCap.ColorArray );
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			} else if( batchFormat == VertexFormat.Pos3fTex2f ) {
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			} else if( batchFormat == VertexFormat.Pos3fCol4b ) {
				GL.DisableClientState( ArrayCap.ColorArray );
			}
			
			batchFormat = format;
			if( format == VertexFormat.Pos3fTex2fCol4b ) {
				GL.EnableClientState( ArrayCap.ColorArray );
				GL.EnableClientState( ArrayCap.TextureCoordArray );
				setupBatchFunc = setupBatchFuncTex2fCol4b;
			} else if( format == VertexFormat.Pos3fTex2f ) {
				GL.EnableClientState( ArrayCap.TextureCoordArray );
				setupBatchFunc = setupBatchFuncTex2f;
			} else if( format == VertexFormat.Pos3fCol4b ) {
				GL.EnableClientState( ArrayCap.ColorArray );
				setupBatchFunc = setupBatchFuncCol4b;
			}
		}
		
		public override void DrawVb( DrawMode mode, int startVertex, int verticesCount ) {
			setupBatchFunc();
			GL.DrawArrays( modeMappings[(int)mode], startVertex, verticesCount );
		}
		
		public override void BindVb( int vb ) {
			GL.BindBufferARB( BufferTarget.ArrayBuffer, vb );
		}
		
		public override void BindIb( int ib ) {
			GL.BindBufferARB( BufferTarget.ElementArrayBuffer, ib );
		}
		
		const DrawElementsType indexType = DrawElementsType.UnsignedShort;
		public override void DrawIndexedVb( DrawMode mode, int indicesCount, int startIndex ) {
			setupBatchFunc();
			GL.DrawElements( modeMappings[(int)mode], indicesCount, indexType, new IntPtr( startIndex * 2 ) );
		}
		
		internal override void DrawIndexedVb_TrisT2fC4b( int indicesCount, int startIndex ) {
			GL.VertexPointer( 3, PointerType.Float, 24, zero );
			GL.ColorPointer( 4, PointerType.UnsignedByte, 24, twelve );
			GL.TexCoordPointer( 2, PointerType.Float, 24, sixteen );
			GL.DrawElements( BeginMode.Triangles, indicesCount, indexType, new IntPtr( startIndex * 2 ) );
		}
		
		internal override void DrawIndexedVb_TrisT2fC4b( int indicesCount, int startVertex, int startIndex ) {
			int offset = startVertex * VertexPos3fTex2fCol4b.Size;
			GL.VertexPointer( 3, PointerType.Float, 24, new IntPtr( offset ) );
			GL.ColorPointer( 4, PointerType.UnsignedByte, 24, new IntPtr( offset + 12 ) );
			GL.TexCoordPointer( 2, PointerType.Float, 24, new IntPtr( offset + 16 ) );
			GL.DrawElements( BeginMode.Triangles, indicesCount, indexType, new IntPtr( startIndex * 2 ) );
		}
		
		IntPtr zero = new IntPtr( 0 ), twelve = new IntPtr( 12 ), sixteen = new IntPtr( 16 );
		void SetupVbPos3fTex2f() {
			GL.VertexPointer( 3, PointerType.Float, VertexPos3fTex2f.Size, zero );
			GL.TexCoordPointer( 2, PointerType.Float, VertexPos3fTex2f.Size, twelve );
		}
		
		void SetupVbPos3fCol4b() {
			GL.VertexPointer( 3, PointerType.Float, VertexPos3fCol4b.Size, zero );
			GL.ColorPointer( 4, PointerType.UnsignedByte, VertexPos3fCol4b.Size, twelve );
		}
		
		void SetupVbPos3fTex2fCol4b() {
			GL.VertexPointer( 3, PointerType.Float, VertexPos3fTex2fCol4b.Size, zero );
			GL.ColorPointer( 4, PointerType.UnsignedByte, VertexPos3fTex2fCol4b.Size, twelve );
			GL.TexCoordPointer( 2, PointerType.Float, VertexPos3fTex2fCol4b.Size, sixteen );
		}
		#endregion
				
		#region Matrix manipulation
		MatrixMode lastMode = 0;
		MatrixMode[] matrixModes = { MatrixMode.Projection, MatrixMode.Modelview, MatrixMode.Texture };
		public override void SetMatrixMode( MatrixType mode ) {
			MatrixMode glMode = matrixModes[(int)mode];
			if( glMode != lastMode ) {
				GL.MatrixMode( glMode );
				lastMode = glMode;
			}
		}
		
		public unsafe override void LoadMatrix( ref Matrix4 matrix ) {
			fixed( Single* ptr = &matrix.Row0.X )
				GL.LoadMatrixf( ptr );
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
		
		public unsafe override void MultiplyMatrix( ref Matrix4 matrix ) {
			fixed( Single* ptr = &matrix.Row0.X )
				GL.MultMatrixf( ptr );
		}
		
		#endregion
		
		public override void BeginFrame( Game game ) {
		}
		
		public override void EndFrame( Game game ) {
			game.SwapBuffers();
		}
		
		public override void SetVSync( Game game, bool value ) {
			game.VSync = value;
		}
		
		public unsafe override void PrintApiSpecificInfo() {
			Utils.Log( "OpenGL vendor: " + new String( (sbyte*)GL.GetString( StringName.Vendor ) ) );
			Utils.Log( "OpenGL renderer: " + new String( (sbyte*)GL.GetString( StringName.Renderer ) ) );
			Utils.Log( "OpenGL version: " + new String( (sbyte*)GL.GetString( StringName.Version ) ) );
			int depthBits = 0;
			GL.GetIntegerv( GetPName.DepthBits, &depthBits );
			Utils.Log( "Depth buffer bits: " + depthBits );
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
					GL.ReadPixels( 0, 0, size.Width, size.Height, GlPixelFormat.Bgra, PixelType.UnsignedByte, fastBmp.Scan0 );
				}
				bmp.RotateFlip( RotateFlipType.RotateNoneFlipY );
				bmp.Save( output, ImageFormat.Png );
			}
		}
		
		public override void OnWindowResize( Game game ) {
			GL.Viewport( 0, 0, game.Width, game.Height );
		}
		
		static void ToggleCap( EnableCap cap, bool value ) {
			if( value ) GL.Enable( cap );
			else GL.Disable( cap );
		}

		public void UpdateTexturePart( int texId, int x, int y, FastBitmap part ) {
			GL.Enable( EnableCap.Texture2D );
			GL.BindTexture( TextureTarget.Texture2D, texId );
			GL.TexSubImage2D( TextureTarget.Texture2D, 0, x, y, part.Width, part.Height,
			                 GlPixelFormat.Bgra, PixelType.UnsignedByte, part.Scan0 );
			GL.Disable( EnableCap.Texture2D );
		}
	}
}
#endif