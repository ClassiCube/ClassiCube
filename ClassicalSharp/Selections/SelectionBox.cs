// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {

	internal class SelectionBox {
		
		public byte ID;
		public Vector3I Min, Max;
		public FastColour Colour;
		public float MinDist, MaxDist;
		
		public SelectionBox(Vector3I start, Vector3I end, FastColour col) {	
			Min = Vector3I.Min(start, end);
			Max = Vector3I.Max(start, end);
			Colour = col;
		}
		
		public void Render(double delta, VertexP3fC4b[] vertices, VertexP3fC4b[] lineVertices,
		                  ref int index, ref int lineIndex) {
			float offset = MinDist < 32 * 32 ? 1/32f : 1/16f;
			Vector3 p1 = (Vector3)Min - new Vector3(offset, offset, offset);
			Vector3 p2 = (Vector3)Max + new Vector3(offset, offset, offset);
			int col = Colour.Pack();
			
			HorQuad(vertices, ref index, col, p1.X, p1.Z, p2.X, p2.Z, p1.Y); // bottom
			HorQuad(vertices, ref index, col, p1.X, p1.Z, p2.X, p2.Z, p2.Y); // top		
			VerQuad(vertices, ref index, col, p1.X, p1.Y, p1.Z, p2.X, p2.Y, p1.Z); // sides
			VerQuad(vertices, ref index, col, p1.X, p1.Y, p2.Z, p2.X, p2.Y, p2.Z);
			VerQuad(vertices, ref index, col, p1.X, p1.Y, p1.Z, p1.X, p2.Y, p2.Z);
			VerQuad(vertices, ref index, col, p2.X, p1.Y, p1.Z, p2.X, p2.Y, p2.Z);
			
			col = new FastColour((byte)~Colour.R, (byte)~Colour.G, (byte)~Colour.B).Pack();
			// bottom face
			Line(lineVertices, ref lineIndex, p1.X, p1.Y, p1.Z, p2.X, p1.Y, p1.Z, col);
			Line(lineVertices, ref lineIndex, p2.X, p1.Y, p1.Z, p2.X, p1.Y, p2.Z, col);
			Line(lineVertices, ref lineIndex, p2.X, p1.Y, p2.Z, p1.X, p1.Y, p2.Z, col);
			Line(lineVertices, ref lineIndex, p1.X, p1.Y, p2.Z, p1.X, p1.Y, p1.Z, col);
			// top face
			Line(lineVertices, ref lineIndex, p1.X, p2.Y, p1.Z, p2.X, p2.Y, p1.Z, col);
			Line(lineVertices, ref lineIndex, p2.X, p2.Y, p1.Z, p2.X, p2.Y, p2.Z, col);
			Line(lineVertices, ref lineIndex, p2.X, p2.Y, p2.Z, p1.X, p2.Y, p2.Z, col);
			Line(lineVertices, ref lineIndex, p1.X, p2.Y, p2.Z, p1.X, p2.Y, p1.Z, col);
			// side faces
			Line(lineVertices, ref lineIndex, p1.X, p1.Y, p1.Z, p1.X, p2.Y, p1.Z, col);
			Line(lineVertices, ref lineIndex, p2.X, p1.Y, p1.Z, p2.X, p2.Y, p1.Z, col);
			Line(lineVertices, ref lineIndex, p2.X, p1.Y, p2.Z, p2.X, p2.Y, p2.Z, col);
			Line(lineVertices, ref lineIndex, p1.X, p1.Y, p2.Z, p1.X, p2.Y, p2.Z, col);
		}
		
		internal static void VerQuad(VertexP3fC4b[] vertices, ref int index, int col,
		                           float x1, float y1, float z1, float x2, float y2, float z2) {
			VertexP3fC4b v; v.Colour = col;
			v.X = x1; v.Y = y1; v.Z = z1; vertices[index++] = v;
			          v.Y = y2;           vertices[index++] = v;
			v.X = x2;           v.Z = z2; vertices[index++] = v;
			          v.Y = y1;           vertices[index++] = v;
		}
		
		internal static void HorQuad(VertexP3fC4b[] vertices, ref int index, int col, 
		                           float x1, float z1, float x2, float z2, float y) {
			VertexP3fC4b v; v.Y = y; v.Colour = col;
			v.X = x1; v.Z = z1; vertices[index++] = v;
			          v.Z = z2; vertices[index++] = v;
			v.X = x2;           vertices[index++] = v;
			          v.Z = z1; vertices[index++] = v;
		}
		
		static void Line(VertexP3fC4b[] vertices, ref int index, float x1, float y1, float z1, 
		          float x2, float y2, float z2, int col) {
			VertexP3fC4b v; v.Colour = col;
			v.X = x1; v.Y = y1; v.Z = z1; vertices[index++] = v;
			v.X = x2; v.Y = y2; v.Z = z2; vertices[index++] = v;
		}
	}
	
	internal class SelectionBoxComparer : IComparer<SelectionBox> {
		
		public int Compare(SelectionBox a, SelectionBox b) {
			// Reversed comparison order because we need to render back to front for alpha blending.
			return a.MinDist == b.MinDist 
				? b.MaxDist.CompareTo(a.MaxDist)
				: b.MinDist.CompareTo(a.MinDist);
		}
		
		internal void Intersect(SelectionBox box, Vector3 origin) {
			Vector3I min = box.Min, max = box.Max;
			float closest = float.PositiveInfinity, furthest = float.NegativeInfinity;
			// Bottom corners
			UpdateDist(origin, min.X, min.Y, min.Z, ref closest, ref furthest);
			UpdateDist(origin, max.X, min.Y, min.Z, ref closest, ref furthest);
			UpdateDist(origin, max.X, min.Y, max.Z, ref closest, ref furthest);
			UpdateDist(origin, min.X, min.Y, max.Z, ref closest, ref furthest);
			// Top corners
			UpdateDist(origin, min.X, max.Y, min.Z, ref closest, ref furthest);
			UpdateDist(origin, max.X, max.Y, min.Z, ref closest, ref furthest);
			UpdateDist(origin, max.X, max.Y, max.Z, ref closest, ref furthest);
			UpdateDist(origin, min.X, max.Y, max.Z, ref closest, ref furthest);
			box.MinDist = closest; box.MaxDist = furthest;
		}
		
		
		static void UpdateDist(Vector3 p, float x2, float y2, float z2,
		                       ref float closest, ref float furthest) {
			float dx = x2 - p.X, dy = y2 - p.Y, dz = z2 - p.Z;
			float dist = dx * dx + dy * dy + dz * dz;
			
			if (dist < closest) closest = dist;
			if (dist > furthest) furthest = dist;
		}
	}
}
