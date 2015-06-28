using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;
using OpenTK;

namespace DefaultPlugin {
	
	public sealed class StandardPickingRenderer : PickingRenderer {

		int vb;
		PickingShader shader;
		
		public StandardPickingRenderer( Game window ) : base( window ) {
			this.window = window;
			graphics = window.Graphics;
			
		}
		
		public override void Init() {
			vb = graphics.CreateDynamicVb( Vector3.SizeInBytes, verticesCount );
			shader = new PickingShader();
			shader.Init( graphics );
		}
		
		int index = 0;
		const int verticesCount = 24 * ( 3 * 2 );
		Vector3[] vertices = new Vector3[verticesCount];
		const float size = 0.0625f;
		const float offset = 0.01f;
		
		public override void Render( double delta, PickedPos pickedPos ) {
			index = 0;
			shader.Bind();
			shader.UpdateFogAndMVPState( ref window.MVP );
			Vector3 p1 = pickedPos.Min - new Vector3( offset, offset, offset );
			Vector3 p2 = pickedPos.Max + new Vector3( offset, offset, offset );
			
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
			
			shader.DrawDynamic( DrawMode.Triangles, Vector3.SizeInBytes, vb, vertices, verticesCount );
		}
		
		public override void Dispose() {
			graphics.DeleteDynamicVb( vb );
		}
		
		void DrawXPlane( float x, float z1, float y1, float z2, float y2 ) {
			vertices[index++] = new Vector3( x, y1, z1 );
			vertices[index++] = new Vector3( x, y2, z1 );
			vertices[index++] = new Vector3( x, y2, z2 );
			
			vertices[index++] = new Vector3( x, y2, z2 );
			vertices[index++] = new Vector3( x, y1, z2 );
			vertices[index++] = new Vector3( x, y1, z1 );
		}
		
		void DrawZPlane( float z, float x1, float y1, float x2, float y2 ) {
			vertices[index++] = new Vector3( x1, y1, z );
			vertices[index++] = new Vector3( x1, y2, z );
			vertices[index++] = new Vector3( x2, y2, z );
			
			vertices[index++] = new Vector3( x2, y2, z );
			vertices[index++] = new Vector3( x2, y1, z );
			vertices[index++] = new Vector3( x1, y1, z );
		}
		
		void DrawYPlane( float y, float x1, float z1, float x2, float z2 ) {
			vertices[index++] = new Vector3( x1, y, z1 );
			vertices[index++] = new Vector3( x1, y, z2 );
			vertices[index++] = new Vector3( x2, y, z2 );
			
			vertices[index++] = new Vector3( x2, y, z2 );
			vertices[index++] = new Vector3( x2, y, z1 );
			vertices[index++] = new Vector3( x1, y, z1 );
		}
	}
}
