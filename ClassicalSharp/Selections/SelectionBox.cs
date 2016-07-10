// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {

	public class SelectionBox {
		
		public byte ID;
		public Vector3I Min, Max;
		public FastColour Colour;
		public float MinDist, MaxDist;
		
		public SelectionBox( Vector3I start, Vector3I end, FastColour col ) {	
			Min = Vector3I.Min( start, end );
			Max = Vector3I.Max( start, end );
			Colour = col;
		}
		
		public void Render( double delta, VertexP3fC4b[] vertices, VertexP3fC4b[] lineVertices,
		                  ref int index, ref int lineIndex ) {
			float offset = MinDist < 32 * 32 ? 1/32f : 1/16f;
			Vector3 p1 = (Vector3)Min - new Vector3( offset, offset, offset );
			Vector3 p2 = (Vector3)Max + new Vector3( offset, offset, offset );		
			int col = Colour.Pack();
			
			YQuad( vertices, ref index, p1.X, p1.Z, p2.X, p2.Z, p1.Y, col ); // bottom
			YQuad( vertices, ref index, p1.X, p1.Z, p2.X, p2.Z, p2.Y, col ); // top
			XQuad( vertices, ref index, p1.X, p1.Y, p2.X, p2.Y, p1.Z, col ); // sides
			XQuad( vertices, ref index, p1.X, p1.Y, p2.X, p2.Y, p2.Z, col );
			ZQuad( vertices, ref index, p1.Z, p1.Y, p2.Z, p2.Y, p1.X, col );
			ZQuad( vertices, ref index, p1.Z, p1.Y, p2.Z, p2.Y, p2.X, col );
			
			col = new FastColour( (byte)~Colour.R, (byte)~Colour.G, (byte)~Colour.B ).Pack();
			// bottom face
			Line( lineVertices, ref lineIndex, p1.X, p1.Y, p1.Z, p2.X, p1.Y, p1.Z, col );
			Line( lineVertices, ref lineIndex, p2.X, p1.Y, p1.Z, p2.X, p1.Y, p2.Z, col );
			Line( lineVertices, ref lineIndex, p2.X, p1.Y, p2.Z, p1.X, p1.Y, p2.Z, col );
			Line( lineVertices, ref lineIndex, p1.X, p1.Y, p2.Z, p1.X, p1.Y, p1.Z, col );
			// top face
			Line( lineVertices, ref lineIndex, p1.X, p2.Y, p1.Z, p2.X, p2.Y, p1.Z, col );
			Line( lineVertices, ref lineIndex, p2.X, p2.Y, p1.Z, p2.X, p2.Y, p2.Z, col );
			Line( lineVertices, ref lineIndex, p2.X, p2.Y, p2.Z, p1.X, p2.Y, p2.Z, col );
			Line( lineVertices, ref lineIndex, p1.X, p2.Y, p2.Z, p1.X, p2.Y, p1.Z, col );
			// side faces
			Line( lineVertices, ref lineIndex, p1.X, p1.Y, p1.Z, p1.X, p2.Y, p1.Z, col );
			Line( lineVertices, ref lineIndex, p2.X, p1.Y, p1.Z, p2.X, p2.Y, p1.Z, col );
			Line( lineVertices, ref lineIndex, p2.X, p1.Y, p2.Z, p2.X, p2.Y, p2.Z, col );
			Line( lineVertices, ref lineIndex, p1.X, p1.Y, p2.Z, p1.X, p2.Y, p2.Z, col );
		}
		
		static void ZQuad( VertexP3fC4b[] vertices, ref int index, float z1, float y1, 
		                  float z2, float y2, float x, int col ) {
			vertices[index++] = new VertexP3fC4b( x, y1, z1, col );
			vertices[index++] = new VertexP3fC4b( x, y2, z1, col );
			vertices[index++] = new VertexP3fC4b( x, y2, z2, col );
			vertices[index++] = new VertexP3fC4b( x, y1, z2, col );
		}
		
		static void XQuad( VertexP3fC4b[] vertices, ref int index, float x1, float y1, 
		                  float x2, float y2, float z, int col ) {
			vertices[index++] = new VertexP3fC4b( x1, y1, z, col );
			vertices[index++] = new VertexP3fC4b( x1, y2, z, col );
			vertices[index++] = new VertexP3fC4b( x2, y2, z, col );
			vertices[index++] = new VertexP3fC4b( x2, y1, z, col );
		}
		
		static void YQuad( VertexP3fC4b[] vertices, ref int index, float x1, float z1, 
		                  float x2, float z2, float y, int col ) {
			vertices[index++] = new VertexP3fC4b( x1, y, z1, col );
			vertices[index++] = new VertexP3fC4b( x1, y, z2, col );
			vertices[index++] = new VertexP3fC4b( x2, y, z2, col );
			vertices[index++] = new VertexP3fC4b( x2, y, z1, col );
		}
		
		static void Line( VertexP3fC4b[] vertices, ref int index, float x1, float y1, float z1, 
		          float x2, float y2, float z2, int col ) {
			vertices[index++] = new VertexP3fC4b( x1, y1, z1, col );
			vertices[index++] = new VertexP3fC4b( x2, y2, z2, col );
		}
	}
}
