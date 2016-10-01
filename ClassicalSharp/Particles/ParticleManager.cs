// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Particles {
	
	public partial class ParticleManager : IGameComponent {
		
		public int ParticlesTexId;
		TerrainParticle[] terrainParticles = new TerrainParticle[maxParticles];
		RainParticle[] rainParticles = new RainParticle[maxParticles];
		int terrainCount, rainCount;
		int[] terrain1DCount, terrain1DIndices;
		
		Game game;
		Random rnd = new Random();
		int vb;
		const int maxParticles = 600;
		
		public void Init( Game game ) {
			this.game = game;
			vb = game.Graphics.CreateDynamicVb( VertexFormat.P3fT2fC4b, maxParticles * 4 );
			game.Events.TerrainAtlasChanged += TerrainAtlasChanged;
			game.UserEvents.BlockChanged += BreakBlockEffect;
			game.Events.TextureChanged += TextureChanged;
		}
		
		public void Ready( Game game ) { }
		public void Reset( Game game ) { rainCount = 0; terrainCount = 0; }
		public void OnNewMap( Game game ) { rainCount = 0; terrainCount = 0; }
		public void OnNewMapLoaded( Game game ) { }

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			terrain1DCount = new int[game.TerrainAtlas1D.TexIds.Length];
			terrain1DIndices = new int[game.TerrainAtlas1D.TexIds.Length];
		}
		
		void TextureChanged( object sender, TextureEventArgs e ) {
			if( e.Name == "particles.png" )
				game.UpdateTexture( ref ParticlesTexId, e.Name, e.Data, false );
		}
		
		VertexP3fT2fC4b[] vertices = new VertexP3fT2fC4b[0];
		public void Render( double delta, float t ) {
			if( terrainCount == 0 && rainCount == 0 ) return;
			IGraphicsApi graphics = game.Graphics;
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			graphics.SetBatchFormat( VertexFormat.P3fT2fC4b );
			
			RenderTerrainParticles( graphics, terrainParticles, terrainCount, delta, t );
			RenderRainParticles( graphics, rainParticles, rainCount, delta, t );
			
			graphics.AlphaTest = false;
			graphics.Texturing = false;
		}
		
		void RenderTerrainParticles( IGraphicsApi graphics, Particle[] particles, int elems, double delta, float t ) {
			int count = elems * 4;
			if( count > vertices.Length )
				vertices = new VertexP3fT2fC4b[count];

			Update1DCounts( particles, elems );
			for( int i = 0; i < elems; i++ ) {
				int index = particles[i].Get1DBatch( game );
				particles[i].Render( game, delta, t, vertices, ref terrain1DIndices[index] );
			}			
			int drawCount = Math.Min( count, maxParticles * 4 );
			if( drawCount == 0 ) return;
			
			graphics.SetDynamicVbData( vb, vertices, drawCount );
			int offset = 0;
			for( int i = 0; i < terrain1DCount.Length; i++ ) {
				int partCount = terrain1DCount[i];
				if( partCount == 0 ) continue;
				
				graphics.BindTexture( game.TerrainAtlas1D.TexIds[i] );
				graphics.DrawIndexedVb( DrawMode.Triangles, partCount * 6 / 4, offset * 6 / 4 );
				offset += partCount;
			}
		}
		
		void Update1DCounts( Particle[] particles, int elems ) {
			for( int i = 0; i < terrain1DCount.Length; i++ ) {
				terrain1DCount[i] = 0;
				terrain1DIndices[i] = 0;
			}
			for( int i = 0; i < elems; i++ ) {
				int index = particles[i].Get1DBatch( game );
				terrain1DCount[index] += 4;
			}
			for( int i = 1; i < terrain1DCount.Length; i++ )
				terrain1DIndices[i] = terrain1DIndices[i - 1] + terrain1DCount[i - 1];			
		}
		
		void RenderRainParticles( IGraphicsApi graphics, Particle[] particles, int elems, double delta, float t ) {
			int count = elems * 4;
			if( count > vertices.Length )
				vertices = new VertexP3fT2fC4b[count];
			
			int index = 0;
			for( int i = 0; i < elems; i++ )
				particles[i].Render( game, delta, t, vertices, ref index );
			
			int drawCount = Math.Min( count, maxParticles * 4 );
			if( drawCount == 0 ) return;
			graphics.BindTexture( ParticlesTexId );
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, vertices, drawCount, drawCount * 6 / 4 );
		}
		
		public void Tick( ScheduledTask task ) {
			TickParticles( terrainParticles, ref terrainCount, task.Interval );
			TickParticles( rainParticles, ref rainCount, task.Interval );
		}
		
		void TickParticles( Particle[] particles, ref int count, double delta ) {
			for( int i = 0; i < count; i++ ) {
				Particle particle = particles[i];
				if( particle.Tick( game, delta ) ) {
					RemoveAt( i, particles, ref count );
					i--;
				}
			}
		}
		
		public void Dispose() {
			game.Graphics.DeleteDynamicVb( ref vb );
			game.Graphics.DeleteTexture( ref ParticlesTexId );
			game.UserEvents.BlockChanged -= BreakBlockEffect;
			game.Events.TerrainAtlasChanged -= TerrainAtlasChanged;			
			game.Events.TextureChanged -= TextureChanged;
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
