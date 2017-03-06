// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Map;
using OpenTK;
using ClassicalSharp.Entities;
using ClassicalSharp.Entities.Other;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Singleplayer {

	public class TNTPhysics {
		Game game;
		World map;
		PhysicsBase physics;
		Random rnd = new Random();
		
		public TNTPhysics(Game game, PhysicsBase physics) {
			this.game = game;
			map = game.World;
			this.physics = physics;
			physics.OnDelete[Block.TNT] = HandleTnt;
		}
		
		Vector3[] rayDirs;
		const float stepLen = 0.3f;
		float[] hardness;
		
		void HandleTnt(int index, BlockID block) {
			float x = (index % map.Width) + 0.5f;
			float z = ((index / map.Width) % map.Length) + 0.5f;
			float y = ((index / map.Width) / map.Length);
			
			TntEntity TNT = new TntEntity(game, -1, 4);
			Vector3 pos = new Vector3(x, y, z);
			TNT.SetLocation(LocationUpdate.MakePos(pos, false), false);
			var ent = game.Entities;
			var entLen = game.Entities.Entities.Length;
			for(int i = entLen  - 1; i >= 0; i--) {
				if(ent[i] == null) {
					ent[i] = TNT;
					break;
				}
			}
		}
		
		void HandleTnt2(int index, BlockID block) {
			float x = (index % map.Width) + 0.5f;
			float z = ((index / map.Width) % map.Length) + 0.5f;
			float y = ((index / map.Width) / map.Length);
			short intensity = (short)RandomNumber(10, 18);
			
			TntEntity TNT = new TntEntity(game, intensity, 4);
			Vector3 pos = new Vector3(x, y, z);
			TNT.SetLocation(LocationUpdate.MakePos(pos, false), false);
			var ent = game.Entities;
			var entLen = game.Entities.Entities.Length;
			for(int i = entLen  - 1; i >= 0; i--) {
				if(ent[i] == null) {
					ent[i] = TNT;
					break;
				}
			}
		}
		
		// Algorithm source: http://minecraft.gamepedia.com/Explosion
		public void Explode(float power, int x, int y, int z) {
			if (rayDirs == null)
				InitExplosionCache();
			
			game.UpdateBlock(x, y, z, Block.Air);
			int index = (y * map.Length + z) * map.Width + x;
			physics.ActivateNeighbours(x, y, z, index);
			
			Vector3 basePos = new Vector3(x, y, z);
			for (int i = 0; i < rayDirs.Length; i++) {
				Vector3 dir = rayDirs[i] * stepLen;
				Vector3 position = basePos;
				float intensity = (float)(0.7 + rnd.NextDouble() * 0.6) * power;
				while (intensity > 0) {
					position += dir;
					intensity -= stepLen * 0.75f;
					Vector3I P = Vector3I.Floor(position);
					if (!map.IsValidPos(P)) break;
					
					int newX = P.X;
					int newY = P.Y;
					int newZ = P.Z;
					bool isTNT = false;
					int newIndex = (newY * game.World.Length + newZ) * game.World.Width + newX;
					if (map.blocks[newIndex] == Block.TNT) isTNT = true;
					
					BlockID block = map.GetBlock(P);
					intensity -= (hardness[block] / 5 + 0.3f) * stepLen;
					if (intensity > 0 && block != 0) {
						game.UpdateBlock(P.X, P.Y, P.Z, Block.Air);
						index = (P.Y * map.Length + P.Z) * map.Width + P.X;
						physics.ActivateNeighbours(P.X, P.Y, P.Z, index);
						if (isTNT) HandleTnt2(newIndex, block);
					}
				}
			}
		}
		
		void InitExplosionCache() {
			hardness = new float[] { 0, 30, 3, 2.5f, 30, 15, 0, 1.8E+07f, 500, 500, 500, 500, 2.5f,
				3, 15, 15, 15, 10, 1, 3, 1.5f, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0,
				0, 0, 30, 30, 30, 30, 30, 0, 7.5f, 30, 6000, 30, 0, 4, 0.5f, 0, 4, 4, 4, 4, 4, 2.5f,
				// Note that the 30, 500, 15, 15 are guesses (CeramicTile --> Crate)
				30, 500, 15, 15, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			rayDirs = new Vector3[1352];
			int index = 0;
			
			// Y bottom and top planes
			for (float x = -1; x <= 1.001f; x += 2f/15) {
				for (float z = -1; z <= 1.001f; z += 2f/15) {
					rayDirs[index++] = Normalise(x, -1, z);
					rayDirs[index++] = Normalise(x, +1, z);
				}
			}
			// Z planes
			for (float y = -1 + 2f/15; y <= 1.001f - 2f/15; y += 2f/15) {
				for (float x = -1; x <= 1.001f; x += 2f/15) {
					rayDirs[index++] = Normalise(x, y, -1);
					rayDirs[index++] = Normalise(x, y, +1);
				}
			}
			// X planes
			for (float y = -1 + 2f/15; y <= 1.001f - 2f/15; y += 2f/15) {
				for (float z = -1 + 2f/15; z <= 1.001f - 2f/15; z += 2f/15) {
					rayDirs[index++] = Normalise(-1, y, z);
					rayDirs[index++] = Normalise(+1, y, z);
				}
			}
		}
		
		static Vector3 Normalise(float x, float y, float z) {
			float scale = 1f / (float)Math.Sqrt(x * x + y * y + z * z);
			return new Vector3(x * scale, y * scale, z * scale);
		}
		
		private static readonly Random random = new Random();
		private static readonly object syncLock = new object();
		public static int RandomNumber(int min, int max)
		{
    		lock(syncLock) { // synchronize
        		return random.Next(min, max);
    		}
		}
	}
}
