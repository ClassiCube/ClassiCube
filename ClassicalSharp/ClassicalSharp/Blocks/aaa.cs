/*// Copyright 2015
using System;
using System.Drawing;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Renderers {

	public unsafe class Clouds3DEnvRenderer : EnvRenderer, IGameComponent {
		
		int cloudsVb = -1, cloudVertices, skyVb = -1, skyVertices;
		
		Game game;
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
			
			game.Graphics.SetBatchFormat(VertexFormat.P3fC4b);
			game.Graphics.BindVb(skyVb);
			if (skyY == normalY) {
				game.Graphics.DrawVb_IndexedTris(skyVertices, 0);
			} else {
				Matrix4 res, m = Matrix4.Identity;
				m.Row3.Y = skyY - normalY; // Y translation matrix
				Matrix4.Mult(out res, ref game.Graphics.View, ref m);
				
				game.Graphics.LoadMatrix(ref m);
				game.Graphics.DrawVb_IndexedTris(skyVertices, 0);
				game.Graphics.LoadMatrix(ref game.Graphics.View);
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
		
		void IGameComponent.Init(Game game) {
			this.game = game;
			base.Init(game);
			game.Graphics.SetFogStart(0);
			game.Graphics.Fog = true;
			ResetAllEnv(null, null);
			
			game.Events.ViewDistanceChanged += ResetAllEnv;
			game.Events.TextureChanged += TextureChangedCore;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
			game.SetViewDistance(game.UserViewDistance, false);
		}
		
		void IGameComponent.OnNewMap(Game game) {
			game.Graphics.Fog = false;
			ContextLost();
		}
		
		void IGameComponent.OnNewMapLoaded(Game game) {
			game.Graphics.Fog = true;
			ResetAllEnv(null, null);
		}
		
		void ResetAllEnv(object sender, EventArgs e) {
			UpdateFog();
			ContextRecreated();
		}
		
		void IDisposable.Dispose() {
			base.Dispose();
			ContextLost();
			
			game.Events.ViewDistanceChanged -= ResetAllEnv;
			game.Events.TextureChanged -= TextureChangedCore;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}
		
		void RenderClouds(double delta) {
			if (game.World.Env.CloudHeight < -2000) return;
			double time = game.accumulator;
			float offset = (float)(time / 2048f * 0.6f * map.Env.CloudsSpeed);
			game.Graphics.SetMatrixMode(MatrixType.Texture);
			Matrix4 m = Matrix4.Identity; m.Row3.X = offset; // translate X axis
			game.Graphics.LoadMatrix(ref m);
			game.Graphics.SetMatrixMode(MatrixType.Modelview);
			
			game.Graphics.AlphaTest = true;
			game.Graphics.Texturing = true;
			game.Graphics.BindTexture(game.CloudsTex);
			game.Graphics.SetBatchFormat(VertexFormat.P3fT2fC4b);
			game.Graphics.BindVb(cloudsVb);
			game.Graphics.DrawIndexedVb_TrisT2fC4b(cloudVertices * 6 / 4, 0);
			game.Graphics.AlphaTest = false;
			game.Graphics.Texturing = false;
			
			if (cloudsJoinVb > 0) {
				Matrix4.Translate(out m, -offset * 2048f, 0, 0);
				Matrix4 res;
				Matrix4.Mult(out res, ref m, ref game.Graphics.View);
				game.Graphics.LoadMatrix(ref res);
				
				game.Graphics.SetBatchFormat(VertexFormat.P3fC4b);
				game.Graphics.BindVb(cloudsJoinVb);
				game.Graphics.DrawIndexedVb_TrisT2fC4b(cloudsJoinVertices, 0);
				
				game.Graphics.SetBatchFormat(VertexFormat.P3fT2fC4b);
				game.Graphics.LoadMatrix(ref game.Graphics.View);
			}
			
			game.Graphics.SetMatrixMode(MatrixType.Texture);
			game.Graphics.LoadIdentityMatrix();
			game.Graphics.SetMatrixMode(MatrixType.Modelview);
		}
		
		void UpdateFog() {
			if (map.blocks == null) return;
			FastColour fogCol = FastColour.White;
			float fogDensity = 0;
			BlockOn(out fogDensity, out fogCol);
			
			if (fogDensity != 0) {
				game.Graphics.SetFogMode(Fog.Exp);
				game.Graphics.SetFogDensity(fogDensity);
			} else if (game.World.Env.ExpFog) {
				game.Graphics.SetFogMode(Fog.Exp);
				// f = 1-z/end   f = e^(-dz)
				//   solve for f = 0.01 gives:
				// e^(-dz)=0.01 --> -dz=ln(0.01)
				// 0.99=z/end   --> z=end*0.99
				//   therefore
				// d = -ln(0.01)/(end*0.99)
				double density = -Math.Log(0.01) / (game.ViewDistance * 0.99);
				game.Graphics.SetFogDensity((float)density);
			} else {
				game.Graphics.SetFogMode(Fog.Linear);
				game.Graphics.SetFogEnd(game.ViewDistance);
			}
			game.Graphics.ClearColour(fogCol);
			game.Graphics.SetFogColour(fogCol);
		}
		
		void ResetClouds() {
			if (map.blocks == null || game.Graphics.LostContext) return;
			game.Graphics.DeleteVb(ref cloudsVb);
			game.Graphics.DeleteVb(ref cloudsJoinVb);
			RebuildClouds((int)game.ViewDistance, legacy ? 128 : 65536);
		}
		
		void ResetSky() {
			if (map.blocks == null || game.Graphics.LostContext) return;
			game.Graphics.DeleteVb(ref skyVb);
			RebuildSky((int)game.ViewDistance, legacy ? 128 : 65536);
		}
		
		void ContextLost() {
			game.Graphics.DeleteVb(ref skyVb);
			game.Graphics.DeleteVb(ref cloudsVb);
			game.Graphics.DeleteVb(ref cloudsJoinVb);
		}
		
		void ContextRecreated() {
			ContextLost();
			ResetClouds();
			ResetSky();
		}
		
		
		void RebuildClouds(int extent, int axisSize) {
			extent = Utils.AdjViewDist(extent);
			int x1 = -extent, x2 = map.Width + extent;
			int z1 = -extent, z2 = map.Length + extent;
			cloudVertices = Utils.CountVertices(x2 - x1, z2 - z1, axisSize) * 2;
			
			VertexP3fT2fC4b[] vertices = new VertexP3fT2fC4b[cloudVertices];
			int i = 0;
			DrawCloudsY(x1, z1, x2, z2, map.Env.CloudHeight, axisSize, map.Env.CloudsCol.Pack(), ref i, vertices);
			DrawCloudsY(x1, z1, x2, z2, map.Env.CloudHeight + 5, axisSize, map.Env.CloudsCol.Pack(), ref i, vertices);
			
			fixed (VertexP3fT2fC4b* ptr = vertices) {
				cloudsVb = game.Graphics.CreateVb((IntPtr)ptr, VertexFormat.P3fT2fC4b, cloudVertices);
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
				skyVb = game.Graphics.CreateVb((IntPtr)ptr, VertexFormat.P3fC4b, skyVertices);
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
		
		void DrawCloudsY(int x1, int z1, int x2, int z2, int y, int axisSize, int col, ref int i, VertexP3fT2fC4b[] vertices) {
			int endX = x2, endZ = z2, startZ = z1;
			// adjust range so that largest negative uv coordinate is shifted to 0 or above.
			float offset = Utils.CeilDiv(-x1, 2048);
			
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
		
		
		int cloudsJoinVb, cloudsJoinVertices;
		
		static bool IsSolid( int x, int y, FastBitmap bmp ) {
			// wrap around
			if( x == -1 ) x = 255;
			if( y == -1 ) y = 255;
			if( x == 256 ) x = 0;
			if( y == 256 ) y = 0;
			return ( bmp.GetRowPtr( y )[x] & 0xFFFFFF ) != 0;
		}
		
		
		void TextureChangedCore(object sender, TextureEventArgs e) {
			if (!(e.Name == "cloud.png" || e.Name == "clouds.png")) return;
			
			using (Bitmap bmp = Platform.ReadBmp(game.Drawer2D, e.Data))
				using (FastBitmap fastBmp = new FastBitmap(bmp, true, true))
			{
				CalcClouds(fastBmp);
			}
		}
		
		void CalcClouds(FastBitmap fastBmp) {
			Console.WriteLine( "BEGIN CLOUDS!" );
			int elements = 0;
			int col = map.Env.CloudsCol.Pack();
			
			for( int y = 0; y < fastBmp.Height; y++ ) {
				int* row = fastBmp.GetRowPtr( y );
				for( int x = 0; x < fastBmp.Width; x++ ) {
					if( ( row[x] & 0xFFFFFF ) != 0 ) {
						if( !IsSolid( x - 1, y, fastBmp ) ) elements++;
						if( !IsSolid( x + 1, y, fastBmp ) ) elements++;
						if( !IsSolid( x, y - 1, fastBmp ) ) elements++;
						if( !IsSolid( x, y + 1, fastBmp ) ) elements++;
					}
				}
			}
			
			cloudsJoinVertices = elements * 4;
			VertexP3fC4b[] vertices = new VertexP3fC4b[cloudsJoinVertices];
			int index = 0;
			
			// Pass #2: Make the vertices.
			int cloudY = map.Height + 2;
			for( int y = 0; y < fastBmp.Height; y++ ) {
				int* row = fastBmp.GetRowPtr( y );
				for( int x = 0; x < fastBmp.Width; x++ ) {
					if( ( row[x] & 0xFFFFFF ) != 0 ) {
						if( !IsSolid( x - 1, y, fastBmp ) ) {
							DrawPlane( x, x, y, y + 1, cloudY, cloudY + 5, col, vertices, ref index );
						}
						if( !IsSolid( x + 1, y, fastBmp ) ) {
							DrawPlane( x + 1, x + 1, y, y + 1, cloudY, cloudY + 5, col, vertices, ref index );
						}
						if( !IsSolid( x, y - 1, fastBmp ) ) {
							DrawPlane( x, x + 1, y, y, cloudY, cloudY + 5, col, vertices, ref index );
						}
						if( !IsSolid( x, y + 1, fastBmp ) ) {
							DrawPlane( x, x + 1, y + 1, y + 1, cloudY, cloudY + 5, col, vertices, ref index );
						}
					}
				}
			}
			
			game.Graphics.DeleteVb(ref cloudsJoinVb);
			fixed (VertexP3fC4b* ptr = vertices) {
				cloudsJoinVb = game.Graphics.CreateVb( (IntPtr)ptr, VertexFormat.P3fC4b, cloudsJoinVertices );
			}
		}

		void DrawPlane( int x1, int x2, int z1, int z2, int y1, int y2, int col, VertexP3fC4b[] vertices, ref int index ) {
			const int scale = 8;
			x1 *= scale; x2 *= scale; z1 *= scale; z2 *= scale;
			vertices[index++] = new VertexP3fC4b( x1, y1 + 0.1f, z1, col );
			vertices[index++] = new VertexP3fC4b( x1, y2 + 0.1f, z1, col );
			vertices[index++] = new VertexP3fC4b( x2, y2 + 0.1f, z2, col );
			vertices[index++] = new VertexP3fC4b( x2, y1 + 0.1f, z2, col );
		}
	}
}*/