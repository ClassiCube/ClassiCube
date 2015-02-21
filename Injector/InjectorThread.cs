using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace Injector {

	public partial class InjectorThread {
		
		TcpClient c2sClient, s2cClient;
		NetworkStream s2cStream, c2sStream;
		StreamInjector s2cReader, c2sReader;
		
		public InjectorThread( TcpClient client, IPEndPoint serverIp ) {
			c2sClient = client;
			s2cClient = new TcpClient();
			s2cClient.Connect( serverIp );
		}
		
		public void Run() {
			s2cStream = new NetworkStream( s2cClient.Client );
			c2sStream = new NetworkStream( c2sClient.Client );
			s2cReader = new StreamInjector( s2cStream, c2sStream );
			c2sReader = new StreamInjector( c2sStream, s2cStream );
			SetupHandlers();
			SetupLogging();
			
			try {
				while( true ) {
					while( s2cStream.DataAvailable ) {
						ReadS2C();
					}
					while( c2sStream.DataAvailable ) {
						ReadC2S();
					}
					Thread.Sleep( 1 );
				}
			} catch( Exception ex ) {
				Console.WriteLine( ex );
			} finally {
				c2sClient.Close();
				s2cClient.Close();
			}
		}
		
		void ReadS2C() {
			byte id = s2cReader.ReadUInt8();
			PacketS2C packet = inboundConstructors[id]();
			packet.ReadData( s2cReader );
			LogS2C( id, packet );
		}
		
		void ReadC2S() {
			byte id = c2sReader.ReadUInt8();
			PacketC2S packet = outboundConstructors[id]();
			packet.ReadData( c2sReader );
			LogC2S( id, packet );
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
			
			RegisterOutbound( 0x00, () => new KeepAliveOutbound() );
			RegisterOutbound( 0x01, () => new LoginRequestOutbound() );
			RegisterOutbound( 0x02, () => new HandshakeOutbound() );
			RegisterOutbound( 0x03, () => new ChatOutbound() );
			RegisterOutbound( 0x07, () => new UseEntityOutbound() );
			RegisterOutbound( 0x09, () => new RespawnOutbound() );
			RegisterOutbound( 0x0A, () => new PlayerOutbound() );
			RegisterOutbound( 0x0B, () => new PlayerPosOutbound() );
			RegisterOutbound( 0x0C, () => new PlayerLookOutbound() );
			RegisterOutbound( 0x0D, () => new PlayerPosAndLookOutbound() );
			RegisterOutbound( 0x0E, () => new PlayerDiggingOutbound() );
			RegisterOutbound( 0x0F, () => new PlayerPlaceBlockOutbound() );
			RegisterOutbound( 0x10, () => new HeldItemChangeOutbound() );
			RegisterOutbound( 0x12, () => new AnimationOutbound() );
			RegisterOutbound( 0x13, () => new EntityActionOutbound() );
			RegisterOutbound( 0x27, () => new AttachEntityOutbound() );
			RegisterOutbound( 0x65, () => new CloseWindowOutbound() );
			RegisterOutbound( 0x66, () => new ClickWindowOutbound() );
			RegisterOutbound( 0x6A, () => new ConfirmTransactionOutbound() );
			RegisterOutbound( 0x82, () => new UpdateSignOutbound() );
			RegisterOutbound( 0xFF, () => new DisconnectOutbound() );
		}
		
		void RegisterOutbound( byte opcode, Func<PacketC2S> cons ) {
			outboundConstructors[opcode] = cons;
		}
		
		void RegisterInbound( byte opcode, Func<PacketS2C> cons ) {
			inboundConstructors[opcode] = cons;
		}
		
		Func<PacketC2S>[] outboundConstructors = new Func<PacketC2S>[256];
		Func<PacketS2C>[] inboundConstructors = new Func<PacketS2C>[256];
	}
}
