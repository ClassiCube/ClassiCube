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
		
		internal void DrawShadow( EntityShadow shadow ) {
			if( shadow == EntityShadow.None ) return;
			float posX = Position.X, posZ = Position.Z;
			float x1 = 0, x2 = 0, z1, z2 = 0;
			if( shadow == EntityShadow.SnapToBlock ) {
				x1 = Utils.Floor( posX ); x2 = x1 + 1;
				z1 = Utils.Floor( posZ ); z2 = z1 + 1;
			} else {
				x1 = posX - 7/16f; x2 = posX + 7/16f;
				z1 = posZ - 7/16f; z2 = posZ + 7/16f;
			}

			int blockX = Utils.Floor( Position.X ), blockZ = Utils.Floor( Position.Z );
			if( !game.Map.IsValidPos( blockX, 0, blockZ ) || Position.Y < 0 ) return;
			CheckShadowTexture();
			BlockInfo info = game.BlockInfo;
			game.Graphics.BindTexture( shadowTex );
			
			int y = Math.Min( (int)Position.Y, game.Map.Height - 1 );
			float shadowY = 0;
			while( y >= 0 ) {
				byte block = game.Map.GetBlock( blockX, y, blockZ );
				if( !(info.IsAir[block] || info.IsSprite[block]) ) {
					shadowY = y + info.MaxBB[block].Y; break;
				}
				y--;
			}
			
			byte rgb = 80;
			if( (Position.Y - y) <= 16 ) { shadowY += 1/32f; rgb = (byte)(80 * (Position.Y - y) / 16); }
			else if( (Position.Y - y) <= 32 ) shadowY += 1/16f;
			else if( (Position.Y - y) <= 96 ) shadowY += 1/8f;
			else shadowY += 1/4f;
			VertexPos3fTex2fCol4b[] verts = game.Graphics.texVerts;
			int vb = game.Graphics.texVb;
			game.Graphics.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			
			FastColour col = new FastColour( rgb, rgb, rgb );
			float u1 = 0, v1 = 0, u2 = 1, v2 = 1;
			if( shadow == EntityShadow.SnapToBlock ) {
				u1 = 63/128f; v1 = 63/128f; u2 = 64/128f; v2 = 64/128f;
			}
			verts[0] = new VertexPos3fTex2fCol4b( x1, shadowY, z1, u1, v1, col );
			verts[1] = new VertexPos3fTex2fCol4b( x2, shadowY, z1, u2, v1, col );
			verts[2] = new VertexPos3fTex2fCol4b( x2, shadowY, z2, u2, v2, col );
			verts[3] = new VertexPos3fTex2fCol4b( x1, shadowY, z2, u1, v2, col );
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, verts, 4, 6 );
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