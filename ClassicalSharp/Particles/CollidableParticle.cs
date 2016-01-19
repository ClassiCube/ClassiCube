using System;
using OpenTK;

namespace ClassicalSharp.Particles {

	public abstract class CollidableParticle : Particle {
		
		protected bool hitTerrain = false, throughLiquids = true;
		public CollidableParticle( Game game ) : base( game) { }
		
		public void ResetState( Vector3 pos, Vector3 velocity, double lifetime ) {
			Position = lastPos = nextPos = pos;
			Velocity = velocity;
			Lifetime = (float)lifetime;
			hitTerrain = false;
		}

		protected bool Tick( float gravity, double delta ) {
			hitTerrain = false;
			lastPos = Position = nextPos;
			byte curBlock = GetBlock( (int)Position.X, (int)Position.Y, (int)Position.Z );
			float minY = Utils.Floor( Position.Y ) + game.BlockInfo.MinBB[curBlock].Y;
			float maxY = Utils.Floor( Position.Y ) + game.BlockInfo.MaxBB[curBlock].Y;			
			if( !CanPassThrough( curBlock ) && Position.Y >= minY && 
			   Position.Y < maxY && CollideHor( curBlock ) )
				return true;
			
			Velocity.Y -= gravity * (float)delta;
			int startY = (int)Math.Floor( Position.Y );
			Position += Velocity * (float)delta * 3;
			int endY = (int)Math.Floor( Position.Y );
			
			if( Velocity.Y > 0 ) {
				// don't test block we are already in
				for( int y = startY + 1; y <= endY && TestY( y, false ); y++ );
			} else {
				for( int y = startY; y >= endY && TestY( y, true ); y-- );
			}
			nextPos = Position;
			Position = lastPos;
			return base.Tick( delta );
		}	
		
		bool TestY( int y, bool topFace ) {
			if( y < 0 ) {
				Position.Y = nextPos.Y = lastPos.Y = 0 + Entity.Adjustment;
				Velocity = Vector3.Zero;
				hitTerrain = true;
				return false;
			}
			
			byte block = GetBlock( (int)Position.X, y, (int)Position.Z );
			if( CanPassThrough( block ) ) return true;
			Vector3 minBB = game.BlockInfo.MinBB[block];
			Vector3 maxBB = game.BlockInfo.MaxBB[block];
			float collideY = y + (topFace ? maxBB.Y : minBB.Y);
			bool collideVer = topFace ? (Position.Y < collideY) : (Position.Y > collideY);
			
			if( collideVer && CollideHor( block ) ) {
				float adjust = topFace ? Entity.Adjustment : -Entity.Adjustment;
				Position.Y = nextPos.Y = lastPos.Y = collideY + adjust;
				Velocity = Vector3.Zero;
				hitTerrain = true;
				return false;
			}
			return true;
		}
		
		bool CanPassThrough( byte block ) {
			return game.BlockInfo.IsAir[block] || game.BlockInfo.IsSprite[block] 
				|| (throughLiquids && game.BlockInfo.IsLiquid[block]);
		}
		
		bool CollideHor( byte block ) {
			Vector3 min = game.BlockInfo.MinBB[block] + Floor( Position );
			Vector3 max = game.BlockInfo.MaxBB[block] + Floor( Position );
			return Position.X >= min.X && Position.Z >= min.Z &&
				Position.X < max.X && Position.Z < max.Z;
		}
		
		static Vector3 Floor( Vector3 v ) {
			return new Vector3( Utils.Floor( v.X ), 0, Utils.Floor( v.Z ) );
		}
		
		byte GetBlock( int x, int y, int z ) {
			if( game.Map.IsValidPos( x, y, z ) )
				return game.Map.GetBlock( x, y, z );
			
			if( y >= game.Map.EdgeHeight ) return (byte)Block.Air;
			if( y >= game.Map.SidesHeight ) return (byte)game.Map.EdgeBlock;
			return (byte)game.Map.SidesBlock;
		}
	}
}
