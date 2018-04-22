// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;
using ClassicalSharp;
using BlockID = System.UInt16;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public static partial class BlockInfo {
		
		internal static void CalcRenderBounds(BlockID block) {
			Vector3 min = MinBB[block], max = MaxBB[block];
			
			if (IsLiquid[block]) {
				min.X -= 0.1f/16f; max.X -= 0.1f/16f;
				min.Z -= 0.1f/16f; max.Z -= 0.1f/16f;
				min.Y -= 1.5f/16f; max.Y -= 1.5f/16f;
			} else if (Draw[block] == DrawType.Translucent && Collide[block] != CollideType.Solid) {
				min.X += 0.1f/16f; max.X += 0.1f/16f;
				min.Z += 0.1f/16f; max.Z += 0.1f/16f;
				min.Y -= 0.1f/16f; max.Y -= 0.1f/16f;
			}
			
			RenderMinBB[block] = min; RenderMaxBB[block] = max;
		}
		
		internal static void CalcLightOffset(BlockID block) {
			int flags = 0xFF;
			Vector3 min = MinBB[block], max = MaxBB[block];
			
			if (min.X != 0) flags &= ~(1 << Side.Left);
			if (max.X != 1) flags &= ~(1 << Side.Right);
			if (min.Z != 0) flags &= ~(1 << Side.Front);
			if (max.Z != 1) flags &= ~(1 << Side.Back);
			
			if ((min.Y != 0 && max.Y == 1) && Draw[block] != DrawType.Gas) {
				flags &= ~(1 << Side.Top);
				flags &= ~(1 << Side.Bottom);
			}
			LightOffset[block] = (byte)flags;
		}
		
		internal static void RecalculateSpriteBB() {
			using (FastBitmap fastBmp = new FastBitmap(TerrainAtlas2D.Atlas, true, true)) {
				for (int i = 0; i < Count; i++) {
					if (Draw[i] != DrawType.Sprite) continue;
					RecalculateBB((BlockID)i, fastBmp);
				}
			}
		}
		
		const float angle = 45f * Utils.Deg2Rad;
		static readonly Vector3 centre = new Vector3(0.5f, 0, 0.5f);
		internal static void RecalculateBB(BlockID block, FastBitmap fastBmp) {
			int elemSize = fastBmp.Width / 16;
			int texId = GetTextureLoc(block, Side.Right);
			int texX = texId & 0x0F, texY = texId >> 4;
			
			float topY = GetSpriteBB_TopY(elemSize, texX, texY, fastBmp);
			float bottomY = GetSpriteBB_BottomY(elemSize, texX, texY, fastBmp);
			float leftX = GetSpriteBB_LeftX(elemSize, texX, texY, fastBmp);
			float rightX = GetSpriteBB_RightX(elemSize, texX, texY, fastBmp);
			
			MinBB[block] = Utils.RotateY(leftX - 0.5f, bottomY, 0, angle) + centre;
			MaxBB[block] = Utils.RotateY(rightX - 0.5f, topY, 0, angle) + centre;
			CalcRenderBounds(block);
		}
		
		unsafe static float GetSpriteBB_TopY(int size, int tileX, int tileY, FastBitmap fastBmp) {
			for (int y = 0; y < size; y++) {
				int* row = fastBmp.GetRowPtr(tileY * size + y) + (tileX * size);
				for (int x = 0; x < size; x++) {
					if ((byte)(row[x] >> 24) != 0)
						return 1 - (float)y / size;
				}
			}
			return 0;
		}
		
		unsafe static float GetSpriteBB_BottomY(int size, int tileX, int tileY, FastBitmap fastBmp) {
			for (int y = size - 1; y >= 0; y--) {
				int* row = fastBmp.GetRowPtr(tileY * size + y) + (tileX * size);
				for (int x = 0; x < size; x++) {
					if ((byte)(row[x] >> 24) != 0)
						return 1 - (float)(y + 1) / size;
				}
			}
			return 1;
		}
		
		unsafe static float GetSpriteBB_LeftX(int size, int tileX, int tileY, FastBitmap fastBmp) {
			for (int x = 0; x < size; x++) {
				for (int y = 0; y < size; y++) {
					int* row = fastBmp.GetRowPtr(tileY * size + y) + (tileX * size);
					if ((byte)(row[x] >> 24) != 0)
						return (float)x / size;
				}
			}
			return 1;
		}
		
		unsafe static float GetSpriteBB_RightX(int size, int tileX, int tileY, FastBitmap fastBmp) {
			for (int x = size - 1; x >= 0; x--) {
				for (int y = 0; y < size; y++) {
					int* row = fastBmp.GetRowPtr(tileY * size + y) + (tileX * size);
					if ((byte)(row[x] >> 24) != 0)
						return (float)(x + 1) / size;
				}
			}
			return 0;
		}
	}
}