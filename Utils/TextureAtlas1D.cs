using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class TextureAtlas1D {
		
		int usedElementsPerAtlas1D;
		internal int elementsPerBitmap;
		public float invElementSize;
		
		public TextureRectangle GetTexRec( int texId, out int index ) {
			index = texId / usedElementsPerAtlas1D;
			int y = texId % usedElementsPerAtlas1D;
			return new TextureRectangle( 0, y * invElementSize, 1, invElementSize );
		}
		
		public int Get1DIndex( int texId ) {
			return texId / usedElementsPerAtlas1D;
		}
		
		public int Get1DRowId( int texId ) {
			return texId % usedElementsPerAtlas1D;
		}
		
		public int[] CreateFrom2DAtlas( IGraphicsApi graphics, TextureAtlas2D atlas2D, int maxVerSize ) {
			int verElements = maxVerSize / atlas2D.verElementSize;
			int totalElements = atlas2D.UsedRowsCount * atlas2D.ElementsPerRow;
			
			int atlasesCount = totalElements / verElements + ( totalElements % verElements != 0 ? 1 : 0 );
			usedElementsPerAtlas1D = Math.Min( verElements, totalElements ); // in case verElements > totalElements
			int atlas1DHeight = Utils.NextPowerOf2( usedElementsPerAtlas1D * atlas2D.verElementSize );
			
			int index = 0;
			int x = 0, y = 0;
			int[] texIds = new int[atlasesCount];
			Utils.LogDebug( "Loaded new atlas: {0} bmps, {1} per bmp", atlasesCount, usedElementsPerAtlas1D );
			
			using( FastBitmap atlas = new FastBitmap( atlas2D.AtlasBitmap, true ) ) {
				for( int i = 0; i < texIds.Length; i++ ) {
					Bitmap atlas1d = new Bitmap( atlas2D.horElementSize, atlas1DHeight );
					using( FastBitmap dest = new FastBitmap( atlas1d, true ) ) {
						for( int j = 0; j < usedElementsPerAtlas1D; j++ ) {
							atlas2D.GetCoords( index, ref x, ref y );
							atlas2D.CopyPortion( x, y, 0, j * atlas2D.verElementSize, atlas, dest );
							index++;
						}
						texIds[i] = graphics.LoadTexture( dest );
					}
					atlas1d.Dispose();
				}
			}
			elementsPerBitmap = atlas1DHeight / atlas2D.verElementSize;
			invElementSize = 1f / elementsPerBitmap;
			return texIds;
		}
	}
}