// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {

	public class AxisLinesRenderer : IGameComponent {
		
		VertexP3fC4b[] vertices;
		int vb;
		Game game;
		const float size = 1/32f;
		
		public void Init( Game game ) { this.game = game; }
		public void Ready( Game game ) { }			
		public void Reset( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }		
		public void Dispose() { game.Graphics.DeleteDynamicVb( ref vb ); }
		
		public void Render( double delta ) {
			if( !game.ShowAxisLines ) return;
			if( vertices == null ) {
				vertices = new VertexP3fC4b[12];
				vb = game.Graphics.CreateDynamicVb( VertexFormat.P3fC4b, vertices.Length );
			}
			game.Graphics.Texturing = false;
			Vector3 P = game.LocalPlayer.Position; P.Y += 0.05f;
			int index = 0;
			
			SelectionBox.HorQuad( vertices, ref index, FastColour.Red.Pack(), 
			                   P.X, P.Z - size, P.X + 3, P.Z + size, P.Y );
			SelectionBox.HorQuad( vertices, ref index, FastColour.Blue.Pack(),
			                   P.X - size, P.Z, P.X + size, P.Z + 3, P.Y );
			if( game.Camera.IsThirdPerson )
				SelectionBox.VerQuad( vertices, ref index, FastColour.Green.Pack(),
				                     P.X - size, P.Y, P.Z + size, P.X + size, P.Y + 3, P.Z - size );
			
			game.Graphics.SetBatchFormat( VertexFormat.P3fC4b );
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, vertices, index, index * 6 / 4 );
		}
	}
}
