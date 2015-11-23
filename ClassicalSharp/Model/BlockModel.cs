using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Model {

	public class BlockModel : IModel {
		
		byte block = (byte)Block.Air;
		public BlockModel( Game game ) : base( game ) {
		}
		
		public override float NameYOffset {
			get { return blockHeight + 0.075f; }
		}
		
		public override float GetEyeY( Player player ) {
			byte block = Byte.Parse( player.ModelName );
			return block == 0 ? 1 : game.BlockInfo.Height[block];
		}
		
		const float adjust = 0.1f;
		public override Vector3 CollisionSize {
			get { return new Vector3( 1 - adjust, blockHeight - adjust, 1 - adjust ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -scale, 0f, -scale, scale, blockHeight, scale ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			// TODO: using 'is' is ugly, but means we can avoid creating
			// a string every single time held block changes.
			if( p is FakePlayer ) {
				Vector3I eyePos = Vector3I.Floor( game.LocalPlayer.EyePosition );
				FastColour baseCol = game.Map.IsLit( eyePos ) ? game.Map.Sunlight : game.Map.Shadowlight;
				col = FastColour.Scale( baseCol, 0.8f );
				block = ((FakePlayer)p).Block;
			} else {
				block = Byte.Parse( p.ModelName );	
			}
			if( block == 0 ) {
				blockHeight = 1;
				return;
			}
			
			graphics.BindTexture( game.TerrainAtlas.TexId );
			blockHeight = game.BlockInfo.Height[block];
			atlas = game.TerrainAtlas;
			BlockInfo = game.BlockInfo;
			
			if( BlockInfo.IsSprite[block] ) {
				float offset = TerrainAtlas2D.invElementSize / 2;
				XQuad( 0f, TileSide.Right, -scale, 0, 0, -offset, false );
				ZQuad( 0f, TileSide.Back, 0, scale, offset, 0, false );
				
				XQuad( 0f, TileSide.Right, 0, scale, offset, 0, false );
				ZQuad( 0f, TileSide.Back, -scale, 0, 0, -offset, false );
			} else {
				YQuad( 0f, TileSide.Bottom );
				XQuad( scale, TileSide.Left, -scale, scale, 0, 0, true );
				ZQuad( -scale, TileSide.Front, -scale, scale, 0, 0, true );
				
				ZQuad( scale, TileSide.Back, -scale, scale, 0, 0, true );
				YQuad( blockHeight, TileSide.Top );
				XQuad( -scale, TileSide.Right, -scale, scale, 0, 0, true );			
			}
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
		}
		float blockHeight;
		TerrainAtlas2D atlas;
		BlockInfo BlockInfo;
		float scale = 0.5f;
		
		public override void Dispose() {
		}
		
		void YQuad( float y, int side ) {
			int texId = BlockInfo.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetAdjTexRec( texId );

			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - scale, pos.Y + y, pos.Z - scale, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + scale, pos.Y + y, pos.Z - scale, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + scale, pos.Y + y, pos.Z + scale, rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - scale, pos.Y + y, pos.Z + scale, rec.U1, rec.V2, col );
		}

		void ZQuad( float z, int side, float x1, float x2, 
		           float u1Offset, float u2Offset, bool swapU ) {
			int texId = BlockInfo.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetAdjTexRec( texId );
			if( blockHeight != 1 )
				rec.V2 = rec.V1 + blockHeight * TerrainAtlas2D.invElementSize * (15.99f/16f);
			
			// need to break into two quads when drawing a sprite model in hand.
			if( swapU ) rec.SwapU();
			rec.U1 += u1Offset;
			rec.U2 += u2Offset;
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x1, pos.Y + 0f, pos.Z + z, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x1, pos.Y + blockHeight, pos.Z + z, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x2, pos.Y + blockHeight, pos.Z + z, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x2, pos.Y + 0f, pos.Z + z, rec.U2, rec.V2, col );
		}

		void XQuad( float x, int side, float z1, float z2, 
		           float u1Offset, float u2Offset, bool swapU ) {
			int texId = BlockInfo.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetAdjTexRec( texId );
			if( blockHeight != 1 )
				rec.V2 = rec.V1 + blockHeight * TerrainAtlas2D.invElementSize * (15.99f/16f);
			
			if( swapU ) rec.SwapU();
			rec.U1 += u1Offset;
			rec.U2 += u2Offset;
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + 0f, pos.Z + z1, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + blockHeight, pos.Z + z1, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + blockHeight, pos.Z + z2, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + 0f, pos.Z + z2, rec.U2, rec.V2, col );
		}
	}
}