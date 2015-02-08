using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class CactusModel : CubeModel {
		
		public CactusModel( Game game, byte block ) : base( game, block ) {
		}
		
		protected override void DrawLeftFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			min.X = 1 / 16f;
			base.DrawLeftFace( ref index, x, y, z, vertices, col );
			min.X = 0;
		}
		
		protected override void DrawRightFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			max.X = 15 / 16f;
			base.DrawRightFace( ref index, x, y, z, vertices, col );
			max.X = 1;
		}
		
		protected override void DrawFrontFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			min.Z = 1 / 16f;
			base.DrawFrontFace( ref index, x, y, z, vertices, col );
			min.Z = 0;
		}
		
		protected override void DrawBackFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			max.Z = 15 / 16f;
			base.DrawBackFace( ref index, x, y, z, vertices, col );
			max.Z = 1;
		}
	}
}
