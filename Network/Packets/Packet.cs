using System;

namespace ClassicalSharp.Network.Packets {
	
	public abstract class InboundPacket {
		
		public abstract void ReadData( NetReader reader );
		
		public abstract void ReadCallback( Game game );
	}
	
	public abstract class OutboundPacket {
		
		public abstract void WriteData( NetWriter writer );
	}
}
