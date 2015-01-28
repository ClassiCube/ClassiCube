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
				
				//System.Diagnostics.Debug.WriteLine( " OUT: " + packet.GetType().Name + ", " + id );
			}
		}
		
		void ReadPackets() {
			byte id = reader.ReadUInt8();		
			InboundPacket packet = inboundConstructors[id]();
			//System.Diagnostics.Debug.WriteLine( "  IN: " + packet.GetType().Name + ", " + id );
			packet.ReadData( reader );
			readQueue.Enqueue( packet );
		}
		
		void SetupHandlers() {
			RegisterInbound( 0x00, () => new KeepAliveInbound() );
			RegisterInbound( 0x01, () => new LoginRequestInbound() );
			RegisterInbound( 0x02, () => new HandshakeInbound() );
			RegisterInbound( 0x03, () => new ChatInbound() );
			RegisterInbound( 0x04, () => new TimeUpdateInbound() );
			RegisterInbound( 0x05, () => new EntityEquipmentInbound() );
			RegisterInbound( 0x06, () => new SpawnPositionInbound() );
			RegisterInbound( 0x08, () => new UpdateHealthInbound() );
			RegisterInbound( 0x09, () => new RespawnInbound() );
			RegisterInbound( 0x0D, () => new PlayerPosAndLookInbound() );
			RegisterInbound( 0x11, () => new UseBedInbound() );
			RegisterInbound( 0x12, () => new AnimationInbound() );
			RegisterInbound( 0x14, () => new SpawnPlayerInbound() );
			RegisterInbound( 0x15, () => new SpawnPickupInbound() );
			RegisterInbound( 0x16, () => new CollectItemInbound() );
			RegisterInbound( 0x17, () => new SpawnObjectInbound() );
			RegisterInbound( 0x18, () => new SpawnMobInbound() );
			RegisterInbound( 0x19, () => new SpawnPaintingInbound() );
			RegisterInbound( 0x1C, () => new EntityVelocityInbound() );
			RegisterInbound( 0x1D, () => new DestroyEntityInbound() );
			RegisterInbound( 0x1E, () => new EntityInbound() );
			RegisterInbound( 0x1F, () => new EntityRelativeMoveInbound() );
			RegisterInbound( 0x20, () => new EntityLookInbound() );
			RegisterInbound( 0x21, () => new EntityLookAndRelativeMoveInbound() );
			RegisterInbound( 0x22, () => new EntityTeleportInbound() );
			RegisterInbound( 0x26, () => new EntityStatusInbound() );
			RegisterInbound( 0x27, () => new AttachEntityInbound() );
			RegisterInbound( 0x28, () => new EntityMetadataInbound() );
			RegisterInbound( 0x32, () => new PrepareChunkInbound() );
			RegisterInbound( 0x33, () => new MapChunkInbound() );
			RegisterInbound( 0x34, () => new MultiBlockChangeInbound() );
			RegisterInbound( 0x35, () => new BlockChangeInbound() );
			RegisterInbound( 0x36, () => new BlockActionInbound() );
			RegisterInbound( 0x3C, () => new ExplosionInbound() );
			RegisterInbound( 0x3D, () => new SoundEffectInbound() );
			RegisterInbound( 0x46, () => new ChangeGameStateInbound() );
			RegisterInbound( 0x47, () => new ThunderboltInbound() );
			RegisterInbound( 0x64, () => new OpenWindowInbound() );
			RegisterInbound( 0x65, () => new CloseWindowInbound() );
			RegisterInbound( 0x67, () => new SetSlotInbound() );
			RegisterInbound( 0x68, () => new WindowItemsInbound() );
			RegisterInbound( 0x69, () => new WindowPropertyInbound() );
			RegisterInbound( 0x6A, () => new ConfirmTransactionInbound() );
			RegisterInbound( 0x82, () => new UpdateSignInbound() );
			RegisterInbound( 0x83, () => new MapsInbound() );
			RegisterInbound( 0xC8, () => new StatisticsInbound() );
			RegisterInbound( 0xFF, () => new DisconnectInbound() );
			
			RegisterOutbound( 0x00, typeof( KeepAliveOutbound ) );
			RegisterOutbound( 0x01, typeof( LoginRequestOutbound ) );
			RegisterOutbound( 0x02, typeof( HandshakeOutbound ) );
			RegisterOutbound( 0x03, typeof( ChatOutbound ) );
			RegisterOutbound( 0x07, typeof( UseEntityOutbound ) );
			RegisterOutbound( 0x09, typeof( RespawnOutbound ) );
			RegisterOutbound( 0x0A, typeof( PlayerOutbound ) );
			RegisterOutbound( 0x0B, typeof( PlayerPosOutbound ) );
			RegisterOutbound( 0x0C, typeof( PlayerLookOutbound ) );
			RegisterOutbound( 0x0D, typeof( PlayerPosAndLookOutbound ) );
			RegisterOutbound( 0x0E, typeof( PlayerDiggingOutbound ) );
			RegisterOutbound( 0x0F, typeof( PlayerPlaceBlockOutbound ) );
			RegisterOutbound( 0x10, typeof( HeldItemChangeOutbound ) );
			RegisterOutbound( 0x12, typeof( AnimationOutbound ) );
			RegisterOutbound( 0x13, typeof( EntityActionOutbound ) );
			RegisterOutbound( 0x27, typeof( AttachEntityOutbound ) );
			RegisterOutbound( 0x65, typeof( CloseWindowOutbound ) );
			RegisterOutbound( 0x66, typeof( ClickWindowOutbound ) );
			RegisterOutbound( 0x6A, typeof( ConfirmTransactionOutbound ) );
			RegisterOutbound( 0x82, typeof( UpdateSignOutbound ) );
			RegisterOutbound( 0xFF, typeof( DisconnectOutbound ) );
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
