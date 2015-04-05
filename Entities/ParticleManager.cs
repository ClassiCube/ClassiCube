using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Particles {
	
	public class ParticleManager {
		
		List<Particle> particles = new List<Particle>();
		public Game Window;
		public OpenGLApi Graphics;
		
		public ParticleManager( Game window ) {
			Window = window;
			Graphics = window.Graphics;
		}
		
		public void Render( double delta, float t ) {
			if( particles.Count == 0 ) return;
			
			VertexPos3fTex2f[] vertices = new VertexPos3fTex2f[particles.Count * 6];
			int index = 0;
			for( int i = 0; i < particles.Count; i++ ) {
				particles[i].Render( delta, t, vertices, ref index );
			}
			
			Graphics.Texturing = true;
			Graphics.Bind2DTexture( Window.TerrainAtlasTexId );
			Graphics.AlphaTest = true;
			Graphics.DrawVertices( DrawMode.Triangles, vertices, vertices.Length );
			Graphics.AlphaTest = false;
			Graphics.Texturing = false;
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
		
		int particleId = 0;
		public void BreakBlockEffect( Vector3I position, byte block ) {
			Vector3 startPos = new Vector3( position.X, position.Y, position.Z );
			int texLoc = Window.BlockInfo.GetOptimTextureLoc( block, TileSide.Left );
			TextureRectangle rec = Window.TerrainAtlas.GetTexRec( texLoc );
			
			float invHorSize = Window.TerrainAtlas.invHorElementSize;
			float invVerSize = Window.TerrainAtlas.invVerElementSize;
			int cellsCountX = (int)( 0.25f / invHorSize );
			int cellsCountY = (int)( 0.25f / invVerSize );
			float elementXSize = invHorSize * 0.25f;
			float elementYSize = invVerSize * 0.25f;
			
			Random rnd = new Random();
			for( int i = 0; i < 25; i++ ) {
				double velX = ( rnd.NextDouble() * 0.8/*5*/ ) - 0.4/*0.25*/;
				double velZ = ( rnd.NextDouble() * 0.8/*5*/ ) - 0.4/*0.25*/;
				double velY = ( rnd.NextDouble() + 0.25 ) * Window.BlockInfo.BlockHeight( block );
				Vector3 velocity = new Vector3( (float)velX, (float)velY, (float)velZ );
				double xOffset = rnd.NextDouble() - 0.125;
				double yOffset = rnd.NextDouble() - 0.125;
				double zOffset = rnd.NextDouble() - 0.125;
				Vector3 pos = startPos + new Vector3( (float)xOffset, (float)yOffset, (float)zOffset );
				TextureRectangle particleRec = rec;
				particleRec.U1 = (float)( rec.U1 + rnd.Next( 0, cellsCountX ) * elementXSize );
				particleRec.V1 = (float)( rec.V1 + rnd.Next( 0, cellsCountY ) * elementYSize );
				particleRec.U2 = particleRec.U1 + elementXSize;
				particleRec.V2 = particleRec.V1 + elementYSize;
				double life = 3 - rnd.NextDouble();
				
				particles.Add( new TerrainParticle( Window, pos, velocity, particleId, life, particleRec ) );
				particleId++;
			}
		}
	}
}
