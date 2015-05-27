using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class TextureAtlas1D {
		
		int usedElementsPerAtlas1D;
		internal int elementsPerBitmap;
		public float invElementSize;
		
		public TextureRectangle GetTexRec( int texId, int uCount, out int index ) {
			index = texId / usedElementsPerAtlas1D;
			int y = texId % usedElementsPerAtlas1D;
			return new TextureRectangle( 0, y * invElementSize, uCount, invElementSize );
		}
		
		public int Get1DIndex( int texId ) {
			return texId / usedElementsPerAtlas1D;
		}
		
		public int Get1DRowId( int texId ) {
			return texId % usedElementsPerAtlas1D;
		}
		
		public int[] CreateFrom2DAtlas( IGraphicsApi graphics, TerrainAtlas2D atlas2D, int maxVerSize ) {
			int verElements = maxVerSize / atlas2D.elementSize;
			int totalElements = atlas2D.UsedRowsCount * TerrainAtlas2D.ElementsPerRow;
			int elemSize = atlas2D.elementSize;
			
			int atlasesCount = totalElements / verElements + ( totalElements % verElements != 0 ? 1 : 0 );
			usedElementsPerAtlas1D = Math.Min( verElements, totalElements ); // in case verElements > totalElements
			int atlas1DHeight = Utils.NextPowerOf2( usedElementsPerAtlas1D * atlas2D.elementSize );
			
			int index = 0;
			int[] texIds = new int[atlasesCount];
			Utils.LogDebug( "Loaded new atlas: {0} bmps, {1} per bmp", atlasesCount, usedElementsPerAtlas1D );
			
			using( FastBitmap atlas = new FastBitmap( atlas2D.AtlasBitmap, true ) ) {
				for( int i = 0; i < texIds.Length; i++ ) {
					Bitmap atlas1d = new Bitmap( atlas2D.elementSize, atlas1DHeight );
					using( FastBitmap dst = new FastBitmap( atlas1d, true ) ) {
						for( int y_1D = 0; y_1D < usedElementsPerAtlas1D; y_1D++ ) {
							int x = index & 0x0F;
							int y = index >> 4;
							Utils.MovePortion( x * elemSize, y * elemSize, 0, y_1D * elemSize, atlas, dst, elemSize );
							index++;
						}
						texIds[i] = graphics.LoadTexture( dst );
					}
					atlas1d.Dispose();
				}
			}
			elementsPerBitmap = atlas1DHeight / atlas2D.elementSize;
			invElementSize = 1f / elementsPerBitmap;
			return texIds;
		}
	}
}