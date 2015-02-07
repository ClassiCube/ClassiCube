using System;
using System.Collections.Generic;
using ClassicalSharp.Window;

namespace ClassicalSharp.Window {
	
	public sealed class WindowManager {
		
		public Window[] Windows = new Window[256];	
		public Game game;
		
		public WindowManager( Game game ) {
			this.game = game;
			Windows[0] = game.Inventory;
		}
		
		public void OpenWindow( byte id, Window window ) {
			if( id != 0 ) {
				Windows[id] = window;
				window.Id = id;
				window.OnOpen();
			}
		}
		
		public void CloseWindow( byte id ) {
			if( id != 0 ) {
				Window window = Windows[id];
				if( window != null ) {
					Windows[id] = null;
					window.OnClose();
				}
			}
		}
		
		public void SetSlot( byte id, int index, Slot slot ) {
			Window window = Windows[id];
			if( window != null ) {
				window.SetSlot( index, slot );
			}
		}
		
		public void SetAllSlots( byte id, Slot[] slots ) {
			Window window = Windows[id];
			if( window != null ) {
				window.SetAllSlots( slots );
			}
		}
		
		public void SetProperty( byte id, short property, short value ) {
			Window window = Windows[id];
			if( window != null ) {
				window.SetProperty( property, value );
			}
		}
		
		public static Window GetFromType( byte type, string title, byte slots, Game game ) {
			if( type == 0 ) return new ChestWindow( title, slots, game );
			if( type == 1 ) return new WorkbenchWindow( title, slots, game );
			if( type == 2 ) return new FurnaceWindow( title, slots, game );
			if( type == 3 ) return new DispenserWindow( title, slots, game );
			return null;
		}
	}
}
