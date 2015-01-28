using System;

namespace ClassicalSharp.Util {
	
	public class NibbleArray {
		
		public byte[] Data;
		
		public int Elements { 
			get { return Data.Length * 2; } 
		}
		
		public NibbleArray( byte[] data ) {
			Data = data;
		}
		
		public NibbleArray( int elements ) {
			Data = new byte[elements / 2];
		}
		
		public byte this[int index] {
			get {
				int packed = Data[index >> 1];
				//return ( index & 0x01 ) == 0 ? packed & 0x0F : ( packed >> 4 ) & 0x0F;
				return (byte)( packed >> ( ( index & 0x01 ) << 2 ) );
			}
			set {
				int packed = Data[index >> 1];
				//Data[index >> 1 ] = ( index & 0x01 ) == 0 ?
				//	(byte)( ( packed & 0xF0 ) | value ) :
				//	(byte)( ( packed & 0x0F ) | ( value << 4 ) );
				packed &= 0xF << ( ( ( index + 1 ) & 0x01 ) << 2 );
				packed |= value << ( ( index & 0x01 ) << 2 );
				Data[index >> 1] = (byte)packed;
			}
		}
	}
}
