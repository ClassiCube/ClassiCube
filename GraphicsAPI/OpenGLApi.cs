//#define TRACK_RESOURCES
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL;
using BmpPixelFormat = System.Drawing.Imaging.PixelFormat;
using GlPixelFormat = OpenTK.Graphics.OpenGL.PixelFormat;

namespace ClassicalSharp.GraphicsAPI {

	public class OpenGLApi : IGraphicsApi {
		
		int textureDimensions;
		const string vboExt = "GL_ARB_vertex_buffer_object";
		BeginMode[] modeMappings = { BeginMode.Triangles, BeginMode.Lines, BeginMode.TriangleStrip };
		
		public unsafe OpenGLApi() {
			int texDims;
			GL.GetInteger( GetPName.MaxTextureSize, &texDims );
			textureDimensions = texDims;
			string extensions = GL.GetString( StringName.Extensions );
			
			if( !extensions.Contains( vboExt ) ) {
				Utils.LogError( "ClassicalSharp post 0.6 version requires OpenGL VBOs." );
				Utils.LogWarning( "You may need to install and/or update your video card drivers." );
				Utils.LogWarning( "Alternatively, you can download the 'DLCompatibility build." );
				throw new InvalidOperationException( "Cannot use OpenGL vbos." );
			}
			base.InitDynamicBuffers();
			
			drawBatchFuncCol4b = DrawVbPos3fCol4bFast;
			drawBatchFuncTex2f = DrawVbPos3fTex2fFast;
			drawBatchFuncTex2fCol4b = DrawVbPos3fTex2fCol4bFast;
			GL.EnableClientState( ArrayCap.VertexArray );
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
			GL.AlphaFunc( alphaFuncs[(int)func], value );
		}
		
		BlendingFactor[] blendFuncs = {
			BlendingFactor.Zero, BlendingFactor.One,
			BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha,
			BlendingFactor.DstAlpha, BlendingFactor.OneMinusDstAlpha,
		};
		public override void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc destFunc ) {
			GL.BlendFunc( blendFuncs[(int)srcFunc], blendFuncs[(int)destFunc] );
		}
		
		public override bool Fog {
			set { ToggleCap( EnableCap.Fog, value ); }
		}
		
		FastColour lastFogCol = FastColour.Black;
		public unsafe override void SetFogColour( FastColour col ) {			
			if( col != lastFogCol ) {
				Vector4 colRGBA = new Vector4( col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f );
				GL.Fog( FogParameter.FogColor, &colRGBA.X );
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
				GL.Fog( param, value );
				last = value;
			}
		}
		
		Fog lastFogMode = (Fog)999;
		FogMode[] fogModes = { FogMode.Linear, FogMode.Exp, FogMode.Exp2 };
		public override void SetFogMode( Fog mode ) {
			if( mode != lastFogMode ) {
				GL.Fog( FogParameter.FogMode, (int)fogModes[(int)mode] );
				lastFogMode = mode;
			}
		}
		
		public override bool FaceCulling {
			set { ToggleCap( EnableCap.CullFace, value ); }
		}
		
		
		#if TRACK_RESOURCES
		Dictionary<int, string> textures = new Dictionary<int, string>();
		#endif

		public unsafe override int LoadTexture( int width, int height, IntPtr scan0 ) {
			if( !Utils.IsPowerOf2( width ) || !Utils.IsPowerOf2( height ) )
				Utils.LogWarning( "Creating a non power of two texture." );
			
			int texId = 0;
			GL.GenTextures( 1, &texId );
			GL.Enable( EnableCap.Texture2D );
			GL.BindTexture( TextureTarget.Texture2D, texId );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest );

			GL.TexImage2D( TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, width, height, 0,
			              GlPixelFormat.Bgra, PixelType.UnsignedByte, scan0 );
			GL.Disable( EnableCap.Texture2D );
			#if TRACK_RESOURCES
			textures.Add( texId, Environment.StackTrace );
			#endif
			return texId;
		}
		
		public override void Bind2DTexture( int texture ) {
			GL.BindTexture( TextureTarget.Texture2D, texture );
		}
		
		public unsafe override void DeleteTexture( ref int texId ) {
			if( texId <= 0 ) return;
			#if TRACK_RESOURCES
			textures.Remove( texId );
			#endif
			int id = texId;
			GL.DeleteTextures( 1, &id );
			texId = -1;
		}
		
		public override bool IsValidTexture( int texId ) {
			return GL.IsTexture( texId );
		}
		
		public override bool Texturing {
			set { ToggleCap( EnableCap.Texture2D, value ); }
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
		
		DepthFunction[] depthFuncs = {
			DepthFunction.Always, DepthFunction.Notequal,
			DepthFunction.Never, DepthFunction.Less,
			DepthFunction.Lequal, DepthFunction.Equal,
			DepthFunction.Gequal, DepthFunction.Greater,
		};
		public override void DepthTestFunc( CompareFunc func ) {
			GL.DepthFunc( depthFuncs[(int)func] );
		}
		
		public override bool DepthTest {
			set { ToggleCap( EnableCap.DepthTest, value ); }
		}
		
		public override bool DepthWrite {
			set { GL.DepthMask( value ); }
		}
		
		PolygonMode[] fillModes = { PolygonMode.Point, PolygonMode.Line, PolygonMode.Fill };
		public override void SetFillType( FillType type ) {
			GL.PolygonMode( MaterialFace.FrontAndBack, fillModes[(int)type] );
		}
		
		#region Vertex buffers
		
		#if TRACK_RESOURCES
		Dictionary<int, string> vbs = new Dictionary<int, string>();
		#endif
		Action<DrawMode, int, int, int> drawBatchFunc;
		Action<DrawMode, int, int, int> drawBatchFuncTex2f;
		Action<DrawMode, int, int, int> drawBatchFuncCol4b;
		Action<DrawMode, int, int, int> drawBatchFuncTex2fCol4b;
		
		
		public unsafe override int CreateDynamicVb( VertexFormat format, int maxVertices ) {
			int id = 0;
			GL.Arb.GenBuffers( 1, &id );
			int sizeInBytes = maxVertices * strideSizes[(int)format];
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.Arb.BufferData( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), IntPtr.Zero, BufferUsageArb.DynamicDraw );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			return id;
		}
		
		public unsafe override int InitVb<T>( T[] vertices, VertexFormat format, int count ) {
			int id = 0;
			GL.Arb.GenBuffers( 1, &id );
			int sizeInBytes = count * strideSizes[(int)format];
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.Arb.BufferData( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsageArb.StaticDraw );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			#if TRACK_RESOURCES
			vbs.Add( id, Environment.StackTrace );
			#endif
			return id;
		}
		
		public unsafe override int InitIb( ushort[] indices, int indicesCount ) {
			int id = 0;
			GL.Arb.GenBuffers( 1, &id );
			int sizeInBytes = indicesCount * sizeof( ushort );
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, id );
			fixed( ushort* ptr = indices ) {
				GL.Arb.BufferData( BufferTargetArb.ElementArrayBuffer, new IntPtr( sizeInBytes ), (IntPtr)ptr, BufferUsageArb.StaticDraw );
			}
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, 0 );
			return id;
		}
		
		public override void DrawDynamicVb<T>( DrawMode mode, int vb, T[] vertices, VertexFormat format, int count ) {
			int sizeInBytes = count * strideSizes[(int)format];
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, vb );
			GL.Arb.BufferSubData( BufferTargetArb.ArrayBuffer, IntPtr.Zero, new IntPtr( sizeInBytes ), vertices );
			
			BeginVbBatch( format );
			DrawVbBatch( mode, vb, 0, count );
			EndVbBatch();
		}
		
		public unsafe override void DeleteDynamicVb( int id ) {
			if( id <= 0 ) return;
			GL.Arb.DeleteBuffers( 1, &id );
		}
		
		public unsafe override void DeleteVb( int id ) {
			if( id <= 0 ) return;
			#if TRACK_RESOURCES
			vbs.Remove( id );
			#endif
			GL.Arb.DeleteBuffers( 1, &id );
		}
		
		public unsafe override void DeleteIb( int id ) {
			if( id <= 0 ) return;
			GL.Arb.DeleteBuffers( 1, &id );
		}
		
		public override bool IsValidVb( int vb ) {
			return GL.Arb.IsBuffer( vb );
		}
		
		public override bool IsValidIb( int ib ) {
			return GL.Arb.IsBuffer( ib );
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
				drawBatchFunc = drawBatchFuncTex2fCol4b;
			} else if( format == VertexFormat.Pos3fTex2f ) {
				GL.EnableClientState( ArrayCap.TextureCoordArray );
				drawBatchFunc = drawBatchFuncTex2f;
			} else if( format == VertexFormat.Pos3fCol4b ) {
				GL.EnableClientState( ArrayCap.ColorArray );
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
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, vb );
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, ib );
			
			int offset = startVertex * VertexPos3fTex2fCol4b.Size;
			GL.VertexPointer( 3, VertexPointerType.Float, 24, new IntPtr( offset ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 24, new IntPtr( offset + 12 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 24, new IntPtr( offset + 16 ) );
			GL.DrawElements( modeMappings[(int)mode], indicesCount, indexType, new IntPtr( startIndex * 2 ) );
		}
		
		public override void EndVbBatch() {
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
		}
		
		public override void EndIndexedVbBatch() {
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, 0 );
		}
		
		void DrawVbPos3fTex2fFast( DrawMode mode, int id, int offset, int verticesCount ) {
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 20, new IntPtr( 0 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 20, new IntPtr( 12 ) );
			GL.DrawArrays( modeMappings[(int)mode], offset, verticesCount );
		}
		
		void DrawVbPos3fCol4bFast( DrawMode mode, int id, int offset, int verticesCount ) {
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 16, new IntPtr( 0 ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 16, new IntPtr( 12 ) );
			GL.DrawArrays( modeMappings[(int)mode], offset, verticesCount );
		}
		
		void DrawVbPos3fTex2fCol4bFast( DrawMode mode, int id, int offset, int verticesCount ) {
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 24, new IntPtr( 0 ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 24, new IntPtr( 12 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 24, new IntPtr( 16 ) );
			GL.DrawArrays( modeMappings[(int)mode], offset, verticesCount );
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
				GL.LoadMatrix( ptr );
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
				GL.MultMatrix( ptr );
		}
		
		public override void Translate( float x, float y, float z ) {
			GL.Translate( x, y, z );
		}
		
		public override void RotateX( float degrees ) {
			GL.Rotate( degrees, 1f, 0f, 0f );
		}
		
		public override void RotateY( float degrees ) {
			GL.Rotate( degrees, 0f, 1f, 0f );
		}
		
		public override void RotateZ( float degrees ) {
			GL.Rotate( degrees, 0f, 0f, 1f );
		}
		
		public override void Scale( float x, float y, float z ) {
			GL.Scale( x, y, z );
		}
		
		#endregion
		
		#if TRACK_RESOURCES
		public override void CheckResources() {
			if( textures.Count > 0 ) {
				foreach( var pair in textures ) {
					Console.WriteLine( pair.Value );
					Console.WriteLine( "for tex id " + pair.Key );
					Console.WriteLine( "===========" );
				}
			}
			if( vbs.Count > 0 ) {
				foreach( var pair in vbs ) {
					Console.WriteLine( pair.Value );
					Console.WriteLine( "for vb id " + pair.Key );
					Console.WriteLine( "===========" );
				}
			}
			Console.WriteLine( "tex " + textures.Count + ", vb" + vbs.Count );
			if( textures.Count > 0 || vbs.Count > 0 ) {
				System.Diagnostics.Debugger.Break();
			}
		}
		#endif
		
		public override void BeginFrame( Game game ) {
		}
		
		public override void EndFrame( Game game ) {
			game.SwapBuffers();
		}
		
		public unsafe override void PrintApiSpecificInfo() {
			Console.WriteLine( "OpenGL vendor: " + GL.GetString( StringName.Vendor ) );
			Console.WriteLine( "OpenGL renderer: " + GL.GetString( StringName.Renderer ) );
			Console.WriteLine( "OpenGL version: " + GL.GetString( StringName.Version ) );
			int depthBits = 0;
			GL.GetInteger( GetPName.DepthBits, &depthBits );
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
					GL.ReadPixels( 0, 0, size.Width, size.Height, GlPixelFormat.Bgra, PixelType.UnsignedByte, fastBmp.Scan0 );
				}
				bmp.RotateFlip( RotateFlipType.RotateNoneFlipY );
				bmp.Save( output, ImageFormat.Png );
			}
		}
		
		public override void OnWindowResize( int newWidth, int newHeight ) {
			GL.Viewport( 0, 0, newWidth, newHeight );
		}
		
		static void ToggleCap( EnableCap cap, bool value ) {
			if( value ) GL.Enable( cap );
			else GL.Disable( cap );
		}
		
		public void SaveTexture( int texId, int width, int height, string path ) {
			GL.Enable( EnableCap.Texture2D );
			GL.BindTexture( TextureTarget.Texture2D, texId );
			using( Bitmap bmp = new Bitmap( width, height, BmpPixelFormat.Format32bppArgb ) ) {
				using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
					GL.GetTexImage( TextureTarget.Texture2D, 0, GlPixelFormat.Bgra, PixelType.UnsignedByte, fastBmp.Scan0 );
				}
				bmp.Save( path );
			}
			GL.Disable( EnableCap.Texture2D );
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