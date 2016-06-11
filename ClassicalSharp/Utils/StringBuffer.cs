// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp {
	
	public class StringBuffer {
		
		public char[] value;
		public int capacity;
		
		public StringBuffer( int capacity ) {
			this.capacity = capacity;
			value = new char[capacity];
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
		
		public StringBuffer AppendColourless( ref int index, string s ) {
			for( int i = 0; i < s.Length; i++ ) {
				char token = s[i];
				if( token == '&' )
					i++;// Skip over the following colour code.
				else
					value[index++] = token;
			}
			return this;
		}
		
		public StringBuffer Append( ref int index, char c) {
			value[index++] = c;
			return this;
		}
		
		static char[] numBuffer = new char[20];
		public StringBuffer AppendNum( ref int index, long num ) {
			int numLen = MakeNum( num );
			for( int i = numLen - 1; i >= 0; i-- )
				value[index++] = numBuffer[i];
			return this;
		}
		
		public StringBuffer AppendPaddedNum( ref int index, int minDigits, long num ) {
			for( int i = 0; i < minDigits; i++ )
				numBuffer[i] = '0';
			int numLen = Math.Max( minDigits, MakeNum( num ) );
			for( int i = numLen - 1; i >= 0; i-- )
				value[index++] = numBuffer[i];
			return this;
		}
		
		int MakeNum( long num ) {
			int len = 0;
			numBuffer[len++] = (char)('0' + (num % 10)); num /= 10;
			
			while( num > 0 ) {
				numBuffer[len++] = (char)('0' + (num % 10)); num /= 10;
			}
			return len;
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
		
		public void InsertAt( int index, string s ) {
			for( int i = 0; i < s.Length; i++ )
				InsertAt( index + i, s[i] );
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
		
		public override string ToString() {
			return new String( value, 0, Length );
		}
	}
}
