// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {
	
	/// <summary> Represents a 2D packed texture atlas, specifically for terrain.png. </summary>
	public class TerrainAtlas2D : IDisposable {
		
		public const int ElementsPerRow = 16, RowsCount = 16;
		public const float invElementSize = 1 / 16f;
		public Bitmap AtlasBitmap;
		public int elementSize;
		IGraphicsApi graphics;
		IDrawer2D drawer;
		
		public TerrainAtlas2D( IGraphicsApi graphics, IDrawer2D drawer ) {
			this.graphics = graphics;
			this.drawer = drawer;
		}
		
		/// <summary> Updates the underlying atlas bitmap, fields, and texture. </summary>
		public void UpdateState( BlockInfo info, Bitmap bmp ) {
			if( !FastBitmap.CheckFormat( bmp.PixelFormat ) ) {
				Utils.LogDebug( "Converting terrain atlas to 32bpp image" );
				drawer.ConvertTo32Bpp( ref bmp );
			}
			
			AtlasBitmap = bmp;
			elementSize = bmp.Width >> 4;
			using( FastBitmap fastBmp = new FastBitmap( bmp, true, true ) )
				info.RecalculateSpriteBB( fastBmp );
		}
		
		/// <summary> Creates a new texture that contains the tile at the specified index. </summary>
		public int LoadTextureElement( int index ) {
			int size = elementSize;
			using( FastBitmap atlas = new FastBitmap( AtlasBitmap, true, true ) )
				using( Bitmap bmp = new Bitmap( size, size ) )
					using( FastBitmap dst = new FastBitmap( bmp, true, false ) )
			{
				FastBitmap.MovePortion( (index & 0x0F) * size, (index >> 4) * 
				                       size, 0, 0, atlas, dst, size );
				return graphics.CreateTexture( dst );
			}
		}
		
		/// <summary> Disposes of the underlying atlas bitmap and texture. </summary>
		public void Dispose() {
			if( AtlasBitmap != null )
				AtlasBitmap.Dispose();
		}
	}
}