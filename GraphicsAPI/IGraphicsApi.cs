using System;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using OpenTK;
using OpenTK.Graphics.OpenGL;
using VbInfo = System.Int32;

namespace ClassicalSharp.GraphicsAPI {
	
	public partial class OpenGLApi {
		
		public virtual int LoadTexture( string path ) {
			if( !File.Exists( path ) ) {
				throw new FileNotFoundException( path + " not found" );
			}
			using( Bitmap bmp = new Bitmap( path ) ) {
				return LoadTexture( bmp );
			}
		}
		
		public void DeleteTexture( ref Texture texture ) {
			DeleteTexture( texture.ID );
			texture.ID = 0;
		}
		
		public virtual VbInfo InitVb<T>( T[] vertices, DrawMode mode, VertexFormat format ) where T : struct {
			return InitVb( vertices, mode, format, vertices.Length );
		}
		
		public virtual IndexedVbInfo InitIndexedVb<T>( T[] vertices, DrawMode mode, VertexFormat format, ushort[] indices ) where T : struct {
			return InitIndexedVb( vertices, vertices.Length, mode, format, indices, indices.Length );
		}
		
		protected int GetSizeInBytes( int count, VertexFormat format ) {
			return count * strideSizes[(int)format];
		}
		protected static int[] strideSizes = new [] { 20, 16, 24 };
		
		public void CheckResources() {
		}
		
		public virtual void Draw2DQuad( float x, float y, float width, float height, FastColour col ) {
			VertexPos3fCol4b[] vertices = {
				new VertexPos3fCol4b( x + width, y, 0, col ),
				new VertexPos3fCol4b( x + width, y + height, 0, col ),
				new VertexPos3fCol4b( x, y, 0, col ),
				new VertexPos3fCol4b( x, y + height, 0, col ),
			};
			BeginDrawClientVertices( DrawMode.TriangleStrip );
			for( int i = 0; i < vertices.Length; i++ ) {
				VertexPos3fCol4b vertex = vertices[i];
				SetVertexAttrib( shader.texCoordsLoc, -1f, -1f );
				SetVertexAttrib( shader.colourLoc, 
				                 new Vector4( vertex.R / 255f, vertex.G / 255f, vertex.B / 255f, vertex.A / 255f ) );
				SetVertexAttrib( shader.positionLoc, vertex.X, vertex.Y, vertex.Z );				
			}
			EndDrawClientVertices();
		}
		
		public void Draw2DTextureVertices( ref Texture tex ) {
			float x1 = tex.X1, y1 = tex.Y1, x2 = tex.X2, y2 = tex.Y2;
			// Have to order them this way because it's a triangle strip.
			VertexPos3fTex2f[] vertices = {
				new VertexPos3fTex2f( x2, y1, 0, tex.U2, tex.V1 ),
				new VertexPos3fTex2f( x2, y2, 0, tex.U2, tex.V2 ),
				new VertexPos3fTex2f( x1, y1, 0, tex.U1, tex.V1 ),
				new VertexPos3fTex2f( x1, y2, 0, tex.U1, tex.V2 ),
			};
			BeginDrawClientVertices( DrawMode.TriangleStrip );
			for( int i = 0; i < vertices.Length; i++ ) {
				VertexPos3fTex2f vertex = vertices[i];
				SetVertexAttrib( shader.colourLoc, new Vector4( 1, 1, 1, 1 ) );
				SetVertexAttrib( shader.texCoordsLoc, vertex.U, vertex.V );
				SetVertexAttrib( shader.positionLoc, vertex.X, vertex.Y, vertex.Z );
			}
			EndDrawClientVertices();
		}
		
		protected GuiShader shader;
		public void Mode2D( GuiShader shader ) {
			DepthTest = false;
			AlphaBlending = true;
			// Please excuse the awful hackery here
			this.shader = shader;
		}
		
		public void Mode3D() {
			DepthTest = true;
			AlphaBlending = false;
		}
	}
	
	[StructLayout( LayoutKind.Sequential, Pack = 1 )]
	public struct IndexedVbInfo {
		public int VbId, IbId;
		
		public IndexedVbInfo( int vb, int ib ) {
			VbId = vb;
			IbId = ib;
		}
	}

	public enum VertexFormat {
		VertexPos3fTex2f = 0,
		VertexPos3fCol4b = 1,
		VertexPos3fTex2fCol4b = 2
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
		None = 0,
		Linear = 1,
		Exp = 2,
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