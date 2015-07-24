using System;
using System.Drawing;
using System.Runtime.InteropServices;

namespace ClassicalSharp {
	
	/// <summary> 3 floats for position (X, Y, Z),<br/>
	/// 4 bytes for colour (R, G, B, A) </summary>
	[StructLayout( LayoutKind.Sequential, Pack = 1 )]
	public struct VertexPos3fCol4b {
		public float X, Y, Z;
		#if !USE_DX
		public byte R, G, B, A;
		#else
		public byte B, G, R, A;
		#endif
		
		public VertexPos3fCol4b( float x, float y, float z, FastColour c ) {
			X = x; Y = y; Z = z;
			R = c.R; G = c.G; B = c.B; A = c.A;
		}
		
		public VertexPos3fCol4b( float x, float y, float z, Color c ) {
			X = x; Y = y; Z = z;
			R = c.R; G = c.G; B = c.B; A = c.A;
		}
		
		public VertexPos3fCol4b( float x, float y, float z, byte r, byte g, byte b, byte a ) {
			X = x; Y = y; Z = z;
			R = r; G = g; B = b; A = a;
		}
		
		public VertexPos3fCol4b( float x, float y, float z, byte r, byte g, byte b ) {
			X = x; Y = y; Z = z;
			R = r; G = g; B = b; A = 255;
		}
		
		public static readonly int Size = 16; // (4 + 4 + 4) + (1 + 1 + 1 + 1)
		
		public override string ToString()  {
			return String.Format( "({0},{1},{2}) ({3},{4},{5},{6})", X, Y, Z, R, G, B, A );
		}
	}
	
	/// <summary> 3 floats for position (X, Y, Z),<br/>
	/// 2 floats for texture coordinates (U, V),<br/>
	/// 4 bytes for colour (R, G, B, A) </summary>
	[StructLayout( LayoutKind.Sequential, Pack = 1 )]
	public struct VertexPos3fTex2fCol4b {
		public float X, Y, Z;	
		#if !USE_DX
		public byte R, G, B, A;
		#else
		public byte B, G, R, A;
		#endif
		public float U, V;
		
		public VertexPos3fTex2fCol4b( float x, float y, float z, float u, float v, FastColour c ) {
			X = x; Y = y; Z = z;
			U = u; V = v;
			R = c.R; G = c.G; B = c.B; A = c.A;
		}
		
		public VertexPos3fTex2fCol4b( float x, float y, float z, float u, float v, Color c ) {
			X = x; Y = y; Z = z;
			U = u; V = v;
			R = c.R; G = c.G; B = c.B; A = c.A;
		}
		
		public VertexPos3fTex2fCol4b( float x, float y, float z, float u, float v, byte r, byte g, byte b, byte a ) {
			X = x; Y = y; Z = z;
			U = u; V = v;
			R = r; G = g; B = b; A = a;
		}
		
		public VertexPos3fTex2fCol4b( float x, float y, float z, float u, float v, byte r, byte g, byte b ) {
			X = x; Y = y; Z = z;
			U = u; V = v;
			R = r; G = g; B = b; A = 255;
		}
		
		public static readonly int Size = 24; // 3 * 4 + 2 * 4 + 4 * 1
		
		public override string ToString()  {
			return String.Format( "({0},{1},{2}) ({3},{4}) ({5},{6},{7},{8})", X, Y, Z, R, U, V, G, B, A );
		}
	}
	
	/// <summary> 3 floats for position (X, Y, Z),<br/>
	/// 2 floats for texture coordinates (U, V)</summary>
	[StructLayout( LayoutKind.Sequential, Pack = 1 )]
	public struct VertexPos3fTex2f {
		public float X, Y, Z;
		public float U, V;
		
		public VertexPos3fTex2f( float x, float y, float z, float u, float v ) {
			X = x; Y = y; Z = z;
			U = u; V = v;
		}
		
		public static readonly int Size = 20; // (4 + 4 + 4) + (4 + 4)
		
		public override string ToString()  {
			return String.Format( "({0},{1},{2}) ({3},{4})", X, Y, Z, U, V );
		}
	}
}