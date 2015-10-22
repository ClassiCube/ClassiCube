using System;

namespace ClassicalSharp {
	
	public sealed class StringBuffer {
		
		char[] value, wrap;
		int capacity;
		
		public StringBuffer( int capacity ) {
			this.capacity = capacity;
			value = new char[capacity];
			wrap = new char[capacity];
		}
		
		public StringBuffer Append( int index, char c ) {
			value[index] = c;
			return this;
		}
		
		public StringBuffer Append( int index, string s ) {
			return Append( ref index, s );
		}
		
		public StringBuffer Append( ref int index, string s ) {
			for( int i = 0; i < s.Length; i++ )
				value[index++] = s[i];
			return this;
		}
		
		public StringBuffer Append( ref int index, char c) {
			value[index++] = c;
			return this;
		}
		
		static char[] numBuffer = new char[20];
		public StringBuffer AppendNum( ref int index, long num ) {
			int numIndex = 0;
			numBuffer[numIndex++] = (char)('0' + (num % 10));
			num /= 10;
			
			while( num > 0 ) {
				numBuffer[numIndex++] = (char)('0' + (num % 10));
				num /= 10;
			}
			
			for( int i = numIndex - 1; i >= 0; i-- )
				value[index++] = numBuffer[i];
			return this;
		}
		
		public StringBuffer Clear() {
			for( int i = 0; i < capacity; i++ )
				value[i] = '\0';
			return this;
		}
		
		public void DeleteAt( int index ) {
			for( int i = index; i < capacity - 1; i++ )
				value[i] = value[i + 1];
			value[capacity - 1] = '\0';
		}
		
		public void InsertAt( int index, char c ) {
			for( int i = capacity - 1; i > index; i-- )
				value[i] = value[i - 1];
			value[index] = c;
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
		
		public string GetString() {
			return new String( value, 0, Length );
		}
		
		public string GetSubstring( int length ) {
			return new String( value, 0, length );
		}
		
		public void WordWrap( ref string[] lines, ref int[] lineLens, int lineSize ) {
			int len = Length;
			for( int i = 0; i < lines.Length; i++ ) {
				lines[i] = null;
				lineLens[i] = 0;
			}
			
			// Need to make a copy because we mutate the characters.
			char[] realText = value;
			MakeWrapCopy();
			
			int linesCount = 0;
			for( int index = 0; index < capacity; index += lineSize ) {
				if( value[index] == '\0' )
					break;
				
				int lineEnd = index + (lineSize - 1);
				int nextLine = index + lineSize;
				linesCount++;
				
				// Do we need word wrapping
				bool needWrap = !IsWrapper( value[lineEnd] ) && nextLine < capacity && !IsWrapper( value[nextLine] );
				int wrappedLen = needWrap ? WrapLine( index, lineSize ) : lineSize;
				
				// Calculate the maximum size of this line
				int lineLen = lineSize;
				for( int i = lineEnd; i >= index; i-- ) {
					if( value[i] != '\0' ) break;
					lineLen--;
				}
				lineLens[index / lineSize] = Math.Min( lineLen, wrappedLen );
			}
			
			// Output the used lines
			OutputLines( ref lines, linesCount, lineSize );
			value = realText;
		}
		
		void MakeWrapCopy() {
			int len = Length;
			for( int i = 0; i < len; i++ )
				wrap[i] = value[i];
			
			for( int i = len; i < capacity; i++ )
				wrap[i] = '\0';		
			value = wrap;
		}
		
		void OutputLines( ref string[] lines, int linesCount, int lineSize ) {
			for( int i = 0; i < capacity; i++ ) {
				if( value[i] == '\0' ) value[i] = ' ';
			}
			
			for( int i = 0; i < Math.Max( 1, linesCount ); i++ ) {
				lines[i] = new String( value, i * lineSize, lineSize );
			}
		}
		
		int WrapLine( int index, int lineSize ) {
			int lineEnd = index + (lineSize - 1);
			// wrap - but we don't want to wrap if the entire line is filled.
			for( int i = lineEnd; i >= index + 1; i-- ) {
				if( IsWrapper( value[i] ) ) {
					for( int j = lineEnd; j >= i + 1; j-- ) {
						InsertAt( index + lineSize, value[j] );
						value[j] = ' ';
					}
					return (i + 1) - index;
				}
			}
			return lineSize;
		}
		
		bool IsWrapper( char c ) {
			return c == '\0' || c == ' ' || c == '-' || c == '>'
				|| c == '<' || c == '/' || c == '\\';
		}
		
		public override string ToString() {
			return new String( value, 0, Length );
		}
	}
}
