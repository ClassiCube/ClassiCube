using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class CubeModel : IBlockModel {
		
		int[] texIds = new int[6];
		TextureRectangle[] recs = new TextureRectangle[6];
		float blockHeight;
		public CubeModel( TextureAtlas2D atlas, BlockInfo info, byte block ) : base( atlas, info, block ) {
			blockHeight = info.BlockHeight( block );
			for( int face = 0; face < 6; face++ ) {
				texIds[face] = info.GetOptimTextureLoc( block, face );
				TextureRectangle rec = atlas.GetTexRec( texIds[face] );
				rec.V2 = rec.V1 + blockHeight * atlas.invVerElementSize;
				recs[face] = rec;
			}
			Pass = info.IsTranslucent( block ) ? BlockPass.Transluscent : BlockPass.Solid;
		}
		
		public override int GetVerticesCount( int face, byte neighbour ) {
			return 6;
		}
		
		public override void DrawFace( int face, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			switch( face ) {
				case TileSide.Left:
					DrawLeftFace( ref index, x, y, z, vertices, col );
					break;
					
				case TileSide.Right:
					DrawRightFace( ref index, x, y, z, vertices, col );
					break;
					
				case TileSide.Front:
					DrawFrontFace( ref index, x, y, z, vertices, col );
					break;
					
				case TileSide.Back:
					DrawBackFace( ref index, x, y, z, vertices, col );
					break;
					
				case TileSide.Bottom:
					DrawBottomFace( ref index, x, y, z, vertices, col );
					break;
					
				case TileSide.Top:
					DrawTopFace( ref index, x, y, z, vertices, col );
					break;
			}
		}
		
		protected void DrawTopFace( ref int index, float x, float y, float z,
		                           VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Top];

			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + blockHeight, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + blockHeight, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + blockHeight, z + 1, rec.U1, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + blockHeight, z + 1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + blockHeight, z + 1, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + blockHeight, z, rec.U2, rec.V1, col );
		}

		protected void DrawBottomFace( ref int index, float x, float y, float z,
		                           VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Bottom];
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U1, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U2, rec.V2, col );
		}

		protected void DrawBackFace( ref int index, float x, float y, float z,
		                           VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Back];
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + blockHeight, z + 1, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + blockHeight, z + 1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, rec.U1, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + blockHeight, z + 1, rec.U2, rec.V1, col );
		}

		protected void DrawFrontFace( ref int index, float x, float y, float z,
		                           VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Back];
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + blockHeight, z, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + blockHeight, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + blockHeight, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U1, rec.V2, col );
		}

		protected void DrawLeftFace( ref int index, float x, float y, float z,
		                           VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Left];
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + blockHeight, z + 1, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + blockHeight, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U1, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + blockHeight, z + 1, rec.U2, rec.V1, col );
		}

		protected void DrawRightFace( ref int index, float x, float y, float z,
		                           VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Right];
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + blockHeight, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + blockHeight, z + 1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U1, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + blockHeight, z, rec.U2, rec.V1, col );
		}
	}
}
