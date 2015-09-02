using System;
using System.Reflection;

namespace ClassicalSharp {
	
	// Class used to minimise memory allocations of strings.
	// Really, only useful for FpsScreen and TextInputWidget as they allocate lots of very small strings.
	// Seriously, you should *not* use this anywhere else, as Empty and Length are O(N).
	internal sealed unsafe class StringBuffer {
		
		internal string value;
		internal int capacity;
		
		static readonly FieldInfo arrayField, lengthField;
		static bool supportsLengthSetting;
		
		static StringBuffer() {
			if( OpenTK.Configuration.RunningOnMono )
				return;
			
			arrayField = typeof( String ).GetField( "m_arrayLength", BindingFlags.NonPublic | BindingFlags.Instance );
			lengthField = typeof( String ).GetField( "m_stringLength", BindingFlags.NonPublic | BindingFlags.Instance );
			
			// Make sure we are running on a framework that has both methods -
			// otherwise we play it safe and just use TrimEnd().
			if( arrayField != null && lengthField != null ) {
				supportsLengthSetting = true;
			} else {
				arrayField = null;
				lengthField = null;
			}
		}
		
		public StringBuffer( int capacity ) {
			this.capacity = capacity;
			value = new String( '\0', capacity );
		}
		
		public StringBuffer Append( int index, char c ) {
			fixed( char* ptr = value ) {
				ptr[index] = c;
				return this;
			}
		}
		
		public StringBuffer Append( int index, string s ) {
			fixed( char* ptr = value ) {
				char* offsetPtr = ptr + index;
				return Append( ref offsetPtr, s );
			}
		}
		
		public StringBuffer Append( ref char* ptr, string s ) {
			for( int i = 0; i < s.Length; i++ ) {
				*ptr++ = s[i];
			}
			return this;
		}
		
		public StringBuffer Append( ref char* ptr, char c) {
			*ptr++ = c;
			return this;
		}
		
		static char[] numBuffer = new char[20];
		public StringBuffer AppendNum( ref char* ptr, long num ) {
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
		
		public StringBuffer Clear() {
			fixed( char* ptr = value ) {
				return Clear( ptr );
			}
		}
		
		public StringBuffer Clear( char* ptr ) {
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
				fixed( char* ptr = value ) {
					for( int i = 0; i < capacity; i++ ) {
						if( ptr[i] != '\0' )
							return false;
					}
				}
				return true;
			}
		}
		
		public int Length {
			get {
				int len = capacity;
				fixed( char* ptr = value ) {
					for( int i = capacity - 1; i >= 0; i-- ) {
						if( ptr[i] != '\0' )
							break;
						len--;
					}
				}
				return len;
			}
		}
		
		/// <summary> Hack that modifies the underlying string's length to avoid memory allocations. </summary>
		/// <returns> The underlying string - ***do not*** store this because it is mutable!
		/// You should only use this string for temporary measuring and drawing. </returns>
		public string UpdateCachedString() {
			return supportsLengthSetting ? value : value.TrimEnd( trimEnd );
		}
		static char[] trimEnd = { '\0' };
		
		public override string ToString() {
			return GetCopy( Length );
		}
		
		public string GetSubstring( int length ) {
			return supportsLengthSetting ? SetLength( length ) : GetCopy( length );
		}
		
		public void RestoreLength() {
			if( supportsLengthSetting )
				SetLength( Length );
		}
		
		string SetLength( int len ) {
			arrayField.SetValue( value, len + 1 );
			lengthField.SetValue( value, len );
			return value;
		}
		
		string GetCopy( int len ) {
			string copiedString = new String( '\0', len );
			fixed( char* src = value, dst = copiedString ) {
				for( int i = 0; i < len; i++ ) {
					dst[i] = src[i];
				}
			}
			return copiedString;
		}	
	}
}
