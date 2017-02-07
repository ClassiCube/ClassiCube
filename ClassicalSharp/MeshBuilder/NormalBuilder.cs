// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp {

	public unsafe sealed class NormalMeshBuilder : ChunkMeshBuilder {
		
		CuboidDrawer drawer = new CuboidDrawer();
		
		protected override int StretchXLiquid(int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block) {
			if (OccludedLiquid(chunkIndex)) return 0;
			int count = 1;
			x++;
			chunkIndex++;
			countIndex += Side.Sides;
			int max = chunkSize - xx;
			
			while (count < max && x < width && CanStretch(block, chunkIndex, x, y, z, Side.Top)
			       && !OccludedLiquid(chunkIndex)) {
				counts[countIndex] = 0;
				count++;
				x++;
				chunkIndex++;
				countIndex += Side.Sides;
			}
			return count;
		}
		
		protected override int StretchX(int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face) {
			int count = 1;
			x++;
			chunkIndex++;
			countIndex += Side.Sides;
			int max = chunkSize - xx;
			bool stretchTile = (info.CanStretch[block] & (1 << face)) != 0;
			
			while (count < max && x < width && stretchTile && CanStretch(block, chunkIndex, x, y, z, face)) {
				counts[countIndex] = 0;
				count++;
				x++;
				chunkIndex++;
				countIndex += Side.Sides;
			}
			return count;
		}
		
		protected override int StretchZ(int zz, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face) {
			int count = 1;
			z++;
			chunkIndex += extChunkSize;
			countIndex += chunkSize * Side.Sides;
			int max = chunkSize - zz;
			bool stretchTile = (info.CanStretch[block] & (1 << face)) != 0;
			
			while (count < max && z < length && stretchTile && CanStretch(block, chunkIndex, x, y, z, face)) {
				counts[countIndex] = 0;
				count++;
				z++;
				chunkIndex += extChunkSize;
				countIndex += chunkSize * Side.Sides;
			}
			return count;
		}
		
		bool CanStretch(byte initialTile, int chunkIndex, int x, int y, int z, int face) {
			byte rawBlock = chunk[chunkIndex];
			return rawBlock == initialTile
				&& !info.IsFaceHidden(rawBlock, chunk[chunkIndex + offsets[face]], face)
				&& (fullBright || IsLit(X, Y, Z, face, initialTile) == IsLit(x, y, z, face, rawBlock));
		}
		
		protected override void PreStretchTiles(int x1, int y1, int z1) {
			base.PreStretchTiles(x1, y1, z1);
			drawer.invVerElementSize = invVerElementSize;
			drawer.elementsPerAtlas1D = elementsPerAtlas1D;
		}
		
		protected override void RenderTile(int index) {
			if (info.Draw[curBlock] == DrawType.Sprite) {
				this.fullBright = info.FullBright[curBlock];
				this.tinted = info.Tinted[curBlock];
				int count = counts[index + Side.Top];
				if (count != 0) DrawSprite(count);
				return;
			}
			
			int leftCount = counts[index++], rightCount = counts[index++],
			frontCount = counts[index++], backCount = counts[index++],
			bottomCount = counts[index++], topCount = counts[index++];
			if (leftCount == 0 && rightCount == 0 && frontCount == 0 &&
			    backCount == 0 && bottomCount == 0 && topCount == 0) return;
			
			bool fullBright = info.FullBright[curBlock];
			bool isTranslucent = info.Draw[curBlock] == DrawType.Translucent;
			int lightFlags = info.LightOffset[curBlock];
			
			drawer.minBB = info.MinBB[curBlock]; drawer.minBB.Y = 1 - drawer.minBB.Y;
			drawer.maxBB = info.MaxBB[curBlock]; drawer.maxBB.Y = 1 - drawer.maxBB.Y;
			
			Vector3 min = info.RenderMinBB[curBlock], max = info.RenderMaxBB[curBlock];
			drawer.x1 = X + min.X; drawer.y1 = Y + min.Y; drawer.z1 = Z + min.Z;
			drawer.x2 = X + max.X; drawer.y2 = Y + max.Y; drawer.z2 = Z + max.Z;
			
			drawer.Tinted = game.BlockInfo.Tinted[curBlock];
			drawer.TintColour = game.BlockInfo.FogColour[curBlock];
			
			if (leftCount != 0) {
				int texId = info.textures[curBlock * Side.Sides + Side.Left];
				int i = texId / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Left) & 1;
				
				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked :
					X >= offset ? lighting.LightCol_XSide_Fast(X - offset, Y, Z) : lighting.OutsideXSide;
				drawer.Left(leftCount, col, texId, part.vertices, ref part.vIndex[Side.Left]);
			}
			
			if (rightCount != 0) {
				int texId = info.textures[curBlock * Side.Sides + Side.Right];
				int i = texId / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Right) & 1;
				
				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked :
					X <= (maxX - offset) ? lighting.LightCol_XSide_Fast(X + offset, Y, Z) : lighting.OutsideXSide;
				drawer.Right(rightCount, col, texId, part.vertices, ref part.vIndex[Side.Right]);
			}
			
			if (frontCount != 0) {
				int texId = info.textures[curBlock * Side.Sides + Side.Front];
				int i = texId / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Front) & 1;
				
				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked :
					Z >= offset ? lighting.LightCol_ZSide_Fast(X, Y, Z - offset) : lighting.OutsideZSide;
				drawer.Front(frontCount, col, texId, part.vertices, ref part.vIndex[Side.Front]);
			}
			
			if (backCount != 0) {
				int texId = info.textures[curBlock * Side.Sides + Side.Back];
				int i = texId / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Back) & 1;
				
				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked :
					Z <= (maxZ - offset) ? lighting.LightCol_ZSide_Fast(X, Y, Z + offset) : lighting.OutsideZSide;
				drawer.Back(backCount, col, texId, part.vertices, ref part.vIndex[Side.Back]);
			}
			
			if (bottomCount != 0) {
				int texId = info.textures[curBlock * Side.Sides + Side.Bottom];
				int i = texId / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Bottom) & 1;
				
				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked : lighting.LightCol_YBottom_Fast(X, Y - offset, Z);
				drawer.Bottom(bottomCount, col, texId, part.vertices, ref part.vIndex[Side.Bottom]);
			}
			
			if (topCount != 0) {
				int texId = info.textures[curBlock * Side.Sides + Side.Top];
				int i = texId / elementsPerAtlas1D;
				int offset = (lightFlags >> Side.Top) & 1;

				DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
				int col = fullBright ? FastColour.WhitePacked : lighting.LightCol_YTop_Fast(X, Y - offset, Z);
				drawer.Top(topCount, col, texId, part.vertices, ref part.vIndex[Side.Top]);
			}
		}
	}
}