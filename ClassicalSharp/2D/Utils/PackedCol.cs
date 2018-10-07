// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.Runtime.InteropServices;
#if ANDROID
using AndroidColor = Android.Graphics.Color;
#endif

namespace ClassicalSharp {
	
	/// <summary> Structure that can be used for quick manipulations of A/R/G/B colours. </summary>
	/// <remarks> This structure is suitable for interop with OpenGL or Direct3D.
	/// The order of each colour component differs depending on the underlying API. </remarks>
	[StructLayout(LayoutKind.Explicit)]
	public struct PackedCol : IEquatable<PackedCol> {
		
		[FieldOffset(0)] public uint Packed;
		#if USE_DX
		[FieldOffset(0)] public byte B;
		[FieldOffset(1)] public byte G;
		[FieldOffset(2)] public byte R;
		[FieldOffset(3)] public byte A;
		#else
		[FieldOffset(0)] public byte R;
		[FieldOffset(1)] public byte G;
		[FieldOffset(2)] public byte B;
		[FieldOffset(3)] public byte A;
		#endif
		
		public PackedCol(byte r, byte g, byte b, byte a) {
			Packed = 0;
			A = a; R = r; G = g; B = b;
		}
		
		public PackedCol(int r, int g, int b, int a) {
			Packed = 0;
			A = (byte)a; R = (byte)r; G = (byte)g; B = (byte)b;
		}
		
		public PackedCol(byte r, byte g, byte b) {
			Packed = 0;
			A = 255; R = r; G = g; B = b;
		}
		
		public PackedCol(int r, int g, int b) {
			Packed = 0;
			A = 255; R = (byte)r; G = (byte)g; B = (byte)b;
		}
		
		/// <summary> Multiplies the RGB components of this instance by the
		/// specified t parameter, where 0 ≤ t ≤ 1 </summary>
		public static PackedCol Scale(PackedCol value, float t) {
			value.R = (byte)(value.R * t);
			value.G = (byte)(value.G * t);
			value.B = (byte)(value.B * t);
			return value;
		}
		
		/// <summary> Linearly interpolates the RGB components of the two colours
		/// by the specified t parameter, where 0 ≤ t ≤ 1 </summary>
		public static PackedCol Lerp(PackedCol a, PackedCol b, float t) {
			a.R = (byte)Utils.Lerp(a.R, b.R, t);
			a.G = (byte)Utils.Lerp(a.G, b.G, t);
			a.B = (byte)Utils.Lerp(a.B, b.B, t);
			return a;
		}
		
		public static PackedCol GetHexEncodedCol(int hex, int lo, int hi) {
			return new PackedCol(
				lo * ((hex >> 2) & 1) + hi * (hex >> 3),
				lo * ((hex >> 1) & 1) + hi * (hex >> 3),
				lo * ((hex >> 0) & 1) + hi * (hex >> 3));
		}

		#if !LAUNCHER
		public const float ShadeX = 0.6f, ShadeZ = 0.8f, ShadeYBottom = 0.5f;
		public static void GetShaded(PackedCol normal, out PackedCol xSide,
		                             out PackedCol zSide, out PackedCol yBottom) {
			xSide = PackedCol.Scale(normal, ShadeX);
			zSide = PackedCol.Scale(normal, ShadeZ);
			yBottom = PackedCol.Scale(normal, ShadeYBottom);
		}
		#endif

		
		/// <summary> Packs this instance into a 32 bit integer, where A occupies
		/// the highest 8 bits and B occupies the lowest 8 bits. </summary>
		public int ToArgb() { return A << 24 | R << 16 | G << 8 | B; }
		
		public static PackedCol Argb(int c) {
			PackedCol col = default(PackedCol);
			col.A = (byte)(c >> 24);
			col.R = (byte)(c >> 16);
			col.G = (byte)(c >> 8);
			col.B = (byte)c;
			return col;
		}
		
		public override bool Equals(object obj) {
			return (obj is PackedCol) && Equals((PackedCol)obj);
		}
		
		public bool Equals(PackedCol other) { return Packed == other.Packed; }
		public override int GetHashCode() { return (int)Packed; }
		
		public override string ToString() {
			return R + ", " + G + ", " + B + " : " + A;
		}
		
		public static bool operator == (PackedCol left, PackedCol right) {
			return left.Packed == right.Packed;
		}
		
		public static bool operator != (PackedCol left, PackedCol right) {
			return left.Packed != right.Packed;
		}
		
		public static PackedCol operator * (PackedCol left, PackedCol right) {
			left.R = (byte)((left.R * right.R) / 255);
			left.G = (byte)((left.G * right.G) / 255);
			left.B = (byte)((left.B * right.B) / 255);
			return left;
		}
		
		public static implicit operator Color(PackedCol col) {
			return Color.FromArgb(col.A, col.R, col.G, col.B);
		}

		#if ANDROID
		public static implicit operator AndroidColor(PackedCol col) {
			return AndroidColor.Argb(col.A, col.R, col.G, col.B);
		}
		#endif
		
		public static PackedCol Red   = new PackedCol(255, 0, 0);
		public static PackedCol Green = new PackedCol(0, 255, 0);
		public static PackedCol Blue  = new PackedCol(0, 0, 255);
		
		public static PackedCol White = new PackedCol(255, 255, 255);
		public static PackedCol Black = new PackedCol(0, 0, 0);

		public static PackedCol Yellow  = new PackedCol(255, 255, 0);
		public static PackedCol Magenta = new PackedCol(255, 0, 255);
		public static PackedCol Cyan    = new PackedCol(0, 255, 255);
		
		public string ToHex() {
			byte[] array = new byte[] { R, G, B };
			int len = array.Length;
			char[] hex = new char[len * 2];
			
			for (int i = 0; i < array.Length; i++) {
				int value = array[i], hi = value >> 4, lo = value & 0x0F;
				// 48 = index of 0, 55 = index of (A - 10)
				hex[i * 2 + 0] = hi < 10 ? (char)(hi + 48) : (char)(hi + 55);
				hex[i * 2 + 1] = lo < 10 ? (char)(lo + 48) : (char)(lo + 55);
			}
			return new String(hex);
		}
		
		public static bool TryParse(string input, out PackedCol value) {
			value = default(PackedCol);
			if (input == null || input.Length < 6) return false;
			if (input.Length > 6 && (input[0] != '#' || input.Length > 7)) return false;
			
			int rH, rL, gH, gL, bH, bL;
			int i = input[0] == '#' ? 1 : 0;
			
			if (!UnHex(input[i + 0], out rH) || !UnHex(input[i + 1], out rL)) return false;
			if (!UnHex(input[i + 2], out gH) || !UnHex(input[i + 3], out gL)) return false;
			if (!UnHex(input[i + 4], out bH) || !UnHex(input[i + 5], out bL)) return false;
			
			value = new PackedCol((rH << 4) | rL, (gH << 4) | gL, (bH << 4) | bL);
			return true;
		}
		
		public static PackedCol Parse(string input) {
			PackedCol value;
			if (!TryParse(input, out value)) throw new FormatException();
			return value;
		}
		
		static bool UnHex(char hex, out int value) {
			value = 0;
			if (hex >= '0' && hex <= '9') {
				value = (hex - '0');
			} else if (hex >= 'a' && hex <= 'f') {
				value = (hex - 'a') + 10;
			} else if (hex >= 'A' && hex <= 'F') {
				value = (hex - 'A') + 10;
			} else {
				return false;
			}
			return true;
		}
	}
}
