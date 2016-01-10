using System;
using System.Drawing;
using System.Drawing.Imaging;
using OpenTK;

namespace ClassicalSharp.GraphicsAPI {
	
	/// <summary> Abstracts a 3D graphics rendering API. </summary>
	public abstract class IGraphicsApi {
		
		/// <summary> Maximum supported length of a dimension (width and height) of a 2D texture. </summary>
		public abstract int MaxTextureDimensions { get; }
		
		/// <summary> Sets whether texturing is applied when rasterizing primitives. </summary>
		public abstract bool Texturing { set; }
		
		internal float MinZNear = 0.1f;
		
		/// <summary> Creates a new native texture with the specified dimensions and using the
		/// image data encapsulated by the Bitmap instance. </summary>
		/// <remarks> Note that should make every effort you can to ensure that the dimensions of the bitmap
		/// are powers of two, because otherwise they will not display properly on certain graphics cards.	<br/>
		/// This method returns -1 if the input image is not a 32bpp format. </remarks>
		public int CreateTexture( Bitmap bmp ) {
			if( !FastBitmap.CheckFormat( bmp.PixelFormat ) ) {
				string line2 = String.Format( "input bitmap was not 32bpp, it was {0}",
				                             bmp.PixelFormat );
				ErrorHandler.LogError( "IGraphicsApi.CreateTexture()",
				                      Environment.NewLine + line2 +
				                      Environment.NewLine + Environment.StackTrace );
				return -1;
			} else {
				Rectangle rec = new Rectangle( 0, 0, bmp.Width, bmp.Height );
				BitmapData data = bmp.LockBits( rec, ImageLockMode.ReadOnly, bmp.PixelFormat );
				int texId = CreateTexture( data.Width, data.Height, data.Scan0 );
				bmp.UnlockBits( data );
				return texId;
			}
		}
		
		/// <summary> Creates a new native texture with the specified dimensions and FastBitmap instance
		/// that encapsulates the pointer to the 32bpp image data.</summary>
		/// <remarks> Note that should make every effort you can to ensure that the dimensions are powers of two,
		/// because otherwise they will not display properly on certain graphics cards.	</remarks>
		public int CreateTexture( FastBitmap bmp ) {
			if( !bmp.IsLocked )
				bmp.LockBits();
			int texId = CreateTexture( bmp.Width, bmp.Height, bmp.Scan0 );
			bmp.UnlockBits();
			return texId;
		}
		
		/// <summary> Creates a new native texture with the specified dimensions and pointer to the 32bpp image data. </summary>
		/// <remarks> Note that should make every effort you can to ensure that the dimensions are powers of two,
		/// because otherwise they will not display properly on certain graphics cards.	</remarks>
		public abstract int CreateTexture( int width, int height, IntPtr scan0 );
		
		/// <summary> Updates the sub-rectangle (texX, texY) -> (texX + part.Width, texY + part.Height)
		/// of the native texture associated with the given ID, with the pixels encapsulated in the 'part' instance. </summary>
		public abstract void UpdateTexturePart( int texId, int texX, int texY, FastBitmap part );
		
		/// <summary> Binds the given texture id so that it can be used for rasterization. </summary>
		public abstract void BindTexture( int texId );
		
		/// <summary> Frees all native resources held for the given texture id. </summary>
		public abstract void DeleteTexture( ref int texId );
		
		/// <summary> Frees all native resources held for the given texture id. </summary>
		public void DeleteTexture( ref Texture texture ) { DeleteTexture( ref texture.ID ); }
		
		/// <summary> Sets whether fog is currently enabled. </summary>
		public abstract bool Fog { set; }
		
		/// <summary> Sets the fog colour that is blended with final primitive colours. </summary>
		public abstract void SetFogColour( FastColour col );
		
		/// <summary> Sets the density of exp and exp^2 fog </summary>
		public abstract void SetFogDensity( float value );
		
		/// <summary> Sets the start radius of fog for linear fog. </summary>
		public abstract void SetFogStart( float value );
		
		/// <summary> Sets the end radius of fog for for linear fog. </summary>
		public abstract void SetFogEnd( float value );
		
		/// <summary> Sets the current fog mode. (linear, exp, or exp^2) </summary>
		public abstract void SetFogMode( Fog fogMode );
		
		/// <summary> Whether back facing primitives should be culled by the 3D graphics api. </summary>
		public abstract bool FaceCulling { set; }

		
		/// <summary> Whether alpha testing is currently enabled. </summary>
		public abstract bool AlphaTest { set; }
		
		/// <summary> Sets the alpha test compare function that is used when alpha testing is enabled. </summary>
		public abstract void AlphaTestFunc( CompareFunc func, float refValue );
		
		/// <summary> Whether alpha blending is currently enabled. </summary>
		public abstract bool AlphaBlending { set; }
		
		/// <summary> Sets the alpha blend function that is used when alpha blending is enabled. </summary>
		public abstract void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc dstFunc );
		
		/// <summary> Clears the underlying back and/or front buffer. </summary>
		public abstract void Clear();
		
		/// <summary> Sets the colour the screen is cleared to when Clear() is called. </summary>
		public abstract void ClearColour( FastColour col );
		
		/// <summary> Whether depth testing is currently enabled. </summary>
		public abstract bool DepthTest { set; }
		
		/// <summary> Sets the depth test compare function that is used when depth testing is enabled. </summary>
		public abstract void DepthTestFunc( CompareFunc func );
		
		/// <summary> Whether writing to the colour buffer is enabled. </summary>
		public abstract bool ColourWrite { set; }
		
		/// <summary> Whether writing to the depth buffer is enabled. </summary>
		public abstract bool DepthWrite { set; }
		
		/// <summary> Creates a vertex buffer that can have its data dynamically updated. </summary>
		public abstract int CreateDynamicVb( VertexFormat format, int maxVertices );
		
		/// <summary> Creates a static vertex buffer that has its data set at creation,
		/// but the vertex buffer's data cannot be updated after creation. </summary>
		public abstract int CreateVb<T>( T[] vertices, VertexFormat format, int count ) where T : struct;
		
		/// <summary> Creates a static vertex buffer that has its data set at creation,
		/// but the vertex buffer's data cannot be updated after creation. </summary>
		public abstract int CreateVb( IntPtr vertices, VertexFormat format, int count );
		
		/// <summary> Creates a static index buffer that has its data set at creation,
		/// but the index buffer's data cannot be updated after creation. </summary>
		public abstract int CreateIb( ushort[] indices, int indicesCount );
		
		/// <summary> Creates a static index buffer that has its data set at creation,
		/// but the index buffer's data cannot be updated after creation. </summary>
		public abstract int CreateIb( IntPtr indices, int indicesCount );
		
		/// <summary> Sets the currently active vertex buffer to the given id. </summary>
		public abstract void BindVb( int vb );
		
		/// <summary> Sets the currently active index buffer to the given id. </summary>
		public abstract void BindIb( int ib );
		
		/// <summary> Frees all native resources held for the dynamic vertex buffer associated with the given id. </summary>
		public abstract void DeleteDynamicVb( int id );
		
		/// <summary> Frees all native resources held for the vertex buffer associated with the given id. </summary>
		public abstract void DeleteVb( int vb );
		
		/// <summary> Frees all native resources held for the index buffer associated with the given id. </summary>
		public abstract void DeleteIb( int ib );
		
		/// <summary> Informs the graphics API that the format of the vertex data used in subsequent
		/// draw calls will be in the given format. </summary>
		public abstract void SetBatchFormat( VertexFormat format );
		
		/// <summary> Binds and draws the specified subset of the vertices in the current dynamic vertex buffer<br/>
		/// This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. </summary>
		public abstract void UpdateDynamicVb<T>( DrawMode mode, int vb, T[] vertices, int count ) where T : struct;
		
		/// <summary> Binds and draws the specified subset of the vertices in the current dynamic vertex buffer<br/>
		/// This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. </summary>
		public abstract void UpdateDynamicIndexedVb<T>( DrawMode mode, int vb, T[] vertices,
		                                             int vCount, int indicesCount ) where T : struct;
		
		/// <summary> Draws the specified subset of the vertices in the current vertex buffer. </summary>
		public abstract void DrawVb( DrawMode mode, int startVertex, int verticesCount );
		
		/// <summary> Draws the specified subset of the vertices in the current vertex buffer. </summary>
		public abstract void DrawIndexedVb( DrawMode mode, int indicesCount, int startIndex );
		
		/// <summary> Optimised version of DrawIndexedVb for VertexFormat.Pos3fTex2fCol4b </summary>
		internal abstract void DrawIndexedVb_TrisT2fC4b( int indicesCount, int offsetVertex, int startIndex );
		
		internal abstract void DrawIndexedVb_TrisT2fC4b( int indicesCount, int startIndex );
		
		protected static int[] strideSizes = { 16, 24 };
		
		
		/// <summary> Sets the matrix type that load/push/pop operations should be applied to. </summary>
		public abstract void SetMatrixMode( MatrixType mode );
		
		/// <summary> Sets the current matrix to the given matrix. </summary>
		public abstract void LoadMatrix( ref Matrix4 matrix );
		
		/// <summary> Sets the current matrix to the identity matrix. </summary>
		public abstract void LoadIdentityMatrix();
		
		/// <summary> Multplies the current matrix by the given matrix, then
		/// sets the current matrix to the result of the multiplication. </summary>
		public abstract void MultiplyMatrix( ref Matrix4 matrix );
		
		/// <summary> Gets the top matrix the current matrix stack and pushes it to the stack. </summary>
		public abstract void PushMatrix();
		
		/// <summary> Removes the top matrix from the current matrix stack, then
		/// sets the current matrix to the new top matrix of the stack. </summary>
		public abstract void PopMatrix();
		
		/// <summary> Outputs a .png screenshot of the backbuffer to the specified file. </summary>
		public abstract void TakeScreenshot( string output, int width, int height );
		
		protected abstract void MakeApiInfo();
		
		public string[] ApiInfo;
		
		public void MakeGraphicsInfo() {
			MakeApiInfo();
			ErrorHandler.AdditionalInfo = ApiInfo;
		}
		
		/// <summary> Adds a warning to chat if this graphics API has problems with the current user's GPU. </summary>
		public virtual void WarnIfNecessary( Chat chat ) {
		}
		
		/// <summary> Informs the graphic api to update its state in preparation for a new frame. </summary>
		public abstract void BeginFrame( GameWindow game );
		
		/// <summary> Informs the graphic api to update its state in preparation for the end of a frame,
		/// and to prepare that frame for display on the monitor. </summary>
		public abstract void EndFrame( GameWindow game );
		
		/// <summary> Sets whether the graphics api should tie frame rendering to the refresh rate of the monitor. </summary>
		public abstract void SetVSync( GameWindow game, bool value );
		
		/// <summary> Raised when the dimensions of the game's window have changed. </summary>
		public abstract void OnWindowResize( GameWindow game );
		
		/// <summary> Delegate that is invoked when the current context is lost,
		/// and is repeatedly invoked until the context can be retrieved. </summary>
		public Action<double> LostContextFunction;
		
		protected void InitDynamicBuffers() {
			quadVb = CreateDynamicVb( VertexFormat.Pos3fCol4b, 4 );
			texVb = CreateDynamicVb( VertexFormat.Pos3fTex2fCol4b, 4 );
		}
		
		public virtual void Dispose() {
			DeleteDynamicVb( quadVb );
			DeleteDynamicVb( texVb );
		}
		
		VertexPos3fCol4b[] quadVerts = new VertexPos3fCol4b[4];
		int quadVb;
		public virtual void Draw2DQuad( float x, float y, float width, float height, FastColour col ) {
			quadVerts[0] = new VertexPos3fCol4b( x, y, 0, col );
			quadVerts[1] = new VertexPos3fCol4b( x + width, y, 0, col );
			quadVerts[2] = new VertexPos3fCol4b( x + width, y + height, 0, col );
			quadVerts[3] = new VertexPos3fCol4b( x, y + height, 0, col );
			SetBatchFormat( VertexFormat.Pos3fCol4b );
			UpdateDynamicIndexedVb( DrawMode.Triangles, quadVb, quadVerts, 4, 6 );
		}
		
		internal VertexPos3fTex2fCol4b[] texVerts = new VertexPos3fTex2fCol4b[4];
		internal int texVb;
		public virtual void Draw2DTexture( ref Texture tex, FastColour col ) {
			float x1 = tex.X1, y1 = tex.Y1, x2 = tex.X2, y2 = tex.Y2;
			#if USE_DX
			// NOTE: see "https://msdn.microsoft.com/en-us/library/windows/desktop/bb219690(v=vs.85).aspx",
			// i.e. the msdn article called "Directly Mapping Texels to Pixels (Direct3D 9)" for why we have to do this.
			x1 -= 0.5f; x2 -= 0.5f;
			y1 -= 0.5f; y2 -= 0.5f;
			#endif
			texVerts[0] = new VertexPos3fTex2fCol4b( x1, y1, 0, tex.U1, tex.V1, col );
			texVerts[1] = new VertexPos3fTex2fCol4b( x2, y1, 0, tex.U2, tex.V1, col );
			texVerts[2] = new VertexPos3fTex2fCol4b( x2, y2, 0, tex.U2, tex.V2, col );
			texVerts[3] = new VertexPos3fTex2fCol4b( x1, y2, 0, tex.U1, tex.V2, col );
			SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			UpdateDynamicIndexedVb( DrawMode.Triangles, texVb, texVerts, 4, 6 );
		}
		
		public static void Make2DQuad( TextureRec xy, TextureRec uv,
		                              VertexPos3fTex2fCol4b[] vertices, ref int index ) {
			float x1 = xy.U1, y1 = xy.V1, x2 = xy.U2, y2 = xy.V2;
			#if USE_DX
			x1 -= 0.5f; x2 -= 0.5f;
			y1 -= 0.5f; y2 -= 0.5f;
			#endif
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, 0, uv.U1, uv.V1, FastColour.White );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y1, 0, uv.U2, uv.V1, FastColour.White );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, 0, uv.U2, uv.V2, FastColour.White );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y2, 0, uv.U1, uv.V2, FastColour.White );
		}
		
		public void Draw2DTexture( ref Texture tex ) {
			Draw2DTexture( ref tex, FastColour.White );
		}
		
		/// <summary> Updates the various matrix stacks and properties so that the graphics API state
		/// is suitable for rendering 2D quads and other 2D graphics to. </summary>
		public void Mode2D( float width, float height, bool setFog ) {
			SetMatrixMode( MatrixType.Projection );
			PushMatrix();
			LoadOrthoMatrix( width, height );
			SetMatrixMode( MatrixType.Modelview );
			PushMatrix();
			LoadIdentityMatrix();
			
			DepthTest = false;
			AlphaBlending = true;
			if( setFog ) Fog = false;
		}
		
		protected virtual void LoadOrthoMatrix( float width, float height ) {
			Matrix4 matrix = Matrix4.CreateOrthographicOffCenter( 0, width, height, 0, -10000, 10000 );
			LoadMatrix( ref matrix );
		}
		
		/// <summary> Updates the various matrix stacks and properties so that the graphics API state
		/// is suitable for rendering 3D vertices. </summary>
		public void Mode3D( bool setFog ) {
			SetMatrixMode( MatrixType.Projection );
			PopMatrix(); // Get rid of orthographic 2D matrix.
			SetMatrixMode( MatrixType.Modelview );
			PopMatrix();
			
			DepthTest = true;
			AlphaBlending = false;
			if( setFog ) Fog = true;
		}
		
		internal unsafe int MakeDefaultIb() {
			const int maxIndices = 65536 / 4 * 6;
			int element = 0;
			ushort* indices = stackalloc ushort[maxIndices];
			IntPtr ptr = (IntPtr)indices;
			
			for( int i = 0; i < maxIndices; i += 6 ) {
				*indices++ = (ushort)(element + 0);
				*indices++ = (ushort)(element + 1);
				*indices++ = (ushort)(element + 2);
				
				*indices++ = (ushort)(element + 2);
				*indices++ = (ushort)(element + 3);
				*indices++ = (ushort)(element + 0);
				element += 4;
			}
			return CreateIb( ptr, maxIndices );
		}
	}

	public enum VertexFormat {
		Pos3fCol4b = 0,
		Pos3fTex2fCol4b = 1,
	}
	
	public enum DrawMode {
		Triangles = 0,
		Lines = 1,
	}
	
	public enum CompareFunc {
		Always = 0,
		NotEqual = 1,
		Never = 2,
		
		Less = 3,
		LessEqual = 4,
		Equal = 5,
		GreaterEqual = 6,
		Greater = 7,
	}
	
	public enum BlendFunc {
		Zero = 0,
		One = 1,
		
		SourceAlpha = 2,
		InvSourceAlpha = 3,
		DestAlpha = 4,
		InvDestAlpha = 5,
	}
	
	public enum Fog {
		Linear = 0,
		Exp = 1,
		Exp2 = 2,
	}
	
	public enum MatrixType {
		Projection = 0,
		Modelview = 1,
		Texture = 2,
	}
}