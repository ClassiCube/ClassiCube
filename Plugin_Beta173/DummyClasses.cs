using System;

namespace PluginBeta173 {
	
	// TODO: Use proper implementation
	public sealed class Slot {
		public short Id;
		public byte Count;
		public short Damage;
		
		public bool IsEmpty {
			get { return Id <= 0; }
		}
	}
}
