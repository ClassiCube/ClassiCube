#include "Vectors.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Constants.h"
#include "Core.h"

void Vec3_Lerp(Vec3* result, const Vec3* a, const Vec3* b, float blend) {
	result->X = blend * (b->X - a->X) + a->X;
	result->Y = blend * (b->Y - a->Y) + a->Y;
	result->Z = blend * (b->Z - a->Z) + a->Z;
}

void Vec3_Normalise(Vec3* v) {
	float scale, lenSquared;
	lenSquared = v->X * v->X + v->Y * v->Y + v->Z * v->Z;
	/* handle zero vector */
	if (!lenSquared) return;

	scale = 1.0f / Math_SqrtF(lenSquared);
	v->X  = v->X * scale;
	v->Y  = v->Y * scale;
	v->Z  = v->Z * scale;
}

void Vec3_Transform(Vec3* result, const Vec3* a, const struct Matrix* mat) {
	/* a could be pointing to result - therefore can't directly assign X/Y/Z */
	float x = a->X * mat->row1.X + a->Y * mat->row2.X + a->Z * mat->row3.X + mat->row4.X;
	float y = a->X * mat->row1.Y + a->Y * mat->row2.Y + a->Z * mat->row3.Y + mat->row4.Y;
	float z = a->X * mat->row1.Z + a->Y * mat->row2.Z + a->Z * mat->row3.Z + mat->row4.Z;
	result->X = x; result->Y = y; result->Z = z;
}

void Vec3_TransformY(Vec3* result, float y, const struct Matrix* mat) {
	result->X = y * mat->row2.X + mat->row4.X;
	result->Y = y * mat->row2.Y + mat->row4.Y;
	result->Z = y * mat->row2.Z + mat->row4.Z;
}

Vec3 Vec3_RotateX(Vec3 v, float angle) {
	float cosA = (float)Math_Cos(angle);
	float sinA = (float)Math_Sin(angle);
	return Vec3_Create3(v.X, cosA * v.Y + sinA * v.Z, -sinA * v.Y + cosA * v.Z);
}

Vec3 Vec3_RotateY(Vec3 v, float angle) {
	float cosA = (float)Math_Cos(angle);
	float sinA = (float)Math_Sin(angle);
	return Vec3_Create3(cosA * v.X - sinA * v.Z, v.Y, sinA * v.X + cosA * v.Z);
}

Vec3 Vec3_RotateY3(float x, float y, float z, float angle) {
	float cosA = (float)Math_Cos(angle);
	float sinA = (float)Math_Sin(angle);
	return Vec3_Create3(cosA * x - sinA * z, y, sinA * x + cosA * z);
}

Vec3 Vec3_RotateZ(Vec3 v, float angle) {
	float cosA = (float)Math_Cos(angle);
	float sinA = (float)Math_Sin(angle);
	return Vec3_Create3(cosA * v.X + sinA * v.Y, -sinA * v.X + cosA * v.Y, v.Z);
}


void IVec3_Floor(IVec3* result, const Vec3* a) {
	result->X = Math_Floor(a->X); result->Y = Math_Floor(a->Y); result->Z = Math_Floor(a->Z);
}

void IVec3_ToVec3(Vec3* result, const IVec3* a) {
	result->X = (float)a->X; result->Y = (float)a->Y; result->Z = (float)a->Z;
}

void IVec3_Min(IVec3* result, const IVec3* a, const IVec3* b) {
	result->X = min(a->X, b->X); result->Y = min(a->Y, b->Y); result->Z = min(a->Z, b->Z);
}

void IVec3_Max(IVec3* result, const IVec3* a, const IVec3* b) {
	result->X = max(a->X, b->X); result->Y = max(a->Y, b->Y); result->Z = max(a->Z, b->Z);
}


Vec3 Vec3_GetDirVector(float yawRad, float pitchRad) {
	float x = -Math_CosF(pitchRad) * -Math_SinF(yawRad);
	float y = -Math_SinF(pitchRad);
	float z = -Math_CosF(pitchRad) * Math_CosF(yawRad);
	return Vec3_Create3(x, y, z);
}

/*void Vec3_GetHeading(Vector3 dir, float* yaw, float* pitch) {
	*pitch = (float)Math_Asin(-dir.Y);
	*yaw =   (float)Math_Atan2(dir.X, -dir.Z);
}*/


const struct Matrix Matrix_Identity = Matrix_IdentityValue;

/* Transposed, source https://open.gl/transformations */

void Matrix_RotateX(struct Matrix* result, float angle) {
	float cosA = (float)Math_Cos(angle);
	float sinA = (float)Math_Sin(angle);
	*result = Matrix_Identity;

	result->row2.Y = cosA;  result->row2.Z = sinA;
	result->row3.Y = -sinA; result->row3.Z = cosA;
}

void Matrix_RotateY(struct Matrix* result, float angle) {
	float cosA = (float)Math_Cos(angle);
	float sinA = (float)Math_Sin(angle);
	*result = Matrix_Identity;

	result->row1.X = cosA; result->row1.Z = -sinA;
	result->row3.X = sinA; result->row3.Z = cosA;
}

void Matrix_RotateZ(struct Matrix* result, float angle) {
	float cosA = (float)Math_Cos(angle);
	float sinA = (float)Math_Sin(angle);
	*result = Matrix_Identity;

	result->row1.X = cosA;  result->row1.Y = sinA;
	result->row2.X = -sinA; result->row2.Y = cosA;
}

void Matrix_Translate(struct Matrix* result, float x, float y, float z) {
	*result = Matrix_Identity;
	result->row4.X = x; result->row4.Y = y; result->row4.Z = z;
}

void Matrix_Scale(struct Matrix* result, float x, float y, float z) {
	*result = Matrix_Identity;
	result->row1.X = x; result->row2.Y = y; result->row3.Z = z;
}

void Matrix_Mul(struct Matrix* result, const struct Matrix* left, const struct Matrix* right) {
	/* Originally from http://www.edais.co.uk/blog/?p=27 */
	float
		lM11 = left->row1.X, lM12 = left->row1.Y, lM13 = left->row1.Z, lM14 = left->row1.W,
		lM21 = left->row2.X, lM22 = left->row2.Y, lM23 = left->row2.Z, lM24 = left->row2.W,
		lM31 = left->row3.X, lM32 = left->row3.Y, lM33 = left->row3.Z, lM34 = left->row3.W,
		lM41 = left->row4.X, lM42 = left->row4.Y, lM43 = left->row4.Z, lM44 = left->row4.W,

		rM11 = right->row1.X, rM12 = right->row1.Y, rM13 = right->row1.Z, rM14 = right->row1.W,
		rM21 = right->row2.X, rM22 = right->row2.Y, rM23 = right->row2.Z, rM24 = right->row2.W,
		rM31 = right->row3.X, rM32 = right->row3.Y, rM33 = right->row3.Z, rM34 = right->row3.W,
		rM41 = right->row4.X, rM42 = right->row4.Y, rM43 = right->row4.Z, rM44 = right->row4.W;

	result->row1.X = (((lM11 * rM11) + (lM12 * rM21)) + (lM13 * rM31)) + (lM14 * rM41);
	result->row1.Y = (((lM11 * rM12) + (lM12 * rM22)) + (lM13 * rM32)) + (lM14 * rM42);
	result->row1.Z = (((lM11 * rM13) + (lM12 * rM23)) + (lM13 * rM33)) + (lM14 * rM43);
	result->row1.W = (((lM11 * rM14) + (lM12 * rM24)) + (lM13 * rM34)) + (lM14 * rM44);

	result->row2.X = (((lM21 * rM11) + (lM22 * rM21)) + (lM23 * rM31)) + (lM24 * rM41);
	result->row2.Y = (((lM21 * rM12) + (lM22 * rM22)) + (lM23 * rM32)) + (lM24 * rM42);
	result->row2.Z = (((lM21 * rM13) + (lM22 * rM23)) + (lM23 * rM33)) + (lM24 * rM43);
	result->row2.W = (((lM21 * rM14) + (lM22 * rM24)) + (lM23 * rM34)) + (lM24 * rM44);

	result->row3.X = (((lM31 * rM11) + (lM32 * rM21)) + (lM33 * rM31)) + (lM34 * rM41);
	result->row3.Y = (((lM31 * rM12) + (lM32 * rM22)) + (lM33 * rM32)) + (lM34 * rM42);
	result->row3.Z = (((lM31 * rM13) + (lM32 * rM23)) + (lM33 * rM33)) + (lM34 * rM43);
	result->row3.W = (((lM31 * rM14) + (lM32 * rM24)) + (lM33 * rM34)) + (lM34 * rM44);

	result->row4.X = (((lM41 * rM11) + (lM42 * rM21)) + (lM43 * rM31)) + (lM44 * rM41);
	result->row4.Y = (((lM41 * rM12) + (lM42 * rM22)) + (lM43 * rM32)) + (lM44 * rM42);
	result->row4.Z = (((lM41 * rM13) + (lM42 * rM23)) + (lM43 * rM33)) + (lM44 * rM43);
	result->row4.W = (((lM41 * rM14) + (lM42 * rM24)) + (lM43 * rM34)) + (lM44 * rM44);
}

void Matrix_LookRot(struct Matrix* result, Vec3 pos, Vec2 rot) {
	struct Matrix rotX, rotY, trans;
	Matrix_RotateX(&rotX, rot.Y);
	Matrix_RotateY(&rotY, rot.X);
	Matrix_Translate(&trans, -pos.X, -pos.Y, -pos.Z);

	Matrix_Mul(result, &rotY, &rotX);
	Matrix_Mul(result, &trans, result);
}

/* TODO: Move to matrix instance instead */
static float
frustum00, frustum01, frustum02, frustum03,
frustum10, frustum11, frustum12, frustum13,
frustum20, frustum21, frustum22, frustum23,
frustum30, frustum31, frustum32, frustum33,
frustum40, frustum41, frustum42, frustum43;

static void FrustumCulling_Normalise(float* plane0, float* plane1, float* plane2, float* plane3) {
	float val1 = *plane0, val2 = *plane1, val3 = *plane2;
	float t = Math_SqrtF(val1 * val1 + val2 * val2 + val3 * val3);
	*plane0 /= t; *plane1 /= t; *plane2 /= t; *plane3 /= t;
}

cc_bool FrustumCulling_SphereInFrustum(float x, float y, float z, float radius) {
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
	/* Don't test NEAR plane, it's pointless */
	return true;
}

void FrustumCulling_CalcFrustumEquations(struct Matrix* projection, struct Matrix* modelView) {
	struct Matrix clipMatrix;
	float* clip = (float*)&clipMatrix;
	Matrix_Mul(&clipMatrix, modelView, projection);

	/* Extract the numbers for the RIGHT plane */
	frustum00 = clip[3]  - clip[0];
	frustum01 = clip[7]  - clip[4];
	frustum02 = clip[11] - clip[8];
	frustum03 = clip[15] - clip[12];
	FrustumCulling_Normalise(&frustum00, &frustum01, &frustum02, &frustum03);

	/* Extract the numbers for the LEFT plane */
	frustum10 = clip[3]  + clip[0];
	frustum11 = clip[7]  + clip[4];
	frustum12 = clip[11] + clip[8];
	frustum13 = clip[15] + clip[12];
	FrustumCulling_Normalise(&frustum10, &frustum11, &frustum12, &frustum13);

	/* Extract the BOTTOM plane */
	frustum20 = clip[3]  + clip[1];
	frustum21 = clip[7]  + clip[5];
	frustum22 = clip[11] + clip[9];
	frustum23 = clip[15] + clip[13];
	FrustumCulling_Normalise(&frustum20, &frustum21, &frustum22, &frustum23);

	/* Extract the TOP plane */
	frustum30 = clip[3]  - clip[1];
	frustum31 = clip[7]  - clip[5];
	frustum32 = clip[11] - clip[9];
	frustum33 = clip[15] - clip[13];
	FrustumCulling_Normalise(&frustum30, &frustum31, &frustum32, &frustum33);

	/* Extract the FAR plane (Different for each graphics backend) */
#if defined CC_BUILD_D3D9 || defined CC_BUILD_D3D11
	/* OpenGL and Direct3D require slightly different behaviour for NEAR clipping planes */
	/* https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf */
	/* (and because reverse Z is used, 'NEAR' plane is actually the 'FAR' clipping plane) */
	frustum40 = clip[2];
	frustum41 = clip[6];
	frustum42 = clip[10];
	frustum43 = clip[14];
#else
	frustum40 = clip[3]  - clip[2];
	frustum41 = clip[7]  - clip[6];
	frustum42 = clip[11] - clip[10];
	frustum43 = clip[15] - clip[14];
#endif
	FrustumCulling_Normalise(&frustum40, &frustum41, &frustum42, &frustum43);
}
