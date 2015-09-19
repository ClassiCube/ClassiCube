using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class BlockModel : IModel {
		
		byte block = (byte)Block.Air;
		public BlockModel( Game game ) : base( game ) {
		}
		
		public override float NameYOffset {
			get { return blockHeight + 0.075f; }
		}
		
		public override float EyeY {
			get { 
				byte block = Byte.Parse( game.LocalPlayer.ModelName );
				return block == 0 ? 1 : game.BlockInfo.BlockHeight( block );
			}
		}
		
		const float adjust = 0.1f;
		public override Vector3 CollisionSize {
			get { return new Vector3( 1 - adjust, blockHeight - adjust, 1 - adjust ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -0.5f, 0f, -0.5f, 0.5f, blockHeight, 0.5f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			block = Byte.Parse( p.ModelName );
			if( block == 0 ) {
				blockHeight = 1;
				return;
			}
			
			graphics.BindTexture( game.TerrainAtlas.TexId );
			blockHeight = game.BlockInfo.BlockHeight( block );
			atlas = game.TerrainAtlas;
			BlockInfo = game.BlockInfo;
			
			if( BlockInfo.IsSprite( block ) ) {
				DrawXFace( 0f, TileSide.Right, false );
				DrawZFace( 0f, TileSide.Back, false );
			} else {
				DrawYFace( blockHeight, TileSide.Top );
				DrawXFace( -0.5f, TileSide.Right, false );
				DrawXFace( 0.5f, TileSide.Left, true );
				DrawZFace( -0.5f, TileSide.Front, true );
				DrawZFace( 0.5f, TileSide.Back, false );
				DrawYFace( 0f, TileSide.Bottom );
			}
		}
		float blockHeight;
		TerrainAtlas2D atlas;
		BlockInfo BlockInfo;
		
		public override void Dispose() {
		}
		
		void DrawYFace( float y, int side ) {
			int texId = BlockInfo.GetTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );

			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + -0.5f, pos.Y + y, pos.Z + -0.5f, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + 0.5f, pos.Y + y, pos.Z + -0.5f, rec.U2, rec.V1, col );			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + 0.5f, pos.Y + y, pos.Z + 0.5f, rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + -0.5f, pos.Y + y, pos.Z + 0.5f, rec.U1, rec.V2, col );
		}

		void DrawZFace( float z, int side, bool swapU ) {
			int texId = BlockInfo.GetTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * TerrainAtlas2D.invElementSize;
			}
			if( swapU ) rec.SwapU();
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + -0.5f, pos.Y + 0f, pos.Z + z, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + -0.5f, pos.Y + blockHeight, pos.Z + z, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + 0.5f, pos.Y + blockHeight, pos.Z + z, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + 0.5f, pos.Y + 0f, pos.Z + z, rec.U2, rec.V2, col );
		}

		void DrawXFace( float x, int side, bool swapU ) {
			int texId = BlockInfo.GetTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * TerrainAtlas2D.invElementSize;
			}
			if( swapU ) rec.SwapU();
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + 0f, pos.Z + -0.5f, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + blockHeight, pos.Z + -0.5f, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + blockHeight, pos.Z + 0.5f, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + 0f, pos.Z + 0.5f, rec.U2, rec.V2, col );
		}
	}
}