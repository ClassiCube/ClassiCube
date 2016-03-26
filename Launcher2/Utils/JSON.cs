// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
//-----------------------------------------------------------------------
// <copyright file="SimpleJson.cs" company="The Outercurve Foundation">
//    Copyright (c) 2011, The Outercurve Foundation.
//
//    Licensed under the MIT License (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//      http://www.opensource.org/licenses/mit-license.php
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
// </copyright>
// <author>Nathan Totten (ntotten.com), Jim Zimmerman (jimzimmerman.com) and Prabir Shrestha (prabir.me)</author>
// <website>https://github.com/facebook-csharp-sdk/simple-json</website>
//-----------------------------------------------------------------------
// original json parsing code from http://techblog.procurios.nl/k/618/news/view/14605/14863/How-do-I-write-my-own-parser-for-JSON.html
// This is a significantly cutdown version of the original simple-json repository.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace Launcher {
	
	public static class Json {
		const int TOKEN_NONE = 0, TOKEN_CURLY_OPEN = 1, TOKEN_CURLY_CLOSE = 2;
		const int TOKEN_SQUARED_OPEN = 3, TOKEN_SQUARED_CLOSE = 4, TOKEN_COLON = 5;
		const int TOKEN_COMMA = 6, TOKEN_STRING = 7, TOKEN_NUMBER = 8; 
		const int TOKEN_TRUE = 9, TOKEN_FALSE = 10, TOKEN_NULL = 11;

		static Dictionary<string, object> ParseObject( string json, ref int index, ref bool success ) {
			Dictionary<string, object> table = new Dictionary<string, object>();
			NextToken( json, ref index ); // skip {
			
			while( true ) {
				int token = LookAhead( json, index );
				if( token == TOKEN_NONE ) {
					success = false; return null;
				} else if( token == TOKEN_COMMA ) {
					NextToken( json, ref index );
				} else if( token == TOKEN_CURLY_CLOSE ) {
					NextToken( json, ref index );
					return table;
				} else {
					string name = ParseString( json, ref index, ref success );
					if( !success ) {
						success = false; return null;
					}			
					token = NextToken( json, ref index );
					if( token != TOKEN_COLON ) {
						success = false; return null;
					}					
					object value = ParseValue( json, ref index, ref success );
					if( !success ) {
						success = false; return null;
					}
					table[name] = value;
				}
			}
		}

		static List<object> ParseArray( string json, ref int index, ref bool success ) {
			List<object> array = new List<object>();
			NextToken( json, ref index ); // [

			while( true ) {
				int token = LookAhead( json, index );
				if( token == TOKEN_NONE ) {
					success = false; return null;
				} else if( token == TOKEN_COMMA ) {
					NextToken( json, ref index );
				} else if( token == TOKEN_SQUARED_CLOSE ) {
					NextToken( json, ref index );
					return array;
				} else {
					object value = ParseValue( json, ref index, ref success );
					if( !success ) return null;
					array.Add( value );
				}
			}
		}

		public static object ParseValue( string json, ref int index, ref bool success ) {
			switch ( LookAhead( json, index ) ) {
				case TOKEN_STRING:
					return ParseString( json, ref index, ref success );
				case TOKEN_NUMBER:
					return ParseNumber( json, ref index, ref success );
				case TOKEN_CURLY_OPEN:
					return ParseObject( json, ref index, ref success );
				case TOKEN_SQUARED_OPEN:
					return ParseArray( json, ref index, ref success );
				case TOKEN_TRUE:
					NextToken( json, ref index );
					return true;
				case TOKEN_FALSE:
					NextToken( json, ref index );
					return false;
				case TOKEN_NULL:
					NextToken( json, ref index );
					return null;
				case TOKEN_NONE:
					break;
			}
			success = false; return null;
		}

		static string ParseString( string json, ref int index, ref bool success ) {
			StringBuilder s = new StringBuilder( 400 );
			EatWhitespace( json, ref index );
			char c = json[index++]; // "
			
			while( true ) {
				if( index == json.Length )
					break;

				c = json[index++];
				if( c == '"' ) {
					return s.ToString();
				} else if( c == '\\' ) {
					if( index == json.Length )
						break;
					c = json[index++];
					
					if( c == 'u' ) {
						int remainingLength = json.Length - index;
						if( remainingLength >= 4 ) {
							// parse the 32 bit hex into an integer codepoint
							uint codePoint;
							string str = json.Substring( index, 4 );
							if( !( success = UInt32.TryParse( str, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out codePoint ) ) )
								return "";
							
							s.Append( new String( (char)codePoint, 1 ) );					
							index += 4; // skip 4 chars
						} else {
							break;
						}
					} else {
						s.Append( '?' );
					}
				} else {
					s.Append( c );
				}
			}
			success = false; return null;
		}

		const StringComparison caseless = StringComparison.OrdinalIgnoreCase;
		static object ParseNumber( string json, ref int index, ref bool success ) {
			EatWhitespace( json, ref index );
			int lastIndex = GetLastIndexOfNumber( json, index );
			int charLength = (lastIndex - index) + 1;
			string str = json.Substring( index, charLength );
			index = lastIndex + 1;
			return str;
		}

		static int GetLastIndexOfNumber( string json, int index ) {
			int lastIndex = index;
			for( ; lastIndex < json.Length; lastIndex++ ) {
				if( "0123456789+-.".IndexOf( json[lastIndex] ) == -1 ) 
					break;
			}
			return lastIndex - 1;
		}

		static void EatWhitespace( string json, ref int index ) {
			for( ; index < json.Length; index++ ) {
				if( " \t\n\r\b\f".IndexOf( json[index] ) == -1 ) 
					break;
			}
		}

		static int LookAhead( string json, int index ) {
			int saveIndex = index;
			return NextToken( json, ref saveIndex );
		}

		static int NextToken( string json, ref int index ) {
			EatWhitespace( json, ref index );
			if( index == json.Length )
				return TOKEN_NONE;
			char c = json[index];
			index++;
			
			if( c == '{' ) return TOKEN_CURLY_OPEN;
			if( c == '}' ) return TOKEN_CURLY_CLOSE;
			if( c == '[' ) return TOKEN_SQUARED_OPEN;
			if( c == ']' ) return TOKEN_SQUARED_CLOSE;
			
			if( c == ',' ) return TOKEN_COMMA;
			if( c == '"' ) return TOKEN_STRING;
			if( c == ':' ) return TOKEN_COLON;
			if( c == '-' || ('0' <= c && c <= '9') )
			   return TOKEN_NUMBER;
			
			index--;
			if( CompareConstant( json, ref index, "false" ) )
				return TOKEN_FALSE;
			if( CompareConstant( json, ref index, "true" ) )
				return TOKEN_TRUE;
			if( CompareConstant( json, ref index, "null" ) )
				return TOKEN_NULL;
			return TOKEN_NONE;
		}
		
		static bool CompareConstant( string json, ref int index, string value ) {
			int remaining = json.Length - index;
			if( remaining < value.Length ) return false;
			
			for( int i = 0; i < value.Length; i++ ) {
				if( json[index + i] != value[i] ) return false;
			}
			index += value.Length;
			return true;
		}
	}
}