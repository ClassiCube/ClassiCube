using System;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
		
		// The designation used is as follows:
		// 0 - left 1 - right 2 - front
		// 3 - back 4 - bottom 5 - top	
		int[] optimTextures = new int[blocksCount * 6];	
		const int Row1 = 0, Row2 = 16, Row3 = 32, Row4 = 48,
		Row5 = 64, Row6 = 80, Row7 = 96, Row8 = 112, Row9 = 128;

		void SetupOptimTextures() {
			SetOptimAllTextures( Block.Stone, Row1 + 1 );
			SetOptimTopTexture( Block.Grass, Row1 );
			SetOptimSideTextures( Block.Grass, Row1 + 3 );
			SetOptimBottomTexture( Block.Grass, Row1 + 2 );
			SetOptimAllTextures( Block.Dirt, Row1 + 2 );
			SetOptimAllTextures( Block.Cobblestone, Row2 );
			SetOptimAllTextures( Block.WoodenPlanks, Row1 + 4 );
			SetOptimAllTextures( Block.Sapling, Row1 + 15 );
			SetOptimAllTextures( Block.Bedrock, Row2 + 1 );
			SetOptimAllTextures( Block.Water, Row1 + 14 );
			SetOptimAllTextures( Block.StillWater, Row1 + 14 );
			SetOptimAllTextures( Block.Lava, Row2 + 13 );
			SetOptimAllTextures( Block.StillLava, Row2 + 13 );
			SetOptimAllTextures( Block.Sand, Row2 + 2 );
			SetOptimAllTextures( Block.Gravel, Row2 + 3 );
			SetOptimAllTextures( Block.GoldOre, Row2 + 14 );
			SetOptimAllTextures( Block.IronOre, Row2 + 15 );
			SetOptimAllTextures( Block.CoalOre, Row3 );
			SetOptimSideTextures( Block.Wood, Row2 + 4 );
			SetOptimTopTexture( Block.Wood, Row2 + 5 );
			SetOptimBottomTexture( Block.Wood, Row2 + 5 );
			SetOptimAllTextures( Block.Leaves, Row2 + 6 );
			SetOptimAllTextures( Block.Sponge, Row3 + 9);
			SetOptimAllTextures( Block.Glass, Row3 + 10 );
			SetOptimAllTextures( Block.Dandelion, Row1 + 13 );
			SetOptimAllTextures( Block.Rose, Row1 + 12 );
			SetOptimAllTextures( Block.BrownMushroom, Row2 + 12 );
			SetOptimAllTextures( Block.RedMushroom, Row2 + 11 );
			SetOptimTopTexture( Block.TNT, Row1 + 9 );
			SetOptimSideTextures( Block.TNT, Row1 + 8 );
			SetOptimBottomTexture( Block.TNT, Row1 + 10 );
			SetOptimTopTexture( Block.Bookshelf, Row1 + 4 );
			SetOptimBottomTexture( Block.Bookshelf, Row1 + 4 );
			SetOptimSideTextures( Block.Bookshelf, Row3 + 1 );
			SetOptimAllTextures( Block.Obsidian, Row3 + 3 );
			
			// CPE blocks
			SetOptimTopTexture( Block.Sandstone, Row2 + 9 );
			SetOptimSideTextures( Block.Sandstone, Row3 + 7 );
			SetOptimBottomTexture( Block.Sandstone, Row4 + 2 );
			SetOptimAllTextures( Block.Snow, Row3 + 11 );
			SetOptimAllTextures( Block.Fire, Row3 + 4 );
		}

		void SetOptimAllTextures( Block blockId, int textureId ) {
			int index = (byte)blockId * 6;
			for( int i = index; i < index + 6; i++ ) {
				optimTextures[i] = textureId;
			}
		}
		
		void SetOptimSideTextures( Block blockId, int textureId ) {
			int index = (byte)blockId * 6;
			for( int i = index; i < index + TileSide.Top; i++ ) {
				optimTextures[i] = textureId;
			}
		}
		
		void SetOptimTopTexture( Block blockId, int textureId ) {
			optimTextures[(byte)blockId * 6 + TileSide.Top] = textureId;
		}
		
		void SetOptimBottomTexture( Block blockId, int textureId ) {
			optimTextures[(byte)blockId * 6 + TileSide.Bottom] = textureId;
		}
		
		/// <summary> Gets the index in the ***optimised*** 2D terrain atlas for the
		/// texture of the face of the given block. </summary>
		/// <param name="block"> Block ID. </param>
		/// <param name="face">Face of the given block, see TileSide constants. </param>
		/// <returns>The index of the texture within the terrain atlas.</returns>
		public int GetOptimTextureLoc( byte block, int face ) {
			return optimTextures[block * 6 + face];
		}
		
		static ushort[] RowFlags = new ushort[] {
			0xFFFF, // y y y y  y y y y  y y y y  y y y y
			0xFFEE, // y y y y  y y y y  y y y n  y y y n
			0xFFE0, // y y y y  y y y y  y y y n  n n n n
			0xFFE0, // y y y y  y y y y  y y y n  n n n n
			0xFFFF, // y y y y  y y y y  y y y y  y y y y
			0xFA00, // y y y y  y n y n  n n n n  n n n n
		};
		
		public static void MakeOptimisedTexture( FastBitmap atlas ) {
			int size = atlas.Width / 16;
			int srcIndex = 0, destIndex = 0;
			
			for( int y = 0; y < 6; y++ ) {
				int flags = RowFlags[y];
				for( int x = 0; x < 16; x++ ) {
					bool isUsed = ( flags & 1 << ( 15 - x ) ) != 0;
					if( isUsed && srcIndex != destIndex ) {
						int srcX = x * size;
						int srcY = y * size;
						int destX = ( destIndex & 0x0F ) * size;
						int destY = ( destIndex >> 4 ) * size;
						MovePortion( srcX, srcY, destX, destY, atlas, size );
					}
					
					srcIndex++;
					destIndex++;
					if( !isUsed ) destIndex--;
				}
			}
		}
		
		unsafe static void MovePortion( int srcX, int srcY, int dstX, int dstY, FastBitmap src, int size ) {
			for( int y = 0; y < size; y++ ) {
				int* srcRow = src.GetRowPtr( srcY + y );
				int* dstRow = src.GetRowPtr( dstY + y );
				for( int x = 0; x < size; x++ ) {					
					dstRow[dstX + x] = srcRow[srcX + x];
				}
			}
		}
	}
}