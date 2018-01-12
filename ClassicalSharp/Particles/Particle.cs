// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Particles {

	public abstract class Particle {		
		public Vector3 Velocity;
		public float Lifetime;
		protected Vector3 lastPos, nextPos;
		protected bool hitTerrain = false;
		public byte Size;
		
		// http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/
		public static void DoRender(ref Matrix4 view, ref Vector2 size, ref Vector3 pos, ref TextureRec rec,
		                            int col, VertexP3fT2fC4b[] vertices, ref int index) {
			float sX = size.X * 0.5f, sY = size.Y * 0.5f;
			Vector3 centre = pos; centre.Y += sY;
			Vector3 a, b;
			
			a.X = view.Row0.X * sX; a.Y = view.Row1.X * sX; a.Z = view.Row2.X * sX; // right * size.X * 0.5f
			b.X = view.Row0.Y * sY; b.Y = view.Row1.Y * sY; b.Z = view.Row2.Y * sY; // up * size.Y * 0.5f
			VertexP3fT2fC4b v; v.Colour = col;
			
			v.X = centre.X - a.X - b.X; v.Y = centre.Y - a.Y - b.Y; v.Z = centre.Z - a.Z - b.Z;
			v.U = rec.U1; v.V = rec.V2; vertices[index++] = v;
			
			v.X = centre.X - a.X + b.X; v.Y = centre.Y - a.Y + b.Y; v.Z = centre.Z - a.Z + b.Z;
			              v.V = rec.V1; vertices[index++] = v;

			v.X = centre.X + a.X + b.X; v.Y = centre.Y + a.Y + b.Y; v.Z = centre.Z + a.Z + b.Z;
			v.U = rec.U2;               vertices[index++] = v;
			
			v.X = centre.X + a.X - b.X; v.Y = centre.Y + a.Y - b.Y; v.Z = centre.Z + a.Z - b.Z;
			              v.V = rec.V2; vertices[index++] = v;
		}
		
		public void ResetState(Vector3 pos, Vector3 velocity, double lifetime) {
			lastPos = nextPos = pos;
			Velocity = velocity;
			Lifetime = (float)lifetime;
			hitTerrain = false;
		}

		protected bool Tick(Game game, float gravity, bool throughLiquids, double delta) {
			hitTerrain = false;
			lastPos = nextPos;
			BlockID curBlock = GetBlock(game, (int)nextPos.X, (int)nextPos.Y, (int)nextPos.Z);
			float minY = Utils.Floor(nextPos.Y) + BlockInfo.MinBB[curBlock].Y;
			float maxY = Utils.Floor(nextPos.Y) + BlockInfo.MaxBB[curBlock].Y;			
			if (!CanPassThrough(game, curBlock, throughLiquids) && nextPos.Y >= minY && nextPos.Y < maxY && CollideHor(game, curBlock))
				return true;
			
			Velocity.Y -= gravity * (float)delta;
			int startY = (int)Math.Floor(nextPos.Y);
			nextPos += Velocity * (float)delta * 3;
			int endY = (int)Math.Floor(nextPos.Y);
			
			if (Velocity.Y > 0) {
				// don't test block we are already in
				for (int y = startY + 1; y <= endY && TestY(game, y, false, throughLiquids); y++);
			} else {
				for (int y = startY; y >= endY && TestY(game, y, true, throughLiquids); y--);
			}
			
			Lifetime -= (float)delta;
			return Lifetime < 0;
		}	
		
		bool TestY(Game game, int y, bool topFace, bool throughLiquids) {
			if (y < 0) {
				nextPos.Y = lastPos.Y = 0 + Entity.Adjustment;
				Velocity = Vector3.Zero;
				hitTerrain = true;
				return false;
			}
			
			BlockID block = GetBlock(game, (int)nextPos.X, y, (int)nextPos.Z);
			if (CanPassThrough(game, block, throughLiquids)) return true;
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
		
		bool CanPassThrough(Game game, BlockID block, bool throughLiquids) {
			byte draw = BlockInfo.Draw[block];
			return draw == DrawType.Gas || draw == DrawType.Sprite
				|| (throughLiquids && BlockInfo.IsLiquid[block]);
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
	
	public sealed class RainParticle : Particle {
		static TextureRec rec = new TextureRec(2/128f, 14/128f, 3/128f, 2/128f);
		
		public bool Tick(Game game, double delta) {
			bool dies = Tick(game, 3.5f, false, delta);
			return hitTerrain ? true : dies;
		}
		
		public void Render(Game game, float t, VertexP3fT2fC4b[] vertices, ref int index) {
			Vector3 pos = Vector3.Lerp(lastPos, nextPos, t);
			Vector2 size; size.X = Size * 0.015625f; size.Y = size.X;
			
			int x = Utils.Floor(pos.X), y = Utils.Floor(pos.Y), z = Utils.Floor(pos.Z);
			int col = game.World.IsValidPos(x, y, z) ? game.Lighting.LightCol(x, y, z) : game.Lighting.Outside;
			DoRender(ref game.Graphics.View, ref size, ref pos, ref rec, col, vertices, ref index);
		}
	}
	
	public sealed class TerrainParticle : Particle {
		internal TextureRec rec;
		internal byte texLoc;
		internal BlockID block;		
		
		public bool Tick(Game game, double delta) {
			return Tick(game, 5.4f, true, delta);
		}
		
		public void Render(Game game, float t, VertexP3fT2fC4b[] vertices, ref int index) {
			Vector3 pos = Vector3.Lerp(lastPos, nextPos, t);
			Vector2 size; size.X = Size * 0.015625f; size.Y = size.X;

			int col = FastColour.WhitePacked;
			if (!BlockInfo.FullBright[block]) {
				int x = Utils.Floor(pos.X), y = Utils.Floor(pos.Y), z = Utils.Floor(pos.Z);
				col = game.World.IsValidPos(x, y, z) ? game.Lighting.LightCol_ZSide(x, y, z) : game.Lighting.OutsideZSide;
			}
			
			if (BlockInfo.Tinted[block]) {
				FastColour fogCol = BlockInfo.FogColour[block];
				FastColour newCol = FastColour.Unpack(col);
				newCol *= fogCol;
				col = newCol.Pack();
			}
			DoRender(ref game.Graphics.View, ref size, ref pos, ref rec, col, vertices, ref index);
		}
	}
}