// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Renderers {
	
	public unsafe sealed class MapBordersRenderer : IGameComponent {
		
		World map;
		Game game;
		IGraphicsApi gfx;
		
		int sidesVb = -1, edgesVb = -1;
		int edgeTexId, sideTexId;
		int sidesVertices, edgesVertices;
		internal bool legacy;
		bool fullBrightSides, fullBrightEdge;
		
		public void UseLegacyMode(bool legacy) {
			this.legacy = legacy;
			ResetSidesAndEdges(null, null);
		}
		
		public void Init(Game game) {
			this.game = game;
			map = game.World;
			gfx = game.Graphics;
			
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
			game.Events.ViewDistanceChanged += ResetSidesAndEdges;
			game.Events.TerrainAtlasChanged += ResetTextures;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
		}
		
		public void RenderSides(double delta) {
			if (sidesVb == -1) return;
			BlockID block = game.World.Env.SidesBlock;
			
			gfx.SetupAlphaState(game.BlockInfo.Draw[block]);
			gfx.Texturing = true;
			
			gfx.BindTexture(sideTexId);
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.BindVb(sidesVb);
			gfx.DrawVb_IndexedTris(sidesVertices * 6 / 4);
			
			gfx.RestoreAlphaState(game.BlockInfo.Draw[block]);
			gfx.Texturing = false;
		}
		
		public void RenderEdges(double delta) {
			if (edgesVb == -1) return;
			BlockID block = game.World.Env.EdgeBlock;		
			
			Vector3 camPos = game.CurrentCameraPos;
			gfx.SetupAlphaState(game.BlockInfo.Draw[block]);
			gfx.Texturing = true;
			
			gfx.BindTexture(edgeTexId);
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.BindVb(edgesVb);
			// Do not draw water when we cannot see it.
			// Fixes some 'depth bleeding through' issues with 16 bit depth buffers on large maps.
			float yVisible = Math.Min(0, map.Env.SidesHeight);
			if (camPos.Y >= yVisible)
				gfx.DrawVb_IndexedTris(edgesVertices * 6 / 4);
			
			gfx.RestoreAlphaState(game.BlockInfo.Draw[block]);
			gfx.Texturing = false;
		}
		
		public void Dispose() {
			ContextLost();
			
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
			game.Events.ViewDistanceChanged -= ResetSidesAndEdges;
			game.Events.TerrainAtlasChanged -= ResetTextures;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}

		public void Ready(Game game) { }
		public void Reset(Game game) { OnNewMap(game); }
		
		public void OnNewMap(Game game) {
			gfx.DeleteVb(ref sidesVb);
			gfx.DeleteVb(ref edgesVb);
			MakeTexture(ref edgeTexId, ref lastEdgeTexLoc, map.Env.EdgeBlock);
			MakeTexture(ref sideTexId, ref lastSideTexLoc, map.Env.SidesBlock);
		}
		
		public void OnNewMapLoaded(Game game) { ResetSidesAndEdges(null, null); }
		
		void EnvVariableChanged(object sender, EnvVarEventArgs e) {
			if (e.Var == EnvVar.EdgeBlock) {
				MakeTexture(ref edgeTexId, ref lastEdgeTexLoc, map.Env.EdgeBlock);
				ResetEdges();
			} else if (e.Var == EnvVar.SidesBlock) {
				MakeTexture(ref sideTexId, ref lastSideTexLoc, map.Env.SidesBlock);
				ResetSides();
			} else if (e.Var == EnvVar.EdgeLevel || e.Var == EnvVar.SidesOffset) {
				ResetSidesAndEdges(null, null);
			} else if (e.Var == EnvVar.SunlightColour) {
				ResetEdges();
			} else if (e.Var == EnvVar.ShadowlightColour) {
				ResetSides();
			}
		}
		
		void ResetTextures(object sender, EventArgs e) {
			lastEdgeTexLoc = lastSideTexLoc = -1;
			MakeTexture(ref edgeTexId, ref lastEdgeTexLoc, map.Env.EdgeBlock);
			MakeTexture(ref sideTexId, ref lastSideTexLoc, map.Env.SidesBlock);
		}

		void ResetSidesAndEdges(object sender, EventArgs e) {
			CalculateRects((int)game.ViewDistance);
			ContextRecreated();
		}
		
		void ResetSides() {
			if (game.World.blocks == null || gfx.LostContext) return;
			gfx.DeleteVb(ref sidesVb);
			RebuildSides(map.Env.SidesHeight, legacy ? 128 : 65536);
		}
		
		void ResetEdges() {
			if (game.World.blocks == null || gfx.LostContext) return;
			gfx.DeleteVb(ref edgesVb);
			RebuildEdges(map.Env.EdgeHeight, legacy ? 128 : 65536);
		}
		
		void ContextLost() {
			gfx.DeleteVb(ref sidesVb);
			gfx.DeleteVb(ref edgesVb);
			gfx.DeleteTexture(ref edgeTexId);
			gfx.DeleteTexture(ref sideTexId);
		}
		
		void ContextRecreated() {
			ResetSides();
			ResetEdges();
			ResetTextures(null, null);
		}

		
		void RebuildSides(int y, int axisSize) {
			BlockID block = game.World.Env.SidesBlock;
			sidesVertices = 0;
			if (game.BlockInfo.Draw[block] == DrawType.Gas) return;
			
			for (int i = 0; i < rects.Length; i++) {
				Rectangle r = rects[i];
				sidesVertices += Utils.CountVertices(r.Width, r.Height, axisSize); // YQuads outside
			}
			sidesVertices += Utils.CountVertices(map.Width, map.Length, axisSize); // YQuads beneath map
			sidesVertices += 2 * Utils.CountVertices(map.Width, Math.Abs(y), axisSize); // ZQuads
			sidesVertices += 2 * Utils.CountVertices(map.Length, Math.Abs(y), axisSize); // XQuads
			
			VertexP3fT2fC4b[] v = new VertexP3fT2fC4b[sidesVertices];
			int index = 0;
			
			fullBrightSides = game.BlockInfo.FullBright[block];
			int col = fullBrightSides ? FastColour.WhitePacked : map.Env.Shadow;
			if (game.BlockInfo.Tinted[block]) {
				col = Utils.Tint(col, game.BlockInfo.FogColour[block]);
			}
			
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
				sidesVb = gfx.CreateVb((IntPtr)ptr, VertexFormat.P3fT2fC4b, sidesVertices);
			}
		}
		
		void RebuildEdges(int y, int axisSize) {
			BlockID block = game.World.Env.EdgeBlock;
			edgesVertices = 0;
			if (game.BlockInfo.Draw[block] == DrawType.Gas) return;
			
			for (int i = 0; i < rects.Length; i++) {
				Rectangle r = rects[i];
				edgesVertices += Utils.CountVertices(r.Width, r.Height, axisSize); // YPlanes outside
			}
			VertexP3fT2fC4b[] v = new VertexP3fT2fC4b[edgesVertices];
			int index = 0;
			
			fullBrightEdge = game.BlockInfo.FullBright[block];
			int col = fullBrightEdge ? FastColour.WhitePacked : map.Env.Sun;
			if (game.BlockInfo.Tinted[block]) {
				col = Utils.Tint(col, game.BlockInfo.FogColour[block]);
			}
			
			for (int i = 0; i < rects.Length; i++) {
				Rectangle r = rects[i];
				DrawY(r.X, r.Y, r.X + r.Width, r.Y + r.Height, y, axisSize, col,
				      HorOffset(block), YOffset(block), v, ref index);
			}

			fixed (VertexP3fT2fC4b* ptr = v) {
				edgesVb = gfx.CreateVb((IntPtr)ptr, VertexFormat.P3fT2fC4b, edgesVertices);
			}
		}

		float HorOffset(BlockID block) {
			BlockInfo info = game.BlockInfo;
			return info.RenderMinBB[block].X - info.MinBB[block].X;
		}
		
		float YOffset(BlockID block) {
			BlockInfo info = game.BlockInfo;
			return info.RenderMinBB[block].Y - info.MinBB[block].Y;
		}
		
		void DrawX(int x, int z1, int z2, int y1, int y2, int axisSize,
		           int col, VertexP3fT2fC4b[] v, ref int i) {
			int endZ = z2, endY = y2, startY = y1;
			for (; z1 < endZ; z1 += axisSize) {
				z2 = z1 + axisSize;
				if (z2 > endZ) z2 = endZ;
				y1 = startY;
				for (; y1 < endY; y1 += axisSize) {
					y2 = y1 + axisSize;
					if (y2 > endY) y2 = endY;
					
					TextureRec rec = new TextureRec(0, 0, z2 - z1, y2 - y1);
					v[i++] = new VertexP3fT2fC4b(x, y1, z1, rec.U1, rec.V2, col);
					v[i++] = new VertexP3fT2fC4b(x, y2, z1, rec.U1, rec.V1, col);
					v[i++] = new VertexP3fT2fC4b(x, y2, z2, rec.U2, rec.V1, col);
					v[i++] = new VertexP3fT2fC4b(x, y1, z2, rec.U2, rec.V2, col);
				}
			}
		}
		
		void DrawZ(int z, int x1, int x2, int y1, int y2, int axisSize,
		           int col, VertexP3fT2fC4b[] v, ref int i) {
			int endX = x2, endY = y2, startY = y1;
			for (; x1 < endX; x1 += axisSize) {
				x2 = x1 + axisSize;
				if (x2 > endX) x2 = endX;
				y1 = startY;
				for (; y1 < endY; y1 += axisSize) {
					y2 = y1 + axisSize;
					if (y2 > endY) y2 = endY;
					
					TextureRec rec = new TextureRec(0, 0, x2 - x1, y2 - y1);
					v[i++] = new VertexP3fT2fC4b(x1, y1, z, rec.U1, rec.V2, col);
					v[i++] = new VertexP3fT2fC4b(x1, y2, z, rec.U1, rec.V1, col);
					v[i++] = new VertexP3fT2fC4b(x2, y2, z, rec.U2, rec.V1, col);
					v[i++] = new VertexP3fT2fC4b(x2, y1, z, rec.U2, rec.V2, col);
				}
			}
		}
		
		void DrawY(int x1, int z1, int x2, int z2, float y, int axisSize,
		           int col, float offset, float yOffset, VertexP3fT2fC4b[] v, ref int i) {
			int endX = x2, endZ = z2, startZ = z1;
			for (; x1 < endX; x1 += axisSize) {
				x2 = x1 + axisSize;
				if (x2 > endX) x2 = endX;
				z1 = startZ;
				for (; z1 < endZ; z1 += axisSize) {
					z2 = z1 + axisSize;
					if (z2 > endZ) z2 = endZ;
					
					TextureRec rec = new TextureRec(0, 0, x2 - x1, z2 - z1);
					v[i++] = new VertexP3fT2fC4b(x1 + offset, y + yOffset, z1 + offset, rec.U1, rec.V1, col);
					v[i++] = new VertexP3fT2fC4b(x1 + offset, y + yOffset, z2 + offset, rec.U1, rec.V2, col);
					v[i++] = new VertexP3fT2fC4b(x2 + offset, y + yOffset, z2 + offset, rec.U2, rec.V2, col);
					v[i++] = new VertexP3fT2fC4b(x2 + offset, y + yOffset, z1 + offset, rec.U2, rec.V1, col);
				}
			}
		}
		
		Rectangle[] rects = new Rectangle[4];
		void CalculateRects(int extent) {
			extent = Utils.AdjViewDist(extent);
			rects[0] = new Rectangle(-extent, -extent, extent + map.Width + extent, extent);
			rects[1] = new Rectangle(-extent, map.Length, extent + map.Width + extent, extent);
			
			rects[2] = new Rectangle(-extent, 0, extent, map.Length);
			rects[3] = new Rectangle(map.Width, 0, extent, map.Length);
		}
		
		int lastEdgeTexLoc, lastSideTexLoc;
		void MakeTexture(ref int id, ref int lastTexLoc, BlockID block) {
			int texLoc = game.BlockInfo.GetTextureLoc(block, Side.Top);
			if (texLoc == lastTexLoc || gfx.LostContext) return;
			lastTexLoc = texLoc;
			
			game.Graphics.DeleteTexture(ref id);
			id = game.TerrainAtlas.LoadTextureElement(gfx, texLoc);
		}
	}
}