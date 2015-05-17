using System;

namespace ClassicalSharp {
	
	// Class used to minimise memory allocations of strings. 
	// Really, only useful for FpsScreen and TextInputWidget as they allocate lots of very small strings.
	// Seriously, you should *not* use this anywhere else, as Empty and Length are O(N).
	internal unsafe class UnsafeString {
		
		internal string value;
		internal int capacity;
		
		public UnsafeString( int capacity ) {
			this.capacity = capacity;
			value = new String( '\0', capacity );
		}
		
		public UnsafeString Append( int index, char c ) {
			fixed( char* ptr = value ) {
				ptr[index] = c;
				return this;
			}
		}
		
		public UnsafeString Append( int index, string s ) {
			fixed( char* ptr = value ) {
				char* offsetPtr = ptr + index;
				return Append( ref offsetPtr, s );
			}
		}
		
		public UnsafeString Append( ref char* ptr, string s ) {
			for( int i = 0; i < s.Length; i++ ) {
				*ptr++ = s[i];
			}
			return this;
		}
		
		static char[] numBuffer = new char[20];
		public UnsafeString AppendNum( ref char* ptr, long num ) {
			int index = 0;
			numBuffer[index++] = (char)( '0' + ( num % 10 ) );
			num /= 10;
			
			while( num > 0 ) {
				numBuffer[index++] = (char)( '0' + ( num % 10 ) );
				num /= 10;
			}
			for( int i = index - 1; i >= 0; i-- ) {
				*ptr++ = numBuffer[i];
			}
			return this;
		}
		
		public UnsafeString Clear() {
			fixed( char* ptr = value ) {
				return Clear( ptr );
			}
		}
		
		public UnsafeString Clear( char* ptr ) {
			for( int i = 0; i < capacity; i++ ) {
				*ptr++ = '\0';
			}
			return this;
		}
		
		public void DeleteAt( int index ) {
			fixed( char* ptr = value ) {
				char* offsetPtr = ptr + index;
				for( int i = index; i < capacity - 1; i++ ) {
					*offsetPtr = *( offsetPtr + 1 );
					offsetPtr++;
				}
				*offsetPtr = '\0';
			}
		}
		
		public void InsertAt( int index, char c ) {
			fixed( char* ptr = value ) {
				char* offsetPtr = ptr + capacity - 1;
				for( int i = capacity - 1; i > index; i-- ) {
					*offsetPtr = *( offsetPtr - 1 );
					offsetPtr--;
				}
				*offsetPtr = c;
			}
		}
		
		
		public bool Empty {
			get {
				for( int i = 0; i < capacity; i++ ) {
					if( value[i] != '\0' ) 
						return false;
				}
				return true;
			}
		}
		
		public int Length {
			get {
				int len = capacity;
				for( int i = capacity - 1; i >= 0; i-- ) {
					if( value[i] != '\0' ) 
						break;
					len--;
				}
				return len;
			}
		}
		
		public override string ToString() {
			return value.TrimEnd( '\0' );
		}
	}
}
