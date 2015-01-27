using System;
using System.IO;
using System.Text;
using ClassicalSharp.Window;

namespace ClassicalSharp.Network {
	
	public class NetWriter : IDisposable {
		
		public Stream stream;
		private byte[] buff;
		
		public NetWriter( Stream output ) {
			if ( !output.CanWrite ) {
				throw new ArgumentException( "cannot write to stream" );
			}
			stream = output;
			buff = new byte[8];
		}
		
		public void Close() {
			Dispose();
		}

		public void Dispose() {
			stream.Close();
		}
		
		public void Flush() {
			stream.Flush();
		}

		public void WriteBoolean( bool value ) {
			stream.WriteByte( (byte)( value ? 1 : 0 ) );
		}

		public void WriteUInt8( byte value ) {
			stream.WriteByte(value);
		}
		
		public void WriteInt8( sbyte value ) {
			stream.WriteByte( (byte)value );
		}

		public void WriteRawBytes( byte[] buffer ) {
			stream.Write( buffer, 0, buffer.Length );
		}

		public void WriteInt16( short value ) {
			WriteUInt16( (ushort)value );
		}

		public void WriteUInt16( ushort value ) {
			buff[0] = (byte)( value >> 8 );
			buff[1] = (byte)value;
			stream.Write( buff, 0, 2 );
		}

		public void WriteInt32( int value ) {
			WriteUInt32( (uint)value );
		}
		
		public void WriteUInt32( uint value ) {
			buff[0] = (byte)( value >> 24 );
			buff[1] = (byte)( value >> 16 );
			buff[2] = (byte)( value >> 8 );
			buff[3] = (byte)value;
			stream.Write( buff, 0, 4 );
		}
		
		public void WriteInt64( long value ) {
			WriteUInt64( (ulong)value );
		}
		
		public void WriteUInt64( ulong value ) {
			buff[0] = (byte)( value >> 56 );
			buff[1] = (byte)( value >> 48 );
			buff[2] = (byte)( value >> 40 );
			buff[3] = (byte)( value >> 32 );
			buff[4] = (byte)( value >> 24 );
			buff[5] = (byte)( value >> 16 );
			buff[6] = (byte)( value >> 8 );
			buff[7] = (byte)( value );
			stream.Write( buff, 0, 8 );
		}
		
		public unsafe void WriteFloat32( float value ) {
			WriteUInt32( *(uint*)&value );
		}
		
		public unsafe void WriteFloat64( double value ) {
			WriteUInt64( *(ulong*)&value );
		}
		
		public void WriteString( string value ) {
			byte[] bytes = Encoding.BigEndianUnicode.GetBytes( value );
			WriteInt16( (short)value.Length );
			WriteRawBytes( bytes );
		}
		
		public void WriteSlot( Slot slot ) {
			WriteInt16( slot.Id );
			if( !slot.IsEmpty ) {
				WriteUInt8( slot.ItemCount );
				WriteInt16( slot.ItemDamage );
			}
		}
	}
}
