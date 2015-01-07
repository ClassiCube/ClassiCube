using System;
using System.Drawing;
using System.IO;
using OpenTK;
using OpenTK.Graphics.OpenGL;

namespace ClassicalSharp.GraphicsAPI {
	
	public abstract class IGraphicsApi {
		
		/// <summary> Whether the underlying graphics API (and card) fully
		/// supports using non power of two textures. </summary>
		public abstract bool SupportsNonPowerOf2Textures { get; }
		
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
		
		public abstract int LoadTexture( Bitmap bmp );
		
		public abstract int LoadTexture( FastBitmap bmp );
		
		public abstract void Bind2DTexture( int texId );
		
		public abstract void DeleteTexture( int texId );
		
		public virtual void DeleteTexture( ref Texture texture ) {
			DeleteTexture( texture.ID );
			texture.ID = 0;
		}
		
		
		/// <summary> Whether fog is currently enabled. </summary>
		public abstract bool Fog { set; }
		
		public abstract void SetFogColour( FastColour col );
		
		public abstract void SetFogDensity( float value );
		
		public abstract void SetFogEnd( float value );
		
		public abstract void SetFogMode( Fog fogMode );
		
		public abstract void SetFogStart( float value );

		
		/// <summary> Sets the alpha test function that is used when alpha testing is enabled. </summary>
		public abstract void AlphaTestFunc( AlphaFunc func, float value );
		
		/// <summary> Whether alpha testing is currently enabled. </summary>
		public abstract bool AlphaTest { set; }
		
		/// <summary> Whether alpha blending is currently enabled. </summary>
		public abstract bool AlphaBlending { set; }
		
		/// <summary> Sets the alpha blend equation that is used when alpha blending is enabled. </summary>
		public abstract void AlphaBlendEq( BlendEquation eq );
		
		/// <summary> Sets the alpha blend function that isused when alpha blending is enabled. </summary>
		public abstract void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc destFunc );
		
		/// <summary> Clears the underlying back and/or front buffer. </summary>
		public abstract void Clear();
		
		/// <summary> Sets the colour the screen is cleared to when Clear() is called. </summary>
		public abstract void ClearColour( FastColour col );
		
		public abstract void ColourMask( bool red, bool green, bool blue, bool alpha );
		
		public abstract void DepthTestFunc( DepthFunc func );
		
		/// <summary> Whether depth testing is currently enabled. </summary>
		public abstract bool DepthTest { set; }
		
		/// <summary> Whether writing to the depth buffer is enabled. </summary>
		public abstract bool DepthWrite { set; }
		
		public virtual void DrawVertices( DrawMode mode, Vector3[] vertices ) {
			DrawVertices( mode, vertices, vertices.Length );
		}
		
		public virtual void DrawVertices( DrawMode mode, VertexPos3fCol4b[] vertices ) {
			DrawVertices( mode, vertices, vertices.Length );
		}
		
		public virtual void DrawVertices( DrawMode mode, VertexPos3fTex2f[] vertices ) {
			DrawVertices( mode, vertices, vertices.Length );
		}
		
		public virtual void DrawVertices( DrawMode mode, VertexPos3fTex2fCol4b[] vertices ) {
			DrawVertices( mode, vertices, vertices.Length );
		}
		
		public abstract void DrawVertices( DrawMode mode, Vector3[] vertices, int count );
		
		public abstract void DrawVertices( DrawMode mode, VertexPos3fCol4b[] vertices, int count );
		
		public abstract void DrawVertices( DrawMode mode, VertexPos3fTex2f[] vertices, int count );
		
		public abstract void DrawVertices( DrawMode mode, VertexPos3fTex2fCol4b[] vertices, int count );
		
		public abstract void SetFillType( FillType type );
		
		public abstract bool AmbientLighting { set; }
		
		public abstract void SetAmbientColour( FastColour col );
		
		public virtual int InitVb<T>( T[] vertices, DrawMode mode, VertexFormat format ) where T : struct {
			return InitVb( vertices, mode, format, vertices.Length );
		}
		
		public abstract int InitVb<T>( T[] vertices, DrawMode mode, VertexFormat format, int count ) where T : struct;
		
		public abstract void DeleteVb( int id );
		
		public virtual void DrawVb( DrawMode mode, int id, int verticesCount, VertexFormat format ) {
			if( format == VertexFormat.VertexPos3f ) DrawVbPos3f( mode, id, verticesCount );
			else if( format == VertexFormat.VertexPos3fTex2f ) DrawVbPos3fTex2f( mode, id, verticesCount );
			else if( format == VertexFormat.VertexPos3fCol4b ) DrawVbPos3fCol4b( mode, id, verticesCount );
			else if( format == VertexFormat.VertexPos3fTex2fCol4b ) DrawVbPos3fTex2fCol4b( mode, id, verticesCount );
		}
		
		public abstract void DrawVbPos3f( DrawMode mode, int id, int verticesCount );
		
		public abstract void DrawVbPos3fTex2f( DrawMode mode, int id, int verticesCount );
		
		public abstract void DrawVbPos3fCol4b( DrawMode mode, int id, int verticesCount );
		
		public abstract void DrawVbPos3fTex2fCol4b( DrawMode mode, int id, int verticesCount );
		
		public abstract void BeginVbBatch( VertexFormat format );
		
		public abstract void DrawVbBatch( DrawMode mode, int id, int verticesCount );
		
		public abstract void EndVbBatch();
		
		protected int GetSizeInBytes( int count, VertexFormat format ) {
			return count * strideSizes[(int)format];
		}
		protected static int[] strideSizes = new [] { 12, 20, 16, 24 };
		
		
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
		
		public virtual void LoadTextureTranslate( float x, float y, float z ) {
			Matrix4 matrix = Matrix4.CreateTranslation( x, y, z );
			LoadMatrix( ref matrix );
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
		
		public abstract void OnWindowResize( int newWidth, int newHeight );
		
		public virtual void Draw2DQuad( float x, float y, float width, float height, FastColour col ) {
			VertexPos3fCol4b[] vertices = {
				new VertexPos3fCol4b( x + width, y, 0, col ),
				new VertexPos3fCol4b( x + width, y + height, 0, col ),
				new VertexPos3fCol4b( x, y, 0, col ),
				new VertexPos3fCol4b( x, y + height, 0, col ),
			};
			DrawVertices( DrawMode.TriangleStrip, vertices );
		}
		
		public void Mode2D( float width, float height ) {
			SetMatrixMode( MatrixType.Projection );
			PushMatrix();
			LoadIdentityMatrix();
			DepthTest = false;
			Matrix4 matrix = Matrix4.CreateOrthographicOffCenter( 0, width, height, 0, 0, 1 );
			matrix.M33 = -1;
			matrix.M43 = 0;
			LoadMatrix( ref matrix );
			SetMatrixMode( MatrixType.Modelview );
			PushMatrix();
			LoadIdentityMatrix();
			AlphaBlending = true;
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
		VertexPos3f = 0,
		VertexPos3fTex2f = 1,
		VertexPos3fCol4b = 2,
		VertexPos3fTex2fCol4b = 3
	}
	
	public enum DrawMode {
		Triangles = 0,
		Lines = 1,
		Points = 2,
		
		TriangleStrip = 3,
		LineStrip = 4,
		TriangleFan = 5,
	}
	
	public enum BlendEquation {
		Add = 0,
		Max = 1,
		Min = 2,
		Subtract = 3,
		ReverseSubtract = 4,
	}
	
	public enum AlphaFunc {
		Always = 0,
		NotEqual = 1,
		Never = 2,
		
		Less = 3,
		LessEqual = 4,
		Equal = 5,
		GreaterEqual = 6,
		Greater = 7,
	}
	
	public enum DepthFunc {
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