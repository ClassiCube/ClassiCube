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
			public int[] vIndex = new int[6], sIndex = new int[6], vCount = new int[6];
			public int iCount, spriteCount;
			
			public void ExpandToCapacity() {
				int vertCount = iCount / 6 * 4;
				if (vertices == null || (vertCount + 2) > vertices.Length) {
					vertices = new VertexP3fT2fC4b[vertCount + 2]; 
					// ensure buffer is up to 64 bits aligned for last element
				}	
				
				// Adjust for the fact that we group all vertices by face.
				sIndex[Side.Left]  = (spriteCount / 6) * 0;
				sIndex[Side.Right] = (spriteCount / 6) * 1;
				sIndex[Side.Front] = (spriteCount / 6) * 2;
				sIndex[Side.Back]  = (spriteCount / 6) * 3;
				
				vIndex[Side.Left]   = (spriteCount / 6) * 4;
				vIndex[Side.Right]  = vIndex[Side.Left]   + (vCount[Side.Left]   / 6) * 4;
				vIndex[Side.Front]  = vIndex[Side.Right]  + (vCount[Side.Right]  / 6) * 4;
				vIndex[Side.Back]   = vIndex[Side.Front]  + (vCount[Side.Front]  / 6) * 4;
				vIndex[Side.Bottom] = vIndex[Side.Back]   + (vCount[Side.Back]   / 6) * 4;
				vIndex[Side.Top]    = vIndex[Side.Bottom] + (vCount[Side.Bottom] / 6) * 4;
			}
			
			public void ResetState() {
				iCount = 0; spriteCount = 0;
				for (int i = 0; i < Side.Sides; i++) {
					vIndex[i] = 0; sIndex[i] = 0; vCount[i] = 0;
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
			int i = atlas.Get1DIndex(info.GetTextureLoc(block, Side.Left));
			DrawInfo part = normalParts[i];
			part.spriteCount += 6 * 4;
			part.iCount += 6 * 4;
		}
		
		void AddVertices(BlockID block, int count, int face) {
			int i = atlas.Get1DIndex(info.GetTextureLoc(block, face));
			DrawInfo part = info.Draw[block] == DrawType.Translucent ? translucentParts[i] : normalParts[i];
			part.iCount += 6;
			part.vCount[face] += 6;
		}
		
		protected void DrawSprite(int count) {
			int texId = info.textures[curBlock * Side.Sides + Side.Right];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			const float blockHeight = 1;
			
			const float u1 = 0, u2 = 15.99f/16f;
			float v1 = vOrigin, v2 = vOrigin + invVerElementSize * 15.99f/16f;
			DrawInfo part = normalParts[i];
			int col = fullBright ? FastColour.WhitePacked : light.LightCol_Sprite_Fast(X, Y, Z);
			if (tinted) col = TintBlock(curBlock, col);
			
			// Draw Z axis
			part.vertices[part.sIndex[0]++] = new VertexP3fT2fC4b(X + 2.50f/16, Y, Z + 2.5f/16, u2, v2, col);
			part.vertices[part.sIndex[0]++] = new VertexP3fT2fC4b(X + 2.50f/16, Y + blockHeight, Z + 2.5f/16, u2, v1, col);
			part.vertices[part.sIndex[0]++] = new VertexP3fT2fC4b(X + 13.5f/16, Y + blockHeight, Z + 13.5f/16, u1, v1, col);
			part.vertices[part.sIndex[0]++] = new VertexP3fT2fC4b(X + 13.5f/16, Y, Z + 13.5f/16, u1, v2, col);
			
			// Draw Z axis mirrored
			part.vertices[part.sIndex[1]++] = new VertexP3fT2fC4b(X + 13.5f/16, Y, Z + 13.5f/16, u2, v2, col);
			part.vertices[part.sIndex[1]++] = new VertexP3fT2fC4b(X + 13.5f/16, Y + blockHeight, Z + 13.5f/16, u2, v1, col);
			part.vertices[part.sIndex[1]++] = new VertexP3fT2fC4b(X + 2.50f/16, Y + blockHeight, Z + 2.5f/16, u1, v1, col);
			part.vertices[part.sIndex[1]++] = new VertexP3fT2fC4b(X + 2.50f/16, Y, Z + 2.5f/16, u1, v2, col);

			// Draw X axis
			part.vertices[part.sIndex[2]++] = new VertexP3fT2fC4b(X + 2.50f/16, Y, Z + 13.5f/16, u2, v2, col);
			part.vertices[part.sIndex[2]++] = new VertexP3fT2fC4b(X + 2.50f/16, Y + blockHeight, Z + 13.5f/16, u2, v1, col);
			part.vertices[part.sIndex[2]++] = new VertexP3fT2fC4b(X + 13.5f/16, Y + blockHeight, Z + 2.5f/16, u1, v1, col);
			part.vertices[part.sIndex[2]++] = new VertexP3fT2fC4b(X + 13.5f/16, Y, Z + 2.5f/16, u1, v2, col);
			
			// Draw X axis mirrored
			part.vertices[part.sIndex[3]++] = new VertexP3fT2fC4b(X + 13.5f/16, Y, Z + 2.5f/16, u2, v2, col);
			part.vertices[part.sIndex[3]++] = new VertexP3fT2fC4b(X + 13.5f/16, Y + blockHeight, Z + 2.5f/16, u2, v1, col);
			part.vertices[part.sIndex[3]++] = new VertexP3fT2fC4b(X + 2.50f/16, Y + blockHeight, Z + 13.5f/16, u1, v1, col);
			part.vertices[part.sIndex[3]++] = new VertexP3fT2fC4b(X + 2.50f/16, Y, Z + 13.5f/16, u1, v2, col);
		}
		
		protected int TintBlock(BlockID curBlock, int col) {
			FastColour fogCol = info.FogColour[curBlock];
			FastColour newCol = FastColour.Unpack(col);
			newCol *= fogCol;
			return newCol.Pack();
		}
	}
}