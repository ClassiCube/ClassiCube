using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using OpenTK;
using OpenTK.Graphics.OpenGL;

namespace ClassicalSharp.GraphicsAPI {
	
	public abstract class IGraphicsApi {
		
		/// <summary> Maximum supported length of a dimension
		/// (i.e. max width and max height) in a 2D texture. </summary>
		public abstract int MaxTextureDimensions { get; }
		
		public abstract bool Texturing { set; }
		
		public virtual int LoadTexture( string path ) {
			if( !File.Exists( path ) ) {
				throw new FileNotFoundException( path + " not found" );
			}
			using( Bitmap bmp = new Bitmap( path ) ) {
				return LoadTexture( bmp );
			}
		}
		
		public virtual int LoadTexture( Bitmap bmp ) {
			Rectangle rec = new Rectangle( 0, 0, bmp.Width, bmp.Height );
			BitmapData data = bmp.LockBits( rec, ImageLockMode.ReadOnly, bmp.PixelFormat );
			int texId = LoadTexture( data.Width, data.Height, data.Scan0 );
			bmp.UnlockBits( data );
			return texId;
		}
		
		public virtual int LoadTexture( FastBitmap bmp ) {
			if( !bmp.IsLocked ) 
				bmp.LockBits();
			int texId = LoadTexture( bmp.Width, bmp.Height, bmp.Scan0 );
			bmp.UnlockBits();
			return texId;
		}
		
		public abstract int LoadTexture( int width, int height, IntPtr scan0 );
		
		public abstract void Bind2DTexture( int texId );
		
		public abstract void DeleteTexture( ref int texId );
		
		public virtual void DeleteTexture( ref Texture texture ) {
			DeleteTexture( ref texture.ID );
		}
		
		public abstract bool IsValidTexture( int texId );
		
		
		/// <summary> Whether fog is currently enabled. </summary>
		public abstract bool Fog { set; }
		
		public abstract void SetFogColour( FastColour col );
		
		public abstract void SetFogDensity( float value );
		
		public abstract void SetFogStart( float value );
		
		public abstract void SetFogEnd( float value );
		
		public abstract void SetFogMode( Fog fogMode );		
		
		public abstract bool FaceCulling { set; }

		
		/// <summary> Sets the alpha test function that is used when alpha testing is enabled. </summary>
		public abstract void AlphaTestFunc( CompareFunc func, float value );
		
		/// <summary> Whether alpha testing is currently enabled. </summary>
		public abstract bool AlphaTest { set; }
		
		/// <summary> Whether alpha blending is currently enabled. </summary>
		public abstract bool AlphaBlending { set; }
		
		/// <summary> Sets the alpha blend function that isused when alpha blending is enabled. </summary>
		public abstract void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc destFunc );
		
		/// <summary> Clears the underlying back and/or front buffer. </summary>
		public abstract void Clear();
		
		/// <summary> Sets the colour the screen is cleared to when Clear() is called. </summary>
		public abstract void ClearColour( FastColour col );
		
		public abstract bool ColourWrite { set; }
		
		public abstract void DepthTestFunc( CompareFunc func );
		
		/// <summary> Whether depth testing is currently enabled. </summary>
		public abstract bool DepthTest { set; }
		
		/// <summary> Whether writing to the depth buffer is enabled. </summary>
		public abstract bool DepthWrite { set; }
		
		public abstract void SetFillType( FillType type );
		
		public abstract int CreateDynamicVb( VertexFormat format, int maxVertices );
		
		public abstract void DrawDynamicVb<T>( DrawMode mode, int vb, T[] vertices, VertexFormat format, int count ) where T : struct;
		
		public virtual int InitVb<T>( T[] vertices, VertexFormat format ) where T : struct {
			return InitVb( vertices, format, vertices.Length );
		}
		
		public abstract int InitVb<T>( T[] vertices, VertexFormat format, int count ) where T : struct;
		
		public abstract int InitIb( ushort[] indices, int indicesCount );
		
		public abstract bool IsValidVb( int vb );
		
		public abstract bool IsValidIb( int ib );
		
		public abstract void DeleteDynamicVb( int id );
		
		public abstract void DeleteVb( int id );
		
		public abstract void DeleteIb( int ib );
		
		public abstract void DrawVb( DrawMode mode, VertexFormat format, int id, int offset, int verticesCount );
		
		public abstract void BeginVbBatch( VertexFormat format );
		
		public abstract void DrawVbBatch( DrawMode mode, int id, int offset, int verticesCount );
		
		public abstract void EndVbBatch();
		
		public abstract void BeginIndexedVbBatch();
		
		public abstract void DrawIndexedVbBatch( DrawMode mode, int vb, int ib, int indicesCount );
		
		public abstract void EndIndexedVbBatch();
		
		protected static int[] strideSizes = { 20, 16, 24 };		
		
		public abstract void SetMatrixMode( MatrixType mode );
		
		public abstract void LoadMatrix( ref Matrix4 matrix );
		
		public virtual void LoadIdentityMatrix() {
			Matrix4 identity = Matrix4.Identity;
			LoadMatrix( ref identity );
		}
		
		public abstract void MultiplyMatrix( ref Matrix4 matrix );
		
		public virtual void Translate( float x, float y, float z ) {
			Matrix4 matrix = Matrix4.CreateTranslation( x, y, z );
			MultiplyMatrix( ref matrix );
		}
		
		public virtual void RotateX( float degrees ) {
			Matrix4 matrix = Matrix4.CreateRotationX( degrees * 0.01745329251f ); // PI / 180
			MultiplyMatrix( ref matrix );
		}
		
		public virtual void RotateY( float degrees ) {
			Matrix4 matrix = Matrix4.CreateRotationY( degrees * 0.01745329251f );
			MultiplyMatrix( ref matrix );
		}
		
		public virtual void RotateZ( float degrees ) {
			Matrix4 matrix = Matrix4.CreateRotationZ( degrees * 0.01745329251f );
			MultiplyMatrix( ref matrix );
		}
		
		public virtual void Scale( float x, float y, float z ) {
			Matrix4 matrix = Matrix4.Scale( x, y, z );
			MultiplyMatrix( ref matrix );
		}
		
		public abstract void PushMatrix();
		
		public abstract void PopMatrix();
		
		
		public abstract void TakeScreenshot( string output, Size size );
		
		public virtual void CheckResources() {
		}
		
		public virtual void PrintApiSpecificInfo() {
		}
		
		public abstract void BeginFrame( Game game );
		
		public abstract void EndFrame( Game game );
		
		public abstract void OnWindowResize( int newWidth, int newHeight );
		
		protected void InitDynamicBuffers() {
			quadVb = CreateDynamicVb( VertexFormat.Pos3fCol4b, 4 );
			texVb = CreateDynamicVb( VertexFormat.Pos3fTex2f, 4 );
		}
		
		public virtual void Dispose() {
			DeleteDynamicVb( quadVb );
			DeleteDynamicVb( texVb );
		}
		
		VertexPos3fCol4b[] quadVertices = new VertexPos3fCol4b[4];
		int quadVb;
		public virtual void Draw2DQuad( float x, float y, float width, float height, FastColour col ) {
			quadVertices[0] = new VertexPos3fCol4b( x + width, y, 0, col );
			quadVertices[1] = new VertexPos3fCol4b( x + width, y + height, 0, col );
			quadVertices[2] = new VertexPos3fCol4b( x, y, 0, col );
			quadVertices[3] = new VertexPos3fCol4b( x, y + height, 0, col );
			DrawDynamicVb( DrawMode.TriangleStrip, quadVb, quadVertices, VertexFormat.Pos3fCol4b, 4 );
		}
		
		VertexPos3fTex2f[] texVertices = new VertexPos3fTex2f[4];
		int texVb;
		public virtual void Draw2DTexture( ref Texture tex ) {
			float x1 = tex.X1, y1 = tex.Y1, x2 = tex.X2, y2 = tex.Y2;
			// Have to order them this way because it's a triangle strip.
			texVertices[0] = new VertexPos3fTex2f( x2, y1, 0, tex.U2, tex.V1 );
			texVertices[1] = new VertexPos3fTex2f( x2, y2, 0, tex.U2, tex.V2 );
			texVertices[2] = new VertexPos3fTex2f( x1, y1, 0, tex.U1, tex.V1 );
			texVertices[3] = new VertexPos3fTex2f( x1, y2, 0, tex.U1, tex.V2 );
			DrawDynamicVb( DrawMode.TriangleStrip, texVb, texVertices, VertexFormat.Pos3fTex2f, 4 );
		}
		
		public void Mode2D( float width, float height ) {
			SetMatrixMode( MatrixType.Projection );
			PushMatrix();
			LoadIdentityMatrix();
			DepthTest = false;
			LoadOrthoMatrix( width, height );
			SetMatrixMode( MatrixType.Modelview );
			PushMatrix();
			LoadIdentityMatrix();
			AlphaBlending = true;
		}
		
		protected virtual void LoadOrthoMatrix( float width, float height ) {
			Matrix4 matrix = Matrix4.CreateOrthographicOffCenter( 0, width, height, 0, 0, 1 );
			LoadMatrix( ref matrix );
		}
		
		public void Mode3D() {
			// Get rid of orthographic 2D matrix.
			SetMatrixMode( MatrixType.Projection );
			PopMatrix();
			SetMatrixMode( MatrixType.Modelview );
			PopMatrix();
			DepthTest = true;
			AlphaBlending = false;
		}
	}

	public enum VertexFormat {
		Pos3fTex2f = 0,
		Pos3fCol4b = 1,
		Pos3fTex2fCol4b = 2,
	}
	
	public enum DrawMode {
		Triangles = 0,
		Lines = 1,
		TriangleStrip = 2,
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
	
	public enum FillType {
		Points = 0,
		Wireframe = 1,
		Solid = 2,
	}
	
	public enum MatrixType {
		Projection = 0,
		Modelview = 1,
		Texture = 2,
	}
}