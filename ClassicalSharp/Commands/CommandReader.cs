using System;

namespace ClassicalSharp.Commands {

	/// <summary> Reads and parses arguments for a client command. </summary>
	/// <remarks> Spaces are designated as the argument separators. </remarks>
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
			curOffset = next + 1; // skip following space
			return arg;
		}
		
		public string NextAll() {
			if( curOffset >= rawInput.Length ) return null;
			string arg = rawInput.Substring( curOffset, rawInput.Length - curOffset );
			curOffset = rawInput.Length;
			return arg;
		}
		
		public bool NextInt( out int value ) {
			return Int32.TryParse( Next(), out value );
		}
		
		public bool NextFloat( out float value ) {
			return Single.TryParse( Next(), out value );
		}
		
		public bool NextHexColour( out FastColour value ) {
			return FastColour.TryParse( Next(), out value );
		}
		
		public bool NextOf<T>( out T value, TryParseFunc<T> parser ) {
			bool success = parser( Next(), out value );
			if( !success ) value = default( T );
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
			get { return CountArgsFrom( curOffset ); }
		}
		
		public int TotalArgs {
			get { return CountArgsFrom( firstArgOffset ); }
		}
		
		int CountArgsFrom( int startOffset ) {
			int count = 0;
			int pos = curOffset;
			curOffset = startOffset;
			while( MoveNext() ) {
				count++;
			}
			curOffset = pos;
			return count;
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
