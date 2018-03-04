// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using ClassicalSharp.Physics;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Renderers {
	public abstract class EnvRenderer : IGameComponent {
		
		protected World map;
		protected Game game;
		protected internal bool legacy, minimal;
		
		public virtual void Init(Game game) {
			this.game = game;
			map = game.World;
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
		}
		
		public virtual void UseLegacyMode(bool legacy) { }
		
		/// <summary> Sets mode to minimal environment rendering.
		/// - only sets the background/clear colour to blended fog colour.
		/// (no smooth fog, clouds, or proper overhead sky) </summary>
		public virtual void UseMinimalMode(bool minimal) { }
		
		public void Ready(Game game) { }
		public virtual void Reset(Game game) { OnNewMap(game); }
		
		public abstract void OnNewMap(Game game);
		
		public abstract void OnNewMapLoaded(Game game);
		
		public virtual void Dispose() {
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
		}
		
		public abstract void Render(double deltaTime);
		
		protected abstract void EnvVariableChanged(object sender, EnvVarEventArgs e);
		
		
		protected void BlockOn(out float fogDensity, out FastColour fogCol) {
			Vector3 pos = game.CurrentCameraPos;
			Vector3I coords = Vector3I.Floor(pos);
			
			BlockID block = game.World.SafeGetBlock(coords);
			AABB blockBB = new AABB(
				(Vector3)coords + BlockInfo.MinBB[block],
				(Vector3)coords + BlockInfo.MaxBB[block]);
			
			if (blockBB.Contains(pos) && BlockInfo.FogDensity[block] != 0) {
				fogDensity = BlockInfo.FogDensity[block];
				fogCol = BlockInfo.FogColour[block];
			} else {
				fogDensity = 0;
				// Blend fog and sky together
				float blend = (float)BlendFactor(game.ViewDistance);
				fogCol = FastColour.Lerp(map.Env.FogCol, map.Env.SkyCol, blend);
			}
		}
		
		double BlendFactor(float x) {
			//return -0.05 + 0.22 * (0.25 * Math.Log(x));
			double blend = -0.13 + 0.28 * (0.25 * Math.Log(x));
			if (blend < 0) blend = 0;
			if (blend > 1) blend = 1;
			return blend;
		}
	}
}
