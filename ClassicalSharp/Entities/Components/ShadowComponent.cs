// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Entities {

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
			int posY = Math.Min( (int)Position.Y, game.World.Height - 1 );
			int index = 0, vb = 0;
			
			VertexP3fT2fC4b[] verts = null;
			int posCount = 0, dataCount = 0;
			Vector3I* coords = stackalloc Vector3I[4];
			ShadowData* data = stackalloc ShadowData[4];
			for( int i = 0; i < 4; i++ ) {
				coords[i] = new Vector3I( int.MinValue );
				data[i] = new ShadowData();
			}
			
			if( mode == EntityShadow.SnapToBlock ) {
				vb = game.Graphics.texVb; verts = game.Graphics.texVerts;
				if( !GetBlocks( coords, ref posCount, posX, posZ, posY, data, ref dataCount ) ) return;
				
				float x1 = Utils.Floor( posX ), z1 = Utils.Floor( posZ );
				DraqSquareShadow( verts, ref index, data[0].Y, 220, x1, z1 );
			} else {
				vb = game.ModelCache.vb; verts = game.ModelCache.vertices;
				
				float x = posX - 7/16f, z = posZ - 7/16f;
				if( GetBlocks( coords, ref posCount, x, z, posY, data, ref dataCount ) && data[0].A > 0 )
					DrawCircle( verts, ref index, data, dataCount, x, z );
				
				x = Math.Max( posX - 7/16f, Utils.Floor( posX + 7/16f ) );
				if( GetBlocks( coords, ref posCount, x, z, posY, data, ref dataCount ) && data[0].A > 0 )
					DrawCircle( verts, ref index, data, dataCount, x, z );
				
				z = Math.Max( posZ - 7/16f, Utils.Floor( posZ + 7/16f ) );
				if( GetBlocks( coords, ref posCount, x, z, posY, data, ref dataCount ) && data[0].A > 0 )
					DrawCircle( verts, ref index, data, dataCount, x, z );
				
				x = posX - 7/16f;
				if( GetBlocks( coords, ref posCount, x, z, posY, data, ref dataCount ) && data[0].A > 0 )
					DrawCircle( verts, ref index, data, dataCount, x, z );
			}
			
			if( index == 0 ) return;
			CheckShadowTexture( game.Graphics );
			
			if( !boundShadowTex ) {
				game.Graphics.BindTexture( shadowTex );
				boundShadowTex = true;
			}
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, verts, index, index * 6 / 4 );
		}
		
		void DraqSquareShadow( VertexP3fT2fC4b[] verts, ref int index, float y, byte alpha, float x, float z ) {
			FastColour col = FastColour.White; col.A = alpha;
			TextureRec rec = new TextureRec( 63/128f, 63/128f, 1/128f, 1/128f );
			verts[index++] = new VertexP3fT2fC4b( x, y, z, rec.U1, rec.V1, col );
			verts[index++] = new VertexP3fT2fC4b( x + 1, y, z, rec.U2, rec.V1, col );
			verts[index++] = new VertexP3fT2fC4b( x + 1, y, z + 1, rec.U2, rec.V2, col );
			verts[index++] = new VertexP3fT2fC4b( x, y, z + 1, rec.U1, rec.V2, col );
		}
		
		void DrawCircle( VertexP3fT2fC4b[] verts, ref int index, 
		                ShadowData* data, int dataCount, float x, float z ) {
			x = Utils.Floor( x ); z = Utils.Floor( z );
			BlockInfo info = game.BlockInfo;
			Vector3 min = info.MinBB[data[0].Block], max = info.MaxBB[data[0].Block];
			
			DrawCoords( verts, ref index, data[0], x + min.X, z + min.Z, x + max.X, z + max.Z );
			for( int i = 1; i < dataCount; i++ ) {
				Vector3 nMin = info.MinBB[data[i].Block], nMax = info.MaxBB[data[i].Block];
				DrawCoords( verts, ref index, data[i], x + min.X, z + nMin.Z, x + max.X, z + min.Z );
				DrawCoords( verts, ref index, data[i], x + min.X, z + max.Z, x + max.X, z + nMax.Z );
				
				DrawCoords( verts, ref index, data[i], x + nMin.X, z + nMin.Z, x + min.X, z + nMax.Z );
				DrawCoords( verts, ref index, data[i], x + max.X, z + nMin.Z, x + nMax.X, z + nMax.Z );
				min = nMin; max = nMax;
			}
		}
		void DrawCoords( VertexP3fT2fC4b[] verts, ref int index, ShadowData data, 
		                float x1, float z1, float x2, float z2 ) {
			Vector3 centre = entity.Position;
			BlockInfo info = game.BlockInfo;
			
			if( lequal( x2, x1 ) || lequal( z2, z1 ) ) return;
			float u1 = (x1 - centre.X) * 16/14f + 0.5f;
			float v1 = (z1 - centre.Z) * 16/14f + 0.5f;
			float u2 = (x2 - centre.X) * 16/14f + 0.5f;
			float v2 = (z2 - centre.Z) * 16/14f + 0.5f;
			if( u2 <= 0 || v2 <= 0 || u1 >= 1 || v1 >= 1 ) return;
			
			x1 = Math.Max( x1, centre.X - 7/16f ); u1 = Math.Max( u1, 0 );
			z1 = Math.Max( z1, centre.Z - 7/16f ); v1 = Math.Max( v1, 0 );
			x2 = Math.Min( x2, centre.X + 7/16f ); u2 = Math.Min( u2, 1 );
			z2 = Math.Min( z2, centre.Z + 7/16f ); v2 = Math.Min( v2, 1 );
			
			FastColour col = FastColour.White; col.A = data.A;
			verts[index++] = new VertexP3fT2fC4b( x1, data.Y, z1, u1, v1, col );
			verts[index++] = new VertexP3fT2fC4b( x2, data.Y, z1, u2, v1, col );
			verts[index++] = new VertexP3fT2fC4b( x2, data.Y, z2, u2, v2, col );
			verts[index++] = new VertexP3fT2fC4b( x1, data.Y, z2, u1, v2, col );
		}
		
		bool GetBlocks( Vector3I* coords, ref int posCount, float x, float z,
		               int posY, ShadowData* data, ref int index ) {
			int blockX = Utils.Floor( x ), blockZ = Utils.Floor( z );
			Vector3I p = new Vector3I( blockX, 0, blockZ );
			BlockInfo info = game.BlockInfo;
			Vector3 Position = entity.Position;
			index = 0;
			
			// Check we have not processed this particular block already.
			if( Position.Y < 0 ) return false;
			for( int i = 0; i < 4; i++ ) {
				if( coords[i] == p ) return false;
				data[i] = new ShadowData();
			}
			coords[posCount] = p; posCount++;
			
			while( posY >= 0 && index < 4 ) {
				byte block = GetShadowBlock( blockX, posY, blockZ );
				posY--;
				if( info.IsAir[block] || info.IsSprite[block] || info.IsLiquid[block] ) continue;				
				float blockY = posY + 1 + info.MaxBB[block].Y;
				if( blockY >= Position.Y + 0.01f ) continue;
				
				data[index].Block = block; data[index].Y = blockY;
				CalcAlpha( Position.Y, ref data[index] );
				index++;				
				// Check if the casted shadow will continue on further down.
				if( info.MinBB[block].X == 0 && info.MaxBB[block].X == 1 &&
				   info.MinBB[block].Z == 0 && info.MaxBB[block].Z == 1 ) return true;
			}
			
			if( index < 4 ) {
				data[index].Block = (byte)game.World.EdgeBlock; data[index].Y = 0;
				CalcAlpha( Position.Y, ref data[index] );
				index++;
			}
			return true;
		}
		
		byte GetShadowBlock( int x, int y, int z ) {
			if( x < 0 || z < 0 || x >= game.World.Width || z >= game.World.Length ) {
				if( y == game.World.EdgeHeight - 1 )
					return (byte)(game.BlockInfo.IsAir[(byte)game.World.EdgeBlock] ? 0 : Block.Bedrock);
				if( y == game.World.SidesHeight - 1 )
					return (byte)(game.BlockInfo.IsAir[(byte)game.World.SidesBlock] ? 0 : Block.Bedrock);
				return (byte)Block.Air;
			}
			return game.World.GetBlock( x, y, z );
		}
		
		struct ShadowData {
			public byte Block;
			public float Y;
			public byte A;
		}
		
		static void CalcAlpha( float playerY, ref ShadowData data ) {
			float y = data.Y;
			if( (playerY - y) <= 6 ) {
				data.A = (byte)(190 - 190 * (playerY - y) / 6);
				data.Y += 1/64f; return;
			}
			
			data.A = 0;
			if( (playerY - y) <= 16 ) data.Y += 1/64f;
			else if( (playerY - y) <= 32 ) data.Y += 1/16f;
			else if( (playerY - y) <= 96 ) data.Y += 1/8f;
			else data.Y += 1/4f;
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
				int outPix = new FastColour( 0, 0, 0, 0 ).ToArgb();
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