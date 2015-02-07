using System;
using System.Drawing;
using ClassicalSharp.Blocks.Model;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Window;

namespace ClassicalSharp.Items {
	
	public class ItemInfo {
		
		const int itemsCount = 65536;
		TextureAtlas2D atlas;
		Item[] itemCache = new Item[itemsCount];
		
		public void Init( TextureAtlas2D atlas ) {
			this.atlas = atlas;		
			foreach( short value in Enum.GetValues( typeof( ItemId ) ) ) {
				itemCache[value] = new Item( value );
			}
			
			InitTextures();
			SetMaxCountRange( 1, ItemId.IronShovel, ItemId.Bow );
			SetMaxCountRange( 1, ItemId.IronSword, ItemId.DiamondAxe );
			SetMaxCountRange( 1, ItemId.MushroomSoup, ItemId.GoldAxe );
			SetMaxCountRange( 1, ItemId.WoodenHoe, ItemId.GoldHoe );
			SetMaxCountRange( 1, ItemId.LeatherCap, ItemId.GoldBoots );
			SetMaxCountRange( 1, ItemId.RawPorkchop, ItemId.CookedPorkchop );
			SetMaxCountRange( 1, ItemId.GoldenApple, ItemId.IronDoor );
			itemCache[(int)ItemId.Snowball].MaxCount = 16;
			itemCache[(int)ItemId.Boat].MaxCount = 1;
			SetMaxCountRange( 1, ItemId.MinecartWithChest, ItemId.MinecartWithFurnace );
			itemCache[(int)ItemId.FishingRod].MaxCount = 1;
			SetMaxCountRange( 1, ItemId.RawFish, ItemId.CookedFish );
			itemCache[(int)ItemId.Cake].MaxCount = 1;
			SetMaxCountRange( 1, ItemId.Map, ItemId.Shears);
			SetMaxCountRange( 1, ItemId.GoldMusicDisc, ItemId.GreenMusicDisc );
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
			            Row6 + 12, Row4 + 12, Row6 + 13 );
			SetTextures( ItemId.GoldMusicDisc, Row16 + 0, Row16 + 1 );
		}
		
		void SetTextures( ItemId startId, params int[] texIds ) {
			int start = (int)startId;
			for( int i = 0; i < texIds.Length; i++ ) {
				itemCache[start + i].texId = texIds[i];
			}
		}
		
		void SetMaxCountRange( byte count, ItemId start, ItemId end ) {
			for( int index = (int)start; index <= (int)end; index++ ) {
				itemCache[index].MaxCount = count;
			}
		}
		
		public int Get2DTextureLoc( short itemId, short itemDamage ) {
			return itemCache[itemId].Get2DTextureLoc( itemDamage );
		}
		
		public int Get2DTextureLoc( Slot slot ) {
			return Get2DTextureLoc( slot.Id, slot.Damage );
		}
		
		public int Make3DModel( int texId, IGraphicsApi graphics, out int verticesCount ) {
			int texX = 0, texY = 0;
			atlas.GetCoords( texId, ref texX, ref texY );
			using( Bitmap bmp = atlas.GetTextureElement( texX, texY ) ) {
				using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
					int elements = 0;
					for( int y = 0; y < 16; y++ ) {
						for( int x = 0; x < 16; x++ ) {
							if( ( fastBmp.GetPixel( x, y ) & 0xFF000000 ) != 0 ) {
								if( !IsSolid( x - 1, y, fastBmp ) ) elements++;
								if( !IsSolid( x + 1, y, fastBmp ) ) elements++;
								if( !IsSolid( x, y - 1, fastBmp ) ) elements++;
								if( !IsSolid( x, y + 1, fastBmp ) ) elements++;
								elements += 2;
							}
						}
					}
					
					verticesCount = elements * 6;
					VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[verticesCount];
					int index = 0;
					for( int y = 0; y < 16; y++ ) {
						for( int x = 0; x < 16; x++ ) {
							int pixel = 0;
							if( ( ( pixel = fastBmp.GetPixel( x, y ) ) & 0xFF000000 ) != 0 ) {
								if( !IsSolid( x - 1, y, fastBmp ) ) {
									AddPixel( x, x, y, y + 1, 0, 1, vertices, ref index, pixel );
								}
								if( !IsSolid( x + 1, y, fastBmp ) ) {
									AddPixel( x + 1, x + 1, y, y + 1, 0, 1, vertices, ref index, pixel );
								}
								if( !IsSolid( x, y - 1, fastBmp ) ) {
									AddPixel( x, x + 1, y, y, 0, 1, vertices, ref index, pixel );
								}
								if( !IsSolid( x, y + 1, fastBmp ) ) {
									AddPixel( x, x + 1, y + 1, y + 1, 0, 1, vertices, ref index, pixel );
								}
								AddPixel( x, x + 1, y, y + 1, 0, 0, vertices, ref index, pixel );
								AddPixel( x, x + 1, y, y + 1, 1, 1, vertices, ref index, pixel );
							}
						}
					}
					return graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fCol4b );
				}
			}
		}
		
		static void AddPixel( int x1, int x2, int y1, int y2, int z1, int z2, VertexPos3fCol4b[] vertices, ref int index, int rawCol ) {
			FastColour col = new FastColour( ( rawCol & 0xFF0000 ) >> 16, ( rawCol & 0xFF00 ) >> 8, ( rawCol & 0xFF ) );
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y2, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z2, col );
			
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y1, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z1, col );
		}
		
		static bool IsSolid( int x, int y, FastBitmap bmp ) {
			if( x == -1 ) return false;
			if( y == -1 ) return false;
			if( x == 16 ) return false;
			if( y == 16 ) return false;
			return ( bmp.GetPixel( x, y ) & 0xFF000000 ) != 0;
		}
	}
}