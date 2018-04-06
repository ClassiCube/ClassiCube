// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;
using BlockID = System.UInt16;
using BlockRaw = System.Byte;

namespace ClassicalSharp.Renderers {

	public sealed class WeatherRenderer : IGameComponent {
		Game game;
		World map;
		
		int rainTexId, snowTexId;
		int vb;
		public short[] heightmap;
		
		const int extent = 4;
		VertexP3fT2fC4b[] vertices = new VertexP3fT2fC4b[8 * (extent * 2 + 1) * (extent * 2 + 1)];
		double rainAcc;
		Vector3I lastPos = new Vector3I(Int32.MinValue);
		
		public void Init(Game game) {
			this.game = game;
			map = game.World;
			game.Events.TextureChanged += TextureChanged;
			
			ContextRecreated();
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
		}
		
		public void Render(double deltaTime) {
			Weather weather = map.Env.Weather;
			if (weather == Weather.Sunny) return;
			if (heightmap == null) InitHeightmap();
			IGraphicsApi gfx = game.Graphics;
			
			gfx.BindTexture(weather == Weather.Rainy ? rainTexId : snowTexId);
			Vector3 camPos = game.CurrentCameraPos;
			Vector3I pos = Vector3I.Floor(camPos);
			bool moved = pos != lastPos;
			lastPos = pos;
			WorldEnv env = game.World.Env;
			
			float speed = (weather == Weather.Rainy ? 1.0f : 0.2f) * env.WeatherSpeed;
			float vOffset = (float)game.accumulator * speed;
			rainAcc += deltaTime;
			bool particles = weather == Weather.Rainy;

			int vCount = 0;
			FastColour col = game.World.Env.Sunlight;
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			
			for (int dx = -extent; dx <= extent; dx++)
				for (int dz = -extent; dz <= extent; dz++)
			{
				int x = pos.X + dx, z = pos.Z + dz;
				float y = RainHeight(x, z);
				float height = Math.Max(game.World.Height, pos.Y + 64) - y;
				if (height <= 0) continue;
				
				if (particles && (rainAcc >= 0.25 || moved))
					game.ParticleManager.AddRainParticle(new Vector3(x, y, z));
				
				float alpha = AlphaAt(dx * dx + dz * dz);
				Utils.Clamp(ref alpha, 0, 0xFF);
				col.A = (byte)alpha;
				
				// NOTE: Making vertex is inlined since this is called millions of times.
				v.Colour = col.Pack();
				float worldV = vOffset + (z & 1) / 2f - (x & 0x0F) / 16f;
				float v1 = y / 6f + worldV, v2 = (y + height) / 6f + worldV;
				float x1 = x,     y1 = y,          z1 = z;
				float x2 = x + 1, y2 = y + height, z2 = z + 1;
				
				v.X = x1; v.Y = y1; v.Z = z1; v.U = 0; v.V = v1; vertices[vCount++] = v;
				v.Y = y2;                    v.V = v2; vertices[vCount++] = v;
				v.X = x2;           v.Z = z2; v.U = 1; 	         vertices[vCount++] = v;
				v.Y = y1;                    v.V = v1; vertices[vCount++] = v;
				
				v.Z = z1;				  	 vertices[vCount++] = v;
				v.Y = y2;                    v.V = v2; vertices[vCount++] = v;
				v.X = x1;           v.Z = z2; v.U = 0;		     vertices[vCount++] = v;
				v.Y = y1;                    v.V = v1; vertices[vCount++] = v;
			}
			if (particles && (rainAcc >= 0.25 || moved)) {
				rainAcc = 0;
			}
			if (vCount == 0) return;
			
			gfx.AlphaTest = false;
			gfx.DepthWrite = false;
			gfx.AlphaArgBlend = true;
			
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.UpdateDynamicVb_IndexedTris(vb, vertices, vCount);
			
			gfx.AlphaArgBlend = false;
			gfx.AlphaTest = true;
			gfx.DepthWrite = true;
		}
		
		float AlphaAt(float x) {
			// Wolfram Alpha: fit {0,178},{1,169},{4,147},{9,114},{16,59},{25,9}
			float falloff = 0.05f * x * x - 7 * x;
			return 178 + falloff * game.World.Env.WeatherFade;
		}

		int length, width, maxY, oneY;
		public void Ready(Game game) { }
		public void Reset(Game game) { OnNewMap(game); }
		
		public void OnNewMap(Game game) {
			heightmap = null;
			lastPos = new Vector3I(Int32.MaxValue);
		}
		
		public void OnNewMapLoaded(Game game) {
			length = map.Length;
			width = map.Width;
			maxY = map.Height - 1;
			oneY = length * width;
		}
		
		void TextureChanged(object sender, TextureEventArgs e) {
			if (e.Name == "snow.png")
				game.UpdateTexture(ref snowTexId, e.Name, e.Data, false);
			else if (e.Name == "rain.png")
				game.UpdateTexture(ref rainTexId, e.Name, e.Data, false);
		}
		
		public void Dispose() {
			game.Graphics.DeleteTexture(ref rainTexId);
			game.Graphics.DeleteTexture(ref snowTexId);
			ContextLost();
			
			game.Events.TextureChanged -= TextureChanged;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}
		
		void InitHeightmap() {
			heightmap = new short[map.Width * map.Length];
			for (int i = 0; i < heightmap.Length; i++) {
				heightmap[i] = short.MaxValue;
			}
		}
		
		float RainHeight(int x, int z) {
			if (x < 0 || z < 0 || x >= width || z >= length) return map.Env.EdgeHeight;
			int index = (x * length) + z;
			int height = heightmap[index];
			int y = height == short.MaxValue ? CalcHeightAt(x, maxY, z, index) : height;
			return y == -1 ? 0 : y + BlockInfo.MaxBB[map.GetBlock(x, y, z)].Y;
		}
		
		int CalcHeightAt(int x, int maxY, int z, int index) {
			int i = (maxY * length + z) * width + x;
			BlockRaw[] blocks = map.blocks;
			
			if (BlockInfo.MaxUsed < 256) {
				for (int y = maxY; y >= 0; y--, i -= oneY) {
					byte draw = BlockInfo.Draw[blocks[i]];
					if (!(draw == DrawType.Gas || draw == DrawType.Sprite)) {
						heightmap[index] = (short)y;
						return y;
					}
				}
			} else {
				BlockRaw[] blocks2 = game.World.blocks2;
				for (int y = maxY; y >= 0; y--, i -= oneY) {
					byte draw = BlockInfo.Draw[blocks[i] | (blocks2[i] << 8)];
					if (!(draw == DrawType.Gas || draw == DrawType.Sprite)) {
						heightmap[index] = (short)y;
						return y;
					}
				}
			}		
			
			heightmap[index] = -1;
			return -1;
		}
		
		internal void OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock) {
			bool didBlock = !(BlockInfo.Draw[oldBlock] == DrawType.Gas || BlockInfo.Draw[oldBlock] == DrawType.Sprite);
			bool nowBlock = !(BlockInfo.Draw[newBlock] == DrawType.Gas || BlockInfo.Draw[newBlock] == DrawType.Sprite);
			if (didBlock == nowBlock) return;
			
			int index = (x * length) + z;
			int height = heightmap[index];
			// Two cases can be skipped here:
			// a) rain height was not calculated to begin with (height is short.MaxValue)
			// b) changed y is below current calculated rain height
			if (y < height) return;
			
			if (nowBlock) {
				// Simple case: Rest of column below is now not visible to rain.
				heightmap[index] = (short)y;
			} else {
				// Part of the column is now visible to rain, we don't know how exactly how high it should be though.
				// However, we know that if the old block was above or equal to rain height, then the new rain height must be <= old block.y
				CalcHeightAt(x, y, z, index);
			}
		}
		
		void ContextLost() { game.Graphics.DeleteVb(ref vb); }
		
		void ContextRecreated() {
			vb = game.Graphics.CreateDynamicVb(VertexFormat.P3fT2fC4b, vertices.Length);
		}
	}
}
