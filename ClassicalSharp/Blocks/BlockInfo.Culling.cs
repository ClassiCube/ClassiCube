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
		
		void UpdateCulling( byte block, byte other ) {
			Vector3 bMin = MinBB[block], bMax = MaxBB[block];
			Vector3 oMin = MinBB[other], oMax = MaxBB[other];			
			if( IsSprite[block] ) {
				SetHidden( block, other, Side.Left, true );
				SetHidden( block, other, Side.Right, true );
				SetHidden( block, other, Side.Front, true );
				SetHidden( block, other, Side.Back, true );
				SetHidden( block, other, Side.Bottom, oMax.Y == 1 );
				SetHidden( block, other, Side.Top, bMax.Y == 1 );
			} else {
				SetXStretch( block, bMin.X == 0 && bMax.X == 1 );
				SetZStretch( block, bMin.Z == 0 && bMax.Z == 1 );
				
				SetHidden( block, other, Side.Left, oMax.X == 1 && bMin.X == 0 );
				SetHidden( block, other, Side.Right, oMin.X == 0 && bMax.X == 1 );
				SetHidden( block, other, Side.Front, oMax.Z == 1 && bMin.Z == 0 );
				SetHidden( block, other, Side.Back, oMin.Z == 0 && bMax.Z == 1 );
				SetHidden( block, other, Side.Bottom, oMax.Y == 1 && bMin.Y == 0 );
				SetHidden( block, other, Side.Top, oMin.Y == 0 && bMax.Y == 1 );
			}
		}
		
		bool IsHidden( byte block, byte other, int side ) {
			// Sprite blocks can never hide faces.
			if( IsSprite[block] ) return false;
			// All blocks (except for say leaves) cull with themselves.
			if( block == other ) return CullWithNeighbours[block];
			
			// Special case for water/lava being offset
			if( IsLiquid[block] && side == Side.Top && 
			   (!IsTranslucent[other] || Collide[other] != CollideType.Solid))
				return false;
			
			// An opaque neighbour (asides from lava) culls the face.
			if( IsOpaque[other] && !IsLiquid[other] ) return true;
			if( !IsTranslucent[block] || !IsTranslucent[other] ) return false;
			
			// e.g. for water / ice, don't need to draw water.
			CollideType bType = Collide[block], oType = Collide[other];
			bool canSkip = (bType == CollideType.Solid && oType == CollideType.Solid) 
				|| bType != CollideType.Solid;
			return canSkip && FaceOccluded( block, other, side );
		}
		
		void SetHidden( byte block, byte other, int side, bool value ) {
			value = IsHidden( block, other, side ) && value;
			int bit = value ? 1 : 0;
			hidden[block * BlocksCount + other] &= (byte)~(1 << side);
			hidden[block * BlocksCount + other] |= (byte)(bit << side);
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