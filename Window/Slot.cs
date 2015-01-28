using System;

namespace ClassicalSharp.Window {
	
	public class Slot {
		
		public short Id;
		
		public byte Count;
		
		public short Damage;
		
		public bool IsEmpty {
			get { return Id < 0; }
		}
		
		public static Slot CreateEmpty() {
			return new Slot() { Id = -1 };
		}
		
		public override string ToString() {
			if( IsEmpty ) return "(empty slot)";
			return "(" + Id + "," + Damage + ":" + Count + ")";
		}

	}
}
