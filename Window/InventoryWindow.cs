using System;
using ClassicalSharp.Network.Packets;

namespace ClassicalSharp.Window {
	
	public class InventoryWindow : Window {
		
		Slot[] slots;
		public InventoryWindow( string title, byte numSlots, Game game ) : base( title, numSlots, game ) {
			slots = new Slot[numSlots];
		}
		
		public override void SetProperty( short property, int value ) {
		}
		
		public short HeldSlotIndex = 0;
		public Slot HeldSlot {
			get { return slots[36 + HeldSlotIndex]; }
			set { slots[36 + HeldSlotIndex] = value; }
		}
		
		public Slot GetHotbarSlot( int index ) {
			return slots[36 + index];
		}
		
		public void SetAndSendHeldSlotIndex( short newIndex ) {
			HeldSlotIndex = newIndex;
			game.Network.SendPacket( new HeldItemChangeOutbound( newIndex ) );
		}
				
		public override Slot GetSlot( int index ) {
			return slots[index];
		}
		
		public override void SetAllSlots( Slot[] values ) {
			slots = values;
		}
		
		public override void SetSlot( int index, Slot value ) {
			slots[index] = value;
		}
	}
}
