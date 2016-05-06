// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		public byte[] hidden = new byte[BlocksCount * BlocksCount];
		
		public bool[] CanStretch = new bool[BlocksCount * Side.Sides];
		
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
			Vector3 tMin = MinBB[tile], tMax = MaxBB[tile];
			Vector3 nMin = MinBB[neighbour], nMax = MaxBB[neighbour];
			
			if( IsSprite[tile] ) {
				SetHidden( tile, neighbour, Side.Left, hidden );
				SetHidden( tile, neighbour, Side.Right, hidden );
				SetHidden( tile, neighbour, Side.Front, hidden );
				SetHidden( tile, neighbour, Side.Back, hidden );
				SetHidden( tile, neighbour, Side.Bottom, hidden && nMax.Y == 1 );
				SetHidden( tile, neighbour, Side.Top, hidden && tMax.Y == 1 );
			} else {
				SetXStretch( tile, tMin.X == 0 && tMax.X == 1 );
				SetZStretch( tile, tMin.Z == 0 && tMax.Z == 1 );
				
				SetHidden( tile, neighbour, Side.Left, hidden && nMax.X == 1 && tMin.X == 0 );
				SetHidden( tile, neighbour, Side.Right, hidden && nMin.X == 0 && tMax.X == 1 );
				SetHidden( tile, neighbour, Side.Front, hidden && nMax.Z == 1 && tMin.Z == 0 );
				SetHidden( tile, neighbour, Side.Back, hidden && nMin.Z == 0 && tMax.Z == 1 );
				SetHidden( tile, neighbour, Side.Bottom, hidden && nMax.Y == 1 && tMin.Y == 0 );
				SetHidden( tile, neighbour, Side.Top, hidden && nMin.Y == 0 && tMax.Y == 1 );
			}
		}
		
		bool IsHidden( byte tile, byte neighbour ) {
			// Sprite blocks can never hide faces.
			if( IsSprite[tile] ) return false;
			// All blocks (except for say leaves) cull with themselves.
			if( tile == neighbour ) return CullWithNeighbours[tile];			
			// An opaque neighbour (asides from lava) culls the face.
			if( IsOpaque[neighbour] && !IsLiquid[neighbour] ) return true;

			// e.g. for water / ice, don't need to draw water.
			return IsTranslucent[tile] && IsTranslucent[neighbour] 
				&& Collide[neighbour] == CollideType.Solid;
		}
		
		void SetHidden( byte tile, byte block, int tileSide, bool value ) {
			int bit = value ? 1 : 0;
			hidden[tile * BlocksCount + block] &= (byte)~(1 << tileSide);
			hidden[tile * BlocksCount + block] |= (byte)(bit << tileSide);
		}
		
		/// <summary> Returns whether the face at the given face of the tile
		/// should be drawn with the neighbour 'block' present on the other side of the face. </summary>
		public bool IsFaceHidden( byte tile, byte block, int tileSide ) {
			return (hidden[tile * BlocksCount + block] & (1 << tileSide)) != 0;
		}
		
		void SetXStretch( byte tile, bool stretch ) {
			CanStretch[tile * Side.Sides + Side.Front] = stretch;
			CanStretch[tile * Side.Sides + Side.Back] = stretch;
			CanStretch[tile * Side.Sides + Side.Top] = stretch;
			CanStretch[tile * Side.Sides + Side.Bottom] = stretch;
		}
		
		void SetZStretch( byte tile, bool stretch ) {
			CanStretch[tile * Side.Sides + Side.Left] = stretch;
			CanStretch[tile * Side.Sides + Side.Right] = stretch;
		}
	}
}