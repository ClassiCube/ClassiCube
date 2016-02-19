using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	partial class Player {
		
		internal static bool boundShadowTex = false;
		
		internal unsafe void DrawShadow( EntityShadow shadow ) {
			float posX = Position.X, posZ = Position.Z;
			int posY = Math.Min( (int)Position.Y, game.Map.Height - 1 );
			float y; byte alpha;
			
			int index = 0, vb = 0;
			VertexPos3fTex2fCol4b[] verts = null;
			int coordsCount = 0;
			Vector3I* coords = stackalloc Vector3I[4];
			for( int i = 0; i < 4; i++ )
				coords[i] = Vector3I.Zero;
			
			if( shadow == EntityShadow.SnapToBlock ) {
				vb = game.Graphics.texVb; verts = game.Graphics.texVerts;
				TextureRec rec = new TextureRec( 63/128f, 63/128f, 1/128f, 1/128f );
				
				if( !CalculateShadow( coords, ref coordsCount, posX, posZ, posY, out y, out alpha ) )
					return;
				alpha = 220;
				float x1 = Utils.Floor( posX ), z1 = Utils.Floor( posZ );
				DrawShadowPart( verts, ref index, y, alpha, x1, z1, x1 + 1, z1 + 1, rec );
			} else {
				vb = game.ModelCache.vb; verts = game.ModelCache.vertices;
				TextureRec rec;
				
				float x1 = posX - 7/16f, x2 = Math.Min( posX + 7/16f, Utils.Floor( x1 ) + 1 );
				float z1 = posZ - 7/16f, z2 = Math.Min( posZ + 7/16f, Utils.Floor( z1 ) + 1 );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, out y, out alpha ) && alpha > 0 ) {
					rec = TextureRec.FromPoints( 0, 0, (x2 - x1) * 16/14f, (z2 - z1) * 16/14f );
					DrawShadowPart( verts, ref index, y, alpha, x1, z1, x2, z2, rec );
				}
				
				x2 = posX + 7/16f; x1 = Math.Max( posX - 7/16f, Utils.Floor( x2 ) );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, out y, out alpha ) && alpha > 0 ) {
					rec = TextureRec.FromPoints( 1 - (x2 - x1) * 16/14f, 0, 1, (z2 - z1) * 16/14f );
					DrawShadowPart( verts, ref index, y, alpha, x1, z1, x2, z2, rec );
				}
				
				z2 = posZ + 7/16f; z1 = Math.Max( posZ - 7/16f, Utils.Floor( z2 ) );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, out y, out alpha ) && alpha > 0 ) {
					rec = TextureRec.FromPoints( 1 - (x2 - x1) * 16/14f, 1 - (z2 - z1) * 16/14f, 1, 1 );
					DrawShadowPart( verts, ref index, y, alpha, x1, z1, x2, z2, rec );
				}
				
				x1 = posX - 7/16f; x2 = Math.Min( posX + 7/16f, Utils.Floor( x1 ) + 1 );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, out y, out alpha ) && alpha > 0 ) {
					rec = TextureRec.FromPoints( 0, 1 - (z2 - z1) * 16/14f, (x2 - x1) * 16/14f, 1 );
					DrawShadowPart( verts, ref index, y, alpha, x1, z1, x2, z2, rec );
				}
			}
			
			if( index == 0 ) return;
			CheckShadowTexture();
			
			if( !boundShadowTex ) {
				game.Graphics.BindTexture( shadowTex );
				boundShadowTex = true;
			}
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, verts, index, index * 6 / 4 );
		}
		
		unsafe bool CalculateShadow( Vector3I* coords, ref int coordsCount, float x, float z, int posY, out float y, out byte alpha ) {
			y = 0; alpha = 0;
			int blockX = Utils.Floor( x ), blockZ = Utils.Floor( z );
			Vector3I p = new Vector3I( blockX, 0, blockZ );
			BlockInfo info = game.BlockInfo;
			
			if( Position.Y < 0 ) return false;
			for( int i = 0; i < 4; i++ )
				if( coords[i] == p ) return false;
			
			while( posY >= 0 ) {
				byte block = GetShadowBlock( blockX, posY, blockZ );
				if( !(info.IsAir[block] || info.IsSprite[block] || info.IsLiquid[block]) ) {
					float blockY = y = posY + info.MaxBB[block].Y;
					if( blockY < Position.Y + 0.01f ) { y = blockY; break; }
				}
				posY--;
			}
			
			coords[coordsCount] = p; coordsCount++;
			if( (Position.Y - posY) <= 6 ) { y += 1/64f; alpha = (byte)(220 - 220 * (Position.Y - posY) / 6); }
			else if( (Position.Y - posY) <= 16 ) { y += 1/64f; alpha = 0; }
			else if( (Position.Y - posY) <= 32 ) { y += 1/16f; alpha = 0; }
			else if( (Position.Y - posY) <= 96 ) { y += 1/8f; alpha = 0; }
			else { y += 1/4f; alpha = 0; }
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
		
		void DrawShadowPart( VertexPos3fTex2fCol4b[] verts, ref int index, float y, byte alpha,
		                    float x1, float z1, float x2, float z2, TextureRec rec ) {
			FastColour col = FastColour.White; col.A = alpha;
			verts[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
			verts[index++] = new VertexPos3fTex2fCol4b( x2, y, z1, rec.U2, rec.V1, col );
			verts[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
			verts[index++] = new VertexPos3fTex2fCol4b( x1, y, z2, rec.U1, rec.V2, col );
		}
		
		int shadowTex = -1;
		unsafe void CheckShadowTexture() {
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
				shadowTex = game.Graphics.CreateTexture( fastBmp );
			}
		}
	}
}