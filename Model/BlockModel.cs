using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class BlockModel : IModel {
		
		byte block = (byte)Block.Air;	
		public BlockModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[6 * 6];
		}
		
		public override float NameYOffset {
			get { return blockHeight + 0.075f; }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			block = Byte.Parse( player.ModelName );
			if( block == 0 ) {
				blockHeight = 1;
				return;
			}
			
			graphics.Bind2DTexture( window.TerrainAtlasTexId );			
			blockHeight = window.BlockInfo.BlockHeight( block );
			atlas = window.TerrainAtlas;
			BlockInfo = window.BlockInfo;			
			index = 0;
			if( BlockInfo.IsSprite( block ) ) {
				DrawSprite();
				graphics.DrawVertices( DrawMode.Triangles, vertices, 6 * 2 );
			} else {
				DrawTopFace();
				DrawLeftFace();
				DrawRightFace();
				DrawFrontFace();
				DrawBackFace();
				DrawBottomFace();
				graphics.DrawVertices( DrawMode.Triangles, vertices, 6 * 6 );
			}
		}		
		float blockHeight;
		TextureAtlas2D atlas;
		BlockInfo BlockInfo;
		
		public override void Dispose() {			
		}
		
		void DrawTopFace() {
			int texId = BlockInfo.GetOptimTextureLoc( block, TileSide.Top );
			TextureRectangle rec = atlas.GetTexRec( texId );

			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f + blockHeight, -0.5f, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, -0.5f, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, 0.5f, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, 0.5f, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f + blockHeight, 0.5f, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f + blockHeight, -0.5f, rec.U1, rec.V1, col );
		}

		void DrawBottomFace() {
			int texId = BlockInfo.GetOptimTextureLoc( block, TileSide.Bottom );
			TextureRectangle rec = atlas.GetTexRec( texId );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, -0.5f, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f, -0.5f, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f, 0.5f, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f, 0.5f, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, 0.5f, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, -0.5f, rec.U1, rec.V1, col );
		}

		void DrawBackFace() {
			int texId = BlockInfo.GetOptimTextureLoc( block, TileSide.Back );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * atlas.invVerElementSize;
			}
			
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, 0.5f, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f + blockHeight, 0.5f, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, 0.5f, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, 0.5f, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f, 0.5f, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, 0.5f, rec.U1, rec.V2, col );
		}

		void DrawFrontFace() {
			int texId = BlockInfo.GetOptimTextureLoc( block, TileSide.Front );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * atlas.invVerElementSize;
			}
			
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, -0.5f, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f + blockHeight, -0.5f, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, -0.5f, rec.U1, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, -0.5f, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f, -0.5f, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, -0.5f, rec.U2, rec.V2, col );
		}

		void DrawLeftFace() {
			int texId = BlockInfo.GetOptimTextureLoc( block, TileSide.Left );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * atlas.invVerElementSize;
			}
			
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, -0.5f, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f + blockHeight, -0.5f, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f + blockHeight, 0.5f, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f + blockHeight, 0.5f, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, 0.5f, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, -0.5f, rec.U1, rec.V2, col );
		}

		void DrawRightFace() {
			int texId = BlockInfo.GetOptimTextureLoc( block, TileSide.Right );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * atlas.invVerElementSize;
			}
			
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f, -0.5f, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, -0.5f, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, 0.5f, rec.U1, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, 0.5f, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f, 0.5f, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f, -0.5f, rec.U2, rec.V2, col );
		}
		
		void DrawSprite() {
			int texId = BlockInfo.GetOptimTextureLoc( block, TileSide.Right );
			TextureRectangle rec = atlas.GetTexRec( texId );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, 0f, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f + blockHeight, 0f, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, 0f, rec.U1, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f + blockHeight, 0f, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0.5f, 0f, 0f, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( -0.5f, 0f, 0f, rec.U2, rec.V2, col );
			

			vertices[index++] = new VertexPos3fTex2fCol4b( 0f, 0f, -0.5f, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0f, 0f + blockHeight, -0.5f, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0f, 0f + blockHeight, 0.5f, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( 0f, 0f + blockHeight, 0.5f, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0f, 0f, 0.5f, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( 0f, 0f, -0.5f, rec.U1, rec.V2, col );
		}
	}
}