using System;
using System.IO;
using ClassicalSharp;
using OpenTK;

namespace DefaultPlugin.Network {

	public partial class ClassicNetworkProcessor : NetworkProcessor {
		
		static byte[] outBuffer = new byte[131];
		static int outIndex = 0;
		private static int MakeLoginPacket( string username, string verKey ) {
			WriteUInt8( 0 ); // packet id
			WriteUInt8( 7 ); // protocol version
			WriteString( username );
			WriteString( verKey );
			WriteUInt8( 0x42 );
			return 1 + 1 + 64 + 64 + 1;
		}
		
		private static int MakeSetBlockPacket( short x, short y, short z, byte mode, byte block ) {
			WriteUInt8( 0x05 ); // packet id
			WriteInt16( x );
			WriteInt16( y );
			WriteInt16( z );
			WriteUInt8( mode );
			WriteUInt8( block );
			return 1 + 3 * 2 + 1 + 1;
		}
		
		private static int MakePositionPacket( Vector3 pos, byte yaw, byte pitch, byte playerId ) {
			WriteUInt8( 0x08 ); // packet id
			WriteUInt8( playerId ); // player id (-1 is self)
			WriteInt16( (short)( pos.X * 32 ) );
			WriteInt16( (short)( (int)( pos.Y * 32 ) + 51 ) );
			WriteInt16( (short)( pos.Z * 32 ) );
			WriteUInt8( yaw );
			WriteUInt8( pitch );
			return 1 + 1 + 3 * 2 + 2 * 1;
		}
		
		private static int MakeMessagePacket( string text ) {
			WriteUInt8( 0x0D ); // packet id
			WriteUInt8( 0xFF ); // unused
			WriteString( text );
			return 1 + 1 + 64;
		}
		
		private static int MakeExtInfo( string appName, int extensionsCount ) {
			WriteUInt8( 0x10 ); // packet id
			WriteString( appName );
			WriteInt16( (short)extensionsCount );
			return 1 + 64 + 2;
		}
		
		private static int MakeExtEntry( string extensionName, int extensionVersion ) {
			WriteUInt8( 0x11 ); // packet id
			WriteString( extensionName );
			WriteInt32( extensionVersion );
			return 1 + 64 + 4;
		}
		
		private static int MakeCustomBlockSupportLevel( byte version ) {
			WriteUInt8( 19 ); // packet id
			WriteUInt8( version );
			return 1 + 1;
		}
		
		static void WriteString( string value ) {
			int count = Math.Min( value.Length, 64 );
			for( int i = 0; i < count; i++ ) {
				char c = value[i];
				outBuffer[outIndex + i] = (byte)( c >= '\u0080' ? '?' : c );
			}
			for( int i = value.Length; i < 64; i++ ) {
				outBuffer[outIndex + i] = (byte)' ';
			}
			outIndex += 64;
		}
		
		static void WriteUInt8( byte value ) {
			outBuffer[outIndex++] = value;
		}
		
		static void WriteInt16( short value ) {
			outBuffer[outIndex++] = (byte)( value >> 8 );
			outBuffer[outIndex++] = (byte)( value );
		}
		
		static void WriteInt32( int value ) {
			outBuffer[outIndex++] = (byte)( value >> 24 );
			outBuffer[outIndex++] = (byte)( value >> 16 );
			outBuffer[outIndex++] = (byte)( value >> 8 );
			outBuffer[outIndex++] = (byte)( value );
		}
		
		void WritePacket( int packetLength ) {
			outIndex = 0;
			if( Disconnected ) return;
			
			try {
				stream.Write( outBuffer, 0, packetLength );
			} catch( IOException ex ) {
				Utils.LogError( "Error while writing packets: {0}{1}", Environment.NewLine, ex );
				Window.Disconnect( "&eLost connection to the server", "Underlying connection was closed" );
				Dispose();
			}
		}
	}
}