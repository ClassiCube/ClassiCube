using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public static class TileSide {
		public const int Left = 0;
		public const int Right = 1;
		public const int Front = 2;
		public const int Back = 3;
		public const int Bottom = 4;
		public const int Top = 5;
	}
	
	public class TerrainAtlas2D : IDisposable {
		
		public const int ElementsPerRow = 16, RowsCount = 16;
		public const float invElementSize = 0.0625f;
		public readonly int UsedRowsCount = 5;
		public Bitmap AtlasBitmap;
		public int elementSize;		
		public IGraphicsApi GraphicsApi;
		public int TexId;
		
		public void UpdateState( Bitmap bmp ) {
			AtlasBitmap = bmp;
			elementSize = bmp.Width >> 4;
			using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
				MakeOptimisedTexture( fastBmp );
				TexId = GraphicsApi.LoadTexture( fastBmp );
			}
		}
		
		public int LoadTextureElement( int index ) {
			int x = index & 0x0F;
			int y = index >> 4;
			using( FastBitmap atlas = new FastBitmap( AtlasBitmap, true ) ) {
				Bitmap bmp = new Bitmap( elementSize, elementSize );
				using( FastBitmap dst = new FastBitmap( bmp, true ) ) {
					Utils.MovePortion( x * elementSize, y * elementSize, 0, 0, atlas, dst, elementSize );
					return GraphicsApi.LoadTexture( dst );
				}
			}
		}
		
		public TextureRectangle GetTexRec( int index ) {
			int x = index & 0x0F;
			int y = index >> 4;
			return new TextureRectangle( x * invElementSize, y * invElementSize, invElementSize, invElementSize );
		}
		
		public void Dispose() {
			if( AtlasBitmap != null ) {
				AtlasBitmap.Dispose();
			}
			GraphicsApi.DeleteTexture( ref TexId );
		}
		
		static ushort[] rowFlags = { 0xFFFF, 0xFFEE, 0xFFE0, 0xFFE0, 0xFFFF, 0xFA00 };		
		void MakeOptimisedTexture( FastBitmap atlas ) {
			int srcIndex = 0, destIndex = 0;
			
			for( int y = 0; y < 6; y++ ) {
				int flags = rowFlags[y];
				for( int x = 0; x < ElementsPerRow; x++ ) {
					bool isUsed = ( flags & 1 << ( 15 - x ) ) != 0;
					if( isUsed && srcIndex != destIndex ) {
						int dstX = ( destIndex & 0x0F ) * elementSize;
						int dstY = ( destIndex >> 4 ) * elementSize;
						Utils.MovePortion( x * elementSize, y * elementSize, dstX, dstY, atlas, atlas, elementSize );
					}
					
					srcIndex++;
					if( isUsed ) destIndex++;
				}
			}
		}
	}
}