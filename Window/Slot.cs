using System;
using System.IO;
using ClassicalSharp.Network;
using fNbt;

namespace ClassicalSharp.Window {
	
	public class Slot {
		
		public short Id;
		
		public byte ItemCount;
		
		public short ItemDamage;
		
		public NbtFile NbtMetadata;
		
		public bool IsEmpty {
			get { return Id < 0; }
		}
		
		public static Slot CreateEmpty() {
			return new Slot() { Id = -1 };
		}
		
		public override string ToString() {
			if( IsEmpty ) return "(empty slot)";
			return "(" + Id + "," + ItemDamage + ":" + ItemCount + ")";
		}

	}
}
