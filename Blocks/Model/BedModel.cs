using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class BedModel : CubeModel {
		
		TextureRectangle headTop, footTop;
		TextureRectangle headFront, footFront, headSide, footSide;
		TextureRectangle[] leftRecs = new TextureRectangle[16];
		TextureRectangle[] rightRecs = new TextureRectangle[16];
		TextureRectangle[] frontRecs = new TextureRectangle[16];
		TextureRectangle[] backRecs = new TextureRectangle[16];
		public BedModel( Game game, byte block ) : base( game, block ) {
			headTop = atlas.GetTexRec( 8, 7 );
			footTop = atlas.GetTexRec( 7, 7 );
			headFront = atlas.GetTexRec( 0, 8 );
			headSide = atlas.GetTexRec( 15, 7 );
			footSide = atlas.GetTexRec( 14, 7 );
			footFront = atlas.GetTexRec( 13, 7 );
			
			leftRecs[0] = footSide;
			leftRecs[2] = footSide.ReverseU();
			leftRecs[3] = footFront;
			leftRecs[8 + 0] = headSide;
			leftRecs[8 + 1] = headFront;
			leftRecs[8 + 2] = headSide.ReverseU();
			
			rightRecs[0] = footSide.ReverseU();
			rightRecs[1] = footFront;
			rightRecs[2] = footSide;
			rightRecs[8 + 0] = headSide.ReverseU();
			rightRecs[8 + 2] = headSide;
			rightRecs[8 + 3] = headFront;
			
			backRecs[1] = footSide.ReverseU();
			backRecs[2] = footFront;
			backRecs[3] = footSide;
			backRecs[8 + 0] = headFront;
			backRecs[8 + 1] = headSide.ReverseU();
			backRecs[8 + 3] = headSide;
			
			frontRecs[0] = footFront;
			frontRecs[1] = footSide;
			frontRecs[3] = footSide.ReverseU();
			frontRecs[8 + 1] = headSide;
			frontRecs[8 + 2] = headFront;
			frontRecs[8 + 3] = headSide.ReverseU();
						
			recs[TileSide.Bottom] = atlas.GetTexRec( 4, 0 );
		}
		
		public override bool FaceHidden( int face, byte meta, ref Neighbours state, byte neighbour ) {
			return face < TileSide.Bottom && base.FaceHidden( face, meta, ref state, neighbour );
		}
		
		public override void DrawFace( int face, byte meta, ref Neighbours state, ref int index,
		                              float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			switch( face ) {
				case TileSide.Bottom:
					min.Y = 3 / 16f;
					DrawBottomFace( ref index, x, y, z, vertices, col );
					min.Y = 0;
					break;
					
				case TileSide.Top:
					DoTopFace( meta, ref index, x, y, z, vertices, col );
					break;
					
				case TileSide.Left:
					recs[TileSide.Left] = leftRecs[meta];
					DrawLeftFace( ref index, x, y, z, vertices, col );
					break;
					
				case TileSide.Right:
					recs[TileSide.Right] = rightRecs[meta];
					DrawRightFace( ref index, x, y, z, vertices, col );
					break;
					
				case TileSide.Front:
					recs[TileSide.Front] = frontRecs[meta];
					DrawFrontFace( ref index, x, y, z, vertices, col );
					break;
					
				case TileSide.Back:
					recs[TileSide.Back] = backRecs[meta];
					DrawBackFace( ref index, x, y, z, vertices, col );
					break;
			}
		}
		
		
		void DoTopFace( byte meta, ref int index, float x, float y, float z,
		               VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			recs[TileSide.Top] = meta >= 0x08 ? headTop : footTop;
			max.Y = 9 / 16f;
			switch( meta & 0x03 ) {
				case 0:
					DrawRotatedTopFace( ref index, x, y, z, vertices, col );
					break;
					
				case 1:
					recs[TileSide.Top] = recs[TileSide.Top].ReverseU();
					DrawTopFace( ref index, x, y, z, vertices, col );
					break;
					
				case 2:
					recs[TileSide.Top] = recs[TileSide.Top].ReverseU();
					DrawRotatedTopFace( ref index, x, y, z, vertices, col );
					break;
					
				case 3:
					DrawTopFace( ref index, x, y, z, vertices, col );
					break;
			}
			max.Y = 1;
		}
	}
}
