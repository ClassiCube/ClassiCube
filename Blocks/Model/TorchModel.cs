using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class TorchModel : CubeModel {
		
		public TorchModel( TextureAtlas2D atlas, BlockInfo info, byte block ) : base( atlas, info, block ) {
			TextureRectangle baseRec = recs[0];
			float scale = atlas.invVerElementSize;
			
			TextureRectangle verRec = baseRec;
			verRec.U1 = baseRec.U1 + ( 7 / 16f ) * scale;
			verRec.U2 = baseRec.U1 + ( 9 / 16f ) * scale;
			verRec.V1 = baseRec.V1 + ( 8 / 16f ) * scale;
			verRec.V2 = baseRec.V1 + ( 10 / 16f ) * scale;
			recs[TileSide.Bottom] = verRec;
			
			verRec.V1 = baseRec.V1 + ( 6 / 16f ) * scale;
			verRec.V2 = baseRec.V1 + ( 8 / 16f ) * scale;
			recs[TileSide.Top] = verRec;
			Pass = BlockPass.Sprite; // TODO: Should this be solid pass?
		}
		
		public override bool FaceHidden( int face, byte neighbour ) {
			return face == TileSide.Bottom && info.IsOpaque( neighbour );
		}
		
		public override int GetVerticesCount( int face, byte neighbour ) {
			return 6;
		}
		
		protected override void DrawLeftFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			min.X = 7 / 16f;
			base.DrawLeftFace( ref index, x, y, z, vertices, col );
			min.X = 0;
		}
		
		protected override void DrawRightFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			max.X = 9 / 16f;
			base.DrawRightFace( ref index, x, y, z, vertices, col );
			max.X = 1;
		}
		
		protected override void DrawFrontFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			min.Z = 7 / 16f;
			base.DrawFrontFace( ref index, x, y, z, vertices, col );
			min.Z = 0;
		}
		
		protected override void DrawBackFace( ref int index, float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			max.Z = 9 / 16f;
			base.DrawBackFace( ref index, x, y, z, vertices, col );
			max.Z = 1;
		}
		
		protected override void DrawTopFace( ref int index, float x, float y, float z,
		                                    VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Top];
			min.X = min.Z = 7 / 16f;
			max.X = max.Z = 9 / 16f;
			max.Y = 10 / 16f;
			base.DrawTopFace( ref index, x, y, z, vertices, col );
			min.X = min.Z = 0;
			max.X = max.Z = 1;
			max.Y = 1;
		}
	}
}
