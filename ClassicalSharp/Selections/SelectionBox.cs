using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {

	public class SelectionBox {
		
		public short ID;
		public const int VerticesCount = 6 * 6, LineVerticesCount = 12 * 2;
		public int Vb, LineVb;
		public IGraphicsApi Graphics;
		
		public Vector3I Min, Max;
		
		public Vector3I Dimensions {
			get { return Max - Min + new Vector3I( 1 ); }
		}
		
		public void Render( double delta ) {
			Graphics.DepthWrite = false;
			Graphics.DrawVb( DrawMode.Triangles, Vb, 0, VerticesCount );
			Graphics.DepthWrite = true;
			Graphics.DrawVb( DrawMode.Lines, LineVb, 0, LineVerticesCount );
		}
		
		static VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[VerticesCount];
		public SelectionBox( Vector3I start, Vector3I end, FastColour col, IGraphicsApi graphics ) {
			Graphics = graphics;
			
			Min = Vector3I.Min( start, end );
			Max = Vector3I.Max( start, end );
			
			int index = 0;
			Vector3 p1 = (Vector3)Min + new Vector3( 0.0625f, 0.0625f, 0.0625f );
			Vector3 p2 = (Vector3)Max - new Vector3( 0.0625f, 0.0625f, 0.0625f );
			
			FastColour lineCol = new FastColour( (byte)~col.R, (byte)~col.G, (byte)~col.B );
			// bottom face
			Line( ref index, p1.X, p1.Y, p1.Z, p2.X, p1.Y, p1.Z, lineCol );
			Line( ref index, p2.X, p1.Y, p1.Z, p2.X, p1.Y, p2.Z, lineCol );
			Line( ref index, p2.X, p1.Y, p2.Z, p1.X, p1.Y, p2.Z, lineCol );
			Line( ref index, p1.X, p1.Y, p2.Z, p1.X, p1.Y, p1.Z, lineCol );
			// top face
			Line( ref index, p1.X, p2.Y, p1.Z, p2.X, p2.Y, p1.Z, lineCol );
			Line( ref index, p2.X, p2.Y, p1.Z, p2.X, p2.Y, p2.Z, lineCol );
			Line( ref index, p2.X, p2.Y, p2.Z, p1.X, p2.Y, p2.Z, lineCol );
			Line( ref index, p1.X, p2.Y, p2.Z, p1.X, p2.Y, p1.Z, lineCol );
			// side faces
			Line( ref index, p1.X, p1.Y, p1.Z, p1.X, p2.Y, p1.Z, lineCol );
			Line( ref index, p2.X, p1.Y, p1.Z, p2.X, p2.Y, p1.Z, lineCol );
			Line( ref index, p2.X, p1.Y, p2.Z, p2.X, p2.Y, p2.Z, lineCol );
			Line( ref index, p1.X, p1.Y, p2.Z, p1.X, p2.Y, p2.Z, lineCol );
			LineVb = Graphics.CreateVb( vertices, VertexFormat.Pos3fCol4b, LineVerticesCount );
			
			index = 0;
			RenderYPlane( ref index, p1.X, p1.Z, p2.X, p2.Z, p1.Y, col ); // bottom
			RenderYPlane( ref index, p1.X, p1.Z, p2.X, p2.Z, p2.Y, col ); // top
			RenderXPlane( ref index, p1.X, p2.X, p1.Y, p2.Y, p1.Z, col ); // sides
			RenderXPlane( ref index, p1.X, p2.X, p1.Y, p2.Y, p2.Z, col );
			RenderZPlane( ref index, p1.Z, p2.Z, p1.Y, p2.Y, p1.X, col );
			RenderZPlane( ref index, p1.Z, p2.Z, p1.Y, p2.Y, p2.X, col );
			Vb = Graphics.CreateVb( vertices, VertexFormat.Pos3fCol4b, VerticesCount );
		}
		
		void Line( ref int index, float x1, float y1, float z1, float x2, float y2, float z2, FastColour col ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z2, col );
		}
		
		void RenderZPlane( ref int index, float z1, float z2, float y1, float y2, float x, FastColour col ) {
			vertices[index++] = new VertexPos3fCol4b( x, y1, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x, y2, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x, y2, z2, col );
			
			vertices[index++] = new VertexPos3fCol4b( x, y2, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x, y1, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x, y1, z1, col );
		}
		
		void RenderXPlane( ref int index, float x1, float x2, float y1, float y2, float z, FastColour col ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y2, z, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z, col );
			
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y1, z, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z, col );
		}
		
		void RenderYPlane( ref int index, float x1, float z1, float x2, float z2, float y, FastColour col ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z2, col );
			
			vertices[index++] = new VertexPos3fCol4b( x2, y, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y, z1, col );
		}
		
		public void Dispose() {
			Graphics.DeleteVb( LineVb );
			Graphics.DeleteVb( Vb );
		}
	}
}
