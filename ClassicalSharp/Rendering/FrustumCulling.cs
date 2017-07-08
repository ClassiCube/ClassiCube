// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp {

	// http://www.crownandcutlass.com/features/technicaldetails/frustum.html
	public class FrustumCulling {
		
		float frustum00, frustum01, frustum02, frustum03,
		frustum10, frustum11, frustum12, frustum13,
		frustum20, frustum21, frustum22, frustum23,
		frustum30, frustum31, frustum32, frustum33,
		frustum40, frustum41, frustum42, frustum43;
		
		public bool SphereInFrustum(float x, float y, float z, float radius) {
			float d = frustum00 * x + frustum01 * y + frustum02 * z + frustum03;
			if (d <= -radius) return false;
			
			d = frustum10 * x + frustum11 * y + frustum12 * z + frustum13;
			if (d <= -radius) return false;
			
			d = frustum20 * x + frustum21 * y + frustum22 * z + frustum23;
			if (d <= -radius) return false;
			
			d = frustum30 * x + frustum31 * y + frustum32 * z + frustum33;
			if (d <= -radius) return false;
			
			d = frustum40 * x + frustum41 * y + frustum42 * z + frustum43;
			if (d <= -radius) return false;
			// Don't test NEAR plane, it's pointless
			return true;
		}
		
		public unsafe void CalcFrustumEquations(ref Matrix4 projection, ref Matrix4 modelView) {
			Matrix4 clipMatrix;
			Matrix4.Mult(out clipMatrix, ref modelView, ref projection);

			float* clip = (float*)&clipMatrix;
			// Extract the numbers for the RIGHT plane
			frustum00 = clip[3] - clip[0];
			frustum01 = clip[7] - clip[4];
			frustum02 = clip[11] - clip[8];
			frustum03 = clip[15] - clip[12];
			Normalise(ref frustum00, ref frustum01, ref frustum02, ref frustum03);

			// Extract the numbers for the LEFT plane
			frustum10 = clip[3] + clip[0];
			frustum11 = clip[7] + clip[4];
			frustum12 = clip[11] + clip[8];
			frustum13 = clip[15] + clip[12];
			Normalise(ref frustum10, ref frustum11, ref frustum12, ref frustum13);

			// Extract the BOTTOM plane
			frustum20 = clip[3] + clip[1];
			frustum21 = clip[7] + clip[5];
			frustum22 = clip[11] + clip[ 9];
			frustum23 = clip[15] + clip[13];
			Normalise(ref frustum20, ref frustum21, ref frustum22, ref frustum23);

			// Extract the TOP plane
			frustum30 = clip[3] - clip[1];
			frustum31 = clip[7] - clip[5];
			frustum32 = clip[11] - clip[9];
			frustum33 = clip[15] - clip[13];
			Normalise(ref frustum30, ref frustum31, ref frustum32, ref frustum33);
			
			// Extract the FAR plane
			frustum40 = clip[3] - clip[2];
			frustum41 = clip[7] - clip[6];
			frustum42 = clip[11] - clip[10];
			frustum43 = clip[15] - clip[14];
			Normalise(ref frustum40, ref frustum41, ref frustum42, ref frustum43);
		}
		
		void Normalise(ref float plane0, ref float plane1, ref float plane2, ref float plane3) {
			float val1 = plane0;
			float val2 = plane1;
			float val3 = plane2;
			float t = (float)Math.Sqrt(val1 * val1 + val2 * val2 + val3 * val3);
			plane0 /= t;
			plane1 /= t;
			plane2 /= t;
			plane3 /= t;
		}
	}
}