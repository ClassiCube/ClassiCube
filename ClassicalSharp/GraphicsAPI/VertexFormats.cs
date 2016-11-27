// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.Runtime.InteropServices;
using OpenTK;

namespace ClassicalSharp.GraphicsAPI {
	
	/// <summary> 3 floats for position (XYZ),<br/>
	/// 4 bytes for colour (RGBA if OpenGL, BGRA if Direct3D) </summary>
	/// <remarks> Use FastColour.Pack to convert colours to the correct swizzling. </remarks>
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct VertexP3fC4b {
		public float X, Y, Z;
		public int Colour;
		
		public VertexP3fC4b(float x, float y, float z, int c) {
			X = x; Y = y; Z = z;
			Colour = c;
		}
		
		public const int Size = 16; // (4 + 4 + 4) + (1 + 1 + 1 + 1)
	}
	
	/// <summary> 3 floats for position (XYZ),<br/>
	/// 2 floats for texture coordinates (UV),<br/>
	/// 4 bytes for colour (RGBA if OpenGL, BGRA if Direct3D) </summary>
	/// <remarks> Use FastColour.Pack to convert colours to the correct swizzling. </remarks>
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct VertexP3fT2fC4b {
		public float X, Y, Z;	
		public int Colour;
		public float U, V;

		public VertexP3fT2fC4b(float x, float y, float z, float u, float v, int c) {
			X = x; Y = y; Z = z; 
			U = u; V = v;
			Colour = c;
		}
		
		public VertexP3fT2fC4b(ref Vector3 p, float u, float v, int c) {
			X = p.X; Y = p.Y; Z = p.Z; 
			U = u; V = v;
			Colour = c;
		}
		
		public const int Size = 24; // 3 * 4 + 2 * 4 + 4 * 1
	}
}