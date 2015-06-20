using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Renderers;

namespace DefaultPlugin {

	public class BlockModel : IModel {
		
		byte block = (byte)Block.Air;
		public BlockModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2f[6 * 6];
			vb = window.Graphics.CreateDynamicVb( VertexFormat.Pos3fTex2f, 6 * 6 );
		}
		
		public override float NameYOffset {
			get { return blockHeight + 0.075f; }
		}
		
		public override string ModelName {
			get { return "block"; }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			block = Byte.Parse( player.ModelName );
			if( block == 0 ) {
				blockHeight = 1;
				return;
			}
			
			graphics.Bind2DTexture( window.TerrainAtlas.TexId );
			blockHeight = window.BlockInfo.BlockHeight( block );
			atlas = window.TerrainAtlas;
			BlockInfo = window.BlockInfo;
			index = 0;
			if( BlockInfo.IsSprite( block ) ) {
				DrawXFace( 0f, TileSide.Right, false );
				DrawZFace( 0f, TileSide.Back, false );
				shader.DrawDynamic( DrawMode.Triangles, VertexPos3fTex2f.Size, vb, vertices, 6 * 2 );
			} else {
				DrawYFace( blockHeight, TileSide.Top );
				DrawXFace( -0.5f, TileSide.Right, false );
				DrawXFace( 0.5f, TileSide.Left, true );
				DrawZFace( -0.5f, TileSide.Front, true );
				DrawZFace( 0.5f, TileSide.Back, false );
				DrawYFace( 0f, TileSide.Bottom );
				shader.DrawDynamic( DrawMode.Triangles, VertexPos3fTex2f.Size, vb, vertices, 6 * 6 );
			}
		}
		float blockHeight;
		TerrainAtlas2D atlas;
		BlockInfo BlockInfo;
		
		public override void Dispose() {
			graphics.DeleteDynamicVb( vb );
		}
		
		void DrawYFace( float y, int side ) {
			int texId = BlockInfo.GetOptimTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );

			vertices[index++] = new VertexPos3fTex2f( -0.5f, y, -0.5f, rec.U1, rec.V1 );
			vertices[index++] = new VertexPos3fTex2f( 0.5f, y, -0.5f, rec.U2, rec.V1 );
			vertices[index++] = new VertexPos3fTex2f( 0.5f, y, 0.5f, rec.U2, rec.V2 );
			
			vertices[index++] = new VertexPos3fTex2f( 0.5f, y, 0.5f, rec.U2, rec.V2 );
			vertices[index++] = new VertexPos3fTex2f( -0.5f, y, 0.5f, rec.U1, rec.V2 );
			vertices[index++] = new VertexPos3fTex2f( -0.5f, y, -0.5f, rec.U1, rec.V1 );
		}

		void DrawZFace( float z, int side, bool swapU ) {
			int texId = BlockInfo.GetOptimTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * TerrainAtlas2D.usedInvVerElemSize;
			}
			if( swapU ) rec.SwapU();
			
			vertices[index++] = new VertexPos3fTex2f( -0.5f, 0f, z, rec.U1, rec.V2 );
			vertices[index++] = new VertexPos3fTex2f( -0.5f, blockHeight, z, rec.U1, rec.V1 );
			vertices[index++] = new VertexPos3fTex2f( 0.5f, blockHeight, z, rec.U2, rec.V1 );
			
			vertices[index++] = new VertexPos3fTex2f( 0.5f, blockHeight, z, rec.U2, rec.V1 );
			vertices[index++] = new VertexPos3fTex2f( 0.5f, 0f, z, rec.U2, rec.V2 );
			vertices[index++] = new VertexPos3fTex2f( -0.5f, 0f, z, rec.U1, rec.V2 );
		}

		void DrawXFace( float x, int side, bool swapU ) {
			int texId = BlockInfo.GetOptimTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * TerrainAtlas2D.usedInvVerElemSize;
			}
			if( swapU ) rec.SwapU();
			
			vertices[index++] = new VertexPos3fTex2f( x, 0f, -0.5f, rec.U1, rec.V2 );
			vertices[index++] = new VertexPos3fTex2f( x, blockHeight, -0.5f, rec.U1, rec.V1 );
			vertices[index++] = new VertexPos3fTex2f( x, blockHeight, 0.5f, rec.U2, rec.V1 );
			
			vertices[index++] = new VertexPos3fTex2f( x, blockHeight, 0.5f, rec.U2, rec.V1 );
			vertices[index++] = new VertexPos3fTex2f( x, 0f, 0.5f, rec.U2, rec.V2 );
			vertices[index++] = new VertexPos3fTex2f( x, 0f, -0.5f, rec.U1, rec.V2 );
		}
	}
}