// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.Globalization;
#if !LAUNCHER
using ClassicalSharp.Model;
#endif
using OpenTK;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
using AndroidColor = Android.Graphics.Color;
#endif

namespace ClassicalSharp {

	// NOTE: This delegate should be removed when using versions later than NET 2.0.
	// ################################################################
	public delegate void Action();
	// ################################################################
	
	public enum Anchor { 
		Min,    // left or top
		Centre, // middle
		Max,    // right or bottom
	}
	
	public static partial class Utils {
		
		public const int StringLength = 64;
		
		/// <summary> Returns a string with all the colour codes stripped from it. </summary>
		public static string StripColours(string value) {
			if (value.IndexOf('&') == -1) return value;
			char[] output = new char[value.Length];
			int usedChars = 0;
			
			for (int i = 0; i < value.Length; i++) {
				char token = value[i];
				if (token == '&') {
					i++; // Skip over the following colour code.
				} else {
					output[usedChars++] = token;
				}
			}
			return new String(output, 0, usedChars);
		}

#if !LAUNCHER		
		public static string RemoveEndPlus(string value) {
			// Workaround for MCDzienny (and others) use a '+' at the end to distinguish classicube.net accounts
			// from minecraft.net accounts. Unfortunately they also send this ending + to the client.
			if (String.IsNullOrEmpty(value)) return value;
			
			return value[value.Length - 1] == '+' ?
				value.Substring(0, value.Length - 1) : value;
		}
#endif
		
		const StringComparison comp = StringComparison.OrdinalIgnoreCase;
		public static bool CaselessEquals(string a, string b) { return a.Equals(b, comp); }
		public static bool CaselessStarts(string a, string b) { return a.StartsWith(b, comp); }
		public static bool CaselessEnds(string a, string b) { return a.EndsWith(b, comp); }
		
		/// <summary> Converts the given byte array of length N to a hex string of length 2N. </summary>
		public static string ToHexString(byte[] array) {
			int len = array.Length;
			char[] hex = new char[len * 2];
			for (int i = 0; i < array.Length; i++) {
				int value = array[i];
				int hi = value >> 4, lo = value & 0x0F;
				
				// 48 = index of 0, 55 = index of (A - 10).
				hex[i * 2 + 0] = hi < 10 ? (char)(hi + 48) : (char)(hi + 55);
				hex[i * 2 + 1] = lo < 10 ? (char)(lo + 48) : (char)(lo + 55);
			}
			return new String(hex);
		}
		
		/// <summary> Returns the hex code represented by the given character.
		/// Throws FormatException if the input character isn't a hex code. </summary>
		public static int ParseHex(char value) {
			int hex;
			if (!TryParseHex(value, out hex))
				throw new FormatException("Invalid hex code given: " + value);
			return hex;
		}
		
		/// <summary> Attempts to return the hex code represented by the given character. </summary>
		public static bool TryParseHex(char value, out int hex) {
			hex = 0;
			if (value >= '0' && value <= '9') {
				hex = (int)(value - '0');
			} else if (value >= 'a' && value <= 'f') {
				hex = (int)(value - 'a') + 10;
			} else if (value >= 'A' && value <= 'F') {
				hex = (int)(value - 'A') + 10;
			} else {
				return false;
			}
			return true;
		}		
				
		public static void LogDebug(string text) {
			try { Console.WriteLine(text); } catch { }
		}
		
		public static void LogDebug(string text, params object[] args) {
			try { Console.WriteLine(String.Format(text, args)); } catch { }
		}
		
		public static int AccumulateWheelDelta(ref float accmulator, float delta) {
			// Some mice may use deltas of say (0.2, 0.2, 0.2, 0.2, 0.2)
			// We must use rounding at final step, not at every intermediate step.
			accmulator += delta;
			int steps = (int)accmulator;
			accmulator -= steps;
			return steps;
		}

#if !LAUNCHER
		/// <summary> Attempts to caselessly parse the given string as a Key enum member,
		/// returning defValue if there was an error parsing. </summary>
		public static bool TryParseEnum<T>(string value, T defValue, out T result) {
			T mapping;
			try {
				mapping = (T)Enum.Parse(typeof(T), value, true);
			} catch (ArgumentException) {
				result = defValue;
				return false;
			}
			result = mapping;
			return true;
		}
	
		public static int AdjViewDist(float value) {
			return (int)(1.4142135 * value);
		}
		
		/// <summary> Returns the number of vertices needed to subdivide a quad. </summary>
		public static int CountVertices(int axis1Len, int axis2Len, int axisSize) {
			return CeilDiv(axis1Len, axisSize) * CeilDiv(axis2Len, axisSize) * 4;
		}
		
		public static int Tint(int col, FastColour tint) {
			FastColour adjCol = FastColour.Unpack(col);
			adjCol *= tint;
			return adjCol.Pack();
		}
	
		/// <summary> Determines the skin type of the specified bitmap. </summary>
		public static SkinType GetSkinType(Bitmap bmp) {
			if (bmp.Width == bmp.Height * 2) {
				return SkinType.Type64x32;
			} else if (bmp.Width == bmp.Height) {
				// Minecraft alex skins have this particular pixel with alpha of 0.
				int scale = bmp.Width / 64;
				
				#if !ANDROID
				int alpha = bmp.GetPixel(54 * scale, 20 * scale).A;
				#else
				int alpha = AndroidColor.GetAlphaComponent(bmp.GetPixel(54 * scale, 20 * scale));
				#endif
				return alpha >= 127 ? SkinType.Type64x64 : SkinType.Type64x64Slim;
			}
			return SkinType.Invalid;
		}
		
		/// <summary> Returns whether the specified string starts with http:// or https:// </summary>
		public static bool IsUrlPrefix(string value, int index) {
			int http = value.IndexOf("http://", index);
			int https = value.IndexOf("https://", index);
			return http == index || https == index;
		}
#endif

		/// <summary> Conversion for code page 437 characters from index 0 to 31 to unicode. </summary>
		const string ControlCharReplacements = "\0☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼";
		
		/// <summary> Conversion for code page 437 characters from index 127 to 255 to unicode. </summary>
		const string ExtendedCharReplacements = "⌂ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒáíóúñÑªº¿⌐¬½¼¡«»" +
			"░▒▓│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌" +
			"█▄▌▐▀αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■\u00a0";

		public static bool IsValidInputChar(char c, bool supportsCP437) {
			if (c == '?') return true;
			byte cp437 = UnicodeToCP437(c);
			if (cp437 == '?') return false; // not code page 437
			
			return supportsCP437 || (cp437 == c);
		}

		public static byte UnicodeToCP437(char c) {
			if (c >= ' ' && c <= '~') return (byte)c;
			
			int cIndex = ControlCharReplacements.IndexOf(c);
			if (cIndex >= 0) return (byte)cIndex;
			int eIndex = ExtendedCharReplacements.IndexOf(c);
			if (eIndex >= 0) return (byte)(127 + eIndex);
			return (byte)'?';
		}
		
		public static char CP437ToUnicode(byte c) {
			if (c < 0x20) return ControlCharReplacements[c];
			if (c < 0x7F) return (char)c;
			return ExtendedCharReplacements[c - 0x7F];
		}
		
		public unsafe static string ToLower(string value) {
			fixed(char* ptr = value) {
				for (int i = 0; i < value.Length; i++) {
					char c = ptr[i];
					if (c < 'A' || c > 'Z') continue;
					c += ' '; ptr[i] = c;
				}
			}
			return value;
		}

#if !LAUNCHER
		// Not all languages use . as their decimal point separator
		public static bool TryParseDecimal(string s, out float result) {
			if (s.IndexOf(',') >= 0) 
				s = s.Replace(',', '.');
			float temp;
			
			result = 0;			
			if (!Single.TryParse(s, style, NumberFormatInfo.InvariantInfo, out temp)) return false;
			if (Single.IsInfinity(temp) || Single.IsNaN(temp)) return false;
			result = temp;
			return true;
		}
		
		public static float ParseDecimal(string s) {
			if (s.IndexOf(',') >= 0) 
				s = s.Replace(',', '.');
			return Single.Parse(s, style, NumberFormatInfo.InvariantInfo);
		}
		
		const NumberStyles style = NumberStyles.AllowLeadingWhite | NumberStyles.AllowTrailingWhite
			| NumberStyles.AllowLeadingSign | NumberStyles.AllowDecimalPoint;
		
#endif
	}
}