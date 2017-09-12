// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Particles {

	public abstract class CollidableParticle : Particle {
		
		protected bool hitTerrain = false, throughLiquids = true;
		
		public void ResetState(Vector3 pos, Vector3 velocity, double lifetime) {
			lastPos = nextPos = pos;
			Velocity = velocity;
			Lifetime = (float)lifetime;
			hitTerrain = false;
		}

		protected bool Tick(Game game, float gravity, double delta) {
			hitTerrain = false;
			lastPos = nextPos;
			BlockID curBlock = GetBlock(game, (int)nextPos.X, (int)nextPos.Y, (int)nextPos.Z);
			float minY = Utils.Floor(nextPos.Y) + BlockInfo.MinBB[curBlock].Y;
			float maxY = Utils.Floor(nextPos.Y) + BlockInfo.MaxBB[curBlock].Y;			
			if (!CanPassThrough(game, curBlock) && nextPos.Y >= minY && nextPos.Y < maxY && CollideHor(game, curBlock))
				return true;
			
			Velocity.Y -= gravity * (float)delta;
			int startY = (int)Math.Floor(nextPos.Y);
			nextPos += Velocity * (float)delta * 3;
			int endY = (int)Math.Floor(nextPos.Y);
			
			if (Velocity.Y > 0) {
				// don't test block we are already in
				for (int y = startY + 1; y <= endY && TestY(game, y, false); y++);
			} else {
				for (int y = startY; y >= endY && TestY(game, y, true); y--);
			}
			return base.Tick(game, delta);
		}	
		
		bool TestY(Game game, int y, bool topFace) {
			if (y < 0) {
				nextPos.Y = lastPos.Y = 0 + Entity.Adjustment;
				Velocity = Vector3.Zero;
				hitTerrain = true;
				return false;
			}
			
			BlockID block = GetBlock(game, (int)nextPos.X, y, (int)nextPos.Z);
			if (CanPassThrough(game, block)) return true;
			Vector3 minBB = BlockInfo.MinBB[block];
			Vector3 maxBB = BlockInfo.MaxBB[block];
			float collideY = y + (topFace ? maxBB.Y : minBB.Y);
			bool collideVer = topFace ? (nextPos.Y < collideY) : (nextPos.Y > collideY);
			
			if (collideVer && CollideHor(game, block)) {
				float adjust = topFace ? Entity.Adjustment : -Entity.Adjustment;
				nextPos.Y = lastPos.Y = collideY + adjust;
				Velocity = Vector3.Zero;
				hitTerrain = true;
				return false;
			}
			return true;
		}
		
		bool CanPassThrough(Game game, BlockID block) {
			byte draw = BlockInfo.Draw[block];
			return draw == DrawType.Gas || draw == DrawType.Sprite
				|| (throughLiquids && BlockInfo.IsLiquid(block));
		}
		
		bool CollideHor(Game game, BlockID block) {
			Vector3 min = BlockInfo.MinBB[block] + FloorHor(nextPos);
			Vector3 max = BlockInfo.MaxBB[block] + FloorHor(nextPos);
			return nextPos.X >= min.X && nextPos.Z >= min.Z &&
				nextPos.X < max.X && nextPos.Z < max.Z;
		}
		
		BlockID GetBlock(Game game, int x, int y, int z) {
			if (game.World.IsValidPos(x, y, z))
				return game.World.GetBlock(x, y, z);
			
			if (y >= game.World.Env.EdgeHeight) return Block.Air;
			if (y >= game.World.Env.SidesHeight) return game.World.Env.EdgeBlock;
			return game.World.Env.SidesBlock;
		}
		
		static Vector3 FloorHor(Vector3 v) {
			return new Vector3(Utils.Floor(v.X), 0, Utils.Floor(v.Z));
		}
	}
}
