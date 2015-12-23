using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Particles {
	
	public class ParticleManager : IDisposable {
		
		public int ParticlesTexId;
		TerrainParticle[] terrainParticles = new TerrainParticle[maxParticles];
		RainParticle[] rainParticles = new RainParticle[maxParticles];
		int terrainCount, rainCount;
		
		Game game;
		Random rnd;
		int vb;
		const int maxParticles = 600;
		
		public ParticleManager( Game game ) {
			this.game = game;
			rnd = new Random();
			vb = game.Graphics.CreateDynamicVb( VertexFormat.Pos3fTex2fCol4b, maxParticles * 4 );
		}
		
		VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[0];
		public void Render( double delta, float t ) {
			if( terrainCount == 0 && rainCount == 0 ) return;
			IGraphicsApi graphics = game.Graphics;
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			graphics.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			
			int count = RenderParticles( terrainParticles, terrainCount, delta, t );
			if( count > 0 ) {
				graphics.BindTexture( game.TerrainAtlas.TexId );
				graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, vertices, count, count * 6 / 4 );
			}
			count = RenderParticles( rainParticles, rainCount, delta, t );
			if( count > 0 ) {
				graphics.BindTexture( ParticlesTexId );
				graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, vertices, count, count * 6 / 4 );
			}
			
			graphics.AlphaTest = false;
			graphics.Texturing = false;
		}
		
		int RenderParticles( Particle[] particles, int elems, double delta, float t ) {
			int count = elems * 4;
			if( count > vertices.Length )
				vertices = new VertexPos3fTex2fCol4b[count];
			
			int index = 0;
			for( int i = 0; i < elems; i++ )
				particles[i].Render( delta, t, vertices, ref index );
			return Math.Min( count, maxParticles * 4 );
		}
		
		public void Tick( double delta ) {
			TickParticles( terrainParticles, ref terrainCount, delta );
			TickParticles( rainParticles, ref rainCount, delta );
		}
		
		void TickParticles( Particle[] particles, ref int count, double delta ) {
			for( int i = 0; i < count; i++ ) {
				Particle particle = particles[i];
				if( particle.Tick( delta ) ) {
					RemoveAt( i, particles, ref count );
					i--;
				}
			}
		}
		
		public void Dispose() {
			game.Graphics.DeleteDynamicVb( vb );
			game.Graphics.DeleteTexture( ref ParticlesTexId );
		}
		
		public void BreakBlockEffect( Vector3I position, byte block ) {
			Vector3 startPos = new Vector3( position.X, position.Y, position.Z );
			int texLoc = game.BlockInfo.GetTextureLoc( block, TileSide.Left );
			TextureRec baseRec = game.TerrainAtlas.GetTexRec( texLoc );			
			const float uvScale = (1/16f) * TerrainAtlas2D.invElementSize;
			const float elemSize = 4 * uvScale;
			
			Vector3 minBB = game.BlockInfo.MinBB[block];
			Vector3 maxBB = game.BlockInfo.MaxBB[block];
			int minU = Math.Min( (int)(minBB.X * 16), (int)(minBB.Z * 16) );
			int maxU = Math.Min( (int)(maxBB.X * 16), (int)(maxBB.Z * 16) );
			int minV = (int)(minBB.Y * 16), maxV = (int)(maxBB.Y * 16);
			// This way we can avoid creating particles which outside the bounds and need to be clamped
			if( minU < 13 && maxU > 13 ) maxU = 13;
			if( minV < 13 && maxV > 13 ) maxV = 13;
			
			for( int i = 0; i < 25; i++ ) {
				double velX = rnd.NextDouble() * 0.8 - 0.4; // [-0.4, 0.4]
				double velZ = rnd.NextDouble() * 0.8 - 0.4;
				double velY = rnd.NextDouble() + 0.2;
				Vector3 velocity = new Vector3( (float)velX, (float)velY, (float)velZ );
				
				double xOffset = rnd.NextDouble() - 0.5; // [-0.5, 0.5]
				double yOffset = (rnd.NextDouble() - 0.125) * maxBB.Y;
				double zOffset = rnd.NextDouble() - 0.5;
				Vector3 pos = startPos + new Vector3( 0.5f + (float)xOffset,
				                                     (float)yOffset, 0.5f + (float)zOffset );
				
				TextureRec rec = baseRec;
				rec.U1 = baseRec.U1 + rnd.Next( minU, maxU ) * uvScale;
				rec.V1 = baseRec.V1 + rnd.Next( minV, maxV ) * uvScale;
				rec.U2 = Math.Min( baseRec.U1 + maxU * uvScale, rec.U1 + elemSize );
				rec.V2 = Math.Min( baseRec.V1 + maxV * uvScale, rec.V1 + elemSize );
				double life = 1.5 - rnd.NextDouble();
				
				TerrainParticle p = AddParticle( terrainParticles, ref terrainCount, false );
				p.ResetState( pos, velocity, life );
				p.rec = rec;
			}
		}
		
		public void AddRainParticle( Vector3 pos ) {
			Vector3 startPos = pos;			
			for( int i = 0; i < 2; i++ ) {
				double velX = rnd.NextDouble() * 0.8 - 0.4; // [-0.4, 0.4]
				double velZ = rnd.NextDouble() * 0.8 - 0.4;
				double velY = rnd.NextDouble() + 0.4;
				Vector3 velocity = new Vector3( (float)velX, (float)velY, (float)velZ );
				
				double xOffset = rnd.NextDouble() - 0.5; // [-0.5, 0.5]
				double yOffset = rnd.NextDouble() * 0.1 + 0.01;
				double zOffset = rnd.NextDouble() - 0.5;
				pos = startPos + new Vector3( 0.5f + (float)xOffset,
				                                     (float)yOffset, 0.5f + (float)zOffset );
				double life = 40;
				RainParticle p = AddParticle( rainParticles, ref rainCount, true );
				p.ResetState( pos, velocity, life );
				p.Big = rnd.Next( 0, 20 ) >= 18;
				p.Tiny = rnd.Next( 0, 30 ) >= 28;
			}
		}
		
		T AddParticle<T>( T[] particles, ref int count, bool rain ) where T : Particle {
			if( count == maxParticles )
				RemoveAt( 0, particles, ref count );
			count++;
			
			T old = particles[count - 1];
			if( old != null ) return old;
			
			T newT = rain ? (T)(object)new RainParticle( game ) 
				: (T)(object)new TerrainParticle( game );
			particles[count - 1] = newT;
			return newT;
		}
		
		void RemoveAt<T>( int index, T[] particles, ref int count ) where T : Particle {
			T removed = particles[index];
			for( int i = index; i < count - 1; i++ ) {
				particles[i] = particles[i + 1];
			}
			
			particles[count - 1] = removed;
			count--;
		}
		
	}
}
