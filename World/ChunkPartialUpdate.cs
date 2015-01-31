using System;

namespace ClassicalSharp.World {
	
	/// <summary> Used in multi block change packets and partial
	/// chunk data packets. </summary>
	public struct ChunkPartialUpdate {
		
		public bool Modified;
		public bool X0Modified;
		public bool Z0Modified;
		public bool X15Modified;
		public bool Z15Modified;
		public bool Y0Modified;
		public bool Y15Modified;
		
		public void UpdateModifiedState( int x, int y, int z ) {
			Modified = true;
			y = y & 0x0F;
			if( !X0Modified ) X0Modified = x == 0;
			if( !X15Modified ) X15Modified = x == 15;
			if( !Z0Modified ) Z0Modified = z == 0;
			if( !Z15Modified ) Z15Modified = z == 15;
			if( !Y0Modified ) Y0Modified = y == 0;
			if( !Y15Modified ) Y15Modified = y == 15;
		}
	}
}
