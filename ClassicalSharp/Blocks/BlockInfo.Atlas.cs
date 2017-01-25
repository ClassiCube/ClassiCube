// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		public byte[] textures = new byte[Block.Count * Side.Sides];
		
		internal void SetSide(int textureId, byte blockId) {
			int index = blockId * Side.Sides;
			for (int i = index; i < index + Side.Bottom; i++)
				textures[i] = (byte)textureId;
		}
		
		internal void SetTex(int textureId, int face, byte blockId) {
			textures[blockId * Side.Sides + face] = (byte)textureId;
		}
		
		/// <summary> Gets the index in the terrain atlas for the texture of the face of the given block. </summary>
		/// <param name="face"> Face of the given block, see TileSide constants. </param>
		public int GetTextureLoc(byte block, int face) {
			return textures[block * Side.Sides + face];
		}
		
		void GetTextureRegion(byte block, int side, out Vector2 min, out Vector2 max) {
			min = Vector2.Zero; max = Vector2.One;
			Vector3 bbMin = MinBB[block], bbMax = MaxBB[block];		
			switch (side) {
				case Side.Left:
				case Side.Right:
					min = new Vector2(bbMin.Z, bbMin.Y);
					max = new Vector2(bbMax.Z, bbMax.Y);
					if (IsLiquid(block)) { min.Y -= 1.5f/16; max.Y -= 1.5f/16; }
					break;
				case Side.Front:
				case Side.Back: 
					min = new Vector2(bbMin.X, bbMin.Y);
					max = new Vector2(bbMax.X, bbMax.Y);
					if (IsLiquid(block)) { min.Y -= 1.5f/16; max.Y -= 1.5f/16; }
					break;
				case Side.Top:
				case Side.Bottom:
					min = new Vector2(bbMin.X, bbMin.Z);
					max = new Vector2(bbMax.X, bbMax.Z); 
					break;
			}
		}
		
		bool FaceOccluded(byte block, byte other, int side) {
			Vector2 bMin, bMax, oMin, oMax;
			GetTextureRegion(block, side, out bMin, out bMax);
			GetTextureRegion(other, side, out oMin, out oMax);
			return bMin.X >= oMin.X && bMin.Y >= oMin.Y
				&& bMax.X <= oMax.X && bMax.Y <= oMax.Y;
		}
		
		static byte[] topTex = new byte[] { 0,  1,  0,  2, 16,  4, 15, 17, 14, 14, 
			30, 30, 18, 19, 32, 33, 34, 21, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71, 
			72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 24, 23,  6,  6,  7,  9,  4, 
			36, 37, 16, 11, 25, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 26, 53, 52, };
		static byte[] sideTex = new byte[] { 0,  1,  3,  2, 16,  4, 15, 17, 14, 14, 
			30, 30, 18, 19, 32, 33, 34, 20, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71, 
			72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 40, 39,  5,  5,  7,  8, 35, 
			36, 37, 16, 11, 41, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 42, 53, 52, };
		static byte[] bottomTex = new byte[] { 0,  1,  2,  2, 16,  4, 15, 17, 14, 14, 
			30, 30, 18, 19, 32, 33, 34, 21, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71, 
			72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 56, 55,  6,  6,  7, 10,  4, 
			36, 37, 16, 11, 57, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 58, 53, 52 };
	}
}