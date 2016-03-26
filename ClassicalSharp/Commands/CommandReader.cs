// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Commands {

	/// <summary> Reads and parses arguments for a client command. </summary>
	/// <remarks> Spaces are designated as the argument separators. </remarks>
	public class CommandReader {
		
		string rawInput;
		int firstArgOffset;
		int curOffset;
		
		/// <summary> Returns the next argument, or null if there are no more arguments left. </summary>
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
		
		/// <summary> Returns all remaining arguments (including the space separators), 
		/// or null if there are no more arguments left. </summary>
		public string NextAll() {
			if( curOffset >= rawInput.Length ) return null;
			string arg = rawInput.Substring( curOffset, rawInput.Length - curOffset );
			curOffset = rawInput.Length;
			return arg;
		}
		
		/// <summary> Attempts to parse the next argument as a 32-bit integer. </summary>
		public bool NextInt( out int value ) {
			return Int32.TryParse( Next(), out value );
		}
		
		/// <summary> Attempts to parse the next argument as a 32-bit floating point number. </summary>
		public bool NextFloat( out float value ) {
			return Single.TryParse( Next(), out value );
		}
		
		/// <summary> Attempts to parse the next argument as a 6 digit hex number. </summary>
		/// <remarks> #xxxxxx or xxxxxx are accepted. </remarks>
		public bool NextHexColour( out FastColour value ) {
			return FastColour.TryParse( Next(), out value );
		}
		
		/// <summary> Attempts to parse the next argument using the specified parsing function. </summary>
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
		
		/// <summary> Total number of arguments yet to be processed. </summary>
		public int RemainingArgs {
			get { return CountArgsFrom( curOffset ); }
		}
		
		/// <summary> Total number of arguments provided by the user. </summary>
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

		/// <summary> Rewinds the internal state back to the first argument. </summary>
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
