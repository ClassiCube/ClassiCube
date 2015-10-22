using System;
using OpenTK;
using ClassicalSharp;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		public Vector3[] MinBB = new Vector3[BlocksCount];
		public Vector3[] MaxBB = new Vector3[BlocksCount];
		
		void InitBoundingBoxes() {
			for( int i = 0; i < 256; i++ ) {
				if( IsSprite[i] ) {
					MinBB[i] = new Vector3( 2.50f/16f, 0, 2.50f/16f );
					MaxBB[i] = new Vector3( 13.5f/16f, 1, 13.5f/16f );
				} else {
					MinBB[i] = Vector3.Zero;
					MaxBB[i] = new Vector3( 1, Height[i], 1 );
				}
			}
		}
		
		public void RecalculateSpriteBB( FastBitmap fastBmp ) {
			int elemSize = fastBmp.Width / 16;
			for( int i = 0; i < 256; i++ ) {
				if( IsSprite[i] ) {
					int texId = GetTextureLoc( (byte)i, TileSide.Top );
					float topY = GetSpriteBB_TopY( elemSize, texId & 0x0F, texId >> 4, fastBmp );
					float bottomY = GetSpriteBB_BottomY( elemSize, texId & 0x0F, texId >> 4, fastBmp );
					float leftX = GetSpriteBB_LeftX( elemSize, texId & 0x0F, texId >> 4, fastBmp ); 
					float rightX = GetSpriteBB_RightX( elemSize, texId & 0x0F, texId >> 4, fastBmp ); 
					
					MinBB[i] = Utils.RotateY( leftX - 0.5f, bottomY, 0, 45f * Utils.Deg2Rad )
						+ new Vector3( 0.5f, 0, 0.5f );
					MaxBB[i] = Utils.RotateY( rightX - 0.5f, topY, 0, 45f * Utils.Deg2Rad )
						+ new Vector3( 0.5f, 0, 0.5f );
				}
			}
		}
		
		const int alphaTest = unchecked( (int)0x80000000 );
		unsafe float GetSpriteBB_TopY( int size, int tileX, int tileY, FastBitmap fastBmp ) {
			for( int y = 0; y < size; y++ ) {
				int* row = fastBmp.GetRowPtr( tileY * size + y ) + (tileX * size);
				for( int x = 0; x < size; x++ ) {
					if( (row[x] & alphaTest) != 0 ) 
						return 1 - (float)y / size;
				}
			}
			return 0;
		}
		
		unsafe float GetSpriteBB_BottomY( int size, int tileX, int tileY, FastBitmap fastBmp ) {
			for( int y = size - 1; y >= 0; y-- ) {
				int* row = fastBmp.GetRowPtr( tileY * size + y ) + (tileX * size);
				for( int x = 0; x < size; x++ ) {
					if( (row[x] & alphaTest) != 0 ) 
						return 1 - (float)(y + 1) / size;
				}
			}
			return 1;
		}
		
		unsafe float GetSpriteBB_LeftX( int size, int tileX, int tileY, FastBitmap fastBmp ) {
			for( int x = 0; x < size; x++ ) {			
				for( int y = 0; y < size; y++ ) {
					int* row = fastBmp.GetRowPtr( tileY * size + y ) + (tileX * size);
					if( (row[x] & alphaTest) != 0 ) 
						return (float)x / size;
				}
			}
			return 1;
		}
		
		unsafe float GetSpriteBB_RightX( int size, int tileX, int tileY, FastBitmap fastBmp ) {
			for( int x = size - 1; x >= 0; x-- ) {	
				for( int y = 0; y < size; y++ ) {
					int* row = fastBmp.GetRowPtr( tileY * size + y ) + (tileX * size);
					if( (row[x] & alphaTest) != 0 ) 
						return (float)(x + 1) / size;
				}
			}
			return 0;
		}
	}
}