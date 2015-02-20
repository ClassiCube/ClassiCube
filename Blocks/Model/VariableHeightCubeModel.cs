using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class VariableHeightCubeModel : IBlockModel {
		
		protected internal TextureRectangle[] recs = new TextureRectangle[6];
		protected float heightX0Z0, heightX0Z1, heightX1Z0, heightX1Z1;
		
		public VariableHeightCubeModel( Game game, byte block ) : base( game, block ) {
			float blockHeight = info.BlockHeight( block );
			for( int face = 0; face < 6; face++ ) {
				int texId = info.GetOptimTextureLoc( block, face );
				TextureRectangle rec = atlas.GetTexRec( texId );
				if( face < TileSide.Bottom ) {
					rec.V2 = rec.V1 + blockHeight * atlas.invVerElementSize;
				}
				recs[face] = rec;
			}
			Pass = info.IsTranslucent( block ) ? BlockPass.Transluscent : BlockPass.Solid;
			heightX0Z0 = heightX0Z1 = heightX1Z0 = heightX1Z1 = blockHeight;
		}
		
		public override int GetVerticesCount( int face, byte meta, ref Neighbours state, byte neighbour ) {
			return 6;
		}
		
		public override void DrawFace( int face, byte meta, ref Neighbours state, ref int index, float x, float y, float z,
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
		
		protected virtual void DrawTopFace( ref int index, float x, float y, float z,
		                                   VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Top];

			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + heightX1Z0, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + heightX0Z0, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + heightX0Z1, z + 1, rec.U1, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + heightX0Z1, z + 1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + heightX1Z1, z + 1, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + heightX1Z0, z, rec.U2, rec.V1, col );
		}

		protected virtual void DrawBottomFace( ref int index, float x, float y, float z,
		                                      VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Bottom];
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U1, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U2, rec.V2, col );
		}

		protected virtual void DrawBackFace( ref int index, float x, float y, float z,
		                                    VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Back];
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + heightX1Z1, z + 1, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + heightX0Z1, z + 1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, rec.U1, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + heightX1Z1, z + 1, rec.U2, rec.V1, col );
		}

		protected virtual void DrawFrontFace( ref int index, float x, float y, float z,
		                                     VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Front];
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + heightX0Z0, z, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + heightX0Z0, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + heightX1Z0, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U1, rec.V2, col );
		}

		protected virtual void DrawLeftFace( ref int index, float x, float y, float z,
		                                    VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Left];
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + heightX0Z1, z + 1, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + heightX0Z0, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U1, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + heightX0Z1, z + 1, rec.U2, rec.V1, col );
		}

		protected virtual void DrawRightFace( ref int index, float x, float y, float z,
		                                     VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Right];
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + heightX1Z0, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + heightX1Z1, z + 1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U1, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + heightX1Z0, z, rec.U2, rec.V1, col );
		}
	}
}
