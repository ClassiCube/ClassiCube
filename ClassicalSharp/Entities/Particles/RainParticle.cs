using System;
using OpenTK;

namespace ClassicalSharp.Particles {

	public sealed class RainParticle : CollidableParticle {
		
		static Vector2 rainSize = new Vector2( 1/8f, 1/8f );
		static TextureRec rec = new TextureRec( 2/128f, 14/128f, 3/128f, 2/128f );
		public RainParticle( Game game, Vector3 pos, Vector3 velocity, double lifetime )
			: base( game, pos, velocity, lifetime ) {
		}
		
		public override void Render( double delta, float t, VertexPos3fTex2fCol4b[] vertices, ref int index ) {
			Position = Vector3.Lerp( lastPos, nextPos, t );
			Vector3 p111, p121, p212, p222;
			Utils.CalcBillboardPoints( rainSize, Position, ref game.View,
			                          out p111, out p121, out p212, out p222 );
			Map map = game.Map;
			FastColour col = map.IsLit( Vector3I.Floor( Position ) ) ? map.Sunlight : map.Shadowlight;
			
			vertices[index++] = new VertexPos3fTex2fCol4b( p111, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( p121, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( p222, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( p212, rec.U2, rec.V2, col );
		}
	}
}
