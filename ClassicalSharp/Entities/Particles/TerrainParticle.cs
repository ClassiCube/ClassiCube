using System;
using OpenTK;

namespace ClassicalSharp.Particles {

	public sealed class TerrainParticle : Particle {
		
		const float gravity = 2.4f;
		double maxY = 0;
		
		public TerrainParticle( Game game, Vector3 pos, Vector3 velocity, double lifetime, TextureRectangle rec )
			: base( game, pos, velocity, lifetime, rec ) {		
			maxY = Position.Y;
		}
		
		public override void Render( double delta, float t, VertexPos3fTex2f[] vertices, ref int index ) {
			Position = Vector3.Lerp( lastPos, nextPos, t );
			Vector3 p111, p121, p212, p222;
			TranslatePoints( out p111, out p121, out p212, out p222 );
			
			vertices[index++] = new VertexPos3fTex2f( p111, Rectangle.U1, Rectangle.V2 );
			vertices[index++] = new VertexPos3fTex2f( p121, Rectangle.U1, Rectangle.V1 );
			vertices[index++] = new VertexPos3fTex2f( p222, Rectangle.U2, Rectangle.V1 );
			
			vertices[index++] = new VertexPos3fTex2f( p222, Rectangle.U2, Rectangle.V1 );
			vertices[index++] = new VertexPos3fTex2f( p212, Rectangle.U2, Rectangle.V2 );
			vertices[index++] = new VertexPos3fTex2f( p111, Rectangle.U1, Rectangle.V2 );
		}

		public override bool Tick( double delta ) {
			lastPos = Position = nextPos;
			Velocity.Y -= gravity * (float)delta;
			int startY = (int)Math.Floor( Position.Y );
			Position += Velocity * (float)delta;
			int endY = (int)Math.Floor( Position.Y );			
			Utils.Clamp( ref Position.X, 0, game.Map.Width - 0.01f );
			Utils.Clamp( ref Position.Z, 0, game.Map.Length - 0.01f );
			
			if( endY <= startY ) {
				for( int y = startY; y >= endY; y-- ) {
					if( y < 0 ) {
						return CollideWithGround( 0 ) ? true : base.Tick( delta );
					}
					byte block = GetBlock( (int)Position.X, y, (int)Position.Z );
					if( block == 0 || game.BlockInfo.IsSprite( block ) || game.BlockInfo.IsLiquid( block ) )
						continue;
					
					float groundHeight = y + game.BlockInfo.BlockHeight( block );				
					if( Position.Y < groundHeight ) {
						return CollideWithGround( groundHeight ) ? true : base.Tick( delta );
					}
				}
			}
			nextPos = Position;
			Position = lastPos;
			return base.Tick( delta );
		}
		
		byte GetBlock( int x, int y, int z ) {
			// If particles are spawned at the top of the map, they can occasionally
			// go outside the top of the map. This is okay, so handle this case.
			if( y >= game.Map.Height ) return 0;
			return game.Map.GetBlock( x, y, z );
		}
		
		bool CollideWithGround( float y ) {
			if( y > maxY ) {
				// prevent the particle teleporting up when a block is 
				// placed on top of the particle, simply die instead.
				return true;
			}
			Position.Y = y;
			maxY = y;
			Velocity = Vector3.Zero;
			nextPos = Position;
			Position = lastPos;
			return false;
		}
		
		public override void Dispose() {
		}
	}
}
