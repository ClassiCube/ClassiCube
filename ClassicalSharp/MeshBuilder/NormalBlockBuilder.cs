// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {

	public unsafe sealed class NormalBlockBuilder {
		
		public BlockInfo info;
		public int elementsPerAtlas1D;
		public float invVerElementSize;
		
		public byte curBlock;
		public bool tinted;
		public Vector3 minBB, maxBB;
		public float x1, y1, z1, x2, y2, z2;
		
		
		public void DrawLeftFace(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;			
			float u1 = minBB.Z, u2 = (count - 1) + maxBB.Z * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			if (tinted) col = TintBlock(curBlock, col);
			
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z2 + (count - 1), u2, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z2 + (count - 1), u2, v2, col);
		}

		public void DrawRightFace(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;			
			float u1 = (count - minBB.Z), u2 = (1 - maxBB.Z) * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			if (tinted) col = TintBlock(curBlock, col);
			
			vertices[index++] = new VertexP3fT2fC4b(x2, y2, z1, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x2, y2, z2 + (count - 1), u2, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x2, y1, z2 + (count - 1), u2, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x2, y1, z1, u1, v2, col);
		}

		public void DrawFrontFace(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;			
			float u1 = (count - minBB.X), u2 = (1 - maxBB.X) * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			if (tinted) col = TintBlock(curBlock, col);
			
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z1, u2, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z1, u2, v1, col);
		}
		
		public void DrawBackFace(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;			
			if (tinted) col = TintBlock(curBlock, col);
			
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z2, u2, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z2, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z2, u1, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z2, u2, v2, col);
		}
		
		public void DrawBottomFace(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			if (tinted) col = TintBlock(curBlock, col);
			
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z2, u2, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z2, u1, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z1, u2, v1, col);
		}

		public void DrawTopFace(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			if (tinted) col = TintBlock(curBlock, col);
			
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z1, u2, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z2, u1, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z2, u2, v2, col);
		}

		int TintBlock(byte curBlock, int col) {
			FastColour fogCol = info.FogColour[curBlock];
			FastColour newCol = FastColour.Unpack(col);
			newCol *= fogCol;
			return newCol.Pack();
		}
	}
}