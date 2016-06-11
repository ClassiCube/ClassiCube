// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public partial class InventoryScreen : Screen {
		
		public override bool HandlesAllInput { get { return true; } }
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			if( draggingMouse ) {
				mouseY -= TableY;
				scrollY = (int)((mouseY - mouseOffset) / ScrollbarScale);
				ClampScrollY();
				return true;
			}
			
			selIndex = -1;
			if( Contains( startX, startY, blocksPerRow * blockSize,
			             maxRows * blockSize, mouseX, mouseY ) ) {
				for( int i = 0; i < blocksTable.Length; i++ ) {
					int x, y;
					GetCoords( i, out x, out y );
					
					if( Contains( x, y, blockSize, blockSize, mouseX, mouseY ) ) {
						selIndex = i;
						break;
					}
				}
			}
			RecreateBlockInfoTexture();
			return true;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( draggingMouse || game.hudScreen.hotbar.HandlesMouseClick( mouseX, mouseY, button ) )
				return true;
			if( button == MouseButton.Left && mouseX >= TableX - scrollbarWidth && mouseX < TableX ) {
				ScrollbarClick( mouseY );
			} else if( button == MouseButton.Left ) {
				if( selIndex != -1 )
					game.Inventory.HeldBlock = blocksTable[selIndex];
				else if( Contains( TableX, TableY, TableWidth, TableHeight, mouseX, mouseY ) )
					return true;
				
				bool hotbar = game.IsKeyDown( Key.AltLeft ) || game.IsKeyDown( Key.AltRight );
				if( !hotbar )
					game.SetNewScreen( null );
			}
			return true;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == game.Mapping( KeyBind.PauseOrExit ) ||
			   key == game.Mapping( KeyBind.Inventory ) ) {
				game.SetNewScreen( null );
			} else if( key == Key.Enter && selIndex != -1 ) {
				game.Inventory.HeldBlock = blocksTable[selIndex];
				game.SetNewScreen( null );
			} else if( (key == Key.Left || key == Key.Keypad4) && selIndex != -1 ) {
				ArrowKeyMove( -1 );
			} else if( (key == Key.Right || key == Key.Keypad6) && selIndex != -1 ) {
				ArrowKeyMove( 1 );
			} else if( (key == Key.Up || key == Key.Keypad8) && selIndex != -1 ) {
				ArrowKeyMove( -blocksPerRow );
			} else if( (key == Key.Down || key == Key.Keypad2) && selIndex != -1 ) {
				ArrowKeyMove( blocksPerRow );
			} else if( key >= Key.Number1 && key <= Key.Number9 ) {
				game.Inventory.HeldBlockIndex = (int)key - (int)Key.Number1;
			}
			return true;
		}
		
		void ArrowKeyMove( int delta ) {
			int startIndex = selIndex;
			selIndex += delta;
			if( selIndex < 0 )
				selIndex -= delta;
			if( selIndex >= blocksTable.Length )
				selIndex -= delta;
			
			int scrollDelta = (selIndex / blocksPerRow) - (startIndex / blocksPerRow);
			scrollY += scrollDelta;
			ClampScrollY();
			RecreateBlockInfoTexture();
			MoveCursorToSelected();
		}
	}
}
