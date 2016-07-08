// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public sealed class PickedPosRenderer : IGameComponent {
		
		IGraphicsApi api;
		Game game;
		int vb;
		
		public void Init( Game game ) {
			api = game.Graphics;
			vb = api.CreateDynamicVb( VertexFormat.P3fC4b, verticesCount );
			this.game = game;
		}

		public void Ready( Game game ) { }
		public void Reset( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		public void Dispose() { api.DeleteDynamicVb( vb ); }
		
		FastColour col = new FastColour( 0, 0, 0, 102 );
		int index;
		const int verticesCount = 16 * 6;
		VertexP3fC4b[] vertices = new VertexP3fC4b[verticesCount];
		
		public void UpdateState( PickedPos selected ) {
			index = 0;
			Vector3 camPos = game.CurrentCameraPos;
			float dist = (camPos - selected.Min).LengthSquared;
			
			float offset = 0.01f;
			if( dist < 4 * 4 ) offset = 0.00625f;
			if( dist < 2 * 2 ) offset = 0.00500f;
			
			Vector3 p1 = selected.Min - new Vector3( offset, offset, offset );
			Vector3 p2 = selected.Max + new Vector3( offset, offset, offset );
			BlockInfo info = game.BlockInfo;
			if( info.IsLiquid( selected.Block ) ) {
				p1.X -= 0.1f/16; p2.X -= 0.1f/16;
				p1.Z -= 0.1f/16; p2.Z -= 0.1f/16;
			} else if( info.IsTranslucent[selected.Block] && info.Collide[selected.Block] != CollideType.Solid ) {
				p1.X += 0.1f/16; p2.X += 0.1f/16;
				p1.Z += 0.1f/16; p2.Z += 0.1f/16;
			}
			
			float size = 1/16f;
			if( dist < 32 * 32 ) size = 1/32f;
			if( dist < 16 * 16 ) size = 1/64f;
			if( dist < 8 * 8 ) size = 1/96f;
			if( dist < 4 * 4 ) size = 1/128f;
			if( dist < 2 * 2 ) size = 1/192f;

			DrawLines( p1, p2, size );
		}
		
		public void Render( double delta ) {
			api.AlphaBlending = true;
			api.DepthWrite = false;			
			api.SetBatchFormat( VertexFormat.P3fC4b );
			api.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, vertices, index, index * 6 / 4 );
			api.DepthWrite = true;
			api.AlphaBlending = false;	
		}
		
		void DrawLines( Vector3 p1, Vector3 p2, float size ) {
			// bottom face
			YQuad( p1.Y, p1.X, p1.Z + size, p1.X + size, p2.Z - size );
			YQuad( p1.Y, p2.X, p1.Z + size, p2.X - size, p2.Z - size );
			YQuad( p1.Y, p1.X, p1.Z, p2.X, p1.Z + size );
			YQuad( p1.Y, p1.X, p2.Z, p2.X, p2.Z - size );
			// top face
			YQuad( p2.Y, p1.X, p1.Z + size, p1.X + size, p2.Z - size );
			YQuad( p2.Y, p2.X, p1.Z + size, p2.X - size, p2.Z - size );
			YQuad( p2.Y, p1.X, p1.Z, p2.X, p1.Z + size );
			YQuad( p2.Y, p1.X, p2.Z, p2.X, p2.Z - size );
			// left face
			XQuad( p1.X, p1.Z, p1.Y + size, p1.Z + size, p2.Y - size );
			XQuad( p1.X, p2.Z, p1.Y + size, p2.Z - size, p2.Y - size );
			XQuad( p1.X, p1.Z, p1.Y, p2.Z, p1.Y + size );
			XQuad( p1.X, p1.Z, p2.Y, p2.Z, p2.Y - size );
			// right face
			XQuad( p2.X, p1.Z, p1.Y + size, p1.Z + size, p2.Y - size );
			XQuad( p2.X, p2.Z, p1.Y + size, p2.Z - size, p2.Y - size );
			XQuad( p2.X, p1.Z, p1.Y, p2.Z, p1.Y + size );
			XQuad( p2.X, p1.Z, p2.Y, p2.Z, p2.Y - size );
			// front face
			ZQuad( p1.Z, p1.X, p1.Y + size, p1.X + size, p2.Y - size );
			ZQuad( p1.Z, p2.X, p1.Y + size, p2.X - size, p2.Y - size );
			ZQuad( p1.Z, p1.X, p1.Y, p2.X, p1.Y + size );
			ZQuad( p1.Z, p1.X, p2.Y, p2.X, p2.Y - size );
			// back face
			ZQuad( p2.Z, p1.X, p1.Y + size, p1.X + size, p2.Y - size );
			ZQuad( p2.Z, p2.X, p1.Y + size, p2.X - size, p2.Y - size );
			ZQuad( p2.Z, p1.X, p1.Y, p2.X, p1.Y + size );
			ZQuad( p2.Z, p1.X, p2.Y, p2.X, p2.Y - size );
		}
		
		void XQuad( float x, float z1, float y1, float z2, float y2 ) {
			vertices[index++] = new VertexP3fC4b( x, y1, z1, col );
			vertices[index++] = new VertexP3fC4b( x, y2, z1, col );
			vertices[index++] = new VertexP3fC4b( x, y2, z2, col );
			vertices[index++] = new VertexP3fC4b( x, y1, z2, col );
		}
		
		void ZQuad( float z, float x1, float y1, float x2, float y2 ) {
			vertices[index++] = new VertexP3fC4b( x1, y1, z, col );
			vertices[index++] = new VertexP3fC4b( x1, y2, z, col );
			vertices[index++] = new VertexP3fC4b( x2, y2, z, col );
			vertices[index++] = new VertexP3fC4b( x2, y1, z, col );
		}
		
		void YQuad( float y, float x1, float z1, float x2, float z2 ) {
			vertices[index++] = new VertexP3fC4b( x1, y, z1, col );
			vertices[index++] = new VertexP3fC4b( x1, y, z2, col );
			vertices[index++] = new VertexP3fC4b( x2, y, z2, col );
			vertices[index++] = new VertexP3fC4b( x2, y, z1, col );
		}
	}
}
