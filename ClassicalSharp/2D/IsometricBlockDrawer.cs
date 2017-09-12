// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Textures;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp {

	public sealed class IsometricBlockDrawer {
		
		Game game;
		TerrainAtlas1D atlas;
		int index;
		float scale;
		VertexP3fT2fC4b[] vertices;
		int vb;
		
		CuboidDrawer drawer = new CuboidDrawer();
		
		public void BeginBatch(Game game, VertexP3fT2fC4b[] vertices, int vb) {
			this.game = game;
			lastTexIndex = -1;
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
			Matrix4 rotY, rotX;
			Matrix4.RotateY(out rotY, 45 * Utils.Deg2Rad);
			Matrix4.RotateX(out rotX, -30f * Utils.Deg2Rad);
			Matrix4.Mult(out transform, ref rotY, ref rotX);
			
			cosX = (float)Math.Cos(30f * Utils.Deg2Rad);
			sinX = (float)Math.Sin(30f * Utils.Deg2Rad);
			cosY = (float)Math.Cos(-45f * Utils.Deg2Rad);
			sinY = (float)Math.Sin(-45f * Utils.Deg2Rad);
		}
		
		public void DrawBatch(BlockID block, float size, float x, float y) {
			atlas = game.TerrainAtlas1D;
			drawer.elementsPerAtlas1D = atlas.elementsPerAtlas1D;
			drawer.invVerElementSize = atlas.invElementSize;
			
			bool bright = BlockInfo.FullBright[block];
			if (BlockInfo.Draw[block] == DrawType.Gas) return;
			
			// isometric coords size: cosY * -scale - sinY * scale
			// we need to divide by (2 * cosY), as the calling function expects size to be in pixels.
			scale = size / (2 * cosY);
			// screen to isometric coords (cos(-x) = cos(x), sin(-x) = -sin(x))
			pos.X = x; pos.Y = y; pos.Z = 0;
			RotateX(cosX, -sinX);
			RotateY(cosY, -sinY);
			
			// See comment in IGraphicsApi.Draw2DTexture()
			pos.X -= 0.5f; pos.Y -= 0.5f;
			
			if (BlockInfo.Draw[block] == DrawType.Sprite) {
				SpriteXQuad(block, true);
				SpriteZQuad(block, true);
				
				SpriteZQuad(block, false);
				SpriteXQuad(block, false);
			} else {
				drawer.minBB = BlockInfo.MinBB[block]; drawer.minBB.Y = 1 - drawer.minBB.Y;
				drawer.maxBB = BlockInfo.MaxBB[block]; drawer.maxBB.Y = 1 - drawer.maxBB.Y;
				
				Vector3 min = BlockInfo.MinBB[block], max = BlockInfo.MaxBB[block];
				drawer.x1 = scale * (1 - min.X * 2) + pos.X; drawer.x2 = scale * (1 - max.X * 2) + pos.X;
				drawer.y1 = scale * (1 - min.Y * 2) + pos.Y; drawer.y2 = scale * (1 - max.Y * 2) + pos.Y;
				drawer.z1 = scale * (1 - min.Z * 2) + pos.Z; drawer.z2 = scale * (1 - max.Z * 2) + pos.Z;
				
				drawer.Tinted = BlockInfo.Tinted[block];
				drawer.TintColour = BlockInfo.FogColour[block];
				
				drawer.Right(1, bright ? colNormal : colXSide, GetTex(block, Side.Right), vertices, ref index);
				drawer.Front(1, bright ? colNormal : colZSide, GetTex(block, Side.Front), vertices, ref index);
				drawer.Top(1,  colNormal                     , GetTex(block, Side.Top),   vertices, ref index);
			}
		}
		
		public void EndBatch() {
			if (index > 0) { lastTexIndex = texIndex; Flush(); }
			lastTexIndex = -1;
			game.Graphics.PopMatrix();
		}
		
		int GetTex(BlockID block, int side) {
			int texLoc = BlockInfo.GetTextureLoc(block, side);
			texIndex = texLoc / atlas.elementsPerAtlas1D;
			
			if (lastTexIndex != texIndex) Flush();
			return texLoc;
		}

		static Vector3 pos = Vector3.Zero;
		void SpriteZQuad(BlockID block, bool firstPart) {
			int texLoc = BlockInfo.GetTextureLoc(block, Side.Right);
			TextureRec rec = atlas.GetTexRec(texLoc, 1, out texIndex);
			if (lastTexIndex != texIndex) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			v.Colour = colNormal;
			
			if (BlockInfo.Tinted[block]) {
				v.Colour = Utils.Tint(v.Colour, BlockInfo.FogColour[block]);
			}
			
			float x1 = firstPart ? 0.5f : -0.1f, x2 = firstPart ? 1.1f : 0.5f;
			rec.U1 = firstPart ? 0.0f : 0.5f; rec.U2 = (firstPart ? 0.5f : 1.0f) * (15.99f/16f);
			float minX = scale * (1 - x1 * 2) + pos.X, maxX = scale * (1 - x2 * 2)   + pos.X;
			float minY = scale * (1 - 0 * 2)  + pos.Y, maxY = scale * (1 - 1.1f * 2) + pos.Y;
			
			v.Z = pos.Z;
			v.X = minX; v.Y = minY; v.U = rec.U2; v.V = rec.V2; vertices[index++] = v;
			            v.Y = maxY;               v.V = rec.V1; vertices[index++] = v;
			v.X = maxX;             v.U = rec.U1;               vertices[index++] = v;
			            v.Y = minY;               v.V = rec.V2; vertices[index++] = v;
		}

		void SpriteXQuad(BlockID block, bool firstPart) {
			int texLoc = BlockInfo.GetTextureLoc(block, Side.Right);
			TextureRec rec = atlas.GetTexRec(texLoc, 1, out texIndex);
			if (lastTexIndex != texIndex) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			v.Colour = colNormal;
			
			if (BlockInfo.Tinted[block]) {
				v.Colour = Utils.Tint(v.Colour, BlockInfo.FogColour[block]);
			}
			
			float z1 = firstPart ? 0.5f : -0.1f, z2 = firstPart ? 1.1f : 0.5f;
			rec.U1 = firstPart ? 0.0f : 0.5f; rec.U2 = (firstPart ? 0.5f : 1.0f) * (15.99f/16f);
			float minY = scale * (1 - 0 * 2)  + pos.Y, maxY = scale * (1 - 1.1f * 2) + pos.Y;
			float minZ = scale * (1 - z1 * 2) + pos.Z, maxZ = scale * (1 - z2 * 2)   + pos.Z;
			
			v.X = pos.X; 
			v.Y = minY; v.Z = minZ; v.U = rec.U2; v.V = rec.V2; vertices[index++] = v;
			v.Y = maxY;                           v.V = rec.V1; vertices[index++] = v;
			            v.Z = maxZ; v.U = rec.U1;               vertices[index++] = v;
			v.Y = minY;                           v.V = rec.V2; vertices[index++] = v;
		}
		
		int lastTexIndex, texIndex;
		void Flush() {
			if (lastTexIndex != -1) {
				game.Graphics.BindTexture(atlas.TexIds[lastTexIndex]);
				game.Graphics.UpdateDynamicVb_IndexedTris(vb, vertices, index);
			}
			
			lastTexIndex = texIndex;
			index = 0;			
		}
		
		/// <summary> Rotates the given 3D coordinates around the x axis. </summary>
		static void RotateX(float cosA, float sinA) {
			float y = cosA * pos.Y + sinA * pos.Z;
			pos.Z = -sinA * pos.Y + cosA * pos.Z;
			pos.Y = y;
		}
		
		/// <summary> Rotates the given 3D coordinates around the y axis. </summary>
		static void RotateY(float cosA, float sinA) {
			float x = cosA * pos.X - sinA * pos.Z;
			pos.Z = sinA * pos.X + cosA * pos.Z;
			pos.X = x;
		}
	}
}