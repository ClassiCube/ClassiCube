// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Runtime.InteropServices;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Textures;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp {
	
	public unsafe partial class ChunkMeshBuilder {
		
		protected DrawInfo[] normalParts, translucentParts;
		protected TerrainAtlas1D atlas;
		protected int arraysCount = 0;
		protected bool fullBright, tinted;
		protected float invVerElementSize;
		protected int elementsPerAtlas1D;
		
		void TerrainAtlasChanged(object sender, EventArgs e) {
			int newArraysCount = game.TerrainAtlas1D.TexIds.Length;
			if (arraysCount == newArraysCount) return;
			arraysCount = newArraysCount;
			Array.Resize(ref normalParts, arraysCount);
			Array.Resize(ref translucentParts, arraysCount);
			
			for (int i = 0; i < normalParts.Length; i++) {
				if (normalParts[i] != null) continue;
				normalParts[i] = new DrawInfo();
				translucentParts[i] = new DrawInfo();
			}
		}
		
		public void Dispose() {
			game.Events.TerrainAtlasChanged -= TerrainAtlasChanged;
		}
		
		protected class DrawInfo {
			public VertexP3fT2fC4b[] vertices;
			public int[] vIndex = new int[6], vCount = new int[6];
			public int spriteCount, sIndex, sAdvance;
			
			public int VerticesCount() {
				int count = spriteCount;
				for (int i = 0; i < vCount.Length; i++) { count += vCount[i]; }
				return count;
			}
			
			public void ExpandToCapacity() {
				int vertsCount = VerticesCount();
				if (vertices == null || (vertsCount + 2) > vertices.Length) {
					vertices = new VertexP3fT2fC4b[vertsCount + 2]; 
					// ensure buffer is up to 64 bits aligned for last element
				}
				sIndex = 0; 
				sAdvance = spriteCount / 4;
				
				vIndex[Side.Left]   = spriteCount;
				vIndex[Side.Right]  = vIndex[Side.Left]   + vCount[Side.Left];
				vIndex[Side.Front]  = vIndex[Side.Right]  + vCount[Side.Right];
				vIndex[Side.Back]   = vIndex[Side.Front]  + vCount[Side.Front];
				vIndex[Side.Bottom] = vIndex[Side.Back]   + vCount[Side.Back];
				vIndex[Side.Top]    = vIndex[Side.Bottom] + vCount[Side.Bottom];
			}
			
			public void ResetState() {
				spriteCount = 0; sIndex = 0; sAdvance = 0;
				for (int i = 0; i < Side.Sides; i++) {
					vIndex[i] = 0; vCount[i] = 0;
				}
			}
		}		

		protected abstract void RenderTile(int index);
		
		protected virtual void PreStretchTiles(int x1, int y1, int z1) {
			atlas = game.TerrainAtlas1D;
			invVerElementSize = atlas.invElementSize;
			elementsPerAtlas1D = atlas.elementsPerAtlas1D;
			arraysCount = atlas.TexIds.Length;
			
			if (normalParts == null) {
				normalParts = new DrawInfo[arraysCount];
				translucentParts = new DrawInfo[arraysCount];
				for (int i = 0; i < normalParts.Length; i++) {
					normalParts[i] = new DrawInfo();
					translucentParts[i] = new DrawInfo();
				}
			} else {
				for (int i = 0; i < normalParts.Length; i++) {
					normalParts[i].ResetState();
					translucentParts[i].ResetState();
				}
			}
		}
		
		protected virtual void PostStretchTiles(int x1, int y1, int z1) {
			for (int i = 0; i < normalParts.Length; i++) {
				normalParts[i].ExpandToCapacity();
				translucentParts[i].ExpandToCapacity();
			}
		}
		
		void AddSpriteVertices(BlockID block) {
			int i = atlas.Get1DIndex(BlockInfo.GetTextureLoc(block, Side.Left));
			DrawInfo part = normalParts[i];
			part.spriteCount += 4 * 4;
		}
		
		void AddVertices(BlockID block, int face) {
			int i = atlas.Get1DIndex(BlockInfo.GetTextureLoc(block, face));
			DrawInfo part = BlockInfo.Draw[block] == DrawType.Translucent ? translucentParts[i] : normalParts[i];
			part.vCount[face] += 4;
		}
		
		protected virtual void DrawSprite(int count) {
			int texId = BlockInfo.textures[curBlock * Side.Sides + Side.Right];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			
			float x1 = X + 2.50f/16, y1 = Y,     z1 = Z + 2.5f/16;
			float x2 = X + 13.5f/16, y2 = Y + 1, z2 = Z + 13.5f/16;
			const float u1 = 0, u2 = 15.99f/16f;
			float v1 = vOrigin, v2 = vOrigin + invVerElementSize * 15.99f/16f;
			
			DrawInfo part = normalParts[i];
			int col = fullBright ? FastColour.WhitePacked : light.LightCol_Sprite_Fast(X, Y, Z);
			if (tinted) col = TintBlock(curBlock, col);
			VertexP3fT2fC4b v; v.Colour = col;
			
			// Draw Z axis
			int index = part.sIndex;
			v.X = x1; v.Y = y1; v.Z = z1; v.U = u2; v.V = v2; part.vertices[index + 0] = v;
			          v.Y = y2;                     v.V = v1; part.vertices[index + 1] = v;
			v.X = x2;           v.Z = z2; v.U = u1;           part.vertices[index + 2] = v;
			          v.Y = y1;                     v.V = v2; part.vertices[index + 3] = v;
			
			// Draw Z axis mirrored
			index += part.sAdvance;
			v.X = x2; v.Y = y1; v.Z = z2; v.U = u2;           part.vertices[index + 0] = v;
			          v.Y = y2;                     v.V = v1; part.vertices[index + 1] = v;
			v.X = x1;           v.Z = z1; v.U = u1;           part.vertices[index + 2] = v;
			          v.Y = y1;                     v.V = v2; part.vertices[index + 3] = v;

			// Draw X axis
			index += part.sAdvance;
			v.X = x1; v.Y = y1; v.Z = z2; v.U = u2;           part.vertices[index + 0] = v;
			          v.Y = y2;                     v.V = v1; part.vertices[index + 1] = v;
			v.X = x2;           v.Z = z1; v.U = u1;           part.vertices[index + 2] = v;
			          v.Y = y1;                     v.V = v2; part.vertices[index + 3] = v;
			
			// Draw X axis mirrored
			index += part.sAdvance;
			v.X = x2; v.Y = y1; v.Z = z1; v.U = u2;           part.vertices[index + 0] = v;
			          v.Y = y2;                     v.V = v1; part.vertices[index + 1] = v;
			v.X = x1;           v.Z = z2; v.U = u1;           part.vertices[index + 2] = v;
			          v.Y = y1;                     v.V = v2; part.vertices[index + 3] = v;
			
			part.sIndex += 4;
		}
		
		protected int TintBlock(BlockID curBlock, int col) {
			FastColour fogCol = BlockInfo.FogColour[curBlock];
			FastColour newCol = FastColour.Unpack(col);
			newCol *= fogCol;
			return newCol.Pack();
		}
	}
}