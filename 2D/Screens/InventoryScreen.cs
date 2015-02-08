using System;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class InventoryScreen : Screen {
		
		NormalScreen underlyingScreen;
		Texture slotTexture;
		const int blockSize = 40;
		const int hotbarCount = 9;
		SlotWidget[] slots = new SlotWidget[45];
		
		public InventoryScreen( Game window ) : base( window ) {
		}
		
		static FastColour windowCol = new FastColour( 200, 200, 200 );
		const int tableHeight = blockSize * 8 + paddingSize * 4;
		const int tableWidth = blockSize * hotbarCount + paddingSize * 2;
		const int tableBottomOffset = blockSize * 2;
		const int paddingSize = blockSize / 8;
		
		public override void Render( double delta ) {
			underlyingScreen.Render( delta );
			GraphicsApi.Draw2DQuad( 0, 0, Window.Width, Window.Height, new FastColour( 255, 255, 255, 100 ) );
			
			int y = Window.Height - tableBottomOffset - tableHeight;
			int x = Window.Width / 2 - tableWidth / 2;
			GraphicsApi.Draw2DQuad( x, y, tableWidth, tableHeight, windowCol );
			
			GraphicsApi.Texturing = true;
			for( int i = 0; i < slots.Length; i++ ) {
				slots[i].RenderBackground();
			}
			for( int i = 0; i < slots.Length; i++ ) {
				slots[i].RenderForeground();
			}
			GraphicsApi.Texturing = false;
		}
		
		public override void Dispose() {
			GraphicsApi.DeleteTexture( ref slotTexture );
			for( int i = 0; i < slots.Length; i++ ) {
				slots[i].Dispose();
			}
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			underlyingScreen.OnResize( oldWidth, oldHeight, width, height );
			MoveSlots();
		}
		
		public override void Init() {
			underlyingScreen = new NormalScreen( Window );
			underlyingScreen.Init();
			slotTexture = Utils2D.CreateFilledSlot( GraphicsApi, 0, 0, blockSize );
			
			for( int i = 0; i < slots.Length; i++ ) {
				int index = i; // must capture the variable.
				SlotWidget widget = new SlotWidget( Window, slotTexture,
				                                   () => Window.Inventory.GetSlot( index ) );
				widget.Init();
				slots[i] = widget;
			}
			MoveSlots();
		}
		
		void MoveSlots() {
			int y = Window.Height - tableBottomOffset - tableHeight;
			int x = Window.Width / 2 - tableWidth / 2;
			y += paddingSize;
			x += paddingSize;
			
			slots[0].MoveTo( x + 8 * blockSize, y + blockSize * 3 / 2 );
			slots[1].MoveTo( x + 5 * blockSize, y + blockSize );
			slots[2].MoveTo( x + 6 * blockSize, y + blockSize );
			slots[3].MoveTo( x + 5 * blockSize, y + blockSize * 2 );
			slots[4].MoveTo( x + 6 * blockSize, y + blockSize * 2 );
			for( int i = 5; i < 9; i++ ) {
				slots[i].MoveTo( x, y );
				y += blockSize;
			}
			y += paddingSize;
			
			for( int i = 9; i < slots.Length; i++ ) {
				slots[i].MoveTo( x + ( i % 9 ) * blockSize, y );
				if( ( i % 9 ) == 8 ) y += blockSize;
				if( i == 35 ) y += paddingSize;
			}
		}
		
		public override bool HandlesAllInput {
			get { return true; }
		}

		public override bool HandlesKeyPress( char key ) {
			return true;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Window.Keys[KeyMapping.PauseOrExit] ) {
				Window.SetNewScreen( underlyingScreen );
			}
			return true;
		}
	}
}
