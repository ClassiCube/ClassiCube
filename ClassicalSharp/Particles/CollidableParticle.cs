using System;
using OpenTK;

namespace ClassicalSharp.Particles {

	public abstract class CollidableParticle : Particle {
		
		protected bool hitTerrain = false;
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
			byte curBlock = game.Map.SafeGetBlock( (int)Position.X, (int)Position.Y, (int)Position.Z );
			float blockY = (int)Position.Y + game.BlockInfo.Height[curBlock];
			if( !CanPassThrough( curBlock ) && Position.Y < blockY ) return true;
			
			Velocity.Y -= gravity * (float)delta;
			int startY = (int)Math.Floor( Position.Y );
			Position += Velocity * (float)delta * 3;
			int endY = (int)Math.Floor( Position.Y );
			Utils.Clamp( ref Position.X, 0, game.Map.Width - 0.01f );
			Utils.Clamp( ref Position.Z, 0, game.Map.Length - 0.01f );
			
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
			
			byte block = game.Map.SafeGetBlock( (int)Position.X, y, (int)Position.Z );
			if( CanPassThrough( block ) ) return true;
			
			float collideY = y + (topFace ? game.BlockInfo.Height[block] : 0);
			bool collide = topFace ? (Position.Y < collideY) : (Position.Y > collideY);
			if( collide ) {
				float adjust = topFace ? Entity.Adjustment : -Entity.Adjustment;
				Position.Y = nextPos.Y = lastPos.Y = collideY + adjust;
				Velocity = Vector3.Zero;
				hitTerrain = true;
				return false;
			}
			return true;
		}
		
		bool CanPassThrough( byte block ) {
			return block == 0 || game.BlockInfo.IsSprite[block] || game.BlockInfo.IsLiquid[block];
		}
	}
}
