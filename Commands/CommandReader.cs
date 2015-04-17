using System;

namespace ClassicalSharp.Commands {

	public class CommandReader {
		
		string rawInput;
		int firstArgOffset;
		int curOffset;
		
		public string Next() {
			if( curOffset >= rawInput.Length ) return null;
			int next = rawInput.IndexOf( ' ', curOffset );
			if( next == -1 ) {
				next = rawInput.Length;
			}
			
			string arg = rawInput.Substring( curOffset, next - curOffset );
			curOffset = next + 1; // skip space
			return arg;
		}
		
		public string NextAll() {
			if( curOffset >= rawInput.Length ) return null;
			string arg = rawInput.Substring( curOffset, rawInput.Length - curOffset );
			curOffset = rawInput.Length;
			return arg;
		}
		
		public bool NextInt( out int value ) {
			value = 0;
			return Int32.TryParse( Next(), out value );
		}
		
		public bool NextFloat( out float value ) {
			value = 0;
			return Single.TryParse( Next(), out value );
		}
		
		public bool NextHexColour( out FastColour value ) {
			value = default( FastColour );
			string next = Next();
			if( String.IsNullOrEmpty( next ) || next.Length < 6 ) {
				return false;
			}

			try {
				int offset = next.Length > 6 ? 1 : 0;
				if( next.Length > 6 && ( next[0] != '#' || next.Length > 7 ) ) {
					return false;
				}
				int r = Utils.ParseHex( next[offset + 0] ) * 16 + Utils.ParseHex( next[offset + 1] );
				int g = Utils.ParseHex( next[offset + 2] ) * 16 + Utils.ParseHex( next[offset + 3] );
				int b = Utils.ParseHex( next[offset + 4] ) * 16 + Utils.ParseHex( next[offset + 5] );
				value = new FastColour( r, g, b );
				return true;
			} catch( FormatException ) {
				return false;
			}
		}
		
		public bool NextOf<T>( out T value, TryParseFunc<T> parser ) {
			bool success = parser( Next(), out value );
			if( !success ) value = default( T);
			return success;
		}
		
		bool MoveNext() {
			if( curOffset >= rawInput.Length ) return false;
			int next = rawInput.IndexOf( ' ', curOffset );
			if( next == -1 ) {
				next = rawInput.Length;
			}
			curOffset = next + 1;
			return true;
		}
		
		public int RemainingArgs {
			get {
				int count = 0;
				int pos = curOffset;
				while( MoveNext() ) {
					count++;
				}
				curOffset = pos;
				return count;
			}
		}
		
		public int TotalArgs {
			get {
				int count = 0;
				int pos = curOffset;
				curOffset = firstArgOffset;
				while( MoveNext() ) {
					count++;
				}
				curOffset = pos;
				return count;
			}
		}

		public void Reset() {
			curOffset = firstArgOffset;
		}
		
		public CommandReader( string input ) {
			rawInput = input.TrimEnd( ' ' );
			// Check that the string has at least one argument - the command name
			int firstSpaceIndex = rawInput.IndexOf( ' ' );
			if( firstSpaceIndex < 0 ) {
				firstSpaceIndex = rawInput.Length - 1;
			}
			firstArgOffset = firstSpaceIndex + 1;
			curOffset = firstArgOffset;
		}
	}
}
