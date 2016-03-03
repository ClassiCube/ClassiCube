using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Network;
using OpenTK;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {

	public abstract partial class Player : Entity {
		
		public string DisplayName, SkinName, SkinIdentifier;
		public SkinType SkinType;
		internal AnimatedComponent anim;
		internal ShadowComponent shadow;
		
		public Player( Game game ) : base( game ) {
			this.game = game;
			StepSize = 0.5f;
			SkinType = game.DefaultPlayerSkinType;
			anim = new AnimatedComponent( game, this );
			shadow = new ShadowComponent( game, this );
			SetModel( "humanoid" );
		}
		
		DateTime lastModelChange = new DateTime( 1, 1, 1 );
		public void SetModel( string modelName ) {
			ModelName = modelName;
			Model = game.ModelCache.GetModel( ModelName );
			lastModelChange = DateTime.UtcNow;
			MobTextureId = -1;
		}
		
		protected Texture nameTex;
		protected internal int PlayerTextureId = -1, MobTextureId = -1;
		
		public override void Despawn() {
			game.Graphics.DeleteTexture( ref PlayerTextureId );
			game.Graphics.DeleteTexture( ref nameTex.ID );
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
		
		protected void CheckSkin() {
			DownloadedItem item;
			game.AsyncDownloader.TryGetItem( SkinIdentifier, out item );
			if( item != null && item.Data != null ) {
				Bitmap bmp = (Bitmap)item.Data;
				game.Graphics.DeleteTexture( ref PlayerTextureId );
				if( !FastBitmap.CheckFormat( bmp.PixelFormat ) )
					game.Drawer2D.ConvertTo32Bpp( ref bmp );
				
				try {
					SkinType = Utils.GetSkinType( bmp );
					if( Model is PlayerModel || Model is ChibiModel )
						ClearHat( bmp, SkinType );
					PlayerTextureId = game.Graphics.CreateTexture( bmp );
					MobTextureId = -1;
					
					// Custom mob textures.
					if( Utils.IsUrlPrefix( SkinName, 0 ) && item.TimeAdded > lastModelChange )
						MobTextureId = PlayerTextureId;					
				} catch( NotSupportedException ) {
					ResetSkin( bmp );
				}
				bmp.Dispose();
			}
		}
		
		void ResetSkin( Bitmap bmp ) {
			string formatString = "Skin {0} has unsupported dimensions({1}, {2}), reverting to default.";
			Utils.LogDebug( formatString, SkinName, bmp.Width, bmp.Height );
			MobTextureId = -1;
			PlayerTextureId = -1;
			SkinType = game.DefaultPlayerSkinType;
		}
		
		unsafe static void ClearHat( Bitmap bmp, SkinType skinType ) {
			using( FastBitmap fastBmp = new FastBitmap( bmp, true, false ) ) {
				int sizeX = (bmp.Width / 64) * 32;
				int yScale = skinType == SkinType.Type64x32 ? 32 : 64;
				int sizeY = (bmp.Height / yScale) * 16;
				
				// determine if we actually need filtering
				for( int y = 0; y < sizeY; y++ ) {
					int* row = fastBmp.GetRowPtr( y );
					row += sizeX;
					for( int x = 0; x < sizeX; x++ ) {
						byte alpha = (byte)(row[x] >> 24);
						if( alpha != 255 ) return;
					}
				}
				
				// only perform filtering when the entire hat is opaque
				int fullWhite = FastColour.White.ToArgb();
				int fullBlack = FastColour.Black.ToArgb();
				for( int y = 0; y < sizeY; y++ ) {
					int* row = fastBmp.GetRowPtr( y );
					row += sizeX;
					for( int x = 0; x < sizeX; x++ ) {
						int pixel = row[x];
						if( pixel == fullWhite || pixel == fullBlack ) row[x] = 0;
					}
				}
			}
		}
	}
}