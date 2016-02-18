#if !USE_DX && !ANDROID
using System;
using System.Drawing;
using System.Drawing.Imaging;
using OpenTK;
using OpenTK.Graphics.OpenGL;
using BmpPixelFormat = System.Drawing.Imaging.PixelFormat;
using GlPixelFormat = OpenTK.Graphics.OpenGL.PixelFormat;

namespace ClassicalSharp.GraphicsAPI {

	/// <summary> Implements IGraphicsAPI using OpenGL 1.5,
	/// or 1.2 with the GL_ARB_vertex_buffer_object extension. </summary>
	public unsafe class OpenGLApi : IGraphicsApi {
		
		BeginMode[] modeMappings;
		public OpenGLApi() {
			InitFields();
			int texDims;
			GL.GetIntegerv( GetPName.MaxTextureSize, &texDims );
			texDimensions = texDims;
			CheckVboSupport();
			base.InitDynamicBuffers();
			
			setupBatchFuncCol4b = SetupVbPos3fCol4b;
			setupBatchFuncTex2fCol4b = SetupVbPos3fTex2fCol4b;
			GL.EnableClientState( ArrayCap.VertexArray );
			GL.EnableClientState( ArrayCap.ColorArray );
		}
		
		void CheckVboSupport() {
			string extensions = new String( (sbyte*)GL.GetString( StringName.Extensions ) );
			string version = new String( (sbyte*)GL.GetString( StringName.Version ) );
			int major = (int)(version[0] - '0'); // x.y. (and so forth)
			int minor = (int)(version[2] - '0');
			if( (major > 1) || (major == 1 && minor >= 5) ) return; // Supported in core since 1.5
			
			Utils.LogDebug( "Using ARB vertex buffer objects" );
			if( !extensions.Contains( "GL_ARB_vertex_buffer_object" ) ) {
				ErrorHandler.LogError( "OpenGL VBO support check",
				                      "Driver does not support OpenGL VBOs, which are required for the OpenGL build." +
				                      Environment.NewLine + "You may need to install and/or update video card drivers." +
				                      Environment.NewLine + "Alternatively, you can download the Direct3D 9 build." );
				throw new InvalidOperationException( "VBO support required for OpenGL build" );
			}
			GL.UseArbVboAddresses();
		}

		public override bool AlphaTest { set { Toggle( EnableCap.AlphaTest, value ); } }
		
		public override bool AlphaBlending { set { Toggle( EnableCap.Blend, value ); } }
		
		Compare[] compareFuncs;
		public override void AlphaTestFunc( CompareFunc func, float value ) {
			GL.AlphaFunc( compareFuncs[(int)func], value );
		}
		
		BlendingFactor[] blendFuncs;
		public override void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc dstFunc ) {
			GL.BlendFunc( blendFuncs[(int)srcFunc], blendFuncs[(int)dstFunc] );
		}
		
		public override bool Fog { set { Toggle( EnableCap.Fog, value ); } }
		
		FastColour lastFogCol = FastColour.Black;
		public override void SetFogColour( FastColour col ) {
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
		FogMode[] fogModes;
		public override void SetFogMode( Fog mode ) {
			if( mode != lastFogMode ) {
				GL.Fogi( FogParameter.FogMode, (int)fogModes[(int)mode] );
				lastFogMode = mode;
			}
		}
		
		public override bool FaceCulling {
			set {
				if( value ) GL.Enable( EnableCap.CullFace );
				else GL.Disable( EnableCap.CullFace );
			}
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
		
		public override bool ColourWrite { set { GL.ColorMask( value, value, value, value ); } }
		
		public override void DepthTestFunc( CompareFunc func ) {
			GL.DepthFunc( compareFuncs[(int)func] );
		}
		
		public override bool DepthTest { set { Toggle( EnableCap.DepthTest, value ); } }
		
		public override bool DepthWrite { set { GL.DepthMask( value ); } }
		
		public override bool AlphaArgBlend { set { } }
		
		#region Texturing
		
		int texDimensions;
		public override int MaxTextureDimensions { get { return texDimensions; } }
		
		public override bool Texturing { set { Toggle( EnableCap.Texture2D, value ); } }
		
		public override int CreateTexture( int width, int height, IntPtr scan0 ) {
			if( !Utils.IsPowerOf2( width ) || !Utils.IsPowerOf2( height ) )
				Utils.LogDebug( "Creating a non power of two texture." );
			
			int texId = 0;
			GL.GenTextures( 1, &texId );
			GL.BindTexture( TextureTarget.Texture2D, texId );
			GL.TexParameteri( TextureTarget.Texture2D, TextureParameterName.MinFilter, (int)TextureFilter.Nearest );
			GL.TexParameteri( TextureTarget.Texture2D, TextureParameterName.MagFilter, (int)TextureFilter.Nearest );

			GL.TexImage2D( TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, width, height,
			              GlPixelFormat.Bgra, PixelType.UnsignedByte, scan0 );
			return texId;
		}
		
		public override void BindTexture( int texture ) {
			GL.BindTexture( TextureTarget.Texture2D, texture );
		}
		
		public override void UpdateTexturePart( int texId, int texX, int texY, FastBitmap part ) {
			GL.BindTexture( TextureTarget.Texture2D, texId );
			GL.TexSubImage2D( TextureTarget.Texture2D, 0, texX, texY, part.Width, part.Height,
			                 GlPixelFormat.Bgra, PixelType.UnsignedByte, part.Scan0 );
		}
		
		public override void DeleteTexture( ref int texId ) {
			if( texId <= 0 ) return;
			int id = texId;
			GL.DeleteTextures( 1, &id );
			texId = -1;
		}
		#endregion
		
		#region Vertex/index buffers
		Action setupBatchFunc, setupBatchFuncCol4b, setupBatchFuncTex2fCol4b;
		
		public override int CreateDynamicVb( VertexFormat format, int maxVertices ) {
			int id = GenAndBind( BufferTarget.ArrayBuffer );
			int sizeInBytes = maxVertices * strideSizes[(int)format];
			GL.BufferData( BufferTarget.ArrayBuffer, new IntPtr( sizeInBytes ), IntPtr.Zero, BufferUsage.DynamicDraw );
			return id;
		}
		
		public override int CreateVb<T>( T[] vertices, VertexFormat format, int count ) {
			int id = GenAndBind( BufferTarget.ArrayBuffer );
			int sizeInBytes = count * strideSizes[(int)format];
			GL.BufferData( BufferTarget.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsage.StaticDraw );
			return id;
		}
		
		public override int CreateVb( IntPtr vertices, VertexFormat format, int count ) {
			int id = GenAndBind( BufferTarget.ArrayBuffer );
			int sizeInBytes = count * strideSizes[(int)format];
			GL.BufferData( BufferTarget.ArrayBuffer, new IntPtr( sizeInBytes ), vertices, BufferUsage.StaticDraw );
			return id;
		}
		
		public override int CreateIb( ushort[] indices, int indicesCount ) {
			int id = GenAndBind( BufferTarget.ElementArrayBuffer );
			int sizeInBytes = indicesCount * sizeof( ushort );
			GL.BufferData( BufferTarget.ElementArrayBuffer, new IntPtr( sizeInBytes ), indices, BufferUsage.StaticDraw );
			return id;
		}
		
		public override int CreateIb( IntPtr indices, int indicesCount ) {
			int id = GenAndBind( BufferTarget.ElementArrayBuffer );
			int sizeInBytes = indicesCount * sizeof( ushort );
			GL.BufferData( BufferTarget.ElementArrayBuffer, new IntPtr( sizeInBytes ), indices, BufferUsage.StaticDraw );
			return id;
		}
		
		static int GenAndBind( BufferTarget target ) {
			int id = 0;
			GL.GenBuffers( 1, &id );
			GL.BindBuffer( target, id );
			return id;
		}
		
		int batchStride;
		public override void UpdateDynamicVb<T>( DrawMode mode, int id, T[] vertices, int count ) {
			GL.BindBuffer( BufferTarget.ArrayBuffer, id );
			GL.BufferSubData( BufferTarget.ArrayBuffer, IntPtr.Zero, new IntPtr( count * batchStride ), vertices );
			
			setupBatchFunc();
			GL.DrawArrays( modeMappings[(int)mode], 0, count );
		}
		
		public override void UpdateDynamicIndexedVb<T>( DrawMode mode, int id, T[] vertices, int vCount, int indicesCount ) {
			GL.BindBuffer( BufferTarget.ArrayBuffer, id );
			GL.BufferSubData( BufferTarget.ArrayBuffer, IntPtr.Zero, new IntPtr( vCount * batchStride ), vertices );
			
			setupBatchFunc();
			GL.DrawElements( modeMappings[(int)mode], indicesCount, indexType, zero );
		}
		
		public override void SetDynamicVbData<T>( DrawMode mode, int id, T[] vertices, int count ) {
			GL.BindBuffer( BufferTarget.ArrayBuffer, id );
			GL.BufferSubData( BufferTarget.ArrayBuffer, IntPtr.Zero, new IntPtr( count * batchStride ), vertices );
		}
		
		public override void DeleteDynamicVb( int id ) {
			if( id <= 0 ) return;
			GL.DeleteBuffers( 1, &id );
		}
		
		public override void DeleteVb( int vb ) {
			if( vb <= 0 ) return;
			GL.DeleteBuffers( 1, &vb );
		}
		
		public override void DeleteIb( int ib ) {
			if( ib <= 0 ) return;
			GL.DeleteBuffers( 1, &ib );
		}
		
		VertexFormat batchFormat = (VertexFormat)999;
		public override void SetBatchFormat( VertexFormat format ) {
			if( format == batchFormat ) return;
			
			if( batchFormat == VertexFormat.Pos3fTex2fCol4b ) {
				GL.DisableClientState( ArrayCap.TextureCoordArray );
			}
			
			batchFormat = format;
			if( format == VertexFormat.Pos3fTex2fCol4b ) {
				GL.EnableClientState( ArrayCap.TextureCoordArray );
				setupBatchFunc = setupBatchFuncTex2fCol4b;
				batchStride = VertexPos3fTex2fCol4b.Size;
			} else {
				setupBatchFunc = setupBatchFuncCol4b;
				batchStride = VertexPos3fCol4b.Size;
			}
		}
		
		public override void BindVb( int vb ) {
			GL.BindBuffer( BufferTarget.ArrayBuffer, vb );
		}
		
		public override void BindIb( int ib ) {
			GL.BindBuffer( BufferTarget.ElementArrayBuffer, ib );
		}
		
		const DrawElementsType indexType = DrawElementsType.UnsignedShort;
		public override void DrawVb( DrawMode mode, int startVertex, int verticesCount ) {
			setupBatchFunc();
			GL.DrawArrays( modeMappings[(int)mode], startVertex, verticesCount );
		}		
		
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
		MatrixMode[] matrixModes;
		public override void SetMatrixMode( MatrixType mode ) {
			MatrixMode glMode = matrixModes[(int)mode];
			if( glMode != lastMode ) {
				GL.MatrixMode( glMode );
				lastMode = glMode;
			}
		}
		
		public override void LoadMatrix( ref Matrix4 matrix ) {
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
		
		public override void MultiplyMatrix( ref Matrix4 matrix ) {
			fixed( Single* ptr = &matrix.Row0.X )
				GL.MultMatrixf( ptr );
		}
		
		#endregion
		
		public override void BeginFrame( Game game ) {
		}
		
		public override void EndFrame( Game game ) {
			game.window.SwapBuffers();
		}
		
		public override void SetVSync( Game game, bool value ) {
			game.VSync = value;
		}
		
		bool isIntelRenderer;
		protected override void MakeApiInfo() {
			string vendor = new String( (sbyte*)GL.GetString( StringName.Vendor ) );
			string renderer = new String( (sbyte*)GL.GetString( StringName.Renderer ) );
			string version = new String( (sbyte*)GL.GetString( StringName.Version ) );
			int depthBits = 0;
			GL.GetIntegerv( GetPName.DepthBits, &depthBits );
			
			ApiInfo = new string[] {
				"--Using OpenGL api--",
				"Vendor: " + vendor,
				"Renderer: " + renderer,
				"GL version: " + version,
				"Max 2D texture dimensions: " + MaxTextureDimensions,
				"Depth buffer bits: " + depthBits,
			};
			isIntelRenderer = renderer.Contains( "Intel" );
		}
		
		public override void WarnIfNecessary( Chat chat ) {
			if( !isIntelRenderer ) return;
			
			chat.Add( "&cIntel graphics cards are known to have issues with the OpenGL build." );
			chat.Add( "&cVSync may not work, and you may see disappearing clouds and map edges." );
			chat.Add( "    " );
			chat.Add( "&cFor Windows, try downloading the Direct3D 9 build as it doesn't have" );
			chat.Add( "&cthese problems. Alternatively, the disappearing graphics can be" );
			chat.Add( "&cpartially fixed by typing \"/client render legacy\" into chat." );
		}
		
		// Based on http://www.opentk.com/doc/graphics/save-opengl-rendering-to-disk
		public override void TakeScreenshot( string output, int width, int height ) {
			using( Bitmap bmp = new Bitmap( width, height, BmpPixelFormat.Format32bppRgb ) ) { // ignore alpha component
				using( FastBitmap fastBmp = new FastBitmap( bmp, true ) )
					GL.ReadPixels( 0, 0, width, height, GlPixelFormat.Bgra, PixelType.UnsignedByte, fastBmp.Scan0 );
				bmp.RotateFlip( RotateFlipType.RotateNoneFlipY );
				bmp.Save( output, ImageFormat.Png );
			}
		}
		
		public override void OnWindowResize( Game game ) {
			GL.Viewport( 0, 0, game.Width, game.Height );
		}
		
		static void Toggle( EnableCap cap, bool value ) {
			if( value ) GL.Enable( cap );
			else GL.Disable( cap );
		}
		
		void InitFields() {
			// See comment in Game() constructor for why this is necessary.			
			blendFuncs = new BlendingFactor[6];
			blendFuncs[0] = BlendingFactor.Zero; blendFuncs[1] = BlendingFactor.One;
			blendFuncs[2] = BlendingFactor.SrcAlpha; blendFuncs[3] = BlendingFactor.OneMinusSrcAlpha;
			blendFuncs[4] = BlendingFactor.DstAlpha; blendFuncs[5] = BlendingFactor.OneMinusDstAlpha;
			compareFuncs = new Compare[8];
			compareFuncs[0] = Compare.Always; compareFuncs[1] = Compare.Notequal;
			compareFuncs[2] = Compare.Never; compareFuncs[3] = Compare.Less;
			compareFuncs[4] = Compare.Lequal; compareFuncs[5] = Compare.Equal;
			compareFuncs[6] = Compare.Gequal; compareFuncs[7] = Compare.Greater;
			
			modeMappings = new BeginMode[2];
			modeMappings[0] = BeginMode.Triangles; modeMappings[1] = BeginMode.Lines;
			fogModes = new FogMode[3];
			fogModes[0] = FogMode.Linear; fogModes[1] = FogMode.Exp;
			fogModes[2] = FogMode.Exp2;
			matrixModes = new MatrixMode[3];
			matrixModes[0] = MatrixMode.Projection; matrixModes[1] = MatrixMode.Modelview;
			matrixModes[2] = MatrixMode.Texture;
		}
	}
}
#endif