// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Commands {

	/// <summary> Reads and parses arguments for a client command. </summary>
	/// <remarks> Spaces are designated as the argument separators. </remarks>
	public class CommandReader {
		
		string rawInput;
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
		
		bool MoveNext() {
			if( curOffset >= rawInput.Length ) return false;
			int next = rawInput.IndexOf( ' ', curOffset );
			if( next == -1 ) {
				next = rawInput.Length;
			}
			curOffset = next + 1;
			return true;
		}
		
		public CommandReader( string input ) {
			rawInput = input.TrimEnd( ' ' );
			curOffset = 1; // skip start / for the ocmmand
		}
	}
}
