using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {

	partial class Player {

		protected Texture nameTex;
		protected internal int PlayerTextureId = -1, MobTextureId = -1;
		
		public override void Despawn() {
			game.Graphics.DeleteTexture( ref PlayerTextureId );
			game.Graphics.DeleteTexture( ref nameTex.ID );
			if( shadowTex != -1 )
				game.Graphics.DeleteTexture( ref shadowTex );
		}
		
		protected void InitRenderingData() {
			using( Font font = new Font( game.FontName, 20 ) ) {
				DrawTextArgs args = new DrawTextArgs( DisplayName, font, true );
				nameTex = game.Drawer2D.MakeBitmappedTextTexture( ref args, 0, 0 );
			}
		}
		
		public void UpdateNameFont() {
			game.Graphics.DeleteTexture( ref nameTex );
			InitRenderingData();
		}
		
		protected void DrawName() {
			IGraphicsApi api = game.Graphics;
			api.BindTexture( nameTex.ID );
			Vector3 pos = Position; pos.Y += Model.NameYOffset;
			
			Vector3 p111, p121, p212, p222;
			FastColour col = FastColour.White;
			Vector2 size = new Vector2( nameTex.Width / 70f, nameTex.Height / 70f );
			Utils.CalcBillboardPoints( size, pos, ref game.View, out p111, out p121, out p212, out p222 );
			api.texVerts[0] = new VertexPos3fTex2fCol4b( p111, nameTex.U1, nameTex.V2, col );
			api.texVerts[1] = new VertexPos3fTex2fCol4b( p121, nameTex.U1, nameTex.V1, col );
			api.texVerts[2] = new VertexPos3fTex2fCol4b( p222, nameTex.U2, nameTex.V1, col );
			api.texVerts[3] = new VertexPos3fTex2fCol4b( p212, nameTex.U2, nameTex.V2, col );
			
			api.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			api.UpdateDynamicIndexedVb( DrawMode.Triangles, api.texVb, api.texVerts, 4, 6 );
		}
		
		internal unsafe void DrawShadow( EntityShadow shadow ) {
			if( shadow == EntityShadow.None ) return;
			float posX = Position.X, posZ = Position.Z;		
			int posY = Math.Min( (int)Position.Y, game.Map.Height - 1 );
			float y; byte rgb;
			
			int index = 0, vb = 0;
			VertexPos3fTex2fCol4b[] verts = null;
			int coordsCount = 0;
			Vector3I* coords = stackalloc Vector3I[4];
			for( int i = 0; i < 4; i++ )
				coords[i] = Vector3I.Zero;
			
			if( shadow == EntityShadow.SnapToBlock ) {
				vb = game.Graphics.texVb; verts = game.Graphics.texVerts;
				TextureRec rec = new TextureRec( 63/128f, 63/128f, 1/128f, 1/128f );
				
				if( !CalculateShadow( coords, ref coordsCount, posX, posZ, posY, out y, out rgb ) ) 
					return;
				float x1 = Utils.Floor( posX ), z1 = Utils.Floor( posZ );
				DrawShadowPart( verts, ref index, y, rgb, x1, z1, x1 + 1, z1 + 1, rec );
			} else {
				vb = game.ModelCache.vb; verts = game.ModelCache.vertices;
				TextureRec rec;
				
				float x1 = posX - 7/16f, x2 = Math.Min( posX + 7/16f, Utils.Floor( x1 ) + 1 );
				float z1 = posZ - 7/16f, z2 = Math.Min( posZ + 7/16f, Utils.Floor( z1 ) + 1 );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, out y, out rgb ) ) {
					rec = TextureRec.FromPoints( 0, 0, (x2 - x1) * 16/14f, (z2 - z1) * 16/14f );
					DrawShadowPart( verts, ref index, y, rgb, x1, z1, x2, z2, rec );
				}
				
				x2 = posX + 7/16f; x1 = Math.Max( posX - 7/16f, Utils.Floor( x2 ) );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, out y, out rgb ) ) {
					rec = TextureRec.FromPoints( 1 - (x2 - x1) * 16/14f, 0, 1, (z2 - z1) * 16/14f );
					DrawShadowPart( verts, ref index, y, rgb, x1, z1, x2, z2, rec );
				}
				
				z2 = posZ + 7/16f; z1 = Math.Max( posZ - 7/16f, Utils.Floor( z2 ) );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, out y, out rgb ) ) {
					rec = TextureRec.FromPoints( 1 - (x2 - x1) * 16/14f, 1 - (z2 - z1) * 16/14f, 1, 1 );
					DrawShadowPart( verts, ref index, y, rgb, x1, z1, x2, z2, rec );
				}
				
				x1 = posX - 7/16f; x2 = Math.Min( posX + 7/16f, Utils.Floor( x1 ) + 1 );
				if( CalculateShadow( coords, ref coordsCount, x1, z1, posY, out y, out rgb ) ) {				
					rec = TextureRec.FromPoints( 0, 1 - (z2 - z1) * 16/14f, (x2 - x1) * 16/14f, 1 );
					DrawShadowPart( verts, ref index, y, rgb, x1, z1, x2, z2, rec );
				}
			}
			
			if( index == 0 ) return;
			CheckShadowTexture();
			game.Graphics.BindTexture( shadowTex );
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, verts, index, index * 6 / 4 );
		}
		
		unsafe bool CalculateShadow( Vector3I* coords, ref int coordsCount, float x, float z, int posY, out float y, out byte rgb ) {
			y = 0; rgb = 80;
			int blockX = Utils.Floor( x ), blockZ = Utils.Floor( z );
			Vector3I p = new Vector3I( blockX, 0, blockZ );
			BlockInfo info = game.BlockInfo;
			
			if( !game.Map.IsValidPos( p ) || Position.Y < 0 ) return false;		
			for( int i = 0; i < 4; i++ )
				if( coords[i] == p ) return false;
			
			while( posY >= 0 ) {
				byte block = game.Map.GetBlock( blockX, posY, blockZ );
				if( !(info.IsAir[block] || info.IsSprite[block] || info.IsLiquid[block]) ) {
					float blockY = y = posY + info.MaxBB[block].Y;
					if( blockY < Position.Y + 0.01f ) { y = blockY; break; }
				}
				posY--;
			}
			
			coords[coordsCount] = p; coordsCount++;
			if( (Position.Y - posY) <= 16 ) { y += 1/64f; rgb = (byte)(80 * (Position.Y - posY) / 16); }
			else if( (Position.Y - posY) <= 32 ) y += 1/16f;
			else if( (Position.Y - posY) <= 96 ) y += 1/8f;
			else y += 1/4f;
			return true;
		}
		
		void DrawShadowPart( VertexPos3fTex2fCol4b[] verts, ref int index, float y, byte rgb,
		                    float x1, float z1, float x2, float z2, TextureRec rec ) {
			FastColour col = new FastColour( rgb, rgb, rgb );
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
				int inPix = new FastColour( 127, 127, 127, 200 ).ToArgb();
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