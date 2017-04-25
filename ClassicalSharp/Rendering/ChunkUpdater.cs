﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
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
	
	/// <summary> Manages the process of building/deleting chunk meshes,
	/// in addition to calculating the visibility of chunks. </summary>
	public sealed class ChunkUpdater : IDisposable {
		
		Game game;
		internal ChunkMeshBuilder builder;
		BlockInfo info;
		
		int width, height, length;
		internal int[] distances;
		internal Vector3I chunkPos = new Vector3I(int.MaxValue);
		int elementsPerBitmap = 0;
		MapRenderer renderer;
		
		public ChunkUpdater(Game game, MapRenderer renderer) {
			this.game = game;
			this.renderer = renderer;
			info = game.BlockInfo;
			
			game.Events.TerrainAtlasChanged += TerrainAtlasChanged;
			game.WorldEvents.OnNewMap += OnNewMap;
			game.WorldEvents.OnNewMapLoaded += OnNewMapLoaded;
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
			
			game.Events.BlockDefinitionChanged += BlockDefinitionChanged;
			game.Events.ViewDistanceChanged += ViewDistanceChanged;
			game.Events.ProjectionChanged += ProjectionChanged;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
		}
		
		public void Dispose() {
			ClearChunkCache();
			renderer.chunks = null;
			renderer.unsortedChunks = null;
			game.Events.TerrainAtlasChanged -= TerrainAtlasChanged;
			game.WorldEvents.OnNewMap -= OnNewMap;
			game.WorldEvents.OnNewMapLoaded -= OnNewMapLoaded;
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
			
			game.Events.BlockDefinitionChanged -= BlockDefinitionChanged;
			game.Events.ViewDistanceChanged -= ViewDistanceChanged;
			game.Events.ProjectionChanged -= ProjectionChanged;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
			builder.Dispose();
		}
		
		public void Refresh() {
			chunkPos = new Vector3I(int.MaxValue);
			renderer.totalUsed = new int[game.TerrainAtlas1D.TexIds.Length];
			if (renderer.chunks == null || game.World.IsNotLoaded) return;
			ClearChunkCache();
			ResetChunkCache();
		}
		
		void RefreshBorders(int clipLevel) {
			chunkPos = new Vector3I(int.MaxValue);
			if (renderer.chunks == null || game.World.IsNotLoaded) return;
			
			int index = 0;
			for (int z = 0; z < chunksZ; z++)
				for (int y = 0; y < chunksY; y++)
					for (int x = 0; x < chunksX; x++)
			{
				bool isBorder = x == 0 || z == 0 || x == (chunksX - 1) || z == (chunksZ - 1);
				if (isBorder && (y * 16) < clipLevel)
					DeleteChunk(renderer.unsortedChunks[index], true);
				index++;
			}
		}
		
		void EnvVariableChanged(object sender, EnvVarEventArgs e) {
			if (e.Var == EnvVar.SunlightColour || e.Var == EnvVar.ShadowlightColour) {
				Refresh();
			} else if (e.Var == EnvVar.EdgeLevel || e.Var == EnvVar.SidesOffset) {
				int oldClip = builder.edgeLevel;
				builder.sidesLevel = Math.Max(0, game.World.Env.SidesHeight);
				builder.edgeLevel = Math.Max(0, game.World.Env.EdgeHeight);
				RefreshBorders(Math.Max(oldClip, builder.edgeLevel));
			}
		}

		void TerrainAtlasChanged(object sender, EventArgs e) {
			if (renderer._1DUsed == -1) {
				renderer.totalUsed = new int[game.TerrainAtlas1D.TexIds.Length];
			} else {
				bool refreshRequired = elementsPerBitmap != game.TerrainAtlas1D.elementsPerBitmap;
				if (refreshRequired) Refresh();
			}
			
			renderer._1DUsed = game.TerrainAtlas1D.CalcMaxUsedRow(game.TerrainAtlas, info);
			elementsPerBitmap = game.TerrainAtlas1D.elementsPerBitmap;
			ResetUsedFlags();
		}
		
		void BlockDefinitionChanged(object sender, EventArgs e) {
			renderer._1DUsed = game.TerrainAtlas1D.CalcMaxUsedRow(game.TerrainAtlas, info);
			ResetUsedFlags();
			Refresh();
		}
		
		void ProjectionChanged(object sender, EventArgs e) {
			lastCamPos = Utils.MaxPos();
		}
		
		void OnNewMap(object sender, EventArgs e) {
			game.ChunkUpdates = 0;
			ClearChunkCache();
			for (int i = 0; i < renderer.totalUsed.Length; i++)
				renderer.totalUsed[i] = 0;
			
			renderer.chunks = null;
			renderer.unsortedChunks = null;
			chunkPos = new Vector3I(int.MaxValue, int.MaxValue, int.MaxValue);
		}
		
		void ViewDistanceChanged(object sender, EventArgs e) {
			lastCamPos = Utils.MaxPos();
			lastRotY = float.MaxValue;
			lastHeadX = float.MaxValue;
		}
		
		internal void ResetUsedFlags() {
			int count = renderer._1DUsed;
			bool[] used = renderer.usedTranslucent;
			if (used == null || count > used.Length) {
				renderer.usedTranslucent = new bool[count];
				renderer.usedNormal = new bool[count];
				renderer.pendingTranslucent = new bool[count];
				renderer.pendingNormal = new bool[count];
			}
			
			for (int i = 0; i < count; i++) {
				renderer.pendingTranslucent[i] = true;
				renderer.usedTranslucent[i] = false;
				renderer.pendingNormal[i] = true;
				renderer.usedNormal[i] = false;
			}
		}
		
		int chunksX, chunksY, chunksZ;
		void OnNewMapLoaded(object sender, EventArgs e) {
			width = NextMultipleOf16(game.World.Width);
			height = NextMultipleOf16(game.World.Height);
			length = NextMultipleOf16(game.World.Length);
			
			chunksX = width >> 4; renderer.chunksX = chunksX;
			chunksY = height >> 4; renderer.chunksY = chunksY;
			chunksZ = length >> 4; renderer.chunksZ = chunksZ;
			
			int count = chunksX * chunksY * chunksZ;
			if (renderer.chunks == null || renderer.chunks.Length != count) {
				renderer.chunks = new ChunkInfo[count];
				renderer.unsortedChunks = new ChunkInfo[count];
				renderer.renderChunks = new ChunkInfo[count];
				distances = new int[count];
			}
			CreateChunkCache();
			builder.OnNewMapLoaded();
			lastCamPos = Utils.MaxPos();
			lastRotY = float.MaxValue;
			lastHeadX = float.MaxValue;
		}
		
		void CreateChunkCache() {
			int index = 0;
			for (int z = 0; z < length; z += 16)
				for (int y = 0; y < height; y += 16)
					for (int x = 0; x < width; x += 16)
			{
				renderer.chunks[index] = new ChunkInfo(x, y, z);
				renderer.unsortedChunks[index] = renderer.chunks[index];
				renderer.renderChunks[index] = renderer.chunks[index];
				distances[index] = 0;
				index++;
			}
		}
		
		void ResetChunkCache() {
			int index = 0;
			for (int z = 0; z < length; z += 16)
				for (int y = 0; y < height; y += 16)
					for (int x = 0; x < width; x += 16)
			{
				renderer.unsortedChunks[index].Reset(x, y, z);
				index++;
			}
		}
		
		void ClearChunkCache() {
			if (renderer.chunks == null) return;
			for (int i = 0; i < renderer.chunks.Length; i++)
				DeleteChunk(renderer.chunks[i], false);
			renderer.totalUsed = new int[game.TerrainAtlas1D.TexIds.Length];
		}
		
		void DeleteChunk(ChunkInfo info, bool decUsed) {
			info.Empty = false; info.AllAir = false;
			#if OCCLUSION
			info.OcclusionFlags = 0;
			info.OccludedFlags = 0;
			#endif
			
			if (info.NormalParts != null)
				DeleteData(ref info.NormalParts, decUsed);
			if (info.TranslucentParts != null)
				DeleteData(ref info.TranslucentParts, decUsed);
		}
		
		void DeleteData(ref ChunkPartInfo[] parts, bool decUsed) {
			if (decUsed) DecrementUsed(parts);
			
			for (int i = 0; i < parts.Length; i++)
				game.Graphics.DeleteVb(ref parts[i].VbId);
			parts = null;
		}
		
		static int NextMultipleOf16(int value) { return (value + 0x0F) & ~0x0F; }
		
		void ContextLost() { ClearChunkCache(); }
		void ContextRecreated() { Refresh(); }
		

		int chunksTarget = 12;
		const double targetTime = (1.0 / 30) + 0.01;
		public void UpdateChunks(double delta) {
			int chunkUpdates = 0;
			chunksTarget += delta < targetTime ? 1 : -1; // build more chunks if 30 FPS or over, otherwise slowdown.
			Utils.Clamp(ref chunksTarget, 4, 20);
			
			LocalPlayer p = game.LocalPlayer;
			Vector3 cameraPos = game.CurrentCameraPos;
			bool samePos = cameraPos == lastCamPos && p.HeadY == lastRotY
				&& p.HeadX == lastHeadX;
			renderer.renderCount = samePos ? UpdateChunksStill(ref chunkUpdates) :
				UpdateChunksAndVisibility(ref chunkUpdates);
			
			lastCamPos = cameraPos;
			lastRotY = p.HeadY; lastHeadX = p.HeadX;
			if (!samePos || chunkUpdates != 0)
				ResetUsedFlags();
		}
		Vector3 lastCamPos;
		float lastRotY, lastHeadX;
		
		int UpdateChunksAndVisibility(ref int chunkUpdates) {
			ChunkInfo[] chunks = renderer.chunks, render = renderer.renderChunks;
			int j = 0;
			int viewDistSqr = AdjustViewDist(game.ViewDistance);
			int userDistSqr = AdjustViewDist(game.UserViewDistance);
			
			for (int i = 0; i < chunks.Length; i++) {
				ChunkInfo info = chunks[i];
				if (info.Empty) continue;
				int distSqr = distances[i];
				bool noData = info.NormalParts == null && info.TranslucentParts == null;
				
				// Unload chunks beyond visible range
				if (!noData && distSqr >= userDistSqr + 32 * 16) {
					DeleteChunk(info, true); continue;
				}
				noData |= info.PendingDelete;
				
				if (noData && distSqr <= viewDistSqr && chunkUpdates < chunksTarget) {
					DeleteChunk(info, true);
					BuildChunk(info, ref chunkUpdates);
				}
				info.Visible = distSqr <= viewDistSqr &&
					game.Culling.SphereInFrustum(info.CentreX, info.CentreY, info.CentreZ, 14); // 14 ~ sqrt(3 * 8^2)
				if (info.Visible && !info.Empty) { render[j] = info; j++; }
			}
			return j;
		}
		
		int UpdateChunksStill(ref int chunkUpdates) {
			ChunkInfo[] chunks = renderer.chunks, render = renderer.renderChunks;
			int j = 0;
			int viewDistSqr = AdjustViewDist(game.ViewDistance);
			int userDistSqr = AdjustViewDist(game.UserViewDistance);
			
			for (int i = 0; i < chunks.Length; i++) {
				ChunkInfo info = chunks[i];
				if (info.Empty) continue;
				int distSqr = distances[i];
				bool noData = info.NormalParts == null && info.TranslucentParts == null;
				
				if (!noData && distSqr >= userDistSqr + 32 * 16) {
					DeleteChunk(info, true); continue;
				}
				noData |= info.PendingDelete;
				
				if (noData && distSqr <= userDistSqr && chunkUpdates < chunksTarget) {
					DeleteChunk(info, true);
					BuildChunk(info, ref chunkUpdates);
					
					// only need to update the visibility of chunks in range.
					info.Visible = distSqr <= viewDistSqr &&
						game.Culling.SphereInFrustum(info.CentreX, info.CentreY, info.CentreZ, 14); // 14 ~ sqrt(3 * 8^2)
					if (info.Visible && !info.Empty) { render[j] = info; j++; }
				} else if (info.Visible) {
					render[j] = info; j++;
				}
			}
			return j;
		}
		
		
		static int AdjustViewDist(float dist) {
			int viewDist = Utils.AdjViewDist(Math.Max(16, dist));
			return (viewDist + 24) * (viewDist + 24);
		}
		
		
		void BuildChunk(ChunkInfo info, ref int chunkUpdates) {
			game.ChunkUpdates++;
			builder.GetDrawInfo(info.CentreX - 8, info.CentreY - 8, info.CentreZ - 8,
			                    ref info.NormalParts, ref info.TranslucentParts, ref info.AllAir);
			
			info.PendingDelete = false;
			if (info.NormalParts == null && info.TranslucentParts == null) {
				info.Empty = true;
			} else {
				if (info.NormalParts != null)
					IncrementUsed(info.NormalParts);
				if (info.TranslucentParts != null)
					IncrementUsed(info.TranslucentParts);
			}
			chunkUpdates++;
		}
		
		void IncrementUsed(ChunkPartInfo[] parts) {
			for (int i = 0; i < parts.Length; i++) {
				if (parts[i].IndicesCount == 0) continue;
				renderer.totalUsed[i]++;
			}
		}
		
		void DecrementUsed(ChunkPartInfo[] parts) {
			for (int i = 0; i < parts.Length; i++) {
				if (parts[i].IndicesCount == 0) continue;
				renderer.totalUsed[i]--;
			}
		}
	}
}