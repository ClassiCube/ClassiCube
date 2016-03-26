// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		public bool[] hidden = new bool[BlocksCount * BlocksCount * TileSide.Sides];
		
		public bool[] CanStretch = new bool[BlocksCount * TileSide.Sides];
		
		public bool[] IsAir = new bool[BlocksCount];

		internal void SetupCullingCache() {
			IsAir[0] = true;
			for( int tile = 1; tile < BlocksCount; tile++ )
				CheckOpaque( tile );
			for( int tile = 0; tile < CanStretch.Length; tile++ )
				CanStretch[tile] = true;
			
			for( int tileI = 1; tileI < BlocksCount; tileI++ ) {
				for( int neighbourI = 1; neighbourI < BlocksCount; neighbourI++ ) {
					byte tile = (byte)tileI, neighbour = (byte)neighbourI;
					UpdateCulling( tile, neighbour );
				}
			}
		}
		
		internal void SetupCullingCache( byte tile ) {
			IsAir[0] = true;
			CheckOpaque( tile );
			CanStretch[tile] = true;
			
			for( int other = 1; other < BlocksCount; other++ ) {
				UpdateCulling( tile, (byte)other );
				UpdateCulling( (byte)other, tile );
			}
		}
		
		void CheckOpaque( int tile ) {
			if( MinBB[tile] != Vector3.Zero || MaxBB[tile] != Vector3.One ) {
				IsOpaque[tile] = false;
				IsTransparent[tile] = true;
			}
		}
		
		void UpdateCulling( byte tile, byte neighbour ) {
			bool hidden = IsHidden( tile, neighbour );
			if( tile == neighbour && !CullWithNeighbours[tile] )
				hidden = false;
			Vector3 tMin = MinBB[tile], tMax = MaxBB[tile];
			Vector3 nMin = MinBB[neighbour], nMax = MaxBB[neighbour];
			
			if( IsSprite[tile] ) {
				SetHidden( tile, neighbour, TileSide.Left, hidden );
				SetHidden( tile, neighbour, TileSide.Right, hidden );
				SetHidden( tile, neighbour, TileSide.Front, hidden );
				SetHidden( tile, neighbour, TileSide.Back, hidden );
				SetHidden( tile, neighbour, TileSide.Bottom, hidden && nMax.Y == 1 );
				SetHidden( tile, neighbour, TileSide.Top, hidden && tMax.Y == 1 );
			} else {
				SetXStretch( tile, tMin.X == 0 && tMax.X == 1 );
				SetZStretch( tile, tMin.Z == 0 && tMax.Z == 1 );
				
				SetHidden( tile, neighbour, TileSide.Left, hidden && nMax.X == 1 && tMin.X == 0 );
				SetHidden( tile, neighbour, TileSide.Right, hidden && nMin.X == 0 && tMax.X == 1 );
				SetHidden( tile, neighbour, TileSide.Front, hidden && nMax.Z == 1 && tMin.Z == 0 );
				SetHidden( tile, neighbour, TileSide.Back, hidden && nMin.Z == 0 && tMax.Z == 1 );
				SetHidden( tile, neighbour, TileSide.Bottom, hidden && nMax.Y == 1 && tMin.Y == 0 );
				SetHidden( tile, neighbour, TileSide.Top, hidden && nMin.Y == 0 && tMax.Y == 1 );
			}
		}
		
		bool IsHidden( byte tile, byte block ) {
			return
				((tile == block || (IsOpaque[block] && !IsLiquid[block])) && !IsSprite[tile]) ||
				((tile == (byte)Block.Water || tile == (byte)Block.StillWater) && block == (byte)Block.Ice);
		}
		
		void SetHidden( byte tile, byte block, int tileSide, bool value ) {
			hidden[( tile * BlocksCount + block ) * TileSide.Sides + tileSide] = value;
		}
		
		/// <summary> Returns whether the face at the given face of the tile
		/// should be drawn with the neighbour 'block' present on the other side of the face. </summary>
		public bool IsFaceHidden( byte tile, byte block, int tileSide ) {
			return hidden[( tile * BlocksCount + block ) * TileSide.Sides + tileSide];
		}
		
		void SetXStretch( byte tile, bool stretch ) {
			CanStretch[tile * TileSide.Sides + TileSide.Front] = stretch;
			CanStretch[tile * TileSide.Sides + TileSide.Back] = stretch;
			CanStretch[tile * TileSide.Sides + TileSide.Top] = stretch;
			CanStretch[tile * TileSide.Sides + TileSide.Bottom] = stretch;
		}
		
		void SetZStretch( byte tile, bool stretch ) {
			CanStretch[tile * TileSide.Sides + TileSide.Left] = stretch;
			CanStretch[tile * TileSide.Sides + TileSide.Right] = stretch;
		}
	}
}