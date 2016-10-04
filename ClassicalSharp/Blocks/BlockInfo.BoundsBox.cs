// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;
using ClassicalSharp;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		public Vector3[] MinBB = new Vector3[Block.Count];
		public Vector3[] MaxBB = new Vector3[Block.Count];
		
		internal byte CalcLightOffset( byte block ) {
			int flags = 0xFF;
			Vector3 min = MinBB[block], max = MaxBB[block];
			
			if( min.X != 0 ) flags &= ~(1 << Side.Left);
			if( max.X != 1 ) flags &= ~(1 << Side.Right);
			if( min.Z != 0 ) flags &= ~(1 << Side.Front);
			if( max.Z != 1 ) flags &= ~(1 << Side.Back);
			
			if( (min.Y != 0 && max.Y == 1) && !IsAir[block] ) {
				flags &= ~(1 << Side.Top);
				flags &= ~(1 << Side.Bottom);
			}
			return (byte)flags;
		}
		
		public void RecalculateSpriteBB( FastBitmap fastBmp ) {
			for( int i = 0; i < Block.Count; i++ ) {
				if( IsSprite[i] ) RecalculateBB( i, fastBmp );
			}
		}
		
		const float angle = 45f * Utils.Deg2Rad;
		static readonly Vector3 centre = new Vector3( 0.5f, 0, 0.5f );
		internal void RecalculateBB( int block, FastBitmap fastBmp ) {
			int elemSize = fastBmp.Width / 16;
			int texId = GetTextureLoc( (byte)block, Side.Right );
			int texX = texId & 0x0F, texY = texId >> 4;
			
			float topY = GetSpriteBB_TopY( elemSize, texX, texY, fastBmp );
			float bottomY = GetSpriteBB_BottomY( elemSize, texX, texY, fastBmp );
			float leftX = GetSpriteBB_LeftX( elemSize, texX, texY, fastBmp );
			float rightX = GetSpriteBB_RightX( elemSize, texX, texY, fastBmp );
			
			MinBB[block] = Utils.RotateY( leftX - 0.5f, bottomY, 0, angle ) + centre;
			MaxBB[block] = Utils.RotateY( rightX - 0.5f, topY, 0, angle ) + centre;
		}
		
		unsafe float GetSpriteBB_TopY( int size, int tileX, int tileY, FastBitmap fastBmp ) {
			for( int y = 0; y < size; y++ ) {
				int* row = fastBmp.GetRowPtr( tileY * size + y ) + (tileX * size);
				for( int x = 0; x < size; x++ ) {
					if( (byte)(row[x] >> 24) != 0 )
						return 1 - (float)y / size;
				}
			}
			return 0;
		}
		
		unsafe float GetSpriteBB_BottomY( int size, int tileX, int tileY, FastBitmap fastBmp ) {
			for( int y = size - 1; y >= 0; y-- ) {
				int* row = fastBmp.GetRowPtr( tileY * size + y ) + (tileX * size);
				for( int x = 0; x < size; x++ ) {
					if( (byte)(row[x] >> 24) != 0 )
						return 1 - (float)(y + 1) / size;
				}
			}
			return 1;
		}
		
		unsafe float GetSpriteBB_LeftX( int size, int tileX, int tileY, FastBitmap fastBmp ) {
			for( int x = 0; x < size; x++ ) {
				for( int y = 0; y < size; y++ ) {
					int* row = fastBmp.GetRowPtr( tileY * size + y ) + (tileX * size);
					if( (byte)(row[x] >> 24) != 0 )
						return (float)x / size;
				}
			}
			return 1;
		}
		
		unsafe float GetSpriteBB_RightX( int size, int tileX, int tileY, FastBitmap fastBmp ) {
			for( int x = size - 1; x >= 0; x-- ) {
				for( int y = 0; y < size; y++ ) {
					int* row = fastBmp.GetRowPtr( tileY * size + y ) + (tileX * size);
					if( (byte)(row[x] >> 24) != 0 )
						return (float)(x + 1) / size;
				}
			}
			return 0;
		}
	}
}