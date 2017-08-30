#include "Matrix.h"
#include "ExtMath.h"
#include "Vectors.h"

Matrix Matrix_Identity = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

/* Transposed, copied from https://open.gl/transformations */

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
	result->Row3.X = x;
	result->Row3.Y = y;
	result->Row3.Z = z;
}

void Matrix_Scale(Matrix* result, Real32 x, Real32 y, Real32 z) {
	*result = Matrix_Identity;
	result->Row0.X = x;
	result->Row1.Y = y;
	result->Row2.Z = z;
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
	/* Transposed, sourced from https://msdn.microsoft.com/en-us/library/dd373965(v=vs.85).aspx */
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
	/* Transposed, sourced from https://msdn.microsoft.com/en-us/library/dd373537(v=vs.85).aspx */
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
	/* Transposed, sourced from https://msdn.microsoft.com/en-us/library/windows/desktop/bb281711(v=vs.85).aspx */
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