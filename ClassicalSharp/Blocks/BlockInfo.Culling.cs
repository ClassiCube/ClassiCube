// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public static partial class BlockInfo {

		internal static void UpdateCulling() {
			for (int block = 0; block <= MaxDefined; block++) {
				CalcStretch((BlockID)block);
				for (int neighbour = 0; neighbour <= MaxDefined; neighbour++) {
					CalcCulling((BlockID)block, (BlockID)neighbour);
				}
			}
		}
		
		internal static void UpdateCulling(BlockID block) {
			CalcStretch(block);
			for (int other = 0; other <= MaxDefined; other++) {
				CalcCulling(block, (BlockID)other);
				CalcCulling((BlockID)other, block);
			}
		}
		
		static void CalcStretch(BlockID block) {
			// faces which can be stretched on X axis
			if (MinBB[block].X == 0 && MaxBB[block].X == 1) {
				CanStretch[block] |= 0x3C;
			} else {
				CanStretch[block] &= 0xC3; // ~0x3C
			}
			
			// faces which can be stretched on Z axis
			if (MinBB[block].Z == 0 && MaxBB[block].Z == 1) {
				CanStretch[block] |= 0x03;
			} else {
				CanStretch[block] &= 0xFC; // ~0x03
			}
		}
		
		static void CalcCulling(BlockID block, BlockID other) {
			Vector3 bMin = MinBB[block], bMax = MaxBB[block];
			Vector3 oMin = MinBB[other], oMax = MaxBB[other];
			if (IsLiquid[block]) bMax.Y -= 1.5f/16;
			if (IsLiquid[other]) oMax.Y -= 1.5f/16;
			
			hidden[(block << Block.Shift) | other] = 0; // set all faces 'not hidden'
			if (Draw[block] == DrawType.Sprite) {
				SetHidden(block, other, Side.Left,  true);
				SetHidden(block, other, Side.Right, true);
				SetHidden(block, other, Side.Front, true);
				SetHidden(block, other, Side.Back,  true);
				SetHidden(block, other, Side.Bottom, oMax.Y == 1);
				SetHidden(block, other, Side.Top,    bMax.Y == 1);
			} else {
				bool bothLiquid = IsLiquid[block] && IsLiquid[other];
				
				SetHidden(block, other, Side.Left,  oMax.X == 1 && bMin.X == 0);
				SetHidden(block, other, Side.Right, oMin.X == 0 && bMax.X == 1);
				SetHidden(block, other, Side.Front, oMax.Z == 1 && bMin.Z == 0);
				SetHidden(block, other, Side.Back,  oMin.Z == 0 && bMax.Z == 1);
				SetHidden(block, other, Side.Bottom, 
				          bothLiquid || (oMax.Y == 1 && bMin.Y == 0));
				SetHidden(block, other, Side.Top, 
				          bothLiquid || (oMin.Y == 0 && bMax.Y == 1));
			}
		}
		
		static bool IsHidden(BlockID block, BlockID other) {
			// Sprite blocks can never hide faces.
			if (Draw[block] == DrawType.Sprite) return false;
			
			// NOTE: Water is always culled by lava
			if ((block == Block.Water || block == Block.StillWater)
			   && (other == Block.Lava || other == Block.StillLava))
				return true;
			
			// All blocks (except for say leaves) cull with themselves.
			if (block == other) return Draw[block] != DrawType.TransparentThick;
			
			// An opaque neighbour (asides from lava) culls the face.
			if (Draw[other] == DrawType.Opaque && !IsLiquid[other]) return true;
			if (Draw[block] != DrawType.Translucent || Draw[other] != DrawType.Translucent) return false;
			
			// e.g. for water / ice, don't need to draw water.
			byte bType = Collide[block], oType = Collide[other];
			bool canSkip = (bType == CollideType.Solid && oType == CollideType.Solid) || bType != CollideType.Solid;
			return canSkip;
		}
		
		static void SetHidden(BlockID block, BlockID other, int side, bool value) {
			value = IsHidden(block, other) && FaceOccluded(block, other, side) && value;
			int bit = value ? 1 : 0;
			hidden[(block << Block.Shift) | other] |= (byte)(bit << side);
		}
		
		/// <summary> Returns whether the face at the given face of the block
		/// should be drawn with the neighbour 'other' present on the other side of the face. </summary>
		public static bool IsFaceHidden(BlockID block, BlockID other, int tileSide) {
			return (hidden[(block << Block.Shift) | other] & (1 << tileSide)) != 0;
		}
	}
}