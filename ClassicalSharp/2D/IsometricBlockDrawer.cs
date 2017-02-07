// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Textures;
using OpenTK;

namespace ClassicalSharp {

	public sealed class IsometricBlockDrawer {
		
		Game game;
		TerrainAtlas1D atlas;
		int index;
		float scale;
		const float invElemSize = TerrainAtlas2D.invElementSize;
		bool bright;
		VertexP3fT2fC4b[] vertices;
		int vb;
		
		CuboidDrawer drawer = new CuboidDrawer();
		
		public void BeginBatch(Game game, VertexP3fT2fC4b[] vertices, int vb) {
			this.game = game;
			lastIndex = -1;
			index = 0;
			this.vertices = vertices;
			this.vb = vb;
			
			game.Graphics.PushMatrix();
			Matrix4 m = transform;
			game.Graphics.MultiplyMatrix(ref m);
		}
		
		static int colNormal = FastColour.WhitePacked, colXSide, colZSide, colYBottom;
		static float cosX, sinX, cosY, sinY;
		static Matrix4 transform;
		
		static IsometricBlockDrawer() {
			FastColour.GetShaded(FastColour.White, out colXSide, out colZSide, out colYBottom);
			transform = Matrix4.RotateY(45 * Utils.Deg2Rad) * Matrix4.RotateX(-26.565f * Utils.Deg2Rad);
			
			cosX = (float)Math.Cos(26.565f * Utils.Deg2Rad);
			sinX = (float)Math.Sin(26.565f * Utils.Deg2Rad);
			cosY = (float)Math.Cos(-45f * Utils.Deg2Rad);
			sinY = (float)Math.Sin(-45f * Utils.Deg2Rad);
		}
		
		public void DrawBatch(byte block, float size, float x, float y) {
			BlockInfo info = game.BlockInfo;
			atlas = game.TerrainAtlas1D;
			drawer.elementsPerAtlas1D = atlas.elementsPerAtlas1D;
			drawer.invVerElementSize = atlas.invElementSize;
			
			bright = info.FullBright[block];
			if (info.Draw[block] == DrawType.Gas) return;
			
			// isometric coords size: cosY * -scale - sinY * scale
			// we need to divide by (2 * cosY), as the calling function expects size to be in pixels.
			scale = size / (2 * cosY);
			// screen to isometric coords (cos(-x) = cos(x), sin(-x) = -sin(x))
			pos.X = x; pos.Y = y; pos.Z = 0;
			Utils.RotateX(ref pos.Y, ref pos.Z, cosX, -sinX);
			Utils.RotateY(ref pos.X, ref pos.Z, cosY, -sinY);
			
			// See comment in IGraphicsApi.Draw2DTexture()
			pos.X -= 0.5f; pos.Y -= 0.5f;
			
			if (info.Draw[block] == DrawType.Sprite) {
				SpriteXQuad(block, true);
				SpriteZQuad(block, true);
				
				SpriteZQuad(block, false);
				SpriteXQuad(block, false);
			} else {
				drawer.minBB = info.MinBB[block]; drawer.minBB.Y = 1 - drawer.minBB.Y;    
				drawer.maxBB = info.MaxBB[block]; drawer.maxBB.Y = 1 - drawer.maxBB.Y;				
				
				Vector3 min = info.MinBB[block], max = info.MaxBB[block];
				drawer.x1 = scale * (1 - min.X * 2) + pos.X; drawer.x2 = scale * (1 - max.X * 2) + pos.X;
				drawer.y1 = scale * (1 - min.Y * 2) + pos.Y; drawer.y2 = scale * (1 - max.Y * 2) + pos.Y;
				drawer.z1 = scale * (1 - min.Z * 2) + pos.Z; drawer.z2 = scale * (1 - max.Z * 2) + pos.Z;
				
				drawer.Tinted = info.Tinted[block];
				drawer.TintColour = info.FogColour[block];
				
				drawer.Right(1, bright ? colNormal : colXSide, GetTex(block, Side.Right), vertices, ref index);
				drawer.Front(1, bright ? colNormal : colZSide, GetTex(block, Side.Front), vertices, ref index);
				drawer.Top(1,  colNormal                     , GetTex(block, Side.Top),   vertices, ref index);
			}
		}
		
		public void EndBatch() {
			if (index > 0) {
				if (texIndex != lastIndex) game.Graphics.BindTexture(atlas.TexIds[texIndex]);
				game.Graphics.UpdateDynamicIndexedVb(DrawMode.Triangles, vb, vertices, index);
				index = 0;
				lastIndex = -1;
			}
			game.Graphics.PopMatrix();
		}
		
		int GetTex(byte block, int side) {
			int texId = game.BlockInfo.GetTextureLoc(block, side);
			texIndex = texId / atlas.elementsPerAtlas1D;
			
			if (lastIndex != texIndex) Flush();
			return texId;
		}

		static Vector3 pos = Vector3.Zero;
		void SpriteZQuad(byte block, bool firstPart) {
			int texLoc = game.BlockInfo.GetTextureLoc(block, Side.Right);
			TextureRec rec = atlas.GetTexRec(texLoc, 1, out texIndex);
			if (lastIndex != texIndex) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			v.Colour = colNormal;
			
			if (game.BlockInfo.Tinted[block]) {
				FastColour fogCol = game.BlockInfo.FogColour[block];
				FastColour newCol = FastColour.Unpack(v.Colour);
				newCol *= fogCol;
				v.Colour = newCol.Pack();
			}
			
			float x1 = firstPart ? 0.5f : -0.1f, x2 = firstPart ? 1.1f : 0.5f;
			rec.U1 = firstPart ? 0.0f : 0.5f; rec.U2 = (firstPart ? 0.5f : 1.0f) * (15.99f/16f);
			float minX = scale * (1 - x1 * 2) + pos.X, maxX = scale * (1 - x2 * 2)   + pos.X;
			float minY = scale * (1 - 0 * 2)  + pos.Y, maxY = scale * (1 - 1.1f * 2) + pos.Y;
			
			v.X = minX; v.Y = minY; v.Z = pos.Z; v.U = rec.U2; v.V = rec.V2; vertices[index++] = v;
			v.X = minX; v.Y = maxY; v.Z = pos.Z; v.U = rec.U2; v.V = rec.V1; vertices[index++] = v;
			v.X = maxX; v.Y = maxY; v.Z = pos.Z; v.U = rec.U1; v.V = rec.V1; vertices[index++] = v;
			v.X = maxX; v.Y = minY; v.Z = pos.Z; v.U = rec.U1; v.V = rec.V2; vertices[index++] = v;
		}

		void SpriteXQuad(byte block, bool firstPart) {
			int texLoc = game.BlockInfo.GetTextureLoc(block, Side.Right);
			TextureRec rec = atlas.GetTexRec(texLoc, 1, out texIndex);
			if (lastIndex != texIndex) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			v.Colour = colNormal;
			
			if (game.BlockInfo.Tinted[block]) {
				FastColour fogCol = game.BlockInfo.FogColour[block];
				FastColour newCol = FastColour.Unpack(v.Colour);
				newCol *= fogCol;
				v.Colour = newCol.Pack();
			}
			
			float z1 = firstPart ? 0.5f : -0.1f, z2 = firstPart ? 1.1f : 0.5f;
			rec.U1 = firstPart ? 0.0f : 0.5f; rec.U2 = (firstPart ? 0.5f : 1.0f) * (15.99f/16f);
			float minY = scale * (1 - 0 * 2)  + pos.Y, maxY = scale * (1 - 1.1f * 2) + pos.Y;
			float minZ = scale * (1 - z1 * 2) + pos.Z, maxZ = scale * (1 - z2 * 2)   + pos.Z;
			
			v.X = pos.X; v.Y = minY; v.Z = minZ; v.U = rec.U2; v.V = rec.V2; vertices[index++] = v;
			v.X = pos.X; v.Y = maxY; v.Z = minZ; v.U = rec.U2; v.V = rec.V1; vertices[index++] = v;
			v.X = pos.X; v.Y = maxY; v.Z = maxZ; v.U = rec.U1; v.V = rec.V1; vertices[index++] = v;
			v.X = pos.X; v.Y = minY; v.Z = maxZ; v.U = rec.U1; v.V = rec.V2; vertices[index++] = v;
		}
		
		int lastIndex, texIndex;
		void Flush() {
			if (lastIndex != -1) {
				game.Graphics.UpdateDynamicIndexedVb(DrawMode.Triangles, vb, vertices, index);
				index = 0;
			}
			
			lastIndex = texIndex;
			game.Graphics.BindTexture(atlas.TexIds[texIndex]);
		}
	}
}