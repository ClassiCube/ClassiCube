using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using OpenTK;
using OpenTK.Graphics.OpenGL;

namespace ClassicalSharp.GraphicsAPI {
	
	public partial class OpenGLApi {
		
		public int LoadTexture( string path ) {
			if( !File.Exists( path ) ) {
				throw new FileNotFoundException( path + " not found" );
			}
			using( Bitmap bmp = new Bitmap( path ) ) {
				return LoadTexture( bmp );
			}
		}
		
		public  int LoadTexture( Bitmap bmp ) {
			Rectangle rec = new Rectangle( 0, 0, bmp.Width, bmp.Height );
			BitmapData data = bmp.LockBits( rec, ImageLockMode.ReadOnly, bmp.PixelFormat );
			int texId = LoadTexture( data.Width, data.Height, data.Scan0 );
			bmp.UnlockBits( data );
			return texId;
		}
		
		public int LoadTexture( FastBitmap bmp ) {
			if( !bmp.IsLocked )
				bmp.LockBits();
			int texId = LoadTexture( bmp.Width, bmp.Height, bmp.Scan0 );
			bmp.UnlockBits();
			return texId;
		}
		
		public void DeleteTexture( ref Texture texture ) {
			DeleteTexture( ref texture.ID );
		}
		
		public int InitVb<T>( T[] vertices, VertexFormat format ) where T : struct {
			return InitVb( vertices, format, vertices.Length );
		}
		
		protected static int[] strideSizes = { 20, 16, 24 };
		
		public void Translate( float x, float y, float z ) {
			Matrix4 matrix = Matrix4.CreateTranslation( x, y, z );
			MultiplyMatrix( ref matrix );
		}
		
		public void RotateX( float degrees ) {
			Matrix4 matrix = Matrix4.CreateRotationX( degrees * 0.01745329251f ); // PI / 180
			MultiplyMatrix( ref matrix );
		}
		
		public void RotateY( float degrees ) {
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
		
		public void CheckResources() {
		}
		
		GuiShader guiShader;
		protected void InitDynamicBuffers() {
			guiShader = new GuiShader();
			guiShader.Init( (OpenGLApi)this );
			quadVb = CreateDynamicVb( VertexFormat.Pos3fTex2fCol4b, 4 );
		}
		
		public void Dispose() {
			DeleteDynamicVb( quadVb );
		}
		
		VertexPos3fTex2fCol4b[] quads = new VertexPos3fTex2fCol4b[4];
		int quadVb;
		public virtual void Draw2DQuad( float x, float y, float width, float height, FastColour col ) {
			quads[0] = new VertexPos3fTex2fCol4b( x + width, y, 0, -100, -100, col );
			quads[1] = new VertexPos3fTex2fCol4b( x + width, y + height, 0, -100, -100, col );
			quads[2] = new VertexPos3fTex2fCol4b( x, y, 0, -100, -100, col );
			quads[3] = new VertexPos3fTex2fCol4b( x, y + height, 0, -100, -100, col );
			guiShader.DrawDynamic( this, DrawMode.TriangleStrip, quadVb, quads, VertexFormat.Pos3fTex2fCol4b, 4 );
		}
		
		public virtual void Draw2DTexture( ref Texture tex ) {
			float x1 = tex.X1, y1 = tex.Y1, x2 = tex.X2, y2 = tex.Y2;
			// Have to order them this way because it's a triangle strip.
			quads[0] = new VertexPos3fTex2fCol4b( x2, y1, 0, tex.U2, tex.V1, FastColour.White );
			quads[1] = new VertexPos3fTex2fCol4b( x2, y2, 0, tex.U2, tex.V2, FastColour.White );
			quads[2] = new VertexPos3fTex2fCol4b( x1, y1, 0, tex.U1, tex.V1, FastColour.White );
			quads[3] = new VertexPos3fTex2fCol4b( x1, y2, 0, tex.U1, tex.V2, FastColour.White );
			guiShader.DrawDynamic( this, DrawMode.TriangleStrip, quadVb, quads, VertexFormat.Pos3fTex2fCol4b, 4 );
		}
		
		public void Mode2D( float width, float height ) {
			UseProgram( guiShader.ProgramId );		
			Matrix4 matrix = Matrix4.CreateOrthographicOffCenter( 0, width, height, 0, 0, 1 );
			SetUniform( guiShader.projLoc, ref matrix );
			DepthTest = false;
			AlphaBlending = true;
		}
		
		public void Mode3D() {
			UseProgram( 0 );
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