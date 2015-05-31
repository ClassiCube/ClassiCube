using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {

	public class WeatherRenderer {
		
		public Game Window;
		Map map;
		IGraphicsApi graphics;
		BlockInfo info;
		public WeatherRenderer( Game window ) {
			Window = window;
			map = Window.Map;
			graphics = window.Graphics;
			info = Window.BlockInfo;
			weatherVb = graphics.CreateDynamicVb( VertexFormat.Pos3fTex2fCol4b, 12 * 9 * 9 );
		}
		
		int weatherVb;
		int rainTexture, snowTexture;
		short[] heightmap;
		float vOffset;
		VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[12 * 9 * 9];
		public void Render( double deltaTime ) {
			Weather weather = map.Weather;
			if( weather == Weather.Sunny ) return;
			
			graphics.Texturing = true;
			graphics.Bind2DTexture( weather == Weather.Rainy ? rainTexture : snowTexture );
			Vector3I pos = Vector3I.Floor( Window.LocalPlayer.Position );
			float speed = weather == Weather.Rainy ? 1f : 0.25f;
			vOffset = -(float)Window.accumulator * speed;
			
			int index = 0;
			graphics.AlphaBlending = true;
			FastColour col = FastColour.White;
			for( int dx = - 4; dx <= 4; dx++ ) {
				for( int dz = -4; dz <= 4; dz++ ) {
					int rainY = Math.Max( pos.Y, GetRainHeight( pos.X + dx, pos.Z + dz ) + 1 );
					int height = Math.Min( 6 - ( rainY - pos.Y ), 6 );
					if( height <= 0 ) continue;
					
					col.A = (byte)Math.Max( 0, AlphaAt( dx * dx + dz * dz ) );
					MakeRainForSquare( pos.X + dx, rainY, height, pos.Z + dz, col, ref index );
				}
			}
			graphics.DrawDynamicVb( DrawMode.Triangles, weatherVb, vertices, VertexFormat.Pos3fTex2fCol4b, index );
			graphics.AlphaBlending = false;
			graphics.Texturing = false;
		}
		
		float AlphaAt( float x ) {
			return (float)( 0.05 * x * x - 8 * x + 178 );
		}
		
		void MakeRainForSquare( int x, int y, int height, int z, FastColour col, ref int index ) {
			float v1 = vOffset + ( z & 0x01 ) * 0.5f - ( x & 0x0F ) * 0.0625f;
			float v2 = height / 6f + v1;
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, 0, v2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + height, z, 0, v1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + height, z + 1, 2, v1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + height, z + 1, 2, v1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, 2, v2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, 0, v2, col );
			
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, 2, v2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + height, z, 2, v1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + height, z + 1, 0, v1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + height, z + 1, 0, v1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, 0, v2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, 2, v2, col );
		}

		int length, width, maxY, oneY;
		void OnNewMap( object sender, EventArgs e ) {
			heightmap = null;
		}
		
		void OnNewMapLoaded( object sender, EventArgs e ) {
			length = map.Length;
			width = map.Width;
			maxY = map.Height - 1;
			oneY = length * width;
			
			heightmap = new short[map.Width * map.Length];
			for( int i = 0; i < heightmap.Length; i++ ) {
				heightmap[i] = short.MaxValue;
			}
		}
		
		public void Init() {
			rainTexture = graphics.LoadTexture( "rain.png" );
			snowTexture = graphics.LoadTexture( "snow.png" );
			Window.OnNewMap += OnNewMap;
			Window.OnNewMapLoaded += OnNewMapLoaded;
		}
		
		public void Dispose() {
			Window.OnNewMap -= OnNewMap;
			Window.OnNewMapLoaded -= OnNewMapLoaded;
			graphics.DeleteTexture( ref rainTexture );
			graphics.DeleteTexture( ref snowTexture );
		}
		
		int GetRainHeight( int x, int z ) {
			if( x < 0 || z < 0 || x >= length || z >= length ) return map.WaterHeight - 1;
			int index = ( x * length ) + z;
			int height = heightmap[index];			
			return height == short.MaxValue ? CalcHeightAt( x, maxY, z, index ) : height;
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
			// Wolfram Alpha: fit {0,178},{1,169},{4,147},{9,114},{16,59},{25,9}
			return !( block == 0 || info.IsSprite( block ) || info.IsLiquid( block ) );
		}
		
		internal void UpdateHeight( int x, int y, int z, byte oldBlock, byte newBlock ) {
			bool didBlock = BlocksRain( oldBlock );
			bool nowBlocks = BlocksRain( newBlock );
			if( didBlock == nowBlocks ) return;
			
			int index = ( x * length ) + z;
			if( nowBlocks ) {
				if( y > GetRainHeight( x, z ) ) {
					heightmap[index] = (short)y;
				}
			} else {
				if( y >= GetRainHeight( x, z ) ) {
					CalcHeightAt( x, y, z, index );
				}
			}
		}
	}
}
