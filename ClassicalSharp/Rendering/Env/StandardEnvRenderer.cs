// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Renderers {

	public unsafe class StandardEnvRenderer : EnvRenderer {
		
		int cloudsVb = -1, cloudVertices, skyVb = -1, skyVertices;
		internal bool legacy;
		
		public override void UseLegacyMode(bool legacy) {
			this.legacy = legacy;
			ContextRecreated();
		}
		
		public override void Render(double deltaTime) {
			if (skyVb == -1 || cloudsVb == -1) return;
			if (!game.SkyboxRenderer.ShouldRender)
				RenderMainEnv(deltaTime);
			UpdateFog();
		}
		
		void RenderMainEnv(double deltaTime) {
			Vector3 pos = game.CurrentCameraPos;
			float normalY = map.Height + 8;
			float skyY = Math.Max(pos.Y + 8, normalY);
			
			gfx.SetBatchFormat(VertexFormat.P3fC4b);
			gfx.BindVb(skyVb);
			if (skyY == normalY) {
				gfx.DrawIndexedVb(DrawMode.Triangles, skyVertices * 6 / 4, 0);
			} else {
				Matrix4 m = Matrix4.Identity;
				m.Row3.Y = skyY - normalY; // Y translation matrix
				
				gfx.PushMatrix();
				gfx.MultiplyMatrix(ref m);
				gfx.DrawIndexedVb(DrawMode.Triangles, skyVertices * 6 / 4, 0);
				gfx.PopMatrix();
			}
			RenderClouds(deltaTime);
		}
		
		protected override void EnvVariableChanged(object sender, EnvVarEventArgs e) {
			if (e.Var == EnvVar.SkyColour) {
				ResetSky();
			} else if (e.Var == EnvVar.FogColour) {
				UpdateFog();
			} else if (e.Var == EnvVar.CloudsColour) {
				ResetClouds();
			} else if (e.Var == EnvVar.CloudsLevel) {
				ResetSky();
				ResetClouds();
			}
		}
		
		public override void Init(Game game) {
			base.Init(game);
			gfx.SetFogStart(0);
			gfx.Fog = true;
			ResetAllEnv(null, null);
			
			game.Events.ViewDistanceChanged += ResetAllEnv;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
			game.SetViewDistance(game.UserViewDistance, false);
		}
		
		public override void OnNewMap(Game game) {
			gfx.Fog = false;
			gfx.DeleteVb(ref skyVb);
			gfx.DeleteVb(ref cloudsVb);
		}
		
		public override void OnNewMapLoaded(Game game) {
			gfx.Fog = true;
			ResetAllEnv(null, null);
		}
		
		void ResetAllEnv(object sender, EventArgs e) {
			UpdateFog();
			ContextRecreated();
		}
		
		public override void Dispose() {
			base.Dispose();
			ContextLost();
			
			game.Events.ViewDistanceChanged -= ResetAllEnv;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}
		
		void RenderClouds(double delta) {
			if (game.World.Env.CloudHeight < -2000) return;
			double time = game.accumulator;
			float offset = (float)(time / 2048f * 0.6f * map.Env.CloudsSpeed);
			gfx.SetMatrixMode(MatrixType.Texture);
			Matrix4 matrix = Matrix4.Identity; matrix.Row3.X = offset; // translate X axis
			gfx.LoadMatrix(ref matrix);
			gfx.SetMatrixMode(MatrixType.Modelview);
			
			gfx.AlphaTest = true;
			gfx.Texturing = true;
			gfx.BindTexture(game.CloudsTex);
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.BindVb(cloudsVb);
			gfx.DrawIndexedVb(DrawMode.Triangles, cloudVertices * 6 / 4, 0);
			gfx.AlphaTest = false;
			gfx.Texturing = false;
			
			gfx.SetMatrixMode(MatrixType.Texture);
			gfx.LoadIdentityMatrix();
			gfx.SetMatrixMode(MatrixType.Modelview);
		}
		
		void UpdateFog() {
			if (map.blocks == null) return;
			FastColour fogCol = FastColour.White;
			float fogDensity = 0;
			BlockID block = BlockOn(out fogDensity, out fogCol);
			
			if (fogDensity != 0) {
				gfx.SetFogMode(Fog.Exp);
				gfx.SetFogDensity(fogDensity);
			} else if (game.World.Env.ExpFog) {
				gfx.SetFogMode(Fog.Exp);
				// f = 1-z/end   f = e^(-dz)
				//   solve for f = 0.01 gives:
				// e^(-dz)=0.01 --> -dz=ln(0.01)
				// 0.99=z/end   --> z=end*0.99
				//   therefore
				// d = -ln(0.01)/(end*0.99)
				double density = -Math.Log(0.01) / (game.ViewDistance * 0.99);
				gfx.SetFogDensity((float)density);
			} else {
				gfx.SetFogMode(Fog.Linear);
				gfx.SetFogEnd(game.ViewDistance);
			}
			gfx.ClearColour(fogCol);
			gfx.SetFogColour(fogCol);
		}
		
		void ResetClouds() {
			if (map.blocks == null || game.Graphics.LostContext) return;
			gfx.DeleteVb(ref cloudsVb);
			RebuildClouds((int)game.ViewDistance, legacy ? 128 : 65536);
		}
		
		void ResetSky() {
			if (map.blocks == null || game.Graphics.LostContext) return;
			gfx.DeleteVb(ref skyVb);
			RebuildSky((int)game.ViewDistance, legacy ? 128 : 65536);
		}
		
		void ContextLost() {
			game.Graphics.DeleteVb(ref skyVb);
			game.Graphics.DeleteVb(ref cloudsVb);
		}
		
		void ContextRecreated() {
			ResetClouds();
			ResetSky();
		}
		
		
		void RebuildClouds(int extent, int axisSize) {
			extent = Utils.AdjViewDist(extent);
			int x1 = -extent, x2 = map.Width + extent;
			int z1 = -extent, z2 = map.Length + extent;
			cloudVertices = Utils.CountVertices(x2 - x1, z2 - z1, axisSize);
			
			VertexP3fT2fC4b[] vertices = new VertexP3fT2fC4b[cloudVertices];
			DrawCloudsY(x1, z1, x2, z2, map.Env.CloudHeight, axisSize, map.Env.CloudsCol.Pack(), vertices);
			fixed (VertexP3fT2fC4b* ptr = vertices) {
				cloudsVb = gfx.CreateVb((IntPtr)ptr, VertexFormat.P3fT2fC4b, cloudVertices);
			}
		}
		
		void RebuildSky(int extent, int axisSize) {
			extent = Utils.AdjViewDist(extent);
			int x1 = -extent, x2 = map.Width + extent;
			int z1 = -extent, z2 = map.Length + extent;
			skyVertices = Utils.CountVertices(x2 - x1, z2 - z1, axisSize);
			
			VertexP3fC4b[] vertices = new VertexP3fC4b[skyVertices];
			int height = Math.Max(map.Height + 2 + 6, map.Env.CloudHeight + 6);
			
			DrawSkyY(x1, z1, x2, z2, height, axisSize, map.Env.SkyCol.Pack(), vertices);
			fixed (VertexP3fC4b* ptr = vertices) {
				skyVb = gfx.CreateVb((IntPtr)ptr, VertexFormat.P3fC4b, skyVertices);
			}
		}
		
		void DrawSkyY(int x1, int z1, int x2, int z2, int y, int axisSize, int col, VertexP3fC4b[] vertices) {
			int endX = x2, endZ = z2, startZ = z1;
			int i = 0;
			
			for (; x1 < endX; x1 += axisSize) {
				x2 = x1 + axisSize;
				if (x2 > endX) x2 = endX;
				z1 = startZ;
				for (; z1 < endZ; z1 += axisSize) {
					z2 = z1 + axisSize;
					if (z2 > endZ) z2 = endZ;
					
					vertices[i++] = new VertexP3fC4b(x1, y, z1, col);
					vertices[i++] = new VertexP3fC4b(x1, y, z2, col);
					vertices[i++] = new VertexP3fC4b(x2, y, z2, col);
					vertices[i++] = new VertexP3fC4b(x2, y, z1, col);
				}
			}
		}
		
		void DrawCloudsY(int x1, int z1, int x2, int z2, int y, int axisSize, int col, VertexP3fT2fC4b[] vertices) {
			int endX = x2, endZ = z2, startZ = z1;
			// adjust range so that largest negative uv coordinate is shifted to 0 or above.
			float offset = Utils.CeilDiv(-x1, 2048);
			int i = 0;
			
			for (; x1 < endX; x1 += axisSize) {
				x2 = x1 + axisSize;
				if (x2 > endX) x2 = endX;
				z1 = startZ;
				for (; z1 < endZ; z1 += axisSize) {
					z2 = z1 + axisSize;
					if (z2 > endZ) z2 = endZ;
					
					vertices[i++] = new VertexP3fT2fC4b(x1, y + 0.1f, z1, x1 / 2048f + offset, z1 / 2048f + offset, col);
					vertices[i++] = new VertexP3fT2fC4b(x1, y + 0.1f, z2, x1 / 2048f + offset, z2 / 2048f + offset, col);
					vertices[i++] = new VertexP3fT2fC4b(x2, y + 0.1f, z2, x2 / 2048f + offset, z2 / 2048f + offset, col);
					vertices[i++] = new VertexP3fT2fC4b(x2, y + 0.1f, z1, x2 / 2048f + offset, z1 / 2048f + offset, col);
				}
			}
		}
	}
}