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

	public partial class OpenGLApi {
		
		int textureDimensions;
		BeginMode[] modeMappings = { BeginMode.Triangles, BeginMode.Lines, BeginMode.TriangleStrip };
		
		public unsafe OpenGLApi() {
			int texDims;
			GL.GetInteger( GetPName.MaxTextureSize, &texDims );
			textureDimensions = texDims;
			InitDynamicBuffers();
			
			drawBatchFuncCol4b = DrawVbPos3fCol4bFast;
			drawBatchFuncTex2f = DrawVbPos3fTex2fFast;
			drawBatchFuncTex2fCol4b = DrawVbPos3fTex2fCol4bFast;
		}

		public int MaxTextureDimensions {
			get { return textureDimensions; }
		}
		
		public bool AlphaTest {
			set { ToggleCap( EnableCap.AlphaTest, value ); }
		}
		
		public bool AlphaBlending {
			set { ToggleCap( EnableCap.Blend, value ); }
		}
		
		AlphaFunction[] alphaFuncs = {
			AlphaFunction.Always, AlphaFunction.Notequal,
			AlphaFunction.Never, AlphaFunction.Less,
			AlphaFunction.Lequal, AlphaFunction.Equal,
			AlphaFunction.Gequal, AlphaFunction.Greater,
		};
		public void AlphaTestFunc( CompareFunc func, float value ) {
			GL.AlphaFunc( alphaFuncs[(int)func], value );
		}
		
		BlendingFactor[] blendFuncs = {
			BlendingFactor.Zero, BlendingFactor.One,
			BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha,
			BlendingFactor.DstAlpha, BlendingFactor.OneMinusDstAlpha,
		};
		public void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc destFunc ) {
			GL.BlendFunc( blendFuncs[(int)srcFunc], blendFuncs[(int)destFunc] );
		}
		
		public bool Fog {
			set { ToggleCap( EnableCap.Fog, value ); }
		}
		
		FastColour lastFogCol = FastColour.Black;
		internal Vector4 modernFogCol;
		public unsafe void SetFogColour( FastColour col ) {			
			if( col != lastFogCol ) {
				Vector4 colRGBA = new Vector4( col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f );
				modernFogCol = colRGBA;
				GL.Fog( FogParameter.FogColor, &colRGBA.X );
				lastFogCol = col;
			}
		}
		
		float lastFogStart = -1, lastFogEnd = -1, lastFogDensity = -1;
		internal float modernFogStart, modernFogEnd, modernFogDensity;
		public void SetFogDensity( float value ) {
			FogParam( FogParameter.FogDensity, value, ref lastFogDensity );
			modernFogDensity = value;
		}
		
		public void SetFogStart( float value ) {
			FogParam( FogParameter.FogStart, value, ref lastFogStart );
			modernFogStart = value;
		}
		
		public void SetFogEnd( float value ) {
			FogParam( FogParameter.FogEnd, value, ref lastFogEnd );
			modernFogEnd = value;
		}
		
		static void FogParam( FogParameter param, float value, ref float last ) {
			if( value != last ) {
				GL.Fog( param, value );
				last = value;
			}
		}
		
		Fog lastFogMode = (Fog)999;
		FogMode[] fogModes = { FogMode.Linear, FogMode.Exp };
		internal int modernFogMode = 0;
		public void SetFogMode( Fog mode ) {
			if( mode != lastFogMode ) {
				GL.Fog( FogParameter.FogMode, (int)fogModes[(int)mode] );
				modernFogMode = mode == ClassicalSharp.GraphicsAPI.Fog.Linear ? 0 : 1;
				lastFogMode = mode;
			}
		}
		
		public bool FaceCulling {
			set { ToggleCap( EnableCap.CullFace, value ); }
		}
		
		
		#if TRACK_RESOURCES
		Dictionary<int, string> textures = new Dictionary<int, string>();
		#endif

		public unsafe int LoadTexture( int width, int height, IntPtr scan0 ) {
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
		
		public void Bind2DTexture( int texture ) {
			GL.BindTexture( TextureTarget.Texture2D, texture );
		}
		
		public unsafe void DeleteTexture( ref int texId ) {
			if( texId <= 0 ) return;
			#if TRACK_RESOURCES
			textures.Remove( texId );
			#endif
			int id = texId;
			GL.DeleteTextures( 1, &id );
			texId = -1;
		}
		
		public bool IsValidTexture( int texId ) {
			return GL.IsTexture( texId );
		}
		
		public bool Texturing {
			set { ToggleCap( EnableCap.Texture2D, value ); }
		}
		
		public void Clear() {
			GL.Clear( ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit );
		}
		
		FastColour lastClearCol;
		public void ClearColour( FastColour col ) {
			if( col != lastClearCol ) {
				GL.ClearColor( col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f );
				lastClearCol = col;
			}
		}
		
		public bool ColourWrite {
			set { GL.ColorMask( value, value, value, value ); }
		}
		
		DepthFunction[] depthFuncs = {
			DepthFunction.Always, DepthFunction.Notequal,
			DepthFunction.Never, DepthFunction.Less,
			DepthFunction.Lequal, DepthFunction.Equal,
			DepthFunction.Gequal, DepthFunction.Greater,
		};
		public void DepthTestFunc( CompareFunc func ) {
			GL.DepthFunc( depthFuncs[(int)func] );
		}
		
		public bool DepthTest {
			set { ToggleCap( EnableCap.DepthTest, value ); }
		}
		
		public bool DepthWrite {
			set { GL.DepthMask( value ); }
		}
		
		public void SetFillType( FillType type ) {;
		}
		
		#region Vertex buffers
		
		#if TRACK_RESOURCES
		Dictionary<int, string> vbs = new Dictionary<int, string>();
		#endif
		Action<DrawMode, int, int, int> drawBatchFunc;
		Action<DrawMode, int, int, int> drawBatchFuncTex2f;
		Action<DrawMode, int, int, int> drawBatchFuncCol4b;
		Action<DrawMode, int, int, int> drawBatchFuncTex2fCol4b;
		
		
		public unsafe int CreateDynamicVb( VertexFormat format, int maxVertices ) {
			int id = 0;
			GL.GenBuffers( 1, &id );
			int sizeInBytes = maxVertices * strideSizes[(int)format];
			GL.BindBuffer( BufferTarget.ArrayBuffer, id );
			GL.BufferData( BufferTarget.ArrayBuffer, new IntPtr( sizeInBytes ), IntPtr.Zero, BufferUsageHint.DynamicDraw );
			GL.BindBuffer( BufferTarget.ArrayBuffer, 0 );
			return id;
		}
		
		public unsafe int InitVb<T>( T[] vertices, VertexFormat format, int count ) where T : struct {
			int id = 0;
			GL.GenBuffers( 1, &id );
			int sizeInBytes = count * strideSizes[(int)format];
			GL.BindBuffer( BufferTarget.ArrayBuffer, id );
			GL.BufferData( BufferTarget.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsageHint.StaticDraw );
			GL.BindBuffer( BufferTarget.ArrayBuffer, 0 );
			#if TRACK_RESOURCES
			vbs.Add( id, Environment.StackTrace );
			#endif
			return id;
		}
		
		public unsafe int InitIb( ushort[] indices, int indicesCount ) {
			int id = 0;
			GL.GenBuffers( 1, &id );
			int sizeInBytes = indicesCount * sizeof( ushort );
			GL.BindBuffer( BufferTarget.ElementArrayBuffer, id );
			fixed( ushort* ptr = indices ) {
				GL.BufferData( BufferTarget.ElementArrayBuffer, new IntPtr( sizeInBytes ), (IntPtr)ptr, BufferUsageHint.StaticDraw );
			}
			GL.BindBuffer( BufferTarget.ElementArrayBuffer, 0 );
			return id;
		}
		
		public void DrawDynamicVb<T>( DrawMode mode, int vb, T[] vertices, VertexFormat format, int count ) where T : struct {
			int sizeInBytes = count * strideSizes[(int)format];
			GL.BindBuffer( BufferTarget.ArrayBuffer, vb );
			GL.BufferSubData( BufferTarget.ArrayBuffer, IntPtr.Zero, new IntPtr( sizeInBytes ), vertices );
			
			BeginVbBatch( format );
			DrawVbBatch( mode, vb, 0, count );
			EndVbBatch();
		}
		
		public unsafe void DeleteDynamicVb( int id ) {
			if( id <= 0 ) return;
			GL.DeleteBuffers( 1, &id );
		}
		
		public unsafe void DeleteVb( int id ) {
			if( id <= 0 ) return;
			#if TRACK_RESOURCES
			vbs.Remove( id );
			#endif
			GL.DeleteBuffers( 1, &id );
		}
		
		public unsafe void DeleteIb( int id ) {
			if( id <= 0 ) return;
			GL.DeleteBuffers( 1, &id );
		}
		
		public bool IsValidVb( int vb ) {
			return GL.IsBuffer( vb );
		}
		
		public bool IsValidIb( int ib ) {
			return GL.IsBuffer( ib );
		}
		
		public void DrawVb( DrawMode mode, VertexFormat format, int id, int startVertex, int verticesCount ) {
			BeginVbBatch( format );
			DrawVbBatch( mode, id, startVertex, verticesCount );
			EndVbBatch();
		}
		
		VertexFormat batchFormat = (VertexFormat)999;
		public void BeginVbBatch( VertexFormat format ) {
			batchFormat = format;
			GL.EnableClientState( ArrayCap.VertexArray );
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
		
		public void BeginIndexedVbBatch() {
			BeginVbBatch( VertexFormat.Pos3fTex2fCol4b );
		}
		
		public void DrawVbBatch( DrawMode mode, int id, int startVertex, int verticesCount ) {
			drawBatchFunc( mode, id, startVertex, verticesCount );
		}
		
		const DrawElementsType indexType = DrawElementsType.UnsignedShort;
		public void DrawIndexedVbBatch( DrawMode mode, int vb, int ib, int indicesCount,
		                                        int startVertex, int startIndex ) {
			GL.BindBuffer( BufferTarget.ArrayBuffer, vb );
			GL.BindBuffer( BufferTarget.ElementArrayBuffer, ib );
			
			int offset = startVertex * VertexPos3fTex2fCol4b.Size;
			GL.VertexPointer( 3, VertexPointerType.Float, 24, new IntPtr( offset ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 24, new IntPtr( offset + 12 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 24, new IntPtr( offset + 16 ) );
			GL.DrawElements( modeMappings[(int)mode], indicesCount, indexType, new IntPtr( startIndex * 2 ) );
		}
		
		public void EndVbBatch() {
			GL.BindBuffer( BufferTarget.ArrayBuffer, 0 );
			GL.DisableClientState( ArrayCap.VertexArray );
			if( batchFormat == VertexFormat.Pos3fTex2fCol4b ) {
				GL.DisableClientState( ArrayCap.ColorArray );
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			} else if( batchFormat == VertexFormat.Pos3fTex2f ) {
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			} else if( batchFormat == VertexFormat.Pos3fCol4b ) {
				GL.DisableClientState( ArrayCap.ColorArray );
			}
		}
		
		public void EndIndexedVbBatch() {
			GL.BindBuffer( BufferTarget.ElementArrayBuffer, 0 );
			EndVbBatch();
		}
		
		void DrawVbPos3fTex2fFast( DrawMode mode, int id, int offset, int verticesCount ) {
			GL.BindBuffer( BufferTarget.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 20, new IntPtr( 0 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 20, new IntPtr( 12 ) );
			GL.DrawArrays( modeMappings[(int)mode], offset, verticesCount );
		}
		
		void DrawVbPos3fCol4bFast( DrawMode mode, int id, int offset, int verticesCount ) {
			GL.BindBuffer( BufferTarget.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 16, new IntPtr( 0 ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 16, new IntPtr( 12 ) );
			GL.DrawArrays( modeMappings[(int)mode], offset, verticesCount );
		}
		
		void DrawVbPos3fTex2fCol4bFast( DrawMode mode, int id, int offset, int verticesCount ) {
			GL.BindBuffer( BufferTarget.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 24, new IntPtr( 0 ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 24, new IntPtr( 12 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 24, new IntPtr( 16 ) );
			GL.DrawArrays( modeMappings[(int)mode], offset, verticesCount );
		}
		#endregion
		
		
		#region Matrix manipulation

		MatrixMode lastMode = 0;
		MatrixMode[] matrixModes = { MatrixMode.Projection, MatrixMode.Modelview, MatrixMode.Texture };
		public void SetMatrixMode( MatrixType mode ) {
			MatrixMode glMode = matrixModes[(int)mode];
			if( glMode != lastMode ) {
				GL.MatrixMode( glMode );
				lastMode = glMode;
			}
		}
		
		public unsafe void LoadMatrix( ref Matrix4 matrix ) {
			fixed( Single* ptr = &matrix.Row0.X )
				GL.LoadMatrix( ptr );
		}
		
		public void LoadIdentityMatrix() {
			GL.LoadIdentity();
		}
		
		public void PushMatrix() {
			GL.PushMatrix();
		}
		
		public void PopMatrix() {
			GL.PopMatrix();
		}
		
		public unsafe void MultiplyMatrix( ref Matrix4 matrix ) {
			fixed( Single* ptr = &matrix.Row0.X )
				GL.MultMatrix( ptr );
		}
		
		#endregion
		
		#if TRACK_RESOURCES
		public void CheckResources() {
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
		
		public void BeginFrame( Game game ) {
		}
		
		public void EndFrame( Game game ) {
			game.SwapBuffers();
		}
		
		public unsafe void PrintApiSpecificInfo() {
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
		public void TakeScreenshot( string output, Size size ) {
			using( Bitmap bmp = new Bitmap( size.Width, size.Height, BmpPixelFormat.Format32bppRgb ) ) { // ignore alpha component
				using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
					GL.ReadPixels( 0, 0, size.Width, size.Height, GlPixelFormat.Bgra, PixelType.UnsignedByte, fastBmp.Scan0 );
				}
				bmp.RotateFlip( RotateFlipType.RotateNoneFlipY );
				bmp.Save( output, ImageFormat.Png );
			}
		}
		
		public void OnWindowResize( int newWidth, int newHeight ) {
			GL.Viewport( 0, 0, newWidth, newHeight );
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