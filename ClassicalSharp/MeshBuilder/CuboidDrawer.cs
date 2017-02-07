// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {

	/// <summary> Draws the vertices for a cuboid region. </summary>
	public unsafe sealed class CuboidDrawer {

		public int elementsPerAtlas1D;
		public float invVerElementSize;
		
		/// <summary> Whether a colour tinting effect should be applied to all faces. </summary>
		public bool Tinted;
		
		/// <summary> The tint colour to multiply colour of faces by. </summary>
		public FastColour TintColour;
		
		public Vector3 minBB, maxBB;
		public float x1, y1, z1, x2, y2, z2;
		
		
		/// <summary> Draws the left face of the given cuboid region. </summary>
		public void Left(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;			
			float u1 = minBB.Z, u2 = (count - 1) + maxBB.Z * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			if (Tinted) col = TintBlock(col);
			
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z2 + (count - 1), u2, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z2 + (count - 1), u2, v2, col);
		}

		/// <summary> Draws the right face of the given cuboid region. </summary>
		public void Right(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;			
			float u1 = (count - minBB.Z), u2 = (1 - maxBB.Z) * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			if (Tinted) col = TintBlock(col);
			
			vertices[index++] = new VertexP3fT2fC4b(x2, y2, z1, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x2, y2, z2 + (count - 1), u2, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x2, y1, z2 + (count - 1), u2, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x2, y1, z1, u1, v2, col);
		}

		/// <summary> Draws the front face of the given cuboid region. </summary>
		public void Front(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;			
			float u1 = (count - minBB.X), u2 = (1 - maxBB.X) * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			if (Tinted) col = TintBlock(col);
			
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z1, u2, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z1, u2, v1, col);
		}
		
		/// <summary> Draws the back face of the given cuboid region. </summary>
		public void Back(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;			
			if (Tinted) col = TintBlock(col);
			
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z2, u2, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z2, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z2, u1, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z2, u2, v2, col);
		}
		
		/// <summary> Draws the bottom face of the given cuboid region. </summary>
		public void Bottom(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			if (Tinted) col = TintBlock(col);
			
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z2, u2, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z2, u1, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, z1, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y1, z1, u2, v1, col);
		}

		/// <summary> Draws the top face of the given cuboid region. </summary>
		public void Top(int count, int col, int texId, VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			if (Tinted) col = TintBlock(col);
			
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z1, u2, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z1, u1, v1, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, z2, u1, v2, col);
			vertices[index++] = new VertexP3fT2fC4b(x2 + (count - 1), y2, z2, u2, v2, col);
		}

		int TintBlock(int col) {
			FastColour rgbCol = FastColour.Unpack(col);
			rgbCol *= TintColour;
			return rgbCol.Pack();
		}
	}
}