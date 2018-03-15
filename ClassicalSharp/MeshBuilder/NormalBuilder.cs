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

	public unsafe sealed class NormalMeshBuilder : ChunkMeshBuilder {
		
		CuboidDrawer drawer = new CuboidDrawer();
		
		protected override int StretchXLiquid(int countIndex, int x, int y, int z, int chunkIndex, BlockID block) {
			if (OccludedLiquid(chunkIndex)) return 0;
			int count = 1;
			x++;
			chunkIndex++;
			countIndex += Side.Sides;
			bool stretchTile = (BlockInfo.CanStretch[block] & (1 << Side.Top)) != 0;
			
			while (x < chunkEndX && stretchTile && CanStretch(block, chunkIndex, x, y, z, Side.Top) && !OccludedLiquid(chunkIndex)) {
				counts[countIndex] = 0;
				count++;
				x++;
				chunkIndex++;
				countIndex += Side.Sides;
			}
			return count;
		}
		
		protected override int StretchX(int countIndex, int x, int y, int z, int chunkIndex, BlockID block, int face) {
			int count = 1;
			x++;
			chunkIndex++;
			countIndex += Side.Sides;
			bool stretchTile = (BlockInfo.CanStretch[block] & (1 << face)) != 0;
			
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
			int count = 1;
			z++;
			chunkIndex += extChunkSize;
			countIndex += chunkSize * Side.Sides;
			bool stretchTile = (BlockInfo.CanStretch[block] & (1 << face)) != 0;
			
			while (z < chunkEndZ && stretchTile && CanStretch(block, chunkIndex, x, y, z, face)) {
				counts[countIndex] = 0;
				count++;
				z++;
				chunkIndex += extChunkSize;
				countIndex += chunkSize * Side.Sides;
			}
			return count;
		}
		
		bool CanStretch(BlockID initial, int chunkIndex, int x, int y, int z, int face) {
			BlockID cur = chunk[chunkIndex];
			return cur == initial
				&& !BlockInfo.IsFaceHidden(cur, chunk[chunkIndex + offsets[face]], face)
				&& (fullBright || (LightCol(X, Y, Z, face, initial) == LightCol(x, y, z, face, cur)));
		}
		
		int LightCol(int x, int y, int z, int face, BlockID block) {
			int offset = (BlockInfo.LightOffset[block] >> face) & 1;
			switch (face) {
				case Side.Left:
					return x < offset          ? light.OutsideXSide   : light.LightCol_XSide_Fast(x - offset, y, z);
				case Side.Right:
					return x > (maxX - offset) ? light.OutsideXSide   : light.LightCol_XSide_Fast(x + offset, y, z);
				case Side.Front:
					return z < offset          ? light.OutsideZSide   : light.LightCol_ZSide_Fast(x, y, z - offset);
				case Side.Back:
					return z > (maxZ - offset) ? light.OutsideZSide   : light.LightCol_ZSide_Fast(x, y, z + offset);
				case Side.Bottom:
					return y <= 0              ? light.OutsideYBottom : light.LightCol_YBottom_Fast(x, y - offset, z);
				case Side.Top:
					return y >= maxY           ? light.Outside        : light.LightCol_YTop_Fast(x, (y + 1) - offset, z);
			}
			return 0;
		}
		
		protected override void PreStretchTiles(int x1, int y1, int z1) {
			base.PreStretchTiles(x1, y1, z1);
			drawer.invVerElementSize  = invVerElementSize;
			drawer.elementsPerAtlas1D = elementsPerAtlas1D;
		}
		
		protected override void RenderTile(int index) {
			if (BlockInfo.Draw[curBlock] == DrawType.Sprite) {
				this.fullBright = BlockInfo.FullBright[curBlock];
				this.tinted = BlockInfo.Tinted[curBlock];
				int count = counts[index + Side.Top];
				if (count != 0) DrawSprite(count);
				return;
			}
			
			int leftCount = counts[index++], rightCount = counts[index++],
			frontCount = counts[index++], backCount = counts[index++],
			bottomCount = counts[index++], topCount = counts[index++];
			if (leftCount == 0 && rightCount == 0 && frontCount == 0 &&
			    backCount == 0 && bottomCount == 0 && topCount == 0) return;
			
			bool fullBright = BlockInfo.FullBright[curBlock];
			bool isTranslucent = BlockInfo.Draw[curBlock] == DrawType.Translucent;
			int lightFlags = BlockInfo.LightOffset[curBlock];
			
			drawer.minBB = BlockInfo.MinBB[curBlock]; drawer.minBB.Y = 1 - drawer.minBB.Y;
			drawer.maxBB = BlockInfo.MaxBB[curBlock]; drawer.maxBB.Y = 1 - drawer.maxBB.Y;
			
			Vector3 min = BlockInfo.RenderMinBB[curBlock], max = BlockInfo.RenderMaxBB[curBlock];
			drawer.x1 = X + min.X; drawer.y1 = Y + min.Y; drawer.z1 = Z + min.Z;
			drawer.x2 = X + max.X; drawer.y2 = Y + max.Y; drawer.z2 = Z + max.Z;
			
			drawer.Tinted = BlockInfo.Tinted[curBlock];
			drawer.TintColour = BlockInfo.FogColour[curBlock];
			
			if (leftCount != 0) {
				int texLoc = BlockInfo.textures[curBlock * Side.Sides + Side.Left];
				int i = texLoc / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Left) & 1;
				
				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked :
					X >= offset ? light.LightCol_XSide_Fast(X - offset, Y, Z) : light.OutsideXSide;
				drawer.Left(leftCount, col, texLoc, part.vertices, ref part.vIndex[Side.Left]);
			}
			
			if (rightCount != 0) {
				int texLoc = BlockInfo.textures[curBlock * Side.Sides + Side.Right];
				int i = texLoc / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Right) & 1;
				
				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked :
					X <= (maxX - offset) ? light.LightCol_XSide_Fast(X + offset, Y, Z) : light.OutsideXSide;
				drawer.Right(rightCount, col, texLoc, part.vertices, ref part.vIndex[Side.Right]);
			}
			
			if (frontCount != 0) {
				int texLoc = BlockInfo.textures[curBlock * Side.Sides + Side.Front];
				int i = texLoc / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Front) & 1;
				
				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked :
					Z >= offset ? light.LightCol_ZSide_Fast(X, Y, Z - offset) : light.OutsideZSide;
				drawer.Front(frontCount, col, texLoc, part.vertices, ref part.vIndex[Side.Front]);
			}
			
			if (backCount != 0) {
				int texLoc = BlockInfo.textures[curBlock * Side.Sides + Side.Back];
				int i = texLoc / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Back) & 1;
				
				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked :
					Z <= (maxZ - offset) ? light.LightCol_ZSide_Fast(X, Y, Z + offset) : light.OutsideZSide;
				drawer.Back(backCount, col, texLoc, part.vertices, ref part.vIndex[Side.Back]);
			}
			
			if (bottomCount != 0) {
				int texLoc = BlockInfo.textures[curBlock * Side.Sides + Side.Bottom];
				int i = texLoc / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Bottom) & 1;
				
				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked : light.LightCol_YBottom_Fast(X, Y - offset, Z);
				drawer.Bottom(bottomCount, col, texLoc, part.vertices, ref part.vIndex[Side.Bottom]);
			}
			
			if (topCount != 0) {
				int texLoc = BlockInfo.textures[curBlock * Side.Sides + Side.Top];
				int i = texLoc / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Top) & 1;

				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked : light.LightCol_YTop_Fast(X, (Y + 1) - offset, Z);
				drawer.Top(topCount, col, texLoc, part.vertices, ref part.vIndex[Side.Top]);
			}
		}
	}
}