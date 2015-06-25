using System;
using System.Drawing;
using System.Drawing.Imaging;
using OpenTK;
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
		
		public bool AlphaBlending {
			set { ToggleCap( EnableCap.Blend, value ); }
		}
		
		BlendingFactor[] blendFuncs = {
			BlendingFactor.Zero, BlendingFactor.One,
			BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha,
			BlendingFactor.DstAlpha, BlendingFactor.OneMinusDstAlpha,
		};
		public void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc destFunc ) {
			GL.BlendFunc( blendFuncs[(int)srcFunc], blendFuncs[(int)destFunc] );
		}
		
		FastColour lastFogCol = FastColour.Black;
		internal Vector4 modernFogCol;
		public void SetFogColour( FastColour col ) {
			modernFogCol = new Vector4( col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f );
		}
		
		internal float modernFogEnd, modernFogDensity;
		public void SetFogDensity( float value ) {
			modernFogDensity = value;
		}
		
		public void SetFogEnd( float value ) {
			modernFogEnd = value;
		}
		
		internal int modernFogMode = 0;
		public void SetFogMode( Fog mode ) {
			modernFogMode = mode == ClassicalSharp.GraphicsAPI.Fog.Linear ? 0 : 1;
		}
		
		public bool FaceCulling {
			set { ToggleCap( EnableCap.CullFace, value ); }
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

		#region Vertex buffers
		
		public unsafe int CreateDynamicVb( int stride, int maxVertices ) {
			int id = 0;
			GL.GenBuffers( 1, &id );
			int sizeInBytes = maxVertices * stride;
			GL.BindBuffer( BufferTarget.ArrayBuffer, id );
			GL.BufferData( BufferTarget.ArrayBuffer, new IntPtr( sizeInBytes ), IntPtr.Zero, BufferUsageHint.DynamicDraw );
			return id;
		}
		
		public unsafe int InitVb<T>( T[] vertices, int stride, int count ) where T : struct {
			int id = 0;
			GL.GenBuffers( 1, &id );
			int sizeInBytes = count * stride;
			GL.BindBuffer( BufferTarget.ArrayBuffer, id );
			GL.BufferData( BufferTarget.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsageHint.StaticDraw );
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
	}
}