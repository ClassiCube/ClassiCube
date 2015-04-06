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
		bool nonPow2;
		const string nonPow2Ext = "GL_ARB_texture_non_power_of_two";
		const string vboExt = "GL_ARB_vertex_buffer_object";
		BeginMode[] modeMappings = new BeginMode[] {
			BeginMode.Triangles, BeginMode.Lines, BeginMode.Points,
			BeginMode.TriangleStrip, BeginMode.LineStrip, BeginMode.TriangleFan
		};
		
		public OpenGLApi() {
			GL.GetInteger( GetPName.MaxTextureSize, out textureDimensions );
			string extensions = GL.GetString( StringName.Extensions );
			nonPow2 = extensions.Contains( nonPow2Ext );
			bool useVbos = extensions.Contains( vboExt );
			if( !useVbos ) {
				Utils.LogError( "Unable to use OpenGL VBOs." );
				throw new NotSupportedException( "Display lists not supported on optimised branch" );
			}
			Setup2DCache();
		}

		public int MaxTextureDimensions {
			get { return textureDimensions; }
		}
		
		public bool SupportsNonPowerOf2Textures {
			get { return nonPow2; }
		}
		
		public bool AlphaBlending {
			set { ToggleCap( EnableCap.Blend, value ); }
		}
		
		BlendEquationMode[] blendEqs = new BlendEquationMode[] {
			BlendEquationMode.FuncAdd, BlendEquationMode.Max,
			BlendEquationMode.Min, BlendEquationMode.FuncSubtract,
			BlendEquationMode.FuncReverseSubtract,
		};
		public void AlphaBlendEq( BlendEquation eq ) {
			GL.BlendEquation( blendEqs[(int)eq] );
		}
		
		BlendingFactorSrc[] srcBlendFuncs = new BlendingFactorSrc[] {
			BlendingFactorSrc.Zero, BlendingFactorSrc.One,
			BlendingFactorSrc.SrcAlpha, BlendingFactorSrc.OneMinusSrcAlpha,
			BlendingFactorSrc.DstAlpha, BlendingFactorSrc.OneMinusDstAlpha,
		};
		BlendingFactorDest[] destBlendFuncs = new BlendingFactorDest[] {
			BlendingFactorDest.Zero, BlendingFactorDest.One,
			BlendingFactorDest.SrcAlpha, BlendingFactorDest.OneMinusSrcAlpha,
			BlendingFactorDest.DstAlpha, BlendingFactorDest.OneMinusDstAlpha,
		};
		public void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc destFunc ) {
			GL.BlendFunc( srcBlendFuncs[(int)srcFunc], destBlendFuncs[(int)destFunc] );
		}
		
		public Fog FogMode = Fog.None;
		public float FogDensity, FogEnd;
		public Vector4 FogColour;
		
		public void SetFogColour( FastColour col ) {
			FogColour = new Vector4( col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f );
		}
		
		public void SetFogDensity( float value ) {
			FogDensity = value;
		}
		
		public void SetFogEnd( float value ) {
			FogEnd = value;
		}
		
		public void SetFogMode( Fog mode ) {
			FogMode = mode;
		}
		
		public bool FaceCulling {
			set { ToggleCap( EnableCap.CullFace, value ); }
		}
		
		
		#if TRACK_RESOURCES
		Dictionary<int, string> textures = new Dictionary<int, string>();
		#endif
		public int LoadTexture( Bitmap bmp ) {
			using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
				return LoadTexture( fastBmp );
			}
		}
		
		public int LoadTexture( FastBitmap bmp ) {
			int texId = GL.GenTexture();
			GL.BindTexture( TextureTarget.Texture2D, texId );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest );
			if( !bmp.IsLocked ) {
				bmp.LockBits();
			}

			GL.TexImage2D( TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, bmp.Width, bmp.Height, 0,
			              GlPixelFormat.Bgra, PixelType.UnsignedByte, bmp.Scan0 );
			bmp.UnlockBits();
			#if TRACK_RESOURCES
			textures.Add( texId, Environment.StackTrace );
			#endif
			return texId;
		}
		
		public void Bind2DTexture( int texture ) {
			GL.BindTexture( TextureTarget.Texture2D, texture );
		}
		
		public void DeleteTexture( int texId ) {
			if( texId <= 0 ) return;
			#if TRACK_RESOURCES
			textures.Remove( texId );
			#endif
			GL.DeleteTexture( texId );
		}
		
		public void Clear() {
			GL.Clear( ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit );
		}
		
		public void Clear( ClearBufferMask mask ) {
			GL.Clear( mask );
		}
		
		FastColour lastClearCol;
		public void ClearColour( FastColour col ) {
			if( col != lastClearCol ) {
				Color4 col4 = new Color4( col.R, col.G, col.B, col.A );
				GL.ClearColor( col4 );
				lastClearCol = col;
			}
		}
		
		public void ColourMask( bool red, bool green, bool blue, bool alpha ) {
			GL.ColorMask( red, green, blue, alpha );
		}
		
		public void Viewport( int width, int height ) {
			GL.Viewport( 0, 0, width, height );
		}
		
		DepthFunction[] depthFuncs = new DepthFunction[] {
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
		
		PolygonMode[] fillModes = new PolygonMode[] { PolygonMode.Point, PolygonMode.Line, PolygonMode.Fill };
		public void SetFillType( FillType type ) {
			GL.PolygonMode( MaterialFace.FrontAndBack, fillModes[(int)type] );
		}
		
		#region Vertex buffers
		
		#if TRACK_RESOURCES
		Dictionary<int, string> vbs = new Dictionary<int, string>();
		Dictionary<int, int> vbMemorys = new Dictionary<int, int>();
		long totalVbMem = 0;
		Dictionary<int, int> indexedVbMemorys = new Dictionary<int, int>();
		long totalIndexedVbMem = 0;
		#endif
		
		public int InitVb<T>( T[] vertices, VertexFormat format, int count ) where T : struct {
			return CreateVb( vertices, format, count, BufferUsageArb.StaticDraw );
		}
		
		public int InitDynamicVb<T>( T[] vertices, VertexFormat format, int count ) where T : struct {
			return CreateVb( vertices, format, count, BufferUsageArb.StreamDraw );
		}
		
		public int CreateEmptyDynamicVb(VertexFormat format, int capacity ) {
			return CreateEmptyVb( format, capacity, BufferUsageArb.StreamDraw );
		}
		
		public void UpdateDynamicVb<T>( int id, T[] vertices, VertexFormat format ) where T : struct {
			// TODO: Is it better to 
			// BindBuffer(null), then BindBuffer(vertices) ?
			int sizeInBytes = GetSizeInBytes( vertices.Length, format );
			//GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.Arb.BufferSubData( BufferTargetArb.ArrayBuffer, IntPtr.Zero, new IntPtr( sizeInBytes ), vertices );
		}
		
		int CreateVb<T>( T[] vertices, VertexFormat format, int count, BufferUsageArb usage ) where T : struct {
			int id = 0;
			GL.Arb.GenBuffers( 1, out id );
			int sizeInBytes = GetSizeInBytes( count, format );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.Arb.BufferData( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, usage );
			return id;
		}
		
		int CreateEmptyVb( VertexFormat format, int capacity, BufferUsageArb usage ) {
			int id = 0;
			GL.Arb.GenBuffers( 1, out id );
			int sizeInBytes = GetSizeInBytes( capacity, format );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.Arb.BufferData( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), IntPtr.Zero, usage );
			return id;
		}
		
		public IndexedVbInfo InitIndexedVb<T>( T[] vertices, int verticesCount, VertexFormat format, 
		                                      ushort[] indices, int elements ) where T : struct {
			IndexedVbInfo info = new IndexedVbInfo( 0, 0 );
			GL.Arb.GenBuffers( 2, out info.VbId );
			
			int sizeInBytes = GetSizeInBytes( verticesCount, format );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, info.VbId );
			GL.Arb.BufferData( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsageArb.StaticDraw );
			
			sizeInBytes = elements * sizeof( ushort );
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, info.IbId );
			GL.Arb.BufferData( BufferTargetArb.ElementArrayBuffer, new IntPtr( sizeInBytes ), indices, BufferUsageArb.StaticDraw );
			#if TRACK_RESOURCES
			indexedVbMemorys.Add( info.VbId, sizeInBytes + GetSizeInBytes( verticesCount, format ) );
			totalIndexedVbMem += sizeInBytes + GetSizeInBytes( verticesCount, format );
			Console.WriteLine( "INDEXED VB MEM " + ( totalIndexedVbMem / 1024.0 / 1024.0 ) );
			#endif
			return info;
		}
		
		public void DeleteVb( int id ) {
			if( id <= 0 ) return;
			#if TRACK_RESOURCES
			vbs.Remove( id );
			int mem;
			if( vbMemorys.TryGetValue( id, out mem ) ) {
				totalVbMem -= mem;
			}
			vbMemorys.Remove( id );
			#endif
			GL.Arb.DeleteBuffers( 1, ref id );
		}
		
		public void DeleteIndexedVb( IndexedVbInfo id ) {
			if( id.VbId <= 0 || id.IbId <= 0 ) return;
			GL.Arb.DeleteBuffers( 2, ref id.VbId );
			#if TRACK_RESOURCES
			int mem;
			if( indexedVbMemorys.TryGetValue( id.VbId, out mem ) ) {
				totalIndexedVbMem -= mem;
			}
			indexedVbMemorys.Remove( id.VbId );
			#endif
		}
		
		public void BindVb( int vb ) {
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, vb );
		}
		
		public void BindIb( int ib ) {
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, ib );
		}
		
		public void DrawVb( DrawMode mode, int offset, int vertices ) {
			GL.DrawArrays( modeMappings[(int)mode], offset, vertices );
		}
		
		public void DrawIndexedVb( DrawMode mode, int indicesCount ) {
			GL.DrawElements( modeMappings[(int)mode], indicesCount, DrawElementsType.UnsignedShort, 0 );
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
		
		public void PrintApiSpecificInfo() {
			Console.WriteLine( "OpenGL vendor: " + GL.GetString( StringName.Vendor ) );
			Console.WriteLine( "OpenGL renderer: " + GL.GetString( StringName.Renderer ) );
			Console.WriteLine( "OpenGL version: " + GL.GetString( StringName.Version ) );
			int depthBits = 0;
			GL.GetInteger( GetPName.DepthBits, out depthBits );
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

		public Color4 GetCol() {
			float[] col = new float[4];
			GL.GetFloat( GetPName.CurrentColor, out col[0] );
			return new Color4( col[0], col[1], col[2], col[3] );
		}
		
		static void ToggleCap( EnableCap cap, bool value ) {
			if( value ) GL.Enable( cap );
			else GL.Disable( cap );
		}
		
		public void SaveTexture( int texId, int width, int height, string path ) {
			GL.Enable( EnableCap.Texture2D );
			GL.BindTexture( TextureTarget.Texture2D, texId );
			//GL.CopyTexSubImage2D( TextureTarget.Texture2D, 0, 0, 0, 0, 0, width, height );
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