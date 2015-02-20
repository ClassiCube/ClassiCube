using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class BedModel : CubeModel {
		
		TextureRectangle headTopRec, footTopRec;
		public BedModel( Game game, byte block ) : base( game, block ) {
			headTopRec = atlas.GetTexRec( 8, 7 );
			footTopRec = atlas.GetTexRec( 7, 7 );
			recs[TileSide.Bottom] = atlas.GetTexRec( 4, 0 );
			recs[TileSide.Left] = atlas.GetTexRec( 15, 7 );
			recs[TileSide.Right] = atlas.GetTexRec( 14, 7 );
			recs[TileSide.Front] = atlas.GetTexRec( 13, 7 );
			recs[TileSide.Back] = atlas.GetTexRec( 0, 8 );
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
					
				default:
					base.DrawFace( face, meta, ref state, ref index, x, y, z, vertices, col );
					break;
			}
		}
		
		void DoTopFace( byte meta, ref int index, float x, float y, float z, 
		               VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			recs[TileSide.Top] = meta >= 0x08 ? headTopRec : footTopRec;
			max.Y = 9 / 16f;
			switch( meta & 0x03 ) {
					case 0:
					DrawRotatedTopFace( ref index, x, y, z, vertices, col );
					break;
					
				case 1:
					{
						float u1 = recs[TileSide.Top].U1;
						recs[TileSide.Top].U1 = recs[TileSide.Top].U2;
						recs[TileSide.Top].U2 = u1;
						DrawTopFace( ref index, x, y, z, vertices, col );
					} break;
					
				case 2:
					{
						float u1 = recs[TileSide.Top].U1;
						recs[TileSide.Top].U1 = recs[TileSide.Top].U2;
						recs[TileSide.Top].U2 = u1;
						DrawRotatedTopFace( ref index, x, y, z, vertices, col );
					} break;
					
				case 3:
					DrawTopFace( ref index, x, y, z, vertices, col );
					break;
			}
			max.Y = 1;
		}
	}
}
