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
		public const int Sides = 6;
	}
	
	/// <summary> Represents a 2D packed texture atlas, specifically for terrain.png. </summary>
	public class TerrainAtlas2D : IDisposable {
		
		public const int ElementsPerRow = 16, RowsCount = 16;
		public const float invElementSize = 1 / 16f;
		public Bitmap AtlasBitmap;
		public int elementSize;
		public int TexId;
		IGraphicsApi graphics;
		IDrawer2D drawer;
		
		public TerrainAtlas2D( IGraphicsApi graphics, IDrawer2D drawer ) {
			this.graphics = graphics;
			this.drawer = drawer;
		}
		
		public void UpdateState( Bitmap bmp ) {
			if( !FastBitmap.CheckFormat( bmp.PixelFormat ) ) {
				Utils.LogWarning( "Converting terrain atlas to 32bpp image" );
				Bitmap newBmp = drawer.ConvertTo32Bpp( bmp );
				bmp.Dispose();
				bmp = newBmp;
			}
			AtlasBitmap = bmp;
			elementSize = bmp.Width >> 4;
			using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
				TexId = graphics.CreateTexture( fastBmp );
			}
		}
		
		public int LoadTextureElement( int index ) {
			int x = index & 0x0F;
			int y = index >> 4;
			int size = elementSize;
			
			using( FastBitmap atlas = new FastBitmap( AtlasBitmap, true ) ) {
				using( Bitmap bmp = new Bitmap( size, size ) ) {
					using( FastBitmap dst = new FastBitmap( bmp, true ) ) {
						FastBitmap.MovePortion( x * size, y * size, 0, 0, atlas, dst, size );
						return graphics.CreateTexture( dst );
					}
				}
			}
		}
		
		public TextureRec GetTexRec( int index ) {
			int x = index & 0x0F;
			int y = index >> 4;
			return new TextureRec( x * invElementSize, y * invElementSize,
			                            invElementSize, invElementSize );
		}
		
		public void Dispose() {
			if( AtlasBitmap != null ) {
				AtlasBitmap.Dispose();
			}
			graphics.DeleteTexture( ref TexId );
		}
	}
}