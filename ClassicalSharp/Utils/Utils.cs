// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Security;
#if !LAUNCHER
using ClassicalSharp.Model;
#endif
using OpenTK;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
using AndroidColor = Android.Graphics.Color;
#endif

// NOTE: This delegate should be removed when using versions later than NET 2.0.
public delegate void Action();

public delegate void EmptyEventFunc();

namespace ClassicalSharp {

	public enum Anchor {
		Min,    // left or top
		Centre, // middle
		Max,    // right or bottom
	}
	
	public static partial class Utils {
		
		public const int StringLength = 64;
		
		[StructLayout(LayoutKind.Sequential, Pack=2)]
		internal struct SYSTEMTIME {
			public ushort Year, Month, DayOfWeek, Day;
			public ushort Hour, Minute, Second, Millis;
		}
		
		[DllImport("kernel32.dll"), SuppressUnmanagedCodeSecurity]
		static extern void GetLocalTime(out SYSTEMTIME st);	
		
		static DateTime LocalNow_Windows() {
			SYSTEMTIME st; GetLocalTime(out st);
			return new DateTime(st.Year, st.Month, st.Day, st.Hour, st.Minute, 
			                    st.Second, st.Millis, DateTimeKind.Local);
		}
		
		public static DateTime LocalNow() {
			// System.NotSupportedException: Can't get timezone name.
			// Gets thrown on some platforms with DateTime.Now
			try {
				// avoid pinvoke-ing GetLocalTime function on non-windows OS
				if (OpenTK.Configuration.RunningOnWindows) return LocalNow_Windows();
			} catch { }
			return DateTime.Now;
		}

		
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
		public static bool CaselessEq(string a, string b)     { return a.Equals(b, comp); }
		public static bool CaselessEquals(string a, string b) { return a.Equals(b, comp); }
		public static bool CaselessStarts(string a, string b) { return a.StartsWith(b, comp); }
		public static bool CaselessEnds(string a, string b)   { return a.EndsWith(b, comp); }
		
		public static void EnsureDirectory(string path) {
			if (Platform.DirectoryExists(path)) return;
			Platform.DirectoryCreate(path);
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
		
		public static string GetFilename(string path) {
			// Ignore directories: convert x/name to name and x\name to name
			for (int i = path.Length - 1; i >= 0; i--) {
				char c = path[i];
				if (c == '/' || c == '\\') return path.Substring(i + 1);
			}
			return path;
		}
		
		public static string ToLower(string src) {
			bool hasUpper = false;
			for (int i = 0; i < src.Length; i++) {
				char c = src[i];
				if (c >= 'A' && c <= 'Z') { hasUpper = true; break; }
			}
			
			if (!hasUpper) return src;
			char[] dst = new char[src.Length];
			
			for (int i = 0; i < src.Length; i++) {
				char c = src[i];
				if (c >= 'A' && c <= 'Z') { c += ' '; }
				dst[i] = c;
			}
			return new string(dst);
		}
		
		public static uint CRC32(byte[] data, int length) {
			uint crc = 0xffffffffU;
			for (int i = 0; i < length; i++) {
				crc ^= data[i];
				for (int j = 0; j < 8; j++)
					crc = (crc >> 1) ^ (crc & 1) * 0xEDB88320;
			}
			return crc ^ 0xffffffffU;
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