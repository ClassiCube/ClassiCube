using System;

namespace ClassicalSharp.Window {
	
	public class FurnaceWindow : Window {
		
		public FurnaceWindow( string title, byte numSlots, Game game ) : base( title, numSlots, game ) {
		}
		
		int burnTime; // progress arrow
		int fuelAmount;
		public override void SetProperty( short property, int value ) {
			if( property == 0 ) burnTime = value;
			else if( property == 1 ) fuelAmount = value;
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
