using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Model {

	public class BlockModel : IModel {
		
		byte block = (byte)Block.Air;
		float blockHeight;
		TerrainAtlas1D atlas;
		const float adjust = 0.1f, extent = 0.5f;
		bool bright;
		
		public BlockModel( Game game ) : base( game ) {
		}
		
		public override bool Bobbing {
			get { return false; }
		}
		
		public override float NameYOffset {
			get { return blockHeight + 0.075f; }
		}
		
		public override float GetEyeY( Player player ) {
			byte block = Byte.Parse( player.ModelName );
			return block == 0 ? 1 : game.BlockInfo.Height[block];
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 1 - adjust, blockHeight - adjust, 1 - adjust ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -extent, 0f, -extent, extent, blockHeight, extent ); }
		}
		
		int lastTexId = -1;
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
			lastTexId = -1;
			blockHeight = game.BlockInfo.Height[block];
			atlas = game.TerrainAtlas1D;
			bright = game.BlockInfo.FullBright[block];
			
			if( game.BlockInfo.IsSprite[block] ) {
				SpriteXQuad( TileSide.Right, true );
				SpriteZQuad( TileSide.Back, false );
				
				SpriteXQuad( TileSide.Right, false );
				SpriteZQuad( TileSide.Back, true );
			} else {
				YQuad( 0f, TileSide.Bottom, FastColour.ShadeYBottom );
				XQuad( extent, TileSide.Left, -extent, extent, true, FastColour.ShadeX );
				ZQuad( -extent, TileSide.Front, -extent, extent, true, FastColour.ShadeZ );
				
				ZQuad( extent, TileSide.Back, -extent, extent, false, FastColour.ShadeZ );
				YQuad( blockHeight, TileSide.Top, 1.0f );
				XQuad( -extent, TileSide.Right, -extent, extent, true, FastColour.ShadeX );
			}
			
			if( index > 0 ) {
				graphics.BindTexture( lastTexId );
				TransformVertices();
				graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
			}
		}
		
		void YQuad( float y, int side, float shade ) {
			int texId = game.BlockInfo.GetTextureLoc( block, side ), texIndex = 0;
			TextureRec rec = atlas.GetTexRec( texId, 1, out texIndex );
			FlushIfNotSame( texIndex );
			FastColour col = bright ? FastColour.White : FastColour.Scale( this.col, shade );
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( -extent, y, -extent, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( extent, y, -extent, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( extent, y, extent, rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( -extent, y, extent, rec.U1, rec.V2, col );
		}

		void ZQuad( float z, int side, float x1, float x2, bool swapU, float shade ) {
			int texId = game.BlockInfo.GetTextureLoc( block, side ), texIndex = 0;
			TextureRec rec = atlas.GetTexRec( texId, 1, out texIndex );
			FlushIfNotSame( texIndex );
			FastColour col = bright ? FastColour.White : FastColour.Scale( this.col, shade );
			
			if( blockHeight != 1 )
				rec.V2 = rec.V1 + blockHeight * atlas.invElementSize * (15.99f/16f);
			if( swapU ) rec.SwapU();
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x1, 0f, z, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x1, blockHeight, z, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x2, blockHeight, z, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x2, 0f, z, rec.U2, rec.V2, col );
		}

		void XQuad( float x, int side, float z1, float z2, bool swapU, float shade ) {
			int texId = game.BlockInfo.GetTextureLoc( block, side ), texIndex = 0;
			TextureRec rec = atlas.GetTexRec( texId, 1, out texIndex );
			FlushIfNotSame( texIndex );
			FastColour col = bright ? FastColour.White : FastColour.Scale( this.col, shade );
			
			if( blockHeight != 1 )
				rec.V2 = rec.V1 + blockHeight * atlas.invElementSize * (15.99f/16f);
			if( swapU ) rec.SwapU();
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x, 0f, z1, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x, blockHeight, z1, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x, blockHeight, z2, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x, 0f, z2, rec.U2, rec.V2, col );
		}
		
		void SpriteZQuad( int side, bool firstPart ) {
			int texId = game.BlockInfo.GetTextureLoc( block, side ), texIndex = 0;
			TextureRec rec = atlas.GetTexRec( texId, 1, out texIndex );
			FlushIfNotSame( texIndex );
			if( blockHeight != 1 )
				rec.V2 = rec.V1 + blockHeight * atlas.invElementSize * (15.99f/16f);
			FastColour col = bright ? FastColour.White : this.col;

			float p1, p2;
			if( firstPart ) { // Need to break into two quads for when drawing a sprite model in hand.
				rec.U1 = 0.5f; p1 = -5.5f/16; p2 = 0.0f/16;	
			} else {
				rec.U2 = 0.5f; p1 = 0.0f/16; p2 = 5.5f/16;
			}
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( p1, 0f, p1, rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( p1, blockHeight, p1, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( p2, blockHeight, p2, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( p2, 0f, p2, rec.U1, rec.V2, col );
		}

		void SpriteXQuad( int side, bool firstPart ) { // dis is broken
			int texId = game.BlockInfo.GetTextureLoc( block, side ), texIndex = 0;
			TextureRec rec = atlas.GetTexRec( texId, 1, out texIndex );
			FlushIfNotSame( texIndex );
			if( blockHeight != 1 )
				rec.V2 = rec.V1 + blockHeight * atlas.invElementSize * (15.99f/16f);
			FastColour col = bright ? FastColour.White : this.col;
			
			float x1, x2, z1, z2;
			if( firstPart ) {
				rec.U1 = 0; rec.U2 = 0.5f; x1 = -5.5f/16; 
				x2 = 0.0f/16; z1 = 5.5f/16; z2 = 0.0f/16;
			} else {
				rec.U1 = 0.5f; rec.U2 = 1f; x1 = 0.0f/16;
				x2 = 5.5f/16; z1 = 0.0f/16; z2 = -5.5f/16;
			}

			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x1, 0f, z1, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x1, blockHeight, z1, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x2, blockHeight, z2, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( x2, 0f, z2, rec.U2, rec.V2, col );
		}
		
		void FlushIfNotSame( int texIndex ) {
			int texId = game.TerrainAtlas1D.TexIds[texIndex];
			if( texId == lastTexId ) return;
			
			if( lastTexId != -1 ) {
				graphics.BindTexture( lastTexId );
				TransformVertices();
				graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb,
				                                cache.vertices, index, index * 6 / 4 );
			}
			lastTexId = texId;
			index = 0;
		}
		
		void TransformVertices() {
			for( int i = 0; i < index; i++ ) {
				VertexPos3fTex2fCol4b vertex = cache.vertices[i];
				Vector3 newPos = Utils.RotateY( vertex.X, vertex.Y, vertex.Z, cosA, sinA ) + pos;
				vertex.X = newPos.X; vertex.Y = newPos.Y; vertex.Z = newPos.Z;
				cache.vertices[i] = vertex;
			}
		}
	}
}