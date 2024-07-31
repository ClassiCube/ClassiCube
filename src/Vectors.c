#include "Vectors.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Constants.h"
#include "Core.h"

void Vec3_Lerp(Vec3* result, const Vec3* a, const Vec3* b, float blend) {
	result->x = blend * (b->x - a->x) + a->x;
	result->y = blend * (b->y - a->y) + a->y;
	result->z = blend * (b->z - a->z) + a->z;
}

void Vec3_Normalise(Vec3* v) {
	float scale, lenSquared;
	lenSquared = v->x * v->x + v->y * v->y + v->z * v->z;
	/* handle zero vector */
	if (!lenSquared) return;

	scale = 1.0f / Math_SqrtF(lenSquared);
	v->x  = v->x * scale;
	v->y  = v->y * scale;
	v->z  = v->z * scale;
}

void Vec3_Transform(Vec3* result, const Vec3* a, const struct Matrix* mat) {
	/* a could be pointing to result - therefore can't directly assign X/Y/Z */
	float x = a->x * mat->row1.x + a->y * mat->row2.x + a->z * mat->row3.x + mat->row4.x;
	float y = a->x * mat->row1.y + a->y * mat->row2.y + a->z * mat->row3.y + mat->row4.y;
	float z = a->x * mat->row1.z + a->y * mat->row2.z + a->z * mat->row3.z + mat->row4.z;
	result->x = x; result->y = y; result->z = z;
}

void Vec3_TransformY(Vec3* result, float y, const struct Matrix* mat) {
	result->x = y * mat->row2.x + mat->row4.x;
	result->y = y * mat->row2.y + mat->row4.y;
	result->z = y * mat->row2.z + mat->row4.z;
}

Vec3 Vec3_RotateX(Vec3 v, float angle) {
	float cosA = Math_CosF(angle);
	float sinA = Math_SinF(angle);
	return Vec3_Create3(v.x, cosA * v.y + sinA * v.z, -sinA * v.y + cosA * v.z);
}

Vec3 Vec3_RotateY(Vec3 v, float angle) {
	float cosA = Math_CosF(angle);
	float sinA = Math_SinF(angle);
	return Vec3_Create3(cosA * v.x - sinA * v.z, v.y, sinA * v.x + cosA * v.z);
}

Vec3 Vec3_RotateY3(float x, float y, float z, float angle) {
	float cosA = Math_CosF(angle);
	float sinA = Math_SinF(angle);
	return Vec3_Create3(cosA * x - sinA * z, y, sinA * x + cosA * z);
}

Vec3 Vec3_RotateZ(Vec3 v, float angle) {
	float cosA = Math_CosF(angle);
	float sinA = Math_SinF(angle);
	return Vec3_Create3(cosA * v.x + sinA * v.y, -sinA * v.x + cosA * v.y, v.z);
}


void IVec3_Floor(IVec3* result, const Vec3* a) {
	result->x = Math_Floor(a->x); result->y = Math_Floor(a->y); result->z = Math_Floor(a->z);
}

void IVec3_ToVec3(Vec3* result, const IVec3* a) {
	result->x = (float)a->x; result->y = (float)a->y; result->z = (float)a->z;
}

void IVec3_Min(IVec3* result, const IVec3* a, const IVec3* b) {
	result->x = min(a->x, b->x); result->y = min(a->y, b->y); result->z = min(a->z, b->z);
}

void IVec3_Max(IVec3* result, const IVec3* a, const IVec3* b) {
	result->x = max(a->x, b->x); result->y = max(a->y, b->y); result->z = max(a->z, b->z);
}


Vec3 Vec3_GetDirVector(float yawRad, float pitchRad) {
	float x = -Math_CosF(pitchRad) * -Math_SinF(yawRad);
	float y = -Math_SinF(pitchRad);
	float z = -Math_CosF(pitchRad) * Math_CosF(yawRad);
	return Vec3_Create3(x, y, z);
}

/*void Vec3_GetHeading(Vector3 dir, float* yaw, float* pitch) {
	*pitch = (float)Math_Asin(-dir.y);
	*yaw =   (float)Math_Atan2(dir.x, -dir.z);
}*/


const struct Matrix Matrix_Identity = Matrix_IdentityValue;

/* Transposed, source https://open.gl/transformations */

void Matrix_RotateX(struct Matrix* result, float angle) {
	float cosA = Math_CosF(angle);
	float sinA = Math_SinF(angle);
	*result = Matrix_Identity;

	result->row2.y = cosA;  result->row2.z = sinA;
	result->row3.y = -sinA; result->row3.z = cosA;
}

void Matrix_RotateY(struct Matrix* result, float angle) {
	float cosA = Math_CosF(angle);
	float sinA = Math_SinF(angle);
	*result = Matrix_Identity;

	result->row1.x = cosA; result->row1.z = -sinA;
	result->row3.x = sinA; result->row3.z = cosA;
}

void Matrix_RotateZ(struct Matrix* result, float angle) {
	float cosA = Math_CosF(angle);
	float sinA = Math_SinF(angle);
	*result = Matrix_Identity;

	result->row1.x = cosA;  result->row1.y = sinA;
	result->row2.x = -sinA; result->row2.y = cosA;
}

void Matrix_Translate(struct Matrix* result, float x, float y, float z) {
	*result = Matrix_Identity;
	result->row4.x = x; result->row4.y = y; result->row4.z = z;
}

void Matrix_Scale(struct Matrix* result, float x, float y, float z) {
	*result = Matrix_Identity;
	result->row1.x = x; result->row2.y = y; result->row3.z = z;
}

void Matrix_Mul(struct Matrix* result, const struct Matrix* left, const struct Matrix* right) {
	/* Originally from http://www.edais.co.uk/blog/?p=27 */
	float
		lM11 = left->row1.x, lM12 = left->row1.y, lM13 = left->row1.z, lM14 = left->row1.w,
		lM21 = left->row2.x, lM22 = left->row2.y, lM23 = left->row2.z, lM24 = left->row2.w,
		lM31 = left->row3.x, lM32 = left->row3.y, lM33 = left->row3.z, lM34 = left->row3.w,
		lM41 = left->row4.x, lM42 = left->row4.y, lM43 = left->row4.z, lM44 = left->row4.w,

		rM11 = right->row1.x, rM12 = right->row1.y, rM13 = right->row1.z, rM14 = right->row1.w,
		rM21 = right->row2.x, rM22 = right->row2.y, rM23 = right->row2.z, rM24 = right->row2.w,
		rM31 = right->row3.x, rM32 = right->row3.y, rM33 = right->row3.z, rM34 = right->row3.w,
		rM41 = right->row4.x, rM42 = right->row4.y, rM43 = right->row4.z, rM44 = right->row4.w;

	result->row1.x = (((lM11 * rM11) + (lM12 * rM21)) + (lM13 * rM31)) + (lM14 * rM41);
	result->row1.y = (((lM11 * rM12) + (lM12 * rM22)) + (lM13 * rM32)) + (lM14 * rM42);
	result->row1.z = (((lM11 * rM13) + (lM12 * rM23)) + (lM13 * rM33)) + (lM14 * rM43);
	result->row1.w = (((lM11 * rM14) + (lM12 * rM24)) + (lM13 * rM34)) + (lM14 * rM44);

	result->row2.x = (((lM21 * rM11) + (lM22 * rM21)) + (lM23 * rM31)) + (lM24 * rM41);
	result->row2.y = (((lM21 * rM12) + (lM22 * rM22)) + (lM23 * rM32)) + (lM24 * rM42);
	result->row2.z = (((lM21 * rM13) + (lM22 * rM23)) + (lM23 * rM33)) + (lM24 * rM43);
	result->row2.w = (((lM21 * rM14) + (lM22 * rM24)) + (lM23 * rM34)) + (lM24 * rM44);

	result->row3.x = (((lM31 * rM11) + (lM32 * rM21)) + (lM33 * rM31)) + (lM34 * rM41);
	result->row3.y = (((lM31 * rM12) + (lM32 * rM22)) + (lM33 * rM32)) + (lM34 * rM42);
	result->row3.z = (((lM31 * rM13) + (lM32 * rM23)) + (lM33 * rM33)) + (lM34 * rM43);
	result->row3.w = (((lM31 * rM14) + (lM32 * rM24)) + (lM33 * rM34)) + (lM34 * rM44);

	result->row4.x = (((lM41 * rM11) + (lM42 * rM21)) + (lM43 * rM31)) + (lM44 * rM41);
	result->row4.y = (((lM41 * rM12) + (lM42 * rM22)) + (lM43 * rM32)) + (lM44 * rM42);
	result->row4.z = (((lM41 * rM13) + (lM42 * rM23)) + (lM43 * rM33)) + (lM44 * rM43);
	result->row4.w = (((lM41 * rM14) + (lM42 * rM24)) + (lM43 * rM34)) + (lM44 * rM44);
}

void Matrix_LookRot(struct Matrix* result, Vec3 pos, Vec2 rot) {
	struct Matrix rotX, rotY, trans;
	Matrix_RotateX(&rotX, rot.y);
	Matrix_RotateY(&rotY, rot.x);
	Matrix_Translate(&trans, -pos.x, -pos.y, -pos.z);

	Matrix_Mul(result, &rotY, &rotX);
	Matrix_Mul(result, &trans, result);
}


struct Plane { float a, b, c, d; };
static struct Plane frustumR, frustumL, frustumB, frustumT, frustumF;

static void FrustumCulling_Normalise(struct Plane* plane) {
	float val1 = plane->a, val2 = plane->b, val3 = plane->c;
	float t = Math_SqrtF(val1 * val1 + val2 * val2 + val3 * val3);
	plane->a /= t; plane->b /= t; plane->c /= t; plane->d /= t;
}

cc_bool FrustumCulling_SphereInFrustum(float x, float y, float z, float radius) {
	float d;

	d = frustumR.a * x + frustumR.b * y + frustumR.c * z + frustumR.d;
	if (d <= -radius) return false;

	d = frustumL.a * x + frustumL.b * y + frustumL.c * z + frustumL.d;
	if (d <= -radius) return false;

	d = frustumB.a * x + frustumB.b * y + frustumB.c * z + frustumB.d;
	if (d <= -radius) return false;

	d = frustumT.a * x + frustumT.b * y + frustumT.c * z + frustumT.d;
	if (d <= -radius) return false;

	d = frustumF.a * x + frustumF.b * y + frustumF.c * z + frustumF.d;
	if (d <= -radius) return false;
	/* Don't test NEAR plane, it's pointless */
	return true;
}

void FrustumCulling_CalcFrustumEquations(struct Matrix* clip) {
	/* Extract the RIGHT plane */
	frustumR.a = clip->row1.w - clip->row1.x;
	frustumR.b = clip->row2.w - clip->row2.x;
	frustumR.c = clip->row3.w - clip->row3.x;
	frustumR.d = clip->row4.w - clip->row4.x;
	FrustumCulling_Normalise(&frustumR);

	/* Extract the LEFT plane */
	frustumL.a = clip->row1.w + clip->row1.x;
	frustumL.b = clip->row2.w + clip->row2.x;
	frustumL.c = clip->row3.w + clip->row3.x;
	frustumL.d = clip->row4.w + clip->row4.x;
	FrustumCulling_Normalise(&frustumL);

	/* Extract the BOTTOM plane */
	frustumB.a = clip->row1.w + clip->row1.y;
	frustumB.b = clip->row2.w + clip->row2.y;
	frustumB.c = clip->row3.w + clip->row3.y;
	frustumB.d = clip->row4.w + clip->row4.y;
	FrustumCulling_Normalise(&frustumB);

	/* Extract the TOP plane */
	frustumT.a = clip->row1.w - clip->row1.y;
	frustumT.b = clip->row2.w - clip->row2.y;
	frustumT.c = clip->row3.w - clip->row3.y;
	frustumT.d = clip->row4.w - clip->row4.y;
	FrustumCulling_Normalise(&frustumT);

	/* Extract the FAR plane (Different for each graphics backend) */
#if (CC_GFX_BACKEND == CC_GFX_BACKEND_D3D9) || (CC_GFX_BACKEND == CC_GFX_BACKEND_D3D11)
	/* OpenGL and Direct3D require slightly different behaviour for NEAR clipping planes */
	/* https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf */
	/* (and because reverse Z is used, 'NEAR' plane is actually the 'FAR' clipping plane) */
	frustumF.a = clip->row1.z;
	frustumF.b = clip->row2.z;
	frustumF.c = clip->row3.z;
	frustumF.d = clip->row4.z;
#else
	frustumF.a = clip->row1.w - clip->row1.z;
	frustumF.b = clip->row2.w - clip->row2.z;
	frustumF.c = clip->row3.w - clip->row3.z;
	frustumF.d = clip->row4.w - clip->row4.z;
#endif
	FrustumCulling_Normalise(&frustumF);
}
