using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class PickingRenderer {
		
		IGraphicsApi graphics;
		int vb;
		
		public PickingRenderer( Game window ) {
			graphics = window.Graphics;
			vb = graphics.CreateDynamicVb( VertexFormat.Pos3fCol4b, verticesCount );
		}
		
		FastColour col = FastColour.Black;
		int index;
		const int verticesCount = 16 * 6;
		VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[verticesCount];
		const float size = 1/32f;
		const float offset = 0.01f;
		
		public void Render( double delta, PickedPos pickedPos ) {
			index = 0;
			Vector3 p1 = pickedPos.Min - new Vector3( offset, offset, offset );
			Vector3 p2 = pickedPos.Max + new Vector3( offset, offset, offset );
			col.A = 150;
			graphics.AlphaBlending = true;
			
			// bottom face
			DrawYPlane( p1.Y, p1.X, p1.Z, p1.X + size, p2.Z );
			DrawYPlane( p1.Y, p2.X, p1.Z, p2.X - size, p2.Z );
			DrawYPlane( p1.Y, p1.X, p1.Z, p2.X, p1.Z + size );
			DrawYPlane( p1.Y, p1.X, p2.Z, p2.X, p2.Z - size );
			// top face
			DrawYPlane( p2.Y, p1.X, p1.Z, p1.X + size, p2.Z );
			DrawYPlane( p2.Y, p2.X, p1.Z, p2.X - size, p2.Z );
			DrawYPlane( p2.Y, p1.X, p1.Z, p2.X, p1.Z + size );
			DrawYPlane( p2.Y, p1.X, p2.Z, p2.X, p2.Z - size );
			// left face
			DrawXPlane( p1.X, p1.Z, p1.Y, p1.Z + size, p2.Y );
			DrawXPlane( p1.X, p2.Z, p1.Y, p2.Z - size, p2.Y );
			DrawXPlane( p1.X, p1.Z, p1.Y, p2.Z, p1.Y + size );
			DrawXPlane( p1.X, p1.Z, p2.Y, p2.Z, p2.Y - size );
			// right face
			DrawXPlane( p2.X, p1.Z, p1.Y, p1.Z + size, p2.Y );
			DrawXPlane( p2.X, p2.Z, p1.Y, p2.Z - size, p2.Y );
			DrawXPlane( p2.X, p1.Z, p1.Y, p2.Z, p1.Y + size );
			DrawXPlane( p2.X, p1.Z, p2.Y, p2.Z, p2.Y - size );
			// front face
			DrawZPlane( p1.Z, p1.X, p1.Y, p1.X + size, p2.Y );
			DrawZPlane( p1.Z, p2.X, p1.Y, p2.X - size, p2.Y );
			DrawZPlane( p1.Z, p1.X, p1.Y, p2.X, p1.Y + size );
			DrawZPlane( p1.Z, p1.X, p2.Y, p2.X, p2.Y - size );
			// back face
			DrawZPlane( p2.Z, p1.X, p1.Y, p1.X + size, p2.Y );
			DrawZPlane( p2.Z, p2.X, p1.Y, p2.X - size, p2.Y );
			DrawZPlane( p2.Z, p1.X, p1.Y, p2.X, p1.Y + size );
			DrawZPlane( p2.Z, p1.X, p2.Y, p2.X, p2.Y - size );
			
			graphics.BeginVbBatch( VertexFormat.Pos3fCol4b );
			graphics.DrawDynamicIndexedVb( DrawMode.Triangles, vb, vertices, verticesCount, verticesCount * 6 / 4 );
			graphics.AlphaBlending = false;
		}
		
		public void Dispose() {
			graphics.DeleteDynamicVb( vb );
		}
		
		void DrawXPlane( float x, float z1, float y1, float z2, float y2 ) {
			vertices[index++] = new VertexPos3fCol4b( x, y1, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x, y2, z1, col );		
			vertices[index++] = new VertexPos3fCol4b( x, y2, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x, y1, z2, col );
		}
		
		void DrawZPlane( float z, float x1, float y1, float x2, float y2 ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y2, z, col );	
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y1, z, col );
		}
		
		void DrawYPlane( float y, float x1, float z1, float x2, float z2 ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z1, col );
		}
	}
}
