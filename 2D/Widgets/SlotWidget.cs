using System;
using System.Drawing;
using ClassicalSharp.Window;

namespace ClassicalSharp {
	
	public sealed class SlotWidget : Widget {
		
		public SlotWidget( Game window, Texture slotTexture, Func<Slot> getSlotFunc ) : base( window ) {
			this.slotTexture = slotTexture;
			this.getSlotFunc = getSlotFunc;
		}
		
		Texture slotTexture;
		Texture countTexture;
		int count = -1;
		const int blockSize = 40;
		Func<Slot> getSlotFunc;
		
		public override void Init() {
			Width = slotTexture.Width = blockSize;
			Height = slotTexture.Height = blockSize;
		}
		
		public override void Render( double delta ) {
		}
		
		public override void Dispose() {
			GraphicsApi.DeleteTexture( ref countTexture );
		}
		
		public void RenderBackground() {
			GraphicsApi.Bind2DTexture( slotTexture.ID );
			slotTexture.X1 = X;
			slotTexture.Y1 = Y;
			slotTexture.RenderNoBind( GraphicsApi );
		}
		
		public void RenderForeground() {
			Slot slot = getSlotFunc();
			if( slot.IsEmpty ) return;
			
			if( count == -1 || count != slot.Count ) {
				GraphicsApi.DeleteTexture( ref countTexture );
				count = slot.Count;
				DrawTextArgs args = new DrawTextArgs( GraphicsApi, count.ToString(), Color.White, true );
				countTexture = Utils2D.MakeTextTexture( "Arial", 12, 0, 0, args );
			}
			
			if( slot.Id <= 255 ) {
				RenderBlock( slot );
			} else {
				RenderItem( slot );
			}
			GraphicsApi.Bind2DTexture( countTexture.ID );
			countTexture.X1 = ( X + 36 ) - countTexture.Width;
			countTexture.Y1 = ( Y + 36 ) - countTexture.Height + 3; // add 3 to account for measured size being over conversative
			countTexture.RenderNoBind( GraphicsApi );
		}
		
		void RenderBlock( Slot slot ) {
			int texId = Window.BlockInfo.GetOptimTextureLoc( (byte)slot.Id, TileSide.Front );
			if( texId == 0 ) return;
			
			TextureRectangle rec = Window.TerrainAtlas.GetTexRec( texId );
			float height = Window.BlockInfo.BlockHeight( (byte)slot.Id );
			rec.V2 = rec.V1 + Window.TerrainAtlas.invVerElementSize * height;
			
			Texture tex = new Texture( -1, X + 4, Y + 4, 32, (int)( 32 * height ), rec );
			GraphicsApi.Bind2DTexture( Window.TerrainAtlasTexId );
			tex.RenderNoBind( GraphicsApi );
		}
		
		void RenderItem( Slot slot ) {
			int texId = Window.ItemInfo.Get2DTextureLoc( slot );
			if( texId == 0 ) return;
			
			TextureRectangle rec = Window.TerrainAtlas.GetTexRec( texId );
			Texture tex = new Texture( -1, X + 4, Y + 4, 32, 32, rec );
			GraphicsApi.Bind2DTexture( Window.ItemsAtlasTexId );
			tex.RenderNoBind( GraphicsApi );
		}
	}
}