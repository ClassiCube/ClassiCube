using System;
using System.Collections.Generic;
using ClassicalSharp.Window;

namespace ClassicalSharp.Window {

	public abstract class Window {
		
		public string Title;
		
		public byte Id;
		
		public Game game;
		
		public Window( string title, byte slots, Game game ) {
			Title = title;
			this.game = game;
		}
		
		protected static Slot[] CreateEmptySlots( int count ) {
			Slot[] slots = new Slot[count];
			for( int i = 0; i < slots.Length; i++ ) {
				slots[i] = Slot.CreateEmpty();
			}
			return slots;
		}
		
		public abstract void SetProperty( short property, int value );
		
		// We can't declare this in the base class because
		// windows share inventory.
		public abstract void SetSlot( int index, Slot value );
		
		public abstract void SetAllSlots( Slot[] values );
		
		public abstract Slot GetSlot( int index );
		
		public virtual void OnOpen() {			
		}
		
		public virtual void OnClose() {			
		}
	}
}
