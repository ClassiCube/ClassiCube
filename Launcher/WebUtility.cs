using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace Launcher {
	
	public static class WebUtility {
		
		static Dictionary<string, char> _lookupTable = new Dictionary<string, char>( 253, StringComparer.Ordinal ) {
			{ "quot", '\x0022' }, { "amp", '\x0026' }, { "apos", '\x0027' }, { "lt", '\x003C' },
			{ "gt", '\x003E' }, { "nbsp", '\x00A0' }, { "iexcl", '\x00A1' }, { "cent", '\x00A2' },
			{ "pound", '\x00A3' }, { "curren", '\x00A4' }, { "yen", '\x00A5' }, { "brvbar", '\x00A6' },
			{ "sect", '\x00A7' }, { "uml", '\x00A8' }, { "copy", '\x00A9' }, { "ordf", '\x00AA' },
			{ "laquo", '\x00AB' }, { "not", '\x00AC' }, { "shy", '\x00AD' }, { "reg", '\x00AE' },
			{ "macr", '\x00AF' }, { "deg", '\x00B0' }, { "plusmn", '\x00B1' }, { "sup2", '\x00B2' },
			{ "sup3", '\x00B3' }, { "acute", '\x00B4' }, { "micro", '\x00B5' }, { "para", '\x00B6' },
			{ "middot", '\x00B7' }, { "cedil", '\x00B8' }, { "sup1", '\x00B9' }, { "ordm", '\x00BA' },
			{ "raquo", '\x00BB' }, { "frac14", '\x00BC' }, { "frac12", '\x00BD' }, { "frac34", '\x00BE' },
			{ "iquest", '\x00BF' }, { "Agrave", '\x00C0' }, { "Aacute", '\x00C1' }, { "Acirc", '\x00C2' },
			{ "Atilde", '\x00C3' }, { "Auml", '\x00C4' }, { "Aring", '\x00C5' }, { "AElig", '\x00C6' },
			{ "Ccedil", '\x00C7' }, { "Egrave", '\x00C8' }, { "Eacute", '\x00C9' }, { "Ecirc", '\x00CA' },
			{ "Euml", '\x00CB' }, { "Igrave", '\x00CC' }, { "Iacute", '\x00CD' }, { "Icirc", '\x00CE' },
			{ "Iuml", '\x00CF' }, { "ETH", '\x00D0' }, { "Ntilde", '\x00D1' }, { "Ograve", '\x00D2' },
			{ "Oacute", '\x00D3' }, { "Ocirc", '\x00D4' }, { "Otilde", '\x00D5' }, { "Ouml", '\x00D6' },
			{ "times", '\x00D7' }, { "Oslash", '\x00D8' }, { "Ugrave", '\x00D9' }, { "Uacute", '\x00DA' },
			{ "Ucirc", '\x00DB' }, { "Uuml", '\x00DC' }, { "Yacute", '\x00DD' }, { "THORN", '\x00DE' },
			{ "szlig", '\x00DF' }, { "agrave", '\x00E0' }, { "aacute", '\x00E1' }, { "acirc", '\x00E2' },
			{ "atilde", '\x00E3' }, { "auml", '\x00E4' }, { "aring", '\x00E5' }, { "aelig", '\x00E6' },
			{ "ccedil", '\x00E7' }, { "egrave", '\x00E8' }, { "eacute", '\x00E9' }, { "ecirc", '\x00EA' },
			{ "euml", '\x00EB' }, { "igrave", '\x00EC' }, { "iacute", '\x00ED' }, { "icirc", '\x00EE' },
			{ "iuml", '\x00EF' }, { "eth", '\x00F0' }, { "ntilde", '\x00F1' }, { "ograve", '\x00F2' },
			{ "oacute", '\x00F3' }, { "ocirc", '\x00F4' }, { "otilde", '\x00F5' }, { "ouml", '\x00F6' },
			{ "divide", '\x00F7' }, { "oslash", '\x00F8' }, { "ugrave", '\x00F9' }, { "uacute", '\x00FA' },
			{ "ucirc", '\x00FB' }, { "uuml", '\x00FC' }, { "yacute", '\x00FD' }, { "thorn", '\x00FE' },
			{ "yuml", '\x00FF' }, { "OElig", '\x0152' }, { "oelig", '\x0153' }, { "Scaron", '\x0160' },
			{ "scaron", '\x0161' }, { "Yuml", '\x0178' }, { "fnof", '\x0192' }, { "circ", '\x02C6' },
			{ "tilde", '\x02DC' }, { "Alpha", '\x0391' }, { "Beta", '\x0392' }, { "Gamma", '\x0393' },
			{ "Delta", '\x0394' }, { "Epsilon", '\x0395' }, { "Zeta", '\x0396' }, { "Eta", '\x0397' },
			{ "Theta", '\x0398' }, { "Iota", '\x0399' }, { "Kappa", '\x039A' }, { "Lambda", '\x039B' },
			{ "Mu", '\x039C' }, { "Nu", '\x039D' }, { "Xi", '\x039E' }, { "Omicron", '\x039F' },
			{ "Pi", '\x03A0' }, { "Rho", '\x03A1' }, { "Sigma", '\x03A3' }, { "Tau", '\x03A4' },
			{ "Upsilon", '\x03A5' }, { "Phi", '\x03A6' }, { "Chi", '\x03A7' }, { "Psi", '\x03A8' },
			{ "Omega", '\x03A9' }, { "alpha", '\x03B1' }, { "beta", '\x03B2' }, { "gamma", '\x03B3' },
			{ "delta", '\x03B4' }, { "epsilon", '\x03B5' }, { "zeta", '\x03B6' }, { "eta", '\x03B7' },
			{ "theta", '\x03B8' }, { "iota", '\x03B9' }, { "kappa", '\x03BA' }, { "lambda", '\x03BB' },
			{ "mu", '\x03BC' }, { "nu", '\x03BD' }, { "xi", '\x03BE' }, { "omicron", '\x03BF' },
			{ "pi", '\x03C0' }, { "rho", '\x03C1' }, { "sigmaf", '\x03C2' }, { "sigma", '\x03C3' },
			{ "tau", '\x03C4' }, { "upsilon", '\x03C5' }, { "phi", '\x03C6' }, { "chi", '\x03C7' },
			{ "psi", '\x03C8' }, { "omega", '\x03C9' }, { "thetasym", '\x03D1' }, { "upsih", '\x03D2' },
			{ "piv", '\x03D6' }, { "ensp", '\x2002' }, { "emsp", '\x2003' }, { "thinsp", '\x2009' },
			{ "zwnj", '\x200C' }, { "zwj", '\x200D' }, { "lrm", '\x200E' }, { "rlm", '\x200F' },
			{ "ndash", '\x2013' }, { "mdash", '\x2014' }, { "lsquo", '\x2018' }, { "rsquo", '\x2019' },
			{ "sbquo", '\x201A' }, { "ldquo", '\x201C' }, { "rdquo", '\x201D' }, { "bdquo", '\x201E' },
			{ "dagger", '\x2020' }, { "Dagger", '\x2021' }, { "bull", '\x2022' }, { "hellip", '\x2026' },
			{ "permil", '\x2030' }, { "prime", '\x2032' }, { "Prime", '\x2033' }, { "lsaquo", '\x2039' },
			{ "rsaquo", '\x203A' }, { "oline", '\x203E' }, { "frasl", '\x2044' }, { "euro", '\x20AC' },
			{ "image", '\x2111' }, { "weierp", '\x2118' }, { "real", '\x211C' }, { "trade", '\x2122' },
			{ "alefsym", '\x2135' }, { "larr", '\x2190' }, { "uarr", '\x2191' }, { "rarr", '\x2192' },
			{ "darr", '\x2193' }, { "harr", '\x2194' }, { "crarr", '\x21B5' }, { "lArr", '\x21D0' },
			{ "uArr", '\x21D1' }, { "rArr", '\x21D2' }, { "dArr", '\x21D3' }, { "hArr", '\x21D4' },
			{ "forall", '\x2200' }, { "part", '\x2202' }, { "exist", '\x2203' }, { "empty", '\x2205' },
			{ "nabla", '\x2207' }, { "isin", '\x2208' }, { "notin", '\x2209' }, { "ni", '\x220B' },
			{ "prod", '\x220F' }, { "sum", '\x2211' }, { "minus", '\x2212' }, { "lowast", '\x2217' },
			{ "radic", '\x221A' }, { "prop", '\x221D' }, { "infin", '\x221E' }, { "ang", '\x2220' },
			{ "and", '\x2227' }, { "or", '\x2228' }, { "cap", '\x2229' }, { "cup", '\x222A' },
			{ "int", '\x222B' }, { "there4", '\x2234' }, { "sim", '\x223C' }, { "cong", '\x2245' },
			{ "asymp", '\x2248' }, { "ne", '\x2260' }, { "equiv", '\x2261' }, { "le", '\x2264' },
			{ "ge", '\x2265' }, { "sub", '\x2282' }, { "sup", '\x2283' }, { "nsub", '\x2284' },
			{ "sube", '\x2286' }, { "supe", '\x2287' }, { "oplus", '\x2295' }, { "otimes", '\x2297' },
			{ "perp", '\x22A5' }, { "sdot", '\x22C5' }, { "lceil", '\x2308' }, { "rceil", '\x2309' },
			{ "lfloor", '\x230A' }, { "rfloor", '\x230B' }, { "lang", '\x2329' }, { "rang", '\x232A' },
			{ "loz", '\x25CA' }, { "spades", '\x2660' }, { "clubs", '\x2663' }, { "hearts", '\x2665' },
			{ "diams", '\x2666' },
		};
		
		static bool Lookup( string entity, out char decoded ) {
			return _lookupTable.TryGetValue( entity, out decoded );
		}
		
		public static string HtmlDecode( string value ) {
			value = value.Replace( "hellip;", "\x2026" ); // minecraft.net doesn't escape this at the end properly.
			if( String.IsNullOrEmpty( value ) || value.IndexOf( '&' ) < 0 ) {
				return value;
			}
			
			StringBuilder sb = new StringBuilder();
			WebUtility.HtmlDecode( value, sb );
			return sb.ToString();
		}
		
		static void HtmlDecode( string value, StringBuilder output ) {
			for( int i = 0; i < value.Length; i++ ) {
				char token = value[i];
				if( token != '&' ) {
					output.Append( token );
					continue;
				}
				
				int entityEnd = value.IndexOf( ';', i + 1 );
				if( entityEnd <= 0 ) {
					output.Append( token );
					continue;
				}
				
				string entity = value.Substring( i + 1, entityEnd - i - 1 );
				if( entity.Length > 1 && entity[0] == '#' ) {
					ushort encodedNumber;
					if( entity[1] == 'x' || entity[1] == 'X' ) {
						ushort.TryParse( entity.Substring( 2 ), NumberStyles.AllowHexSpecifier, NumberFormatInfo.InvariantInfo, out encodedNumber );
					} else {
						ushort.TryParse( entity.Substring( 1 ), NumberStyles.Integer, NumberFormatInfo.InvariantInfo, out encodedNumber );
					}
					if( encodedNumber != 0 ) {
						output.Append( (char)encodedNumber );
						i = entityEnd;
					}
				} else {
					i = entityEnd;
					char decodedEntity;
					if( Lookup( entity, out decodedEntity ) ) {
						output.Append( decodedEntity );
					} else { // Invalid token.
						output.Append( '&' );
						output.Append( entity );
						output.Append( ';' );
					}
				}
			}
		}
	}
}
