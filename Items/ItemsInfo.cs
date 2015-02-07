using System;
using ClassicalSharp.Blocks.Model;

namespace ClassicalSharp {
	
	public class ItemInfo {
		
		int[] textures = new int[itemsCount];
		const int itemsCount = 65536;
		
		public void Init( TextureAtlas2D atlas ) {
			InitTextures();
		}
		
		const int Row1 = 0, Row2 = 16, Row3 = 32, Row4 = 48,
		Row5 = 64, Row6 = 80, Row7 = 96, Row8 = 112, Row9 = 128,
		Row10 = 144, Row11 = 160, Row12 = 176, Row13 = 192,
		Row14 = 208, Row15 = 224, Row16 = 240;
		
		void InitTextures() {
			SetTextures( ItemId.IronShovel, Row6 + 2, Row7 + 2, Row8 + 2,
			            Row1 + 6, Row1 + 10, Row2 + 6, Row3 + 6, Row1 + 8,
			            Row4 + 8, Row2 + 8, Row3 + 8, Row5 + 3, Row5 + 0,
			            Row6 + 0, Row7 + 0, Row8 + 0, Row5 + 1, Row6 + 1,
			            Row7 + 1, Row8 + 1, Row5 + 3, Row6 + 3, Row7 + 3,
			            Row8 + 3, Row4 + 5, Row5 + 7, Row5 + 8, Row5 + 4,
			            Row6 + 4, Row7 + 4, Row8 + 4, Row1 + 8, Row2 + 8,
			            Row3 + 8, Row9 + 0, Row9 + 1, Row9 + 2, Row9 + 3,
			            Row9 + 3, Row1 + 9, Row2 + 9, Row3 + 9, Row1 + 0,
			            Row2 + 0, Row3 + 0, Row4 + 0, Row1 + 1, Row2 + 1,
			            Row3 + 1, Row4 + 1, Row1 + 2, Row2 + 2, Row3 + 2,
			            Row4 + 2, Row1 + 3, Row2 + 3, Row3 + 3, Row4 + 3,
			            Row1 + 4, Row2 + 4, Row3 + 4, Row4 + 4, Row1 + 7,
			            Row6 + 8, Row6 + 9, Row2 + 10, Row1 + 11, Row3 + 10,
			            Row3 + 11, Row5 + 10, Row5 + 11, Row5 + 12, Row9 + 8,
			            Row7 + 9, Row3 + 12, Row4 + 8, Row1 + 14, Row9 + 9,
			            Row7 + 8, Row5 + 13, Row2 + 6, Row3 + 9, Row2 + 11,
			            Row10 + 8, Row11 + 8, Row1 + 12, Row4 + 6, Row5 + 5,
			            Row5 + 6, Row5 + 9, Row6 + 9, Row6 + 10, Row5 + 13,
			            Row2 + 12, Row1 + 13, Row2 + 13, Row3 + 13, Row6 + 6,
			            Row6 + 12, Row4 + 12, Row6 + 13, Row16 + 0, Row16 + 1 );
			}
		
		void SetTexture( ItemId id, int texId ) {
			textures[(int)id] = texId;
		}
		
		void SetTextures( ItemId startId, params int[] texIds ) {
			int start = (int)startId;
			for( int i = 0; i < texIds.Length; i++ ) {
				textures[start + i] = texIds[i];
			}
		}
		
		public int Get2DTextureLoc( short item ) {
			return textures[item];
		}
	}
}