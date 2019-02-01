// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using ClassicalSharp.Textures;
using OpenTK;
using BlockID = System.UInt16;

namespace ClassicalSharp.Renderers {
	
	public unsafe sealed class MapBordersRenderer : IGameComponent {
		
		World map;
		Game game;
		
		int sidesVb, edgesVb;
		int edgesTex, sidesTex;
		int sidesVertices, edgesVertices;
		internal bool legacy;
		bool fullBrightSides, fullBrightEdge;
		
		public void UseLegacyMode(bool legacy) {
			this.legacy = legacy;
			ResetSidesAndEdges();
		}
		
		void IGameComponent.Init(Game game) {
			this.game = game;
			map = game.World;
			
			Events.EnvVariableChanged  += EnvVariableChanged;
			Events.ViewDistanceChanged += ResetSidesAndEdges;
			Events.TerrainAtlasChanged += ResetTextures;
			Events.ContextLost      += ContextLost;
			Events.ContextRecreated += ContextRecreated;
		}
		
		void RenderBorders(BlockID block, int vb, int tex, int count) {
			if (vb == 0) return;
			IGraphicsApi gfx = game.Graphics;

			gfx.Texturing = true;
			gfx.SetupAlphaState(BlockInfo.Draw[block]);
			gfx.EnableMipmaps();

			gfx.BindTexture(tex);
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.BindVb(vb);
			if (count > 0) gfx.DrawVb_IndexedTris(count);

			gfx.DisableMipmaps();
			gfx.RestoreAlphaState(BlockInfo.Draw[block]);
			gfx.Texturing = false;
		}
		
		public void RenderSides(double delta) {
			RenderBorders(game.World.Env.SidesBlock, sidesVb,
			              sidesTex, sidesVertices);
		}
		
		public void RenderEdges(double delta) {
			// Do not draw water when player cannot see it
			// Fixes some 'depth bleeding through' issues with 16 bit depth buffers on large maps
			Vector3 camPos = game.CurrentCameraPos;
			int vertices = 0, yVisible = Math.Min(0, map.Env.SidesHeight);
			if (camPos.Y >= yVisible) vertices = edgesVertices;
			
			RenderBorders(game.World.Env.EdgeBlock, edgesVb,
			              edgesTex, vertices);
		}
		
		void IDisposable.Dispose() {
			ContextLost();
			
			Events.EnvVariableChanged  -= EnvVariableChanged;
			Events.ViewDistanceChanged -= ResetSidesAndEdges;
			Events.TerrainAtlasChanged -= ResetTextures;
			Events.ContextLost      -= ContextLost;
			Events.ContextRecreated -= ContextRecreated;
		}

		void IGameComponent.Ready(Game game) { }
		void IGameComponent.Reset(Game game) { OnNewMap(game); }
		
		public void OnNewMap(Game game) {
			game.Graphics.DeleteVb(ref sidesVb);
			game.Graphics.DeleteVb(ref edgesVb);
			MakeTexture(ref edgesTex, ref lastEdgeTexLoc, map.Env.EdgeBlock);
			MakeTexture(ref sidesTex, ref lastSideTexLoc, map.Env.SidesBlock);
		}
		
		void IGameComponent.OnNewMapLoaded(Game game) { ResetSidesAndEdges(); }
		
		void EnvVariableChanged(EnvVar envVar) {
			if (envVar == EnvVar.EdgeBlock) {
				MakeTexture(ref edgesTex, ref lastEdgeTexLoc, map.Env.EdgeBlock);
				ResetEdges();
			} else if (envVar == EnvVar.SidesBlock) {
				MakeTexture(ref sidesTex, ref lastSideTexLoc, map.Env.SidesBlock);
				ResetSides();
			} else if (envVar == EnvVar.EdgeLevel || envVar == EnvVar.SidesOffset) {
				ResetSidesAndEdges();
			} else if (envVar == EnvVar.SunCol) {
				ResetEdges();
			} else if (envVar == EnvVar.ShadowCol) {
				ResetSides();
			}
		}
		
		void ResetTextures() {
			lastEdgeTexLoc = -1; lastSideTexLoc = -1;
			MakeTexture(ref edgesTex, ref lastEdgeTexLoc, map.Env.EdgeBlock);
			MakeTexture(ref sidesTex, ref lastSideTexLoc, map.Env.SidesBlock);
		}

		void ResetSidesAndEdges() {
			CalculateRects();
			ContextRecreated();
		}
		
		void ResetSides() {
			if (!game.World.HasBlocks || game.Graphics.LostContext) return;
			game.Graphics.DeleteVb(ref sidesVb);
			RebuildSides(map.Env.SidesHeight, legacy ? 128 : 65536);
		}
		
		void ResetEdges() {
			if (!game.World.HasBlocks || game.Graphics.LostContext) return;
			game.Graphics.DeleteVb(ref edgesVb);
			RebuildEdges(map.Env.EdgeHeight, legacy ? 128 : 65536);
		}
		
		void ContextLost() {
			game.Graphics.DeleteVb(ref sidesVb);
			game.Graphics.DeleteVb(ref edgesVb);
			game.Graphics.DeleteTexture(ref edgesTex);
			game.Graphics.DeleteTexture(ref sidesTex);
		}
		
		void ContextRecreated() {
			ResetSides();
			ResetEdges();
			ResetTextures();
		}

		
		void RebuildSides(int y, int axisSize) {
			BlockID block = game.World.Env.SidesBlock;
			sidesVertices = 0;
			if (BlockInfo.Draw[block] == DrawType.Gas) return;
			
			for (int i = 0; i < rects.Length; i++) {
				Rectangle r = rects[i];
				sidesVertices += Utils.CountVertices(r.Width, r.Height, axisSize); // YQuads outside
			}
			sidesVertices += Utils.CountVertices(map.Width, map.Length, axisSize); // YQuads beneath map
			sidesVertices += 2 * Utils.CountVertices(map.Width, Math.Abs(y), axisSize); // ZQuads
			sidesVertices += 2 * Utils.CountVertices(map.Length, Math.Abs(y), axisSize); // XQuads
			
			VertexP3fT2fC4b[] v = new VertexP3fT2fC4b[sidesVertices];
			int index = 0;
			
			fullBrightSides = BlockInfo.FullBright[block];
			PackedCol col = fullBrightSides ? PackedCol.White : map.Env.Shadow;
			if (BlockInfo.Tinted[block]) { col *= BlockInfo.FogCol[block]; }
			
			for (int i = 0; i < rects.Length; i++) {
				Rectangle r = rects[i];
				DrawY(r.X, r.Y, r.X + r.Width, r.Y + r.Height, y, axisSize, col, 0, YOffset(block), v, ref index);
			}
			
			// Work properly for when ground level is below 0
			int y1 = 0, y2 = y;
			if (y < 0) { y1 = y; y2 = 0; }
			DrawY(0, 0, map.Width, map.Length, 0, axisSize, col, 0, 0, v, ref index);
			DrawZ(0, 0, map.Width, y1, y2, axisSize, col, v, ref index);
			DrawZ(map.Length, 0, map.Width, y1, y2, axisSize, col, v, ref index);
			DrawX(0, 0, map.Length, y1, y2, axisSize, col, v, ref index);
			DrawX(map.Width, 0, map.Length, y1, y2, axisSize, col, v, ref index);
			
			fixed (VertexP3fT2fC4b* ptr = v) {
				sidesVb = game.Graphics.CreateVb((IntPtr)ptr, VertexFormat.P3fT2fC4b, sidesVertices);
			}
		}
		
		void RebuildEdges(int y, int axisSize) {
			BlockID block = game.World.Env.EdgeBlock;
			edgesVertices = 0;
			if (BlockInfo.Draw[block] == DrawType.Gas) return;
			
			for (int i = 0; i < rects.Length; i++) {
				Rectangle r = rects[i];
				edgesVertices += Utils.CountVertices(r.Width, r.Height, axisSize); // YPlanes outside
			}
			VertexP3fT2fC4b[] v = new VertexP3fT2fC4b[edgesVertices];
			int index = 0;
			
			fullBrightEdge = BlockInfo.FullBright[block];
			PackedCol col = fullBrightEdge ? PackedCol.White : map.Env.Sun;
			if (BlockInfo.Tinted[block]) { col *= BlockInfo.FogCol[block]; }
			
			for (int i = 0; i < rects.Length; i++) {
				Rectangle r = rects[i];
				DrawY(r.X, r.Y, r.X + r.Width, r.Y + r.Height, y, axisSize, col,
				      HorOffset(block), YOffset(block), v, ref index);
			}

			fixed (VertexP3fT2fC4b* ptr = v) {
				edgesVb = game.Graphics.CreateVb((IntPtr)ptr, VertexFormat.P3fT2fC4b, edgesVertices);
			}
		}

		float HorOffset(BlockID block) {
			return BlockInfo.RenderMinBB[block].X - BlockInfo.MinBB[block].X;
		}
		
		float YOffset(BlockID block) {
			return BlockInfo.RenderMinBB[block].Y - BlockInfo.MinBB[block].Y;
		}
		
		void DrawX(int x, int z1, int z2, int y1, int y2, int axisSize,
		           PackedCol col, VertexP3fT2fC4b[] vertices, ref int i) {
			int endZ = z2, endY = y2, startY = y1;
			VertexP3fT2fC4b v;
			v.X = x; v.Col = col;
			
			for (; z1 < endZ; z1 += axisSize) {
				z2 = z1 + axisSize;
				if (z2 > endZ) z2 = endZ;
				y1 = startY;
				for (; y1 < endY; y1 += axisSize) {
					y2 = y1 + axisSize;
					if (y2 > endY) y2 = endY;
					
					float u2 = z2 - z1, v2 = y2 - y1;
					v.Y = y1; v.Z = z1; v.U = 0f; v.V = v2; vertices[i++] = v;
					v.Y = y2;                     v.V = 0f; vertices[i++] = v;
					v.Z = z2; v.U = u2;           vertices[i++] = v;
					v.Y = y1;                     v.V = v2; vertices[i++] = v;
				}
			}
		}
		
		void DrawZ(int z, int x1, int x2, int y1, int y2, int axisSize,
		           PackedCol col, VertexP3fT2fC4b[] vertices, ref int i) {
			int endX = x2, endY = y2, startY = y1;
			VertexP3fT2fC4b v;
			v.Z = z; v.Col = col;
			
			for (; x1 < endX; x1 += axisSize) {
				x2 = x1 + axisSize;
				if (x2 > endX) x2 = endX;
				y1 = startY;
				for (; y1 < endY; y1 += axisSize) {
					y2 = y1 + axisSize;
					if (y2 > endY) y2 = endY;
					
					float u2 = x2 - x1, v2 = y2 - y1;
					v.X = x1; v.Y = y1; v.U = 0f; v.V = v2; vertices[i++] = v;
					v.Y = y2;           v.V = 0f; vertices[i++] = v;
					v.X = x2;           v.U = u2;           vertices[i++] = v;
					v.Y = y1;           v.V = v2; vertices[i++] = v;
				}
			}
		}
		
		void DrawY(int x1, int z1, int x2, int z2, float y, int axisSize,
		           PackedCol col, float offset, float yOffset, VertexP3fT2fC4b[] vertices, ref int i) {
			int endX = x2, endZ = z2, startZ = z1;
			VertexP3fT2fC4b v;
			v.Y = y + yOffset; v.Col = col;
			
			for (; x1 < endX; x1 += axisSize) {
				x2 = x1 + axisSize;
				if (x2 > endX) x2 = endX;
				z1 = startZ;
				for (; z1 < endZ; z1 += axisSize) {
					z2 = z1 + axisSize;
					if (z2 > endZ) z2 = endZ;
					
					float u2 = x2 - x1, v2 = z2 - z1;
					v.X = x1 + offset; v.Z = z1 + offset; v.U = 0f; v.V = 0f; vertices[i++] = v;
					v.Z = z2 + offset;           v.V = v2; vertices[i++] = v;
					v.X = x2 + offset;                    v.U = u2;           vertices[i++] = v;
					v.Z = z1 + offset;           v.V = 0f; vertices[i++] = v;
				}
			}
		}
		
		Rectangle[] rects = new Rectangle[4];
		void CalculateRects() {
			int extent = Utils.AdjViewDist(game.ViewDistance);
			rects[0] = new Rectangle(-extent, -extent, extent + map.Width + extent, extent);
			rects[1] = new Rectangle(-extent, map.Length, extent + map.Width + extent, extent);
			
			rects[2] = new Rectangle(-extent, 0, extent, map.Length);
			rects[3] = new Rectangle(map.Width, 0, extent, map.Length);
		}
		
		int lastEdgeTexLoc, lastSideTexLoc;
		void MakeTexture(ref int id, ref int lastTexLoc, BlockID block) {
			int texLoc = BlockInfo.GetTextureLoc(block, Side.Top);
			if (texLoc == lastTexLoc || game.Graphics.LostContext) return;
			lastTexLoc = texLoc;
			
			game.Graphics.DeleteTexture(ref id);
			id = Atlas2D.LoadTile(texLoc);
		}
	}
}