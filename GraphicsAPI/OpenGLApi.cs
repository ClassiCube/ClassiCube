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
		internal float modernFogEnd, modernFogDensity;
		public void SetFogDensity( float value ) {
			FogParam( FogParameter.FogDensity, value, ref lastFogDensity );
			modernFogDensity = value;
		}
		
		public void SetFogStart( float value ) {
			FogParam( FogParameter.FogStart, value, ref lastFogStart );
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
			return texId;
		}
		
		public void Bind2DTexture( int texture ) {
			GL.BindTexture( TextureTarget.Texture2D, texture );
		}
		
		public unsafe void DeleteTexture( ref int texId ) {
			if( texId <= 0 ) return;
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
		
		public unsafe void DeleteDynamicVb( int id ) {
			if( id <= 0 ) return;
			GL.DeleteBuffers( 1, &id );
		}
		
		public unsafe void DeleteVb( int id ) {
			if( id <= 0 ) return;
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
		
		#endregion
		
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