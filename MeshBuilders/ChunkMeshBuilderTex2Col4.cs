using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ChunkMeshBuilderTex2Col4 : ChunkMeshBuilder {
		
		TextureAtlas2D atlas;
		DrawInfo1DPart Solid = new DrawInfo1DPart();
		DrawInfo1DPart Translucent = new DrawInfo1DPart();
		DrawInfo1DPart Sprite = new DrawInfo1DPart();
		
		public ChunkMeshBuilderTex2Col4( Game window ) : base( window ) {
		}
		
		class DrawInfo1DPart {
			public VertexPos3fTex2fCol4b[] vertices;
			public int index, count;
			
			public DrawInfo1DPart() {
				vertices = new VertexPos3fTex2fCol4b[0];
			}
			
			public void ExpandToCapacity() {
				if( count > vertices.Length ) {
					vertices = new VertexPos3fTex2fCol4b[count];
				}
			}
			
			public void ResetState() {
				index = 0;
				count = 0;
			}
		}
		
		public override bool UsesLighting {
			get { return true; }
		}
		
		FastColour GetColour( int x, int y, int z, ref FastColour sunlight, ref FastColour shadow ) {
			if( !map.IsValidPos( x, y, z ) ) return sunlight;
			return y > GetLightHeight( x, z ) ? sunlight : shadow;
		}
		
		FastColour GetColourAdj( int x, int y, int z, ref FastColour sunlight, ref FastColour shadow ) {
			if( !map.IsValidPos( x, y, z ) ) return sunlight;
			return y > GetLightHeightAdj( x, z ) ? sunlight : shadow;
		}
		
		int GetLightHeight( int x, int z ) {
			int y = map.GetLightHeight( x, z );
			if( y == -1 ) return -1;
			return y;
		}
		
		int GetLightHeightAdj( int x, int z ) {
			int y = map.GetLightHeight( x, z );
			if( y == -1 ) return -1;
			return BlockInfo.BlockHeight( map.GetBlock( x, y, z ) ) == 1 ? y : y - 1;
		}
		
		protected override SectionDrawInfo GetChunkInfo( int x, int y, int z ) {
			SectionDrawInfo info = new SectionDrawInfo();
			info.SolidParts = GetPartInfo( Solid );
			info.TranslucentParts = GetPartInfo( Translucent );
			info.SpriteParts = GetPartInfo( Sprite );
			return info;
		}
		
		ChunkPartInfo GetPartInfo( DrawInfo1DPart part ) {
			ChunkPartInfo info = new ChunkPartInfo( 0, part.count );
			if( part.count > 0 ) {
				info.VboID = Graphics.InitVb( part.vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b, part.count );
			}
			return info;
		}
		
		bool isTranslucent;
		float blockHeight;
		float invVerElementSize;
		protected override void PreRenderTile() {
			blockHeight = -1;
		}
		
		protected override void PreStretchTiles( int x1, int y1, int z1 ) {
			invVerElementSize = Window.TerrainAtlas.invVerElementSize;
			atlas = Window.TerrainAtlas;
			
			Solid.ResetState();
			Sprite.ResetState();
			Translucent.ResetState();
		}
		
		protected override void PostStretchTiles( int x1, int y1, int z1 ) {
			Solid.ExpandToCapacity();
			Sprite.ExpandToCapacity();
			Translucent.ExpandToCapacity();
		}
		
		public override void BeginRender() {
			Graphics.BeginVbBatch( VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		public override void Render( ChunkPartInfo info ) {
			Graphics.DrawVbBatch( DrawMode.Triangles, info.VboID, info.VerticesCount );
		}
		
		public override void EndRender() {
			Graphics.EndVbBatch();
		}
		
		protected override void AddSpriteVertices( byte tile ) {
			Sprite.count += 6 + 6;
		}
		
		protected override void AddVertices( byte tile, int face ) {
			if( BlockInfo.IsTranslucent( tile ) ) {
				Translucent.count += 6;
			} else {
				Solid.count += 6;
			}
		}

		protected override void DrawTopFace() {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Top );
			TextureRectangle rec = atlas.GetTexRec( texId );
			FastColour col = GetColour( X, Y + 1, Z, ref map.Sunlight, ref map.Shadowlight );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;

			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U2, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V2, col );
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + 1, rec.U2, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U2, rec.V1, col );
		}

		protected override void DrawBottomFace() {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Bottom );
			TextureRectangle rec = atlas.GetTexRec( texId );
			FastColour col = GetColour( X, Y - 1, Z, ref map.SunlightYBottom, ref map.ShadowlightYBottom );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + 1, rec.U2, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V1, col );
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U2, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + 1, rec.U2, rec.V2, col );
		}

		protected override void DrawBackFace() {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Back );
			TextureRectangle rec = atlas.GetTexRec( texId );
			FastColour col = GetColourAdj( X, Y, Z + 1, ref map.SunlightZSide, ref map.ShadowlightZSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + 1, rec.U2, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
		}

		protected override void DrawFrontFace() {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Front );
			TextureRectangle rec = atlas.GetTexRec( texId );
			FastColour col = GetColourAdj( X, Y, Z - 1, ref map.SunlightZSide, ref map.ShadowlightZSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U1, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U2, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U2, rec.V1, col );
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U2, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U1, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U1, rec.V2, col );
		}

		protected override void DrawLeftFace() {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Left );
			TextureRectangle rec = atlas.GetTexRec( texId );
			FastColour col = GetColourAdj( X - 1, Y, Z, ref map.SunlightXSide, ref map.ShadowlightXSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V2, col );
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U2, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
		}

		protected override void DrawRightFace() {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			TextureRectangle rec = atlas.GetTexRec( texId );
			FastColour col = GetColourAdj( X + 1, Y, Z, ref map.SunlightXSide, ref map.ShadowlightXSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U2, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + 1, rec.U1, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + 1, rec.U1, rec.V2, col );
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U2, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U2, rec.V1, col );
		}
		
		protected override void DrawSprite() {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			TextureRectangle rec = atlas.GetTexRec( texId );
			FastColour col = GetColour( X, Y + 1, Z, ref map.Sunlight, ref map.Shadowlight );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
			}
			DrawInfo1DPart part = Sprite;
			
			// Draw Z axis
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 0.5f, rec.U2, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 0.5f, rec.U2, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + 0.5f, rec.U1, rec.V1, col );
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + 0.5f, rec.U1, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + 0.5f, rec.U1, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 0.5f, rec.U2, rec.V2, col );
			
			// Draw X axis
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z, rec.U1, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z, rec.U1, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
			
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z + 1, rec.U2, rec.V2, col );
			part.vertices[part.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z, rec.U1, rec.V2, col );
		}
	}
}