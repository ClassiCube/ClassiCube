using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class SeedsModel : CubeModel {
		
		public SeedsModel( Game game, byte block ) : base( game, block ) {
			Pass = BlockPass.Sprite;
			FlipU( ref recs[TileSide.Right] );
			FlipU( ref recs[TileSide.Front] );
			min.Y = -1 / 16f; // farmland is 15/16 height
			max.Y = 15 / 16f;
		}
		
		static void FlipU( ref TextureRectangle rec ) {
			float u1 = rec.U1;
			rec.U1 = rec.U2;
			rec.U2 = u1;
		}
		
		public override bool HasFace( int face ) {
			return face < TileSide.Bottom;
		}
		
		public override bool FaceHidden( int face, byte meta, Neighbours state, byte neighbour ) {
			return false;
		}
		
		public override int GetVerticesCount( int face, byte meta, Neighbours state, byte neighbour ) {
			return 6;
		}
		
		public override void DrawFace( int face, byte meta, Neighbours state, ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			col = FastColour.White;
			base.DrawFace( face, meta, state, ref index, x, y, z, vertices, col );
		}
		
		protected override void DrawLeftFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			min.X = 4 / 16f;
			base.DrawLeftFace( ref index, x, y, z, vertices, col );
			min.X = 0;
		}
		
		protected override void DrawRightFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			max.X = 12 / 16f;
			base.DrawRightFace( ref index, x, y, z, vertices, col );
			max.X = 1;
		}
		
		protected override void DrawFrontFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			min.Z = 4 / 16f;
			base.DrawFrontFace( ref index, x, y, z, vertices, col );
			min.Z = 0;
		}
		
		protected override void DrawBackFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			max.Z = 12 / 16f;
			base.DrawBackFace( ref index, x, y, z, vertices, col );
			max.Z = 1;
		}
	}
}
