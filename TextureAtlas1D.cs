using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class TextureAtlas1D : IDisposable {
		
		public int ElementsPerRow;
		internal int elementsPerBitmap;
		public float invElementSize;
		
		public TextureAtlas1D( IGraphicsApi graphics, Bitmap[] bmps, int elementsPerBitmap ) {
			Init( graphics.SupportsNonPowerOf2Textures, bmps, elementsPerBitmap );
		}
		
		void Init( bool canUseNonPow2, Bitmap[] bmps, int elementsPerBitmap ) {
			this.elementsPerBitmap = elementsPerBitmap;
			ElementsPerRow = this.elementsPerBitmap;
			if( !canUseNonPow2 ) {
				int glHeight = Utils.NextPowerOf2( bmps[0].Height );
				int elementSize = bmps[0].Height / this.elementsPerBitmap;
				this.elementsPerBitmap = glHeight / elementSize;
			}
			invElementSize = 1f / this.elementsPerBitmap;
		}
		
		public TextureRectangle GetTexRec( int texId, out int index ) {
			index = texId / ElementsPerRow;
			int y = texId % ElementsPerRow;
			return new TextureRectangle( 0, y * invElementSize, 1, invElementSize );
		}
		
		public int Get1DIndex( int texId ) {
			return texId / ElementsPerRow;
		}
		
		public int Get1DRowId( int texId ) {
			return texId % ElementsPerRow;
		}
		
		public void Dispose() {
		}
	}
}