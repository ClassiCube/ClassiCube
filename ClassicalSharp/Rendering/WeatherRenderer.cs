// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Renderers {

	public class WeatherRenderer : IGameComponent {
		
		Game game;
		World map;
		IGraphicsApi graphics;
		BlockInfo info;
		
		public void Init( Game game ) {
			this.game = game;
			map = game.World;
			graphics = game.Graphics;
			info = game.BlockInfo;
			weatherVb = graphics.CreateDynamicVb( VertexFormat.P3fT2fC4b, vertices.Length );
			game.WorldEvents.OnNewMap += OnNewMap;
			game.WorldEvents.OnNewMapLoaded += OnNewMapLoaded;
		}
		
		public void Reset( Game game ) { }
		
		int weatherVb;
		short[] heightmap;
		float vOffset;
		const int extent = 4;
		VertexP3fT2fC4b[] vertices = new VertexP3fT2fC4b[8 * (extent * 2 + 1) * (extent * 2 + 1)];
		double rainAcc;
		Vector3I lastPos = new Vector3I( Int32.MinValue );
		
		public void Render( double deltaTime ) {
			Weather weather = map.Weather;
			if( weather == Weather.Sunny ) return;
			if( heightmap == null ) InitHeightmap();			
			
			graphics.BindTexture( weather == Weather.Rainy ? game.RainTexId : game.SnowTexId );
			Vector3 camPos = game.CurrentCameraPos;
			Vector3I pos = Vector3I.Floor( camPos );
			bool moved = pos != lastPos;
			lastPos = pos;
			                              
			float speed = weather == Weather.Rainy ? 1f : 0.20f;
			vOffset = -(float)game.accumulator * speed;
			rainAcc += deltaTime;
			bool particles = weather == Weather.Rainy;

			int index = 0;
			graphics.AlphaTest = false;
			graphics.DepthWrite = false;
			FastColour col = game.World.Sunlight;
			for( int dx = -extent; dx <= extent; dx++ ) {
				for( int dz = -extent; dz <= extent; dz++ ) {
					float rainY = GetRainHeight( pos.X + dx, pos.Z + dz );
					float height = Math.Max( game.World.Height, pos.Y + 64 ) - rainY;
					if( height <= 0 ) continue;
					
					if( particles && (rainAcc >= 0.25 || moved) )
						game.ParticleManager.AddRainParticle( new Vector3( pos.X + dx, rainY, pos.Z + dz ) );
					col.A = (byte)Math.Max( 0, AlphaAt( dx * dx + dz * dz ) );
					MakeRainForSquare( pos.X + dx, rainY, height, pos.Z + dz, col, ref index );
				}
			}
			if( particles && (rainAcc >= 0.25 || moved) )
				rainAcc = 0;
			
			if( index > 0 ) {
				graphics.SetBatchFormat( VertexFormat.P3fT2fC4b );
				graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, weatherVb, vertices, index, index * 6 / 4 );
			}
			graphics.AlphaTest = true;
			graphics.DepthWrite = true;
		}
		
		float AlphaAt( float x ) {
			// Wolfram Alpha: fit {0,178},{1,169},{4,147},{9,114},{16,59},{25,9}
			return 0.05f * x * x - 7 * x + 178;
		}
		
		void MakeRainForSquare( int x, float y, float height, int z, FastColour col, ref int index ) {
			float worldV = vOffset + (z & 1) / 2f - (x & 0x0F) / 16f;
			float v1 = y / 6f + worldV;
			float v2 = (y + height) / 6f + worldV;
			
			vertices[index++] = new VertexP3fT2fC4b( x, y, z, 0, v2, col );
			vertices[index++] = new VertexP3fT2fC4b( x, y + height, z, 0, v1, col );
			vertices[index++] = new VertexP3fT2fC4b( x + 1, y + height, z + 1, 2, v1, col );
			vertices[index++] = new VertexP3fT2fC4b( x + 1, y, z + 1, 2, v2, col );
			
			vertices[index++] = new VertexP3fT2fC4b( x + 1, y, z, 2, v2, col );
			vertices[index++] = new VertexP3fT2fC4b( x + 1, y + height, z, 2, v1, col );
			vertices[index++] = new VertexP3fT2fC4b( x, y + height, z + 1, 0, v1, col );
			vertices[index++] = new VertexP3fT2fC4b( x, y, z + 1, 0, v2, col );
		}

		int length, width, maxY, oneY;
		void OnNewMap( object sender, EventArgs e ) {
			heightmap = null;
			lastPos = new Vector3I( Int32.MaxValue );
		}
		
		void OnNewMapLoaded( object sender, EventArgs e ) {
			length = map.Length;
			width = map.Width;
			maxY = map.Height - 1;
			oneY = length * width;
		}
		
		void InitHeightmap() {
			heightmap = new short[map.Width * map.Length];
			for( int i = 0; i < heightmap.Length; i++ ) {
				heightmap[i] = short.MaxValue;
			}
		}
		
		public void Dispose() {
			game.WorldEvents.OnNewMap -= OnNewMap;
			game.WorldEvents.OnNewMapLoaded -= OnNewMapLoaded;
			graphics.DeleteDynamicVb( weatherVb );
		}
		
		float GetRainHeight( int x, int z ) {
			if( x < 0 || z < 0 || x >= width || z >= length ) return map.EdgeHeight;
			int index = (x * length) + z;
			int height = heightmap[index];
			int y = height == short.MaxValue ? CalcHeightAt( x, maxY, z, index ) : height;
			return y == -1 ? 0 :
				y + game.BlockInfo.MaxBB[map.GetBlock( x, y, z )].Y;
		}
		
		int CalcHeightAt( int x, int maxY, int z, int index ) {
			int mapIndex = ( maxY * length + z ) * width + x;
			for( int y = maxY; y >= 0; y-- ) {
				byte block = map.mapData[mapIndex];
				if( BlocksRain( block ) ) {
					heightmap[index] = (short)y;
					return y;
				}
				mapIndex -= oneY;
			}
			heightmap[index] = -1;
			return -1;
		}
		
		bool BlocksRain( byte block ) {
			return !(info.IsAir[block] || info.IsSprite[block]);
		}
		
		internal void UpdateHeight( int x, int y, int z, byte oldBlock, byte newBlock ) {
			if( game.World.IsNotLoaded || heightmap == null ) return;
			bool didBlock = BlocksRain( oldBlock );
			bool nowBlocks = BlocksRain( newBlock );
			if( didBlock == nowBlocks ) return;
			
			int index = (x * length) + z;
			int height = heightmap[index];
			if( height == short.MaxValue ) {
				if( map.Weather == Weather.Sunny ) return;
				// We have to calculate the entire column for visibility, because the old/new block info is
				// useless if there is another block higher than block.y that stops rain.
				CalcHeightAt( x, maxY, z, index );
			} else if( y >= height ) {
				if( nowBlocks ) {
					heightmap[index] = (short)y;
				} else {
					// Part of the column is now visible to rain, we don't know how exactly how high it should be though.
					// However, we know that if the old block was above or equal to rain height, then the new rain height must be <= old block.y
					CalcHeightAt( x, y, z, index );
				}
			}
		}
	}
}
