using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class FenceModel : CubeModel {
		
		TextureRectangle[] frontTop = new TextureRectangle[6];
		TextureRectangle[] frontBottom = new TextureRectangle[6];
		TextureRectangle[] backTop = new TextureRectangle[6];
		TextureRectangle[] backBottom = new TextureRectangle[6];
		TextureRectangle[] leftTop = new TextureRectangle[6];
		TextureRectangle[] leftBottom = new TextureRectangle[6];
		TextureRectangle[] rightTop = new TextureRectangle[6];
		TextureRectangle[] rightBottom = new TextureRectangle[6];
		
		public FenceModel( Game game, byte block ) : base( game, block ) {
			min = new Vector3( 6 / 16f, 0f, 6 / 16f );
			max = new Vector3( 10 / 16f, 1f, 10 / 16f );
			
			TextureRectangle baseRec = recs[0];
			float scale = atlas.invVerElementSize;
			for( int face = 0; face < TileSide.Bottom; face++ ) {
				recs[face].U1 = baseRec.U1 + min.X * scale;
				recs[face].U2 = baseRec.U1 + max.X * scale;
			}
			
			TextureRectangle verRec = baseRec;
			verRec.U1 = baseRec.U1 + min.X * scale;
			verRec.U2 = baseRec.U1 + max.X * scale;
			verRec.V1 = baseRec.V1 + min.Z * scale;
			verRec.V2 = baseRec.V1 + max.Z * scale;
			recs[TileSide.Bottom] = verRec;
			recs[TileSide.Top] = verRec;
			
			frontTop[TileSide.Top] = frontTop[TileSide.Bottom] =
				frontBottom[TileSide.Top] = frontBottom[TileSide.Bottom] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 7 / 16f ) * scale, baseRec.U1 + ( 9 / 16f ) * scale,
				                            baseRec.V1 + ( 0 / 16f ) * scale, baseRec.V1 + ( 8 / 16f ) * scale );
			frontTop[TileSide.Left] = frontTop[TileSide.Right] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 0 / 16f ) * scale, baseRec.U1 + ( 8 / 16f ) * scale,
				                            baseRec.V1 + ( 1 / 16f ) * scale, baseRec.V1 + ( 4 / 16f ) * scale );
			frontBottom[TileSide.Left] = frontBottom[TileSide.Right] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 0 / 16f ) * scale, baseRec.U1 + ( 8 / 16f ) * scale,
				                            baseRec.V1 + ( 7 / 16f ) * scale, baseRec.V1 + ( 10 / 16f ) * scale );
			
			backTop[TileSide.Top] = backTop[TileSide.Bottom] =
				backBottom[TileSide.Top] = backBottom[TileSide.Bottom] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 7 / 16f ) * scale, baseRec.U1 + ( 9 / 16f ) * scale,
				                            baseRec.V1 + ( 8 / 16f ) * scale, baseRec.V1 + ( 16 / 16f ) * scale );
			backTop[TileSide.Left] = backTop[TileSide.Right] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 8 / 16f ) * scale, baseRec.U1 + ( 16 / 16f ) * scale,
				                            baseRec.V1 + ( 1 / 16f ) * scale, baseRec.V1 + ( 4 / 16f ) * scale );
			backBottom[TileSide.Left] = backBottom[TileSide.Right] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 8 / 16f ) * scale, baseRec.U1 + ( 16 / 16f ) * scale,
				                            baseRec.V1 + ( 7 / 16f ) * scale, baseRec.V1 + ( 10 / 16f ) * scale );
			
			leftTop[TileSide.Top] = leftTop[TileSide.Bottom] =
				leftBottom[TileSide.Top] = leftBottom[TileSide.Bottom] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 0 / 16f ) * scale, baseRec.U1 + ( 8 / 16f ) * scale,
				                            baseRec.V1 + ( 7 / 16f ) * scale, baseRec.V1 + ( 9 / 16f ) * scale );
			leftTop[TileSide.Front] = leftTop[TileSide.Back] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 0 / 16f ) * scale, baseRec.U1 + ( 8 / 16f ) * scale,
				                            baseRec.V1 + ( 1 / 16f ) * scale, baseRec.V1 + ( 4 / 16f ) * scale );
			leftBottom[TileSide.Front] = leftBottom[TileSide.Back] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 0 / 16f ) * scale, baseRec.U1 + ( 8 / 16f ) * scale,
				                            baseRec.V1 + ( 7 / 16f ) * scale, baseRec.V1 + ( 10 / 16f ) * scale );
			
			rightTop[TileSide.Top] = rightTop[TileSide.Bottom] =
				rightBottom[TileSide.Top] = rightBottom[TileSide.Bottom] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 8 / 16f ) * scale, baseRec.U1 + ( 16 / 16f ) * scale,
				                            baseRec.V1 + ( 7 / 16f ) * scale, baseRec.V1 + ( 9 / 16f ) * scale );
			rightTop[TileSide.Front] = rightTop[TileSide.Back] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 8 / 16f ) * scale, baseRec.U1 + ( 16 / 16f ) * scale,
				                            baseRec.V1 + ( 1 / 16f ) * scale, baseRec.V1 + ( 4 / 16f ) * scale );
			rightBottom[TileSide.Front] = rightBottom[TileSide.Back] =
				TextureRectangle.FromPoints( baseRec.U1 + ( 8 / 16f ) * scale, baseRec.U1 + ( 16 / 16f ) * scale,
				                            baseRec.V1 + ( 7 / 16f ) * scale, baseRec.V1 + ( 10 / 16f ) * scale );
			
			NeedsNeighbourState = true;
		}
		
		public override bool FaceHidden( int face, byte meta, Neighbours state, byte neighbour ) {
			return face >= TileSide.Bottom && ( neighbour == block || info.IsOpaque( neighbour ) );
		}
		
		public override int GetVerticesCount( int face, byte meta, Neighbours state, byte neighbour ) {
			return 6 + ( neighbour == block ? ( 6 * 4 ) * 2 : 0 );
		}
		
		public override void DrawFace( int face, byte meta, Neighbours state, ref int index,
		                              float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			if( face == TileSide.Front && state.Front == block ) {
				TextureRectangle[] origRecs = recs;
				min = new Vector3( 7 / 16f, 12 / 16f, 0 / 16f );
				max = new Vector3( 9 / 16f, 15 / 16f, 8 / 16f );
				DrawBars( frontTop, frontBottom, origRecs, ref index, x, y, z, vertices, true );
			} else if( face == TileSide.Back && state.Back == block ) {
				TextureRectangle[] origRecs = recs;
				min = new Vector3( 7 / 16f, 12 / 16f, 8 / 16f );
				max = new Vector3( 9 / 16f, 15 / 16f, 16 / 16f );
				DrawBars( backTop, backBottom, origRecs, ref index, x, y, z, vertices, true );
			} else if( face == TileSide.Left && state.Left == block ) {
				TextureRectangle[] origRecs = recs;
				min = new Vector3( 0 / 16f, 12 / 16f, 7 / 16f );
				max = new Vector3( 8 / 16f, 15 / 16f, 9 / 16f );
				DrawBars( leftTop, leftBottom, origRecs, ref index, x, y, z, vertices, false );
			} else if( face == TileSide.Right && state.Right == block ) {
				TextureRectangle[] origRecs = recs;
				min = new Vector3( 8 / 16f, 12 / 16f, 7 / 16f );
				max = new Vector3( 16 / 16f, 15 / 16f, 9 / 16f );
				DrawBars( rightTop, rightBottom, origRecs, ref index, x, y, z, vertices, false );
			}
			base.DrawFace( face, meta, state, ref index, x, y, z, vertices, col );
		}
		
		void DrawBars( TextureRectangle[] top, TextureRectangle[] bottom, TextureRectangle[] orig, ref int index,
		              float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, bool zBar ) {
			recs = top;
			DrawTopFace( ref index, x, y, z, vertices, game.Map.Sunlight );
			DrawBottomFace( ref index, x, y, z, vertices, game.Map.SunlightYBottom );
			if( zBar ) {
				DrawLeftFace( ref index, x, y, z, vertices, game.Map.SunlightXSide );
				DrawRightFace( ref index, x, y, z, vertices, game.Map.SunlightXSide );
			} else {
				DrawFrontFace( ref index, x, y, z, vertices, game.Map.SunlightZSide );
				DrawBackFace( ref index, x, y, z, vertices, game.Map.SunlightZSide );
			}
			
			recs = bottom;
			min.Y = 6 / 16f;
			max.Y = 9 / 16f;
			DrawTopFace( ref index, x, y, z, vertices, game.Map.Sunlight );
			DrawBottomFace( ref index, x, y, z, vertices, game.Map.SunlightYBottom );
			if( zBar ) {
				DrawLeftFace( ref index, x, y, z, vertices, game.Map.SunlightXSide );
				DrawRightFace( ref index, x, y, z, vertices, game.Map.SunlightXSide );
			} else {
				DrawFrontFace( ref index, x, y, z, vertices, game.Map.SunlightZSide );
				DrawBackFace( ref index, x, y, z, vertices, game.Map.SunlightZSide );
			}
			
			recs = orig;
			min = new Vector3( 6 / 16f, 0f, 6 / 16f );
			max = new Vector3( 10 / 16f, 1f, 10 / 16f );
		}
	}
}
