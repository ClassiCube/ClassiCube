// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Particles {

	public sealed class RainParticle : CollidableParticle {
		
		static Vector2 bigSize = new Vector2( 1/16f, 1/16f );
		static Vector2 smallSize = new Vector2( 0.75f/16f, 0.75f/16f );
		static Vector2 tinySize = new Vector2( 0.5f/16f, 0.5f/16f );
		static TextureRec rec = new TextureRec( 2/128f, 14/128f, 3/128f, 2/128f );
		
		public RainParticle() { throughLiquids = false; }
		
		public bool Big, Tiny;
		
		public override bool Tick( Game game, double delta ) {
			bool dies = Tick( game, 3.5f, delta );
			return hitTerrain ? true : dies;
		}
		
		public override int Get1DBatch( Game game ) { return 0; }
		
		public override void Render( Game game, double delta, float t,
		                            VertexP3fT2fC4b[] vertices, ref int index ) {
			Position = Vector3.Lerp( lastPos, nextPos, t );
			Vector3 p111, p121, p212, p222;
			Vector2 size = Big ? bigSize : (Tiny ? tinySize : smallSize );
			Utils.CalcBillboardPoints( size, Position, ref game.View,
			                          out p111, out p121, out p212, out p222 );
			World map = game.World;
			FastColour col = map.IsLit( Vector3I.Floor( Position ) ) ? map.Sunlight : map.Shadowlight;
			
			vertices[index++] = new VertexP3fT2fC4b( p111, rec.U1, rec.V2, col );
			vertices[index++] = new VertexP3fT2fC4b( p121, rec.U1, rec.V1, col );
			vertices[index++] = new VertexP3fT2fC4b( p222, rec.U2, rec.V1, col );
			vertices[index++] = new VertexP3fT2fC4b( p212, rec.U2, rec.V2, col );
		}
	}
}
