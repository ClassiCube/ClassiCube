using System;

namespace Injector {
	
	// NOTE: These delegates should be removed when using versions later than NET 2.0.
	// ################################################################
	public delegate void Action<T1, T2>( T1 arg1, T2 arg2 );
	public delegate TResult Func<TResult>();
	// ################################################################
	
	public static class Utils {
		
		public static double PackedToDegrees( byte packed ) {
			return packed * 360.0 / 256.0;
		}
		
		public static string ToSpacedHexString( byte[] array ) {
			int len = array.Length;
			char[] chars = new char[len * 3];
			for( int i = 0; i < array.Length; i++ ) {
				int value = array[i];
				int index = i * 3;
				int upperNibble = value >> 4;
				int lowerNibble = value & 0x0F;
				chars[index] = upperNibble < 10 ? (char)( upperNibble + 48 ) : (char)( upperNibble + 55 ); // 48 = index of 0, 55 = index of (A - 10).
				chars[index + 1] = lowerNibble < 10 ? (char)( lowerNibble + 48 ) : (char)( lowerNibble + 55 );
				chars[index + 2] = ' ';
			}
			return new String( chars );
		}
	}
	
	public class Slot {
		
		public short Id;
		
		public byte Count;
		
		public short Damage;
		
		public bool IsEmpty {
			get { return Id < 0; }
		}
		
		public static Slot CreateEmpty() {
			return new Slot() { Id = -1 };
		}
		
		const string format = "({0} of [{1},{2}])";
		public override string ToString() {
			if( IsEmpty ) return "(empty slot)";
			return String.Format( format, Count, Id, Damage );
		}
	}
}