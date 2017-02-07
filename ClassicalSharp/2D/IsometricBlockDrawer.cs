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
		
		NormalBlockBuilder drawer = new NormalBlockBuilder();
		
		public void BeginBatch(Game game, VertexP3fT2fC4b[] vertices, int vb) {
			this.game = game;
			lastIndex = -1;
			index = 0;
			this.vertices = vertices;
			this.vb = vb;
		}
		
		static int colNormal = FastColour.WhitePacked, colXSide, colZSide, colYBottom;
		static float cosX, sinX, cosY, sinY;
		static IsometricBlockDrawer() {
			FastColour.GetShaded(FastColour.White, out colXSide, out colZSide, out colYBottom);
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
			drawer.info = game.BlockInfo;
			
			bright = info.FullBright[block];
			if (info.Draw[block] == DrawType.Gas) return;
			
			// isometric coords size: cosY * -scale - sinY * scale
			// we need to divide by (2 * cosY), as the calling function expects size to be in pixels.
			scale = size / (2 * cosY);
			// screen to isometric coords (cos(-x) = cos(x), sin(-x) = -sin(x))
			pos.X = x; pos.Y = y; pos.Z = 0;
			Utils.RotateX(ref pos.Y, ref pos.Z, cosX, -sinX);
			Utils.RotateY(ref pos.X, ref pos.Z, cosY, -sinY);
			
			if (info.Draw[block] == DrawType.Sprite) {
				SpriteXQuad(block, true);
				SpriteZQuad(block, true);
				
				SpriteZQuad(block, false);
				SpriteXQuad(block, false);
			} else {
				drawer.minBB = info.MinBB[block];    drawer.maxBB = info.MaxBB[block];
				drawer.minBB.Y = 1 - drawer.minBB.Y; drawer.maxBB.Y = 1 - drawer.maxBB.Y;
				
				Vector3 min = game.BlockInfo.MinBB[block], max = game.BlockInfo.MaxBB[block];
				drawer.x1 = scale * (1 - min.X * 2); drawer.x2 = scale * (1 - max.X * 2);
				drawer.y1 = scale * (1 - min.Y * 2); drawer.y2 = scale * (1 - max.Y * 2);
				drawer.z1 = scale * (1 - min.Z * 2); drawer.z2 = scale * (1 - max.Z * 2);
				
				drawer.DrawRightFace(1, bright ? colNormal : colXSide, GetTex(block, Side.Right), vertices, ref index); TransformLast4();
				drawer.DrawFrontFace(1, bright ? colNormal : colZSide, GetTex(block, Side.Front), vertices, ref index); TransformLast4();
				drawer.DrawTopFace(1,  colNormal                     , GetTex(block, Side.Top),   vertices, ref index); TransformLast4();
			}
		}
		
		public void EndBatch() {
			if (index == 0) return;
			if (texIndex != lastIndex) game.Graphics.BindTexture(atlas.TexIds[texIndex]);
			
			game.Graphics.UpdateDynamicIndexedVb(DrawMode.Triangles, vb, vertices, index);
			index = 0;
			lastIndex = -1;
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
			float minX = scale * (1 - x1 * 2), maxX = scale * (1 - x2 * 2);
			float minY = scale * (1 - 0 * 2), maxY = scale * (1 - 1.1f * 2);
			
			v.X = minX; v.Y = minY; v.Z = 0; v.U = rec.U2; v.V = rec.V2; vertices[index++] = v;
			v.X = minX; v.Y = maxY; v.Z = 0; v.U = rec.U2; v.V = rec.V1; vertices[index++] = v;
			v.X = maxX; v.Y = maxY; v.Z = 0; v.U = rec.U1; v.V = rec.V1; vertices[index++] = v;
			v.X = maxX; v.Y = minY; v.Z = 0; v.U = rec.U1; v.V = rec.V2; vertices[index++] = v;
			TransformLast4();
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
			float minY = scale * (1 - 0 * 2), maxY = scale * (1 - 1.1f * 2);
			float minZ = scale * (1 - z1 * 2), maxZ = scale * (1 - z2 * 2);
			
			v.X = 0; v.Y = minY; v.Z = minZ; v.U = rec.U2; v.V = rec.V2; vertices[index++] = v;
			v.X = 0; v.Y = maxY; v.Z = minZ; v.U = rec.U2; v.V = rec.V1; vertices[index++] = v;
			v.X = 0; v.Y = maxY; v.Z = maxZ; v.U = rec.U1; v.V = rec.V1; vertices[index++] = v;
			v.X = 0; v.Y = minY; v.Z = maxZ; v.U = rec.U1; v.V = rec.V2; vertices[index++] = v;
			TransformLast4();
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
		
		void Transform(ref VertexP3fT2fC4b v) {
			v.X += pos.X; v.Y += pos.Y; v.Z += pos.Z;
			//Vector3 p = new Vector3(v.X, v.Y, v.Z) + pos;
			//p = Utils.RotateY(p - pos, time) + pos;
			//v coords = p
			
			// See comment in IGraphicsApi.Draw2DTexture()
			v.X -= 0.5f; v.Y -= 0.5f;
			float t = cosY * v.X - sinY * v.Z; v.Z = sinY * v.X + cosY * v.Z; v.X = t; // Inlined RotY
			t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t;      // Inlined RotX
		}
		
		void TransformLast4() {
			for (int i = index - 4; i < index; i++)
				Transform(ref vertices[i]);
		}
	}
}