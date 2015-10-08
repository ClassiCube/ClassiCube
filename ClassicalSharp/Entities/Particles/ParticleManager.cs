using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Particles {
	
	public class ParticleManager : IDisposable {
		
		List<Particle> particles = new List<Particle>();
		Game game;
		IGraphicsApi graphics;
		int vb;
		
		public ParticleManager( Game game ) {
			this.game = game;
			graphics = game.Graphics;
			vb = graphics.CreateDynamicVb( VertexFormat.Pos3fTex2fCol4b, 1000 );
		}
		
		VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[0];
		public void Render( double delta, float t ) {
			if( particles.Count == 0 ) return;
			
			int count = particles.Count * 4;
			if( count > vertices.Length ) {
				vertices = new VertexPos3fTex2fCol4b[count];
			}
			int index = 0;
			for( int i = 0; i < particles.Count; i++ ) {
				particles[i].Render( delta, t, vertices, ref index );
			}
			
			graphics.Texturing = true;
			graphics.BindTexture( game.TerrainAtlas.TexId );
			graphics.AlphaTest = true;
			count = Math.Min( count, 1000 );
			
			graphics.BeginVbBatch( VertexFormat.Pos3fTex2fCol4b );
			graphics.DrawDynamicIndexedVb( DrawMode.Triangles, vb, vertices, count, count * 6 / 4 );
			graphics.AlphaTest = false;
			graphics.Texturing = false;
		}
		
		public void Tick( double delta ) {
			for( int i = 0; i < particles.Count; i++ ) {
				Particle particle = particles[i];
				if( particle.Tick( delta ) ) {				
					particles.RemoveAt( i );
					i--;
					particle.Dispose();
				}
			}
		}
		
		public void Dispose() {
			graphics.DeleteDynamicVb( vb );
		}
		
		public void BreakBlockEffect( Vector3I position, byte block ) {
			Vector3 startPos = new Vector3( position.X, position.Y, position.Z );
			int texLoc = game.BlockInfo.GetTextureLoc( block, TileSide.Left );
			TextureRec rec = game.TerrainAtlas.GetTexRec( texLoc );
			
			float invSize = TerrainAtlas2D.invElementSize;
			int cellsCountX = (int)( 0.25f / invSize );
			int cellsCountY = (int)( 0.25f / invSize );
			float elementXSize = invSize * 0.25f;
			float elementYSize = invSize * 0.25f;
			
			Random rnd = new Random();
			for( int i = 0; i < 25; i++ ) {
				double velX = ( rnd.NextDouble() * 0.8/*5*/ ) - 0.4/*0.25*/;
				double velZ = ( rnd.NextDouble() * 0.8/*5*/ ) - 0.4/*0.25*/;
				double velY = ( rnd.NextDouble() + 0.25 ) * game.BlockInfo.Height[block];
				Vector3 velocity = new Vector3( (float)velX, (float)velY, (float)velZ );
				double xOffset = rnd.NextDouble() - 0.125;
				double yOffset = rnd.NextDouble() - 0.125;
				double zOffset = rnd.NextDouble() - 0.125;
				Vector3 pos = startPos + new Vector3( (float)xOffset, (float)yOffset, (float)zOffset );
				TextureRec particleRec = rec;
				particleRec.U1 = (float)( rec.U1 + rnd.Next( 0, cellsCountX ) * elementXSize );
				particleRec.V1 = (float)( rec.V1 + rnd.Next( 0, cellsCountY ) * elementYSize );
				particleRec.U2 = particleRec.U1 + elementXSize;
				particleRec.V2 = particleRec.V1 + elementYSize;
				double life = 1.5 - rnd.NextDouble();
				
				particles.Add( new TerrainParticle( game, pos, velocity, life, particleRec ) );
			}
		}
	}
}
