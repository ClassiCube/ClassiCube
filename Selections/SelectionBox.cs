using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {

	public class SelectionBox {
		public short ID;
		
		public FastColour Colour, LineColour;
		public int Vb, VerticesCount;
		public int LineVb, LineVerticesCount;
		public OpenGLApi Graphics;
		
		public Vector3I Min, Max;
		
		public Vector3I Dimensions {
			get { return Max - Min + new Vector3I( 1 ); }
		}
		
		public long Volume {
			get { 
				Vector3I dim = Dimensions;
				return dim.X * dim.Y * dim.Z;
			}
		}
		
		public void Render( SelectionShader shader ) {
			Graphics.DepthWrite = false;
			shader.Draw( DrawMode.Triangles, VertexPos3fCol4b.Size, Vb, 0, VerticesCount );
			Graphics.DepthWrite = true;
			shader.Draw( DrawMode.Lines, VertexPos3fCol4b.Size, LineVb, 0, LineVerticesCount );
		}
		
		public SelectionBox( Vector3I start, Vector3I end, FastColour col, OpenGLApi graphics ) {
			Graphics = graphics;
			VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[12 * 2];
			Min = Vector3I.Min( start, end );
			Max = Vector3I.Max( start,end );
			
			int index = 0;
			Vector3 p1 = (Vector3)Min + new Vector3( 0.0625f, 0.0625f, 0.0625f );
			Vector3 p2 = (Vector3)Max - new Vector3( 0.0625f, 0.0625f, 0.0625f );
			
			LineColour = new FastColour( (byte)~col.R, (byte)~col.G, (byte)~col.B );
			// bottom face
			Line( vertices, ref index, p1.X, p1.Y, p1.Z, p2.X, p1.Y, p1.Z );
			Line( vertices, ref index, p2.X, p1.Y, p1.Z, p2.X, p1.Y, p2.Z );
			Line( vertices, ref index, p2.X, p1.Y, p2.Z, p1.X, p1.Y, p2.Z );
			Line( vertices, ref index, p1.X, p1.Y, p2.Z, p1.X, p1.Y, p1.Z );
			// top face
			Line( vertices, ref index, p1.X, p2.Y, p1.Z, p2.X, p2.Y, p1.Z );
			Line( vertices, ref index, p2.X, p2.Y, p1.Z, p2.X, p2.Y, p2.Z );
			Line( vertices, ref index, p2.X, p2.Y, p2.Z, p1.X, p2.Y, p2.Z );
			Line( vertices, ref index, p1.X, p2.Y, p2.Z, p1.X, p2.Y, p1.Z );
			// side faces
			Line( vertices, ref index, p1.X, p1.Y, p1.Z, p1.X, p2.Y, p1.Z );
			Line( vertices, ref index, p2.X, p1.Y, p1.Z, p2.X, p2.Y, p1.Z );
			Line( vertices, ref index, p2.X, p1.Y, p2.Z, p2.X, p2.Y, p2.Z );
			Line( vertices, ref index, p1.X, p1.Y, p2.Z, p1.X, p2.Y, p2.Z );
			LineVerticesCount = vertices.Length;
			LineVb = Graphics.InitVb( vertices, VertexPos3fCol4b.Size );
			
			vertices = new VertexPos3fCol4b[6 * 6];
			index = 0;
			Colour = col;
			RenderYPlane( vertices, ref index, p1.X, p1.Z, p2.X, p2.Z, p1.Y ); // bottom
			RenderYPlane( vertices, ref index, p1.X, p1.Z, p2.X, p2.Z, p2.Y ); // top
			RenderXPlane( vertices, ref index, p1.X, p2.X, p1.Y, p2.Y, p1.Z ); // sides
			RenderXPlane( vertices, ref index, p1.X, p2.X, p1.Y, p2.Y, p2.Z );
			RenderZPlane( vertices, ref index, p1.Z, p2.Z, p1.Y, p2.Y, p1.X );
			RenderZPlane( vertices, ref index, p1.Z, p2.Z, p1.Y, p2.Y, p2.X );
			VerticesCount = vertices.Length;
			Vb = Graphics.InitVb( vertices, VertexPos3fCol4b.Size );
		}
		
		void Line( VertexPos3fCol4b[] vertices, ref int index, float x1, float y1, float z1, float x2, float y2, float z2 ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z1, LineColour );
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z2, LineColour );
		}
		
		void RenderZPlane( VertexPos3fCol4b[] vertices, ref int index, float z1, float z2, float y1, float y2, float x ) {
			vertices[index++] = new VertexPos3fCol4b( x, y1, z1, Colour );
			vertices[index++] = new VertexPos3fCol4b( x, y2, z1, Colour );
			vertices[index++] = new VertexPos3fCol4b( x, y2, z2, Colour );
			
			vertices[index++] = new VertexPos3fCol4b( x, y2, z2, Colour );
			vertices[index++] = new VertexPos3fCol4b( x, y1, z2, Colour );
			vertices[index++] = new VertexPos3fCol4b( x, y1, z1, Colour );
		}
		
		void RenderXPlane( VertexPos3fCol4b[] vertices, ref int index, float x1, float x2, float y1, float y2, float z ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z, Colour );
			vertices[index++] = new VertexPos3fCol4b( x1, y2, z, Colour );
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z, Colour );
			
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z, Colour );
			vertices[index++] = new VertexPos3fCol4b( x2, y1, z, Colour );
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z, Colour );
		}
		
		void RenderYPlane( VertexPos3fCol4b[] vertices, ref int index, float x1, float z1, float x2, float z2, float y ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y, z1, Colour );
			vertices[index++] = new VertexPos3fCol4b( x1, y, z2, Colour );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z2, Colour );
			
			vertices[index++] = new VertexPos3fCol4b( x2, y, z2, Colour );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z1, Colour );
			vertices[index++] = new VertexPos3fCol4b( x1, y, z1, Colour );
		}
		
		public void Dispose() {
			Graphics.DeleteVb( LineVb );
			Graphics.DeleteVb( Vb );
		}
	}
}
