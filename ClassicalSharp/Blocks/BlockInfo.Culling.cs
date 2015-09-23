using System;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
		
		bool[] hidden = new bool[BlocksCount * BlocksCount * TileSide.Sides];

		internal void SetupCullingCache() {
			for( int tileI = 1; tileI < BlocksCount; tileI++ ) {
				for( int neighbourI = 1; neighbourI < BlocksCount; neighbourI++ ) {
					byte tile = (byte)tileI, neighbour = (byte)neighbourI;
					bool hidden = IsHidden( tile, neighbour );
					if( hidden ) {
						SetHidden( tile, neighbour, TileSide.Left, true );
						SetHidden( tile, neighbour, TileSide.Right, true );
						SetHidden( tile, neighbour, TileSide.Front, true );
						SetHidden( tile, neighbour, TileSide.Back, true );
						SetHidden( tile, neighbour, TileSide.Top, Height[tile] == 1 );
						SetHidden( tile, neighbour, TileSide.Bottom, Height[neighbour] == 1 );
					}
				}
			}
			
			for( int i = 0; i < TileSide.Sides; i++ ) {
				SetHidden( (byte)Block.Leaves, (byte)Block.Leaves, i, false );
			}
		}
		
		bool IsHidden( byte tile, byte block ) {
			return 
				((tile == block || (IsOpaque[block] && !IsLiquid[block])) && !IsSprite[tile]) ||
				(IsLiquid[tile] && block == (byte)Block.Ice);
		}
		
		void SetHidden( byte tile, byte block, int tileSide, bool value ) {
			hidden[( tile * BlocksCount + block ) * TileSide.Sides + tileSide] = value;
		}
		
		public bool IsFaceHidden( byte tile, byte block, int tileSide ) {
			return hidden[( tile * BlocksCount + block ) * TileSide.Sides + tileSide];
		}
	}
}