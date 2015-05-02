using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public struct TextureRectangle {
		public float U1, V1, U2, V2;
		
		public TextureRectangle( float u, float v, float uWidth, float vHeight ) {
			U1 = u;
			V1 = v;
			U2 = u + uWidth;
			V2 = v + vHeight;
		}
		
		public static TextureRectangle FromPoints( float u1, float u2, float v1, float v2 ) {
			TextureRectangle rec;
			rec.U1 = u1;
			rec.U2 = u2;
			rec.V1 = v1;
			rec.V2 = v2;
			return rec;
		}

		public override string ToString() {
			return String.Format( "{0}, {1} : {2}, {3}", U1, V1, U2, V2 );
		}
		
		internal void SwapU() {
			float u2 = U2;
			U2 = U1;
			U1 = u2;
		}
	}
	
	public static class TileSide {
		public const int Left = 0;
		public const int Right = 1;
		public const int Front = 2;
		public const int Back = 3;
		public const int Bottom = 4;
		public const int Top = 5;
	}
	
	public class TextureAtlas2D : IDisposable {
		
		public int ElementsPerRow;
		public int RowsCount;
		public int UsedRowsCount;
		public Bitmap AtlasBitmap;
		public int horElementSize, verElementSize;
		public float invHorElementSize, invVerElementSize;
		public IGraphicsApi GraphicsApi;
		
		public TextureAtlas2D( IGraphicsApi graphics, string path, int elementsPerRow, int rows, int usedRows ) {
			Bitmap bmp = new Bitmap( path );
			GraphicsApi = graphics;
			Init( bmp, elementsPerRow, rows, usedRows );
		}
		
		public TextureAtlas2D( IGraphicsApi graphics, Bitmap bmp, int elementsPerRow, int rows, int usedRows ) {
			GraphicsApi = graphics;
			Init( bmp, elementsPerRow, rows, usedRows );
		}
		
		void Init( Bitmap bmp, int elementsPerRow, int rows, int usedRows ) {
			AtlasBitmap = bmp;
			ElementsPerRow = elementsPerRow;
			RowsCount = rows;
			horElementSize = bmp.Width / ElementsPerRow;
			verElementSize = bmp.Height / RowsCount;
			UsedRowsCount = usedRows;
			invHorElementSize = (float)horElementSize / bmp.Width;
			invVerElementSize = (float)verElementSize / bmp.Height;
		}
		
		/// <summary> Loads the texture at the specified coordinates within the atlas. </summary>
		/// <param name="x"> horizontal position. (i.e. 1st image = 0, 2nd image = 1, etc</param>
		/// <param name="bottomY"> vertical position. (i.e. 1st row = 0, 2nd row = 1, etc)</param>
		/// <returns> The ID of the loaded texture at the specified coordinates.</returns>
		public int LoadTextureElement( int x, int y ) {
			using( Bitmap bmp = GetTextureElement( x, y ) ) {
				return GraphicsApi.LoadTexture( bmp );
			}
		}
		
		public int LoadTextureElement( int index ) {
			int x = 0, y = 0;
			GetCoords( index, ref x, ref y );
			return LoadTextureElement( x, y );
		}
		
		public TextureRectangle GetTexRec( int x, int y ) {
			return new TextureRectangle( x * invHorElementSize, y * invVerElementSize,
			                            invHorElementSize, invVerElementSize );
		}
		
		public TextureRectangle GetTexRec( int index ) {
			int x = 0, y = 0;
			GetCoords( index, ref x, ref y );
			return new TextureRectangle( x * invHorElementSize, y * invVerElementSize,
			                            invHorElementSize, invVerElementSize );
		}
		
		public Bitmap GetTextureElement( int x, int y ) {
			using( FastBitmap atlas = new FastBitmap( AtlasBitmap, true ) ) {
				Bitmap bmp = new Bitmap( horElementSize, verElementSize );
				using( FastBitmap dest = new FastBitmap( bmp, true) ) {
					CopyPortion( x, y, 0, 0, atlas, dest );
				}
				return bmp;
			}
		}
		
		/// <summary> Converts the *used* portion of this 2D texture atlas into an array of 1D textures. </summary>
		/// <param name="maxVerSize"> Maximum height for any bitmap. (the resulting bitmaps may
		/// have less height than this amount)</param>
		/// <param name="elementsPerBitmap"> Number of elements in each bitmap.
		/// (note that if there is more than one bitmap, the last bitmap may have less elements)</param>
		/// <remarks> Element size is bmps[0].Height / elementsPerBitmap</remarks>
		/// <returns> The array of bitmaps. </returns>
		public Bitmap[] Into1DAtlases( int maxVerSize, out int elementsPerBitmap ) {
			int verElements = maxVerSize / verElementSize;
			int totalElements = UsedRowsCount * ElementsPerRow;
			
			int atlasesCount = totalElements / verElements + ( totalElements % verElements != 0 ? 1 : 0 );
			Bitmap[] atlases = new Bitmap[atlasesCount];
			elementsPerBitmap = Math.Min( verElements, totalElements ); // in case verElements > totalElements
			int index = 0;
			
			int x = 0, y = 0;
			using( FastBitmap atlas = new FastBitmap( AtlasBitmap, true ) ) {

				for( int i = 0; i < atlases.Length; i++ ) {
					Bitmap atlas1d = new Bitmap( horElementSize, elementsPerBitmap * verElementSize );
					using( FastBitmap dest = new FastBitmap( atlas1d, true ) ) {
						for( int j = 0; j < elementsPerBitmap; j++ ) {
							GetCoords( index, ref x, ref y );
							CopyPortion( x, y, 0, j * verElementSize, atlas, dest );
							index++;
						}
					}
					atlases[i] = atlas1d;
				}
			}
			return atlases;
		}
		
		void GetCoords( int id, ref int x, ref int y ) {
			x = id % ElementsPerRow;
			y = id / ElementsPerRow;
		}
		
		unsafe void CopyPortion( int tileX, int tileY, int dstX, int dstY, FastBitmap atlas, FastBitmap dst ) {
			int atlasX = tileX * horElementSize;
			int atlasY = tileY * verElementSize;
			for( int y = 0; y < verElementSize; y++ ) {
				int* srcRow = atlas.GetRowPtr( atlasY + y );
				int* dstRow = dst.GetRowPtr( dstY + y );
				for( int x = 0; x < horElementSize; x++ ) {
					dstRow[dstX + x] = srcRow[atlasX + x];
				}
			}
		}
		
		public void Dispose() {
			if( AtlasBitmap != null ) {
				AtlasBitmap.Dispose();
			}
		}
	}
}