//#define TRACK_RESOURCES
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL;
using BmpPixelFormat = System.Drawing.Imaging.PixelFormat;
using GlPixelFormat = OpenTK.Graphics.OpenGL.PixelFormat;

namespace ClassicalSharp.GraphicsAPI {

	public class OpenGLApi : IGraphicsApi {
		
		int textureDimensions;
		bool nonPow2;
		const string nonPow2Ext = "GL_ARB_texture_non_power_of_two";
		const string vboExt1 = "GL_ARB_vertex_buffer_object",
		vboExt2 = "GLX_ARB_vertex_buffer_object";
		bool useVbos = true;
		BeginMode[] modeMappings = new BeginMode[] {
			BeginMode.Triangles, BeginMode.Lines, BeginMode.Points,
			BeginMode.TriangleStrip, BeginMode.LineStrip, BeginMode.TriangleFan
		};
		
		public OpenGLApi() {
			GL.GetInteger( GetPName.MaxTextureSize, out textureDimensions );
			string extensions = GL.GetString( StringName.Extensions );
			nonPow2 = extensions.Contains( nonPow2Ext );
			useVbos = extensions.Contains( vboExt1 ) || extensions.Contains( vboExt2 );
			if( useVbos ) {
				SetupVb();
			} else {
				Utils.LogWarning( "Unable to use OpenGL VBOs, you may experience reduced performance." );
			}
		}

		public override int MaxTextureDimensions {
			get { return textureDimensions; }
		}
		
		public override bool SupportsNonPowerOf2Textures {
			get { return nonPow2; }
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
		public override void AlphaTestFunc( AlphaFunc func, float value ) {
			GL.AlphaFunc( alphaFuncs[(int)func], value );
		}
		
		BlendEquationMode[] blendEqs = new BlendEquationMode[] {
			BlendEquationMode.FuncAdd, BlendEquationMode.Max,
			BlendEquationMode.Min, BlendEquationMode.FuncSubtract,
			BlendEquationMode.FuncReverseSubtract,
		};
		public override void AlphaBlendEq( BlendEquation eq ) {
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
		public override void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc destFunc ) {
			GL.BlendFunc( srcBlendFuncs[(int)srcFunc], destBlendFuncs[(int)destFunc] );
		}
		
		public override bool Fog {
			set { ToggleCap( EnableCap.Fog, value ); }
		}
		
		public override void SetFogColour( FastColour col ) {
			float[] colRGBA = { col.R / 255f, col.G / 255f, col.B / 255f, col.A / 255f };
			GL.Fog( FogParameter.FogColor, colRGBA );
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
		
		
		#if TRACK_RESOURCES
		Dictionary<int, string> textures = new Dictionary<int, string>();
		#endif
		public override int LoadTexture( Bitmap bmp ) {
			using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
				return LoadTexture( fastBmp );
			}
		}
		
		public override int LoadTexture( FastBitmap bmp ) {
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
		
		public override void Bind2DTexture( int texture ) {
			GL.BindTexture( TextureTarget.Texture2D, texture );
		}
		
		public override void DeleteTexture( int texId ) {
			if( texId <= 0 ) return;
			#if TRACK_RESOURCES
			textures.Remove( texId );
			#endif
			GL.DeleteTexture( texId );
		}
		
		public override bool Texturing {
			set { ToggleCap( EnableCap.Texture2D, value ); }
		}
		
		// Based on http://www.opentk.com/doc/graphics/save-opengl-rendering-to-disk
		public static void TakeScreenshot( Size size, string output, ImageFormat format ) {
			using( Bitmap bmp = new Bitmap( size.Width, size.Height, BmpPixelFormat.Format32bppRgb ) ) { // ignoring alpha componen
				using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
					GL.ReadPixels( 0, 0, size.Width, size.Height, GlPixelFormat.Bgra, PixelType.UnsignedByte, fastBmp.Scan0 );
				}
				bmp.RotateFlip( RotateFlipType.RotateNoneFlipY );
				bmp.Save( output, format );
			}
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
		public override void DepthTestFunc( DepthFunc func ) {
			GL.DepthFunc( depthFuncs[(int)func] );
		}
		
		public override bool DepthTest {
			set { ToggleCap( EnableCap.DepthTest, value ); }
		}
		
		public override bool DepthWrite {
			set { GL.DepthMask( value ); }
		}
		
		
		public override void DrawVertices( DrawMode mode, Vector3[] vertices ) {
			//GL.DrawArrays( BeginMode.Triangles, 0, vertices.Length );
			// We can't just use GL.DrawArrays since we'd have to pin the array to prevent it from being moved around in memory.
			// Feasible alternatives:
			// - Use a dynamically updated VBO, and resize it (i.e. create a new bigger VBO) if required.
			// - Immediate mode.
			GL.Begin( modeMappings[(int)mode] );
			for( int i = 0; i < vertices.Length; i++ ) {
				GL.Vertex3( vertices[i] );
			}
			GL.End();
		}
		
		public override void DrawVertices( DrawMode mode, VertexPos3fCol4b[] vertices ) {
			GL.Begin( modeMappings[(int)mode] );
			for( int i = 0; i < vertices.Length; i++ ) {
				VertexPos3fCol4b vertex = vertices[i];
				GL.Color4( vertex.R, vertex.G, vertex.B, vertex.A );
				GL.Vertex3( vertex.X, vertex.Y, vertex.Z );
			}
			GL.Color4( 1f, 1f, 1f, 1f );
			GL.End();
		}
		
		public override void DrawVertices( DrawMode mode, VertexPos3fTex2f[] vertices ) {
			GL.Begin( modeMappings[(int)mode] );
			for( int i = 0; i < vertices.Length; i++ ) {
				VertexPos3fTex2f vertex = vertices[i];
				GL.TexCoord2( vertex.U, vertex.V );
				GL.Vertex3( vertex.X, vertex.Y, vertex.Z );
			}
			GL.End();
		}
		
		public override void DrawVertices( DrawMode mode, VertexPos3fTex2fCol4b[] vertices ) {
			GL.Begin( modeMappings[(int)mode] );
			for( int i = 0; i < vertices.Length; i++ ) {
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
		
		public override bool AmbientLighting {
			set {
				if( value ) {
					GL.Enable( EnableCap.Lighting );
					GL.Enable( EnableCap.ColorMaterial );
					GL.ColorMaterial( MaterialFace.FrontAndBack, ColorMaterialParameter.Ambient );
				} else {
					GL.Disable( EnableCap.Lighting );
					GL.Disable( EnableCap.ColorMaterial );
				}
			}
		}
		
		public override void SetAmbientColour( FastColour col ) {
			float[] colRGBA = { col.R / 255f, col.G / 255f, col.B / 255f, 1f };
			GL.LightModel( LightModelParameter.LightModelAmbient, colRGBA );
		}
		
		#region Vertex buffers
		
		#if TRACK_RESOURCES
		Dictionary<int, string> vbs = new Dictionary<int, string>();
		#endif
		Action<DrawMode, int, int>[] drawBatchFuncs;
		Action<DrawMode, int, int> drawBatchFunc;
		void SetupVb() {
			drawBatchFuncs = new Action<DrawMode, int, int>[4];
			drawBatchFuncs[0] = (mode, id, count) => DrawVbPos3fFast( mode, id, count );
			drawBatchFuncs[1] = (mode, id, count) => DrawVbPos3fTex2fFast( mode, id, count );
			drawBatchFuncs[2] = (mode, id, count) => DrawVbPos3fCol4bFast( mode, id, count );
			drawBatchFuncs[3] = (mode, id, count) => DrawVbPos3fTex2fCol4bFast( mode, id, count );
		}
		
		int GenBufferId() {
			int[] ids = new int[1];
			GL.Arb.GenBuffers( 1, ids );
			return ids[0];
		}
		
		public override int InitVb<T>( T[] vertices, DrawMode mode, VertexFormat format, int count ) {
			if( !useVbos ) {
				return CreateDisplayList( vertices, mode, format, count );
			}
			int id = GenBufferId();
			int sizeInBytes = GetSizeInBytes( count, format );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.Arb.BufferData( BufferTargetArb.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsageArb.StaticDraw );
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			#if TRACK_RESOURCES
			vbs.Add( id, Environment.StackTrace );
			#endif
			return id;
		}
		
		// Okay this isn't exactly the prettiest code ever.
		unsafe int CreateDisplayList<T>( T[] vertices, DrawMode mode, VertexFormat format, int count ) where T : struct {
			int id = GL.GenLists( 1 );
			int stride = strideSizes[(int)format];
			GL.NewList( id, ListMode.Compile );
			GL.EnableClientState( ArrayCap.VertexArray );
			
			if( format == VertexFormat.VertexPos3f ) {
				fixed( Vector3* p = (vertices as Vector3[]) ) {
					GL.VertexPointer( 3, VertexPointerType.Float, stride, (IntPtr)( 0 + (byte*)p ) );
					GL.DrawArrays( BeginMode.Triangles, 0, count );
				}
			} else if( format == VertexFormat.VertexPos3fCol4b ) {
				GL.EnableClientState( ArrayCap.ColorArray );
				fixed( VertexPos3fCol4b* p = (vertices as VertexPos3fCol4b[]) ) {
					GL.VertexPointer( 3, VertexPointerType.Float, stride, (IntPtr)( 0 + (byte*)p ) );
					GL.ColorPointer( 4, ColorPointerType.UnsignedByte, stride, (IntPtr)( 12 + (byte*)p ) );
					GL.DrawArrays( BeginMode.Triangles, 0, count );
				}
			} else if( format == VertexFormat.VertexPos3fTex2f ) {
				GL.EnableClientState( ArrayCap.TextureCoordArray );
				fixed( VertexPos3fTex2f* p = (vertices as VertexPos3fTex2f[]) ) {
					GL.VertexPointer( 3, VertexPointerType.Float, stride, (IntPtr)( 0 + (byte*)p) );
					GL.TexCoordPointer( 2, TexCoordPointerType.Float, stride, (IntPtr)( 12 + (byte*)p ) );
					GL.DrawArrays( BeginMode.Triangles, 0, count );
				}
			} else if( format == VertexFormat.VertexPos3fTex2fCol4b ) {
				GL.EnableClientState( ArrayCap.ColorArray );
				GL.EnableClientState( ArrayCap.TextureCoordArray );
				fixed( VertexPos3fTex2fCol4b* p = (vertices as VertexPos3fTex2fCol4b[]) ) {
					GL.VertexPointer( 3, VertexPointerType.Float, stride, (IntPtr)( 0 + (byte*)p ) );
					GL.TexCoordPointer( 2, TexCoordPointerType.Float, stride, (IntPtr)( 12 + (byte*)p ) );
					GL.ColorPointer( 4, ColorPointerType.UnsignedByte, stride, (IntPtr)( 20 + (byte*)p ) );
					GL.DrawArrays( BeginMode.Triangles, 0, count );
				}
			}
			
			GL.DisableClientState( ArrayCap.VertexArray );
			if( format == VertexFormat.VertexPos3fCol4b ) {
				GL.DisableClientState( ArrayCap.ColorArray );
			} else if( format == VertexFormat.VertexPos3fTex2f ) {
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			} else if( format == VertexFormat.VertexPos3fTex2fCol4b ) {
				GL.DisableClientState( ArrayCap.ColorArray );
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			}
			GL.Color3( 1f, 1f, 1f ); // NOTE: not sure why, but display lists seem to otherwise modify global colour..
			GL.EndList();
			#if TRACK_RESOURCES
			vbs.Add( id, Environment.StackTrace );
			#endif
			return id;
		}
		
		public override void DeleteVb( int id ) {
			#if TRACK_RESOURCES
			vbs.Remove( id );
			#endif
			if( !useVbos ) {
				GL.DeleteLists( id, 1 );
				return;
			}
			GL.DeleteBuffers( 1, new [] { id } );
		}
		
		public override void DrawVbPos3f( DrawMode mode, int id, int verticesCount ) {
			if( !useVbos ) {
				GL.CallList( id );
				return;
			}
			GL.EnableClientState( ArrayCap.VertexArray );
			
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 12, new IntPtr( 0 ) );
			GL.DrawArrays( modeMappings[(int)mode], 0, verticesCount );
			
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, 0 );
			GL.DisableClientState( ArrayCap.VertexArray );
		}
		
		public override void DrawVbPos3fTex2f( DrawMode mode, int id, int verticesCount ) {
			if( !useVbos ) {
				GL.CallList( id );
				return;
			}
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
		
		public override void DrawVbPos3fCol4b( DrawMode mode, int id, int verticesCount ) {
			if( !useVbos ) {
				GL.CallList( id );
				return;
			}
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
		
		public override void DrawVbPos3fTex2fCol4b( DrawMode mode, int id, int verticesCount ) {
			if( !useVbos ) {
				GL.CallList( id );
				return;
			}
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
		
		VertexFormat batchFormat;
		public override void BeginVbBatch( VertexFormat format ) {
			if( !useVbos ) {
				return;
			}
			batchFormat = format;
			GL.EnableClientState( ArrayCap.VertexArray );
			if( batchFormat == VertexFormat.VertexPos3fCol4b ) {
				GL.EnableClientState( ArrayCap.ColorArray );
			} else if( batchFormat == VertexFormat.VertexPos3fTex2f ) {
				GL.EnableClientState( ArrayCap.TextureCoordArray );
			} else if( batchFormat == VertexFormat.VertexPos3fTex2fCol4b ) {
				GL.EnableClientState( ArrayCap.ColorArray );
				GL.EnableClientState( ArrayCap.TextureCoordArray );
			}
			drawBatchFunc = drawBatchFuncs[(int)batchFormat];
		}
		
		public override void DrawVbBatch( DrawMode mode, int id, int verticesCount ) {
			if( !useVbos ) {
				GL.CallList( id );
				return;
			}
			drawBatchFunc( mode, id, verticesCount );
		}
		
		public override void EndVbBatch() {
			if( !useVbos ) {
				return;
			}
			GL.DisableClientState( ArrayCap.VertexArray );
			if( batchFormat == VertexFormat.VertexPos3fCol4b ) {
				GL.DisableClientState( ArrayCap.ColorArray );
			} else if( batchFormat == VertexFormat.VertexPos3fTex2f ) {
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			} else if( batchFormat == VertexFormat.VertexPos3fTex2fCol4b ) {
				GL.DisableClientState( ArrayCap.ColorArray );
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			}
		}
		
		void DrawVbPos3fFast( DrawMode mode, int id, int verticesCount ) {
			GL.Arb.BindBuffer( BufferTargetArb.ArrayBuffer, id );
			GL.VertexPointer( 3, VertexPointerType.Float, 12, new IntPtr( 0 ) );
			GL.DrawArrays( modeMappings[(int)mode], 0, verticesCount );
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