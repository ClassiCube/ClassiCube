using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {

	/// <summary> Entity component that draws square and circle shadows beneath entities. </summary>
	public unsafe sealed class ShadowComponent {
		
		Game game;
		Entity entity;
		public ShadowComponent( Game game, Entity entity ) {
			this.game = game;
			this.entity = entity;
		}
		
		internal void Draw() {
			EntityShadow mode = game.Players.ShadowMode;
			Vector3 Position = entity.Position;
			float posX = Position.X, posZ = Position.Z;
			int posY = Math.Min( (int)Position.Y, game.Map.Height - 1 );			
			int index = 0, vb = 0;
			
			VertexPos3fTex2fCol4b[] verts = null;
			ShadowData data = new ShadowData();
			int coordsCount = 0;
			Vector3I* coords = stackalloc Vector3I[4];
			for( int i = 0; i < 4; i++ )
				coords[i] = new Vector3I( int.MinValue );
			
			if( mode == EntityShadow.SnapToBlock ) {
				vb = game.Graphics.texVb; verts = game.Graphics.texVerts;						
				if( !CalculateShadow( coords, ref coordsCount, posX, posZ, posY, ref data ) ) return;
				
				float x1 = Utils.Floor( posX ), z1 = Utils.Floor( posZ );
				DraqSquareShadow( verts, ref index, data.Y, 220, x1, z1 );
			} else {
				vb = game.ModelCache.vb; verts = game.ModelCache.vertices;
				
				float x1 = posX - 7/16f, x2 = Math.Min( posX + 7/16f, Utils.Floor( x1 ) + 1 );
				float z1 = posZ - 7/16f, z2 = Math.Min( posZ + 7/16f, Utils.Floor( z1 ) + 1 );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, ref data ) && data.A > 0 )
					DrawCoords( verts, ref index, data, new Vector2( x1, z1 ), new Vector2( x2, z2 ), Position );
				
				x2 = posX + 7/16f; x1 = Math.Max( posX - 7/16f, Utils.Floor( x2 ) );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, ref data ) && data.A > 0 )
					DrawCoords( verts, ref index, data, new Vector2( x1, z1 ), new Vector2( x2, z2 ), Position );
				
				z2 = posZ + 7/16f; z1 = Math.Max( posZ - 7/16f, Utils.Floor( z2 ) );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, ref data ) && data.A > 0 )
					DrawCoords( verts, ref index, data, new Vector2( x1, z1 ), new Vector2( x2, z2 ), Position );
				
				x1 = posX - 7/16f; x2 = Math.Min( posX + 7/16f, Utils.Floor( x1 ) + 1 );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, ref data ) && data.A > 0 )
					DrawCoords( verts, ref index, data, new Vector2( x1, z1 ), new Vector2( x2, z2 ), Position );
			}
			
			if( index == 0 ) return;
			CheckShadowTexture( game.Graphics );
			
			if( !boundShadowTex ) {
				game.Graphics.BindTexture( shadowTex );
				boundShadowTex = true;
			}
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, verts, index, index * 6 / 4 );
		}
		
		bool CalculateShadow( Vector3I* coords, ref int coordsCount, float x, float z, int posY, ref ShadowData data ) {
			data = new ShadowData();
			int blockX = Utils.Floor( x ), blockZ = Utils.Floor( z );
			Vector3I p = new Vector3I( blockX, 0, blockZ );
			BlockInfo info = game.BlockInfo;
			Vector3 Position = entity.Position;
			
			if( Position.Y < 0 ) return false;
			for( int i = 0; i < 4; i++ )
				if( coords[i] == p ) return false;
			
			while( posY >= 0 ) {
				byte block = GetShadowBlock( blockX, posY, blockZ );
				if( !(info.IsAir[block] || info.IsSprite[block] || info.IsLiquid[block]) ) {
					float blockY = posY + info.MaxBB[block].Y;
					if( blockY < Position.Y + 0.01f ) { 
						data.Block = block; data.Y = blockY; break; 
					}
				}
				posY--;
			}
			if( posY == -1 ) data.Y = 0;
			
			coords[coordsCount] = p; coordsCount++;
			CalcAlpha( Position.Y, ref data.Y, ref data.A );
			return true;
		}
		
		byte GetShadowBlock( int x, int y, int z ) {
			if( x < 0 || z < 0 || x >= game.Map.Width || z >= game.Map.Length ) {
				if (y == game.Map.EdgeHeight - 1)
					return (byte)(game.BlockInfo.IsAir[(byte)game.Map.EdgeBlock] ? 0 : Block.Bedrock);
				if (y == game.Map.SidesHeight - 1)
					return (byte)(game.BlockInfo.IsAir[(byte)game.Map.SidesBlock] ? 0 : Block.Bedrock);
				return (byte)Block.Air;
			}
			return game.Map.GetBlock( x, y, z );
		}
		
		
		
		void DraqSquareShadow( VertexPos3fTex2fCol4b[] verts, ref int index, float y, byte alpha, float x, float z ) {
			FastColour col = FastColour.White; col.A = alpha;
			TextureRec rec = new TextureRec( 63/128f, 63/128f, 1/128f, 1/128f );
			verts[index++] = new VertexPos3fTex2fCol4b( x, y, z, rec.U1, rec.V1, col );
			verts[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, rec.U2, rec.V1, col );
			verts[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, rec.U2, rec.V2, col );
			verts[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, rec.U1, rec.V2, col );
		}
		
		void DrawCoords( VertexPos3fTex2fCol4b[] verts, ref int index, ShadowData data, Vector2 p1, Vector2 p2, Vector3 centre ) {
			if( lequal( p2.X, p1.X ) || lequal( p2.Y, p1.Y ) ) return;
			float u1 = (p1.X - centre.X) * 16/14f + 0.5f;
			float v1 = (p1.Y - centre.Z) * 16/14f + 0.5f;
			float u2 = (p2.X - centre.X) * 16/14f + 0.5f;
			float v2 = (p2.Y - centre.Z) * 16/14f + 0.5f;
			if( u2 <= 0 || v2 <= 0 || u1 >= 1 || v1 >= 1 ) return;
			
			p1.X = Math.Max( p1.X, centre.X - 14/16f ); u1 = Math.Max( u1, 0 );
			p1.Y = Math.Max( p1.Y, centre.Z - 14/16f ); v1 = Math.Max( v1, 0 );
			p2.X = Math.Min( p2.X, centre.X + 14/16f ); u2 = Math.Min( u2, 1 );
			p2.Y = Math.Min( p2.Y, centre.Z + 14/16f ); v2 = Math.Min( v2, 1 );
			
			FastColour col = FastColour.White; col.A = data.A;
			verts[index++] = new VertexPos3fTex2fCol4b( p1.X, data.Y, p1.Y, u1, v1, col );
			verts[index++] = new VertexPos3fTex2fCol4b( p2.X, data.Y, p1.Y, u2, v1, col );
			verts[index++] = new VertexPos3fTex2fCol4b( p2.X, data.Y, p2.Y, u2, v2, col );
			verts[index++] = new VertexPos3fTex2fCol4b( p1.X, data.Y, p2.Y, u1, v2, col );
		}
		
		struct ShadowData {
			public byte Block;
			public float Y;
			public byte A;
		}
		
		static void CalcAlpha( float playerY, ref float y, ref byte alpha ) {
			if( (playerY - y) <= 6 ) { y += 1/64f; alpha = (byte)(190 - 190 * (playerY - y) / 6); }
			else if( (playerY - y) <= 16 ) { y += 1/64f; alpha = 0; }
			else if( (playerY - y) <= 32 ) { y += 1/16f; alpha = 0; }
			else if( (playerY - y) <= 96 ) { y += 1/8f; alpha = 0; }
			else { y += 1/4f; alpha = 0; }
		}
		
		static bool lequal(float a, float b) {
			return a < b || Math.Abs(a - b) < 0.001f;
		}
		
		
		internal static bool boundShadowTex = false;
		internal static int shadowTex = -1;
		static void CheckShadowTexture( IGraphicsApi graphics ) {
			if( shadowTex != -1 ) return;
			const int size = 128, half = size / 2;
			using( Bitmap bmp = new Bitmap( size, size ) )
				using( FastBitmap fastBmp = new FastBitmap( bmp, true, false ) )
			{
				int inPix = new FastColour( 0, 0, 0, 200 ).ToArgb();
				int outPix = inPix & 0xFFFFFF;
				for( int y = 0; y < fastBmp.Height; y++ ) {
					int* row = fastBmp.GetRowPtr( y );
					for( int x = 0; x < fastBmp.Width; x++ ) {
						double dist = (half - (x + 0.5)) * (half - (x + 0.5)) +
							(half - (y + 0.5)) * (half - (y + 0.5));
						row[x] = dist < half * half ? inPix : outPix;
					}
				}
				shadowTex = graphics.CreateTexture( fastBmp );
			}
		}
	}
}