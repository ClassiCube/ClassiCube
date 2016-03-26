// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Particles {

	public sealed class TerrainParticle : CollidableParticle {
		
		static Vector2 terrainSize = new Vector2( 1/8f, 1/8f );
		internal TextureRec rec;
		internal int texLoc;
		
		public override bool Tick( Game game, double delta ) {
			return Tick( game, 5.4f, delta );
		}
		
		public override int Get1DBatch( Game game ) {
			return game.TerrainAtlas1D.Get1DIndex( texLoc );
		}
		
		public override void Render( Game game, double delta, float t,
		                            VertexPos3fTex2fCol4b[] vertices, ref int index ) {
			Position = Vector3.Lerp( lastPos, nextPos, t );
			Vector3 p111, p121, p212, p222;
			Utils.CalcBillboardPoints( terrainSize, Position, ref game.View,
			                          out p111, out p121, out p212, out p222 );
			World map = game.World;
			FastColour col = map.IsLit( Vector3I.Floor( Position ) ) ? map.Sunlight : map.Shadowlight;
			
			vertices[index++] = new VertexPos3fTex2fCol4b( p111, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( p121, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( p222, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( p212, rec.U2, rec.V2, col );
		}
	}
}
