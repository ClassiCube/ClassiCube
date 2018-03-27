// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;
#if ANDROID
using Android.Graphics;
#endif
using BlockID = System.UInt16;

namespace ClassicalSharp.Entities {

	/// <summary> Entity component that draws square and circle shadows beneath entities. </summary>
	public unsafe static class ShadowComponent {
		
		static Game game;
		static Entity entity;
		static float radius = 7f;
		
		internal static void Draw(Game game, Entity entity) {
			ShadowComponent.game = game;
			ShadowComponent.entity = entity;
			
			Vector3 Position = entity.Position;
			if (Position.Y < 0) return;
			
			float posX = Position.X, posZ = Position.Z;
			int posY = Math.Min((int)Position.Y, game.World.MaxY);
			int index = 0, vb = 0;
			radius = 7f * Math.Min(entity.ModelScale.Y, 1) * entity.Model.ShadowScale;
			
			VertexP3fT2fC4b[] verts = null;
			int dataCount = 0;
			ShadowData* data = stackalloc ShadowData[4];
			
			if (game.Entities.ShadowMode == EntityShadow.SnapToBlock) {
				vb = game.Graphics.texVb; verts = game.Graphics.texVerts;
				int x1 = Utils.Floor(posX), z1 = Utils.Floor(posZ);
				
				if (!GetBlocks(x1, posY, z1, data, ref dataCount)) return;
				DrawSquareShadow(verts, ref index, data[0].Y, x1, z1);
			} else {
				vb = game.ModelCache.vb; verts = game.ModelCache.vertices;
				int x1 = Utils.Floor(posX - radius/16f), z1 = Utils.Floor(posZ - radius/16f);
				int x2 = Utils.Floor(posX + radius/16f), z2 = Utils.Floor(posZ + radius/16f);
				
				if (GetBlocks(x1, posY, z1, data, ref dataCount) && data[0].A > 0) {
					DrawCircle(verts, ref index, data, dataCount, x1, z1);
				}
				if (x1 != x2 && GetBlocks(x2, posY, z1, data, ref dataCount) && data[0].A > 0) {
					DrawCircle(verts, ref index, data, dataCount, x2, z1);
				}
				if (z1 != z2 && GetBlocks(x1, posY, z2, data, ref dataCount) && data[0].A > 0) {
					DrawCircle(verts, ref index, data, dataCount, x1, z2);
				}
				if (x1 != x2 && z1 != z2 && GetBlocks(x2, posY, z2, data, ref dataCount) && data[0].A > 0) {
					DrawCircle(verts, ref index, data, dataCount, x2, z2);
				}
			}
			
			if (index == 0) return;
			CheckShadowTexture(game.Graphics);
			
			if (!boundShadowTex) {
				game.Graphics.BindTexture(shadowTex);
				boundShadowTex = true;
			}
			game.Graphics.UpdateDynamicVb_IndexedTris(vb, verts, index);
		}
		
		const byte c = 255; // avoids 'ambiguous match' compile errors.
		static void DrawSquareShadow(VertexP3fT2fC4b[] verts, ref int index,
		                             float y, float x, float z) {
			int col = new FastColour(c, c, c, (byte)220).Pack();
			TextureRec rec = new TextureRec(63/128f, 63/128f, 1/128f, 1/128f);
			verts[index++] = new VertexP3fT2fC4b(x, y, z, rec.U1, rec.V1, col);
			verts[index++] = new VertexP3fT2fC4b(x + 1, y, z, rec.U2, rec.V1, col);
			verts[index++] = new VertexP3fT2fC4b(x + 1, y, z + 1, rec.U2, rec.V2, col);
			verts[index++] = new VertexP3fT2fC4b(x, y, z + 1, rec.U1, rec.V2, col);
		}
		
		static void DrawCircle(VertexP3fT2fC4b[] verts, ref int index,
		                       ShadowData* data, int dataCount, float x, float z) {
			Vector3 min = BlockInfo.MinBB[data[0].Block], max = BlockInfo.MaxBB[data[0].Block];
			
			DrawCoords(verts, ref index, data[0], x + min.X, z + min.Z, x + max.X, z + max.Z);
			for (int i = 1; i < dataCount; i++) {
				Vector3 nMin = BlockInfo.MinBB[data[i].Block], nMax = BlockInfo.MaxBB[data[i].Block];
				DrawCoords(verts, ref index, data[i], x + min.X, z + nMin.Z, x + max.X, z + min.Z);
				DrawCoords(verts, ref index, data[i], x + min.X, z + max.Z, x + max.X, z + nMax.Z);
				
				DrawCoords(verts, ref index, data[i], x + nMin.X, z + nMin.Z, x + min.X, z + nMax.Z);
				DrawCoords(verts, ref index, data[i], x + max.X, z + nMin.Z, x + nMax.X, z + nMax.Z);
				min = nMin; max = nMax;
			}
		}
		
		static void DrawCoords(VertexP3fT2fC4b[] verts, ref int index, ShadowData data,
		                       float x1, float z1, float x2, float z2) {
			Vector3 cen = entity.Position;
			
			if (lequal(x2, x1) || lequal(z2, z1)) return;
			float u1 = (x1 - cen.X) * 16/(radius * 2) + 0.5f;
			float v1 = (z1 - cen.Z) * 16/(radius * 2) + 0.5f;
			float u2 = (x2 - cen.X) * 16/(radius * 2) + 0.5f;
			float v2 = (z2 - cen.Z) * 16/(radius * 2) + 0.5f;
			if (u2 <= 0 || v2 <= 0 || u1 >= 1 || v1 >= 1) return;
			
			x1 = Math.Max(x1, cen.X - radius/16f); u1 = u1 >= 0 ? u1 : 0;
			z1 = Math.Max(z1, cen.Z - radius/16f); v1 = v1 >= 0 ? v1 : 0;
			x2 = Math.Min(x2, cen.X + radius/16f); u2 = u2 <= 1 ? u2 : 1;
			z2 = Math.Min(z2, cen.Z + radius/16f); v2 = v2 <= 1 ? v2 : 1;
			
			int col = new FastColour(c, c, c, data.A).Pack();
			VertexP3fT2fC4b v; v.Y = data.Y; v.Colour = col;
			v.X = x1; v.Z = z1; v.U = u1; v.V = v1; verts[index++] = v;
			v.X = x2;           v.U = u2;           verts[index++] = v;
			          v.Z = z2;           v.V = v2; verts[index++] = v;
			v.X = x1;           v.U = u1;           verts[index++] = v;
		}
		
		static bool GetBlocks(int x, int y, int z, ShadowData* data, ref int index) {
			float posY = entity.Position.Y;
			index = 0;
			bool outside = x < 0 || z < 0 || x >= game.World.Width || z >= game.World.Length;
			
			while (y >= 0 && index < 4) {
				BlockID block;
				if (!outside) {
					block = game.World.GetBlock(x, y, z);
				} else if (y == game.World.Env.EdgeHeight - 1) {
					block = BlockInfo.Draw[game.World.Env.EdgeBlock]  == DrawType.Gas ? Block.Air : Block.Bedrock;
				} else if (y == game.World.Env.SidesHeight - 1) {
					block = BlockInfo.Draw[game.World.Env.SidesBlock] == DrawType.Gas ? Block.Air : Block.Bedrock;
				} else {
					block = Block.Air;
				}
				y--;
				
				byte draw = BlockInfo.Draw[block];
				if (draw == DrawType.Gas || draw == DrawType.Sprite || BlockInfo.IsLiquid[block]) continue;
				float blockY = (y + 1) + BlockInfo.MaxBB[block].Y;
				if (blockY >= posY + 0.01f) continue;
				
				data[index].Block = block; data[index].Y = blockY;
				CalcAlpha(posY, ref data[index]);
				index++;
				
				// Check if the casted shadow will continue on further down.
				if (BlockInfo.MinBB[block].X == 0 && BlockInfo.MaxBB[block].X == 1 &&
				    BlockInfo.MinBB[block].Z == 0 && BlockInfo.MaxBB[block].Z == 1) return true;
			}
			
			if (index < 4) {
				data[index].Block = game.World.Env.EdgeBlock; data[index].Y = 0;
				CalcAlpha(posY, ref data[index]);
				index++;
			}
			return true;
		}
		
		struct ShadowData {
			public BlockID Block;
			public float Y;
			public byte A;
		}
		
		static void CalcAlpha(float playerY, ref ShadowData data) {
			float height = playerY - data.Y;
			if (height <= 6) {
				data.A = (byte)(160 - 160 * height / 6);
				data.Y += 1/64f; return;
			}
			
			data.A = 0;
			if (height <= 16) data.Y += 1/64f;
			else if (height <= 32) data.Y += 1/16f;
			else if (height <= 96) data.Y += 1/8f;
			else data.Y += 1/4f;
		}
		
		static bool lequal(float a, float b) {
			return a < b || Math.Abs(a - b) < 0.001f;
		}
		
		internal static bool boundShadowTex = false;
		internal static int shadowTex = -1;
		static void CheckShadowTexture(IGraphicsApi gfx) {
			if (shadowTex != -1) return;
			const int size = 128, half = size / 2;
			using (Bitmap bmp = Platform.CreateBmp(size, size))
				using (FastBitmap fastBmp = new FastBitmap(bmp, true, false))
			{
				int inPix = new FastColour(0, 0, 0, 200).ToArgb();
				int outPix = new FastColour(0, 0, 0, 0).ToArgb();
				for (int y = 0; y < fastBmp.Height; y++) {
					int* row = fastBmp.GetRowPtr(y);
					for (int x = 0; x < fastBmp.Width; x++) {
						double dist = (half - (x + 0.5)) * (half - (x + 0.5)) +
							(half - (y + 0.5)) * (half - (y + 0.5));
						row[x] = dist < half * half ? inPix : outPix;
					}
				}
				shadowTex = gfx.CreateTexture(fastBmp, false, false);
			}
		}
	}
}