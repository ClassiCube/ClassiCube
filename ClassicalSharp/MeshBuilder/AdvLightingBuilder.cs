// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp {

	public unsafe sealed class AdvLightingMeshBuilder : ChunkMeshBuilder {
		
		Vector3 minBB, maxBB;
		bool isTranslucent;
		int initBitFlags, lightFlags;
		float x1, y1, z1, x2, y2, z2;
		
		protected override int StretchXLiquid(int countIndex, int x, int y, int z, int chunkIndex, BlockID block) {
			if (OccludedLiquid(chunkIndex)) return 0;
			initBitFlags = ComputeLightFlags(x, y, z, chunkIndex);
			bitFlags[chunkIndex] = initBitFlags;
			
			int count = 1;
			x++;
			chunkIndex++;
			countIndex += Side.Sides;
			
			while (x < chunkEndX && CanStretch(block, chunkIndex, x, y, z, Side.Top) && !OccludedLiquid(chunkIndex)) {
				counts[countIndex] = 0;
				count++;
				x++;
				chunkIndex++;
				countIndex += Side.Sides;
			}
			return count;
		}
		
		protected override int StretchX(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, int face) {
			initBitFlags = ComputeLightFlags(x, y, z, chunkIndex);
			bitFlags[chunkIndex] = initBitFlags;
			
			int count = 1;
			x++;
			chunkIndex++;
			countIndex += Side.Sides;
			bool stretchTile = (info.CanStretch[block] & (1 << face)) != 0;
			
			while (x < chunkEndX && stretchTile && CanStretch(block, chunkIndex, x, y, z, face)) {
				counts[countIndex] = 0;
				count++;
				x++;
				chunkIndex++;
				countIndex += Side.Sides;
			}
			return count;
		}
		
		protected override int StretchZ(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, int face) {
			initBitFlags = ComputeLightFlags(x, y, z, chunkIndex);
			bitFlags[chunkIndex] = initBitFlags;
			
			int count = 1;
			z++;
			chunkIndex += extChunkSize;
			countIndex += chunkSize * Side.Sides;
			bool stretchTile = (info.CanStretch[block] & (1 << face)) != 0;
			
			while (z < chunkEndZ && stretchTile && CanStretch(block, chunkIndex, x, y, z, face)) {
				counts[countIndex] = 0;
				count++;
				z++;
				chunkIndex += extChunkSize;
				countIndex += chunkSize * Side.Sides;
			}
			return count;
		}
		
		bool CanStretch(BlockID initialBlock, int chunkIndex, int x, int y, int z, int face) {
			BlockID rawBlock = chunk[chunkIndex];
			bitFlags[chunkIndex] = ComputeLightFlags(x, y, z, chunkIndex);
			return rawBlock == initialBlock
				&& !info.IsFaceHidden(rawBlock, chunk[chunkIndex + offsets[face]], face)
				&& (initBitFlags == bitFlags[chunkIndex]
				    // Check that this face is either fully bright or fully in shadow
				    && (initBitFlags == 0 || (initBitFlags & masks[face]) == masks[face]));
		}
		
		
		protected override void RenderTile(int index) {
			if (info.Draw[curBlock] == DrawType.Sprite) {
				fullBright = info.FullBright[curBlock];
				tinted = info.Tinted[curBlock];
				int count = counts[index + Side.Top];
				if (count != 0) DrawSprite(count);
				return;
			}
			
			int leftCount = counts[index++], rightCount = counts[index++],
			frontCount = counts[index++], backCount = counts[index++],
			bottomCount = counts[index++], topCount = counts[index++];
			if (leftCount == 0 && rightCount == 0 && frontCount == 0 &&
			    backCount == 0 && bottomCount == 0 && topCount == 0) return;
			
			fullBright = info.FullBright[curBlock];
			isTranslucent = info.Draw[curBlock] == DrawType.Translucent;
			lightFlags = info.LightOffset[curBlock];
			tinted = info.Tinted[curBlock];
			
			Vector3 min = info.RenderMinBB[curBlock], max = info.RenderMaxBB[curBlock];
			x1 = X + min.X; y1 = Y + min.Y; z1 = Z + min.Z;
			x2 = X + max.X; y2 = Y + max.Y; z2 = Z + max.Z;
			
			this.minBB = info.MinBB[curBlock]; this.maxBB = info.MaxBB[curBlock];
			minBB.Y = 1 - minBB.Y; maxBB.Y = 1 - maxBB.Y;
			
			if (leftCount != 0) DrawLeftFace(leftCount);
			if (rightCount != 0) DrawRightFace(rightCount);
			if (frontCount != 0) DrawFrontFace(frontCount);
			if (backCount != 0) DrawBackFace(backCount);
			if (bottomCount != 0) DrawBottomFace(bottomCount);
			if (topCount != 0) DrawTopFace(topCount);
		}
		
		
		void DrawLeftFace(int count) {
			int texId = info.textures[curBlock * Side.Sides + Side.Left];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Left) & 1;
			
			float u1 = minBB.Z, u2 = (count - 1) + maxBB.Z * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			int F = bitFlags[cIndex];
			int aY0_Z0 = ((F >> xM1_yM1_zM1) & 1) + ((F >> xM1_yCC_zM1) & 1) + ((F >> xM1_yM1_zCC) & 1) + ((F >> xM1_yCC_zCC) & 1);
			int aY0_Z1 = ((F >> xM1_yM1_zP1) & 1) + ((F >> xM1_yCC_zP1) & 1) + ((F >> xM1_yM1_zCC) & 1) + ((F >> xM1_yCC_zCC) & 1);
			int aY1_Z0 = ((F >> xM1_yP1_zM1) & 1) + ((F >> xM1_yCC_zM1) & 1) + ((F >> xM1_yP1_zCC) & 1) + ((F >> xM1_yCC_zCC) & 1);
			int aY1_Z1 = ((F >> xM1_yP1_zP1) & 1) + ((F >> xM1_yCC_zP1) & 1) + ((F >> xM1_yP1_zCC) & 1) + ((F >> xM1_yCC_zCC) & 1);
			
			int col0_0 = fullBright ? FastColour.WhitePacked : lerpX[aY0_Z0], col1_0 = fullBright ? FastColour.WhitePacked : lerpX[aY1_Z0];
			int col1_1 = fullBright ? FastColour.WhitePacked : lerpX[aY1_Z1], col0_1 = fullBright ? FastColour.WhitePacked : lerpX[aY0_Z1];
			if (tinted) {
				FastColour tint = info.FogColour[curBlock];
				col0_0 = Utils.Tint(col0_0, tint);
				col1_0 = Utils.Tint(col1_0, tint);
				col1_1 = Utils.Tint(col1_1, tint);
				col0_1 = Utils.Tint(col0_1, tint);
			}
			
			if (aY0_Z0 + aY1_Z1 > aY0_Z1 + aY1_Z0) {
				part.vertices[part.vIndex[Side.Left]++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col1_0);
				part.vertices[part.vIndex[Side.Left]++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v2, col0_0);
				part.vertices[part.vIndex[Side.Left]++] = new VertexP3fT2fC4b(x1, y1, z2 + (count - 1), u2, v2, col0_1);
				part.vertices[part.vIndex[Side.Left]++] = new VertexP3fT2fC4b(x1, y2, z2 + (count - 1), u2, v1, col1_1);
			} else {
				part.vertices[part.vIndex[Side.Left]++] = new VertexP3fT2fC4b(x1, y2, z2 + (count - 1), u2, v1, col1_1);
				part.vertices[part.vIndex[Side.Left]++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col1_0);
				part.vertices[part.vIndex[Side.Left]++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v2, col0_0);
				part.vertices[part.vIndex[Side.Left]++] = new VertexP3fT2fC4b(x1, y1, z2 + (count - 1), u2, v2, col0_1);
			}
		}

		void DrawRightFace(int count) {
			int texId = info.textures[curBlock * Side.Sides + Side.Right];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Right) & 1;
			
			float u1 = (count - minBB.Z), u2 = (1 - maxBB.Z) * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			int F = bitFlags[cIndex];
			int aY0_Z0 = ((F >> xP1_yM1_zM1) & 1) + ((F >> xP1_yCC_zM1) & 1) + ((F >> xP1_yM1_zCC) & 1) + ((F >> xP1_yCC_zCC) & 1);
			int aY0_Z1 = ((F >> xP1_yM1_zP1) & 1) + ((F >> xP1_yCC_zP1) & 1) + ((F >> xP1_yM1_zCC) & 1) + ((F >> xP1_yCC_zCC) & 1);
			int aY1_Z0 = ((F >> xP1_yP1_zM1) & 1) + ((F >> xP1_yCC_zM1) & 1) + ((F >> xP1_yP1_zCC) & 1) + ((F >> xP1_yCC_zCC) & 1);
			int aY1_Z1 = ((F >> xP1_yP1_zP1) & 1) + ((F >> xP1_yCC_zP1) & 1) + ((F >> xP1_yP1_zCC) & 1) + ((F >> xP1_yCC_zCC) & 1);
			
			int col0_0 = fullBright ? FastColour.WhitePacked : lerpX[aY0_Z0], col1_0 = fullBright ? FastColour.WhitePacked : lerpX[aY1_Z0];
			int col1_1 = fullBright ? FastColour.WhitePacked : lerpX[aY1_Z1], col0_1 = fullBright ? FastColour.WhitePacked : lerpX[aY0_Z1];
			if (tinted) {
				FastColour tint = info.FogColour[curBlock];
				col0_0 = Utils.Tint(col0_0, tint);
				col1_0 = Utils.Tint(col1_0, tint);
				col1_1 = Utils.Tint(col1_1, tint);
				col0_1 = Utils.Tint(col0_1, tint);
			}
			
			if (aY0_Z0 + aY1_Z1 > aY0_Z1 + aY1_Z0) {
				part.vertices[part.vIndex[Side.Right]++] = new VertexP3fT2fC4b(x2, y2, z1, u1, v1, col1_0);
				part.vertices[part.vIndex[Side.Right]++] = new VertexP3fT2fC4b(x2, y2, z2 + (count - 1), u2, v1, col1_1);
				part.vertices[part.vIndex[Side.Right]++] = new VertexP3fT2fC4b(x2, y1, z2 + (count - 1), u2, v2, col0_1);
				part.vertices[part.vIndex[Side.Right]++] = new VertexP3fT2fC4b(x2, y1, z1, u1, v2, col0_0);
			} else {
				part.vertices[part.vIndex[Side.Right]++] = new VertexP3fT2fC4b(x2, y2, z2 + (count - 1), u2, v1, col1_1);
				part.vertices[part.vIndex[Side.Right]++] = new VertexP3fT2fC4b(x2, y1, z2 + (count - 1), u2, v2, col0_1);
				part.vertices[part.vIndex[Side.Right]++] = new VertexP3fT2fC4b(x2, y1, z1, u1, v2, col0_0);
				part.vertices[part.vIndex[Side.Right]++] = new VertexP3fT2fC4b(x2, y2, z1, u1, v1, col1_0);
			}
		}

		void DrawFrontFace(int count) {
			int texId = info.textures[curBlock * Side.Sides + Side.Front];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Front) & 1;
			
			float u1 = (count - minBB.X), u2 = (1 - maxBB.X) * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			int F = bitFlags[cIndex];
			int aX0_Y0 = ((F >> xM1_yM1_zM1) & 1) + ((F >> xM1_yCC_zM1) & 1) + ((F >> xCC_yM1_zM1) & 1) + ((F >> xCC_yCC_zM1) & 1);
			int aX0_Y1 = ((F >> xM1_yP1_zM1) & 1) + ((F >> xM1_yCC_zM1) & 1) + ((F >> xCC_yP1_zM1) & 1) + ((F >> xCC_yCC_zM1) & 1);
			int aX1_Y0 = ((F >> xP1_yM1_zM1) & 1) + ((F >> xP1_yCC_zM1) & 1) + ((F >> xCC_yM1_zM1) & 1) + ((F >> xCC_yCC_zM1) & 1);
			int aX1_Y1 = ((F >> xP1_yP1_zM1) & 1) + ((F >> xP1_yCC_zM1) & 1) + ((F >> xCC_yP1_zM1) & 1) + ((F >> xCC_yCC_zM1) & 1);
			
			int col0_0 = fullBright ? FastColour.WhitePacked : lerpZ[aX0_Y0], col1_0 = fullBright ? FastColour.WhitePacked : lerpZ[aX1_Y0];
			int col1_1 = fullBright ? FastColour.WhitePacked : lerpZ[aX1_Y1], col0_1 = fullBright ? FastColour.WhitePacked : lerpZ[aX0_Y1];
			if (tinted) {
				FastColour tint = info.FogColour[curBlock];
				col0_0 = Utils.Tint(col0_0, tint);
				col1_0 = Utils.Tint(col1_0, tint);
				col1_1 = Utils.Tint(col1_1, tint);
				col0_1 = Utils.Tint(col0_1, tint);
			}
			
			if (aX1_Y1 + aX0_Y0 > aX0_Y1 + aX1_Y0) {
				part.vertices[part.vIndex[Side.Front]++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z1, u2, v2, col1_0);
				part.vertices[part.vIndex[Side.Front]++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v2, col0_0);
				part.vertices[part.vIndex[Side.Front]++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col0_1);
				part.vertices[part.vIndex[Side.Front]++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z1, u2, v1, col1_1);
			} else {
				part.vertices[part.vIndex[Side.Front]++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v2, col0_0);
				part.vertices[part.vIndex[Side.Front]++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col0_1);
				part.vertices[part.vIndex[Side.Front]++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z1, u2, v1, col1_1);
				part.vertices[part.vIndex[Side.Front]++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z1, u2, v2, col1_0);
			}
		}
		
		void DrawBackFace(int count) {
			int texId = info.textures[curBlock * Side.Sides + Side.Back];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Back) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			int F = bitFlags[cIndex];
			int aX0_Y0 = ((F >> xM1_yM1_zP1) & 1) + ((F >> xM1_yCC_zP1) & 1) + ((F >> xCC_yM1_zP1) & 1) + ((F >> xCC_yCC_zP1) & 1);
			int aX1_Y0 = ((F >> xP1_yM1_zP1) & 1) + ((F >> xP1_yCC_zP1) & 1) + ((F >> xCC_yM1_zP1) & 1) + ((F >> xCC_yCC_zP1) & 1);
			int aX0_Y1 = ((F >> xM1_yP1_zP1) & 1) + ((F >> xM1_yCC_zP1) & 1) + ((F >> xCC_yP1_zP1) & 1) + ((F >> xCC_yCC_zP1) & 1);
			int aX1_Y1 = ((F >> xP1_yP1_zP1) & 1) + ((F >> xP1_yCC_zP1) & 1) + ((F >> xCC_yP1_zP1) & 1) + ((F >> xCC_yCC_zP1) & 1);
			
			int col1_1 = fullBright ? FastColour.WhitePacked : lerpZ[aX1_Y1], col1_0 = fullBright ? FastColour.WhitePacked : lerpZ[aX1_Y0];
			int col0_0 = fullBright ? FastColour.WhitePacked : lerpZ[aX0_Y0], col0_1 = fullBright ? FastColour.WhitePacked : lerpZ[aX0_Y1];
			if (tinted) {
				FastColour tint = info.FogColour[curBlock];
				col0_0 = Utils.Tint(col0_0, tint);
				col1_0 = Utils.Tint(col1_0, tint);
				col1_1 = Utils.Tint(col1_1, tint);
				col0_1 = Utils.Tint(col0_1, tint);
			}
			
			if (aX1_Y1 + aX0_Y0 > aX0_Y1 + aX1_Y0) {
				part.vertices[part.vIndex[Side.Back]++] = new VertexP3fT2fC4b(x1, y2, z2, u1, v1, col0_1);
				part.vertices[part.vIndex[Side.Back]++] = new VertexP3fT2fC4b(x1, y1, z2, u1, v2, col0_0);
				part.vertices[part.vIndex[Side.Back]++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z2, u2, v2, col1_0);
				part.vertices[part.vIndex[Side.Back]++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z2, u2, v1, col1_1);
			} else {
				part.vertices[part.vIndex[Side.Back]++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z2, u2, v1, col1_1);
				part.vertices[part.vIndex[Side.Back]++] = new VertexP3fT2fC4b(x1, y2, z2, u1, v1, col0_1);
				part.vertices[part.vIndex[Side.Back]++] = new VertexP3fT2fC4b(x1, y1, z2, u1, v2, col0_0);
				part.vertices[part.vIndex[Side.Back]++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z2, u2, v2, col1_0);
			}
		}
		
		void DrawBottomFace(int count) {
			int texId = info.textures[curBlock * Side.Sides + Side.Bottom];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Bottom) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			int F = bitFlags[cIndex];
			int aX0_Z0 = ((F >> xM1_yM1_zM1) & 1) + ((F >> xM1_yM1_zCC) & 1) + ((F >> xCC_yM1_zM1) & 1) + ((F >> xCC_yM1_zCC) & 1);
			int aX1_Z0 = ((F >> xP1_yM1_zM1) & 1) + ((F >> xP1_yM1_zCC) & 1) + ((F >> xCC_yM1_zM1) & 1) + ((F >> xCC_yM1_zCC) & 1);
			int aX0_Z1 = ((F >> xM1_yM1_zP1) & 1) + ((F >> xM1_yM1_zCC) & 1) + ((F >> xCC_yM1_zP1) & 1) + ((F >> xCC_yM1_zCC) & 1);
			int aX1_Z1 = ((F >> xP1_yM1_zP1) & 1) + ((F >> xP1_yM1_zCC) & 1) + ((F >> xCC_yM1_zP1) & 1) + ((F >> xCC_yM1_zCC) & 1);
			
			int col0_1 = fullBright ? FastColour.WhitePacked : lerpY[aX0_Z1], col1_1 = fullBright ? FastColour.WhitePacked : lerpY[aX1_Z1];
			int col1_0 = fullBright ? FastColour.WhitePacked : lerpY[aX1_Z0], col0_0 = fullBright ? FastColour.WhitePacked : lerpY[aX0_Z0];
			if (tinted) {
				FastColour tint = info.FogColour[curBlock];
				col0_0 = Utils.Tint(col0_0, tint);
				col1_0 = Utils.Tint(col1_0, tint);
				col1_1 = Utils.Tint(col1_1, tint);
				col0_1 = Utils.Tint(col0_1, tint);
			}
			
			if (aX0_Z1 + aX1_Z0 > aX0_Z0 + aX1_Z1) {
				part.vertices[part.vIndex[Side.Bottom]++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z2, u2, v2, col1_1);
				part.vertices[part.vIndex[Side.Bottom]++] = new VertexP3fT2fC4b(x1, y1, z2, u1, v2, col0_1);
				part.vertices[part.vIndex[Side.Bottom]++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v1, col0_0);
				part.vertices[part.vIndex[Side.Bottom]++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z1, u2, v1, col1_0);
			} else {
				part.vertices[part.vIndex[Side.Bottom]++] = new VertexP3fT2fC4b(x1, y1, z2, u1, v2, col0_1);
				part.vertices[part.vIndex[Side.Bottom]++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v1, col0_0);
				part.vertices[part.vIndex[Side.Bottom]++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z1, u2, v1, col1_0);
				part.vertices[part.vIndex[Side.Bottom]++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z2, u2, v2, col1_1);
			}
		}

		void DrawTopFace(int count) {
			int texId = info.textures[curBlock * Side.Sides + Side.Top];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Top) & 1;

			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			int F = bitFlags[cIndex];
			int aX0_Z0 = ((F >> xM1_yP1_zM1) & 1) + ((F >> xM1_yP1_zCC) & 1) + ((F >> xCC_yP1_zM1) & 1) + ((F >> xCC_yP1_zCC) & 1);
			int aX1_Z0 = ((F >> xP1_yP1_zM1) & 1) + ((F >> xP1_yP1_zCC) & 1) + ((F >> xCC_yP1_zM1) & 1) + ((F >> xCC_yP1_zCC) & 1);
			int aX0_Z1 = ((F >> xM1_yP1_zP1) & 1) + ((F >> xM1_yP1_zCC) & 1) + ((F >> xCC_yP1_zP1) & 1) + ((F >> xCC_yP1_zCC) & 1);
			int aX1_Z1 = ((F >> xP1_yP1_zP1) & 1) + ((F >> xP1_yP1_zCC) & 1) + ((F >> xCC_yP1_zP1) & 1) + ((F >> xCC_yP1_zCC) & 1);
			
			int col0_0 = fullBright ? FastColour.WhitePacked : lerp[aX0_Z0], col1_0 = fullBright ? FastColour.WhitePacked : lerp[aX1_Z0];
			int col1_1 = fullBright ? FastColour.WhitePacked : lerp[aX1_Z1], col0_1 = fullBright ? FastColour.WhitePacked : lerp[aX0_Z1];
			if (tinted) {
				FastColour tint = info.FogColour[curBlock];
				col0_0 = Utils.Tint(col0_0, tint);
				col1_0 = Utils.Tint(col1_0, tint);
				col1_1 = Utils.Tint(col1_1, tint);
				col0_1 = Utils.Tint(col0_1, tint);
			}
			
			if (aX0_Z0 + aX1_Z1 > aX0_Z1 + aX1_Z0) {
				part.vertices[part.vIndex[Side.Top]++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z1, u2, v1, col1_0);
				part.vertices[part.vIndex[Side.Top]++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col0_0);
				part.vertices[part.vIndex[Side.Top]++] = new VertexP3fT2fC4b(x1, y2, z2, u1, v2, col0_1);
				part.vertices[part.vIndex[Side.Top]++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z2, u2, v2, col1_1);
			} else {
				part.vertices[part.vIndex[Side.Top]++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col0_0);
				part.vertices[part.vIndex[Side.Top]++] = new VertexP3fT2fC4b(x1, y2, z2, u1, v2, col0_1);
				part.vertices[part.vIndex[Side.Top]++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z2, u2, v2, col1_1);
				part.vertices[part.vIndex[Side.Top]++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z1, u2, v1, col1_0);
			}
		}
		
		int[] lerp = new int[5], lerpX = new int[5], lerpZ = new int[5], lerpY = new int[5];		
		protected override void PreStretchTiles(int x1, int y1, int z1) {
			base.PreStretchTiles(x1, y1, z1);
			for (int i = 0; i <= 4; i++) {
				lerp[i]  = Lerp(env.Shadow, env.Sun, i / 4f);
				lerpX[i] = Lerp(env.ShadowXSide, env.SunXSide, i / 4f);
				lerpZ[i] = Lerp(env.ShadowZSide, env.SunZSide, i / 4f);
				lerpY[i] = Lerp(env.ShadowYBottom, env.SunYBottom, i / 4f);
			}
		}
		
		static int Lerp(int a, int b, float t) {
			int c = FastColour.BlackPacked;
			c |= (byte)Utils.Lerp((a & 0xFF0000) >> 16, (b & 0xFF0000) >> 16, t) << 16;
			c |= (byte)Utils.Lerp((a & 0x00FF00) >> 8, (b & 0x00FF00) >> 8, t) << 8;
			c |= (byte)Utils.Lerp(a & 0x0000FF, b & 0x0000FF, t);
			return c;
		}
		
		
		#region Light computation
		
		int ComputeLightFlags(int x, int y, int z, int cIndex) {
			if (fullBright) return (1 << xP1_yP1_zP1) - 1; // all faces fully bright

			return
				Lit(x - 1, y, z - 1, cIndex - 1 - 18) << xM1_yM1_zM1 |
				Lit(x - 1, y, z,     cIndex - 1)      << xM1_yM1_zCC |
				Lit(x - 1, y, z + 1, cIndex - 1 + 18) << xM1_yM1_zP1 |
				Lit(x, y, z - 1,     cIndex + 0 - 18) << xCC_yM1_zM1 |
				Lit(x, y, z,         cIndex + 0)      << xCC_yM1_zCC |
				Lit(x, y, z + 1 ,    cIndex + 0 + 18) << xCC_yM1_zP1 |
				Lit(x + 1, y, z - 1, cIndex + 1 - 18) << xP1_yM1_zM1 |
				Lit(x + 1, y, z,     cIndex + 1)      << xP1_yM1_zCC |
				Lit(x + 1, y, z + 1, cIndex + 1 + 18) << xP1_yM1_zP1 ;
		}
		
		const int xM1_yM1_zM1 = 0,  xM1_yCC_zM1 = 1,  xM1_yP1_zM1 = 2;
		const int xCC_yM1_zM1 = 3,  xCC_yCC_zM1 = 4,  xCC_yP1_zM1 = 5;
		const int xP1_yM1_zM1 = 6,  xP1_yCC_zM1 = 7,  xP1_yP1_zM1 = 8;

		const int xM1_yM1_zCC = 9,  xM1_yCC_zCC = 10, xM1_yP1_zCC = 11;
		const int xCC_yM1_zCC = 12, xCC_yCC_zCC = 13, xCC_yP1_zCC = 14;
		const int xP1_yM1_zCC = 15, xP1_yCC_zCC = 16, xP1_yP1_zCC = 17;

		const int xM1_yM1_zP1 = 18, xM1_yCC_zP1 = 19, xM1_yP1_zP1 = 20;
		const int xCC_yM1_zP1 = 21, xCC_yCC_zP1 = 22, xCC_yP1_zP1 = 23;
		const int xP1_yM1_zP1 = 24, xP1_yCC_zP1 = 25, xP1_yP1_zP1 = 26;
		
		int Lit(int x, int y, int z, int cIndex) {
			if (y < 0 || y >= height) return 7; // all faces lit
			
			// TODO: check sides height (if sides > edges), check if edge block casts a shadow
			if (x < 0 || z < 0 || x >= width || z >= length) {
				return y >= edgeLevel ? 7 : y == (edgeLevel - 1) ? 6 : 0;
			}
			
			int flags = 0;
			BlockID block = chunk[cIndex];
			int lightHeight = light.heightmap[(z * width) + x];
			lightFlags = info.LightOffset[block];

			// Use fact Light(Y.Bottom) == Light((Y - 1).Top)
			int offset = (lightFlags >> Side.Bottom) & 1;
			flags |= ((y - offset) > lightHeight ? 1 : 0);
			
			// Light is same for all the horizontal faces
			flags |= (y > lightHeight ? 2 : 0);
			
			// Use fact Light((Y + 1).Bottom) == Light(Y.Top)
			offset = (lightFlags >> Side.Top) & 1;
			flags |= ((y - offset) >= lightHeight ? 4 : 0);
			
			// Dynamic lighting
			if (info.FullBright[block])               flags |= 5;
			if (info.FullBright[chunk[cIndex + 324]]) flags |= 4;
			if (info.FullBright[chunk[cIndex - 324]]) flags |= 1;
			return flags;
		}
		
		static int[] masks = {
			// Left face
			(1 << xM1_yM1_zM1) | (1 << xM1_yM1_zCC) | (1 << xM1_yM1_zP1) |
				(1 << xM1_yCC_zM1) | (1 << xM1_yCC_zCC) | (1 << xM1_yCC_zP1) |
				(1 << xM1_yP1_zM1) | (1 << xM1_yP1_zCC) | (1 << xM1_yP1_zP1),
			// Right face
			(1 << xP1_yM1_zM1) | (1 << xP1_yM1_zCC) | (1 << xP1_yM1_zP1) |
				(1 << xP1_yP1_zM1) | (1 << xP1_yP1_zCC) | (1 << xP1_yP1_zP1) |
				(1 << xP1_yCC_zM1) | (1 << xP1_yCC_zCC) | (1 << xP1_yCC_zP1),
			// Front face
			(1 << xM1_yM1_zM1) | (1 << xCC_yM1_zM1) | (1 << xP1_yM1_zM1) |
				(1 << xM1_yCC_zM1) | (1 << xCC_yCC_zM1) | (1 << xP1_yCC_zM1) |
				(1 << xM1_yP1_zM1) | (1 << xCC_yP1_zM1) | (1 << xP1_yP1_zM1),
			// Back face
			(1 << xM1_yM1_zP1) | (1 << xCC_yM1_zP1) | (1 << xP1_yM1_zP1) |
				(1 << xM1_yCC_zP1) | (1 << xCC_yCC_zP1) | (1 << xP1_yCC_zP1) |
				(1 << xM1_yP1_zP1) | (1 << xCC_yP1_zP1) | (1 << xP1_yP1_zP1),
			// Bottom face
			(1 << xM1_yM1_zM1) | (1 << xM1_yM1_zCC) | (1 << xM1_yM1_zP1) |
				(1 << xCC_yM1_zM1) | (1 << xCC_yM1_zCC) | (1 << xCC_yM1_zP1) |
				(1 << xP1_yM1_zM1) | (1 << xP1_yM1_zCC) | (1 << xP1_yM1_zP1),
			// Top face
			(1 << xM1_yP1_zM1) | (1 << xM1_yP1_zCC) | (1 << xM1_yP1_zP1) |
				(1 << xCC_yP1_zM1) | (1 << xCC_yP1_zCC) | (1 << xCC_yP1_zP1) |
				(1 << xP1_yP1_zM1) | (1 << xP1_yP1_zCC) | (1 << xP1_yP1_zP1),
		};
		
		#endregion
	}
}