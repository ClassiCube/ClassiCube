using System;
using System.Drawing;

namespace ClassicalSharp.Blocks.Model {
	
	public class BiomeColouredModel : IBlockModel {
		
		int[] colours;
		IBlockModel drawer;
		public BiomeColouredModel( string colourMapFile, IBlockModel drawer ) : base( drawer.atlas, drawer.info, drawer.block ) {
			this.drawer = drawer;
			Pass = drawer.Pass;
			GetColours( colourMapFile );
		}
		
		unsafe void GetColours( string colourMapFile ) {
			using( Bitmap bmp = new Bitmap( colourMapFile ) ) {
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
		
		public override bool HasFace( int face ) {
			return drawer.HasFace( face );
		}
		
		public override bool FaceHidden( int face, byte neighbour ) {
			return drawer.FaceHidden( face, neighbour );
		}
		
		public override int GetVerticesCount( int face, byte neighbour ) {
			return drawer.GetVerticesCount( face, neighbour );
		}
		
		int GetColour( int x, int y ) {
			return colours[( y << 8 ) + x];
		}
		
		//http://minecraft.gamepedia.com/index.php?title=Biome#Technical_details
		int GetColour( float temperature, float rainfall ) {
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
		
		float[] scales = new float[] { 0.6f, 0.6f, 0.8f, 0.8f, 0.5f, 1f };
		public override void DrawFace( int face, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			// TODO: Also according to the page, we have to average the eight neighbours for proper biome blending.
			// Though since we don't even use biomes yet, I guess that doesn't really matter.
			int colour = GetColour( (float)( x % 64 ) / 63, (float)( z % 64 ) / 63 );
			col = new FastColour( ( colour & 0xFF0000 ) >> 16, ( colour & 0xFF00 ) >> 8, ( colour & 0xFF ) );
			col = FastColour.Scale( col, scales[face] );			
			drawer.DrawFace( face, ref index, x, y, z, vertices, col );
		}
	}
}
