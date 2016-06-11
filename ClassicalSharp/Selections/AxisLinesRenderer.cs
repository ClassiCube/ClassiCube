// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {

	public class AxisLinesRenderer : IGameComponent {
		
		VertexP3fC4b[] vertices;
		int vb;
		Game game;
		
		public void Init( Game game ) {
			this.game = game;
		}

		public void Ready( Game game ) { }			
		public void Reset( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		
		public void Dispose() {
			game.Graphics.DeleteDynamicVb( vb );
		}
		
		public void Render( double delta ) {
			if( !game.ShowAxisLines ) 
				return;
			if( vertices == null ) {
				vertices = new VertexP3fC4b[12];
				vb = game.Graphics.CreateDynamicVb( VertexFormat.P3fC4b, vertices.Length );
			}
			game.Graphics.Texturing = false;			
			Vector3 pos = game.LocalPlayer.Position; pos.Y += 0.05f;
			int index = 0;
			const float size = 1/32f;
			
			HorQuad( ref index, pos.X, pos.Z - size, pos.X + 3, pos.Z + size, pos.Y, FastColour.Red );
			HorQuad( ref index, pos.X - size, pos.Z, pos.X + size, pos.Z + 3, pos.Y, FastColour.Blue );
			if( game.Camera.IsThirdPerson )
				VerQuad( ref index, pos.X - size, pos.Y, pos.Z + size, pos.X + size, pos.Y + 3, pos.Z - size, FastColour.Green );
			
			game.Graphics.SetBatchFormat( VertexFormat.P3fC4b );
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, vertices, index, index * 6 / 4 );
		}
		
		void VerQuad( ref int index, float x1, float y1, float z1, float x2, float y2, float z2, FastColour col ) {
			vertices[index++] = new VertexP3fC4b( x1, y1, z1, col );
			vertices[index++] = new VertexP3fC4b( x1, y2, z1, col );
			vertices[index++] = new VertexP3fC4b( x2, y2, z2, col );
			vertices[index++] = new VertexP3fC4b( x2, y1, z2, col );
		}
		
		void HorQuad( ref int index, float x1, float z1, float x2, float z2, float y, FastColour col ) {
			vertices[index++] = new VertexP3fC4b( x1, y, z1, col );
			vertices[index++] = new VertexP3fC4b( x1, y, z2, col );
			vertices[index++] = new VertexP3fC4b( x2, y, z2, col );
			vertices[index++] = new VertexP3fC4b( x2, y, z1, col );
		}
	}
}
