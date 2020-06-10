#include "Vectors.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Constants.h"

void Vec3_Lerp(Vec3* result, const Vec3* a, const Vec3* b, float blend) {
	result->X = blend * (b->X - a->X) + a->X;
	result->Y = blend * (b->Y - a->Y) + a->Y;
	result->Z = blend * (b->Z - a->Z) + a->Z;
}

void Vec3_Normalize(Vec3* result, const Vec3* a) {
	float lenSquared = a->X * a->X + a->Y * a->Y + a->Z * a->Z;
	float scale = 1.0f / Math_SqrtF(lenSquared);
	result->X = a->X * scale;
	result->Y = a->Y * scale;
	result->Z = a->Z * scale;
}

void Vec3_Transform(Vec3* result, const Vec3* a, const struct Matrix* mat) {
	/* a could be pointing to result - can't directly assign X/Y/Z therefore */
	float x = a->X * mat->Row0.X + a->Y * mat->Row1.X + a->Z * mat->Row2.X + mat->Row3.X;
	float y = a->X * mat->Row0.Y + a->Y * mat->Row1.Y + a->Z * mat->Row2.Y + mat->Row3.Y;
	float z = a->X * mat->Row0.Z + a->Y * mat->Row1.Z + a->Z * mat->Row2.Z + mat->Row3.Z;
	result->X = x; result->Y = y; result->Z = z;
}

void Vec3_TransformY(Vec3* result, float y, const struct Matrix* mat) {
	result->X = y * mat->Row1.X + mat->Row3.X;
	result->Y = y * mat->Row1.Y + mat->Row3.Y;
	result->Z = y * mat->Row1.Z + mat->Row3.Z;
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


const struct Matrix Matrix_Identity = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

/* Transposed, source https://open.gl/transformations */

void Matrix_RotateX(struct Matrix* result, float angle) {
	float cosA = (float)Math_Cos(angle);
	float sinA = (float)Math_Sin(angle);
	*result = Matrix_Identity;

	result->Row1.Y = cosA;  result->Row1.Z = sinA;
	result->Row2.Y = -sinA; result->Row2.Z = cosA;
}

void Matrix_RotateY(struct Matrix* result, float angle) {
	float cosA = (float)Math_Cos(angle);
	float sinA = (float)Math_Sin(angle);
	*result = Matrix_Identity;

	result->Row0.X = cosA; result->Row0.Z = -sinA;
	result->Row2.X = sinA; result->Row2.Z = cosA;
}

void Matrix_RotateZ(struct Matrix* result, float angle) {
	float cosA = (float)Math_Cos(angle);
	float sinA = (float)Math_Sin(angle);
	*result = Matrix_Identity;

	result->Row0.X = cosA;  result->Row0.Y = sinA;
	result->Row1.X = -sinA; result->Row1.Y = cosA;
}

void Matrix_Translate(struct Matrix* result, float x, float y, float z) {
	*result = Matrix_Identity;
	result->Row3.X = x; result->Row3.Y = y; result->Row3.Z = z;
}

void Matrix_Scale(struct Matrix* result, float x, float y, float z) {
	*result = Matrix_Identity;
	result->Row0.X = x; result->Row1.Y = y; result->Row2.Z = z;
}

void Matrix_Mul(struct Matrix* result, const struct Matrix* left, const struct Matrix* right) {
	/* Originally from http://www.edais.co.uk/blog/?p=27 */
	float
		lM11 = left->Row0.X, lM12 = left->Row0.Y, lM13 = left->Row0.Z, lM14 = left->Row0.W,
		lM21 = left->Row1.X, lM22 = left->Row1.Y, lM23 = left->Row1.Z, lM24 = left->Row1.W,
		lM31 = left->Row2.X, lM32 = left->Row2.Y, lM33 = left->Row2.Z, lM34 = left->Row2.W,
		lM41 = left->Row3.X, lM42 = left->Row3.Y, lM43 = left->Row3.Z, lM44 = left->Row3.W,

		rM11 = right->Row0.X, rM12 = right->Row0.Y, rM13 = right->Row0.Z, rM14 = right->Row0.W,
		rM21 = right->Row1.X, rM22 = right->Row1.Y, rM23 = right->Row1.Z, rM24 = right->Row1.W,
		rM31 = right->Row2.X, rM32 = right->Row2.Y, rM33 = right->Row2.Z, rM34 = right->Row2.W,
		rM41 = right->Row3.X, rM42 = right->Row3.Y, rM43 = right->Row3.Z, rM44 = right->Row3.W;

	result->Row0.X = (((lM11 * rM11) + (lM12 * rM21)) + (lM13 * rM31)) + (lM14 * rM41);
	result->Row0.Y = (((lM11 * rM12) + (lM12 * rM22)) + (lM13 * rM32)) + (lM14 * rM42);
	result->Row0.Z = (((lM11 * rM13) + (lM12 * rM23)) + (lM13 * rM33)) + (lM14 * rM43);
	result->Row0.W = (((lM11 * rM14) + (lM12 * rM24)) + (lM13 * rM34)) + (lM14 * rM44);

	result->Row1.X = (((lM21 * rM11) + (lM22 * rM21)) + (lM23 * rM31)) + (lM24 * rM41);
	result->Row1.Y = (((lM21 * rM12) + (lM22 * rM22)) + (lM23 * rM32)) + (lM24 * rM42);
	result->Row1.Z = (((lM21 * rM13) + (lM22 * rM23)) + (lM23 * rM33)) + (lM24 * rM43);
	result->Row1.W = (((lM21 * rM14) + (lM22 * rM24)) + (lM23 * rM34)) + (lM24 * rM44);

	result->Row2.X = (((lM31 * rM11) + (lM32 * rM21)) + (lM33 * rM31)) + (lM34 * rM41);
	result->Row2.Y = (((lM31 * rM12) + (lM32 * rM22)) + (lM33 * rM32)) + (lM34 * rM42);
	result->Row2.Z = (((lM31 * rM13) + (lM32 * rM23)) + (lM33 * rM33)) + (lM34 * rM43);
	result->Row2.W = (((lM31 * rM14) + (lM32 * rM24)) + (lM33 * rM34)) + (lM34 * rM44);

	result->Row3.X = (((lM41 * rM11) + (lM42 * rM21)) + (lM43 * rM31)) + (lM44 * rM41);
	result->Row3.Y = (((lM41 * rM12) + (lM42 * rM22)) + (lM43 * rM32)) + (lM44 * rM42);
	result->Row3.Z = (((lM41 * rM13) + (lM42 * rM23)) + (lM43 * rM33)) + (lM44 * rM43);
	result->Row3.W = (((lM41 * rM14) + (lM42 * rM24)) + (lM43 * rM34)) + (lM44 * rM44);
}

void Matrix_Orthographic(struct Matrix* result, float left, float right, float top, float bottom, float zNear, float zFar) {
	/* Transposed, source https://msdn.microsoft.com/en-us/library/dd373965(v=vs.85).aspx */
	*result = Matrix_Identity;

	result->Row0.X =  2.0f / (right - left);
	result->Row1.Y =  2.0f / (top - bottom);
	result->Row2.Z = -2.0f / (zFar - zNear);

	result->Row3.X = -(right + left) / (right - left);
	result->Row3.Y = -(top + bottom) / (top - bottom);
	result->Row3.Z = -(zFar + zNear) / (zFar - zNear);
}

static double Tan_Simple(double x) { return Math_Sin(x) / Math_Cos(x); }
void Matrix_PerspectiveFieldOfView(struct Matrix* result, float fovy, float aspect, float zNear, float zFar) {
	float c = zNear * (float)Tan_Simple(0.5f * fovy);
	Matrix_PerspectiveOffCenter(result, -c * aspect, c * aspect, -c, c, zNear, zFar);
}

void Matrix_PerspectiveOffCenter(struct Matrix* result, float left, float right, float bottom, float top, float zNear, float zFar) {
	/* Transposed, source https://msdn.microsoft.com/en-us/library/dd373537(v=vs.85).aspx */
	*result = Matrix_Identity;
	result->Row3.W = 0.0f;

	result->Row0.X = (2.0f * zNear) / (right - left);
	result->Row1.Y = (2.0f * zNear) / (top - bottom);
	result->Row3.Z = -(2.0f * zFar * zNear) / (zFar - zNear);

	result->Row2.X = (right + left) / (right - left);
	result->Row2.Y = (top + bottom) / (top - bottom);
	result->Row2.Z = -(zFar + zNear) / (zFar - zNear);
	result->Row2.W = -1.0f;
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

	/* Extract the FAR plane */
	frustum40 = clip[3]  - clip[2];
	frustum41 = clip[7]  - clip[6];
	frustum42 = clip[11] - clip[10];
	frustum43 = clip[15] - clip[14];
	FrustumCulling_Normalise(&frustum40, &frustum41, &frustum42, &frustum43);
}
