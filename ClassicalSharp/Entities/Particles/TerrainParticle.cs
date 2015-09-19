using System;
using OpenTK;

namespace ClassicalSharp.Particles {

	public sealed class TerrainParticle : Particle {
		
		const float gravity = 2.4f;
		
		public TerrainParticle( Game game, Vector3 pos, Vector3 velocity, double lifetime, TextureRectangle rec )
			: base( game, pos, velocity, lifetime, rec ) {
		}
		
		public override void Render( double delta, float t, VertexPos3fTex2fCol4b[] vertices, ref int index ) {
			Position = Vector3.Lerp( lastPos, nextPos, t );
			Vector3 p111, p121, p212, p222;
			TranslatePoints( out p111, out p121, out p212, out p222 );
			Map map = game.Map;
			FastColour col = map.IsLit( Vector3I.Floor( Position ) ) ? map.Sunlight : map.Shadowlight;
			
			vertices[index++] = new VertexPos3fTex2fCol4b( p111, Rectangle.U1, Rectangle.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( p121, Rectangle.U1, Rectangle.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( p222, Rectangle.U2, Rectangle.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( p212, Rectangle.U2, Rectangle.V2, col );
		}

		public override bool Tick( double delta ) {
			lastPos = Position = nextPos;
			byte curBlock = GetBlockSafe( (int)Position.X, (int)Position.Y, (int)Position.Z );
			if( !CanPassThrough( curBlock ) ) return true;
			
			Velocity.Y -= gravity * (float)delta;
			int startY = (int)Math.Floor( Position.Y );
			Position += Velocity * (float)delta;
			int endY = (int)Math.Floor( Position.Y );
			Utils.Clamp( ref Position.X, 0, game.Map.Width - 0.01f );
			Utils.Clamp( ref Position.Z, 0, game.Map.Length - 0.01f );
			
			if( Velocity.Y > 0 ) {
				for( int y = startY; y <= endY && TestY( y, false ); y++ );
			} else {
				for( int y = startY; y >= endY && TestY( y, true ); y-- );
			}
			nextPos = Position;
			Position = lastPos;
			return base.Tick( delta );
		}
		
		byte GetBlockSafe( int x, int y, int z ) {
			if( !game.Map.IsValidPos( x, y, z ) ) return 0;
			return game.Map.GetBlock( x, y, z );
		}
		
		bool TestY( int y, bool topFace ) {
			if( y < 0 ) {
				Position.Y = nextPos.Y = lastPos.Y = 0 + Entity.Adjustment;
				Velocity = Vector3.Zero;
				return false;
			}
			
			byte block = GetBlockSafe( (int)Position.X, y, (int)Position.Z );
			if( CanPassThrough( block ) ) return true;
			
			float collideY = y;
			if( topFace )
				collideY += game.BlockInfo.BlockHeight( block );
			
			bool collide = topFace ? (Position.Y < collideY) : (Position.Y > collideY );
			if( collide ) {
				float adjust = topFace ? Entity.Adjustment : -Entity.Adjustment;
				Position.Y = nextPos.Y = lastPos.Y = collideY + adjust;
				Velocity = Vector3.Zero;
				return false;
			}
			return true;
		}
		
		bool CanPassThrough( byte block ) {
			return block == 0 || game.BlockInfo.IsSprite( block ) || game.BlockInfo.IsLiquid( block );
		}
		
		public override void Dispose() {
		}
	}
}
