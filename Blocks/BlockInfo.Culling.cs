using System;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
		
		bool[] hidden = new bool[BlocksCount * BlocksCount * 6];

		void SetupCullingCache() {
			for( byte tile = 1; tile < BlocksCount; tile++ ) {
				for( byte neighbour = 1; neighbour < BlocksCount; neighbour++ ) {
					bool hidden = IsHidden( tile, neighbour );
					if( hidden ) {
						SetHidden( tile, neighbour, TileSide.Left, true );
						SetHidden( tile, neighbour, TileSide.Right, true );
						SetHidden( tile, neighbour, TileSide.Front, true );
						SetHidden( tile, neighbour, TileSide.Back, true );
						SetHidden( tile, neighbour, TileSide.Top, BlockHeight( tile ) == 1 );
						SetHidden( tile, neighbour, TileSide.Bottom, BlockHeight( neighbour ) == 1 );
					}
				}
			}
		}
		
		bool IsHidden( byte tile, byte block ) {
			return 
				( ( tile == block || ( IsOpaque( block ) && !IsLiquid( block ) ) ) && !IsSprite( tile ) ) || 
				( IsLiquid( tile ) && block == (byte)Block.Ice );
		}
		
		void SetHidden( byte tile, byte block, int tileSide, bool value ) {
			hidden[( tile * BlocksCount + block ) * 6 + tileSide] = value;
		}
		
		public bool IsFaceHidden( byte tile, byte block, int tileSide ) {
			return hidden[( tile * BlocksCount + block ) * 6 + tileSide];
		}
	}
}