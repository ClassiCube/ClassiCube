using System;

namespace ClassicalSharp.Window {
	
	public class ChestWindow : Window {
		
		public ChestWindow( string title, byte numSlots, Game game ) : base( title, numSlots, game ) {	
		}
		
		public override void SetProperty( short property, int value ) {
		}
				
		public override Slot GetSlot( int index ) {
			throw new NotImplementedException();
		}
		
		public override void SetAllSlots( Slot[] values ) {
			throw new NotImplementedException();
		}
		
		public override void SetSlot( int index, Slot value ) {
			throw new NotImplementedException();
		}
	}
}
