using System;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class InventoryScreen : Screen {
		
		NormalScreen underlyingScreen;
		Texture slotTexture;
		const int blockSize = 40;
		const int hotbarCount = 9;
		SlotWidget[] hotbarSlots = new SlotWidget[hotbarCount];
		
		public InventoryScreen( Game window ) : base( window ) {
		}
		
		static FastColour windowCol = new FastColour( 200, 200, 200 );
		public override void Render( double delta ) {
			underlyingScreen.Render( delta );
			GraphicsApi.Draw2DQuad( 0, 0, Window.Width, Window.Height, new FastColour( 255, 255, 255, 100 ) );
			
			int y = Window.Height - blockSize - blockSize - blockSize;
			int x = Window.Width / 2 - ( blockSize * hotbarCount ) / 2;
			GraphicsApi.Draw2DQuad( x - blockSize / 4, y, 
			                       blockSize * hotbarCount + blockSize / 2, blockSize + blockSize / 4, windowCol );
			
			GraphicsApi.Texturing = true;
			for( int i = 0; i < hotbarCount; i++ ) {
				hotbarSlots[i].RenderBackground();
			}	
			for( int i = 0; i < hotbarCount; i++ ) {
				hotbarSlots[i].RenderForeground();
			}
			GraphicsApi.Texturing = false;
		}
		
		public override void Dispose() {
			GraphicsApi.DeleteTexture( ref slotTexture );
			for( int i = 0; i < hotbarCount; i++ ) {
				hotbarSlots[i].Dispose();
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
			
			for( int i = 0; i < hotbarCount; i++ ) {
				int index = i; // must capture the variable.
				SlotWidget widget = new SlotWidget( Window, slotTexture,
				                                   () => Window.Inventory.GetHotbarSlot( index ) );
				widget.Init();
				hotbarSlots[i] = widget;
			}
			MoveSlots();
		}
		
		void MoveSlots() {
			int y = Window.Height - blockSize - blockSize - blockSize;
			int x = Window.Width / 2 - ( blockSize * hotbarCount ) / 2;
			
			for( int i = 0; i < hotbarCount; i++ ) {
				hotbarSlots[i].MoveTo( x + i * blockSize, y );
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
