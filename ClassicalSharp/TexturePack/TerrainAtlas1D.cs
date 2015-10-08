using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	/// <summary> Represents a 2D packed texture atlas that has been converted into an array of 1D atlases. </summary>
	public sealed class TerrainAtlas1D : IDisposable {
		
		int usedElementsPerAtlas1D;
		internal int elementsPerBitmap;
		public float invElementSize;
		public int[] TexIds;
		IGraphicsApi graphics;
		
		public TerrainAtlas1D( IGraphicsApi graphics ) {
			this.graphics = graphics;
		}
		
		public TextureRec GetTexRec( int texId, int uCount, out int index ) {
			index = texId / usedElementsPerAtlas1D;
			int y = texId % usedElementsPerAtlas1D;
			return new TextureRec( 0, y * invElementSize, uCount, invElementSize );
		}
		
		public int Get1DIndex( int texId ) {
			return texId / usedElementsPerAtlas1D;
		}
		
		public int Get1DRowId( int texId ) {
			return texId % usedElementsPerAtlas1D;
		}
		
		public void UpdateState( TerrainAtlas2D atlas2D ) {
			int maxVerSize = Math.Min( 4096, graphics.MaxTextureDimensions );
			int verElements = maxVerSize / atlas2D.elementSize;
			int totalElements = TerrainAtlas2D.RowsCount * TerrainAtlas2D.ElementsPerRow;
			int elemSize = atlas2D.elementSize;
			
			int atlasesCount = Utils.CeilDiv( totalElements, verElements );
			usedElementsPerAtlas1D = Math.Min( verElements, totalElements );
			int atlas1DHeight = Utils.NextPowerOf2( usedElementsPerAtlas1D * atlas2D.elementSize );
			
			int index = 0;
			TexIds = new int[atlasesCount];
			Utils.LogDebug( "Loaded new atlas: {0} bmps, {1} per bmp", atlasesCount, usedElementsPerAtlas1D );
			
			using( FastBitmap atlas = new FastBitmap( atlas2D.AtlasBitmap, true ) ) {
				for( int i = 0; i < TexIds.Length; i++ ) {
					Bitmap atlas1d = new Bitmap( atlas2D.elementSize, atlas1DHeight );
					using( FastBitmap dst = new FastBitmap( atlas1d, true ) ) {
						for( int y_1D = 0; y_1D < usedElementsPerAtlas1D; y_1D++ ) {
							int x = index & 0x0F;
							int y = index >> 4;
							FastBitmap.MovePortion( x * elemSize, y * elemSize, 0, y_1D * elemSize, atlas, dst, elemSize );
							index++;
						}
						TexIds[i] = graphics.CreateTexture( dst );
					}
					atlas1d.Dispose();
				}
			}
			elementsPerBitmap = atlas1DHeight / atlas2D.elementSize;
			invElementSize = 1f / elementsPerBitmap;
		}
		
		public int CalcMaxUsedRow( TerrainAtlas2D atlas2D, BlockInfo info ) {
			int maxVerSize = Math.Min( 4096, graphics.MaxTextureDimensions );
			int verElements = maxVerSize / atlas2D.elementSize;
			int totalElements = GetMaxUsedRow( info.textures ) * TerrainAtlas2D.ElementsPerRow;	
			Utils.LogDebug( "Used atlases: " + Utils.CeilDiv( totalElements, verElements ) );
			return Utils.CeilDiv( totalElements, verElements );		
		}
		
		int GetMaxUsedRow( int[] textures ) {
			int maxElem = 0;
			for( int i = 0; i < textures.Length; i++ )
				maxElem = Math.Max( maxElem, textures[i] );
			return (maxElem >> 4) + 1;
		}
		
		public void Dispose() {
			if( TexIds != null ) {
				for( int i = 0; i < TexIds.Length; i++ ) {
					graphics.DeleteTexture( ref TexIds[i] );
				}
			}
		}
	}
}