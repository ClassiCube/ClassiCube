// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Textures;
using OpenTK;

namespace ClassicalSharp {

	/// <summary> Draws the vertices for a cuboid region. </summary>
	public sealed class CuboidDrawer {
		/// <summary> Whether a colour tinting effect should be applied to all faces. </summary>
		public bool Tinted;
		
		/// <summary> The tint colour to multiply colour of faces by. </summary>
		public PackedCol TintCol;
		
		public Vector3 minBB, maxBB;
		public float x1, y1, z1, x2, y2, z2;
		const float uv2Scale = 15.99f/16f;
		
		
		public void Left(int count, PackedCol col, int texLoc, 
		                 VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texLoc % Atlas1D.TilesPerAtlas) * Atlas1D.invTileSize;
			float u1 = minBB.Z, u2 = (count - 1) + maxBB.Z * uv2Scale;
			float v1 = vOrigin + maxBB.Y * Atlas1D.invTileSize;
			float v2 = vOrigin + minBB.Y * Atlas1D.invTileSize * uv2Scale;
			if (Tinted) { col *= TintCol; }
			
			VertexP3fT2fC4b v; v.X = x1; v.Col = col;
			v.Y = y2; v.Z = z2 + (count - 1); v.U = u2; v.V = v1; vertices[index++] = v; 
			          v.Z = z1;               v.U = u1;           vertices[index++] = v;
			v.Y = y1;                                   v.V = v2; vertices[index++] = v;
			          v.Z = z2 + (count - 1); v.U = u2;           vertices[index++] = v;
		}

		public void Right(int count, PackedCol col, int texLoc, 
		                  VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texLoc % Atlas1D.TilesPerAtlas) * Atlas1D.invTileSize;
			float u1 = (count - minBB.Z), u2 = (1 - maxBB.Z) * uv2Scale;
			float v1 = vOrigin + maxBB.Y * Atlas1D.invTileSize;
			float v2 = vOrigin + minBB.Y * Atlas1D.invTileSize * uv2Scale;
			if (Tinted) { col *= TintCol; }
			
			VertexP3fT2fC4b v; v.X = x2; v.Col = col;
			v.Y = y2; v.Z = z1;               v.U = u1; v.V = v1; vertices[index++] = v;
			          v.Z = z2 + (count - 1); v.U = u2;           vertices[index++] = v;
			v.Y = y1;                                   v.V = v2; vertices[index++] = v;
			          v.Z = z1;               v.U = u1;           vertices[index++] = v;
		}

		public void Front(int count, PackedCol col, int texLoc, 
		                  VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texLoc % Atlas1D.TilesPerAtlas) * Atlas1D.invTileSize;
			float u1 = (count - minBB.X), u2 = (1 - maxBB.X) * uv2Scale;
			float v1 = vOrigin + maxBB.Y * Atlas1D.invTileSize;
			float v2 = vOrigin + minBB.Y * Atlas1D.invTileSize * uv2Scale;
			if (Tinted) { col *= TintCol; }
			
			VertexP3fT2fC4b v; v.Z = z1; v.Col = col;
			v.X = x2 + (count - 1); v.Y = y1; v.U = u2; v.V = v2; vertices[index++] = v;
			v.X = x1;                         v.U = u1;           vertices[index++] = v;
			                        v.Y = y2;           v.V = v1; vertices[index++] = v;
			v.X = x2 + (count - 1);           v.U = u2;           vertices[index++] = v;
		}
		
		public void Back(int count, PackedCol col, int texLoc, 
		                 VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texLoc % Atlas1D.TilesPerAtlas) * Atlas1D.invTileSize;
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * uv2Scale;
			float v1 = vOrigin + maxBB.Y * Atlas1D.invTileSize;
			float v2 = vOrigin + minBB.Y * Atlas1D.invTileSize * uv2Scale;			
			if (Tinted) { col *= TintCol; }
			
			VertexP3fT2fC4b v; v.Z = z2; v.Col = col;
			v.X = x2 + (count - 1); v.Y = y2; v.U = u2; v.V = v1; vertices[index++] = v;
			v.X = x1;                         v.U = u1;           vertices[index++] = v;
			                        v.Y = y1;           v.V = v2; vertices[index++] = v;
			v.X = x2 + (count - 1);           v.U = u2;           vertices[index++] = v;
		}
		
		public void Bottom(int count, PackedCol col, int texLoc, 
		                   VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texLoc % Atlas1D.TilesPerAtlas) * Atlas1D.invTileSize;
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * uv2Scale;
			float v1 = vOrigin + minBB.Z * Atlas1D.invTileSize;
			float v2 = vOrigin + maxBB.Z * Atlas1D.invTileSize * uv2Scale;
			if (Tinted) { col *= TintCol; }
			
			VertexP3fT2fC4b v; v.Y = y1; v.Col = col;
			v.X = x2 + (count - 1); v.Z = z2; v.U = u2; v.V = v2; vertices[index++] = v;
			v.X = x1;                         v.U = u1;           vertices[index++] = v;
			                        v.Z = z1;           v.V = v1; vertices[index++] = v;
			v.X = x2 + (count - 1);           v.U = u2;           vertices[index++] = v;
		}

		public void Top(int count, PackedCol col, int texLoc, 
		                VertexP3fT2fC4b[] vertices, ref int index) {
			float vOrigin = (texLoc % Atlas1D.TilesPerAtlas) * Atlas1D.invTileSize;
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * uv2Scale;
			float v1 = vOrigin + minBB.Z * Atlas1D.invTileSize;
			float v2 = vOrigin + maxBB.Z * Atlas1D.invTileSize * uv2Scale;
			if (Tinted) { col *= TintCol; }
			
			VertexP3fT2fC4b v; v.Y = y2; v.Col = col;
			v.X = x2 + (count - 1); v.Z = z1; v.U = u2; v.V = v1; vertices[index++] = v;
			v.X = x1;                         v.U = u1;           vertices[index++] = v;
			                        v.Z = z2;           v.V = v2; vertices[index++] = v;
			v.X = x2 + (count - 1);           v.U = u2;           vertices[index++] = v;
		}
	}
}