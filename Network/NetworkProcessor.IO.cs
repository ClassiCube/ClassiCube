using System;
using System.Threading;
using ClassicalSharp.Network.Packets;
using ClassicalSharp.Util;

namespace ClassicalSharp {
	
	public partial class NetworkProcessor {
		
		ConcurrentQueue<InboundPacket> readQueue = new ConcurrentQueue<InboundPacket>();
		ConcurrentQueue<OutboundPacket> writeQueue = new ConcurrentQueue<OutboundPacket>();
		public bool StopProcessing = false;
		
		public void IoThread() {
			while( !StopProcessing ) {
				while( stream.DataAvailable ) {
					ReadPackets();
				}
				WritePackets();
				Thread.Sleep( 1 );
			}
		}
		
		Thread thread;
		public void RunIoThreadAsync() {
			thread = new Thread( IoThread );
			thread.Name = "ClassicalSharp.NetIoThread";
			thread.Start();
		}
		
		void WritePackets() {
			OutboundPacket packet = null;
			while( writeQueue.Dequeue( ref packet ) ) {
				int id = Array.IndexOf( outboundTypes, packet.GetType() );
				if( id < 0 ) throw new InvalidOperationException( "invalid packet" );
				
				writer.WriteUInt8( (byte)id );
				packet.WriteData( writer );
				
				System.Diagnostics.Debug.WriteLine( " OUT: " + packet.GetType().Name + ", " + id );
			}
		}
		
		void ReadPackets() {
			byte id = reader.ReadUInt8();		
			InboundPacket packet = inboundConstructors[id]();
			System.Diagnostics.Debug.WriteLine( "  IN: " + packet.GetType().Name + ", " + id );
			packet.ReadData( reader );
			readQueue.Enqueue( packet );
		}
		
		void SetupHandlers() {
		}
		
		void RegisterOutbound( byte opcode, Type outboundType ) {
			outboundTypes[opcode] = outboundType;
		}
		
		void RegisterInbound( byte opcode, Func<InboundPacket> cons ) {
			inboundConstructors[opcode] = cons;
		}
		
		public Type[] outboundTypes = new Type[256];
		public Func<InboundPacket>[] inboundConstructors = new Func<InboundPacket>[256];
	}
}
