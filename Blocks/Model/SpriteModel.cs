using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class SpriteModel : IBlockModel {
		
		protected TextureRectangle rec;
		public SpriteModel( Game game, byte block ) : base( game, block ) {
			int texId = info.GetOptimTextureLoc( block, TileSide.Top );
			rec = atlas.GetTexRec( texId );
			Pass = BlockPass.Sprite;
		}
		
		public override bool HasFace( int face ) {
			return face == TileSide.Top;
		}
		
		public override bool FaceHidden( int face, byte meta, ref Neighbours state, byte neighbour ) {
			return false;
		}
		
		public override int GetVerticesCount( int face, byte meta, ref Neighbours state, byte neighbour ) {
			return 6 + 6;
		}
		
		public override void DrawFace( int face, byte meta, ref Neighbours state, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			// TODO: Rotate this.
			// Draw Z axis
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + 1, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + 1, z + 1, rec.U1, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + 1, z + 1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U2, rec.V2, col );
			
			// Draw X axis
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + 1, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + 1, z + 1, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + 1, z + 1, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U1, rec.V2, col );
		}
	}
}
