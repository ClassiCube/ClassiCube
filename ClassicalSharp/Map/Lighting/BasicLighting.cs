// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Map {
	
	/// <summary> Manages lighting through a simple heightmap, where each block is either in sun or shadow. </summary>
	public sealed partial class BasicLighting : IWorldLighting {
		
		int oneY, shadow, shadowZSide, shadowXSide, shadowYBottom;
		Game game;
		
		public override void Reset(Game game) { heightmap = null; }
		
		public override void OnNewMap(Game game) {
			SetSun(WorldEnv.DefaultSunlight);
			SetShadow(WorldEnv.DefaultShadowlight);
			heightmap = null;
		}
		
		public override void OnNewMapLoaded(Game game) {
			width = game.World.Width;
			height = game.World.Height;
			length = game.World.Length;
			this.game = game;
			oneY = width * length;
			
			heightmap = new short[width * length];
			Refresh();
		}
		
		public override void Init(Game game) {
			this.game = game;
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
			SetSun(WorldEnv.DefaultSunlight);
			SetShadow(WorldEnv.DefaultShadowlight);
		}
		
		public override void Dispose() {
			if (game != null)
				game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
			heightmap = null;
		}

		void EnvVariableChanged(object sender, EnvVarEventArgs e) {
			if (e.Var == EnvVar.SunlightColour) {
				SetSun(game.World.Env.Sunlight);
			} else if (e.Var == EnvVar.ShadowlightColour) {
				SetShadow(game.World.Env.Shadowlight);
			}
		}
		
		void SetSun(FastColour col) {
			Outside = col.Pack();
			FastColour.GetShaded(col, out OutsideXSide, out OutsideZSide, out OutsideYBottom);
		}
		
		void SetShadow(FastColour col) {
			shadow = col.Pack();
			FastColour.GetShaded(col, out shadowXSide, out shadowZSide, out shadowYBottom);
		}
		
		
		public unsafe override void LightHint(int startX, int startZ, BlockID* mapPtr) {
			int x1 = Math.Max(startX, 0), x2 = Math.Min(width, startX + 18);
			int z1 = Math.Max(startZ, 0), z2 = Math.Min(length, startZ + 18);
			int xCount = x2 - x1, zCount = z2 - z1;
			int* skip = stackalloc int[xCount * zCount];
			
			int elemsLeft = InitialHeightmapCoverage(x1, z1, xCount, zCount, skip);
			if (!CalculateHeightmapCoverage(x1, z1, xCount, zCount, elemsLeft, skip, mapPtr)) {
				FinishHeightmapCoverage(x1, z1, xCount, zCount, skip);
			}
		}
		
		int GetLightHeight(int x, int z) {
			int index = (z * width) + x;
			int lightH = heightmap[index];
			return lightH == short.MaxValue ? CalcHeightAt(x, height - 1, z, index) : lightH;
		}
		
		
		// Outside colour is same as sunlight colour, so we reuse when possible
		public override bool IsLit(int x, int y, int z) {
			return y > GetLightHeight(x, z);
		}

		public override int LightCol(int x, int y, int z) {
			return y > GetLightHeight(x, z) ? Outside : shadow;
		}
		
		public override int LightCol_ZSide(int x, int y, int z) {
			return y > GetLightHeight(x, z) ? OutsideXSide : shadowXSide;
		}
		

		public override int LightCol_Sprite_Fast(int x, int y, int z) {
			return y > heightmap[(z * width) + x] ? Outside : shadow;
		}
		
		public override int LightCol_YTop_Fast(int x, int y, int z) {
			return y > heightmap[(z * width) + x] ? Outside : shadow;
		}
		
		public override int LightCol_YBottom_Fast(int x, int y, int z) {
			return y > heightmap[(z * width) + x] ? OutsideYBottom : shadowYBottom;
		}
		
		public override int LightCol_XSide_Fast(int x, int y, int z) {
			return y > heightmap[(z * width) + x] ? OutsideXSide : shadowXSide;
		}
		
		public override int LightCol_ZSide_Fast(int x, int y, int z) {
			return y > heightmap[(z * width) + x] ? OutsideZSide : shadowZSide;
		}
		
		
		public override void Refresh() {
			for (int i = 0; i < heightmap.Length; i++)
				heightmap[i] = short.MaxValue;
		}
	}
}
