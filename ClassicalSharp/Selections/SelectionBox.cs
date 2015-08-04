using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {

	public class SelectionBox {
		
		public short ID;
		const int VerticesCount = 6 * 4, LineVerticesCount = 12 * 2, IndicesCount = 6 * 6;
		public int Vb;
		public IGraphicsApi Graphics;
		
		public Vector3I Min, Max;
		
		public Vector3I Dimensions {
			get { return Max - Min + new Vector3I( 1 ); }
		}
		
		public void Render( double delta ) {
			Graphics.DepthWrite = false;
			Graphics.BindVb( Vb );
			Graphics.DrawIndexedVb( DrawMode.Triangles, IndicesCount, 0 );	
			Graphics.DepthWrite = true;
			Graphics.DrawVb( DrawMode.Lines, VerticesCount, LineVerticesCount );
		}
		
		static VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[VerticesCount + LineVerticesCount];
		public SelectionBox( Vector3I start, Vector3I end, FastColour col, IGraphicsApi graphics ) {
			Graphics = graphics;		
			Min = Vector3I.Min( start, end );
			Max = Vector3I.Max( start, end );			
			int index = 0;
			Vector3 p1 = (Vector3)Min + new Vector3( 0.0625f, 0.0625f, 0.0625f );
			Vector3 p2 = (Vector3)Max - new Vector3( 0.0625f, 0.0625f, 0.0625f );
			
			RenderYPlane( ref index, p1.X, p1.Z, p2.X, p2.Z, p1.Y, col ); // bottom
			RenderYPlane( ref index, p1.X, p1.Z, p2.X, p2.Z, p2.Y, col ); // top
			RenderXPlane( ref index, p1.X, p2.X, p1.Y, p2.Y, p1.Z, col ); // sides
			RenderXPlane( ref index, p1.X, p2.X, p1.Y, p2.Y, p2.Z, col );
			RenderZPlane( ref index, p1.Z, p2.Z, p1.Y, p2.Y, p1.X, col );
			RenderZPlane( ref index, p1.Z, p2.Z, p1.Y, p2.Y, p2.X, col );
			
			col = new FastColour( (byte)~col.R, (byte)~col.G, (byte)~col.B );
			// bottom face
			Line( ref index, p1.X, p1.Y, p1.Z, p2.X, p1.Y, p1.Z, col );
			Line( ref index, p2.X, p1.Y, p1.Z, p2.X, p1.Y, p2.Z, col );
			Line( ref index, p2.X, p1.Y, p2.Z, p1.X, p1.Y, p2.Z, col );
			Line( ref index, p1.X, p1.Y, p2.Z, p1.X, p1.Y, p1.Z, col );
			// top face
			Line( ref index, p1.X, p2.Y, p1.Z, p2.X, p2.Y, p1.Z, col );
			Line( ref index, p2.X, p2.Y, p1.Z, p2.X, p2.Y, p2.Z, col );
			Line( ref index, p2.X, p2.Y, p2.Z, p1.X, p2.Y, p2.Z, col );
			Line( ref index, p1.X, p2.Y, p2.Z, p1.X, p2.Y, p1.Z, col );
			// side faces
			Line( ref index, p1.X, p1.Y, p1.Z, p1.X, p2.Y, p1.Z, col );
			Line( ref index, p2.X, p1.Y, p1.Z, p2.X, p2.Y, p1.Z, col );
			Line( ref index, p2.X, p1.Y, p2.Z, p2.X, p2.Y, p2.Z, col );
			Line( ref index, p1.X, p1.Y, p2.Z, p1.X, p2.Y, p2.Z, col );
			
			Vb = Graphics.CreateVb( vertices, VertexFormat.Pos3fCol4b, VerticesCount + LineVerticesCount );
		}
		
		void Line( ref int index, float x1, float y1, float z1, float x2, float y2, float z2, FastColour col ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z2, col );
		}
		
		void RenderZPlane( ref int index, float z1, float z2, float y1, float y2, float x, FastColour col ) {
			vertices[index++] = new VertexPos3fCol4b( x, y1, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x, y2, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x, y2, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x, y1, z2, col );
		}
		
		void RenderXPlane( ref int index, float x1, float x2, float y1, float y2, float z, FastColour col ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y2, z, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y1, z, col );
		}
		
		void RenderYPlane( ref int index, float x1, float z1, float x2, float z2, float y, FastColour col ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z1, col );
		}
		
		public void Dispose() {
			Graphics.DeleteVb( Vb );
		}
	}
}
