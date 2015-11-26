using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Particles {
	
	public class ParticleManager : IDisposable {
		
		public int ParticlesTexId;
		List<Particle> terrainParticles = new List<Particle>();
		List<Particle> rainParticles = new List<Particle>();
		Game game;
		Random rnd;
		int vb;
		
		public ParticleManager( Game game ) {
			this.game = game;
			rnd = new Random();
			vb = game.Graphics.CreateDynamicVb( VertexFormat.Pos3fTex2fCol4b, 1000 );
		}
		
		VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[0];
		public void Render( double delta, float t ) {
			if( terrainParticles.Count == 0 && rainParticles.Count == 0 ) return;
			IGraphicsApi graphics = game.Graphics;
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			graphics.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			
			int count = RenderParticles( terrainParticles, delta, t );
			if( count > 0 ) {
				graphics.BindTexture( game.TerrainAtlas.TexId );
				graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, vertices, count, count * 6 / 4 );
			}
			count = RenderParticles( rainParticles, delta, t );
			if( count > 0 ) {
				graphics.BindTexture( ParticlesTexId );
				graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, vertices, count, count * 6 / 4 );
			}
			
			graphics.AlphaTest = false;
			graphics.Texturing = false;
		}
		
		int RenderParticles( List<Particle> particles, double delta, float t ) {
			int count = particles.Count * 4;
			if( count > vertices.Length )
				vertices = new VertexPos3fTex2fCol4b[count];
			
			int index = 0;
			for( int i = 0; i < particles.Count; i++ )
				particles[i].Render( delta, t, vertices, ref index );				
			return Math.Min( count, 1000 );
		}
		
		public void Tick( double delta ) {
			TickParticles( terrainParticles, delta );
			TickParticles( rainParticles, delta );
		}
		
		void TickParticles( List<Particle> particles, double delta ) {
			for( int i = 0; i < particles.Count; i++ ) {
				Particle particle = particles[i];
				if( particle.Tick( delta ) ) {
					particles.RemoveAt( i );
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
			TextureRec rec = game.TerrainAtlas.GetTexRec( texLoc );
			
			const float invSize = TerrainAtlas2D.invElementSize;
			const int cellsCount = (int)((1/4f) / invSize);
			const float elemSize = invSize / 4f;
			float blockHeight = game.BlockInfo.Height[block];
			
			for( int i = 0; i < 25; i++ ) {
				double velX = rnd.NextDouble() * 0.8 - 0.4; // [-0.4, 0.4]
				double velZ = rnd.NextDouble() * 0.8 - 0.4;
				double velY = rnd.NextDouble() + 0.2;
				Vector3 velocity = new Vector3( (float)velX, (float)velY, (float)velZ );
				
				double xOffset = rnd.NextDouble() - 0.5; // [-0.5, 0.5]
				double yOffset = (rnd.NextDouble() - 0.125) * blockHeight;
				double zOffset = rnd.NextDouble() - 0.5;
				Vector3 pos = startPos + new Vector3( 0.5f + (float)xOffset,
				                                     (float)yOffset, 0.5f + (float)zOffset );
				
				TextureRec particleRec = rec;
				particleRec.U1 = rec.U1 + rnd.Next( 0, cellsCount ) * elemSize;
				particleRec.V1 = rec.V1 + rnd.Next( 0, cellsCount ) * elemSize;
				particleRec.U2 = particleRec.U1 + elemSize;
				particleRec.V2 = particleRec.V1 + elemSize;
				double life = 1.5 - rnd.NextDouble();
				
				terrainParticles.Add( new TerrainParticle( game, pos, velocity, life, particleRec ) );
			}
		}
		
		public void AddRainParticle( Vector3 pos ) {
			Vector3 startPos = pos;
			
			for( int i = 0; i < 5; i++ ) {
				double velX = rnd.NextDouble() * 0.8 - 0.4; // [-0.4, 0.4]
				double velZ = rnd.NextDouble() * 0.8 - 0.4;
				double velY = rnd.NextDouble() + 0.5;
				Vector3 velocity = new Vector3( (float)velX, (float)velY, (float)velZ );
				
				double xOffset = rnd.NextDouble() - 0.5; // [-0.5, 0.5]
				double yOffset = 0.01;
				double zOffset = rnd.NextDouble() - 0.5;
				pos = startPos + new Vector3( 0.5f + (float)xOffset,
				                                     (float)yOffset, 0.5f + (float)zOffset );
				double life = 3.5 - rnd.NextDouble();		
				rainParticles.Add( new RainParticle( game, pos, velocity, life ) );
			}
		}
	}
}
