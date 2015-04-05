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
		}

		public int MaxTextureDimensions {
			get { return textureDimensions; }
		}
		
		public bool SupportsNonPowerOf2Textures {
			get { return nonPow2; }
		}
		
		public bool AlphaTest {
			set { ToggleCap( EnableCap.AlphaTest, value ); }
		}
		
		public bool AlphaBlending {
			set { ToggleCap( EnableCap.Blend, value ); }
		}
		
		AlphaFunction[] alphaFuncs = new AlphaFunction[] {
			AlphaFunction.Always, AlphaFunction.Notequal,
			AlphaFunction.Never, AlphaFunction.Less,
			AlphaFunction.Lequal, AlphaFunction.Equal,
			AlphaFunction.Gequal, AlphaFunction.Greater,
		};
		public void AlphaTestFunc( CompareFunc func, float value ) {
			GL.AlphaFunc( alphaFuncs[(int)func], value );
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
			GL.Enable( EnableCap.Texture2D );
			GL.BindTexture( TextureTarget.Texture2D, texId );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest );
			if( !bmp.IsLocked ) {
				bmp.LockBits();
			}

			GL.TexImage2D( TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, bmp.Width, bmp.Height, 0,
			              GlPixelFormat.Bgra, PixelType.UnsignedByte, bmp.Scan0 );
			bmp.UnlockBits();
			GL.Disable( EnableCap.Texture2D );
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
		
		public bool Texturing {
			set { ToggleCap( EnableCap.Texture2D, value ); }
		}
		
		public void Clear() {
			GL.Clear( ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit );
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
		
		public void DrawVertices( DrawMode mode, VertexPos3fCol4b[] vertices, int count ) {
			//GL.DrawArrays( BeginMode.Triangles, 0, vertices.Length );
			// We can't just use GL.DrawArrays since we'd have to pin the array to prevent it from being moved around in memory.
			// Feasible alternatives:
			// - Use a dynamically updated VBO, and resize it (i.e. create a new bigger VBO) if required.
			// - Immediate mode.
			GL.Begin( modeMappings[(int)mode] );
			for( int i = 0; i < count; i++ ) {
				VertexPos3fCol4b vertex = vertices[i];
				GL.Color4( vertex.R, vertex.G, vertex.B, vertex.A );
				GL.Vertex3( vertex.X, vertex.Y, vertex.Z );
			}
			GL.Color4( 1f, 1f, 1f, 1f );
			GL.End();
		}
		
		public void DrawVertices( DrawMode mode, VertexPos3fTex2f[] vertices, int count ) {
			GL.Begin( modeMappings[(int)mode] );
			for( int i = 0; i < count; i++ ) {
				VertexPos3fTex2f vertex = vertices[i];
				GL.TexCoord2( vertex.U, vertex.V );
				GL.Vertex3( vertex.X, vertex.Y, vertex.Z );
			}
			GL.End();
		}
		
		public void DrawVertices( DrawMode mode, VertexPos3fTex2fCol4b[] vertices, int count ) {
			GL.Begin( modeMappings[(int)mode] );
			for( int i = 0; i < count; i++ ) {
				VertexPos3fTex2fCol4b vertex = vertices[i];
				GL.TexCoord2( vertex.U, vertex.V );
				GL.Color4( vertex.R, vertex.G, vertex.B, vertex.A );
				GL.Vertex3( vertex.X, vertex.Y, vertex.Z );
			}
			GL.Color4( 1f, 1f, 1f, 1f );
			GL.End();
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
		
		public int InitVb<T>( T[] vertices, DrawMode mode, VertexFormat format, int count ) where T : struct {
			int id = 0;
			GL.Arb.GenBuffers( 1, out id );
			int sizeInBytes = GetSizeInBytes( count, format );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.Arb.BufferData( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsageArb.StaticDraw );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			#if TRACK_RESOURCES
			vbs.Add( id, Environment.StackTrace );
			vbMemorys.Add( id, sizeInBytes );
			totalVbMem += sizeInBytes;
			Console.WriteLine( "VB MEM " + ( totalVbMem / 1024.0 / 1024.0 ) );
			#endif
			return id;
		}
		
		public IndexedVbInfo InitIndexedVb<T>( T[] vertices, int verticesCount, DrawMode mode, 
		                                               VertexFormat format, ushort[] indices, int elements ) where T : struct {
			IndexedVbInfo info = new IndexedVbInfo( 0, 0 );
			GL.Arb.GenBuffers( 2, out info.VbId );
			
			int sizeInBytes = GetSizeInBytes( verticesCount, format );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, info.VbId );
			GL.Arb.BufferData( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsageArb.StaticDraw );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			
			sizeInBytes = elements * sizeof( ushort );
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, info.IbId );
			GL.Arb.BufferData( BufferTargetArb.ElementArrayBuffer, new IntPtr( sizeInBytes ), indices, BufferUsageArb.StaticDraw );
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, 0 );
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
		
		public void DrawArrays( DrawMode mode, int offset, int vertices ) {
			GL.DrawArrays( modeMappings[(int)mode], offset, vertices );
		}
		
		public void DrawElements( DrawMode mode, int indicesCount ) {
			GL.DrawElements( modeMappings[(int)mode], indicesCount, DrawElementsType.UnsignedShort, 0 );
		}
		
		public void DrawVbPos3fTex2f( DrawMode mode, int id, int verticesCount ) {
			GL.EnableClientState( ArrayCap.VertexArray );
			GL.EnableClientState( ArrayCap.TextureCoordArray );
			
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 20, new IntPtr( 0 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 20, new IntPtr( 12 ) );
			GL.DrawArrays( modeMappings[(int)mode], 0, verticesCount );
			
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			GL.DisableClientState( ArrayCap.VertexArray );
			GL.DisableClientState( ArrayCap.TextureCoordArray );
		}
		
		public void DrawVbPos3fCol4b( DrawMode mode, int id, int verticesCount ) {
			GL.EnableClientState( ArrayCap.VertexArray );
			GL.EnableClientState( ArrayCap.ColorArray );
			
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 16, new IntPtr( 0 ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 16, new IntPtr( 12 ) );
			GL.DrawArrays( modeMappings[(int)mode], 0, verticesCount );
			
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			GL.DisableClientState( ArrayCap.VertexArray );
			GL.DisableClientState( ArrayCap.ColorArray );
		}
		
		public void DrawVbPos3fTex2fCol4b( DrawMode mode, int id, int verticesCount ) {
			GL.EnableClientState( ArrayCap.VertexArray );
			GL.EnableClientState( ArrayCap.TextureCoordArray );
			GL.EnableClientState( ArrayCap.ColorArray );
			
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 24, new IntPtr( 0 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 24, new IntPtr( 12 ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 24, new IntPtr( 20 ) );
			GL.DrawArrays( modeMappings[(int)mode], 0, verticesCount );
			
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			GL.DisableClientState( ArrayCap.VertexArray );
			GL.DisableClientState( ArrayCap.TextureCoordArray );
			GL.DisableClientState( ArrayCap.ColorArray );
		}
		
		#endregion
		
		
		#region Matrix manipulation

		MatrixMode lastMode = 0;
		MatrixMode[] matrixModes = new MatrixMode[] {
			MatrixMode.Projection, MatrixMode.Modelview, MatrixMode.Texture,
		};
		public void SetMatrixMode( MatrixType mode ) {
			MatrixMode glMode = matrixModes[(int)mode];
			if( glMode != lastMode ) {
				GL.MatrixMode( glMode );
				lastMode = glMode;
			}
		}
		
		public void LoadMatrix( ref Matrix4 matrix ) {
			GL.LoadMatrix( ref matrix );
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
		
		public void MultiplyMatrix( ref Matrix4 matrix ) {
			GL.MultMatrix( ref matrix );
		}
		
		public void Translate( float x, float y, float z ) {
			GL.Translate( x, y, z );
		}
		
		public void RotateX( float degrees ) {
			GL.Rotate( degrees, 1f, 0f, 0f );
		}
		
		public void RotateY( float degrees ) {
			GL.Rotate( degrees, 0f, 1f, 0f );
		}
		
		public void RotateZ( float degrees ) {
			GL.Rotate( degrees, 0f, 0f, 1f );
		}
		
		public void Scale( float x, float y, float z ) {
			GL.Scale( x, y, z );
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
		
		public void BeginDrawClientVertices( DrawMode mode ) {
			GL.Begin( (BeginMode)modeMappings[(int)mode] );
		}
		
		public void EndDrawClientVertices() {
			GL.End();
		}
		
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