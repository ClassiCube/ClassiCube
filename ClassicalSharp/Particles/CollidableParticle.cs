// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using OpenTK;

namespace ClassicalSharp.Particles {

	public abstract class CollidableParticle : Particle {
		
		protected bool hitTerrain = false, throughLiquids = true;
		
		public void ResetState( Vector3 pos, Vector3 velocity, double lifetime ) {
			Position = lastPos = nextPos = pos;
			Velocity = velocity;
			Lifetime = (float)lifetime;
			hitTerrain = false;
		}

		protected bool Tick( Game game, float gravity, double delta ) {
			hitTerrain = false;
			lastPos = Position = nextPos;
			byte curBlock = GetBlock( game, (int)Position.X, (int)Position.Y, (int)Position.Z );
			float minY = Utils.Floor( Position.Y ) + game.BlockInfo.MinBB[curBlock].Y;
			float maxY = Utils.Floor( Position.Y ) + game.BlockInfo.MaxBB[curBlock].Y;			
			if( !CanPassThrough( game, curBlock ) && Position.Y >= minY && 
			   Position.Y < maxY && CollideHor( game, curBlock ) )
				return true;
			
			Velocity.Y -= gravity * (float)delta;
			int startY = (int)Math.Floor( Position.Y );
			Position += Velocity * (float)delta * 3;
			int endY = (int)Math.Floor( Position.Y );
			
			if( Velocity.Y > 0 ) {
				// don't test block we are already in
				for( int y = startY + 1; y <= endY && TestY( game, y, false ); y++ );
			} else {
				for( int y = startY; y >= endY && TestY( game, y, true ); y-- );
			}
			nextPos = Position;
			Position = lastPos;
			return base.Tick( game, delta );
		}	
		
		bool TestY( Game game, int y, bool topFace ) {
			if( y < 0 ) {
				Position.Y = nextPos.Y = lastPos.Y = 0 + Entity.Adjustment;
				Velocity = Vector3.Zero;
				hitTerrain = true;
				return false;
			}
			
			byte block = GetBlock( game, (int)Position.X, y, (int)Position.Z );
			if( CanPassThrough( game, block ) ) return true;
			Vector3 minBB = game.BlockInfo.MinBB[block];
			Vector3 maxBB = game.BlockInfo.MaxBB[block];
			float collideY = y + (topFace ? maxBB.Y : minBB.Y);
			bool collideVer = topFace ? (Position.Y < collideY) : (Position.Y > collideY);
			
			if( collideVer && CollideHor( game, block ) ) {
				float adjust = topFace ? Entity.Adjustment : -Entity.Adjustment;
				Position.Y = nextPos.Y = lastPos.Y = collideY + adjust;
				Velocity = Vector3.Zero;
				hitTerrain = true;
				return false;
			}
			return true;
		}
		
		bool CanPassThrough( Game game, byte block ) {
			return game.BlockInfo.IsAir[block] || game.BlockInfo.IsSprite[block] 
				|| (throughLiquids && game.BlockInfo.IsLiquid[block]);
		}
		
		bool CollideHor( Game game, byte block ) {
			Vector3 min = game.BlockInfo.MinBB[block] + FloorHor( Position );
			Vector3 max = game.BlockInfo.MaxBB[block] + FloorHor( Position );
			return Position.X >= min.X && Position.Z >= min.Z &&
				Position.X < max.X && Position.Z < max.Z;
		}
		
		byte GetBlock( Game game, int x, int y, int z ) {
			if( game.World.IsValidPos( x, y, z ) )
				return game.World.GetBlock( x, y, z );
			
			if( y >= game.World.Env.EdgeHeight ) return Block.Air;
			if( y >= game.World.Env.SidesHeight ) return game.World.Env.EdgeBlock;
			return game.World.Env.SidesBlock;
		}
		
		static Vector3 FloorHor( Vector3 v ) {
			return new Vector3( Utils.Floor( v.X ), 0, Utils.Floor( v.Z ) );
		}
	}
}
