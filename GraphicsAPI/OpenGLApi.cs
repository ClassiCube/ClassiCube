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
		bool useVbos = true;
		BeginMode[] modeMappings = new BeginMode[] {
			BeginMode.Triangles, BeginMode.Lines, BeginMode.Points,
			BeginMode.TriangleStrip, BeginMode.LineStrip, BeginMode.TriangleFan
		};
		
		public OpenGLApi() {
			GL.GetInteger( GetPName.MaxTextureSize, out textureDimensions );
			string extensions = GL.GetString( StringName.Extensions );
			useVbos = extensions.Contains( vboExt );
			if( !useVbos ) {
				Utils.LogWarning( "Unable to use OpenGL VBOs, you may experience reduced performance." );
			} else {
				drawBatchFuncCol4b = DrawVbPos3fCol4bFast;
				drawBatchFuncTex2f = DrawVbPos3fTex2fFast;
				drawBatchFuncTex2fCol4b = DrawVbPos3fTex2fCol4bFast;
			}
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
		
		AlphaFunction[] alphaFuncs = new AlphaFunction[] {
			AlphaFunction.Always, AlphaFunction.Notequal,
			AlphaFunction.Never, AlphaFunction.Less,
			AlphaFunction.Lequal, AlphaFunction.Equal,
			AlphaFunction.Gequal, AlphaFunction.Greater,
		};
		public override void AlphaTestFunc( CompareFunc func, float value ) {
			GL.AlphaFunc( alphaFuncs[(int)func], value );
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
		public override void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc destFunc ) {
			GL.BlendFunc( srcBlendFuncs[(int)srcFunc], destBlendFuncs[(int)destFunc] );
		}
		
		public override bool Fog {
			set { ToggleCap( EnableCap.Fog, value ); }
		}
		
		public unsafe override void SetFogColour( FastColour col ) {
			Vector4 colRGBA = new Vector4( col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f );
			GL.Fog( FogParameter.FogColor, &colRGBA.X );
		}
		
		public override void SetFogDensity( float value ) {
			GL.Fog( FogParameter.FogDensity, value );
		}
		
		public override void SetFogEnd( float value ) {
			GL.Fog( FogParameter.FogEnd, value );
		}
		
		FogMode[] fogModes = new FogMode[] { FogMode.Linear, FogMode.Exp, FogMode.Exp2 };
		public override void SetFogMode( Fog mode ) {
			GL.Fog( FogParameter.FogMode, (int)fogModes[(int)mode] );
		}
		
		public override void SetFogStart( float value ) {
			GL.Fog( FogParameter.FogStart, value );
		}
		
		public override bool FaceCulling {
			set { ToggleCap( EnableCap.CullFace, value ); }
		}
		
		
		#if TRACK_RESOURCES
		Dictionary<int, string> textures = new Dictionary<int, string>();
		#endif
		public override int LoadTexture( Bitmap bmp ) {
			Rectangle rec = new Rectangle( 0, 0, bmp.Width, bmp.Height );
			BitmapData data = bmp.LockBits( rec, ImageLockMode.ReadOnly, bmp.PixelFormat );
			int texId = LoadTexture( data.Width, data.Height, data.Scan0 );
			bmp.UnlockBits( data );
			return texId;
		}
		
		public override int LoadTexture( FastBitmap bmp ) {
			if( !bmp.IsLocked ) bmp.LockBits();
			int texId = LoadTexture( bmp.Width, bmp.Height, bmp.Scan0 );
			bmp.UnlockBits();
			return texId;
		}
		
		static int LoadTexture( int width, int height, IntPtr scan0 ) {
			if( !Utils.IsPowerOf2( width ) || !Utils.IsPowerOf2( height ) )
				Utils.LogWarning( "Creating a non power of two texture." );
			
			int texId = 0;
			GL.GenTextures( 1, out texId );
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
		
		public override void DeleteTexture( ref int texId ) {
			if( texId <= 0 ) return;
			#if TRACK_RESOURCES
			textures.Remove( texId );
			#endif
			GL.DeleteTextures( 1, ref texId );
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
				Color4 col4 = new Color4( col.R, col.G, col.B, col.A );
				GL.ClearColor( col4 );
				lastClearCol = col;
			}
		}
		
		public override void ColourMask( bool red, bool green, bool blue, bool alpha ) {
			GL.ColorMask( red, green, blue, alpha );
		}
		
		DepthFunction[] depthFuncs = new DepthFunction[] {
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
		
		public override void DrawVertices( DrawMode mode, VertexPos3fCol4b[] vertices, int count ) {
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
		
		public override void DrawVertices( DrawMode mode, VertexPos3fTex2f[] vertices, int count ) {
			GL.Begin( modeMappings[(int)mode] );
			for( int i = 0; i < count; i++ ) {
				VertexPos3fTex2f vertex = vertices[i];
				GL.TexCoord2( vertex.U, vertex.V );
				GL.Vertex3( vertex.X, vertex.Y, vertex.Z );
			}
			GL.End();
		}
		
		public override void DrawVertices( DrawMode mode, VertexPos3fTex2fCol4b[] vertices, int count ) {
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
		public override void SetFillType( FillType type ) {
			GL.PolygonMode( MaterialFace.FrontAndBack, fillModes[(int)type] );
		}
		
		#region Vertex buffers
		
		#if TRACK_RESOURCES
		Dictionary<int, string> vbs = new Dictionary<int, string>();
		#endif
		Action<DrawMode, int, int> drawBatchFunc;
		Action<DrawMode, int, int> drawBatchFuncTex2f;
		Action<DrawMode, int, int> drawBatchFuncCol4b;
		Action<DrawMode, int, int> drawBatchFuncTex2fCol4b;
		
		public override int InitVb<T>( T[] vertices, DrawMode mode, VertexFormat format, int count ) {
			if( !useVbos ) {
				return CreateDisplayList( vertices, mode, format, count );
			}
			int id = 0;
			GL.Arb.GenBuffers( 1, out id );
			int sizeInBytes = GetSizeInBytes( count, format );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.Arb.BufferData( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsageArb.StaticDraw );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			#if TRACK_RESOURCES
			vbs.Add( id, Environment.StackTrace );
			#endif
			return id;
		}
		
		public override IndexedVbInfo InitIndexedVb<T>( T[] vertices, ushort[] indices, DrawMode mode, 
		                              int verticesCount, int indicesCount ) {
			if( !useVbos ) {
				return CreateIndexedDisplayList( vertices, indices, mode, verticesCount, indicesCount );
			}
			IndexedVbInfo info = default( IndexedVbInfo );
			GL.Arb.GenBuffers( 2, out info.Vb );
			int sizeInBytes = GetSizeInBytes( verticesCount, VertexFormat.VertexPos3fTex2fCol4b );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, info.Vb );
			GL.Arb.BufferData( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsageArb.StaticDraw );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			
			sizeInBytes = indicesCount * sizeof( ushort );
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, info.Ib );
			GL.Arb.BufferData( BufferTargetArb.ElementArrayBuffer, new IntPtr( sizeInBytes ), indices, BufferUsageArb.StaticDraw );
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, 0 );
			return info;
		}
		
		IndexedVbInfo CreateIndexedDisplayList<T>( T[] vertices, ushort[] indices, DrawMode mode, 
		                              int verticesCount, int indicesCount ) where T : struct {
			GCHandle handle;
			int id = SetupDisplayListState( vertices, VertexFormat.VertexPos3fTex2fCol4b, out handle );
			GL.DrawElements( modeMappings[(int)mode], indicesCount, DrawElementsType.UnsignedShort, indices );
			RestoreDisplayListState( VertexFormat.VertexPos3fTex2fCol4b, ref handle );
			return new IndexedVbInfo( id, id );
		}
		
		int CreateDisplayList<T>( T[] vertices, DrawMode mode, VertexFormat format, int count ) where T : struct {
			GCHandle handle;
			int id = SetupDisplayListState( vertices, format, out handle );
			GL.DrawArrays( modeMappings[(int)mode], 0, count );
			RestoreDisplayListState( format, ref handle );
			#if TRACK_RESOURCES
			vbs.Add( id, Environment.StackTrace );
			#endif
			return id;
		}
		
		unsafe static int SetupDisplayListState<T>( T[] vertices, VertexFormat format, out GCHandle handle ) {
			int id = GL.GenLists( 1 );
			int stride = strideSizes[(int)format];
			GL.NewList( id, ListMode.Compile );
			GL.EnableClientState( ArrayCap.VertexArray );
			
			handle = GCHandle.Alloc( vertices, GCHandleType.Pinned );
			IntPtr p = handle.AddrOfPinnedObject();
			GL.VertexPointer( 3, VertexPointerType.Float, stride, (IntPtr)( 0 + (byte*)p ) );
			
			EnableClientState( format );
			if( format == VertexFormat.VertexPos3fCol4b ) {
				GL.ColorPointer( 4, ColorPointerType.UnsignedByte, stride, (IntPtr)( 12 + (byte*)p ) );
			} else if( format == VertexFormat.VertexPos3fTex2f ) {
				GL.TexCoordPointer( 2, TexCoordPointerType.Float, stride, (IntPtr)( 12 + (byte*)p ) );
			} else if( format == VertexFormat.VertexPos3fTex2fCol4b ) {
				GL.TexCoordPointer( 2, TexCoordPointerType.Float, stride, (IntPtr)( 12 + (byte*)p ) );
				GL.ColorPointer( 4, ColorPointerType.UnsignedByte, stride, (IntPtr)( 20 + (byte*)p ) );
			}
			return id;
		}
		
		static void RestoreDisplayListState( VertexFormat format, ref GCHandle handle ) {
			handle.Free();
			DisableClientState( format );
			GL.Color3( 1f, 1f, 1f ); // NOTE: not sure why, but display lists seem to otherwise modify global colour..
			GL.EndList();
		}
		
		public override void DeleteVb( int id ) {
			if( id <= 0 ) return;
			#if TRACK_RESOURCES
			vbs.Remove( id );
			#endif
			if( !useVbos ) {
				GL.DeleteLists( id, 1 );
				return;
			}
			GL.Arb.DeleteBuffers( 1, ref id );
		}
		
		public override void DeleteIndexedVb( IndexedVbInfo id ) {
			if( id.Vb <= 0 && id.Ib <= 0 ) return;
			if( !useVbos ) {
				GL.DeleteLists( id.Vb, 1 );
				return;
			}
			GL.Arb.DeleteBuffers( 2, ref id.Vb );
		}
		
		public override bool IsValidVb( int vb ) {
			return useVbos ? GL.Arb.IsBuffer( vb ) : GL.IsList( vb );
		}
		
		public override void DrawVbPos3fTex2f( DrawMode mode, int id, int verticesCount ) {
			BeginVbBatch( VertexFormat.VertexPos3fTex2f );
			DrawVbBatch( mode, id, verticesCount );
			EndVbBatch();
		}
		
		public override void DrawVbPos3fCol4b( DrawMode mode, int id, int verticesCount ) {
			BeginVbBatch( VertexFormat.VertexPos3fCol4b );
			DrawVbBatch( mode, id, verticesCount );
			EndVbBatch();
		}
		
		public override void DrawVbPos3fTex2fCol4b( DrawMode mode, int id, int verticesCount ) {
			BeginVbBatch( VertexFormat.VertexPos3fTex2fCol4b );
			DrawVbBatch( mode, id, verticesCount );
			EndVbBatch();
		}
		
		VertexFormat batchFormat;
		public override void BeginVbBatch( VertexFormat format ) {
			if( !useVbos ) return;
			batchFormat = format;
			EnableClientState( format );
			if( format == VertexFormat.VertexPos3fTex2fCol4b ) {
				drawBatchFunc = drawBatchFuncTex2fCol4b;
			} else if( format == VertexFormat.VertexPos3fCol4b ) {
				drawBatchFunc =drawBatchFuncCol4b;
			} else if( format == VertexFormat.VertexPos3fTex2f ) {
				drawBatchFunc = drawBatchFuncTex2f;
			}
		}
		
		public override void BeginIndexedVbBatch() {
			if( !useVbos ) return;
			EnableClientState( VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		public override void DrawVbBatch( DrawMode mode, int id, int verticesCount ) {
			if( !useVbos ) {
				GL.CallList( id );
				return;
			}
			drawBatchFunc( mode, id, verticesCount );
		}
		
		public override void DrawIndexedVbBatch( DrawMode mode, IndexedVbInfo id, int indicesCount ) {
			if( !useVbos ) {
				GL.CallList( id.Vb );
				return;
			}
			
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id.Vb );
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, id.Ib );
			GL.VertexPointer( 3, VertexPointerType.Float, 24, new IntPtr( 0 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 24, new IntPtr( 12 ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 24, new IntPtr( 20 ) );
			GL.DrawElements( modeMappings[(int)mode], indicesCount, DrawElementsType.UnsignedShort, 0 );
		}
		
		public override void EndVbBatch() {
			if( !useVbos ) return;
			DisableClientState( batchFormat );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
		}
		
		public override void EndIndexedVbBatch() {
			if( !useVbos ) return;
			DisableClientState( VertexFormat.VertexPos3fTex2fCol4b );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			GL.Arb.BindBuffer( BufferTargetArb.ElementArrayBuffer, 0 );
		}
		
		void DrawVbPos3fTex2fFast( DrawMode mode, int id, int verticesCount ) {
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 20, new IntPtr( 0 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 20, new IntPtr( 12 ) );
			GL.DrawArrays( modeMappings[(int)mode], 0, verticesCount );
		}
		
		void DrawVbPos3fCol4bFast( DrawMode mode, int id, int verticesCount ) {
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 16, new IntPtr( 0 ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 16, new IntPtr( 12 ) );
			GL.DrawArrays( modeMappings[(int)mode], 0, verticesCount );
		}
		
		void DrawVbPos3fTex2fCol4bFast( DrawMode mode, int id, int verticesCount ) {
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 24, new IntPtr( 0 ) );
			GL.TexCoordPointer( 2, TexCoordPointerType.Float, 24, new IntPtr( 12 ) );
			GL.ColorPointer( 4, ColorPointerType.UnsignedByte, 24, new IntPtr( 20 ) );
			GL.DrawArrays( modeMappings[(int)mode], 0, verticesCount );
		}
		
		static void EnableClientState( VertexFormat format ) {
			GL.EnableClientState( ArrayCap.VertexArray );
			if( format == VertexFormat.VertexPos3fCol4b ) {
				GL.EnableClientState( ArrayCap.ColorArray );
			} else if( format == VertexFormat.VertexPos3fTex2f ) {
				GL.EnableClientState( ArrayCap.TextureCoordArray );
			} else if( format == VertexFormat.VertexPos3fTex2fCol4b ) {
				GL.EnableClientState( ArrayCap.ColorArray );
				GL.EnableClientState( ArrayCap.TextureCoordArray );
			}
		}
		
		static void DisableClientState( VertexFormat format ) {
			GL.DisableClientState( ArrayCap.VertexArray );
			if( format == VertexFormat.VertexPos3fCol4b ) {
				GL.DisableClientState( ArrayCap.ColorArray );
			} else if( format == VertexFormat.VertexPos3fTex2f ) {
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			} else if( format == VertexFormat.VertexPos3fTex2fCol4b ) {
				GL.DisableClientState( ArrayCap.ColorArray );
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			}
		}
		#endregion
		
		
		#region Matrix manipulation

		MatrixMode lastMode = 0;
		MatrixMode[] matrixModes = new MatrixMode[] {
			MatrixMode.Projection, MatrixMode.Modelview, MatrixMode.Texture,
		};
		public override void SetMatrixMode( MatrixType mode ) {
			MatrixMode glMode = matrixModes[(int)mode];
			if( glMode != lastMode ) {
				GL.MatrixMode( glMode );
				lastMode = glMode;
			}
		}
		
		public override void LoadMatrix( ref Matrix4 matrix ) {
			GL.LoadMatrix( ref matrix );
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
		
		public override void MultiplyMatrix( ref Matrix4 matrix ) {
			GL.MultMatrix( ref matrix );
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
		
		public override void PrintApiSpecificInfo() {
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