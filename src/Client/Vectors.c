#include "Vectors.h"
#include "ExtMath.h"
#include "Funcs.h"

Vector3 Vector3_Create1(Real32 value) {
	Vector3 v; v.X = value; v.Y = value; v.Z = value; return v;
}

Vector3 Vector3_Create3(Real32 x, Real32 y, Real32 z) {
	Vector3 v; v.X = x; v.Y = y; v.Z = z; return v;
}

Vector3I Vector3I_Create1(Int32 value) {
	Vector3I v; v.X = value; v.Y = value; v.Z = value; return v;
}

Vector3I Vector3I_Create3(Int32 x, Int32 y, Int32 z) {
	Vector3I v; v.X = x; v.Y = y; v.Z = z; return v;
}

Real32 Vector3_Length(Vector3* v) {
	Real32 lenSquared = v->X * v->X + v->Y * v->Y + v->Z * v->Z;
	return Math_Sqrt(lenSquared);
}

Real32 Vector3_LengthSquared(Vector3* v) {
	return v->X * v->X + v->Y * v->Y + v->Z * v->Z;
}

void Vector3_Add(Vector3* result, Vector3* a, Vector3* b) { 
	result->X = a->X + b->X; result->Y = a->Y + b->Y; result->Z = a->Z + b->Z;
}

void Vector3_Add1(Vector3* result, Vector3* a, Real32 b) {
	result->X = a->X + b; result->Y = a->Y + b; result->Z = a->Z + b;
}

void Vector3_Subtract(Vector3* result, Vector3* a, Vector3* b) {
	result->X = a->X - b->X; result->Y = a->Y - b->Y; result->Z = a->Z - b->Z;
}

void Vector3_Multiply1(Vector3* result, Vector3* a, Real32 scale) {
	result->X = a->X * scale; result->Y = a->Y * scale; result->Z = a->Z * scale;
}

void Vector3_Multiply3(Vector3* result, Vector3* a, Vector3* scale) {
	result->X = a->X * scale->X; result->Y = a->Y * scale->Y; result->Z = a->Z * scale->Z;
}

void Vector3_Negate(Vector3* result, Vector3* a) { 
	result->X = -a->X; result->Y = -a->Y; result->Z = -a->Z;
}

void Vector3_Lerp(Vector3* result, Vector3* a, Vector3* b, Real32 blend) {
	result->X = blend * (b->X - a->X) + a->X;
	result->Y = blend * (b->Y - a->Y) + a->Y;
	result->Z = blend * (b->Z - a->Z) + a->Z;
}

Real32 Vector3_Dot(Vector3* a, Vector3* b) {
	return a->X * b->X + a->Y * b->Y + a->Z * b->Z;
}

void Vector3_Cross(Vector3* result, Vector3* a, Vector3* b) {
	/* a or b could be pointing to result - can't directly assign X/Y/Z therefore */
	Real32 x = a->Y * b->Z - a->Z * b->Y;
	Real32 y = a->Z * b->X - a->X * b->Z;
	Real32 z = a->X * b->Y - a->Y * b->X;
	result->X = x; result->Y = y; result->Z = z;
}

void Vector3_Normalize(Vector3* result, Vector3* a) {
	Real32 scale = 1.0f / Vector3_Length(a);
	result->X = a->X * scale;
	result->Y = a->Y * scale;
	result->Z = a->Z * scale;
}

void Vector3_Transform(Vector3* result, Vector3* a, Matrix* mat) {
	/* a could be pointing to result - can't directly assign X/Y/Z therefore */
	Real32 x = a->X * mat->Row0.X + a->Y * mat->Row1.X + a->Z * mat->Row2.X + mat->Row3.X;
	Real32 y = a->X * mat->Row0.Y + a->Y * mat->Row1.Y + a->Z * mat->Row2.Y + mat->Row3.Y;
	Real32 z = a->X * mat->Row0.Z + a->Y * mat->Row1.Z + a->Z * mat->Row2.Z + mat->Row3.Z;
	result->X = x; result->Y = y; result->Z = z;
}

void Vector3_TransformX(Vector3* result, Real32 x, Matrix* mat) {
	result->X = x * mat->Row0.X + mat->Row3.X;
	result->Y = x * mat->Row0.Y + mat->Row3.Y;
	result->Z = x * mat->Row0.Z + mat->Row3.Z;
}

void Vector3_TransformY(Vector3* result, Real32 y, Matrix* mat) {
	result->X = y * mat->Row1.X + mat->Row3.X;
	result->Y = y * mat->Row1.Y + mat->Row3.Y;
	result->Z = y * mat->Row1.Z + mat->Row3.Z;
}

void Vector3_TransformZ(Vector3* result, Real32 z, Matrix* mat) {
	result->X = z * mat->Row2.X + mat->Row3.X;
	result->Y = z * mat->Row2.Y + mat->Row3.Y;
	result->Z = z * mat->Row2.Z + mat->Row3.Z;
}

Vector3 Vector3_RotateX(Vector3 v, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	return Vector3_Create3(v.X, cosA * v.Y + sinA * v.Z, -sinA * v.Y + cosA * v.Z);
}

Vector3 Vector3_RotateY(Vector3 v, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	return Vector3_Create3(cosA * v.X - sinA * v.Z, v.Y, sinA * v.X + cosA * v.Z);
}

Vector3 Vector3_RotateY3(Real32 x, Real32 y, Real32 z, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	return Vector3_Create3(cosA * x - sinA * z, y, sinA * x + cosA * z);
}

Vector3 Vector3_RotateZ(Vector3 v, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	return Vector3_Create3(cosA * v.X + sinA * v.Y, -sinA * v.X + cosA * v.Y, v.Z);
}


#define Vec3_EQ(a, b) a->X == b->X && a->Y == b->Y && a->Z == b->Z
#define Vec3_NE(a, b) a->X != b->X || a->Y != b->Y || a->Z != b->Z

bool Vector3_Equals(Vector3* a, Vector3* b) { return Vec3_EQ(a, b); }
bool Vector3_NotEquals(Vector3* a, Vector3* b) { return Vec3_NE(a, b); }
bool Vector3I_Equals(Vector3I* a, Vector3I* b) { return Vec3_EQ(a, b); }
bool Vector3I_NotEquals(Vector3I* a, Vector3I* b) { return Vec3_NE(a, b); }


void Vector3I_Floor(Vector3I* result, Vector3* a) {
	result->X = Math_Floor(a->X); result->Y = Math_Floor(a->Y); result->Z = Math_Floor(a->Z);
}

void Vector3I_ToVector3(Vector3* result, Vector3I* a) {
	result->X = (Real32)a->X; result->Y = (Real32)a->Y; result->Z = (Real32)a->Z;
}

void Vector3I_Min(Vector3I* result, Vector3I* a, Vector3I* b) {
	result->X = min(a->X, b->X); result->Y = min(a->Y, b->Y); result->Z = min(a->Z, b->Z);
}

void Vector3I_Max(Vector3I* result, Vector3I* a, Vector3I* b) {
	result->X = max(a->X, b->X); result->Y = max(a->Y, b->Y); result->Z = max(a->Z, b->Z);
}


Vector3 Vector3_GetDirVector(Real32 yawRad, Real32 pitchRad) {
	Real32 x = -Math_Cos(pitchRad) * -Math_Sin(yawRad);
	Real32 y = -Math_Sin(pitchRad);
	Real32 z = -Math_Cos(pitchRad) * Math_Cos(yawRad);
	return Vector3_Create3(x, y, z);
}

void Vector3_GetHeading(Vector3 dir, Real32* yaw, Real32* pitch) {
	*pitch = Math_Asin(-dir.Y);
	*yaw = Math_Atan2(dir.X, -dir.Z);
}


Matrix Matrix_Identity = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

/* Transposed, source https://open.gl/transformations */

void Matrix_RotateX(Matrix* result, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	*result = Matrix_Identity;
	result->Row1.Y = cosA; result->Row1.Z = sinA;
	result->Row2.Y = -sinA; result->Row2.Z = cosA;
}

void Matrix_RotateY(Matrix* result, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	*result = Matrix_Identity;
	result->Row1.X = cosA; result->Row1.Z = -sinA;
	result->Row2.X = sinA; result->Row2.Z = cosA;
}

void Matrix_RotateZ(Matrix* result, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	*result = Matrix_Identity;
	result->Row1.X = cosA; result->Row1.Y = sinA;
	result->Row2.X = -sinA; result->Row2.Y = cosA;
}

void Matrix_Translate(Matrix* result, Real32 x, Real32 y, Real32 z) {
	*result = Matrix_Identity;
	result->Row3.X = x; result->Row3.Y = y; result->Row3.Z = z;
}

void Matrix_Scale(Matrix* result, Real32 x, Real32 y, Real32 z) {
	*result = Matrix_Identity;
	result->Row0.X = x; result->Row1.Y = y; result->Row2.Z = z;
}

void Matrix_Mul(Matrix* result, Matrix* left, Matrix* right) {
	/* Originally from http://www.edais.co.uk/blog/?p=27 */
	Real32
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

void Matrix_Orthographic(Matrix* result, Real32 width, Real32 height, Real32 zNear, Real32 zFar) {
	Matrix_OrthographicOffCenter(result, -width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, zNear, zFar);
}

void Matrix_OrthographicOffCenter(Matrix* result, Real32 left, Real32 right, Real32 bottom, Real32 top, Real32 zNear, Real32 zFar) {
	/* Transposed, source https://msdn.microsoft.com/en-us/library/dd373965(v=vs.85).aspx */
	*result = Matrix_Identity;

	result->Row0.X = 2.0f / (right - left);
	result->Row1.Y = 2.0f / (top - bottom);
	result->Row2.Z = -2.0f / (zFar - zNear);

	result->Row3.X = -(right + left) / (right - left);
	result->Row3.Y = -(top + bottom) / (top - bottom);
	result->Row3.Z = -(zFar + zNear) / (zFar - zNear);
}

void Matrix_PerspectiveFieldOfView(Matrix* result, Real32 fovy, Real32 aspect, Real32 zNear, Real32 zFar) {
	Real32 c = zNear * Math_Tan(0.5f * fovy);
	Matrix_PerspectiveOffCenter(result, -c * aspect, c * aspect, -c, c, zNear, zFar);
}

void Matrix_PerspectiveOffCenter(Matrix* result, Real32 left, Real32 right, Real32 bottom, Real32 top, Real32 zNear, Real32 zFar) {
	/* Transposed, source https://msdn.microsoft.com/en-us/library/dd373537(v=vs.85).aspx */
	*result = Matrix_Identity;

	result->Row0.X = (2.0f * zNear) / (right - left);
	result->Row1.Y = (2.0f * zNear) / (top - bottom);
	result->Row3.Z = -(2.0f * zFar * zNear) / (zFar - zNear);

	result->Row2.X = (right + left) / (right - left);
	result->Row2.Y = (top + bottom) / (top - bottom);
	result->Row2.Z = -(zFar + zNear) / (zFar - zNear);
	result->Row2.W = -1.0f;
}

void Matrix_LookAt(Matrix* result, Vector3 eye, Vector3 target, Vector3 up) {
	/* Transposed, source https://msdn.microsoft.com/en-us/library/windows/desktop/bb281711(v=vs.85).aspx */
	Vector3 x, y, z;
	Vector3_Subtract(&z, &eye, &target); Vector3_Normalize(&z, &z);
	Vector3_Cross(&x, &up, &z);          Vector3_Normalize(&x, &x);
	Vector3_Cross(&y, &z, &x);           Vector3_Normalize(&y, &y);

	result->Row0.X = x.X; result->Row0.Y = y.X; result->Row0.Z = z.X; result->Row0.W = 0.0f;
	result->Row1.X = x.Y; result->Row1.Y = y.Y; result->Row1.Z = z.Y; result->Row1.W = 0.0f;
	result->Row2.X = x.Z; result->Row2.Y = y.Z; result->Row2.Z = z.Z; result->Row2.W = 0.0f;

	result->Row3.X = -Vector3_Dot(&x, &eye);
	result->Row3.Y = -Vector3_Dot(&y, &eye);
	result->Row3.Z = -Vector3_Dot(&z, &eye);
	result->Row3.W = 1.0f;
}

Real32
frustum00, frustum01, frustum02, frustum03,
frustum10, frustum11, frustum12, frustum13,
frustum20, frustum21, frustum22, frustum23,
frustum30, frustum31, frustum32, frustum33,
frustum40, frustum41, frustum42, frustum43;

void FrustumCulling_Normalise(Real32* plane0, Real32* plane1, Real32* plane2, Real32* plane3) {
	Real32 val1 = *plane0, val2 = *plane1, val3 = *plane2;
	Real32 t = Math_Sqrt(val1 * val1 + val2 * val2 + val3 * val3);
	*plane0 /= t; *plane1 /= t; *plane2 /= t; *plane3 /= t;
}

bool FrustumCulling_SphereInFrustum(Real32 x, Real32 y, Real32 z, Real32 radius) {
	Real32 d = frustum00 * x + frustum01 * y + frustum02 * z + frustum03;
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

void FrustumCulling_CalcFrustumEquations(Matrix* projection, Matrix* modelView) {
	Matrix clipMatrix;
	Matrix_Mul(&clipMatrix, modelView, projection);

	Real32* clip = (Real32*)&clipMatrix;
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