using System;
using System.Drawing;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
		
		int[] colours;		
		unsafe void MakeColours() {
			using( Bitmap bmp = new Bitmap( "grasscolor.png" ) ) {
				using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
					colours = new int[256 * 256];
					int index = 0;
					for( int y = 0; y < 256; y++ ) {
						int* rowPtr = fastBmp.GetRowPtr( y );
						for( int x = 0; x < 256; x++ ) {
							colours[index++] = rowPtr[x];
						}
					}
				}
			}
		}
		
		public int GetColour( int x, int y ) {
			return colours[( y << 8 ) + x];
		}
		
		//http://minecraft.gamepedia.com/index.php?title=Biome#Technical_details
		public int GetColour( float temperature, float rainfall ) {
			float adjTemp = Clamp( temperature );
			float adjRainfall = Clamp( rainfall ) * adjTemp;
			// "Treating the lower left corner as temperature = 1.0 and rainfall = 0.0, with adjusted temperature decreasing to 0.0 at the right edge and 
			// adjusted rainfall increasing to 1.0 at the top edge"
			int x = 255 - (int)( adjTemp * 255 );
			int y = 255 - (int)( adjRainfall * 255 );
			return GetColour( x, y );
		}
		
		static float Clamp( float value ) {
			if( value < 0 ) return 0;
			if( value > 1 ) return 1;
			return value;
		}
	}
}