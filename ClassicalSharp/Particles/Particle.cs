// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Particles {

	public abstract class Particle {
		
		public Vector3 Position;
		public Vector3 Velocity;
		public float Lifetime;
		protected Vector3 lastPos, nextPos;

		public abstract int Get1DBatch( Game game );
		
		public abstract void Render( Game game, double delta, float t,
		                            VertexP3fT2fC4b[] vertices, ref int index );
		
		protected void DoRender( Game game, ref Vector2 size, ref TextureRec rec,
		                        int col, VertexP3fT2fC4b[] vertices, ref int index ) {
			Vector3 p111, p121, p212, p222;
			Utils.CalcBillboardPoints( size, Position, ref game.View,
			                          out p111, out p121, out p212, out p222 );

			vertices[index++] = new VertexP3fT2fC4b( ref p111, rec.U1, rec.V2, col );
			vertices[index++] = new VertexP3fT2fC4b( ref p121, rec.U1, rec.V1, col );
			vertices[index++] = new VertexP3fT2fC4b( ref p222, rec.U2, rec.V1, col );
			vertices[index++] = new VertexP3fT2fC4b( ref p212, rec.U2, rec.V2, col );
		}
		
		public virtual bool Tick( Game game, double delta ) {
			Lifetime -= (float)delta;
			return Lifetime < 0;
		}
	}
	
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
			Vector2 size = Big ? bigSize : (Tiny ? tinySize : smallSize);
			
			World map = game.World;
			int col = map.IsLit( Position ) ? map.Env.Sun : map.Env.Shadow;
			DoRender( game, ref size, ref rec, col, vertices, ref index );
		}
	}
	
	public sealed class TerrainParticle : CollidableParticle {
		
		static Vector2 terrainSize = new Vector2( 1/8f, 1/8f );
		internal TextureRec rec;
		internal ushort flags; // lower 8 bits for tex location, next bit for fullbright
		
		public override bool Tick( Game game, double delta ) {
			return Tick( game, 5.4f, delta );
		}
		
		public override int Get1DBatch( Game game ) {
			return game.TerrainAtlas1D.Get1DIndex( flags & 0xFF );
		}
		
		public override void Render( Game game, double delta, float t,
		                            VertexP3fT2fC4b[] vertices, ref int index ) {
			Position = Vector3.Lerp( lastPos, nextPos, t );
			
			World map = game.World;
			bool fullBright = (flags & 0x100) != 0;
			int col = fullBright ? FastColour.WhitePacked
				: (map.IsLit( Position ) ? map.Env.SunZSide : map.Env.ShadowZSide);
			DoRender( game, ref terrainSize, ref rec, col, vertices, ref index );
		}
	}
}