// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.Map;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Textures;
using OpenTK;
using BlockID = System.UInt16;

namespace ClassicalSharp.Renderers {
	
	public class ChunkInfo {
		
		public ushort CentreX, CentreY, CentreZ;
		public bool Visible, Empty, PendingDelete, AllAir;
		
		public bool DrawLeft, DrawRight, DrawFront, DrawBack, DrawBottom, DrawTop;
		#if OCCLUSION
		public bool Visited = false, Occluded = false;
		public byte OcclusionFlags, OccludedFlags, DistanceFlags;
		#endif
		
		public ChunkPartInfo[] NormalParts;
		public ChunkPartInfo[] TranslucentParts;
		
		public ChunkInfo(int x, int y, int z) { Reset(x, y, z); }
		
		public void Reset(int x, int y, int z) {
			CentreX = (ushort)(x + 8);
			CentreY = (ushort)(y + 8);
			CentreZ = (ushort)(z + 8);
			
			Visible = true; Empty = false; PendingDelete = false; AllAir = false;
			DrawLeft = false; DrawRight = false; DrawFront = false;
			DrawBack = false; DrawBottom = false; DrawTop = false;
		}
	}
	
	public partial class MapRenderer : IDisposable {
		
		Game game;
		
		internal int _1DUsed = -1, chunksX, chunksY, chunksZ;
		internal int renderCount = 0;
		internal ChunkInfo[] chunks, renderChunks, unsortedChunks;
		internal bool[] usedTranslucent, usedNormal;
		internal bool[] pendingTranslucent, pendingNormal;
		internal int[] normalPartsCount, translucentPartsCount;
		internal ChunkUpdater updater;
		bool inTranslucent = false;
		
		public MapRenderer(Game game) {
			this.game = game;
			updater = new ChunkUpdater(game, this);
			SetMeshBuilder(DefaultMeshBuilder());
		}
		
		public void Dispose() { updater.Dispose(); }
		
		/// <summary> Discards any built meshes for all chunks in the map.</summary>
		public void Refresh() { updater.Refresh(); }
		
		/// <summary> Retrieves the information for the given chunk. </summary>
		public ChunkInfo GetChunk(int cx, int cy, int cz) {
			return unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
		}
		
		/// <summary> Marks the given chunk as needing to be deleted. </summary>
		public void RefreshChunk(int cx, int cy, int cz) {
			if (cx < 0 || cy < 0 || cz < 0 ||
			    cx >= chunksX || cy >= chunksY || cz >= chunksZ) return;
			
			ChunkInfo info = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
			if (info.AllAir) return; // do not recreate chunks completely air
			info.Empty = false;
			info.PendingDelete = true;
		}
		
		/// <summary> Potentially generates meshes for several pending chunks. </summary>
		public void Update(double deltaTime) {
			if (chunks == null) return;
			ChunkSorter.UpdateSortOrder(game, updater);
			updater.UpdateChunks(deltaTime);
		}
		
		/// <summary> Sets the mesh builder that is used to generate meshes for chunks. </summary>
		public void SetMeshBuilder(ChunkMeshBuilder newBuilder) {
			if (updater.builder != null) updater.builder.Dispose();
			
			updater.builder = newBuilder;
			updater.builder.Init(game);
			updater.builder.OnNewMapLoaded();
		}

		/// <summary> Creates a new instance of the default mesh builder implementation. </summary>
		public ChunkMeshBuilder DefaultMeshBuilder() {
			if (game.SmoothLighting)
				return new AdvLightingMeshBuilder();
			return new NormalMeshBuilder();
		}
		
		
		/// <summary> Renders all opaque and transparent blocks. </summary>
		/// <remarks> Pixels are either treated as fully replacing existing pixel, or skipped. </remarks>
		public void RenderNormal(double deltaTime) {
			if (chunks == null) return;
			IGraphicsApi gfx = game.Graphics;
			
			int[] texIds = TerrainAtlas1D.TexIds;
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.Texturing = true;
			gfx.AlphaTest = true;
			
			gfx.EnableMipmaps();
			for (int batch = 0; batch < _1DUsed; batch++) {
				if (normalPartsCount[batch] <= 0) continue;
				if (pendingNormal[batch] || usedNormal[batch]) {
					gfx.BindTexture(texIds[batch]);
					RenderNormalBatch(batch);
					pendingNormal[batch] = false;
				}
			}
			gfx.DisableMipmaps();
			
			CheckWeather(deltaTime);
			gfx.AlphaTest = false;
			gfx.Texturing = false;
			#if DEBUG_OCCLUSION
			DebugPickedPos();
			#endif
		}
		
		/// <summary> Renders all translucent (e.g. water) blocks. </summary>
		/// <remarks> Pixels drawn blend into existing geometry. </remarks>
		public void RenderTranslucent(double deltaTime) {
			if (chunks == null) return;
			IGraphicsApi gfx = game.Graphics;
			
			// First fill depth buffer
			int vertices = game.Vertices;
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.Texturing = false;
			gfx.AlphaBlending = false;
			gfx.ColourWriteMask(false, false, false, false);
			for (int batch = 0; batch < _1DUsed; batch++) {
				if (translucentPartsCount[batch] <= 0) continue;
				if (pendingTranslucent[batch] || usedTranslucent[batch]) {
					RenderTranslucentBatch(batch);
					pendingTranslucent[batch] = false;
				}
			}
			game.Vertices = vertices;
			
			// Then actually draw the transluscent blocks
			gfx.AlphaBlending = true;
			gfx.Texturing = true;
			gfx.ColourWriteMask(true, true, true, true);
			gfx.DepthWrite = false; // we already calculated depth values in depth pass
			
			int[] texIds = TerrainAtlas1D.TexIds;
			gfx.EnableMipmaps();
			for (int batch = 0; batch < _1DUsed; batch++) {
				if (translucentPartsCount[batch] <= 0) continue;
				if (!usedTranslucent[batch]) continue;
				gfx.BindTexture(texIds[batch]);
				RenderTranslucentBatch(batch);
			}
			gfx.DisableMipmaps();
			
			gfx.DepthWrite = true;
			// If we weren't under water, render weather after to blend properly
			if (!inTranslucent && game.World.Env.Weather != Weather.Sunny) {
				gfx.AlphaTest = true;
				game.WeatherRenderer.Render(deltaTime);
				gfx.AlphaTest = false;
			}
			gfx.AlphaBlending = false;
			gfx.Texturing = false;
		}
		
		
		void CheckWeather(double deltaTime) {
			WorldEnv env = game.World.Env;
			Vector3 pos = game.CurrentCameraPos;
			Vector3I coords = Vector3I.Floor(pos);
			
			BlockID block = game.World.SafeGetBlock(coords);
			bool outside = coords.X < 0 || coords.Y < 0 || coords.Z < 0 || coords.X >= game.World.Width || coords.Z >= game.World.Length;
			inTranslucent = BlockInfo.Draw[block] == DrawType.Translucent
				|| (pos.Y < env.EdgeHeight && outside);

			// If we are under water, render weather before to blend properly
			if (!inTranslucent || env.Weather == Weather.Sunny) return;
			game.Graphics.AlphaBlending = true;
			game.WeatherRenderer.Render(deltaTime);
			game.Graphics.AlphaBlending = false;
		}
		
		void RenderNormalBatch(int batch) {
			IGraphicsApi gfx = game.Graphics;
			for (int i = 0; i < renderCount; i++) {
				ChunkInfo info = renderChunks[i];
				if (info.NormalParts == null) continue;

				ChunkPartInfo part = info.NormalParts[batch];
				if (part.VerticesCount == 0) continue;
				usedNormal[batch] = true;
				
				gfx.BindVb(part.VbId);
				bool drawLeft = info.DrawLeft && part.LeftCount > 0;
				bool drawRight = info.DrawRight && part.RightCount > 0;
				bool drawBottom = info.DrawBottom && part.BottomCount > 0;
				bool drawTop = info.DrawTop && part.TopCount > 0;
				bool drawFront = info.DrawFront && part.FrontCount > 0;
				bool drawBack = info.DrawBack && part.BackCount > 0;
				
				int offset = part.SpriteCount;
				if (drawLeft && drawRight) {
					gfx.FaceCulling = true;
					gfx.DrawIndexedVb_TrisT2fC4b(part.LeftCount + part.RightCount, offset);
					gfx.FaceCulling = false;
					game.Vertices += part.LeftCount + part.RightCount;
				} else if (drawLeft) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.LeftCount, offset);
					game.Vertices += part.LeftCount;
				} else if (drawRight) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.RightCount, offset + part.LeftCount);
					game.Vertices += part.RightCount;
				}
				offset += part.LeftCount + part.RightCount;
				
				if (drawFront && drawBack) {
					gfx.FaceCulling = true;
					gfx.DrawIndexedVb_TrisT2fC4b(part.FrontCount + part.BackCount, offset);
					gfx.FaceCulling = false;
					game.Vertices += part.FrontCount + part.BackCount;
				} else if (drawFront) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.FrontCount, offset);
					game.Vertices += part.FrontCount;
				} else if (drawBack) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.BackCount, offset + part.FrontCount);
					game.Vertices += part.BackCount;
				}
				offset += part.FrontCount + part.BackCount;
				
				if (drawBottom && drawTop) {
					gfx.FaceCulling = true;
					gfx.DrawIndexedVb_TrisT2fC4b(part.BottomCount + part.TopCount, offset);
					gfx.FaceCulling = false;
					game.Vertices += part.TopCount + part.BottomCount;
				} else if (drawBottom) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.BottomCount, offset);
					game.Vertices += part.BottomCount;
				} else if (drawTop) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.TopCount, offset + part.BottomCount);
					game.Vertices += part.TopCount;
				}
				
				if (part.SpriteCount == 0) continue;
				int count = part.SpriteCount / 4; // 4 per sprite
				gfx.FaceCulling = true;
				if (info.DrawRight || info.DrawFront) {
					gfx.DrawIndexedVb_TrisT2fC4b(count, 0); game.Vertices += count;
				}
				if (info.DrawLeft || info.DrawBack) {
					gfx.DrawIndexedVb_TrisT2fC4b(count, count); game.Vertices += count;
				}
				if (info.DrawLeft || info.DrawFront) {
					gfx.DrawIndexedVb_TrisT2fC4b(count, count * 2); game.Vertices += count;
				}
				if (info.DrawRight || info.DrawBack) {
					gfx.DrawIndexedVb_TrisT2fC4b(count, count * 3); game.Vertices += count;
				}
				gfx.FaceCulling = false;
			}
		}

		void RenderTranslucentBatch(int batch) {
			IGraphicsApi gfx = game.Graphics;
			for (int i = 0; i < renderCount; i++) {
				ChunkInfo info = renderChunks[i];
				if (info.TranslucentParts == null) continue;
				
				ChunkPartInfo part = info.TranslucentParts[batch];
				if (part.VerticesCount == 0) continue;
				usedTranslucent[batch] = true;
				
				gfx.BindVb(part.VbId);
				bool drawLeft = (inTranslucent || info.DrawLeft) && part.LeftCount > 0;
				bool drawRight = (inTranslucent || info.DrawRight) && part.RightCount > 0;
				bool drawBottom = (inTranslucent || info.DrawBottom) && part.BottomCount > 0;
				bool drawTop = (inTranslucent || info.DrawTop) && part.TopCount > 0;
				bool drawFront = (inTranslucent || info.DrawFront) && part.FrontCount > 0;
				bool drawBack = (inTranslucent || info.DrawBack) && part.BackCount > 0;
				
				int offset = 0;
				if (drawLeft && drawRight) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.LeftCount + part.RightCount, offset);
					game.Vertices += (part.LeftCount + part.RightCount);
				} else if (drawLeft) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.LeftCount, offset);
					game.Vertices += part.LeftCount;
				} else if (drawRight) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.RightCount, offset + part.LeftCount);
					game.Vertices += part.RightCount;
				}
				offset += part.LeftCount + part.RightCount;
				
				if (drawFront && drawBack) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.FrontCount + part.BackCount, offset);
					game.Vertices += (part.FrontCount + part.BackCount);
				} else if (drawFront) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.FrontCount, offset);
					game.Vertices += part.FrontCount;
				} else if (drawBack) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.BackCount, offset + part.FrontCount);
					game.Vertices += part.BackCount;
				}
				offset += part.FrontCount + part.BackCount;
				
				if (drawBottom && drawTop) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.BottomCount + part.TopCount, offset);
					game.Vertices += (part.BottomCount + part.TopCount);
				} else if (drawBottom) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.BottomCount, offset);
					game.Vertices += part.BottomCount;
				} else if (drawTop) {
					gfx.DrawIndexedVb_TrisT2fC4b(part.TopCount, offset + part.BottomCount);
					game.Vertices += part.TopCount;
				}
			}
		}
	}
}